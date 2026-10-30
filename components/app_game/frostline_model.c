/* Pure game model for Frostline Hold.
 *
 * No drawing, no hardware, no dynamic allocation: everything lives in the
 * caller-provided state struct and the fixed-size pools inside it.
 *
 * SPDX-License-Identifier: Apache-2.0 */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "include/frostline_model.h"
#include "data/frostline_level.h"

#define FL_NO_TARGET 0xFF
#define FL_SLOW_TICKS 45
#define FL_SLOW_FACTOR 0.7f
#define FL_SELL_PERCENT 50

typedef struct {
    int16_t damage;
    int16_t range;
    uint16_t cooldown;
    int16_t aoe;
    uint16_t projectile_speed;
    uint16_t cost;
} fl_tower_stat_t;

/* Ranges are generous because the roads snake in short segments: a tower has
 * to cover more than the stretch it physically stands next to. */
static const fl_tower_stat_t FL_TOWER_STATS[4] = {
    {0, 0, 0, 0, 0, 0},           /* FL_TOWER_NONE */
    {18, 130, 30, 0, 300, 70},    /* FL_TOWER_GUARD */
    {12, 120, 37, 44, 240, 120},  /* FL_TOWER_FROST */
    {30, 170, 60, 52, 180, 160},  /* FL_TOWER_SIEGE */
};

/* ------------------------------------------------------------------ */
/* Lane geometry                                                       */
/* ------------------------------------------------------------------ */

static float g_lane_a_cum[FL_LANE_A_POINTS];
static float g_lane_b_cum[FL_LANE_B_POINTS];
static float g_lane_a_len;
static float g_lane_b_len;
static bool g_geom_ready;

static void fl_geom_init(void)
{
    if (g_geom_ready) {
        return;
    }
    for (int i = 1; i < FL_LANE_A_POINTS; ++i) {
        float dx = (float)(FL_LANE_A[i].x - FL_LANE_A[i - 1].x);
        float dy = (float)(FL_LANE_A[i].y - FL_LANE_A[i - 1].y);
        g_lane_a_cum[i] = g_lane_a_cum[i - 1] + sqrtf(dx * dx + dy * dy);
    }
    g_lane_a_len = g_lane_a_cum[FL_LANE_A_POINTS - 1];
    for (int i = 1; i < FL_LANE_B_POINTS; ++i) {
        float dx = (float)(FL_LANE_B[i].x - FL_LANE_B[i - 1].x);
        float dy = (float)(FL_LANE_B[i].y - FL_LANE_B[i - 1].y);
        g_lane_b_cum[i] = g_lane_b_cum[i - 1] + sqrtf(dx * dx + dy * dy);
    }
    g_lane_b_len = g_lane_b_cum[FL_LANE_B_POINTS - 1];
    g_geom_ready = true;
}

static void fl_lane_pos(uint8_t lane, float dist, float *out_x, float *out_y)
{
    const fl_point_t *points = lane ? FL_LANE_B : FL_LANE_A;
    const float *cum = lane ? g_lane_b_cum : g_lane_a_cum;
    int count = lane ? FL_LANE_B_POINTS : FL_LANE_A_POINTS;

    if (dist >= cum[count - 1]) {
        *out_x = (float)points[count - 1].x;
        *out_y = (float)points[count - 1].y;
        return;
    }
    for (int i = 1; i < count; ++i) {
        if (dist <= cum[i]) {
            float segment = cum[i] - cum[i - 1];
            float t = segment > 0.0f ? (dist - cum[i - 1]) / segment : 0.0f;
            *out_x = (float)points[i - 1].x
                     + ((float)points[i].x - (float)points[i - 1].x) * t;
            *out_y = (float)points[i - 1].y
                     + ((float)points[i].y - (float)points[i - 1].y) * t;
            return;
        }
    }
    *out_x = (float)points[0].x;
    *out_y = (float)points[0].y;
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static uint32_t fl_rand(fl_state_t *state)
{
    uint32_t x = state->seed;
    if (x == 0) {
        x = 0x1D872B41u;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state->seed = x;
    return x;
}

static float fl_apply_armor(float damage, int16_t armor)
{
    float reduced = damage * 60.0f / (60.0f + (float)armor);
    float minimum = damage * 0.15f;
    return reduced < minimum ? minimum : reduced;
}

static bool fl_any_enemy_alive(const fl_state_t *state)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        if (state->enemies[i].alive) {
            return true;
        }
    }
    return false;
}

static int fl_spot_index(int16_t x, int16_t y)
{
    int best = -1;
    float best_dist = (float)FL_BUILD_RADIUS * FL_BUILD_RADIUS;
    for (int i = 0; i < FL_BUILD_SPOT_COUNT; ++i) {
        float dx = (float)(x - FL_BUILD_SPOTS[i].x);
        float dy = (float)(y - FL_BUILD_SPOTS[i].y);
        float d2 = dx * dx + dy * dy;
        if (d2 <= best_dist) {
            best_dist = d2;
            best = i;
        }
    }
    return best;
}

static int fl_tower_at_spot(const fl_state_t *state, int spot)
{
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        if (state->towers[i].active
                && state->towers[i].x == FL_BUILD_SPOTS[spot].x
                && state->towers[i].y == FL_BUILD_SPOTS[spot].y) {
            return i;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Simulation                                                          */
/* ------------------------------------------------------------------ */

static void fl_spawn_enemy(fl_state_t *state, uint8_t kind, uint8_t lane)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *enemy = &state->enemies[i];
        if (enemy->alive) {
            continue;
        }
        const fl_enemy_stat_t *stat = &FL_ENEMY_STATS[kind];
        enemy->kind = kind;
        enemy->max_hp = (float)stat->hp;
        enemy->hp = enemy->max_hp;
        enemy->armor = stat->armor;
        enemy->lane = lane;
        enemy->dist = 0.0f;
        enemy->slow_ticks = 0;
        enemy->alive = 1;
        fl_lane_pos(lane, 0.0f, &enemy->x, &enemy->y);
        return;
    }
}

static void fl_damage_enemy(fl_state_t *state, fl_enemy_t *enemy,
                             int16_t raw_damage, bool apply_slow)
{
    enemy->hp -= fl_apply_armor((float)raw_damage, enemy->armor);
    if (apply_slow) {
        enemy->slow_ticks = FL_SLOW_TICKS;
    }
    if (enemy->hp > 0.0f) {
        return;
    }
    enemy->alive = 0;
    state->kills++;
    state->gold += (uint16_t)FL_ENEMY_STATS[enemy->kind].bounty;
    state->score_kills += (uint32_t)FL_ENEMY_STATS[enemy->kind].score;
    state->score += (uint32_t)FL_ENEMY_STATS[enemy->kind].score;
}

static void fl_spawn_projectile(fl_state_t *state, fl_tower_kind_t kind,
                                int target, int16_t damage, float x, float y,
                                float tx, float ty)
{
    for (int i = 0; i < FL_MAX_PROJECTILES; ++i) {
        fl_projectile_t *shot = &state->projectiles[i];
        if (shot->active) {
            continue;
        }
        shot->kind = (uint8_t)kind;
        shot->target = (uint8_t)target;
        shot->x = x;
        shot->y = y;
        shot->tx = tx;
        shot->ty = ty;
        shot->damage = damage;
        shot->speed = FL_TOWER_STATS[kind].projectile_speed;
        shot->ttl = 120;
        shot->active = 1;
        return;
    }
}

/* Target the enemy closest to the goal, which is the classic "first" rule. */
static int fl_pick_target(const fl_state_t *state, const fl_tower_t *tower,
                           const fl_tower_stat_t *stat)
{
    int best = -1;
    float best_dist = -1.0f;
    float range2 = (float)stat->range * (float)stat->range;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &state->enemies[i];
        if (!enemy->alive) {
            continue;
        }
        float dx = enemy->x - (float)tower->x;
        float dy = enemy->y - (float)tower->y;
        if (dx * dx + dy * dy > range2) {
            continue;
        }
        if (best < 0 || enemy->dist > best_dist) {
            best = i;
            best_dist = enemy->dist;
        }
    }
    return best;
}

static void fl_update_towers(fl_state_t *state)
{
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        fl_tower_t *tower = &state->towers[i];
        if (!tower->active) {
            continue;
        }
        const fl_tower_stat_t *stat = &FL_TOWER_STATS[tower->kind];
        if (tower->cooldown > 0) {
            tower->cooldown--;
            continue;
        }
        int target = fl_pick_target(state, tower, stat);
        if (target < 0) {
            continue;
        }
        const fl_enemy_t *enemy = &state->enemies[target];
        fl_spawn_projectile(state, tower->kind, target, stat->damage,
                            (float)tower->x, (float)tower->y - 20.0f,
                            enemy->x, enemy->y);
        tower->cooldown = stat->cooldown;
    }
}

static void fl_impact_projectile(fl_state_t *state, fl_projectile_t *shot)
{
    const fl_tower_stat_t *stat = &FL_TOWER_STATS[shot->kind];
    bool slow = (shot->kind == FL_TOWER_FROST);
    if (stat->aoe > 0) {
        float aoe2 = (float)stat->aoe * (float)stat->aoe;
        for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
            fl_enemy_t *enemy = &state->enemies[i];
            if (!enemy->alive) {
                continue;
            }
            float dx = enemy->x - shot->tx;
            float dy = enemy->y - shot->ty;
            if (dx * dx + dy * dy > aoe2) {
                continue;
            }
            fl_damage_enemy(state, enemy, shot->damage, slow);
        }
        return;
    }
    if (shot->target < FL_MAX_ENEMIES) {
        fl_enemy_t *enemy = &state->enemies[shot->target];
        if (enemy->alive) {
            fl_damage_enemy(state, enemy, shot->damage, slow);
        }
    }
}

static void fl_update_projectiles(fl_state_t *state)
{
    for (int i = 0; i < FL_MAX_PROJECTILES; ++i) {
        fl_projectile_t *shot = &state->projectiles[i];
        if (!shot->active) {
            continue;
        }
        if (shot->target < FL_MAX_ENEMIES && state->enemies[shot->target].alive) {
            shot->tx = state->enemies[shot->target].x;
            shot->ty = state->enemies[shot->target].y;
        }
        float dx = shot->tx - shot->x;
        float dy = shot->ty - shot->y;
        float distance = sqrtf(dx * dx + dy * dy);
        float step = (float)shot->speed / 30.0f;
        if (distance <= step + 2.0f) {
            fl_impact_projectile(state, shot);
            shot->active = 0;
            continue;
        }
        shot->x += dx / distance * step;
        shot->y += dy / distance * step;
        if (shot->ttl > 0) {
            shot->ttl--;
        } else {
            shot->active = 0;
        }
    }
}

static void fl_update_enemies(fl_state_t *state)
{
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *enemy = &state->enemies[i];
        if (!enemy->alive) {
            continue;
        }
        if (enemy->slow_ticks > 0) {
            enemy->slow_ticks--;
        }
        float speed = (float)FL_ENEMY_STATS[enemy->kind].speed;
        if (enemy->slow_ticks > 0) {
            speed *= FL_SLOW_FACTOR;
        }
        enemy->dist += speed / 30.0f;
        if (enemy->lane == 1 && enemy->dist >= g_lane_b_len) {
            enemy->lane = 0;
            enemy->dist = FL_MERGE_DISTANCE + (enemy->dist - g_lane_b_len);
        }
        fl_lane_pos(enemy->lane, enemy->dist, &enemy->x, &enemy->y);
        if (enemy->dist >= g_lane_a_len) {
            enemy->alive = 0;
            state->leaks++;
            if (state->lives > 0) {
                state->lives--;
            }
            if (state->lives == 0) {
                state->phase = FL_PHASE_LOST;
            }
        }
    }
}

static void fl_update_spawning(fl_state_t *state)
{
    if (state->wave > FL_WAVE_COUNT) {
        return;
    }
    const fl_wave_t *wave = &FL_WAVES[state->wave - 1];
    while (state->batch_index < wave->batch_count) {
        const fl_batch_t *batch = &wave->batches[state->batch_index];
        if (state->spawn_timer > 0) {
            state->spawn_timer--;
            return;
        }
        if (state->batch_spawned < batch->count) {
            fl_spawn_enemy(state, batch->kind, batch->lane);
            state->batch_spawned++;
            state->spawn_timer = batch->interval;
            return;
        }
        state->batch_index++;
        state->batch_spawned = 0;
        state->spawn_timer = 0;
    }
}

static void fl_update_wave_state(fl_state_t *state)
{
    if (state->phase != FL_PHASE_WAVE) {
        return;
    }
    const fl_wave_t *wave = &FL_WAVES[state->wave - 1];
    if (state->batch_index < wave->batch_count || fl_any_enemy_alive(state)) {
        return;
    }
    state->gold += wave->reward;
    state->score_waves += wave->score_bonus;
    state->score += wave->score_bonus;
    state->wave++;
    if (state->wave > FL_WAVE_COUNT) {
        state->score_bonus = (uint32_t)state->lives * FL_SCORE_PER_LIFE;
        if (state->leaks == 0) {
            state->score_bonus += FL_SCORE_FLAWLESS;
        }
        state->score += state->score_bonus;
        state->phase = FL_PHASE_WON;
        return;
    }
    state->phase = FL_PHASE_PREP;
    state->prep_ticks = FL_PREP_TICKS;
}

static void fl_step(fl_state_t *state)
{
    state->tick++;
    if (state->phase == FL_PHASE_PREP) {
        if (state->prep_ticks > 0) {
            state->prep_ticks--;
        }
        if (state->prep_ticks == 0) {
            state->phase = FL_PHASE_WAVE;
            state->batch_index = 0;
            state->batch_spawned = 0;
            state->spawn_timer = 0;
        }
        return;
    }
    if (state->phase != FL_PHASE_WAVE) {
        return;
    }
    fl_update_spawning(state);
    fl_update_towers(state);
    fl_update_projectiles(state);
    fl_update_enemies(state);
    fl_update_wave_state(state);
}

/* ------------------------------------------------------------------ */
/* Public API                                                          */
/* ------------------------------------------------------------------ */

void fl_state_reset(fl_state_t *state, uint32_t seed)
{
    fl_geom_init();
    memset(state, 0, sizeof(*state));
    state->gold = 190;
    state->lives = 20;
    state->wave = 1;
    state->phase = FL_PHASE_PREP;
    state->speed = 1;
    state->selected_kind = FL_TOWER_NONE;
    state->selected_tower = -1;
    state->prep_ticks = FL_FIRST_PREP_TICKS;
    state->seed = seed ? seed : 0x464C5354u;
    (void)fl_rand(state);
}

void fl_state_update(fl_state_t *state)
{
    if (state->paused) {
        return;
    }
    uint8_t steps = state->speed ? state->speed : 1;
    for (uint8_t i = 0; i < steps; ++i) {
        fl_step(state);
    }
}

void fl_toggle_speed(fl_state_t *state)
{
    state->speed = (uint8_t)((state->speed % 3) + 1);
}

void fl_toggle_pause(fl_state_t *state)
{
    state->paused = !state->paused;
}

bool fl_build_tower(fl_state_t *state, int16_t x, int16_t y, fl_tower_kind_t kind)
{
    if (kind <= FL_TOWER_NONE || kind > FL_TOWER_SIEGE) {
        return false;
    }
    const fl_tower_stat_t *stat = &FL_TOWER_STATS[kind];
    if (state->gold < stat->cost) {
        return false;
    }
    int spot = fl_spot_index(x, y);
    if (spot < 0 || fl_tower_at_spot(state, spot) >= 0) {
        return false;
    }
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        fl_tower_t *tower = &state->towers[i];
        if (tower->active) {
            continue;
        }
        tower->kind = kind;
        tower->x = FL_BUILD_SPOTS[spot].x;
        tower->y = FL_BUILD_SPOTS[spot].y;
        tower->level = 1;
        tower->branch = 0;
        tower->cooldown = 0;
        tower->invested = stat->cost;
        tower->active = true;
        state->gold = (uint16_t)(state->gold - stat->cost);
        return true;
    }
    return false;
}

bool fl_sell_tower(fl_state_t *state, int index)
{
    if (index < 0 || index >= FL_MAX_TOWERS || !state->towers[index].active) {
        return false;
    }
    uint32_t refund = state->towers[index].invested * FL_SELL_PERCENT / 100u;
    state->gold = (uint16_t)(state->gold + refund);
    state->towers[index].active = false;
    return true;
}

void fl_set_pointer(fl_state_t *state, int32_t x, int32_t y, bool down)
{
    bool pressed = down && !state->pointer_down_prev;
    state->pointer_down_prev = down;
    if (!pressed) {
        return;
    }

    if (state->phase == FL_PHASE_WON || state->phase == FL_PHASE_LOST) {
        /* The end panel covers the screen, so any tap starts a new battle. */
        uint32_t seed = state->seed ^ 0x9E3779B9u;
        fl_state_reset(state, seed);
        return;
    }

    if (y < FL_HUD_HEIGHT) {
        if (x >= FL_SPEED_BTN_X && x < FL_SPEED_BTN_X + FL_SPEED_BTN_W
                && y >= FL_BTN_Y && y < FL_BTN_Y + FL_BTN_H) {
            fl_toggle_speed(state);
        } else if (x >= FL_PAUSE_BTN_X && x < FL_PAUSE_BTN_X + FL_PAUSE_BTN_W
                   && y >= FL_BTN_Y && y < FL_BTN_Y + FL_BTN_H) {
            fl_toggle_pause(state);
        }
        return;
    }

    if (y >= FL_BAR_TOP) {
        for (int i = 0; i < FL_TOWER_CARD_COUNT; ++i) {
            int card_x = 8 + i * FL_TOWER_CARD_GAP;
            if (x >= card_x && x < card_x + FL_TOWER_CARD_W
                    && y >= FL_TOWER_CARD_Y && y < FL_TOWER_CARD_Y + FL_TOWER_CARD_H) {
                state->selected_kind = (uint8_t)(FL_TOWER_GUARD + i);
                state->selected_tower = -1;
                return;
            }
        }
        return;
    }

    /* Battlefield: an existing tower wins over an empty build spot. */
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        const fl_tower_t *tower = &state->towers[i];
        if (!tower->active) {
            continue;
        }
        float dx = (float)x - (float)tower->x;
        float dy = (float)y - ((float)tower->y - 20.0f);
        if (dx * dx + dy * dy <= 30.0f * 30.0f) {
            state->selected_tower = (int8_t)i;
            state->selected_kind = FL_TOWER_NONE;
            return;
        }
    }

    if (state->selected_kind != FL_TOWER_NONE) {
        fl_build_tower(state, (int16_t)x, (int16_t)y,
                       (fl_tower_kind_t)state->selected_kind);
    }
}

const char *fl_phase_name(const fl_state_t *state)
{
    switch (state->phase) {
    case FL_PHASE_PREP: return "prep";
    case FL_PHASE_WAVE: return "wave";
    case FL_PHASE_WON: return "won";
    case FL_PHASE_LOST: return "lost";
    default: return "unknown";
    }
}

uint32_t fl_state_hash(const fl_state_t *state)
{
    uint32_t hash = 2166136261u;
#define FL_MIX(value) do { \
        hash ^= (uint32_t)(value); \
        hash *= 16777619u; \
    } while (0)
    FL_MIX(state->tick);
    FL_MIX(state->gold);
    FL_MIX(state->lives);
    FL_MIX(state->score);
    FL_MIX(state->score_kills);
    FL_MIX(state->score_waves);
    FL_MIX(state->score_bonus);
    FL_MIX(state->leaks);
    FL_MIX(state->wave);
    FL_MIX(state->phase);
    FL_MIX(state->kills);
    FL_MIX(state->speed);
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        if (!state->towers[i].active) {
            continue;
        }
        FL_MIX(state->towers[i].kind);
        FL_MIX(state->towers[i].x);
        FL_MIX(state->towers[i].y);
        FL_MIX(state->towers[i].level);
    }
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        if (!state->enemies[i].alive) {
            continue;
        }
        FL_MIX((uint32_t)(state->enemies[i].hp * 100.0f));
        FL_MIX((uint32_t)(state->enemies[i].dist * 100.0f));
        FL_MIX(state->enemies[i].kind);
    }
#undef FL_MIX
    return hash;
}

int fl_state_json(const fl_state_t *state, char *output, size_t capacity)
{
    if (!output || !capacity) {
        return -1;
    }
    int size = snprintf(output, capacity,
                        "{\"phase\":\"%s\",\"wave\":%u,\"gold\":%u,\"lives\":%u,"
                        "\"score\":%lu,\"score_kills\":%lu,\"score_waves\":%lu,"
                        "\"score_bonus\":%lu,\"leaks\":%u,"
                        "\"kills\":%lu,\"tick\":%lu,\"speed\":%u,\"state_hash\":\"%08lx\"}",
                        fl_phase_name(state), (unsigned)state->wave,
                        (unsigned)state->gold, (unsigned)state->lives,
                        (unsigned long)state->score,
                        (unsigned long)state->score_kills,
                        (unsigned long)state->score_waves,
                        (unsigned long)state->score_bonus,
                        (unsigned)state->leaks,
                        (unsigned long)state->kills, (unsigned long)state->tick,
                        (unsigned)state->speed,
                        (unsigned long)fl_state_hash(state));
    return (size < 0 || (size_t)size >= capacity) ? -1 : size;
}

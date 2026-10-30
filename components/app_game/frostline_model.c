/* Pure game model for Frostline Hold.
 *
 * No drawing, no hardware, no dynamic allocation: everything lives in the
 * caller-provided state struct and the fixed-size pools inside it.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "include/frostline_model.h"
#include "data/frostline_level.h"

#define FL_NO_TARGET 0xFF
#define FL_SLOW_TICKS 45
#define FL_SLOW_PERCENT 70
#define FL_BURN_TICKS 90
#define FL_STUN_TICKS 20
#define FL_CAST_TICKS 45      /* how often a caster acts */
#define FL_HEAL_AMOUNT 16     /* health the shaman restores */
#define FL_BANNER_ARMOR 10    /* armour the banner grants */
#define FL_BANNER_TICKS 120   /* how long the banner's gift lasts */
#define FL_SAP_LIVES 3        /* lives a sap takes out of the gate */
#define FL_SELL_PERCENT 50

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

/* Armour is a flat cut, not a percentage. A percentage made big hits
 * worthless against armour, which killed the "one huge bolt" branch; with a
 * flat cut a heavy shot is the right answer to heavy armour, and only the
 * piercing upgrade ignores armour altogether. */
static float fl_apply_armor(float damage, int16_t armor)
{
    float reduced = damage - (float)armor;
    float minimum = damage * 0.25f;
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
        /* Later waves are tougher without needing a new roster for each. */
        uint16_t hp_mult = 100;
        uint16_t armor_add = 0;
        if (state->wave >= 1 && state->wave <= FL_WAVE_COUNT) {
            hp_mult = FL_WAVES[state->wave - 1].hp_mult;
            armor_add = FL_WAVES[state->wave - 1].armor_add;
        }
        enemy->kind = kind;
        enemy->max_hp = (float)stat->hp * (float)hp_mult / 100.0f;
        enemy->hp = enemy->max_hp;
        enemy->armor = (int16_t)(stat->armor + armor_add);
        enemy->behaviour = stat->behaviour;
        enemy->cast_timer = 30;
        enemy->buff_ticks = 0;
        enemy->armor_bonus = 0;
        enemy->lane = lane;
        enemy->dist = 0.0f;
        enemy->slow_ticks = 0;
        enemy->alive = 1;
        fl_lane_pos(lane, 0.0f, &enemy->x, &enemy->y);
        return;
    }
}

/* Everything a single hit carries: the damage, the blast radius and the
 * effects the shooting tower's upgrades gave it. */
typedef struct {
    int16_t damage;
    int16_t aoe;
    uint16_t slow_percent;
    uint8_t flags;
    bool slow;
} fl_shot_t;

static void fl_damage_enemy(fl_state_t *state, fl_enemy_t *enemy,
                             const fl_shot_t *shot)
{
    int16_t raw_damage = shot->damage;
    float damage = (float)raw_damage;
    if (!(shot->flags & FL_SHOT_PIERCE)) {
        damage = fl_apply_armor(damage,
                                (int16_t)(enemy->armor + enemy->armor_bonus));
    }
    enemy->hp -= damage;
    if (shot->slow && enemy->behaviour != FL_BEHAVIOUR_TITAN) {
        enemy->slow_ticks = FL_SLOW_TICKS;
        enemy->slow_percent = shot->slow_percent;
    }
    if (shot->flags & FL_SHOT_BURN) {
        enemy->burn_ticks = FL_BURN_TICKS;
        enemy->burn_damage = (uint16_t)(raw_damage / 3);
    }
    if (shot->flags & FL_SHOT_STUN
            && enemy->behaviour != FL_BEHAVIOUR_TITAN) {
        enemy->stun_ticks = FL_STUN_TICKS;
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
                                float tx, float ty, uint8_t flags,
                                uint16_t speed, int16_t aoe,
                                uint16_t slow_percent)
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
        shot->speed = speed;
        shot->flags = flags;
        shot->aoe = aoe;
        shot->slow_percent = slow_percent;
        shot->ttl = 120;
        shot->active = 1;
        return;
    }
}

/* Target the enemy closest to the goal, which is the classic "first" rule. */
static int fl_pick_target(const fl_state_t *state, const fl_tower_t *tower,
                           int16_t range)
{
    int best = -1;
    float best_dist = -1.0f;
    float range2 = (float)range * (float)range;
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

/* The second target of a repeater: the next one down the road. */
static int fl_pick_second_target(const fl_state_t *state,
                                  const fl_tower_t *tower, int16_t range,
                                  int first)
{
    int best = -1;
    float best_dist = -1.0f;
    float range2 = (float)range * (float)range;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &state->enemies[i];
        if (!enemy->alive || i == first) {
            continue;
        }
        float dx = enemy->x - (float)tower->x;
        float dy = enemy->y - (float)tower->y;
        if (dx * dx + dy * dy > range2) {
            continue;
        }
        if (best < 0 || enemy->dist < best_dist) {
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
        fl_tower_stats_t stat = fl_tower_stats(tower);
        if (tower->cooldown > 0) {
            tower->cooldown--;
            continue;
        }
        int target = fl_pick_target(state, tower, stat.range);
        if (target < 0) {
            continue;
        }
        const fl_enemy_t *enemy = &state->enemies[target];
        uint8_t flags = 0;
        if (stat.pierce) {
            flags |= FL_SHOT_PIERCE;
        }
        if (stat.burn) {
            flags |= FL_SHOT_BURN;
        }
        if (stat.stun) {
            flags |= FL_SHOT_STUN;
        }
        fl_spawn_projectile(state, tower->kind, target, stat.damage,
                            (float)tower->x, (float)tower->y - 20.0f,
                            enemy->x, enemy->y, flags, stat.projectile_speed,
                            stat.aoe, stat.slow_percent);
        if (stat.double_shot) {
            int second = fl_pick_second_target(state, tower, stat.range, target);
            if (second >= 0) {
                const fl_enemy_t *other = &state->enemies[second];
                fl_spawn_projectile(state, tower->kind, second, stat.damage,
                                    (float)tower->x, (float)tower->y - 20.0f,
                                    other->x, other->y, flags,
                                    stat.projectile_speed, stat.aoe,
                                    stat.slow_percent);
            }
        }
        tower->cooldown = stat.cooldown;
    }
}

static void fl_impact_projectile(fl_state_t *state, fl_projectile_t *shot)
{
    fl_shot_t hit;
    hit.damage = shot->damage;
    hit.aoe = shot->aoe;
    hit.flags = shot->flags;
    hit.slow = (shot->kind == FL_TOWER_FROST);
    hit.slow_percent = shot->slow_percent ? shot->slow_percent : FL_SLOW_PERCENT;
    if (hit.aoe > 0) {
        float aoe2 = (float)hit.aoe * (float)hit.aoe;
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
            fl_damage_enemy(state, enemy, &hit);
        }
        return;
    }
    if (shot->target < FL_MAX_ENEMIES) {
        fl_enemy_t *enemy = &state->enemies[shot->target];
        if (enemy->alive) {
            fl_damage_enemy(state, enemy, &hit);
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

/* The shaman mends the most wounded ally beside it. */
static void fl_heal_allies(fl_state_t *state, fl_enemy_t *caster)
{
    const float radius = 95.0f;
    int best = -1;
    float worst = 1.0f;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *other = &state->enemies[i];
        if (!other->alive || other == caster) {
            continue;
        }
        float dx = other->x - caster->x;
        float dy = other->y - caster->y;
        if (dx * dx + dy * dy > radius * radius) {
            continue;
        }
        if (other->max_hp <= 0.0f) {
            continue;
        }
        float ratio = other->hp / other->max_hp;
        if (ratio < worst) {
            worst = ratio;
            best = i;
        }
    }
    if (best >= 0) {
        fl_enemy_t *other = &state->enemies[best];
        other->hp += (float)FL_HEAL_AMOUNT;
        if (other->hp > other->max_hp) {
            other->hp = other->max_hp;
        }
    }
}

/* The banner hardens everyone around it for a while. */
static void fl_banner_allies(fl_state_t *state, fl_enemy_t *caster)
{
    const float radius = 115.0f;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *other = &state->enemies[i];
        if (!other->alive) {
            continue;
        }
        float dx = other->x - caster->x;
        float dy = other->y - caster->y;
        if (dx * dx + dy * dy > radius * radius) {
            continue;
        }
        other->armor_bonus = FL_BANNER_ARMOR;
        other->buff_ticks = FL_BANNER_TICKS;
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
        if (enemy->stun_ticks > 0) {
            enemy->stun_ticks--;
        }
        if (enemy->buff_ticks > 0) {
            enemy->buff_ticks--;
            if (enemy->buff_ticks == 0) {
                enemy->armor_bonus = 0;
            }
        }
        if (enemy->behaviour == FL_BEHAVIOUR_HEALER
                || enemy->behaviour == FL_BEHAVIOUR_BANNER) {
            if (enemy->cast_timer > 0) {
                enemy->cast_timer--;
            } else {
                enemy->cast_timer = FL_CAST_TICKS;
                if (enemy->behaviour == FL_BEHAVIOUR_HEALER) {
                    fl_heal_allies(state, enemy);
                } else {
                    fl_banner_allies(state, enemy);
                }
            }
        }
        if (enemy->burn_ticks > 0) {
            enemy->burn_ticks--;
            if (enemy->burn_ticks % 30 == 0) {
                fl_shot_t burn;
                burn.damage = (int16_t)enemy->burn_damage;
                burn.aoe = 0;
                burn.flags = FL_SHOT_PIERCE;
                burn.slow = false;
                burn.slow_percent = FL_SLOW_PERCENT;
                fl_damage_enemy(state, enemy, &burn);
                if (!enemy->alive) {
                    continue;
                }
            }
        }
        if (enemy->stun_ticks > 0) {
            continue;
        }
        float speed = (float)FL_ENEMY_STATS[enemy->kind].speed;
        if (enemy->slow_ticks > 0) {
            speed *= (float)enemy->slow_percent / 100.0f;
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
            uint8_t cost = (enemy->behaviour == FL_BEHAVIOUR_SAP)
                           ? FL_SAP_LIVES : 1;
            for (uint8_t i = 0; i < cost && state->lives > 0; ++i) {
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
/* Tower stats                                                         */
/* ------------------------------------------------------------------ */

fl_tower_stats_t fl_tower_stats(const fl_tower_t *tower)
{
    fl_tower_stats_t out;
    out.damage = 0;
    out.range = 0;
    out.cooldown = 1;
    out.aoe = 0;
    out.projectile_speed = 0;
    out.slow_percent = FL_SLOW_PERCENT;
    out.pierce = 0;
    out.burn = 0;
    out.stun = 0;
    out.double_shot = 0;
    if (!tower || tower->kind <= FL_TOWER_NONE || tower->kind > FL_TOWER_SIEGE) {
        return out;
    }
    uint8_t level_index = (uint8_t)(tower->level ? tower->level - 1 : 0);
    if (level_index >= FL_TOWER_LEVEL_COUNT) {
        level_index = FL_TOWER_LEVEL_COUNT - 1;
    }
    const fl_tower_level_t *level = &FL_TOWER_LEVELS[tower->kind][level_index];
    out.damage = level->damage;
    out.range = level->range;
    out.cooldown = level->cooldown ? level->cooldown : 1;
    out.aoe = level->aoe;
    out.projectile_speed = level->projectile_speed;

    /* The chosen upgrades stack on top of the level. */
    for (int stage = 0; stage < FL_TOWER_STAGE_COUNT; ++stage) {
        for (int option = 0; option < FL_TOWER_OPTION_COUNT; ++option) {
            int bit = stage * FL_TOWER_OPTION_COUNT + option;
            if (!(tower->mod_mask & (1u << bit))) {
                continue;
            }
            const fl_tower_mod_t *mod =
                &FL_TOWER_MODS[tower->kind][stage][option];
            if (mod->damage_mult) {
                out.damage = (int16_t)(out.damage * mod->damage_mult / 100);
            }
            if (mod->range_delta) {
                out.range = (int16_t)(out.range + mod->range_delta);
            }
            if (mod->cooldown_mult) {
                out.cooldown = (uint16_t)(out.cooldown * mod->cooldown_mult / 100);
            }
            if (mod->aoe_delta) {
                out.aoe = (int16_t)(out.aoe + mod->aoe_delta);
            }
            if (mod->slow_mult) {
                out.slow_percent =
                    (uint16_t)(out.slow_percent * mod->slow_mult / 100);
            }
            out.pierce |= mod->pierce;
            out.burn |= mod->burn;
            out.stun |= mod->stun;
            out.double_shot |= mod->double_shot;
        }
    }
    if (out.range < 20) {
        out.range = 20;
    }
    if (out.cooldown < 4) {
        out.cooldown = 4;
    }
    return out;
}

uint16_t fl_upgrade_cost(const fl_state_t *state, int index)
{
    if (!state || index < 0 || index >= FL_MAX_TOWERS) {
        return 0;
    }
    const fl_tower_t *tower = &state->towers[index];
    if (!tower->active || tower->level >= FL_TOWER_LEVEL_COUNT) {
        return 0;
    }
    return FL_TOWER_LEVELS[tower->kind][tower->level].upgrade;
}

bool fl_upgrade_tower(fl_state_t *state, int index, int option)
{
    if (!state || index < 0 || index >= FL_MAX_TOWERS) {
        return false;
    }
    fl_tower_t *tower = &state->towers[index];
    if (!tower->active || tower->level >= FL_TOWER_LEVEL_COUNT) {
        return false;
    }
    if (option < 0 || option >= FL_TOWER_OPTION_COUNT) {
        return false;
    }
    uint16_t cost = FL_TOWER_LEVELS[tower->kind][tower->level].upgrade;
    if (state->gold < cost) {
        return false;
    }
    /* One bit per stage records which option was taken. */
    int stage = tower->level - 1;
    int bit = stage * FL_TOWER_OPTION_COUNT + option;
    tower->mod_mask = (uint8_t)(tower->mod_mask | (1u << bit));
    tower->level++;
    tower->invested += cost;
    tower->cooldown = 0;
    state->gold = (uint16_t)(state->gold - cost);
    return true;
}

int fl_tower_option(const fl_tower_t *tower, int stage)
{
    if (!tower || stage < 0 || stage >= FL_TOWER_STAGE_COUNT) {
        return -1;
    }
    for (int option = 0; option < FL_TOWER_OPTION_COUNT; ++option) {
        int bit = stage * FL_TOWER_OPTION_COUNT + option;
        if (tower->mod_mask & (1u << bit)) {
            return option;
        }
    }
    return -1;
}

const char *fl_upgrade_name(fl_tower_kind_t kind, int stage, int option)
{
    if (kind <= FL_TOWER_NONE || kind > FL_TOWER_SIEGE) {
        return "";
    }
    if (stage < 0 || stage >= FL_TOWER_STAGE_COUNT) {
        return "";
    }
    if (option < 0 || option >= FL_TOWER_OPTION_COUNT) {
        return "";
    }
    return FL_TOWER_MOD_NAMES[kind][stage][option];
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
    const fl_tower_level_t *stat = &FL_TOWER_LEVELS[kind][0];
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
        tower->mod_mask = 0;
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

    /* The upgrade panel sits over the build bar while a tower is picked. */
    if (state->selected_tower >= 0 && y >= FL_PANEL_TOP) {
        for (int i = 0; i < FL_TOWER_OPTION_COUNT; ++i) {
            int card_x = FL_UPGRADE_CARD_X0
                         + i * (FL_UPGRADE_CARD_W + FL_UPGRADE_CARD_GAP);
            if (x >= card_x && x < card_x + FL_UPGRADE_CARD_W
                    && y >= FL_UPGRADE_CARD_Y
                    && y < FL_UPGRADE_CARD_Y + FL_UPGRADE_CARD_H) {
                fl_upgrade_tower(state, state->selected_tower, i);
                return;
            }
        }
        /* Tapping the panel but not a card sells the tower. */
        if (x >= FL_UPGRADE_CARD_X0
                && x < FL_UPGRADE_CARD_X0
                     + FL_TOWER_OPTION_COUNT * FL_UPGRADE_CARD_W
                     + (FL_TOWER_OPTION_COUNT - 1) * FL_UPGRADE_CARD_GAP
                && y < FL_UPGRADE_CARD_Y) {
            fl_sell_tower(state, state->selected_tower);
            state->selected_tower = -1;
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
        return;
    }
    state->selected_tower = -1;
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

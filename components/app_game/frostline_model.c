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
#define FL_HERO_HIT_DAMAGE 6  /* what one enemy strike costs the hero */
#define FL_SELL_PERCENT 50

/* The newer towers. */
#define FL_CHAIN_RADIUS 64        /* how far lightning leaps to the next enemy */
#define FL_CHAIN_FALLOFF 80       /* percent of the bite left after a leap */
#define FL_HEX_TICKS 90           /* how long a hex strips the armour */
#define FL_ZONE_PULSE_TICKS 10    /* a burning patch bites this often */
#define FL_ZONE_SLOW_PERCENT 60   /* tar: speed percent inside the fire */
#define FL_SOLDIER_SPEED 48.0f    /* px/s, walking out to the road */
#define FL_SOLDIER_HOLD 2         /* enemies one soldier holds at once */
#define FL_SOLDIER_BLOCK_RADIUS 14 /* how close an enemy comes to be held */
#define FL_SOLDIER_REACH 22       /* how far a soldier's blow reaches */
#define FL_SOLDIER_RESPAWN 300    /* ticks before a fallen soldier returns */
#define FL_SOLDIER_MAX 4          /* the largest garrison a hall keeps */
#define FL_ENEMY_HIT_TICKS 20     /* how often a held enemy strikes back */

/* The newer enemies. */
#define FL_CRACKED_SPEED 150      /* percent: a golem out of its shell hurries */
#define FL_SPLIT_COUNT 3          /* slimelets a slime bursts into */
#define FL_SPLIT_SPACING 9.0f     /* px between them along the road */
#define FL_SPLIT_WARD_TICKS 12    /* the slimelets' moment of grace */
#define FL_BURROW_UP_TICKS 105    /* a burrower above the road */
#define FL_BURROW_DOWN_TICKS 66   /* and under it */
#define FL_BURROW_SPEED 170       /* percent, tunnelling */
#define FL_WYRM_UP_TICKS 210
#define FL_WYRM_DOWN_TICKS 96
#define FL_WYRM_SPEED 150
#define FL_WYRM_BROOD 3           /* skitters it brings up with it */
#define FL_KING_FREEZE_TICKS 210  /* how often the king freezes a tower */
#define FL_KING_FREEZE_RANGE 170
#define FL_KING_RETRY_TICKS 30    /* no tower in reach: look again soon */
#define FL_FREEZE_TICKS 120       /* how long a tower stays encased */
#define FL_KING_WARD_TICKS 90     /* the ward at each third of its health */
#define FL_KING_GUARD 3           /* wolves it calls when the ward goes up */
#define FL_KING_WARDS 2
#define FL_BOSS_LIVES 10          /* lives a boss takes out of the gate */

_Static_assert(FL_LEVEL_COUNT <= FL_MAX_LEVELS,
               "the progress record has fewer slots than the campaign");
_Static_assert(FL_MAX_HERO_POSTS <= FL_MAX_HEROES,
               "a level has more hero posts than the hero pool");

/* ------------------------------------------------------------------ */
/* Lane geometry                                                       */
/* ------------------------------------------------------------------ */

/* Distance along each road up to every point, per level and lane. The
 * tables are filled once; the level data itself is constant. */
static float g_lane_cum[FL_LEVEL_COUNT][2][FL_MAX_LANE_POINTS];
static float g_lane_len[FL_LEVEL_COUNT][2];
static bool g_geom_ready;

static const fl_level_t *fl_level_of(const fl_state_t *state)
{
    return &FL_LEVELS[state->level < FL_LEVEL_COUNT ? state->level : 0];
}

static void fl_lane_points(const fl_level_t *level, uint8_t lane,
                           const fl_point_t **points, int *count)
{
    *points = lane ? level->lane_b : level->lane_a;
    *count = lane ? level->lane_b_count : level->lane_a_count;
}

static void fl_geom_init(void)
{
    if (g_geom_ready) {
        return;
    }
    for (int l = 0; l < FL_LEVEL_COUNT; ++l) {
        for (uint8_t lane = 0; lane < 2; ++lane) {
            const fl_point_t *points;
            int count;
            fl_lane_points(&FL_LEVELS[l], lane, &points, &count);
            float *cum = g_lane_cum[l][lane];
            cum[0] = 0.0f;
            for (int i = 1; i < count; ++i) {
                float dx = (float)(points[i].x - points[i - 1].x);
                float dy = (float)(points[i].y - points[i - 1].y);
                cum[i] = cum[i - 1] + sqrtf(dx * dx + dy * dy);
            }
            g_lane_len[l][lane] = count > 0 ? cum[count - 1] : 0.0f;
        }
    }
    g_geom_ready = true;
}

static float fl_lane_length(const fl_state_t *state, uint8_t lane)
{
    return g_lane_len[state->level][lane ? 1 : 0];
}

static void fl_lane_pos(const fl_state_t *state, uint8_t lane, float dist,
                        float *out_x, float *out_y)
{
    const fl_point_t *points;
    int count;
    fl_lane_points(fl_level_of(state), lane, &points, &count);
    const float *cum = g_lane_cum[state->level][lane ? 1 : 0];

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

/* Raise one event bit. The runtime reads the accumulated set once per frame
 * and turns it into sound, haptics and effects. */
static void fl_raise(fl_state_t *state, uint32_t bit)
{
    if (!(state->events & bit)) {
        state->event_serial++;
    }
    state->events |= bit;
}

/* A won run keeps the best stars and score it earned on this level, so the
 * next level opens on the campaign map at once. */
static void fl_record_progress(fl_state_t *state)
{
    uint8_t level = state->level;
    int stars = fl_star_rating(state);
    if (stars > state->level_stars[level]) {
        state->level_stars[level] = (uint8_t)stars;
    }
    if (state->score > state->level_best[level]) {
        state->level_best[level] = state->score;
    }
}

static int fl_spot_index(const fl_state_t *state, int16_t x, int16_t y)
{
    const fl_level_t *level = fl_level_of(state);
    int best = -1;
    float best_dist = (float)FL_BUILD_RADIUS * FL_BUILD_RADIUS;
    for (int i = 0; i < level->spot_count; ++i) {
        float dx = (float)(x - level->spots[i].x);
        float dy = (float)(y - level->spots[i].y);
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
    const fl_point_t *at = &fl_level_of(state)->spots[spot];
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        if (state->towers[i].active
                && state->towers[i].x == at->x
                && state->towers[i].y == at->y) {
            return i;
        }
    }
    return -1;
}

/* ------------------------------------------------------------------ */
/* Simulation                                                          */
/* ------------------------------------------------------------------ */

bool fl_enemy_flies(const fl_enemy_t *enemy)
{
    return enemy->behaviour == FL_BEHAVIOUR_FLYER;
}

/* Under the road an enemy cannot be aimed at, blasted or held. */
static bool fl_targetable(const fl_enemy_t *enemy)
{
    return enemy->alive && !enemy->burrowed;
}

/* On the road, within reach of soldiers, heroes, blasts and fire. */
static bool fl_grounded(const fl_enemy_t *enemy)
{
    return fl_targetable(enemy) && !fl_enemy_flies(enemy);
}

/* The titan and the bosses shrug off slows and stuns. */
static bool fl_resists_control(const fl_enemy_t *enemy)
{
    return enemy->behaviour == FL_BEHAVIOUR_TITAN
           || enemy->behaviour == FL_BEHAVIOUR_WYRM
           || enemy->behaviour == FL_BEHAVIOUR_KING;
}

/* The siege and the ember tower lob at the ground; birds are out of reach. */
static bool fl_hits_air(fl_tower_kind_t kind)
{
    return kind != FL_TOWER_SIEGE && kind != FL_TOWER_EMBER;
}

/* Where a shot aims: a flyer's body is above its shadow on the road. */
static float fl_aim_y(const fl_enemy_t *enemy)
{
    return fl_enemy_flies(enemy) ? enemy->y - (float)FL_FLY_HEIGHT : enemy->y;
}

static fl_enemy_t *fl_spawn_enemy_at(fl_state_t *state, uint8_t kind,
                                     uint8_t lane, float dist);

static void fl_spawn_enemy(fl_state_t *state, uint8_t kind, uint8_t lane)
{
    fl_spawn_enemy_at(state, kind, lane, 0.0f);
}

/* A brood called up around a boss, strung out behind it on its road. */
static void fl_call_brood(fl_state_t *state, const fl_enemy_t *caller,
                          uint8_t kind, int count)
{
    uint8_t lane = caller->lane;
    float dist = caller->dist;
    for (int i = 0; i < count; ++i) {
        float back = dist - 14.0f * (float)(i + 1);
        fl_spawn_enemy_at(state, kind, lane, back > 0.0f ? back : 0.0f);
    }
    fl_raise(state, FL_EVENT_BOSS);
}

static fl_enemy_t *fl_spawn_enemy_at(fl_state_t *state, uint8_t kind,
                                     uint8_t lane, float dist)
{
    const fl_level_t *level = fl_level_of(state);
    if (level->lane_b_count == 0) {
        lane = 0;   /* a single-road level sends everything down lane A */
    }
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *enemy = &state->enemies[i];
        if (enemy->alive) {
            continue;
        }
        const fl_enemy_stat_t *stat = &FL_ENEMY_STATS[kind];
        /* Later waves are tougher without needing a new roster for each. */
        uint16_t hp_mult = 100;
        uint16_t armor_add = 0;
        if (state->wave >= 1 && state->wave <= level->wave_count) {
            hp_mult = level->waves[state->wave - 1].hp_mult;
            armor_add = level->waves[state->wave - 1].armor_add;
        }
        /* A slot is reused, so nothing of its last occupant may linger:
         * no burn, stun or hex carried over to a fresh enemy. */
        memset(enemy, 0, sizeof(*enemy));
        enemy->kind = kind;
        enemy->max_hp = (float)stat->hp * (float)hp_mult / 100.0f;
        enemy->hp = enemy->max_hp;
        enemy->armor = (int16_t)(stat->armor + armor_add);
        enemy->behaviour = stat->behaviour;
        enemy->cast_timer = 30;
        enemy->lane = lane;
        enemy->dist = dist;
        enemy->alive = 1;
        switch (enemy->behaviour) {
        case FL_BEHAVIOUR_BURROWER:
            enemy->phase_ticks = FL_BURROW_UP_TICKS;
            break;
        case FL_BEHAVIOUR_WYRM:
            enemy->phase_ticks = FL_WYRM_UP_TICKS;
            fl_raise(state, FL_EVENT_BOSS);
            break;
        case FL_BEHAVIOUR_KING:
            enemy->phase_ticks = FL_KING_FREEZE_TICKS;
            fl_raise(state, FL_EVENT_BOSS);
            break;
        default:
            break;
        }
        fl_lane_pos(state, lane, dist, &enemy->x, &enemy->y);
        return enemy;
    }
    return NULL;
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

/* The golem's shell gives way at half health: its armour falls away and it
 * hurries on, lighter. */
static void fl_check_shell(fl_state_t *state, fl_enemy_t *enemy)
{
    if (enemy->behaviour != FL_BEHAVIOUR_GOLEM || enemy->stage
            || enemy->hp > enemy->max_hp * 0.5f) {
        return;
    }
    enemy->stage = 1;
    int16_t shell = FL_ENEMY_STATS[enemy->kind].armor;
    enemy->armor = (int16_t)(enemy->armor > shell ? enemy->armor - shell : 0);
    fl_raise(state, FL_EVENT_CRACK);
}

/* At each third of its health the king wards itself and calls wolves. The
 * blow that crosses the line stops at it, so no single hit skips a ward. */
static void fl_check_ward(fl_state_t *state, fl_enemy_t *enemy)
{
    if (enemy->behaviour != FL_BEHAVIOUR_KING || enemy->stage >= FL_KING_WARDS) {
        return;
    }
    float line = enemy->max_hp * (float)(FL_KING_WARDS - enemy->stage)
                 / (float)(FL_KING_WARDS + 1);
    if (enemy->hp > line) {
        return;
    }
    enemy->hp = line;
    enemy->stage++;
    enemy->ward_ticks = FL_KING_WARD_TICKS;
    fl_call_brood(state, enemy, FL_ENEMY_WOLF, FL_KING_GUARD);
}

/* A slime bursts where it fell. The slimelets are warded for a moment, or
 * the very blast that burst the slime would take them too. */
static void fl_burst_slime(fl_state_t *state, const fl_enemy_t *slime)
{
    for (int i = 0; i < FL_SPLIT_COUNT; ++i) {
        float offset = ((float)i - (float)(FL_SPLIT_COUNT - 1) / 2.0f)
                       * FL_SPLIT_SPACING;
        float dist = slime->dist + offset;
        fl_enemy_t *child = fl_spawn_enemy_at(state, FL_ENEMY_SLIMELET,
                                              slime->lane,
                                              dist > 0.0f ? dist : 0.0f);
        if (child) {
            child->ward_ticks = FL_SPLIT_WARD_TICKS;
            child->slow_ticks = slime->slow_ticks;
            child->slow_percent = slime->slow_percent;
        }
    }
    fl_raise(state, FL_EVENT_SPLIT);
}

static void fl_damage_enemy(fl_state_t *state, fl_enemy_t *enemy,
                             const fl_shot_t *shot)
{
    if (enemy->ward_ticks > 0) {
        return;
    }
    int16_t raw_damage = shot->damage;
    float damage = (float)raw_damage;
    /* A hexed enemy has no armour against anything. */
    if (!(shot->flags & FL_SHOT_PIERCE) && enemy->hex_ticks == 0) {
        damage = fl_apply_armor(damage,
                                (int16_t)(enemy->armor + enemy->armor_bonus));
    }
    enemy->hp -= damage;
    if (shot->flags & FL_SHOT_HEX) {
        enemy->hex_ticks = FL_HEX_TICKS;
    }
    if (shot->slow && !fl_resists_control(enemy)) {
        enemy->slow_ticks = FL_SLOW_TICKS;
        enemy->slow_percent = shot->slow_percent;
    }
    if (shot->flags & FL_SHOT_BURN) {
        enemy->burn_ticks = FL_BURN_TICKS;
        enemy->burn_damage = (uint16_t)(raw_damage / 3);
    }
    if (shot->flags & FL_SHOT_STUN && !fl_resists_control(enemy)) {
        enemy->stun_ticks = FL_STUN_TICKS;
    }
    fl_check_ward(state, enemy);
    if (enemy->hp > 0.0f) {
        fl_check_shell(state, enemy);
        return;
    }
    enemy->alive = 0;
    state->kills++;
    state->gold += (uint16_t)FL_ENEMY_STATS[enemy->kind].bounty;
    state->score_kills += (uint32_t)FL_ENEMY_STATS[enemy->kind].score;
    state->score += (uint32_t)FL_ENEMY_STATS[enemy->kind].score;
    state->last_kill_kind = (uint8_t)enemy->kind;
    fl_raise(state, FL_EVENT_KILLED);
    if (enemy->behaviour == FL_BEHAVIOUR_SPLITTER) {
        /* A copy: the slimelets may well take the slime's own slot. */
        fl_enemy_t slime = *enemy;
        fl_burst_slime(state, &slime);
    }
}

static void fl_spawn_projectile(fl_state_t *state, fl_tower_kind_t kind,
                                int target, int16_t damage, float x, float y,
                                float tx, float ty, uint8_t flags,
                                uint16_t speed, int16_t aoe,
                                uint16_t slow_percent, uint8_t zone_ticks)
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
        shot->zone_ticks = zone_ticks;
        shot->ttl = 120;
        shot->active = 1;
        return;
    }
}

/* Where a tower's shot leaves it: the ballista deck, the spire's tip. The
 * view draws bolts and beams from the points the model records, so the two
 * cannot disagree. */
float fl_muzzle_height(fl_tower_kind_t kind)
{
    switch (kind) {
    case FL_TOWER_STORM: return 34.0f;
    case FL_TOWER_RUNE:  return 34.0f;
    case FL_TOWER_EMBER: return 30.0f;
    default:             return 20.0f;
    }
}

static uint32_t fl_shot_event(fl_tower_kind_t kind)
{
    switch (kind) {
    case FL_TOWER_GUARD: return FL_EVENT_SHOT_GUARD;
    case FL_TOWER_FROST: return FL_EVENT_SHOT_FROST;
    case FL_TOWER_STORM: return FL_EVENT_SHOT_STORM;
    case FL_TOWER_RUNE:  return FL_EVENT_SHOT_RUNE;
    case FL_TOWER_EMBER: return FL_EVENT_SHOT_EMBER;
    default:             return FL_EVENT_SHOT_SIEGE;
    }
}

/* Start a bolt or beam at the tower. When every slot is still fading the
 * oldest gives way, so a busy screen never drops a fresh one. */
static fl_arc_t *fl_new_arc(fl_state_t *state, const fl_tower_t *tower)
{
    int pick = 0;
    for (int i = 0; i < FL_MAX_ARCS; ++i) {
        if (state->arcs[i].ttl < state->arcs[pick].ttl) {
            pick = i;
        }
    }
    fl_arc_t *arc = &state->arcs[pick];
    arc->x[0] = tower->x;
    arc->y[0] = (int16_t)((float)tower->y - fl_muzzle_height(tower->kind));
    arc->count = 1;
    arc->kind = (uint8_t)tower->kind;
    arc->ttl = FL_ARC_TICKS;
    arc->seed = (uint8_t)(state->tick * 37u + (uint32_t)pick * 11u);
    return arc;
}

static void fl_arc_add(fl_arc_t *arc, const fl_enemy_t *enemy)
{
    if (arc->count >= FL_ARC_POINTS) {
        return;
    }
    /* Struck at the chest, not the feet; a bird at its body in the air. */
    arc->x[arc->count] = (int16_t)enemy->x;
    arc->y[arc->count] = (int16_t)(fl_enemy_flies(enemy) ? fl_aim_y(enemy)
                                                         : enemy->y - 10.0f);
    arc->count++;
}

/* The nearest living enemy to a point that the bolt has not struck yet. */
static int fl_next_in_chain(const fl_state_t *state, float x, float y,
                            const uint8_t *struck, int struck_count)
{
    int best = -1;
    float best_d2 = (float)FL_CHAIN_RADIUS * FL_CHAIN_RADIUS;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &state->enemies[i];
        if (!fl_targetable(enemy)) {
            continue;
        }
        bool seen = false;
        for (int s = 0; s < struck_count; ++s) {
            if (struck[s] == i) {
                seen = true;
                break;
            }
        }
        if (seen) {
            continue;
        }
        float dx = enemy->x - x;
        float dy = enemy->y - y;
        float d2 = dx * dx + dy * dy;
        if (d2 <= best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    return best;
}

/* Target the enemy closest to the goal, which is the classic "first" rule. */
static int fl_pick_target(const fl_state_t *state, const fl_tower_t *tower,
                           int16_t range)
{
    int best = -1;
    float best_dist = -1.0f;
    float range2 = (float)range * (float)range;
    bool air = fl_hits_air(tower->kind);
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &state->enemies[i];
        if (!fl_targetable(enemy) || (!air && fl_enemy_flies(enemy))) {
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
    bool air = fl_hits_air(tower->kind);
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &state->enemies[i];
        if (!fl_targetable(enemy) || i == first
                || (!air && fl_enemy_flies(enemy))) {
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

/* Storm: the bolt strikes the first enemy, then leaps to the nearest one it
 * has not struck yet, losing some of its bite at every leap. */
static void fl_fire_chain(fl_state_t *state, const fl_tower_t *tower,
                          const fl_tower_stats_t *stat, int first,
                          uint8_t flags)
{
    fl_arc_t *arc = fl_new_arc(state, tower);
    uint8_t struck[FL_ARC_POINTS];
    int reach = stat->extra < 1 ? 1 : stat->extra;
    if (reach > FL_ARC_POINTS - 1) {
        reach = FL_ARC_POINTS - 1;
    }
    int count = 0;
    int current = first;
    int16_t damage = stat->damage;
    while (current >= 0 && count < reach) {
        fl_enemy_t *enemy = &state->enemies[current];
        struck[count++] = (uint8_t)current;
        fl_arc_add(arc, enemy);
        float x = enemy->x;
        float y = enemy->y;
        fl_shot_t hit;
        hit.damage = damage;
        hit.aoe = 0;
        hit.flags = flags;
        hit.slow = false;
        hit.slow_percent = FL_SLOW_PERCENT;
        fl_damage_enemy(state, enemy, &hit);
        damage = (int16_t)(damage * FL_CHAIN_FALLOFF / 100);
        if (damage < 1) {
            damage = 1;
        }
        current = fl_next_in_chain(state, x, y, struck, count);
    }
}

/* Rune: a beam that lands the instant it is fired and ignores armour. */
static void fl_fire_beam(fl_state_t *state, const fl_tower_t *tower,
                         const fl_tower_stats_t *stat, int target,
                         uint8_t flags)
{
    fl_enemy_t *enemy = &state->enemies[target];
    fl_arc_t *arc = fl_new_arc(state, tower);
    fl_arc_add(arc, enemy);
    fl_shot_t hit;
    hit.damage = stat->damage;
    hit.aoe = 0;
    hit.flags = (uint8_t)(flags | FL_SHOT_PIERCE);
    hit.slow = false;
    hit.slow_percent = FL_SLOW_PERCENT;
    fl_damage_enemy(state, enemy, &hit);
}

static void fl_fire_projectile(fl_state_t *state, const fl_tower_t *tower,
                               const fl_tower_stats_t *stat, int target,
                               uint8_t flags)
{
    const fl_enemy_t *enemy = &state->enemies[target];
    uint8_t zone_ticks = 0;
    if (tower->kind == FL_TOWER_EMBER) {
        flags |= FL_SHOT_ZONE;
        zone_ticks = (uint8_t)(stat->extra > 255 ? 255 : stat->extra);
    }
    fl_spawn_projectile(state, tower->kind, target, stat->damage,
                        (float)tower->x,
                        (float)tower->y - fl_muzzle_height(tower->kind),
                        enemy->x, fl_aim_y(enemy), flags, stat->projectile_speed,
                        stat->aoe, stat->slow_percent, zone_ticks);
}

static void fl_update_towers(fl_state_t *state)
{
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        fl_tower_t *tower = &state->towers[i];
        if (!tower->active) {
            continue;
        }
        if (tower->shot_tick < 255) {
            tower->shot_tick++;
        }
        if (tower->frozen_ticks > 0) {
            tower->frozen_ticks--;
            continue;
        }
        /* A barracks does its fighting through its soldiers. */
        if (tower->kind == FL_TOWER_BARRACKS) {
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
        if (stat.hex) {
            flags |= FL_SHOT_HEX;
        }
        if (stat.slow) {
            flags |= FL_SHOT_SLOW;
        }
        /* The repeater and the prism also take the next enemy down the road;
         * it is picked before the first shot, which may kill its target. */
        int second = stat.double_shot
                     ? fl_pick_second_target(state, tower, stat.range, target)
                     : -1;
        switch (tower->kind) {
        case FL_TOWER_STORM:
            fl_fire_chain(state, tower, &stat, target, flags);
            break;
        case FL_TOWER_RUNE:
            fl_fire_beam(state, tower, &stat, target, flags);
            if (second >= 0 && state->enemies[second].alive) {
                fl_fire_beam(state, tower, &stat, second, flags);
            }
            break;
        default:
            fl_fire_projectile(state, tower, &stat, target, flags);
            if (second >= 0) {
                fl_fire_projectile(state, tower, &stat, second, flags);
            }
            break;
        }
        fl_raise(state, fl_shot_event(tower->kind));
        tower->cooldown = stat.cooldown;
        tower->shot_tick = 0;
    }
}

/* Set a patch of road burning where an ember pot lands. When every patch is
 * alight, the one closest to burning out gives way. */
static void fl_spawn_zone(fl_state_t *state, const fl_projectile_t *shot)
{
    int pick = 0;
    for (int i = 0; i < FL_MAX_ZONES; ++i) {
        if (!state->zones[i].active) {
            pick = i;
            break;
        }
        if (state->zones[i].ticks < state->zones[pick].ticks) {
            pick = i;
        }
    }
    fl_zone_t *zone = &state->zones[pick];
    zone->x = (int16_t)shot->tx;
    zone->y = (int16_t)shot->ty;
    zone->radius = shot->aoe;
    zone->damage = shot->damage;
    zone->ticks = shot->zone_ticks;
    zone->life = shot->zone_ticks;
    zone->slow = (shot->flags & FL_SHOT_SLOW) ? 1 : 0;
    zone->active = 1;
}

static void fl_impact_projectile(fl_state_t *state, fl_projectile_t *shot)
{
    if (shot->flags & FL_SHOT_ZONE) {
        fl_spawn_zone(state, shot);
        fl_raise(state, FL_EVENT_IMPACT);
        return;
    }
    fl_shot_t hit;
    hit.damage = shot->damage;
    hit.aoe = shot->aoe;
    hit.flags = shot->flags;
    hit.slow = shot->kind == FL_TOWER_FROST || (shot->flags & FL_SHOT_SLOW);
    hit.slow_percent = shot->slow_percent ? shot->slow_percent : FL_SLOW_PERCENT;
    if (hit.aoe > 0) {
        /* Only the area blasts get their own boom; a plain hit is covered by
         * the shot sound, and doubling every hit would turn the mix to mud. */
        fl_raise(state, FL_EVENT_IMPACT);
        float aoe2 = (float)hit.aoe * (float)hit.aoe;
        bool air = fl_hits_air((fl_tower_kind_t)shot->kind);
        for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
            fl_enemy_t *enemy = &state->enemies[i];
            if (!fl_targetable(enemy) || (!air && fl_enemy_flies(enemy))) {
                continue;
            }
            float dx = enemy->x - shot->tx;
            float dy = fl_aim_y(enemy) - shot->ty;
            if (dx * dx + dy * dy > aoe2) {
                continue;
            }
            fl_damage_enemy(state, enemy, &hit);
        }
        return;
    }
    if (shot->target < FL_MAX_ENEMIES) {
        fl_enemy_t *enemy = &state->enemies[shot->target];
        if (fl_targetable(enemy)) {
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
        if (shot->target < FL_MAX_ENEMIES) {
            const fl_enemy_t *target = &state->enemies[shot->target];
            if (fl_targetable(target)) {
                shot->tx = target->x;
                shot->ty = fl_aim_y(target);
            } else if (target->alive) {
                /* It dived: the shot flies on to where it was. */
                shot->target = FL_NO_TARGET;
            }
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

/* Burning road bites everyone standing in it every pulse. Fire ignores
 * armour, the way the siege tower's burn does. */
static void fl_update_zones(fl_state_t *state)
{
    for (int z = 0; z < FL_MAX_ZONES; ++z) {
        fl_zone_t *zone = &state->zones[z];
        if (!zone->active) {
            continue;
        }
        if (zone->ticks == 0) {
            zone->active = 0;
            continue;
        }
        bool pulse = (zone->life - zone->ticks) % FL_ZONE_PULSE_TICKS == 0;
        zone->ticks--;
        if (!pulse) {
            continue;
        }
        float radius2 = (float)zone->radius * (float)zone->radius;
        fl_shot_t burn;
        burn.damage = zone->damage;
        burn.aoe = 0;
        burn.flags = FL_SHOT_PIERCE;
        burn.slow = false;
        burn.slow_percent = FL_SLOW_PERCENT;
        for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
            fl_enemy_t *enemy = &state->enemies[i];
            if (!fl_grounded(enemy)) {
                continue;
            }
            float dx = enemy->x - (float)zone->x;
            float dy = enemy->y - (float)zone->y;
            if (dx * dx + dy * dy > radius2) {
                continue;
            }
            if (zone->slow && !fl_resists_control(enemy)) {
                /* Long enough to last until the next pulse renews it. */
                enemy->slow_ticks = FL_ZONE_PULSE_TICKS + 2;
                enemy->slow_percent = FL_ZONE_SLOW_PERCENT;
            }
            fl_damage_enemy(state, enemy, &burn);
        }
    }
}

static void fl_update_arcs(fl_state_t *state)
{
    for (int i = 0; i < FL_MAX_ARCS; ++i) {
        if (state->arcs[i].ttl > 0) {
            state->arcs[i].ttl--;
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

/* The hero that an enemy has to walk past, if any. */
static int fl_hero_in_the_way(const fl_state_t *state, float x, float y)
{
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        const fl_hero_t *hero = &state->heroes[i];
        if (!hero->active || !hero->alive) {
            continue;
        }
        float dx = x - (float)hero->x;
        float dy = y - (float)hero->y;
        if (dx * dx + dy * dy <= (float)FL_HERO_BLOCK_RADIUS * FL_HERO_BLOCK_RADIUS) {
            return i;
        }
    }
    return -1;
}

/* The hero shoots at the enemy closest to the gate. */
static void fl_update_heroes(fl_state_t *state)
{
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        fl_hero_t *hero = &state->heroes[i];
        if (!hero->active) {
            continue;
        }
        if (!hero->alive) {
            if (hero->respawn_timer > 0) {
                hero->respawn_timer--;
                if (hero->respawn_timer == 0) {
                    hero->alive = 1;
                    hero->hp = hero->max_hp;
                    hero->cooldown = 0;
                }
            }
            continue;
        }
        if (hero->skill_timer > 0) {
            hero->skill_timer--;
        }
        if (hero->cooldown > 0) {
            hero->cooldown--;
            continue;
        }
        const fl_hero_stat_t *stat = &FL_HERO_STATS[hero->kind];
        int best = -1;
        float best_dist = -1.0f;
        float range2 = (float)stat->range * (float)stat->range;
        for (int e = 0; e < FL_MAX_ENEMIES; ++e) {
            const fl_enemy_t *enemy = &state->enemies[e];
            if (!fl_targetable(enemy)) {
                continue;
            }
            float dx = enemy->x - (float)hero->x;
            float dy = enemy->y - (float)hero->y;
            if (dx * dx + dy * dy > range2) {
                continue;
            }
            if (best < 0 || enemy->dist > best_dist) {
                best = e;
                best_dist = enemy->dist;
            }
        }
        if (best < 0) {
            continue;
        }
        fl_enemy_t *enemy = &state->enemies[best];
        fl_shot_t shot;
        shot.damage = stat->damage;
        shot.aoe = 0;
        shot.flags = 0;
        shot.slow = false;
        shot.slow_percent = FL_SLOW_PERCENT;
        fl_damage_enemy(state, enemy, &shot);
        hero->cooldown = stat->cooldown;
        fl_raise(state, FL_EVENT_HERO_SHOT);
    }
}

/* ------------------------------------------------------------------ */
/* Barracks soldiers                                                   */
/* ------------------------------------------------------------------ */

/* A garrison stands in a tight knot across the road: two side by side, the
 * third behind them, the fourth in front. */
static const int8_t FL_FORMATION[FL_SOLDIER_MAX][2] = {
    {-8, -3}, {8, -3}, {0, 6}, {0, -11},
};

/* The closest point on any road of the level, where a hall musters. */
static void fl_nearest_road_point(const fl_state_t *state, float x, float y,
                                  int16_t *out_x, int16_t *out_y)
{
    const fl_level_t *level = fl_level_of(state);
    float best = -1.0f;
    float best_x = x;
    float best_y = y;
    for (uint8_t lane = 0; lane < 2; ++lane) {
        const fl_point_t *points;
        int count;
        fl_lane_points(level, lane, &points, &count);
        for (int i = 1; i < count; ++i) {
            float x0 = (float)points[i - 1].x;
            float y0 = (float)points[i - 1].y;
            float dx = (float)points[i].x - x0;
            float dy = (float)points[i].y - y0;
            float length2 = dx * dx + dy * dy;
            float t = length2 > 0.0f ? ((x - x0) * dx + (y - y0) * dy) / length2
                                     : 0.0f;
            t = t < 0.0f ? 0.0f : (t > 1.0f ? 1.0f : t);
            float px = x0 + dx * t;
            float py = y0 + dy * t;
            float d2 = (px - x) * (px - x) + (py - y) * (py - y);
            if (best < 0.0f || d2 < best) {
                best = d2;
                best_x = px;
                best_y = py;
            }
        }
    }
    *out_x = (int16_t)lroundf(best_x);
    *out_y = (int16_t)lroundf(best_y);
}

static int fl_garrison_size(const fl_tower_stats_t *stat)
{
    int size = stat->extra;
    return size < 1 ? 1 : (size > FL_SOLDIER_MAX ? FL_SOLDIER_MAX : size);
}

static bool fl_has_soldier(const fl_state_t *state, int tower, int slot)
{
    for (int s = 0; s < FL_MAX_SOLDIERS; ++s) {
        const fl_soldier_t *soldier = &state->soldiers[s];
        if (soldier->active && soldier->tower == tower && soldier->slot == slot) {
            return true;
        }
    }
    return false;
}

/* Every hall keeps its full garrison: a new hall, or one that took the
 * veterans upgrade, sends its missing soldiers out of the door. */
static void fl_muster(fl_state_t *state)
{
    for (int t = 0; t < FL_MAX_TOWERS; ++t) {
        const fl_tower_t *tower = &state->towers[t];
        if (!tower->active || tower->kind != FL_TOWER_BARRACKS) {
            continue;
        }
        fl_tower_stats_t stat = fl_tower_stats(tower);
        int size = fl_garrison_size(&stat);
        for (int slot = 0; slot < size; ++slot) {
            if (fl_has_soldier(state, t, slot)) {
                continue;
            }
            for (int s = 0; s < FL_MAX_SOLDIERS; ++s) {
                fl_soldier_t *soldier = &state->soldiers[s];
                if (soldier->active) {
                    continue;
                }
                memset(soldier, 0, sizeof(*soldier));
                soldier->x = (float)tower->x;
                soldier->y = (float)tower->y;
                soldier->max_hp = (uint16_t)stat.hp;
                soldier->hp = soldier->max_hp;
                soldier->tower = (uint8_t)t;
                soldier->slot = (uint8_t)slot;
                soldier->active = 1;
                soldier->alive = 1;
                soldier->swing = 255;
                break;
            }
        }
    }
}

static void fl_update_soldiers(fl_state_t *state)
{
    fl_muster(state);
    for (int s = 0; s < FL_MAX_SOLDIERS; ++s) {
        fl_soldier_t *soldier = &state->soldiers[s];
        if (!soldier->active) {
            continue;
        }
        const fl_tower_t *tower = &state->towers[soldier->tower];
        if (!tower->active || tower->kind != FL_TOWER_BARRACKS) {
            soldier->active = 0;   /* the hall was sold */
            continue;
        }
        fl_tower_stats_t stat = fl_tower_stats(tower);
        if (soldier->max_hp != (uint16_t)stat.hp) {
            /* An upgrade toughens the garrison and patches it up. */
            soldier->max_hp = (uint16_t)stat.hp;
            if (soldier->alive) {
                soldier->hp = soldier->max_hp;
            }
        }
        if (!soldier->alive) {
            if (soldier->respawn_timer > 0) {
                soldier->respawn_timer--;
            }
            if (soldier->respawn_timer == 0) {
                soldier->alive = 1;
                soldier->hp = soldier->max_hp;
                soldier->x = (float)tower->x;
                soldier->y = (float)tower->y;
                soldier->cooldown = 0;
            }
            continue;
        }
        if (soldier->swing < 255) {
            soldier->swing++;
        }

        /* Walk to its place in the formation. */
        float px = (float)(tower->rally_x + FL_FORMATION[soldier->slot][0]);
        float py = (float)(tower->rally_y + FL_FORMATION[soldier->slot][1]);
        float dx = px - soldier->x;
        float dy = py - soldier->y;
        float distance = sqrtf(dx * dx + dy * dy);
        float step = FL_SOLDIER_SPEED / 30.0f;
        soldier->moving = distance > step ? 1 : 0;
        if (soldier->moving) {
            soldier->x += dx / distance * step;
            soldier->y += dy / distance * step;
        } else {
            soldier->x = px;
            soldier->y = py;
        }

        /* Strike the nearest enemy within reach. */
        if (soldier->cooldown > 0) {
            soldier->cooldown--;
            continue;
        }
        int best = -1;
        float best_d2 = (float)FL_SOLDIER_REACH * FL_SOLDIER_REACH;
        for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
            const fl_enemy_t *enemy = &state->enemies[i];
            if (!fl_grounded(enemy)) {
                continue;
            }
            float ex = enemy->x - soldier->x;
            float ey = enemy->y - soldier->y;
            float d2 = ex * ex + ey * ey;
            if (d2 <= best_d2) {
                best_d2 = d2;
                best = i;
            }
        }
        if (best < 0) {
            continue;
        }
        fl_shot_t blow;
        blow.damage = stat.damage;
        blow.aoe = 0;
        blow.flags = 0;
        blow.slow = false;
        blow.slow_percent = FL_SLOW_PERCENT;
        fl_damage_enemy(state, &state->enemies[best], &blow);
        soldier->cooldown = stat.cooldown;
        soldier->swing = 0;
        fl_raise(state, FL_EVENT_CLASH);
    }
}

/* The soldier holding an enemy, keeping or claiming one with room to spare.
 * `held` counts the enemies each soldier holds this tick. Returns -1 when
 * the enemy is free to walk on. */
static int fl_soldier_in_the_way(fl_state_t *state, fl_enemy_t *enemy,
                                 uint8_t held[FL_MAX_SOLDIERS])
{
    if (enemy->soldier) {
        int s = enemy->soldier - 1;
        if (state->soldiers[s].active && state->soldiers[s].alive) {
            return s;
        }
        enemy->soldier = 0;
    }
    float radius2 = (float)FL_SOLDIER_BLOCK_RADIUS * FL_SOLDIER_BLOCK_RADIUS;
    for (int s = 0; s < FL_MAX_SOLDIERS; ++s) {
        const fl_soldier_t *soldier = &state->soldiers[s];
        if (!soldier->active || !soldier->alive
                || held[s] >= FL_SOLDIER_HOLD) {
            continue;
        }
        float dx = enemy->x - soldier->x;
        float dy = enemy->y - soldier->y;
        if (dx * dx + dy * dy > radius2) {
            continue;
        }
        held[s]++;
        enemy->soldier = (uint8_t)(s + 1);
        return s;
    }
    return -1;
}

/* A held enemy fights back. */
static void fl_strike_soldier(fl_state_t *state, fl_enemy_t *enemy,
                              fl_soldier_t *soldier)
{
    if (enemy->attack_timer > 0) {
        enemy->attack_timer--;
        return;
    }
    enemy->attack_timer = FL_ENEMY_HIT_TICKS;
    fl_raise(state, FL_EVENT_CLASH);
    if (soldier->hp > FL_HERO_HIT_DAMAGE) {
        soldier->hp = (uint16_t)(soldier->hp - FL_HERO_HIT_DAMAGE);
        return;
    }
    soldier->hp = 0;
    soldier->alive = 0;
    soldier->respawn_timer = FL_SOLDIER_RESPAWN;
    enemy->soldier = 0;
}

/* Burrowers and the wyrm spend a spell above the road, then one under it.
 * The wyrm brings a brood of skitters up with it every time it surfaces. */
static void fl_tick_burrow(fl_state_t *state, fl_enemy_t *enemy)
{
    if (enemy->phase_ticks > 0) {
        enemy->phase_ticks--;
        return;
    }
    bool wyrm = enemy->behaviour == FL_BEHAVIOUR_WYRM;
    enemy->burrowed = enemy->burrowed ? 0 : 1;
    fl_raise(state, FL_EVENT_BURROW);
    if (enemy->burrowed) {
        enemy->soldier = 0;      /* it slips out of a soldier's grip */
        enemy->burn_ticks = 0;   /* and the earth smothers any fire */
        enemy->phase_ticks = wyrm ? FL_WYRM_DOWN_TICKS : FL_BURROW_DOWN_TICKS;
        return;
    }
    enemy->phase_ticks = wyrm ? FL_WYRM_UP_TICKS : FL_BURROW_UP_TICKS;
    if (wyrm) {
        fl_call_brood(state, enemy, FL_ENEMY_SKITTER, FL_WYRM_BROOD);
    }
}

/* The king's cold reaches out to the nearest tower still firing and locks
 * it in ice for a while. A barracks is left alone: its soldiers fight on
 * whether the hall is frozen or not. */
static void fl_tick_king(fl_state_t *state, fl_enemy_t *king)
{
    if (king->phase_ticks > 0) {
        king->phase_ticks--;
        return;
    }
    int best = -1;
    float best_d2 = (float)FL_KING_FREEZE_RANGE * FL_KING_FREEZE_RANGE;
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        const fl_tower_t *tower = &state->towers[i];
        if (!tower->active || tower->kind == FL_TOWER_BARRACKS
                || tower->frozen_ticks > 0) {
            continue;
        }
        float dx = (float)tower->x - king->x;
        float dy = (float)tower->y - king->y;
        float d2 = dx * dx + dy * dy;
        if (d2 <= best_d2) {
            best_d2 = d2;
            best = i;
        }
    }
    if (best < 0) {
        king->phase_ticks = FL_KING_RETRY_TICKS;
        return;
    }
    fl_tower_t *tower = &state->towers[best];
    tower->frozen_ticks = FL_FREEZE_TICKS;
    /* The beam runs from the tower to the king's staff, held high. */
    fl_arc_t *arc = fl_new_arc(state, tower);
    arc->kind = FL_ARC_FREEZE;
    arc->ttl = FL_ARC_TICKS * 2;
    arc->x[1] = (int16_t)(king->x + 14.0f);
    arc->y[1] = (int16_t)(king->y - 62.0f);
    arc->count = 2;
    king->phase_ticks = FL_KING_FREEZE_TICKS;
    fl_raise(state, FL_EVENT_FREEZE);
}

/* Walking pace before any slow: a golem out of its shell and anything
 * tunnelling under the road go faster. */
static float fl_enemy_speed(const fl_enemy_t *enemy)
{
    float speed = (float)FL_ENEMY_STATS[enemy->kind].speed;
    if (enemy->behaviour == FL_BEHAVIOUR_GOLEM && enemy->stage) {
        speed = speed * FL_CRACKED_SPEED / 100.0f;
    }
    if (enemy->burrowed) {
        int boost = enemy->behaviour == FL_BEHAVIOUR_WYRM ? FL_WYRM_SPEED
                                                          : FL_BURROW_SPEED;
        speed = speed * (float)boost / 100.0f;
    }
    return speed;
}

static void fl_update_enemies(fl_state_t *state)
{
    /* How many enemies each soldier holds, from the claims that still
     * stand; fl_soldier_in_the_way adds the new ones as they come. */
    uint8_t held[FL_MAX_SOLDIERS];
    memset(held, 0, sizeof(held));
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &state->enemies[i];
        if (enemy->alive && enemy->soldier) {
            const fl_soldier_t *soldier = &state->soldiers[enemy->soldier - 1];
            if (soldier->active && soldier->alive) {
                held[enemy->soldier - 1]++;
            }
        }
    }

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
        if (enemy->hex_ticks > 0) {
            enemy->hex_ticks--;
        }
        if (enemy->ward_ticks > 0) {
            enemy->ward_ticks--;
        }
        if (enemy->behaviour == FL_BEHAVIOUR_BURROWER
                || enemy->behaviour == FL_BEHAVIOUR_WYRM) {
            fl_tick_burrow(state, enemy);
        } else if (enemy->behaviour == FL_BEHAVIOUR_KING) {
            fl_tick_king(state, enemy);
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
        /* A hero standing in the road has to be dealt with first; birds and
         * tunnellers pass over and under it. */
        int blocker = fl_grounded(enemy)
                      ? fl_hero_in_the_way(state, enemy->x, enemy->y) : -1;
        if (blocker >= 0) {
            fl_hero_t *hero = &state->heroes[blocker];
            if (enemy->attack_timer > 0) {
                enemy->attack_timer--;
            }
            if (enemy->attack_timer == 0) {
                enemy->attack_timer = FL_ENEMY_HIT_TICKS;
                if (hero->hp > FL_HERO_HIT_DAMAGE) {
                    hero->hp = (uint16_t)(hero->hp - FL_HERO_HIT_DAMAGE);
                    fl_raise(state, FL_EVENT_HERO_HIT);
                } else {
                    hero->hp = 0;
                    hero->alive = 0;
                    hero->respawn_timer =
                        FL_HERO_STATS[hero->kind].respawn_ticks;
                    fl_raise(state, FL_EVENT_HERO_DOWN);
                }
            }
            continue;
        }
        /* So does a soldier with room to hold one more. */
        int holder = fl_grounded(enemy)
                     ? fl_soldier_in_the_way(state, enemy, held) : -1;
        if (holder >= 0) {
            fl_strike_soldier(state, enemy, &state->soldiers[holder]);
            continue;
        }
        float speed = fl_enemy_speed(enemy);
        if (enemy->slow_ticks > 0) {
            speed *= (float)enemy->slow_percent / 100.0f;
        }
        enemy->dist += speed / 30.0f;
        float lane_b_len = fl_lane_length(state, 1);
        if (enemy->lane == 1 && enemy->dist >= lane_b_len) {
            enemy->lane = 0;
            enemy->dist = fl_level_of(state)->merge_distance
                          + (enemy->dist - lane_b_len);
        }
        fl_lane_pos(state, enemy->lane, enemy->dist, &enemy->x, &enemy->y);
        if (enemy->dist >= fl_lane_length(state, 0)) {
            enemy->alive = 0;
            state->leaks++;
            fl_raise(state, FL_EVENT_LEAK);
            uint8_t cost = 1;
            if (enemy->behaviour == FL_BEHAVIOUR_SAP) {
                cost = FL_SAP_LIVES;
            } else if (enemy->behaviour == FL_BEHAVIOUR_WYRM
                       || enemy->behaviour == FL_BEHAVIOUR_KING) {
                cost = FL_BOSS_LIVES;
            }
            for (uint8_t i = 0; i < cost && state->lives > 0; ++i) {
                state->lives--;
            }
            if (state->lives == 0) {
                state->phase = FL_PHASE_LOST;
                fl_raise(state, FL_EVENT_LOST);
            }
        }
    }
}

static void fl_update_spawning(fl_state_t *state)
{
    const fl_level_t *level = fl_level_of(state);
    if (state->wave > level->wave_count) {
        return;
    }
    const fl_wave_t *wave = &level->waves[state->wave - 1];
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
    const fl_level_t *level = fl_level_of(state);
    const fl_wave_t *wave = &level->waves[state->wave - 1];
    if (state->batch_index < wave->batch_count || fl_any_enemy_alive(state)) {
        return;
    }
    state->gold += wave->reward;
    state->score_waves += wave->score_bonus;
    state->score += wave->score_bonus;
    state->wave++;
    fl_raise(state, FL_EVENT_WAVE_CLEAR);
    if (state->wave > level->wave_count) {
        state->score_bonus = (uint32_t)state->lives * FL_SCORE_PER_LIFE;
        if (state->leaks == 0) {
            state->score_bonus += FL_SCORE_FLAWLESS;
        }
        state->score += state->score_bonus;
        state->phase = FL_PHASE_WON;
        fl_record_progress(state);
        fl_raise(state, FL_EVENT_WON);
        return;
    }
    state->phase = FL_PHASE_PREP;
    state->prep_ticks = FL_PREP_TICKS;
}

static void fl_step(fl_state_t *state)
{
    state->tick++;
    if (state->phase == FL_PHASE_PREP) {
        /* Between waves the garrisons still walk out and the last fires
         * and bolts still fade. */
        fl_update_soldiers(state);
        fl_update_zones(state);
        fl_update_arcs(state);
        if (state->prep_ticks > 0) {
            state->prep_ticks--;
        }
        if (state->prep_ticks == 0) {
            state->phase = FL_PHASE_WAVE;
            state->batch_index = 0;
            state->batch_spawned = 0;
            state->spawn_timer = 0;
            fl_raise(state, FL_EVENT_WAVE_START);
        }
        return;
    }
    if (state->phase != FL_PHASE_WAVE) {
        return;
    }
    fl_update_spawning(state);
    fl_update_heroes(state);
    fl_update_soldiers(state);
    fl_update_towers(state);
    fl_update_projectiles(state);
    fl_update_zones(state);
    fl_update_arcs(state);
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
    out.hex = 0;
    out.slow = 0;
    out.extra = 0;
    out.hp = 0;
    if (!tower || tower->kind <= FL_TOWER_NONE
            || tower->kind >= FL_TOWER_KIND_COUNT) {
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
    out.extra = level->extra;
    out.hp = level->hp;

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
            if (mod->hp_mult) {
                out.hp = (int16_t)(out.hp * mod->hp_mult / 100);
            }
            out.extra = (int16_t)(out.extra + mod->extra_delta);
            out.pierce |= mod->pierce;
            out.burn |= mod->burn;
            out.stun |= mod->stun;
            out.double_shot |= mod->double_shot;
            out.hex |= mod->hex;
            out.slow |= mod->slow;
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
        fl_raise(state, FL_EVENT_DENIED);
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
    fl_raise(state, FL_EVENT_UPGRADED);
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
    if (kind <= FL_TOWER_NONE || kind >= FL_TOWER_KIND_COUNT) {
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

/* Clear the battle and set up wave 1 of the given level. The progress and
 * the PRNG stream are the caller's business. */
static void fl_begin_level(fl_state_t *state, uint8_t level)
{
    fl_geom_init();
    /* Keep what outlives a run, wipe the rest. */
    uint32_t best[FL_MAX_LEVELS];
    uint8_t stars[FL_MAX_LEVELS];
    memcpy(best, state->level_best, sizeof(best));
    memcpy(stars, state->level_stars, sizeof(stars));
    uint32_t seed = state->seed;
    uint16_t serial = state->run_serial;
    uint32_t events = state->events;
    uint32_t event_serial = state->event_serial;
    memset(state, 0, sizeof(*state));
    memcpy(state->level_best, best, sizeof(best));
    memcpy(state->level_stars, stars, sizeof(stars));
    state->seed = seed;
    state->run_serial = (uint16_t)(serial + 1u);
    state->events = events;
    state->event_serial = event_serial;

    const fl_level_t *data = &FL_LEVELS[level];
    state->level = level;
    state->menu_level = level;
    state->gold = data->start_gold;
    state->lives = data->lives;
    state->start_lives = data->lives;
    state->wave = 1;
    state->phase = FL_PHASE_PREP;
    state->speed = 1;
    state->selected_tower = -1;
    state->ring_spot = -1;
    state->ring_post = -1;
    state->ring_pick = -1;
    state->prep_ticks = FL_FIRST_PREP_TICKS;
}

void fl_state_reset(fl_state_t *state, uint32_t seed)
{
    memset(state, 0, sizeof(*state));
    state->seed = seed ? seed : 0x464C5354u;
    fl_begin_level(state, 0);
    (void)fl_rand(state);
}

uint8_t fl_level_count(void)
{
    return FL_LEVEL_COUNT;
}

bool fl_level_unlocked(const fl_state_t *state, uint8_t level)
{
    if (level >= FL_LEVEL_COUNT) {
        return false;
    }
    return level == 0 || state->level_stars[level - 1] > 0;
}

bool fl_state_start_level(fl_state_t *state, uint8_t level)
{
    if (!fl_level_unlocked(state, level)) {
        return false;
    }
    /* A fresh stream per run, so a retry is not a replay of the last one. */
    state->seed ^= 0x9E3779B9u + state->run_serial;
    fl_begin_level(state, level);
    (void)fl_rand(state);
    fl_raise(state, FL_EVENT_LEVEL_START);
    return true;
}

void fl_state_open_menu(fl_state_t *state)
{
    uint8_t level = state->level;
    fl_begin_level(state, level);
    state->phase = FL_PHASE_MENU;
    /* Point the map at the newest level the player can fight. */
    uint8_t pick = 0;
    for (uint8_t i = 0; i < FL_LEVEL_COUNT; ++i) {
        if (fl_level_unlocked(state, i)) {
            pick = i;
        }
    }
    state->menu_level = pick;
    fl_raise(state, FL_EVENT_MENU);
}

bool fl_tower_available(const fl_state_t *state, fl_tower_kind_t kind)
{
    if (kind <= FL_TOWER_NONE || kind >= FL_TOWER_KIND_COUNT) {
        return false;
    }
    return (fl_level_of(state)->tower_mask & (1u << kind)) != 0;
}

fl_tower_kind_t fl_new_tower(uint8_t level)
{
    /* The first stage's towers are the starting armoury, not a reward. */
    if (level == 0 || level >= FL_LEVEL_COUNT) {
        return FL_TOWER_NONE;
    }
    unsigned fresh = FL_LEVELS[level].tower_mask
                     & ~(unsigned)FL_LEVELS[level - 1].tower_mask;
    for (int kind = FL_TOWER_GUARD; kind < FL_TOWER_KIND_COUNT; ++kind) {
        if (fresh & (1u << kind)) {
            return (fl_tower_kind_t)kind;
        }
    }
    return FL_TOWER_NONE;
}

uint8_t fl_wave_count(const fl_state_t *state)
{
    return fl_level_of(state)->wave_count;
}

void fl_panel_button_rect(int button, int *x, int *y, int *w, int *h)
{
    *x = FL_PANEL_BTN_X0 + button * (FL_PANEL_BTN_W + FL_PANEL_BTN_GAP);
    *y = FL_PANEL_BTN_Y;
    *w = FL_PANEL_BTN_W;
    *h = FL_PANEL_BTN_H;
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

uint32_t fl_events_take(fl_state_t *state)
{
    if (!state) {
        return 0;
    }
    uint32_t events = state->events;
    state->events = 0;
    return events;
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
    if (!fl_tower_available(state, kind)) {
        return false;
    }
    const fl_tower_level_t *stat = &FL_TOWER_LEVELS[kind][0];
    if (state->gold < stat->cost) {
        fl_raise(state, FL_EVENT_DENIED);
        return false;
    }
    int spot = fl_spot_index(state, x, y);
    if (spot < 0 || fl_tower_at_spot(state, spot) >= 0) {
        return false;
    }
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        fl_tower_t *tower = &state->towers[i];
        if (tower->active) {
            continue;
        }
        tower->kind = kind;
        tower->x = fl_level_of(state)->spots[spot].x;
        tower->y = fl_level_of(state)->spots[spot].y;
        tower->level = 1;
        tower->mod_mask = 0;
        tower->cooldown = 0;
        tower->invested = stat->cost;
        tower->shot_tick = 255;
        tower->frozen_ticks = 0;
        /* A hall musters on the stretch of road nearest to it. */
        fl_nearest_road_point(state, (float)tower->x, (float)tower->y,
                              &tower->rally_x, &tower->rally_y);
        tower->active = true;
        state->gold = (uint16_t)(state->gold - stat->cost);
        fl_raise(state, FL_EVENT_BUILT);
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
    /* A sold hall takes its garrison with it, at once, so a tower built on
     * the same slot never inherits them. */
    for (int s = 0; s < FL_MAX_SOLDIERS; ++s) {
        if (state->soldiers[s].active && state->soldiers[s].tower == index) {
            state->soldiers[s].active = 0;
        }
    }
    fl_raise(state, FL_EVENT_SOLD);
    return true;
}

bool fl_build_hero(fl_state_t *state, int16_t x, int16_t y, fl_hero_kind_t kind)
{
    if (kind < 0 || kind >= FL_HERO_KIND_COUNT) {
        return false;
    }
    const fl_hero_stat_t *stat = &FL_HERO_STATS[kind];
    if (state->gold < stat->cost) {
        fl_raise(state, FL_EVENT_DENIED);
        return false;
    }
    /* The nearest free post takes the hero. */
    const fl_level_t *level = fl_level_of(state);
    int post = -1;
    float best = 0.0f;
    for (int i = 0; i < level->post_count && i < FL_MAX_HEROES; ++i) {
        if (state->heroes[i].active) {
            continue;
        }
        float dx = (float)x - (float)level->posts[i].x;
        float dy = (float)y - (float)level->posts[i].y;
        float distance = dx * dx + dy * dy;
        if (post >= 0 && distance >= best) {
            continue;
        }
        best = distance;
        post = i;
    }
    if (post < 0) {
        return false;
    }
    fl_hero_t *hero = &state->heroes[post];
    hero->kind = kind;
    hero->x = level->posts[post].x;
    hero->y = level->posts[post].y;
    hero->max_hp = (uint16_t)stat->hp;
    hero->hp = hero->max_hp;
    hero->cooldown = 0;
    hero->skill_timer = 0;
    hero->respawn_timer = 0;
    hero->active = 1;
    hero->alive = 1;
    state->gold = (uint16_t)(state->gold - stat->cost);
    fl_raise(state, FL_EVENT_HERO_PLACED);
    return true;
}

bool fl_cast_hero_skill(fl_state_t *state, int index)
{
    if (!state || index < 0 || index >= FL_MAX_HEROES) {
        return false;
    }
    fl_hero_t *hero = &state->heroes[index];
    if (!hero->active || !hero->alive || hero->skill_timer > 0) {
        return false;
    }
    const fl_hero_stat_t *stat = &FL_HERO_STATS[hero->kind];
    float radius2 = (float)stat->skill_radius * (float)stat->skill_radius;
    fl_shot_t shot;
    shot.damage = stat->skill_damage;
    shot.aoe = 0;
    shot.flags = 0;
    shot.slow = true;
    shot.slow_percent = FL_SLOW_PERCENT;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        fl_enemy_t *enemy = &state->enemies[i];
        if (!fl_targetable(enemy)) {
            continue;
        }
        float dx = enemy->x - (float)hero->x;
        float dy = enemy->y - (float)hero->y;
        if (dx * dx + dy * dy > radius2) {
            continue;
        }
        fl_damage_enemy(state, enemy, &shot);
        if (enemy->alive && !fl_resists_control(enemy)) {
            enemy->slow_ticks = stat->skill_slow_ticks;
            enemy->slow_percent = FL_SLOW_PERCENT;
        }
    }
    hero->skill_timer = stat->skill_cooldown;
    fl_raise(state, FL_EVENT_NOVA);
    return true;
}

int fl_star_rating(const fl_state_t *state)
{
    if (!state || state->phase != FL_PHASE_WON) {
        return 0;
    }
    /* Three stars for keeping three quarters of the gate, two for two
     * fifths; with 20 lives that is 15 and 8. */
    uint32_t lives = state->lives;
    uint32_t start = state->start_lives ? state->start_lives : 20u;
    if (lives * 4u >= start * 3u) {
        return 3;
    }
    if (lives * 5u >= start * 2u) {
        return 2;
    }
    return 1;
}

static bool fl_inside(int32_t x, int32_t y, int bx, int by, int bw, int bh)
{
    return x >= bx && x < bx + bw && y >= by && y < by + bh;
}

/* Which panel button a tap landed on, or -1. */
static int fl_panel_button_at(int32_t x, int32_t y)
{
    for (int i = 0; i < FL_PANEL_BTN_COUNT; ++i) {
        int bx, by, bw, bh;
        fl_panel_button_rect(i, &bx, &by, &bw, &bh);
        if (fl_inside(x, y, bx, by, bw, bh)) {
            return i;
        }
    }
    return -1;
}

/* The campaign map: a tap on a node picks it, the FIGHT button starts it. */
static void fl_menu_tap(fl_state_t *state, int32_t x, int32_t y)
{
    if (fl_inside(x, y, FL_MENU_GO_X, FL_MENU_GO_Y, FL_MENU_GO_W, FL_MENU_GO_H)) {
        fl_state_start_level(state, state->menu_level);
        return;
    }
    for (uint8_t i = 0; i < FL_LEVEL_COUNT; ++i) {
        float dx = (float)(x - FL_LEVELS[i].node.x);
        float dy = (float)(y - FL_LEVELS[i].node.y);
        if (dx * dx + dy * dy <= (float)FL_NODE_RADIUS * FL_NODE_RADIUS
                && fl_level_unlocked(state, i)) {
            state->menu_level = i;
            return;
        }
    }
}

/* The end-of-run panel: back to the map, fight again, or march on. */
static void fl_end_tap(fl_state_t *state, int32_t x, int32_t y)
{
    switch (fl_panel_button_at(x, y)) {
    case FL_PANEL_BTN_MAP:
        fl_state_open_menu(state);
        break;
    case FL_PANEL_BTN_RETRY:
        fl_state_start_level(state, state->level);
        break;
    case FL_PANEL_BTN_NEXT:
        /* Only a won run offers the next level, and only if there is one. */
        if (state->phase == FL_PHASE_WON) {
            fl_state_start_level(state, (uint8_t)(state->level + 1));
        }
        break;
    default:
        break;
    }
}

/* The pause panel: the same row, with "resume" in the last slot. */
static void fl_pause_tap(fl_state_t *state, int32_t x, int32_t y)
{
    switch (fl_panel_button_at(x, y)) {
    case FL_PANEL_BTN_MAP:
        fl_state_open_menu(state);
        break;
    case FL_PANEL_BTN_RETRY:
        fl_state_start_level(state, state->level);
        break;
    case FL_PANEL_BTN_NEXT:
        state->paused = false;
        break;
    default:
        break;
    }
}

/* ------------------------------------------------------------------ */
/* Build ring                                                          */
/* ------------------------------------------------------------------ */

/* What the open ring offers: every tower of the level on a pad, the hero
 * on a post. */
static int fl_ring_entries(const fl_state_t *state, uint8_t out[FL_RING_MAX])
{
    if (state->ring_post >= 0) {
        out[0] = FL_RING_HERO;
        return 1;
    }
    if (state->ring_spot < 0) {
        return 0;
    }
    int count = 0;
    for (int kind = FL_TOWER_GUARD; kind < FL_TOWER_KIND_COUNT
                                    && count < FL_RING_MAX; ++kind) {
        if (fl_tower_available(state, (fl_tower_kind_t)kind)) {
            out[count++] = (uint8_t)kind;
        }
    }
    return count;
}

int fl_ring_count(const fl_state_t *state)
{
    uint8_t entries[FL_RING_MAX];
    return fl_ring_entries(state, entries);
}

uint8_t fl_ring_entry(const fl_state_t *state, int index)
{
    uint8_t entries[FL_RING_MAX];
    int count = fl_ring_entries(state, entries);
    return (index >= 0 && index < count) ? entries[index] : FL_TOWER_NONE;
}

uint16_t fl_ring_cost(const fl_state_t *state, int index)
{
    uint8_t entry = fl_ring_entry(state, index);
    if (entry == FL_RING_HERO) {
        return FL_HERO_STATS[FL_HERO_WARDEN].cost;
    }
    if (entry == FL_TOWER_NONE) {
        return 0;
    }
    return FL_TOWER_LEVELS[entry][0].cost;
}

void fl_ring_anchor(const fl_state_t *state, int *x, int *y)
{
    const fl_level_t *level = fl_level_of(state);
    const fl_point_t *at = &level->spots[0];
    if (state->ring_post >= 0) {
        at = &level->posts[state->ring_post];
    } else if (state->ring_spot >= 0) {
        at = &level->spots[state->ring_spot];
    }
    *x = at->x;
    *y = at->y;
}

int fl_ring_radius(int count)
{
    if (count > 5) {
        return FL_RING_RADIUS_FULL;
    }
    return count > 3 ? FL_RING_RADIUS_WIDE : FL_RING_RADIUS;
}

void fl_ring_center(const fl_state_t *state, int *x, int *y)
{
    int ax, ay;
    fl_ring_anchor(state, &ax, &ay);
    /* Slide the ring inwards until every medallion clears the glass edge
     * and the HUD. Clamping the centre is enough: the corners' curves stay
     * outside a ring this small at any of the allowed centres. */
    int reach = fl_ring_radius(fl_ring_count(state)) + FL_RING_ICON_R
                + FL_RING_EDGE;
    int min_x = reach;
    int max_x = FL_VIEW_WIDTH - reach;
    int min_y = FL_HUD_Y + FL_HUD_H + reach;
    int max_y = FL_VIEW_HEIGHT - reach;
    *x = ax < min_x ? min_x : (ax > max_x ? max_x : ax);
    *y = ay < min_y ? min_y : (ay > max_y ? max_y : ay);
}

void fl_ring_slot(const fl_state_t *state, int index, int *x, int *y)
{
    int count = fl_ring_count(state);
    int cx, cy;
    fl_ring_center(state, &cx, &cy);
    if (count <= 0) {
        *x = cx;
        *y = cy;
        return;
    }
    /* Evenly round the circle, the first medallion at twelve o'clock. */
    float angle = -1.5707963f + (float)index * 6.2831853f / (float)count;
    float radius = (float)fl_ring_radius(count);
    *x = cx + (int)lroundf(cosf(angle) * radius);
    *y = cy + (int)lroundf(sinf(angle) * radius);
}

static void fl_ring_close(fl_state_t *state)
{
    state->ring_spot = -1;
    state->ring_post = -1;
    state->ring_pick = -1;
}

/* The post a tap landed on, when no hero holds it yet; -1 otherwise. */
static int fl_free_post_at(const fl_state_t *state, int32_t x, int32_t y)
{
    const fl_level_t *level = fl_level_of(state);
    for (int i = 0; i < level->post_count && i < FL_MAX_HEROES; ++i) {
        float dx = (float)(x - level->posts[i].x);
        float dy = (float)(y - level->posts[i].y);
        if (dx * dx + dy * dy <= (float)FL_BUILD_RADIUS * FL_BUILD_RADIUS
                && !state->heroes[i].active) {
            return i;
        }
    }
    return -1;
}

/* While the ring is open it has the first say over a tap. The first tap on
 * a medallion picks it, the second builds it; a tap on the pad itself
 * closes the ring. Returns false when the tap missed the ring, which then
 * closes and lets the tap fall through to whatever lies beneath. */
static bool fl_ring_tap(fl_state_t *state, int32_t x, int32_t y)
{
    int count = fl_ring_count(state);
    int hit = -1;
    for (int i = 0; i < count; ++i) {
        int sx, sy;
        fl_ring_slot(state, i, &sx, &sy);
        float dx = (float)(x - sx);
        float dy = (float)(y - sy);
        /* A little slack beyond the drawn disc: fingers are wider than it. */
        float reach = (float)(FL_RING_ICON_R + 4);
        if (dx * dx + dy * dy <= reach * reach) {
            hit = i;
            break;
        }
    }
    if (hit < 0) {
        int ax, ay;
        fl_ring_anchor(state, &ax, &ay);
        float dx = (float)(x - ax);
        float dy = (float)(y - ay);
        fl_ring_close(state);
        return dx * dx + dy * dy <= (float)FL_BUILD_RADIUS * FL_BUILD_RADIUS;
    }
    if (state->ring_pick != hit) {
        state->ring_pick = (int8_t)hit;
        return true;
    }
    int ax, ay;
    fl_ring_anchor(state, &ax, &ay);
    uint8_t entry = fl_ring_entry(state, hit);
    bool built = entry == FL_RING_HERO
        ? fl_build_hero(state, (int16_t)ax, (int16_t)ay, FL_HERO_WARDEN)
        : fl_build_tower(state, (int16_t)ax, (int16_t)ay, (fl_tower_kind_t)entry);
    /* Short of gold the ring stays open: the denial sound says why. */
    if (built) {
        fl_ring_close(state);
    }
    return true;
}

void fl_set_pointer(fl_state_t *state, int32_t x, int32_t y, bool down)
{
    bool pressed = down && !state->pointer_down_prev;
    state->pointer_down_prev = down;
    if (!pressed) {
        return;
    }

    if (state->phase == FL_PHASE_MENU) {
        fl_menu_tap(state, x, y);
        return;
    }
    if (state->phase == FL_PHASE_WON || state->phase == FL_PHASE_LOST) {
        fl_end_tap(state, x, y);
        return;
    }
    /* While paused, only the pause button and the pause panel listen. */
    if (state->paused && !(y < FL_HUD_HEIGHT && x >= FL_PAUSE_BTN_X
                           && x < FL_PAUSE_BTN_X + FL_PAUSE_BTN_W)) {
        fl_pause_tap(state, x, y);
        return;
    }

    if (y < FL_HUD_HEIGHT) {
        if (x >= FL_SKILL_BTN_X && x < FL_SKILL_BTN_X + FL_SKILL_BTN_W
                && y >= FL_SKILL_BTN_Y && y < FL_SKILL_BTN_Y + FL_SKILL_BTN_H) {
            for (int i = 0; i < FL_MAX_HEROES; ++i) {
                if (state->heroes[i].active && state->heroes[i].alive) {
                    fl_cast_hero_skill(state, i);
                    break;
                }
            }
            return;
        }
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

    if (fl_ring_count(state) > 0 && fl_ring_tap(state, x, y)) {
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
            return;
        }
    }

    /* An empty pad opens the build ring; a free post opens the hero's. */
    state->selected_tower = -1;
    int spot = fl_spot_index(state, (int16_t)x, (int16_t)y);
    if (spot >= 0 && fl_tower_at_spot(state, spot) < 0) {
        state->ring_spot = (int8_t)spot;
        state->ring_pick = -1;
        return;
    }
    int post = fl_free_post_at(state, x, y);
    if (post >= 0) {
        state->ring_post = (int8_t)post;
        state->ring_pick = -1;
    }
}

const char *fl_phase_name(const fl_state_t *state)
{
    switch (state->phase) {
    case FL_PHASE_PREP: return "prep";
    case FL_PHASE_WAVE: return "wave";
    case FL_PHASE_WON: return "won";
    case FL_PHASE_LOST: return "lost";
    case FL_PHASE_MENU: return "menu";
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
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        FL_MIX(state->heroes[i].active);
        FL_MIX(state->heroes[i].alive);
        FL_MIX(state->heroes[i].hp);
        FL_MIX(state->heroes[i].skill_timer);
    }
    FL_MIX(state->level);
    FL_MIX(state->menu_level);
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
        FL_MIX(state->towers[i].frozen_ticks);
    }
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        if (!state->enemies[i].alive) {
            continue;
        }
        FL_MIX((uint32_t)(state->enemies[i].hp * 100.0f));
        FL_MIX((uint32_t)(state->enemies[i].dist * 100.0f));
        FL_MIX(state->enemies[i].kind);
        FL_MIX(state->enemies[i].burrowed);
        FL_MIX(state->enemies[i].stage);
        FL_MIX(state->enemies[i].ward_ticks);
    }
    for (int i = 0; i < FL_MAX_SOLDIERS; ++i) {
        if (!state->soldiers[i].active) {
            continue;
        }
        FL_MIX(state->soldiers[i].alive);
        FL_MIX(state->soldiers[i].hp);
        FL_MIX((uint32_t)(state->soldiers[i].x * 100.0f));
    }
    for (int i = 0; i < FL_MAX_ZONES; ++i) {
        if (state->zones[i].active) {
            FL_MIX(state->zones[i].ticks);
        }
    }
    FL_MIX((uint8_t)state->ring_spot);
    FL_MIX((uint8_t)state->ring_post);
    FL_MIX((uint8_t)state->ring_pick);
#undef FL_MIX
    return hash;
}

int fl_state_json(const fl_state_t *state, char *output, size_t capacity)
{
    return fl_state_json_sfx(state, NULL, output, capacity);
}

/* The Host preview picks up the sound to play from the state JSON. The name
 * is supplied by the runtime, so the model only carries it. */
int fl_state_json_sfx(const fl_state_t *state, const char *sfx,
                      char *output, size_t capacity)
{
    if (!output || !capacity) {
        return -1;
    }
    const char *sound = (sfx && sfx[0]) ? sfx : "";
    int size = snprintf(output, capacity,
                        "{\"phase\":\"%s\",\"level\":%u,\"menu_level\":%u,"
                        "\"paused\":%s,\"ring_spot\":%d,\"ring_post\":%d,"
                        "\"ring_pick\":%d,\"selected_tower\":%d,"
                        "\"wave\":%u,\"gold\":%u,\"lives\":%u,"
                        "\"score\":%lu,\"score_kills\":%lu,\"score_waves\":%lu,"
                        "\"score_bonus\":%lu,\"leaks\":%u,"
                        "\"kills\":%lu,\"tick\":%lu,\"speed\":%u,"
                        "\"sfx\":\"%s\",\"state_hash\":\"%08lx\"}",
                        fl_phase_name(state), (unsigned)state->level,
                        (unsigned)state->menu_level,
                        state->paused ? "true" : "false",
                        (int)state->ring_spot, (int)state->ring_post,
                        (int)state->ring_pick, (int)state->selected_tower,
                        (unsigned)state->wave,
                        (unsigned)state->gold, (unsigned)state->lives,
                        (unsigned long)state->score,
                        (unsigned long)state->score_kills,
                        (unsigned long)state->score_waves,
                        (unsigned long)state->score_bonus,
                        (unsigned)state->leaks,
                        (unsigned long)state->kills, (unsigned long)state->tick,
                        (unsigned)state->speed, sound,
                        (unsigned long)fl_state_hash(state));
    return (size < 0 || (size_t)size >= capacity) ? -1 : size;
}

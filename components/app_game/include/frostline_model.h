/* Pure game model for Frostline Hold.
 *
 * This component never touches the screen, touch input, audio or any other
 * hardware. It receives pointer commands and advances a fixed-step simulation,
 * so the same code produces the same result in the Host simulator and on the
 * device.
 *
 * SPDX-License-Identifier: Apache-2.0 */
#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FL_MAX_TOWERS 32
#define FL_MAX_ENEMIES 96
#define FL_MAX_PROJECTILES 128

/* Screen layout. The map covers the whole panel; the HUD and build bar are
 * drawn on top of it. */
#define FL_VIEW_WIDTH 480
#define FL_VIEW_HEIGHT 480
#define FL_HUD_HEIGHT 44
#define FL_BAR_TOP 412
#define FL_BAR_HEIGHT 68
#define FL_TOWER_CARD_COUNT 3
#define FL_TOWER_CARD_W 96
#define FL_TOWER_CARD_H 56
#define FL_TOWER_CARD_Y (FL_BAR_TOP + 6)
#define FL_TOWER_CARD_GAP 102
#define FL_SPEED_BTN_X 428
#define FL_SPEED_BTN_W 44
#define FL_PAUSE_BTN_X 382
#define FL_PAUSE_BTN_W 42
#define FL_BTN_Y 8
#define FL_BTN_H 28

#define FL_BUILD_RADIUS 26      /* how close a tap must be to a build spot */
#define FL_TOWER_SELL_RATIO 0.5f

/* The upgrade panel covers the bottom of the screen when a tower is picked. */
#define FL_PANEL_TOP 392
#define FL_PANEL_HEIGHT 88
#define FL_UPGRADE_CARD_W 186
#define FL_UPGRADE_CARD_H 54
#define FL_UPGRADE_CARD_Y 420
#define FL_UPGRADE_CARD_X0 14
#define FL_UPGRADE_CARD_GAP 20

typedef enum {
    FL_TOWER_NONE = 0,
    FL_TOWER_GUARD = 1,
    FL_TOWER_FROST = 2,
    FL_TOWER_SIEGE = 3,
} fl_tower_kind_t;

typedef enum {
    FL_ENEMY_WOLF = 0,
    FL_ENEMY_RAIDER = 1,
    FL_ENEMY_SKITTER = 2,
    FL_ENEMY_HUSK = 3,
    FL_ENEMY_SHAMAN = 4,
    FL_ENEMY_BANNERMAN = 5,
    FL_ENEMY_SAP = 6,
    FL_ENEMY_TITAN = 7,
} fl_enemy_kind_t;

/* What an enemy does beyond walking. */
#define FL_BEHAVIOUR_PLAIN   0
#define FL_BEHAVIOUR_HEALER  1
#define FL_BEHAVIOUR_BANNER  2
#define FL_BEHAVIOUR_SAP     3
#define FL_BEHAVIOUR_TITAN   4

typedef enum {
    FL_PHASE_PREP = 0,   /* countdown to the next wave */
    FL_PHASE_WAVE = 1,   /* wave in progress */
    FL_PHASE_WON = 2,
    FL_PHASE_LOST = 3,
} fl_phase_t;

typedef struct {
    int16_t x, y;
} fl_point_t;

typedef struct {
    fl_tower_kind_t kind;
    int16_t x, y;
    uint8_t level;       /* 1..3 */
    /* One bit per upgrade option: bit 0/1 is the level 1->2 choice, bit 2/3
     * the level 2->3 choice. Exactly one bit per stage is ever set. */
    uint8_t mod_mask;
    uint16_t cooldown;   /* ticks until the next shot */
    uint32_t invested;   /* total gold spent, for the sell refund */
    bool active;
} fl_tower_t;

typedef struct {
    fl_enemy_kind_t kind;
    float x, y;
    float hp, max_hp;
    int16_t armor;
    uint8_t lane;        /* 0 = lane A, 1 = lane B */
    float dist;          /* distance travelled along the lane */
    uint16_t slow_ticks; /* remaining slow */
    uint16_t slow_percent; /* enemy speed percent while slowed */
    uint16_t burn_ticks; /* remaining burn */
    uint16_t burn_damage;/* damage per second while burning */
    uint16_t stun_ticks; /* remaining stun */
    uint16_t cast_timer; /* ticks until the next heal or banner pulse */
    uint16_t buff_ticks; /* how long the banner's hardening lasts */
    int16_t armor_bonus; /* armour granted by a banner */
    uint8_t behaviour;   /* FL_BEHAVIOUR_* */
    uint8_t alive;
} fl_enemy_t;

typedef struct {
    uint8_t kind;        /* tower kind that fired it */
    uint8_t target;      /* enemy index, 0xFF when the target died */
    float x, y;
    float tx, ty;        /* current target position */
    int16_t damage;
    int16_t aoe;         /* blast radius in px, 0 for a single target */
    uint16_t slow_percent;/* enemy speed percent this shot applies */
    uint16_t speed;      /* px per second */
    uint8_t flags;       /* FL_SHOT_* bits */
    uint8_t ttl;
    uint8_t active;
} fl_projectile_t;

typedef struct {
    uint16_t gold;
    uint8_t lives;
    uint32_t score;       /* points, running total */
    uint32_t score_kills; /* points earned by killing enemies */
    uint32_t score_waves; /* points earned by clearing waves */
    uint32_t score_bonus; /* end-of-run bonuses: remaining lives, no leaks */
    uint16_t leaks;       /* enemies that reached the gate */
    uint8_t wave;          /* 1-based; FL_WAVE_COUNT + 1 means finished */
    uint8_t phase;
    uint8_t speed;         /* 1, 2 or 3 */
    uint8_t selected_kind; /* tower kind armed in the build bar */
    int8_t selected_tower; /* index of the inspected tower, -1 when none */
    uint32_t tick;
    uint32_t seed;
    uint32_t kills;
    uint32_t prep_ticks;   /* ticks left in the preparation phase */
    uint8_t batch_index;   /* index into the current wave's batch list */
    uint8_t batch_spawned; /* enemies already spawned from that batch */
    uint8_t spawn_timer;
    uint8_t wave_active;
    bool pointer_down_prev;
    bool paused;
    fl_tower_t towers[FL_MAX_TOWERS];
    fl_enemy_t enemies[FL_MAX_ENEMIES];
    fl_projectile_t projectiles[FL_MAX_PROJECTILES];
} fl_state_t;

/* --- lifecycle ------------------------------------------------------- */
/** Reset to the start of wave 1 with the given PRNG seed. */
void fl_state_reset(fl_state_t *state, uint32_t seed);
/** Advance the simulation by one call (speed multiplier applied inside). */
void fl_state_update(fl_state_t *state);

/* --- input ----------------------------------------------------------- */
/** Feed a pointer sample. Press edges are detected internally. */
void fl_set_pointer(fl_state_t *state, int32_t x, int32_t y, bool down);
/** Cycle the simulation speed between 1x, 2x and 3x. */
void fl_toggle_speed(fl_state_t *state);
/** Toggle the paused flag. */
void fl_toggle_pause(fl_state_t *state);

/* Shot flags carried from the tower that fired. */
#define FL_SHOT_PIERCE 0x01
#define FL_SHOT_BURN   0x02
#define FL_SHOT_STUN   0x04

/* The stats a tower actually shoots with, after its chosen upgrades. */
typedef struct {
    int16_t damage;
    int16_t range;
    uint16_t cooldown;
    int16_t aoe;
    uint16_t projectile_speed;
    uint16_t slow_percent;  /* enemy speed percent while slowed */
    uint8_t pierce;
    uint8_t burn;
    uint8_t stun;
    uint8_t double_shot;    /* the shot also hits a second enemy */
} fl_tower_stats_t;

/* --- gameplay -------------------------------------------------------- */
/** The stats of a tower with the upgrades it has taken into account. */
fl_tower_stats_t fl_tower_stats(const fl_tower_t *tower);
/** What it costs to reach the next level; 0 when the tower is maxed. */
uint16_t fl_upgrade_cost(const fl_state_t *state, int index);
/** Take one of the two upgrades offered at the next stage. */
bool fl_upgrade_tower(fl_state_t *state, int index, int option);
/** The name of the option offered at a stage, e.g. "BALLISTA". */
const char *fl_upgrade_name(fl_tower_kind_t kind, int stage, int option);
/** Which option was taken at a stage: 0 or 1, or -1 when none was. */
int fl_tower_option(const fl_tower_t *tower, int stage);
/** Build a tower of the given kind at the nearest free build spot. */
bool fl_build_tower(fl_state_t *state, int16_t x, int16_t y, fl_tower_kind_t kind);
/** Sell the tower at the given index and refund half of what was spent. */
bool fl_sell_tower(fl_state_t *state, int index);

/* --- introspection --------------------------------------------------- */
uint32_t fl_state_hash(const fl_state_t *state);
int fl_state_json(const fl_state_t *state, char *output, size_t capacity);
const char *fl_phase_name(const fl_state_t *state);

/* --- score ------------------------------------------------------------ */
/* Points awarded per kill and per cleared wave come from the level data.
 * The end-of-run bonus pays for the lives still standing and, when nothing
 * ever reached the gate, a flawless bonus. */
#define FL_SCORE_PER_LIFE 100
#define FL_SCORE_FLAWLESS 1000

#ifdef __cplusplus
}
#endif

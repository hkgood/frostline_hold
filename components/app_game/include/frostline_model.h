/* Pure game model for Frostline Hold.
 *
 * This component never touches the screen, touch input, audio or any other
 * hardware. It receives pointer commands and advances a fixed-step simulation,
 * so the same code produces the same result in the Host simulator and on the
 * device.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
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
#define FL_MAX_HEROES 2
#define FL_MAX_SOLDIERS 24      /* barracks garrisons, all halls together */
#define FL_MAX_ZONES 16         /* burning patches left by ember pots */
#define FL_MAX_ARCS 12          /* lightning and rune beams being drawn */
#define FL_ARC_POINTS 7         /* the tower plus up to six struck enemies */
#define FL_ARC_TICKS 8          /* how long a bolt or beam stays drawn */

/* Screen layout. The map covers the whole panel; the HUD is drawn on top of
 * it, and the build ring and upgrade panel open over it on demand.
 *
 * The Mosaico glass cuts a 60px quarter-circle out of each corner. Every
 * control lives inside that rounded rectangle, so a tap on the glass is a
 * tap on a real button. */
#define FL_VIEW_WIDTH 480
#define FL_VIEW_HEIGHT 480
#define FL_CORNER_RADIUS 60
#define FL_HUD_HEIGHT 44
#define FL_HUD_X 36
#define FL_HUD_Y 6
#define FL_HUD_W 408
#define FL_HUD_H 34
/* The hero's frost nova button sits inside the top-left curve. */
#define FL_SKILL_BTN_X 44
#define FL_SKILL_BTN_Y 9
#define FL_SKILL_BTN_W 30
#define FL_SKILL_BTN_H 28
#define FL_SPEED_BTN_X 400
#define FL_SPEED_BTN_W 44
#define FL_PAUSE_BTN_X 356
#define FL_PAUSE_BTN_W 36
#define FL_BTN_Y 9
#define FL_BTN_H 28

#define FL_BUILD_RADIUS 26      /* how close a tap must be to a build spot */
#define FL_TOWER_SELL_RATIO 0.5f

/* The build ring: tapping an empty pad (or a free hero post) opens a ring
 * of medallions round it, one per tower the level offers. The first tap on
 * a medallion picks it and shows its reach; a second tap builds. Near the
 * glass edge the whole ring slides inwards so every medallion stays on the
 * panel and clear of the HUD. */
#define FL_RING_MAX 8           /* entries a ring can hold */
#define FL_RING_RADIUS 50       /* ring of three or fewer */
#define FL_RING_RADIUS_WIDE 60  /* ring of four or five */
#define FL_RING_RADIUS_FULL 68  /* ring of six or more */
#define FL_RING_ICON_R 20       /* a medallion's touch radius */
#define FL_RING_EDGE 6          /* gap kept between a medallion and the glass */
#define FL_RING_HERO 0x80       /* ring entry code for the hero */

/* The upgrade panel covers the bottom of the screen when a tower is picked. */
#define FL_PANEL_X 36
#define FL_PANEL_TOP 404
#define FL_PANEL_W 408
#define FL_PANEL_HEIGHT 66
#define FL_UPGRADE_CARD_W 156
#define FL_UPGRADE_CARD_H 40
#define FL_UPGRADE_CARD_Y 426
#define FL_UPGRADE_CARD_X0 56
#define FL_UPGRADE_CARD_GAP 16

/* The campaign. A level index is 0-based; the data lives in FL_LEVELS. */
#define FL_MAX_LEVELS 24        /* slots in the progress record */
#define FL_NODE_RADIUS 30       /* how close a tap must be to a map node */
#define FL_DRAG_SLOP 8          /* px a press travels before it drags */
enum { FL_DRAG_NONE, FL_DRAG_PRESSED, FL_DRAG_MOVING };

/* The campaign map's level card, inside the bottom of the rounded glass. */
#define FL_MENU_CARD_X 64
#define FL_MENU_CARD_Y 368
#define FL_MENU_CARD_W 352
#define FL_MENU_CARD_H 64
#define FL_MENU_GO_X 316        /* the FIGHT button inside the card */
#define FL_MENU_GO_Y 378
#define FL_MENU_GO_W 88
#define FL_MENU_GO_H 44

/* The end-of-run and pause panels share one row of three buttons:
 * the map, a retry, and either the next level or "resume". */
#define FL_PANEL_BTN_COUNT 3
#define FL_PANEL_BTN_X0 84
#define FL_PANEL_BTN_Y 332
#define FL_PANEL_BTN_W 96
#define FL_PANEL_BTN_H 40
#define FL_PANEL_BTN_GAP 12

/* The title screen's two buttons, stacked under the logo. */
#define FL_TITLE_BTN_X 150
#define FL_TITLE_BTN_W 180
#define FL_TITLE_BTN_H 44
#define FL_TITLE_START_Y 368
#define FL_TITLE_SETTINGS_Y 420

/* The campaign map's way back to the title, in the open sky top right. */
#define FL_MENU_BACK_X 352
#define FL_MENU_BACK_Y 62
#define FL_MENU_BACK_W 80
#define FL_MENU_BACK_H 34

/* The settings screen: one row per setting, the back button below. A
 * volume row has a minus and a plus button round its level bar; the other
 * rows have one wide button. */
#define FL_SET_ROW_Y0 96
#define FL_SET_ROW_GAP 48
#define FL_SET_ROW_H 40
#define FL_SET_LABEL_X 64
#define FL_SET_MINUS_X 236
#define FL_SET_PLUS_X 376
#define FL_SET_STEP_W 44
#define FL_SET_WIDE_X 236
#define FL_SET_WIDE_W 184
#define FL_SET_BACK_X 180
#define FL_SET_BACK_Y 390
#define FL_SET_BACK_W 120
#define FL_SET_BACK_H 40
#define FL_VOLUME_MAX 4

typedef enum {
    FL_SET_ROW_MUSIC = 0,
    FL_SET_ROW_SOUND,
    FL_SET_ROW_HAPTICS,
    FL_SET_ROW_LANGUAGE,
    FL_SET_ROW_THEME,
    FL_SET_ROW_RESET,
    FL_SET_ROW_COUNT,
} fl_settings_row_t;

/* The ending screen ignores taps until the story has had time to show. */
#define FL_ENDING_MIN_TICKS 90

typedef enum {
    FL_PANEL_BTN_MAP = 0,
    FL_PANEL_BTN_RETRY = 1,
    FL_PANEL_BTN_NEXT = 2,      /* "RESUME" on the pause panel */
} fl_panel_button_t;

/* Mirrors TOWER_ORDER in assets_src/artgen/tower_data.py. */
typedef enum {
    FL_TOWER_NONE = 0,
    FL_TOWER_GUARD = 1,
    FL_TOWER_FROST = 2,
    FL_TOWER_SIEGE = 3,
    FL_TOWER_STORM = 4,     /* lightning leaping between enemies */
    FL_TOWER_RUNE = 5,      /* a beam that ignores armour */
    FL_TOWER_BARRACKS = 6,  /* soldiers who hold the road */
    FL_TOWER_EMBER = 7,     /* a fire pot that leaves the road burning */
} fl_tower_kind_t;

typedef enum {
    FL_HERO_WARDEN = 0,
} fl_hero_kind_t;

typedef enum {
    FL_ENEMY_WOLF = 0,
    FL_ENEMY_RAIDER = 1,
    FL_ENEMY_SKITTER = 2,
    FL_ENEMY_HUSK = 3,
    FL_ENEMY_SHAMAN = 4,
    FL_ENEMY_BANNERMAN = 5,
    FL_ENEMY_SAP = 6,
    FL_ENEMY_TITAN = 7,
    FL_ENEMY_CROW = 8,      /* flies over soldiers, out of the siege's reach */
    FL_ENEMY_GOLEM = 9,     /* a stone shell that cracks at half health */
    FL_ENEMY_SLIME = 10,    /* bursts into three slimelets when it dies */
    FL_ENEMY_SLIMELET = 11,
    FL_ENEMY_BURROWER = 12, /* dives under the road now and then */
    FL_ENEMY_WYRM = 13,     /* the mine's boss: dives, then surfaces in a brood */
    FL_ENEMY_KING = 14,     /* the fortress boss: freezes towers, wards itself */
} fl_enemy_kind_t;

/* What an enemy does beyond walking. */
#define FL_BEHAVIOUR_PLAIN    0
#define FL_BEHAVIOUR_HEALER   1
#define FL_BEHAVIOUR_BANNER   2
#define FL_BEHAVIOUR_SAP      3
#define FL_BEHAVIOUR_TITAN    4
#define FL_BEHAVIOUR_FLYER    5
#define FL_BEHAVIOUR_GOLEM    6
#define FL_BEHAVIOUR_SPLITTER 7
#define FL_BEHAVIOUR_BURROWER 8
#define FL_BEHAVIOUR_WYRM     9
#define FL_BEHAVIOUR_KING     10

/* A batch's lane byte: the lane in bit 0, and in the top bit the mark of a
 * champion, the mini-boss that closes a chapter's opening level. A champion
 * is a tougher, richer copy of its kind that shrugs off slows and stuns. */
#define FL_LANE_MASK 0x01
#define FL_LANE_CHAMPION 0x80
#define FL_CHAMPION_HP 600       /* percent of its wave's health */
#define FL_CHAMPION_ARMOR 3
#define FL_CHAMPION_BOUNTY 5     /* times its kind's gold and points */

/* A flyer is drawn and struck this far above the road it follows. */
#define FL_FLY_HEIGHT 18
/* The arc kind of the king's freezing beam; tower kinds stay below it. */
#define FL_ARC_FREEZE 15

typedef enum {
    FL_PHASE_PREP = 0,   /* countdown to the next wave */
    FL_PHASE_WAVE = 1,   /* wave in progress */
    FL_PHASE_WON = 2,
    FL_PHASE_LOST = 3,
    FL_PHASE_MENU = 4,   /* the campaign map: pick a level */
    FL_PHASE_TITLE = 5,  /* the title screen the game boots into */
    FL_PHASE_SETTINGS = 6,
    FL_PHASE_ENDING = 7, /* after the last level is won */
} fl_phase_t;

/* What the player set on the settings screen. The model only keeps the
 * values; the runtime stores them and applies them to sound and haptics. */
typedef struct {
    uint8_t music;    /* 0..FL_VOLUME_MAX */
    uint8_t sound;    /* 0..FL_VOLUME_MAX */
    uint8_t haptics;  /* 0 or 1 */
    uint8_t lang;     /* 0 = Chinese, 1 = English */
    uint8_t theme;    /* FL_THEME_DAY or FL_THEME_NIGHT */
} fl_settings_t;

/* Day draws ink on paper; night swaps the two over the whole screen. */
#define FL_THEME_DAY 0
#define FL_THEME_NIGHT 1

typedef struct {
    int16_t x, y;
} fl_point_t;

/* A hero stands at a post and holds the road. */
typedef struct {
    fl_hero_kind_t kind;
    int16_t x, y;
    uint16_t hp;
    uint16_t max_hp;
    uint16_t cooldown;      /* ticks until the next shot */
    uint16_t skill_timer;   /* ticks until the skill is ready again */
    uint16_t respawn_timer; /* ticks until the hero comes back */
    uint8_t active;         /* a hero has been placed here */
    uint8_t alive;
} fl_hero_t;

typedef struct {
    fl_tower_kind_t kind;
    int16_t x, y;
    uint8_t level;       /* 1..3 */
    /* One bit per upgrade option: bit 0/1 is the level 1->2 choice, bit 2/3
     * the level 2->3 choice. Exactly one bit per stage is ever set. */
    uint8_t mod_mask;
    uint16_t cooldown;   /* ticks until the next shot */
    uint32_t invested;   /* total gold spent, for the sell refund */
    int16_t rally_x, rally_y; /* barracks: where its soldiers hold the road */
    uint8_t shot_tick;   /* ticks since it last fired, for the recoil */
    /* Towards its last target in eighths of a turn: 0 east, 2 south (the
     * screen's y grows down). Only the art turns with it. */
    uint8_t aim;
    uint16_t frozen_ticks; /* encased in the king's ice: it cannot fire */
    bool active;
} fl_tower_t;

/* A barracks soldier. It walks out of its hall to a place on the road, holds
 * up to FL_SOLDIER_HOLD enemies there, and walks out again after it falls. */
typedef struct {
    float x, y;
    uint16_t hp;
    uint16_t max_hp;
    uint16_t cooldown;      /* ticks until its next blow */
    uint16_t respawn_timer; /* ticks until it comes back */
    uint8_t tower;          /* index of its hall */
    uint8_t slot;           /* its place in the hall's formation */
    uint8_t active;
    uint8_t alive;
    uint8_t swing;          /* ticks since its last blow, for the animation */
    uint8_t moving;         /* still walking to its place, for the animation */
} fl_soldier_t;

/* A patch of road set burning by an ember pot. */
typedef struct {
    int16_t x, y;
    int16_t radius;
    int16_t damage;         /* per pulse, to every enemy inside */
    uint16_t ticks;         /* how long it still burns */
    uint16_t life;          /* how long it burned in all, for the fade */
    uint8_t slow;           /* the tar upgrade: it also slows */
    uint8_t active;
} fl_zone_t;

/* A bolt of lightning or a rune beam: the tower first, then every enemy it
 * struck. The model only records it; the view draws it while it fades. */
typedef struct {
    int16_t x[FL_ARC_POINTS];
    int16_t y[FL_ARC_POINTS];
    uint8_t count;
    uint8_t kind;           /* tower kind that fired it */
    uint8_t ttl;
    uint8_t seed;           /* so each bolt jags its own way */
} fl_arc_t;

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
    uint16_t attack_timer; /* ticks until the next hit on a hero */
    uint16_t buff_ticks; /* how long the banner's hardening lasts */
    int16_t armor_bonus; /* armour granted by a banner */
    uint16_t hex_ticks;  /* while > 0 every hit ignores its armour */
    uint16_t ward_ticks; /* while > 0 nothing harms it */
    /* Burrowers and the wyrm: ticks left in this spell above or below the
     * road; the king: ticks until it freezes the next tower. */
    uint16_t phase_ticks;
    uint8_t burrowed;    /* under the road: nothing can see or stop it */
    uint8_t stage;       /* golem: shell cracked; king: wards spent */
    uint8_t behaviour;   /* FL_BEHAVIOUR_* */
    uint8_t soldier;     /* 1-based soldier holding it, 0 when none */
    uint8_t champion;    /* a chapter's mini-boss */
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
    uint8_t zone_ticks;  /* ember: how long the fire it leaves burns */
    uint8_t active;
} fl_projectile_t;

typedef struct {
    uint16_t gold;
    uint8_t lives;
    uint32_t score;       /* points, running total */
    uint32_t score_kills; /* points earned by killing enemies */
    uint32_t score_waves; /* points earned by clearing waves */
    uint32_t score_bonus; /* end-of-run bonuses: remaining lives, no leaks */
    /* Campaign progress. It outlives a run: starting a level or opening the
     * map keeps it, and the runtime loads it from and saves it to the
     * device's record. */
    uint32_t level_best[FL_MAX_LEVELS];  /* best score per level */
    uint8_t level_stars[FL_MAX_LEVELS];  /* 0 = not cleared yet */
    uint8_t level;         /* the level being played, or last played */
    uint8_t menu_level;    /* the node picked on the campaign map */
    /* The campaign map is wider than the screen and drags sideways. A press
     * there may become a drag; a release that never dragged is a tap. */
    int16_t menu_scroll;   /* map x at the screen's left edge */
    int16_t drag_x;        /* screen x where the press began */
    int16_t drag_scroll;   /* menu_scroll when it began */
    uint8_t drag;          /* FL_DRAG_* */
    uint16_t run_serial;   /* counts started runs, so a run is saved once */
    uint8_t start_lives;   /* the lives this level grants, for the stars */
    uint16_t leaks;       /* enemies that reached the gate */
    uint8_t wave;          /* 1-based; wave_count + 1 means finished */
    uint8_t phase;
    uint8_t speed;         /* 1, 2 or 3 */
    int8_t selected_tower; /* index of the inspected tower, -1 when none */
    /* The build ring: open on a build spot or a hero post, -1 when closed,
     * and the entry picked by the first tap, -1 before it. */
    int8_t ring_spot;
    int8_t ring_post;
    int8_t ring_pick;
    uint32_t tick;
    uint32_t seed;
    uint32_t kills;
    uint32_t prep_ticks;   /* ticks left in the preparation phase */
    /* What happened during the last update, as FL_EVENT_* bits. The runtime
     * clears and consumes them once per frame. */
    uint32_t events;
    uint32_t event_serial;   /* counts the updates that raised anything */
    uint8_t last_kill_kind;  /* which enemy died last, for the kill sound */
    /* Settings outlive runs like the progress does. The serial counts
     * changes so the runtime knows when to store and apply them; the reset
     * serial counts progress wipes. */
    fl_settings_t settings;
    uint16_t settings_serial;
    uint16_t reset_serial;
    uint8_t reset_armed;     /* the first tap on "reset" asks again */
    uint16_t screen_ticks;   /* ticks spent on the title, map or ending */
    fl_hero_t heroes[FL_MAX_HEROES];
    uint8_t batch_index;   /* index into the current wave's batch list */
    uint8_t batch_spawned; /* enemies already spawned from that batch */
    uint8_t spawn_timer;
    uint8_t wave_active;
    bool pointer_down_prev;
    bool paused;
    fl_tower_t towers[FL_MAX_TOWERS];
    fl_enemy_t enemies[FL_MAX_ENEMIES];
    fl_projectile_t projectiles[FL_MAX_PROJECTILES];
    fl_soldier_t soldiers[FL_MAX_SOLDIERS];
    fl_zone_t zones[FL_MAX_ZONES];
    fl_arc_t arcs[FL_MAX_ARCS];
} fl_state_t;

/* --- lifecycle ------------------------------------------------------- */
/** Wipe everything, progress included, and start level 0 at wave 1. */
void fl_state_reset(fl_state_t *state, uint32_t seed);
/** Start a level at wave 1, keeping the campaign progress.
 *  Returns false when the level does not exist or is still locked. */
bool fl_state_start_level(fl_state_t *state, uint8_t level);
/** Leave the battle for the campaign map, keeping the progress. */
void fl_state_open_menu(fl_state_t *state);
/** Show the title screen, keeping the progress. */
void fl_state_open_title(fl_state_t *state);
/** The defaults: full sound, haptics on, Chinese. */
fl_settings_t fl_default_settings(void);
/** Where a settings row's buttons sit: `part` 0 is the minus (or the wide
 * button), 1 the plus. Returns false when the row has no such part. */
bool fl_settings_button_rect(int row, int part, int *x, int *y, int *w, int *h);
/** A level is open once the one before it has been cleared. */
bool fl_level_unlocked(const fl_state_t *state, uint8_t level);
/** How many levels the campaign has. */
uint8_t fl_level_count(void);
/** Whether the build ring offers this tower kind on the current level. */
bool fl_tower_available(const fl_state_t *state, fl_tower_kind_t kind);
/** The tower a level offers for the first time, or FL_TOWER_NONE. */
fl_tower_kind_t fl_new_tower(uint8_t level);
/* Which of its four looks a tower wears: level 1, level 2, or the level 3
 * branch its second upgrade chose. */
int fl_tower_variant(const fl_tower_t *tower);
/* Whether a catapult, which turns its whole frame, faces left. */
bool fl_tower_faces_left(const fl_tower_t *tower);
/* Where a tower's shot leaves it for its current aim: the ballista's bolt
 * tip, the catapult's bucket at the top of its swing, the spire's orb. The
 * art is drawn to the same rig (data/frostline_tower_rig.h). */
void fl_tower_muzzle(const fl_tower_t *tower, float *x, float *y);
/** Whether an enemy is in the air: no soldier, hero or siege can touch it. */
bool fl_enemy_flies(const fl_enemy_t *enemy);
/** The number of waves in the current level. */
uint8_t fl_wave_count(const fl_state_t *state);
/** Advance the simulation by one call (speed multiplier applied inside). */
void fl_state_update(fl_state_t *state);

/* --- input ----------------------------------------------------------- */
/** Feed a pointer sample. Press edges are detected internally. */
void fl_set_pointer(fl_state_t *state, int32_t x, int32_t y, bool down);
/** Cycle the simulation speed between 1x, 2x and 3x. */
void fl_toggle_speed(fl_state_t *state);
/** Toggle the paused flag. */
void fl_toggle_pause(fl_state_t *state);

/* --- build ring ------------------------------------------------------ */
/** How many medallions the open ring holds; 0 when it is closed. */
int fl_ring_count(const fl_state_t *state);
/** What a ring entry builds: a tower kind, or FL_RING_HERO. */
uint8_t fl_ring_entry(const fl_state_t *state, int index);
/** What a ring entry costs. */
uint16_t fl_ring_cost(const fl_state_t *state, int index);
/** The centre of the open ring, which may sit off its pad near an edge. */
/* How far the medallions sit from the ring's centre for a ring of `count`. */
int fl_ring_radius(int count);
void fl_ring_center(const fl_state_t *state, int *x, int *y);
/** Where a ring entry's medallion sits on screen. */
void fl_ring_slot(const fl_state_t *state, int index, int *x, int *y);
/** Where the open ring builds: the pad or post it was opened on. */
void fl_ring_anchor(const fl_state_t *state, int *x, int *y);

/* Shot flags carried from the tower that fired. */
#define FL_SHOT_PIERCE 0x01
#define FL_SHOT_BURN   0x02
#define FL_SHOT_STUN   0x04
#define FL_SHOT_HEX    0x08   /* strips the target's armour for a while */
#define FL_SHOT_SLOW   0x10   /* slows, whatever tower fired it */
#define FL_SHOT_ZONE   0x20   /* sets the road burning where it lands */

/* Events raised during a simulation step.
 *
 * The model itself stays free of any presentation concern: it only says what
 * happened. The runtime turns these bits into sound, haptics and effects, so
 * the same simulation drives the device and the Host simulator. */
#define FL_EVENT_BUILT       (1u << 0)   /* a tower went up */
#define FL_EVENT_UPGRADED    (1u << 1)   /* a tower took an upgrade */
#define FL_EVENT_SOLD        (1u << 2)   /* a tower was sold */
#define FL_EVENT_DENIED      (1u << 3)   /* not enough gold */
#define FL_EVENT_SHOT_GUARD  (1u << 4)   /* a guard tower fired */
#define FL_EVENT_SHOT_FROST  (1u << 5)   /* a frost tower fired */
#define FL_EVENT_SHOT_SIEGE  (1u << 6)   /* a siege tower fired */
#define FL_EVENT_HERO_SHOT   (1u << 7)   /* the hero struck */
#define FL_EVENT_IMPACT      (1u << 8)   /* an area blast went off */
#define FL_EVENT_KILLED      (1u << 9)   /* an enemy died */
#define FL_EVENT_LEAK        (1u << 10)  /* an enemy reached the gate */
#define FL_EVENT_WAVE_START  (1u << 11)  /* a wave began */
#define FL_EVENT_WAVE_CLEAR  (1u << 12)  /* a wave was cleared */
#define FL_EVENT_HERO_HIT    (1u << 13)  /* the hero was struck */
#define FL_EVENT_HERO_DOWN   (1u << 14)  /* the hero fell */
#define FL_EVENT_NOVA        (1u << 15)  /* the frost nova went off */
#define FL_EVENT_WON         (1u << 16)  /* the run was won */
#define FL_EVENT_LOST        (1u << 17)  /* the run was lost */
#define FL_EVENT_HERO_PLACED (1u << 18)  /* a hero took a post */
#define FL_EVENT_LEVEL_START (1u << 19)  /* a level began at wave 1 */
#define FL_EVENT_MENU        (1u << 20)  /* the campaign map opened */
#define FL_EVENT_SHOT_STORM  (1u << 21)  /* a storm spire loosed lightning */
#define FL_EVENT_SHOT_RUNE   (1u << 22)  /* a rune obelisk fired its beam */
#define FL_EVENT_SHOT_EMBER  (1u << 23)  /* an ember tower lobbed a pot */
#define FL_EVENT_CLASH       (1u << 24)  /* a soldier struck or was struck */
#define FL_EVENT_CRACK       (1u << 25)  /* a golem's shell broke */
#define FL_EVENT_SPLIT       (1u << 26)  /* a slime burst into slimelets */
#define FL_EVENT_BURROW      (1u << 27)  /* something dived or surfaced */
#define FL_EVENT_FREEZE      (1u << 28)  /* the king froze a tower */
#define FL_EVENT_BOSS        (1u << 29)  /* a boss arrived or called its brood */

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
    uint8_t hex;            /* the hit strips armour */
    uint8_t slow;           /* the hit, or the fire it leaves, slows */
    int16_t extra;          /* chain length, soldiers, or burn ticks */
    int16_t hp;             /* a soldier's health */
} fl_tower_stats_t;

/* --- gameplay -------------------------------------------------------- */
/** Take the events raised since the last call and clear them.
 *
 * Events accumulate across input and update, so a tower built by a tap and
 * the shot it fires in the same frame are both reported. */
uint32_t fl_events_take(fl_state_t *state);
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
/** Build a tower of the given kind at the build spot within reach of the
 *  point. The touch path goes through the build ring; this is the direct
 *  call it ends in. */
bool fl_build_tower(fl_state_t *state, int16_t x, int16_t y, fl_tower_kind_t kind);
/** Sell the tower at the given index and refund half of what was spent. */
bool fl_sell_tower(fl_state_t *state, int index);
/** Put a hero on the nearest free hero post. */
bool fl_build_hero(fl_state_t *state, int16_t x, int16_t y, fl_hero_kind_t kind);
/** Cast the hero's frost nova around it. */
bool fl_cast_hero_skill(fl_state_t *state, int index);
/** The stars this run earned: three, two or one; 0 unless it was won. */
int fl_star_rating(const fl_state_t *state);
/** Where a button of the end or pause panel sits on screen. */
void fl_panel_button_rect(int button, int *x, int *y, int *w, int *h);

/* --- introspection --------------------------------------------------- */
uint32_t fl_state_hash(const fl_state_t *state);
int fl_state_json(const fl_state_t *state, char *output, size_t capacity);
/** The state as JSON, plus the name of the sound the Host preview should
 * play. The name comes from the runtime; the model only carries it. */
int fl_state_json_sfx(const fl_state_t *state, const char *sfx,
                      char *output, size_t capacity);
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

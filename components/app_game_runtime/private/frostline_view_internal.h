/* What the parts of the view share: the loaded sheets, the drawing helpers,
 * and the entry points of the effect, scenery and screen modules.
 *
 *   frostline_view.c      the battlefield, the HUD and the campaign map
 *   frostline_fx.c        hits, shots, blasts, deaths and screen shake
 *   frostline_scenery.c   the living details of each map
 *   frostline_screens.c   title, settings, ending, banners and cards
 *
 * Only the view's own files include this header.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "../include/frostline_view.h"
#include "../../app_game/data/frostline_level.h"
#include "raylib_lite_2d.h"
#include "raylib_lite_assets.h"
#include "raylib_lite_raylib.h"

/* The packer puts the source art centred in a cell that is four pixels
 * larger, so every sprite frame carries two pixels of padding per side. */
#define SPRITE_PAD 2

/* Sprite sheets carry the palette already; a neutral tint draws them as-is. */
#define TINT_NONE ((Color){255, 255, 255, 255})

/* Every size is the native cell of its sheet, so nothing is ever rescaled
 * and the dither patterns land on the panel pixel for pixel. */
#define TOWER_DRAW_SIZE 56.0f
#define ENEMY_DRAW_SIZE 40.0f
#define HERO_DRAW_SIZE 48.0f
#define HUSK_DRAW_SIZE 48.0f
#define TITAN_DRAW_SIZE 80.0f
#define PAD_DRAW_SIZE 40.0f
#define PROJECTILE_DRAW_SIZE 16.0f
#define ICON_DRAW_SIZE 28
#define CASTLE_FX_SIZE 28.0f
/* A standing sprite has its feet this far above the bottom of its cell. */
#define SPRITE_FOOT 6.0f

#define ENEMY_FRAMES 6
#define CASTLE_FLAG_FRAMES 6
#define CASTLE_FLAME_FRAMES 4
#define SOLDIER_DRAW_SIZE 40.0f
#define SOLDIER_WALK_FRAMES 4
#define EMBER_FLAME_FRAMES 4
#define SPARK_FRAMES 6

/* Tower sprites are "tower_<key>_<variant>": level 1, level 2, then the two
 * level 3 branches. */
#define TOWER_VARIANTS 4

/* The crow's body sits this far down its cell (CROW_BODY_Y in sprites.py),
 * so the view can hang it exactly where the model aims at it. */
#define CROW_BODY_Y 22

/* Which sheet each enemy lives on, and how many ticks each walk frame
 * holds: the heavier the creature, the slower its stride. */
enum { SHEET_PROPS, SHEET_HUSK, SHEET_TITAN };

typedef struct {
    uint8_t sheet;
    uint8_t step;
    const char *alt;   /* suffix of its second look, or NULL */
} enemy_look_t;

/* The engine's 5x7 face doubled: what a finger presses in English. */
#define BUTTON_TEXT 16
#define BUTTON_TEXT_HEIGHT 14

typedef struct {
    raylib_lite_atlas_t terrain;
    raylib_lite_atlas_t units;
    raylib_lite_atlas_t heroes;
    raylib_lite_atlas_t props;
    raylib_lite_atlas_t props_husk;
    raylib_lite_atlas_t props_titan;
    raylib_lite_atlas_t fx;
    raylib_lite_atlas_t icons;
    raylib_lite_atlas_t castle_fx;
    raylib_lite_atlas_t title;
    raylib_lite_asset_id_t maps[FL_LEVEL_COUNT];   /* "map_<key>" per level */
    raylib_lite_asset_id_t campaign_map;
    raylib_lite_asset_id_t nodes[FL_MAX_LEVELS];   /* medallion per stage */
    raylib_lite_asset_id_t node_locked;
    raylib_lite_asset_id_t towers[FL_TOWER_KIND_COUNT][TOWER_VARIANTS];
    raylib_lite_asset_id_t enemies[FL_ENEMY_KIND_COUNT][ENEMY_FRAMES];
    raylib_lite_asset_id_t enemies_alt[FL_ENEMY_KIND_COUNT][ENEMY_FRAMES];
    raylib_lite_asset_id_t pads[2];
    raylib_lite_asset_id_t projectile;
    raylib_lite_asset_id_t ember_pot[EMBER_FLAME_FRAMES];
    raylib_lite_asset_id_t ember_flame[EMBER_FLAME_FRAMES];
    raylib_lite_asset_id_t spark[SPARK_FRAMES];
    raylib_lite_asset_id_t soldier_walk[SOLDIER_WALK_FRAMES];
    raylib_lite_asset_id_t soldier_strike;
    raylib_lite_asset_id_t icon_heart;
    raylib_lite_asset_id_t icon_coin;
    raylib_lite_asset_id_t icon_score;
    raylib_lite_asset_id_t icon_nova;
    raylib_lite_asset_id_t icon_star;
    raylib_lite_asset_id_t cards[FL_TOWER_KIND_COUNT];   /* by tower kind */
    raylib_lite_asset_id_t card_hero;
    raylib_lite_asset_id_t hero[4];
    raylib_lite_asset_id_t flag[CASTLE_FLAG_FRAMES];
    raylib_lite_asset_id_t flame[CASTLE_FLAME_FRAMES];
    /* The title sheet: the fortress panorama and the two logos. */
    raylib_lite_asset_id_t title_backdrop;
    raylib_lite_asset_id_t logo[2];                 /* by language */
    raylib_lite_asset_id_t ending_backdrop;
} view_assets_t;

extern view_assets_t fv_assets;

/* --- drawing helpers (frostline_view.c) ------------------------------- */
void draw_padded(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
                 float x, float y, float size);
void draw_full(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
               float x, float y, float size);
/* A sprite's padded frame drawn at its own size with its top-left at x, y;
 * returns false when the sheet lacks it. */
bool draw_native(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
                 float x, float y);
void draw_panel(int x, int y, int w, int h, bool inverted);
bool corner_cut(int x, int y);
uint32_t jag_hash(uint32_t a);
void draw_dotted_circle(int cx, int cy, int radius, int march);
void draw_shard(int x, int y, int r);
void draw_ground_shadow(int cx, int cy, int rx, int ry);
/* A plate button: paper with ink text, or ink with a paper frame when it is
 * not on offer; `beckon` breathes a second frame round it. */
void draw_button(int x, int y, int w, int h, const char *label,
                 bool enabled, bool beckon, uint32_t tick);
const fl_level_t *current_level(const fl_state_t *model);
/* Names in the current language. */
const char *level_name(uint8_t level);
const char *tower_name(int kind);
const char *upgrade_name(int kind, int stage, int option);
const enemy_look_t *enemy_look(int kind);
/* The sheet and cell size an enemy is drawn from. */
raylib_lite_atlas_t enemy_sheet(int kind, float *size);
/* A dithered disc or ring: ink on the cells of a 4x4 Bayer pattern below
 * `density` (0..1). */
void draw_dither_disc(int cx, int cy, int r, float density, Color color);
void draw_dither_ellipse(int cx, int cy, int rx, int ry, float density,
                         Color color);
/* A faceted five-point star; `filled` false draws only the outline. */
void draw_star(int cx, int cy, int r, bool filled);

/* --- effects (frostline_fx.c) ----------------------------------------- */
/* Compare the model with the last frame and start the effects it implies. */
void fx_update(const fl_state_t *model);
/* Scorch marks and splats, under the actors. */
void fx_draw_ground(const fl_state_t *model);
/* The shots in flight, drawn per tower kind. */
void fx_draw_projectiles(const fl_state_t *model);
/* Sparks, debris, smoke and rings, over the actors. */
void fx_draw_air(const fl_state_t *model);
/* Burning, stunned and hexed enemies show it. */
void fx_draw_status(const fl_state_t *model);
/* The camera offset of the current screen shake. */
void fx_shake_offset(int *dx, int *dy);
/* Start or strengthen a screen shake. */
void fx_shake(int strength, int ticks);
/* How far a freshly struck enemy is knocked back this frame. */
void fx_enemy_jolt(int index, int *dx, int *dy);
/* Forget everything: a new level or a screen change. */
void fx_reset(void);

/* --- scenery (frostline_scenery.c) ------------------------------------ */
/* The map's own life under the actors: water, mist, critters... */
void scenery_draw_under(const fl_state_t *model);
/* ...and over them: falling snow, drifting cloud shadows, auroras. */
void scenery_draw_over(const fl_state_t *model);
/* A flock of birds crossing the sky now and then. */
void scenery_draw_birds(uint32_t tick);
/* Curtains of light across a band of sky, for any screen with a night sky. */
void scenery_draw_aurora(int top, int height, uint32_t tick, float strength);

/* --- screens (frostline_screens.c) ------------------------------------ */
void screens_draw_title(const fl_state_t *model);
void screens_draw_settings(const fl_state_t *model);
void screens_draw_ending(const fl_state_t *model);
/* Banners over a battle: the wave call, a boss's entrance, the card that
 * introduces a new enemy. Non-blocking: the battle runs underneath. */
void screens_update_battle(const fl_state_t *model);
void screens_draw_battle(const fl_state_t *model);

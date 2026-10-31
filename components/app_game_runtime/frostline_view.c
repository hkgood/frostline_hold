/* Rendering for Frostline Hold.
 *
 * The view owns every drawing decision; the model owns every rule. All colour
 * is ink or paper so the panel keeps the 1-bit look, and all tone comes from
 * dithered textures baked at build time.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "include/frostline_view.h"
#include "../app_game/data/frostline_level.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Where the moving parts hang on the castle plate baked into the map. These
 * mirror CASTLE_* in assets_src/artgen/ai_terrain.py and must agree. */
#define CASTLE_ORIGIN_X (FL_GOAL.x - 92)
#define CASTLE_ORIGIN_Y (FL_GOAL.y + 16 - 100)
#define CASTLE_POLE_X (CASTLE_ORIGIN_X + 93)
#define CASTLE_POLE_TOP (CASTLE_ORIGIN_Y + 12)
static const int CASTLE_BRAZIERS[2][2] = {{66, 89}, {112, 91}};

/* Tower sprites are "tower_<key>_<variant>": level 1, level 2, then the two
 * level 3 branches. The keys come from the generated level data. */
#define TOWER_VARIANTS 4
static const char *const TOWER_VARIANT_NAMES[TOWER_VARIANTS] = {
    "lv1", "lv2", "lv3a", "lv3b",
};

/* Which sheet each enemy lives on, and how many ticks each walk frame
 * holds: the heavier the creature, the slower its stride. */
enum { SHEET_PROPS, SHEET_HUSK, SHEET_TITAN };

typedef struct {
    uint8_t sheet;
    uint8_t step;
    const char *alt;   /* suffix of its second look, or NULL */
} enemy_look_t;

static const enemy_look_t ENEMY_LOOKS[FL_ENEMY_KIND_COUNT] = {
    [FL_ENEMY_WOLF] = {SHEET_PROPS, 4, NULL},
    [FL_ENEMY_RAIDER] = {SHEET_PROPS, 4, NULL},
    [FL_ENEMY_SKITTER] = {SHEET_PROPS, 4, NULL},
    [FL_ENEMY_HUSK] = {SHEET_HUSK, 6, NULL},
    [FL_ENEMY_SHAMAN] = {SHEET_PROPS, 4, NULL},
    [FL_ENEMY_BANNERMAN] = {SHEET_PROPS, 4, NULL},
    [FL_ENEMY_SAP] = {SHEET_PROPS, 4, NULL},
    [FL_ENEMY_TITAN] = {SHEET_TITAN, 8, NULL},
    [FL_ENEMY_CROW] = {SHEET_PROPS, 3, NULL},
    [FL_ENEMY_GOLEM] = {SHEET_HUSK, 7, "cracked"},   /* out of its shell */
    [FL_ENEMY_SLIME] = {SHEET_HUSK, 6, NULL},
    [FL_ENEMY_SLIMELET] = {SHEET_PROPS, 4, NULL},
    [FL_ENEMY_BURROWER] = {SHEET_PROPS, 4, "under"},  /* tunnelling */
    [FL_ENEMY_WYRM] = {SHEET_TITAN, 8, "under"},
    [FL_ENEMY_KING] = {SHEET_TITAN, 9, NULL},
};

/* The crow's body sits this far down its cell (CROW_BODY_Y in sprites.py),
 * so the view can hang it exactly where the model aims at it. */
#define CROW_BODY_Y 22

#define SOLDIER_DRAW_SIZE 40.0f
#define SOLDIER_WALK_FRAMES 4
#define EMBER_FLAME_FRAMES 4

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
    raylib_lite_asset_id_t spark[6];
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
} view_assets_t;

static view_assets_t s_assets;

void frostline_view_init(void)
{
    memset(&s_assets, 0, sizeof(s_assets));
    s_assets.terrain = raylib_lite_atlas_load("terrain.atlas");
    s_assets.units = raylib_lite_atlas_load("units.atlas");
    s_assets.heroes = raylib_lite_atlas_load("heroes.atlas");
    s_assets.props = raylib_lite_atlas_load("props.atlas");
    s_assets.props_husk = raylib_lite_atlas_load("props_husk.atlas");
    s_assets.props_titan = raylib_lite_atlas_load("props_titan.atlas");
    s_assets.fx = raylib_lite_atlas_load("fx.atlas");
    s_assets.icons = raylib_lite_atlas_load("icons.atlas");
    s_assets.castle_fx = raylib_lite_atlas_load("castle_fx.atlas");

    for (int i = 0; i < FL_LEVEL_COUNT; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "map_%s", FL_LEVELS[i].key);
        s_assets.maps[i] = raylib_lite_asset_id(name);
    }
    s_assets.campaign_map = raylib_lite_asset_id("map_campaign");
    for (int i = 0; i < FL_MAX_LEVELS; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "node_%d", i);
        s_assets.nodes[i] = raylib_lite_asset_id(name);
    }
    s_assets.node_locked = raylib_lite_asset_id("node_locked");
    for (int kind = FL_TOWER_GUARD; kind < FL_TOWER_KIND_COUNT; ++kind) {
        char name[32];
        for (int variant = 0; variant < TOWER_VARIANTS; ++variant) {
            snprintf(name, sizeof(name), "tower_%s_%s", FL_TOWER_KEYS[kind],
                     TOWER_VARIANT_NAMES[variant]);
            s_assets.towers[kind][variant] = raylib_lite_asset_id(name);
        }
        snprintf(name, sizeof(name), "card_%s", FL_TOWER_KEYS[kind]);
        s_assets.cards[kind] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < EMBER_FLAME_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "ember_pot_%d", i);
        s_assets.ember_pot[i] = raylib_lite_asset_id(name);
        snprintf(name, sizeof(name), "ember_flame_%d", i);
        s_assets.ember_flame[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < 6; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "spark_%d", i);
        s_assets.spark[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < SOLDIER_WALK_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "soldier_%d", i);
        s_assets.soldier_walk[i] = raylib_lite_asset_id(name);
    }
    s_assets.soldier_strike = raylib_lite_asset_id("soldier_strike");
    for (int kind = 0; kind < FL_ENEMY_KIND_COUNT; ++kind) {
        const char *key = FL_ENEMY_KEYS[kind];
        const char *alt = ENEMY_LOOKS[kind].alt;
        for (int frame = 0; frame < ENEMY_FRAMES; ++frame) {
            char name[40];
            snprintf(name, sizeof(name), "enemy_%s_%d", key, frame);
            s_assets.enemies[kind][frame] = raylib_lite_asset_id(name);
            if (alt != NULL) {
                snprintf(name, sizeof(name), "enemy_%s_%s_%d", key, alt, frame);
            }
            s_assets.enemies_alt[kind][frame] = raylib_lite_asset_id(name);
        }
    }
    for (int i = 0; i < 2; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "build_pad_%d", i);
        s_assets.pads[i] = raylib_lite_asset_id(name);
    }
    s_assets.projectile = raylib_lite_asset_id("projectile_0_0");
    s_assets.icon_heart = raylib_lite_asset_id("icon_heart");
    s_assets.icon_coin = raylib_lite_asset_id("icon_coin");
    s_assets.icon_score = raylib_lite_asset_id("icon_score");
    s_assets.icon_nova = raylib_lite_asset_id("icon_nova");
    s_assets.icon_star = raylib_lite_asset_id("icon_star");
    s_assets.card_hero = raylib_lite_asset_id("card_hero");
    for (int i = 0; i < 4; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "hero_warden_%d", i);
        s_assets.hero[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < CASTLE_FLAG_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "castle_flag_%d", i);
        s_assets.flag[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < CASTLE_FLAME_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "castle_flame_%d", i);
        s_assets.flame[i] = raylib_lite_asset_id(name);
    }
}

void frostline_view_shutdown(void)
{
    raylib_lite_atlas_unload(s_assets.terrain);
    raylib_lite_atlas_unload(s_assets.units);
    raylib_lite_atlas_unload(s_assets.heroes);
    raylib_lite_atlas_unload(s_assets.props);
    raylib_lite_atlas_unload(s_assets.props_husk);
    raylib_lite_atlas_unload(s_assets.props_titan);
    raylib_lite_atlas_unload(s_assets.fx);
    raylib_lite_atlas_unload(s_assets.icons);
    raylib_lite_atlas_unload(s_assets.castle_fx);
    memset(&s_assets, 0, sizeof(s_assets));
}

bool frostline_view_ready(void)
{
    return s_assets.terrain.texture.id != 0 && s_assets.units.texture.id != 0
           && s_assets.heroes.texture.id != 0
           && s_assets.props.texture.id != 0 && s_assets.fx.texture.id != 0
           && s_assets.props_husk.texture.id != 0
           && s_assets.props_titan.texture.id != 0
           && s_assets.icons.texture.id != 0
           && s_assets.castle_fx.texture.id != 0;
}

/* ------------------------------------------------------------------ */

static void draw_padded(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
                         float x, float y, float size)
{
    const raylib_lite_sprite_frame_t *frame = raylib_lite_atlas_get_frame(atlas, id);
    if (!frame) {
        return;
    }
    Rectangle source = frame->source;
    source.x += SPRITE_PAD;
    source.y += SPRITE_PAD;
    source.width -= SPRITE_PAD * 2;
    source.height -= SPRITE_PAD * 2;
    DrawTexturePro(atlas.texture, source, (Rectangle){x, y, size, size},
                   (Vector2){0.0f, 0.0f}, 0.0f, TINT_NONE);
}

static void draw_full(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
                      float x, float y, float size)
{
    const raylib_lite_sprite_frame_t *frame = raylib_lite_atlas_get_frame(atlas, id);
    if (!frame) {
        return;
    }
    DrawTexturePro(atlas.texture, frame->source, (Rectangle){x, y, size, size},
                   (Vector2){0.0f, 0.0f}, 0.0f, TINT_NONE);
}

/* The engine font is a 5x7 grid scaled by whole steps of size / 8, so every
 * size from 8 to 15 draws the same 7 px glyph. Anything a finger presses
 * uses the doubled glyph, 14 px tall. */
#define BUTTON_TEXT 16
#define BUTTON_TEXT_HEIGHT 14

static void draw_text_centered(const char *text, int center_x, int y, int size,
                               Color color)
{
    int width = MeasureText(text, size);
    DrawText(text, center_x - width / 2, y, size, color);
}

static void draw_panel(int x, int y, int w, int h, bool inverted)
{
    DrawRectangle(x, y, w, h, inverted ? FL_PAPER : FL_INK);
    DrawRectangleLines(x, y, w, h, inverted ? FL_INK : FL_PAPER);
}

/* ------------------------------------------------------------------ */

static const fl_level_t *current_level(const fl_state_t *model)
{
    return &FL_LEVELS[model->level < FL_LEVEL_COUNT ? model->level : 0];
}

static void draw_background(const fl_state_t *model)
{
    uint8_t level = model->level < FL_LEVEL_COUNT ? model->level : 0;
    draw_full(s_assets.terrain, s_assets.maps[level], 0.0f, 0.0f, 480.0f);
}

static bool corner_cut(int x, int y)
{
    if (x >= FL_CORNER_RADIUS && x < FL_VIEW_WIDTH - FL_CORNER_RADIUS) {
        return false;
    }
    if (y >= FL_CORNER_RADIUS && y < FL_VIEW_HEIGHT - FL_CORNER_RADIUS) {
        return false;
    }
    int cx = x < FL_CORNER_RADIUS ? FL_CORNER_RADIUS
                                  : FL_VIEW_WIDTH - FL_CORNER_RADIUS;
    int cy = y < FL_CORNER_RADIUS ? FL_CORNER_RADIUS
                                  : FL_VIEW_HEIGHT - FL_CORNER_RADIUS;
    int dx = x - cx;
    int dy = y - cy;
    return dx * dx + dy * dy > FL_CORNER_RADIUS * FL_CORNER_RADIUS;
}

/* Snow, a travelling glint on the road, and the gate's cloth and fire.
 * The stone of the castle is baked into the map; only the parts that move
 * are drawn here. */
static void draw_world_motion(const fl_state_t *model)
{
    uint32_t tick = model->tick;
    const fl_level_t *level = current_level(model);
    for (int i = 0; i < 22; ++i) {
        uint32_t speed = 1u + (uint32_t)(i % 3);
        int x = (int)((i * 67u + tick * speed) % 400u) + 40;
        int y = (int)((i * 41u + tick * (speed + 1u)) % 300u) + 56;
        if (corner_cut(x, y)) {
            continue;
        }
        /* One pixel and a dark neighbour, so a flake reads on snow and on ink. */
        if (((unsigned)i + tick / 6u) % 2u == 0u) {
            DrawRectangle(x, y, 1, 1, FL_PAPER);
            DrawRectangle(x + 1, y, 1, 1, FL_INK);
        } else {
            DrawRectangle(x, y, 1, 1, FL_INK);
            DrawRectangle(x, y + 1, 1, 1, FL_PAPER);
        }
    }

    int head = (int)((tick / 2u) % (uint32_t)level->lane_a_count);
    for (int i = 0; i < 4; ++i) {
        int index = (head + i * 6) % level->lane_a_count;
        int x = level->lane_a[index].x;
        int y = level->lane_a[index].y;
        DrawRectangle(x, y, 1, 1, FL_PAPER);
        DrawRectangle(x + 1, y, 1, 1, FL_INK);
    }
    if (level->lane_b_count > 0) {
        int head_b = (int)((tick / 3u) % (uint32_t)level->lane_b_count);
        for (int i = 0; i < 2; ++i) {
            int index = (head_b + i * 6) % level->lane_b_count;
            int x = level->lane_b[index].x;
            int y = level->lane_b[index].y;
            DrawRectangle(x, y, 1, 1, FL_PAPER);
            DrawRectangle(x + 1, y, 1, 1, FL_INK);
        }
    }

    /* The pole is part of the castle plate; the cloth hangs off its top and
     * flies left, with the pole at the cell's right edge. */
    int flag = (int)((tick / 5u) % CASTLE_FLAG_FRAMES);
    draw_padded(s_assets.castle_fx, s_assets.flag[flag],
                (float)(CASTLE_POLE_X - (int)CASTLE_FX_SIZE + 2),
                (float)(CASTLE_POLE_TOP - 2), CASTLE_FX_SIZE);

    /* The braziers burn out of step with each other. */
    for (int i = 0; i < 2; ++i) {
        int flame = (int)((tick / 3u + (unsigned)i * 2u) % CASTLE_FLAME_FRAMES);
        int bx = CASTLE_ORIGIN_X + CASTLE_BRAZIERS[i][0];
        int by = CASTLE_ORIGIN_Y + CASTLE_BRAZIERS[i][1];
        draw_padded(s_assets.castle_fx, s_assets.flame[flame],
                    (float)(bx - 14), (float)(by - 22), CASTLE_FX_SIZE);
    }
}

static bool spot_taken(const fl_state_t *model, const fl_point_t *spot)
{
    for (int t = 0; t < FL_MAX_TOWERS; ++t) {
        if (model->towers[t].active && model->towers[t].x == spot->x
                && model->towers[t].y == spot->y) {
            return true;
        }
    }
    return false;
}

/* Every empty pad is always on show, so the player can see where a tower
 * goes without arming anything first. The pad the ring opened from
 * breathes; the rest sit still. A free hero post shows only until the hero
 * stands on it. */
static void draw_build_pads(const fl_state_t *model)
{
    const fl_level_t *level = current_level(model);
    int breathe = (int)((model->tick / 12u) % 2u);
    for (int i = 0; i < level->spot_count; ++i) {
        const fl_point_t *spot = &level->spots[i];
        if (spot_taken(model, spot)) {
            continue;
        }
        int frame = model->ring_spot == i ? breathe : 0;
        draw_padded(s_assets.props, s_assets.pads[frame],
                    (float)(spot->x - PAD_DRAW_SIZE / 2),
                    (float)(spot->y - PAD_DRAW_SIZE / 2), PAD_DRAW_SIZE);
    }
    for (int i = 0; i < level->post_count && i < FL_MAX_HEROES; ++i) {
        if (model->heroes[i].active) {
            continue;
        }
        const fl_point_t *post = &level->posts[i];
        int frame = model->ring_post == i ? breathe : 0;
        draw_padded(s_assets.props, s_assets.pads[frame],
                    (float)(post->x - PAD_DRAW_SIZE / 2),
                    (float)(post->y - PAD_DRAW_SIZE / 2), PAD_DRAW_SIZE);
    }
}

/* ------------------------------------------------------------------ */
/* Build ring                                                          */
/* ------------------------------------------------------------------ */

/* A dashed circle that reads on snow and on forest: an ink dot every six
 * pixels of arc with a paper fleck on it. `march` turns it slowly so the
 * ring looks alive. */
static void draw_dotted_circle(int cx, int cy, int radius, int march)
{
    int dots = (int)(6.2831853f * (float)radius / 6.0f);
    if (dots < 12) {
        dots = 12;
    }
    float turn = (float)march * 0.01f;
    for (int i = 0; i < dots; ++i) {
        float angle = turn + (float)i * 6.2831853f / (float)dots;
        int x = cx + (int)lroundf(cosf(angle) * (float)radius);
        int y = cy + (int)lroundf(sinf(angle) * (float)radius);
        if (corner_cut(x, y) || y < FL_HUD_Y + FL_HUD_H) {
            continue;
        }
        DrawRectangle(x - 1, y - 1, 3, 3, FL_INK);
        DrawRectangle(x, y, 1, 1, FL_PAPER);
    }
}

/* The ring springs open from its pad. The model does not keep time for the
 * menu, so the view notes the tick the ring first appeared at. */
static int s_ring_key = -1;
static uint32_t s_ring_opened;

static float ring_opening(const fl_state_t *model)
{
    int key = model->ring_post >= 0 ? 100 + model->ring_post : model->ring_spot;
    if (key != s_ring_key) {
        s_ring_key = key;
        s_ring_opened = model->tick;
    }
    float t = (float)(model->tick - s_ring_opened + 1u) / 5.0f;
    if (t >= 1.0f) {
        return 1.0f;
    }
    /* Ease out with a touch of overshoot, like a spring settling. */
    float u = 1.0f - t;
    return 1.0f - u * u * u + 0.12f * sinf(t * 3.14159f);
}

static void draw_price_tag(int cx, int y, uint16_t cost, bool affordable)
{
    const char *text = TextFormat("%u", (unsigned)cost);
    int w = MeasureText(text, 10) + 8;
    int x = cx - w / 2;
    if (affordable) {
        DrawRectangle(x - 1, y - 1, w + 2, 14, FL_INK);
        DrawRectangle(x, y, w, 12, FL_PAPER);
        DrawText(text, x + 4, y + 1, 10, FL_INK);
    } else {
        /* Short of gold: an ink tag with the price struck through. */
        DrawRectangle(x - 1, y - 1, w + 2, 14, FL_PAPER);
        DrawRectangle(x, y, w, 12, FL_INK);
        DrawText(text, x + 4, y + 1, 10, FL_PAPER);
        DrawLine(x + 2, y + 9, x + w - 2, y + 3, FL_PAPER);
    }
}

/* One medallion: an ink disc with a paper rim and the tower's portrait.
 * The picked one gets a heavy rim and a pulse; an unaffordable one is
 * dimmed with ink scanlines over its portrait. */
static void draw_medallion(int x, int y, raylib_lite_asset_id_t portrait,
                           bool picked, bool affordable, uint32_t tick)
{
    const int r = FL_RING_ICON_R;
    DrawCircle(x, y, (float)(r + 2), FL_INK);
    DrawCircle(x, y, (float)r, FL_PAPER);
    DrawCircle(x, y, (float)(r - (picked ? 3 : 2)), FL_INK);
    draw_padded(s_assets.icons, portrait, (float)(x - ICON_DRAW_SIZE / 2),
                (float)(y - ICON_DRAW_SIZE / 2), (float)ICON_DRAW_SIZE);
    if (!affordable) {
        for (int dy = -ICON_DRAW_SIZE / 2; dy < ICON_DRAW_SIZE / 2; dy += 2) {
            int half = (int)sqrtf((float)((r - 3) * (r - 3) - dy * dy));
            DrawRectangle(x - half, y + dy, half * 2, 1, FL_INK);
        }
    }
    if (picked) {
        int pulse = (int)((tick / 2u) % 6u);
        DrawCircleLines(x, y, (float)(r + 4 + pulse), FL_PAPER);
        DrawCircleLines(x, y, (float)(r + 5 + pulse), FL_INK);
    }
}

static const char *ring_label(uint8_t entry)
{
    if (entry == FL_RING_HERO) {
        return "WARDEN";
    }
    return entry < FL_TOWER_KIND_COUNT ? FL_TOWER_LABELS[entry] : "";
}

static void draw_ring(const fl_state_t *model)
{
    int count = fl_ring_count(model);
    if (count <= 0) {
        s_ring_key = -1;
        return;
    }
    float open = ring_opening(model);
    int ax, ay, cx, cy;
    fl_ring_anchor(model, &ax, &ay);
    fl_ring_center(model, &cx, &cy);

    /* The picked tower's reach, drawn round the pad it would stand on. */
    int pick = model->ring_pick;
    if (pick >= 0) {
        uint8_t entry = fl_ring_entry(model, pick);
        int reach = entry == FL_RING_HERO
                        ? FL_HERO_STATS[FL_HERO_WARDEN].range
                        : FL_TOWER_LEVELS[entry][0].range;
        draw_dotted_circle(ax, ay, reach, (int)model->tick);
    }

    /* When the ring had to slide off its pad to stay on the glass, a cord
     * ties it back so it is clear which pad it builds on. */
    if (cx != ax || cy != ay) {
        DrawLineEx((Vector2){(float)ax, (float)ay}, (Vector2){(float)cx, (float)cy},
                   3.0f, FL_INK);
        DrawLine(ax, ay, cx, cy, FL_PAPER);
    }

    /* The band the medallions sit on. */
    int radius = (int)((float)fl_ring_radius(count) * open);
    if (count > 1) {
        DrawCircleLines(cx, cy, (float)(radius - 1), FL_INK);
        DrawCircleLines(cx, cy, (float)radius, FL_PAPER);
        DrawCircleLines(cx, cy, (float)(radius + 1), FL_INK);
    }

    for (int i = 0; i < count; ++i) {
        int sx, sy;
        fl_ring_slot(model, i, &sx, &sy);
        sx = cx + (int)lroundf((float)(sx - cx) * open);
        sy = cy + (int)lroundf((float)(sy - cy) * open);
        uint8_t entry = fl_ring_entry(model, i);
        uint16_t cost = fl_ring_cost(model, i);
        bool affordable = model->gold >= cost;
        raylib_lite_asset_id_t portrait =
            entry == FL_RING_HERO ? s_assets.card_hero : s_assets.cards[entry];
        draw_medallion(sx, sy, portrait, pick == i, affordable, model->tick);
        draw_price_tag(sx, sy + FL_RING_ICON_R - 4, cost, affordable);
    }

    /* The picked tower's name on a plate under the ring, or over it when
     * the ring sits low on the glass; the second tap builds it. */
    if (pick >= 0 && open >= 1.0f) {
        const char *name = ring_label(fl_ring_entry(model, pick));
        const char *hint = "TAP AGAIN";
        int w = MeasureText(name, BUTTON_TEXT) + MeasureText(hint, 10) + 26;
        int h = 22;
        int y = cy + radius + FL_RING_ICON_R + 14;
        if (y + h > FL_VIEW_HEIGHT - 14) {
            y = cy - radius - FL_RING_ICON_R - 12 - h;
        }
        int x = cx - w / 2;
        if (x < FL_CORNER_RADIUS / 2) {
            x = FL_CORNER_RADIUS / 2;
        } else if (x + w > FL_VIEW_WIDTH - FL_CORNER_RADIUS / 2) {
            x = FL_VIEW_WIDTH - FL_CORNER_RADIUS / 2 - w;
        }
        DrawRectangle(x - 1, y - 1, w + 2, h + 2, FL_PAPER);
        DrawRectangle(x, y, w, h, FL_INK);
        DrawText(name, x + 8, y + (h - BUTTON_TEXT_HEIGHT) / 2, BUTTON_TEXT,
                 FL_PAPER);
        DrawText(hint, x + w - 8 - MeasureText(hint, 10), y + 7, 10, FL_PAPER);
    }
}

static int tower_variant(const fl_tower_t *tower)
{
    int variant = (int)tower->level - 1;
    if (tower->level >= 3) {
        int option = fl_tower_option(tower, 1);
        variant = 2 + (option < 0 ? 0 : option);
    }
    return (variant < 0 || variant > 3) ? 0 : variant;
}

/* Selection: four corner brackets round the sprite, ink with a paper core so
 * they read on snow and on forest alike. */
static void draw_brackets(int x, int y, int w, int h)
{
    const int arm = 6;
    for (int corner = 0; corner < 4; ++corner) {
        bool right = corner & 1;
        bool bottom = corner & 2;
        int cx = right ? x + w - 1 : x;
        int cy = bottom ? y + h - 1 : y;
        int hx = right ? cx - arm + 1 : cx;
        int vy = bottom ? cy - arm + 1 : cy;
        DrawRectangle(hx - 1, cy - 1, arm + 2, 3, FL_INK);
        DrawRectangle(cx - 1, vy - 1, 3, arm + 2, FL_INK);
        DrawRectangle(hx, cy, arm, 1, FL_PAPER);
        DrawRectangle(cx, vy, 1, arm, FL_PAPER);
    }
}

static uint32_t jag_hash(uint32_t a);

/* One prism of ice: a paper wedge in an ink case, with a facet line down
 * its lit side. */
static void draw_ice_spike(int x, int base, int h, int w)
{
    if (h < 2) {
        return;
    }
    DrawTriangle((Vector2){(float)x, (float)(base - h - 1)},
                 (Vector2){(float)(x - w - 1), (float)(base + 1)},
                 (Vector2){(float)(x + w + 1), (float)(base + 1)}, FL_INK);
    DrawTriangle((Vector2){(float)x, (float)(base - h)},
                 (Vector2){(float)(x - w), (float)base},
                 (Vector2){(float)(x + w), (float)base}, FL_PAPER);
    DrawLine(x, base - h + 2, x + w / 2, base, FL_INK);
}

/* A tower the frost king has frozen: a crust of prisms round its plinth
 * and rime glittering up its body. The prisms sink back as it thaws. */
static void draw_frozen(const fl_tower_t *tower, uint32_t tick)
{
    static const int8_t SPIKES[][3] = {   /* dx, height, half width */
        {-15, 10, 3}, {-9, 18, 4}, {-2, 12, 3}, {5, 21, 4}, {12, 13, 3},
        {17, 8, 2},
    };
    float thaw = (float)tower->frozen_ticks / 24.0f;
    if (thaw > 1.0f) {
        thaw = 1.0f;
    }
    int base = tower->y + 4;
    /* Rime: paper flecks scattered over the body, a few winking. */
    for (int k = 0; k < 14; ++k) {
        uint32_t h = jag_hash((uint32_t)tower->x * 131u + (uint32_t)k * 977u);
        int fx = tower->x - 12 + (int)(h % 25u);
        int fy = tower->y - 6 - (int)((h >> 8) % 34u);
        if (((h >> 16) + tick / 4u) % 7u == 0u) {
            DrawRectangle(fx - 1, fy, 3, 1, FL_PAPER);
            DrawRectangle(fx, fy - 1, 1, 3, FL_PAPER);
        } else {
            DrawRectangle(fx, fy, 1, 1, FL_PAPER);
        }
    }
    for (size_t k = 0; k < sizeof(SPIKES) / sizeof(SPIKES[0]); ++k) {
        draw_ice_spike(tower->x + SPIKES[k][0], base,
                       (int)((float)SPIKES[k][1] * thaw), SPIKES[k][2]);
    }
}

static void draw_tower(const fl_state_t *model, int i)
{
    const fl_tower_t *tower = &model->towers[i];
    int kind = (int)tower->kind;
    if (kind <= FL_TOWER_NONE || kind >= FL_TOWER_KIND_COUNT) {
        return;
    }
    float x = (float)tower->x - TOWER_DRAW_SIZE / 2.0f;
    float y = (float)tower->y - TOWER_DRAW_SIZE + SPRITE_FOOT;
    /* The heavy throwers kick down a pixel as they let fly. */
    bool kicks = kind == FL_TOWER_SIEGE || kind == FL_TOWER_EMBER;
    float recoil = (kicks && tower->shot_tick < 3) ? 1.0f : 0.0f;
    draw_padded(s_assets.units, s_assets.towers[kind][tower_variant(tower)], x,
                y + recoil, TOWER_DRAW_SIZE);

    /* The spire's tip and the obelisk's eye flare as they fire. */
    if ((kind == FL_TOWER_STORM || kind == FL_TOWER_RUNE)
            && tower->shot_tick < 6) {
        int mx = tower->x;
        int my = tower->y - (int)fl_muzzle_height((fl_tower_kind_t)kind);
        int flare = 6 - (int)tower->shot_tick;
        DrawCircle(mx, my, (float)(flare / 2 + 2), FL_INK);
        DrawCircle(mx, my, (float)(flare / 2 + 1), FL_PAPER);
        if (kind == FL_TOWER_STORM) {
            /* Four short rays, turned a little every flare. */
            for (int r = 0; r < 4; ++r) {
                float a = (float)r * 1.5707963f + (float)tower->shot_tick * 0.4f;
                int ex = mx + (int)lroundf(cosf(a) * (float)(flare + 3));
                int ey = my + (int)lroundf(sinf(a) * (float)(flare + 3));
                DrawLine(mx, my, ex, ey, FL_PAPER);
            }
        }
    }

    /* Level pips on the plinth: filled for every level reached. */
    for (int level = 0; level < 3; ++level) {
        int px = tower->x - 10 + level * 7;
        bool filled = level < tower->level;
        DrawRectangle(px, tower->y + 3, 6, 4, FL_INK);
        if (!filled) {
            DrawRectangle(px + 1, tower->y + 4, 4, 2, FL_PAPER);
        }
    }
    if (tower->frozen_ticks > 0) {
        draw_frozen(tower, model->tick);
    }
    if (model->selected_tower == (int8_t)i) {
        draw_brackets((int)x + 4, (int)y, (int)TOWER_DRAW_SIZE - 8,
                      (int)TOWER_DRAW_SIZE);
    }
}

static void draw_hero(const fl_state_t *model, int i)
{
    const fl_hero_t *hero = &model->heroes[i];
    int frame = (int)((model->tick / 8u) % 4u);
    float x = (float)hero->x - HERO_DRAW_SIZE / 2.0f;
    float y = (float)hero->y - HERO_DRAW_SIZE + SPRITE_FOOT;
    draw_padded(s_assets.heroes, s_assets.hero[frame], x, y, HERO_DRAW_SIZE);
    /* Health bar: an ink trough with a paper core that drains. */
    int bar_x = hero->x - 14;
    int bar_y = (int)y - 4;
    DrawRectangle(bar_x, bar_y, 28, 4, FL_INK);
    int filled = hero->max_hp ? (int)((int)hero->hp * 26 / (int)hero->max_hp) : 0;
    DrawRectangle(bar_x + 1, bar_y + 1, filled, 2, FL_PAPER);
}

/* The shadow a flyer casts on the road: a dithered oval, ink on every other
 * pixel, so the ground still shows through it. */
static void draw_ground_shadow(int cx, int cy, int rx, int ry)
{
    for (int dy = -ry; dy <= ry; ++dy) {
        float v = (float)dy / (float)ry;
        int half = (int)((float)rx * sqrtf(1.0f - v * v));
        for (int dx = -half; dx <= half; ++dx) {
            if (((cx + dx + cy + dy) & 1) == 0) {
                DrawRectangle(cx + dx, cy + dy, 1, 1, FL_INK);
            }
        }
    }
}

/* A ward: a shell of ice round the body that nothing gets through. It turns
 * slowly, and flickers in its last ticks so the opening can be seen
 * coming. */
static void draw_ward(int cx, int cy, int radius, uint16_t ticks, uint32_t tick)
{
    if (ticks < 12 && (ticks / 2u) % 2u == 0u) {
        return;
    }
    draw_dotted_circle(cx, cy, radius, (int)(tick * 6u));
    draw_dotted_circle(cx, cy, radius - 3, -(int)(tick * 4u));
}

static void draw_enemy(const fl_state_t *model, int i)
{
    const fl_enemy_t *enemy = &model->enemies[i];
    int kind = (int)enemy->kind;
    if (kind < 0 || kind >= FL_ENEMY_KIND_COUNT) {
        return;
    }
    const enemy_look_t *look = &ENEMY_LOOKS[kind];
    float size = ENEMY_DRAW_SIZE;
    raylib_lite_atlas_t sheet = s_assets.props;
    if (look->sheet == SHEET_HUSK) {
        size = HUSK_DRAW_SIZE;
        sheet = s_assets.props_husk;
    } else if (look->sheet == SHEET_TITAN) {
        size = TITAN_DRAW_SIZE;
        sheet = s_assets.props_titan;
    }
    /* Each enemy walks on its own beat, so a column does not march in step. */
    int frame = (int)((model->tick / look->step + (unsigned)i * 2u) % ENEMY_FRAMES);
    /* The second look: a golem out of its shell, a burrower under the road. */
    bool alt = (kind == FL_ENEMY_GOLEM && enemy->stage > 0) || enemy->burrowed;
    raylib_lite_asset_id_t id = alt ? s_assets.enemies_alt[kind][frame]
                                    : s_assets.enemies[kind][frame];
    /* Whole pixels only: a fractional position would smear the dither. */
    int ex = (int)enemy->x;
    int ey = (int)enemy->y;
    float x = (float)ex - size / 2.0f;
    float y = (float)ey - size + SPRITE_FOOT;
    if (fl_enemy_flies(enemy)) {
        /* Hang the body where the towers aim, its shadow on the road. */
        draw_ground_shadow(ex, ey, 7, 2);
        y = (float)(ey - FL_FLY_HEIGHT - CROW_BODY_Y);
    }
    draw_padded(sheet, id, x, y, size);

    int body_y = (int)(y + size * 0.55f);
    if (enemy->ward_ticks > 0) {
        draw_ward(ex, body_y, (int)(size * 0.42f), enemy->ward_ticks, model->tick);
    }
    /* Nothing to show for one under the road: it cannot be hit there. */
    if (enemy->burrowed) {
        return;
    }

    /* Health bar over the head, only once the enemy is hurt, so a fresh
     * column reads as creatures rather than a row of bars. A boss always
     * shows its bar, a wide one. */
    bool boss = look->sheet == SHEET_TITAN;
    float ratio = enemy->max_hp > 0.0f ? enemy->hp / enemy->max_hp : 0.0f;
    if (!boss && ratio >= 1.0f && enemy->slow_ticks == 0) {
        return;
    }
    int bar_w = boss ? 40 : (size > ENEMY_DRAW_SIZE ? 26 : 18);
    int bar_x = ex - bar_w / 2;
    int bar_y = fl_enemy_flies(enemy) ? (int)y + 8 : (int)y - 4;
    DrawRectangle(bar_x - 1, bar_y - 1, bar_w + 2, 4, FL_INK);
    DrawRectangle(bar_x, bar_y, (int)(ratio * (float)bar_w), 2, FL_PAPER);
    if (kind == FL_ENEMY_KING) {
        /* Notches at the thirds, where the king wards himself. */
        for (int third = 1; third <= 2; ++third) {
            DrawRectangle(bar_x + bar_w * third / 3, bar_y - 2, 1, 6, FL_INK);
        }
    }
    if (enemy->slow_ticks > 0) {
        /* Frost: a dotted rime along the bar. */
        for (int dx = 0; dx < bar_w; dx += 2) {
            DrawRectangle(bar_x + dx, bar_y + 3, 1, 1, FL_PAPER);
        }
    }
}

/* A barracks soldier: walking frames on the way out, a stance at its post,
 * the blow for a few ticks after it strikes. */
static void draw_soldier(const fl_state_t *model, int i)
{
    const fl_soldier_t *soldier = &model->soldiers[i];
    raylib_lite_asset_id_t id;
    if (soldier->swing < 5) {
        id = s_assets.soldier_strike;
    } else if (soldier->moving) {
        id = s_assets.soldier_walk[(model->tick / 4u + (unsigned)i)
                                   % SOLDIER_WALK_FRAMES];
    } else {
        /* At its post it shifts its weight now and then. */
        id = s_assets.soldier_walk[((model->tick / 24u + (unsigned)i) % 4u) == 0u
                                       ? 1 : 0];
    }
    int sx = (int)soldier->x;
    int sy = (int)soldier->y;
    float x = (float)sx - SOLDIER_DRAW_SIZE / 2.0f;
    float y = (float)sy - SOLDIER_DRAW_SIZE + SPRITE_FOOT;
    draw_padded(s_assets.props, id, x, y, SOLDIER_DRAW_SIZE);
    if (soldier->hp < soldier->max_hp && soldier->max_hp > 0) {
        int bar_x = sx - 7;
        int bar_y = (int)y + 8;
        DrawRectangle(bar_x - 1, bar_y - 1, 16, 4, FL_INK);
        DrawRectangle(bar_x, bar_y, (int)soldier->hp * 14 / (int)soldier->max_hp,
                      2, FL_PAPER);
    }
}

enum { ACTOR_TOWER, ACTOR_HERO, ACTOR_ENEMY, ACTOR_SOLDIER };

typedef struct {
    int16_t foot_y;
    uint8_t type;
    uint8_t index;
} actor_t;

/* Towers, heroes and enemies drawn back to front by where they stand, so a
 * raider walking behind a keep is hidden by it instead of pasted on top. */
static void draw_actors(const fl_state_t *model)
{
    actor_t actors[FL_MAX_TOWERS + FL_MAX_HEROES + FL_MAX_ENEMIES
                   + FL_MAX_SOLDIERS];
    int count = 0;
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        if (model->towers[i].active) {
            actors[count++] = (actor_t){model->towers[i].y, ACTOR_TOWER, (uint8_t)i};
        }
    }
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        if (model->heroes[i].active && model->heroes[i].alive) {
            actors[count++] = (actor_t){model->heroes[i].y, ACTOR_HERO, (uint8_t)i};
        }
    }
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        if (model->enemies[i].alive) {
            actors[count++] = (actor_t){(int16_t)model->enemies[i].y, ACTOR_ENEMY,
                                        (uint8_t)i};
        }
    }
    for (int i = 0; i < FL_MAX_SOLDIERS; ++i) {
        if (model->soldiers[i].active && model->soldiers[i].alive) {
            actors[count++] = (actor_t){(int16_t)model->soldiers[i].y,
                                        ACTOR_SOLDIER, (uint8_t)i};
        }
    }
    /* Insertion sort: the list is short and nearly sorted frame to frame. */
    for (int i = 1; i < count; ++i) {
        actor_t key = actors[i];
        int j = i - 1;
        while (j >= 0 && actors[j].foot_y > key.foot_y) {
            actors[j + 1] = actors[j];
            --j;
        }
        actors[j + 1] = key;
    }
    for (int i = 0; i < count; ++i) {
        switch (actors[i].type) {
        case ACTOR_TOWER:
            draw_tower(model, actors[i].index);
            break;
        case ACTOR_HERO:
            draw_hero(model, actors[i].index);
            break;
        case ACTOR_SOLDIER:
            draw_soldier(model, actors[i].index);
            break;
        default:
            draw_enemy(model, actors[i].index);
            break;
        }
    }
}

static void draw_projectiles(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_PROJECTILES; ++i) {
        const fl_projectile_t *shot = &model->projectiles[i];
        if (!shot->active) {
            continue;
        }
        int px = (int)shot->x;
        int py = (int)shot->y;
        if (shot->kind == FL_TOWER_EMBER) {
            /* A tumbling fire pot that sheds sparks behind it. */
            float dx = shot->tx - shot->x;
            float dy = shot->ty - shot->y;
            float length = sqrtf(dx * dx + dy * dy);
            if (length > 0.5f) {
                dx /= length;
                dy /= length;
            }
            for (int k = 1; k <= 3; ++k) {
                int tx = px - (int)lroundf(dx * (float)(k * 5));
                int ty = py - (int)lroundf(dy * (float)(k * 5))
                         - (int)((model->tick + (unsigned)k) % 2u);
                Color c = ((unsigned)k + model->tick / 2u) % 2u ? FL_PAPER : FL_INK;
                DrawRectangle(tx, ty, 4 - k, 4 - k, c);
            }
            int frame = (int)((model->tick / 3u + (unsigned)i) % EMBER_FLAME_FRAMES);
            draw_padded(s_assets.fx, s_assets.ember_pot[frame],
                        (float)px - PROJECTILE_DRAW_SIZE / 2.0f,
                        (float)py - PROJECTILE_DRAW_SIZE / 2.0f,
                        PROJECTILE_DRAW_SIZE);
            continue;
        }
        draw_padded(s_assets.fx, s_assets.projectile,
                    (float)px - PROJECTILE_DRAW_SIZE / 2.0f,
                    (float)py - PROJECTILE_DRAW_SIZE / 2.0f,
                    PROJECTILE_DRAW_SIZE);
    }
}

/* A cheap integer hash, so every bolt jags its own way frame to frame
 * without any state in the view. */
static uint32_t jag_hash(uint32_t a)
{
    a ^= a >> 16;
    a *= 0x7feb352du;
    a ^= a >> 15;
    a *= 0x846ca68bu;
    a ^= a >> 16;
    return a;
}

/* One leap of a bolt: five kinks pushed sideways off the straight line,
 * an ink casing round a paper core, and now and then a short fork. */
static void draw_bolt(int x0, int y0, int x1, int y1, uint32_t seed, int ttl)
{
    enum { PIECES = 5 };
    float dx = (float)(x1 - x0);
    float dy = (float)(y1 - y0);
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 1.0f) {
        return;
    }
    float nx = -dy / length;
    float ny = dx / length;
    float spread = length > 40.0f ? 6.0f : 3.5f;
    Vector2 pts[PIECES + 1];
    for (int k = 0; k <= PIECES; ++k) {
        float t = (float)k / (float)PIECES;
        float off = 0.0f;
        if (k > 0 && k < PIECES) {
            off = ((float)(jag_hash(seed + (uint32_t)k * 131u) % 21u) - 10.0f)
                  / 10.0f * spread;
        }
        pts[k] = (Vector2){(float)x0 + dx * t + nx * off,
                           (float)y0 + dy * t + ny * off};
    }
    /* Bright and heavy when it lands, thinning to a filament as it fades. */
    float casing = ttl >= 6 ? 4.0f : (ttl >= 3 ? 3.0f : 0.0f);
    float core = ttl >= 6 ? 2.0f : 1.0f;
    for (int k = 0; k < PIECES && casing > 0.0f; ++k) {
        DrawLineEx(pts[k], pts[k + 1], casing, FL_INK);
    }
    for (int k = 0; k < PIECES; ++k) {
        if (ttl < 3 && k % 2) {
            continue;   /* breaking up */
        }
        DrawLineEx(pts[k], pts[k + 1], core, FL_PAPER);
    }
    if (ttl >= 5 && jag_hash(seed ^ 0x5bd1u) % 2u == 0u) {
        int k = 1 + (int)(jag_hash(seed + 7u) % 3u);
        float side = jag_hash(seed + 9u) % 2u ? 1.0f : -1.0f;
        Vector2 tip = {pts[k].x + (dx / length * 4.0f + nx * side * 7.0f),
                       pts[k].y + (dy / length * 4.0f + ny * side * 7.0f)};
        DrawLineEx(pts[k], tip, 3.0f, FL_INK);
        DrawLineV(pts[k], tip, FL_PAPER);
    }
}

/* A rune beam: a hollow lance that narrows as it fades, glyph ticks
 * running down it, and a sigil blooming where it lands. */
static void draw_beam(const fl_arc_t *arc, uint32_t tick)
{
    Vector2 a = {(float)arc->x[0], (float)arc->y[0]};
    Vector2 b = {(float)arc->x[1], (float)arc->y[1]};
    int ttl = arc->ttl;
    float width = (float)ttl * 0.75f + 1.0f;
    DrawLineEx(a, b, width + 2.0f, FL_INK);
    DrawLineEx(a, b, width, FL_PAPER);
    if (ttl > 4) {
        DrawLineV(a, b, FL_INK);
    }
    float dx = b.x - a.x;
    float dy = b.y - a.y;
    float length = sqrtf(dx * dx + dy * dy);
    if (length > 1.0f && ttl > 3) {
        float nx = -dy / length;
        float ny = dx / length;
        /* Glyph ticks crawl from the obelisk to the target. */
        for (float s = (float)((tick * 3u) % 10u); s < length; s += 10.0f) {
            float t = s / length;
            float cx = a.x + dx * t;
            float cy = a.y + dy * t;
            DrawLine((int)(cx - nx * 3.0f), (int)(cy - ny * 3.0f),
                     (int)(cx + nx * 3.0f), (int)(cy + ny * 3.0f), FL_INK);
        }
    }
    int bloom = 4 + (FL_ARC_TICKS - ttl) * 2;
    DrawCircleLines((int)b.x, (int)b.y, (float)(bloom + 1), FL_INK);
    DrawCircleLines((int)b.x, (int)b.y, (float)bloom, FL_PAPER);
    for (int r = 0; r < 4; ++r) {
        float angle = (float)r * 1.5707963f + (float)ttl * 0.3f;
        int gx = (int)b.x + (int)lroundf(cosf(angle) * (float)bloom);
        int gy = (int)b.y + (int)lroundf(sinf(angle) * (float)bloom);
        DrawRectangle(gx - 2, gy - 2, 5, 5, FL_INK);
        DrawRectangle(gx - 1, gy - 1, 3, 3, FL_PAPER);
    }
}

/* A paper diamond in an ink case: one shard of the frost king's beam. */
static void draw_shard(int x, int y, int r)
{
    for (int dy = -r - 1; dy <= r + 1; ++dy) {
        int half = r + 1 - abs(dy);
        DrawRectangle(x - half, y + dy, half * 2 + 1, 1, FL_INK);
    }
    for (int dy = -r; dy <= r; ++dy) {
        int half = r - abs(dy);
        DrawRectangle(x - half, y + dy, half * 2 + 1, 1, FL_PAPER);
    }
}

/* The frost king's beam, from his staff (point 1) down to the tower it
 * freezes (point 0). For its first half it is a solid lance with shards
 * racing along it and a snowflake bursting on the tower; then the lance is
 * gone and the shards hang where they were, falling and shrinking. */
static void draw_freeze_beam(const fl_arc_t *arc, uint32_t tick)
{
    const int life = FL_ARC_TICKS * 2;
    int ttl = arc->ttl;
    int age = life - ttl;
    float sx = (float)arc->x[1], sy = (float)arc->y[1];
    float tx = (float)arc->x[0], ty = (float)arc->y[0];
    float dx = tx - sx, dy = ty - sy;
    float length = sqrtf(dx * dx + dy * dy);
    if (length < 1.0f) {
        return;
    }
    bool lance = ttl > FL_ARC_TICKS;
    if (lance) {
        Vector2 a = {sx, sy};
        Vector2 b = {tx, ty};
        DrawLineEx(a, b, 5.0f, FL_INK);
        DrawLineEx(a, b, 3.0f, FL_PAPER);
        DrawLineV(a, b, FL_INK);
    }
    /* Shards every ten pixels, marching towards the tower. */
    int fall = lance ? 0 : (FL_ARC_TICKS - ttl);
    for (float s = (float)((age * 3) % 10); s < length; s += 10.0f) {
        float t = s / length;
        int x = (int)(sx + dx * t);
        int y = (int)(sy + dy * t) + fall * fall / 3;
        int r = lance ? 2 : (ttl > 4 ? 2 : 1);
        if (!lance && jag_hash((uint32_t)s + (uint32_t)arc->seed) % 3u == 0u) {
            continue;   /* some have melted already */
        }
        draw_shard(x, y, r);
    }
    /* The staff's crystal flares as it lets fly. */
    int glow = ttl / 3 + 2;
    DrawCircle((int)sx, (int)sy, (float)(glow + 1), FL_INK);
    DrawCircle((int)sx, (int)sy, (float)glow, FL_PAPER);
    if (lance) {
        /* A six-armed snowflake bursting on the tower. */
        int arm = 4 + age * 2;
        for (int k = 0; k < 6; ++k) {
            float angle = (float)k * 1.0471976f + (float)tick * 0.05f;
            Vector2 tip = {tx + cosf(angle) * (float)arm, ty + sinf(angle) * (float)arm};
            DrawLineEx((Vector2){tx, ty}, tip, 3.0f, FL_INK);
            DrawLineV((Vector2){tx, ty}, tip, FL_PAPER);
            draw_shard((int)tip.x, (int)tip.y, 1);
        }
    }
}

static void draw_arcs(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_ARCS; ++i) {
        const fl_arc_t *arc = &model->arcs[i];
        if (arc->ttl == 0 || arc->count < 2) {
            continue;
        }
        if (arc->kind == FL_TOWER_RUNE) {
            draw_beam(arc, model->tick);
            continue;
        }
        if (arc->kind == FL_ARC_FREEZE) {
            draw_freeze_beam(arc, model->tick);
            continue;
        }
        /* The bolt re-jags every other tick, so it crackles as it fades. */
        uint32_t seed = (uint32_t)arc->seed * 977u + (uint32_t)(arc->ttl / 2u);
        for (int k = 0; k + 1 < arc->count; ++k) {
            draw_bolt(arc->x[k], arc->y[k], arc->x[k + 1], arc->y[k + 1],
                      seed + (uint32_t)k * 31u, arc->ttl);
        }
        /* A burst on every enemy it struck. */
        int frame = FL_ARC_TICKS - arc->ttl;
        if (frame >= 0 && frame < 6) {
            for (int k = 1; k < arc->count; ++k) {
                draw_padded(s_assets.fx, s_assets.spark[frame],
                            (float)arc->x[k] - PROJECTILE_DRAW_SIZE / 2.0f,
                            (float)arc->y[k] - PROJECTILE_DRAW_SIZE / 2.0f,
                            PROJECTILE_DRAW_SIZE);
            }
        }
    }
}

/* A patch of burning road: scorched hatching that thins as it dies down,
 * tongues of flame dancing on it, sparks lifting off. Tar bubbles too. */
static void draw_zones(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_ZONES; ++i) {
        const fl_zone_t *zone = &model->zones[i];
        if (!zone->active || zone->life == 0) {
            continue;
        }
        float fade = (float)zone->ticks / (float)zone->life;
        int rx = zone->radius;
        int ry = (int)((float)zone->radius * 0.42f);
        if (ry < 4) {
            ry = 4;
        }
        int step = zone->slow ? 2 : (fade > 0.35f ? 2 : 3);
        for (int dy = -ry + (i % 2); dy <= ry; dy += step) {
            float v = (float)dy / (float)ry;
            int half = (int)((float)rx * sqrtf(1.0f - v * v));
            if (half > 0) {
                DrawLine(zone->x - half, zone->y + dy, zone->x + half,
                         zone->y + dy, FL_INK);
            }
        }
        DrawEllipseLines(zone->x, zone->y, (float)rx, (float)ry, FL_INK);

        if (zone->slow) {
            /* Bubbles swell and pop in the tar. */
            for (int k = 0; k < 3; ++k) {
                uint32_t h = jag_hash((uint32_t)i * 17u + (uint32_t)k);
                int bx = zone->x + (int)(h % (uint32_t)(rx + 1)) - rx / 2;
                int by = zone->y + (int)((h >> 8) % (uint32_t)(ry + 1)) - ry / 2;
                int grow = (int)((model->tick / 4u + h) % 4u);
                DrawCircleLines(bx, by, (float)(1 + grow), FL_PAPER);
            }
        }

        int flames = fade > 0.6f ? 3 : (fade > 0.25f ? 2 : 1);
        for (int k = 0; k < flames; ++k) {
            float angle = (float)k * 2.0943951f + (float)(i % 3) * 0.7f;
            int fx = zone->x + (int)lroundf(cosf(angle) * (float)rx * 0.5f);
            int fy = zone->y + (int)lroundf(sinf(angle) * (float)ry * 0.5f);
            int frame = (int)((model->tick / 3u + (unsigned)(k * 3 + i))
                              % EMBER_FLAME_FRAMES);
            draw_padded(s_assets.fx, s_assets.ember_flame[frame],
                        (float)fx - PROJECTILE_DRAW_SIZE / 2.0f,
                        (float)fy - PROJECTILE_DRAW_SIZE + 3.0f,
                        PROJECTILE_DRAW_SIZE);
        }
        for (int k = 0; k < 4; ++k) {
            uint32_t h = jag_hash((uint32_t)i * 53u + (uint32_t)k * 7u);
            int sx = zone->x + (int)(h % (uint32_t)(rx * 2 + 1)) - rx;
            int rise = (int)((model->tick + h) % 18u);
            int sy = zone->y - rise;
            DrawRectangle(sx, sy, 1, 1, rise % 4 < 2 ? FL_PAPER : FL_INK);
        }
    }
}

/* The picked tower's reach on the ground under it. A barracks shows where
 * its soldiers hold the road instead: a small pennant on the rally point. */
static void draw_selection_reach(const fl_state_t *model)
{
    if (model->selected_tower < 0) {
        return;
    }
    const fl_tower_t *tower = &model->towers[model->selected_tower];
    if (!tower->active) {
        return;
    }
    fl_tower_stats_t stat = fl_tower_stats(tower);
    draw_dotted_circle(tower->x, tower->y, stat.range, (int)model->tick);
    if (tower->kind == FL_TOWER_BARRACKS) {
        int x = tower->rally_x;
        int y = tower->rally_y;
        DrawLine(tower->x, tower->y, x, y, FL_INK);
        DrawRectangle(x - 1, y - 14, 3, 15, FL_INK);
        DrawRectangle(x, y - 13, 1, 13, FL_PAPER);
        int wave = (int)((model->tick / 6u) % 2u);
        DrawTriangle((Vector2){(float)x + 1, (float)y - 14},
                     (Vector2){(float)x + 1, (float)y - 7},
                     (Vector2){(float)x + 9, (float)(y - 11 + wave)}, FL_INK);
        DrawTriangle((Vector2){(float)x + 2, (float)y - 12},
                     (Vector2){(float)x + 2, (float)y - 9},
                     (Vector2){(float)x + 6, (float)(y - 11 + wave)}, FL_PAPER);
    }
}

static void draw_hud(const fl_state_t *model)
{
    /* The bar stops short of the bezel, so the glass curve stays map. */
    DrawRectangle(FL_HUD_X, FL_HUD_Y, FL_HUD_W, FL_HUD_H, FL_INK);
    DrawRectangle(FL_HUD_X, FL_HUD_Y + FL_HUD_H - 2, FL_HUD_W, 2, FL_PAPER);

    /* The hero's frost nova: lit when it is ready, dark when it is not. */
    int ready = 0;
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        const fl_hero_t *hero = &model->heroes[i];
        if (hero->active && hero->alive && hero->skill_timer == 0) {
            ready = 1;
        }
    }
    bool hero_placed = false;
    for (int i = 0; i < FL_MAX_HEROES; ++i) {
        if (model->heroes[i].active) {
            hero_placed = true;
        }
    }
    draw_panel(FL_SKILL_BTN_X, FL_SKILL_BTN_Y, FL_SKILL_BTN_W, FL_SKILL_BTN_H,
               ready);
    if (ready) {
        draw_padded(s_assets.icons, s_assets.icon_nova,
                    (float)(FL_SKILL_BTN_X + 1), (float)FL_SKILL_BTN_Y,
                    (float)ICON_DRAW_SIZE);
    } else if (hero_placed) {
        DrawText("...", FL_SKILL_BTN_X + 10, FL_SKILL_BTN_Y + 10, 14, FL_PAPER);
    }

    /* The icons are shaded solids with paper highlights, so they read on
     * the ink bar without a plate. The cluster ends before the pause button. */
    const float icon_y = (float)(FL_HUD_Y + 2);
    draw_padded(s_assets.icons, s_assets.icon_heart, 78.0f, icon_y,
                (float)ICON_DRAW_SIZE);
    DrawText(TextFormat("%u", (unsigned)model->lives), 106, 14, 16, FL_PAPER);
    draw_padded(s_assets.icons, s_assets.icon_coin, 140.0f, icon_y,
                (float)ICON_DRAW_SIZE);
    DrawText(TextFormat("%u", (unsigned)model->gold), 168, 14, 16, FL_PAPER);

    /* Points read as a third resource so the run has a running tally. */
    draw_padded(s_assets.icons, s_assets.icon_score, 212.0f, icon_y,
                (float)ICON_DRAW_SIZE);
    DrawText(TextFormat("%lu", (unsigned long)model->score), 240, 14, 16, FL_PAPER);

    DrawText("WAVE", 304, 8, 12, FL_PAPER);
    DrawText(TextFormat("%02u/%02u", (unsigned)model->wave,
                        (unsigned)fl_wave_count(model)),
             304, 20, 14, FL_PAPER);

    /* Pause and speed buttons. */
    draw_panel(FL_PAUSE_BTN_X, FL_BTN_Y, FL_PAUSE_BTN_W, FL_BTN_H, false);
    if (model->paused) {
        draw_panel(FL_PAUSE_BTN_X + 14, FL_BTN_Y + 7, 6, 14, true);
        draw_panel(FL_PAUSE_BTN_X + 23, FL_BTN_Y + 7, 6, 14, true);
    } else {
        draw_panel(FL_PAUSE_BTN_X + 16, FL_BTN_Y + 7, 7, 14, true);
    }
    draw_panel(FL_SPEED_BTN_X, FL_BTN_Y, FL_SPEED_BTN_W, FL_BTN_H, false);
    draw_text_centered(TextFormat("%ux", (unsigned)model->speed),
                       FL_SPEED_BTN_X + FL_SPEED_BTN_W / 2, FL_BTN_Y + 7, 16, FL_PAPER);
}

/* The panel that opens when a tower is picked: its numbers, the two upgrades
 * on offer, and the sell button. */
static void draw_upgrade_panel(const fl_state_t *model)
{
    const fl_tower_t *tower = &model->towers[model->selected_tower];
    if (!tower->active) {
        return;
    }
    DrawRectangle(FL_PANEL_X, FL_PANEL_TOP, FL_PANEL_W, FL_PANEL_HEIGHT, FL_INK);
    DrawRectangle(FL_PANEL_X, FL_PANEL_TOP, FL_PANEL_W, 2, FL_PAPER);

    const char *label = FL_TOWER_LABELS[tower->kind];
    DrawText(label, FL_PANEL_X + 8, FL_PANEL_TOP + 5, 16, FL_PAPER);
    int label_end = FL_PANEL_X + 8 + MeasureText(label, 16);
    DrawText(TextFormat("LV%u", (unsigned)tower->level), label_end + 8,
             FL_PANEL_TOP + 6, 14, FL_PAPER);

    /* The numbers that matter for this kind of tower. */
    fl_tower_stats_t stat = fl_tower_stats(tower);
    const char *numbers;
    switch (tower->kind) {
    case FL_TOWER_STORM:
        numbers = TextFormat("DMG %d  RNG %d  JUMPS %d", stat.damage,
                             stat.range, stat.extra);
        break;
    case FL_TOWER_BARRACKS:
        numbers = TextFormat("MEN %d  HP %d  DMG %d", stat.extra, stat.hp,
                             stat.damage);
        break;
    case FL_TOWER_EMBER:
        numbers = TextFormat("DMG %d  RNG %d  BURN %ds", stat.damage,
                             stat.range, stat.extra / 30);
        break;
    default:
        numbers = TextFormat("DMG %d  RNG %d  CD %d", stat.damage, stat.range,
                             stat.cooldown);
        break;
    }
    DrawText(numbers, label_end + 46, FL_PANEL_TOP + 6, 12, FL_PAPER);
    DrawText(TextFormat("SELL %u", (unsigned)(tower->invested / 2)),
             FL_PANEL_X + FL_PANEL_W - 78, FL_PANEL_TOP + 6, 12, FL_PAPER);

    if (tower->level >= FL_TOWER_LEVEL_COUNT) {
        /* Show what the tower ended up as instead of an empty panel. */
        char line[64];
        const char *first = fl_upgrade_name(tower->kind, 0,
                                            fl_tower_option(tower, 0));
        const char *second = fl_upgrade_name(tower->kind, 1,
                                             fl_tower_option(tower, 1));
        snprintf(line, sizeof(line), "%s + %s", first, second);
        draw_text_centered(line, FL_VIEW_WIDTH / 2, FL_UPGRADE_CARD_Y + 18, 16,
                           FL_PAPER);
        return;
    }

    int stage = (int)tower->level - 1;
    uint16_t cost = fl_upgrade_cost(model, model->selected_tower);
    bool affordable = model->gold >= cost;
    for (int i = 0; i < FL_TOWER_OPTION_COUNT; ++i) {
        int card_x = FL_UPGRADE_CARD_X0
                     + i * (FL_UPGRADE_CARD_W + FL_UPGRADE_CARD_GAP);
        draw_panel(card_x, FL_UPGRADE_CARD_Y, FL_UPGRADE_CARD_W,
                   FL_UPGRADE_CARD_H, true);
        DrawText(fl_upgrade_name(tower->kind, stage, i), card_x + 8,
                 FL_UPGRADE_CARD_Y + 5, BUTTON_TEXT, FL_INK);
        DrawText(TextFormat("LV%u", (unsigned)tower->level + 1), card_x + 8,
                 FL_UPGRADE_CARD_Y + 27, 12, FL_INK);
        const char *price = TextFormat("%u", (unsigned)cost);
        int price_w = MeasureText(price, BUTTON_TEXT);
        int price_x = card_x + FL_UPGRADE_CARD_W - 8 - price_w;
        int price_y = FL_UPGRADE_CARD_Y + 21;
        DrawText(price, price_x, price_y, BUTTON_TEXT, FL_INK);
        if (!affordable) {
            /* Strike the price through so an unaffordable upgrade reads. */
            DrawLine(price_x - 3, price_y + BUTTON_TEXT_HEIGHT - 2,
                     price_x + price_w + 3, price_y + 2, FL_INK);
        }
    }
}

/* A plate button: paper with ink text, or ink with a paper frame when it is
 * not on offer. */
static void draw_button(int x, int y, int w, int h, const char *label,
                        bool enabled, bool beckon, uint32_t tick)
{
    int text_y = y + (h - BUTTON_TEXT_HEIGHT) / 2;
    if (!enabled) {
        DrawRectangleLines(x, y, w, h, FL_PAPER);
        draw_text_centered(label, x + w / 2, text_y, BUTTON_TEXT, FL_PAPER);
        return;
    }
    draw_panel(x, y, w, h, true);
    if (beckon && (tick / 15u) % 2u == 0u) {
        /* The button the player most likely wants breathes a second frame. */
        DrawRectangleLines(x - 3, y - 3, w + 6, h + 6, FL_PAPER);
    }
    draw_text_centered(label, x + w / 2, text_y, BUTTON_TEXT, FL_INK);
}

static void draw_panel_buttons(const char *const labels[FL_PANEL_BTN_COUNT],
                               const bool enabled[FL_PANEL_BTN_COUNT],
                               int beckon, uint32_t tick)
{
    for (int i = 0; i < FL_PANEL_BTN_COUNT; ++i) {
        int x, y, w, h;
        fl_panel_button_rect(i, &x, &y, &w, &h);
        draw_button(x, y, w, h, labels[i], enabled[i], i == beckon, tick);
    }
}

/* Three stars in a row: earned ones are the faceted star, the rest an empty
 * frame where the star would sit. */
static void draw_star_row(int x, int y, int stars)
{
    for (int i = 0; i < 3; ++i) {
        int sx = x + i * 34;
        if (i < stars) {
            draw_padded(s_assets.icons, s_assets.icon_star, (float)(sx - 1),
                        (float)y, (float)ICON_DRAW_SIZE);
        } else {
            DrawRectangleLines(sx + 4, y + 4, ICON_DRAW_SIZE - 8,
                               ICON_DRAW_SIZE - 8, FL_PAPER);
        }
    }
}

static void draw_overlay(const fl_state_t *model)
{
    if (model->phase != FL_PHASE_WON && model->phase != FL_PHASE_LOST) {
        return;
    }
    /* An inset panel, so the verdict sits inside the glass instead of
     * running into the four cut corners. */
    DrawRectangle(64, 88, 352, 304, FL_INK);
    DrawRectangleLines(64, 88, 352, 304, FL_PAPER);
    bool won = model->phase == FL_PHASE_WON;
    const char *title = won ? "HOLD SECURED" : "THE GATE FALLS";
    draw_text_centered(title, FL_VIEW_WIDTH / 2, 104, 28, FL_PAPER);
    draw_text_centered(current_level(model)->name, FL_VIEW_WIDTH / 2, 136, 16,
                       FL_PAPER);

    /* The points total, with the breakdown underneath it. */
    draw_text_centered(TextFormat("%lu", (unsigned long)model->score),
                       FL_VIEW_WIDTH / 2, 160, 32, FL_PAPER);
    draw_star_row(FL_VIEW_WIDTH / 2 - 48, 198, fl_star_rating(model));

    draw_text_centered(TextFormat("KILLS %u      WAVE BONUS %lu",
                                  (unsigned)model->kills,
                                  (unsigned long)model->score_waves),
                       FL_VIEW_WIDTH / 2, 236, 14, FL_PAPER);
    draw_text_centered(TextFormat("LIVES %u      BONUS %lu",
                                  (unsigned)model->lives,
                                  (unsigned long)model->score_bonus),
                       FL_VIEW_WIDTH / 2, 258, 14, FL_PAPER);
    /* The wave counter steps past the last wave once the run is won. */
    unsigned waves = (unsigned)fl_wave_count(model);
    unsigned shown_wave = (unsigned)model->wave;
    if (shown_wave > waves) {
        shown_wave = waves;
    }
    draw_text_centered(TextFormat("WAVE %02u/%02u      BEST %lu", shown_wave,
                                  waves,
                                  (unsigned long)model->level_best[model->level]),
                       FL_VIEW_WIDTH / 2, 286, 14, FL_PAPER);

    /* Map, retry, and - after a win, when there is one - the next stage. */
    bool has_next = won && model->level + 1 < fl_level_count();

    /* The reward for pushing on: the tower the next stage brings, its
     * portrait on a paper plate beside the news. */
    fl_tower_kind_t fresh = has_next ? fl_new_tower((uint8_t)(model->level + 1))
                                     : FL_TOWER_NONE;
    if (fresh != FL_TOWER_NONE) {
        const char *news = TextFormat("NEXT STAGE UNLOCKS  %s",
                                      FL_TOWER_LABELS[fresh]);
        int w = MeasureText(news, 12) + ICON_DRAW_SIZE + 14;
        int x = FL_VIEW_WIDTH / 2 - w / 2;
        int blink = (model->tick / 20u) % 2u == 0u;
        DrawRectangle(x - 4, 300, w + 8, 28, blink ? FL_PAPER : FL_INK);
        DrawRectangleLines(x - 4, 300, w + 8, 28, FL_PAPER);
        DrawCircle(x + ICON_DRAW_SIZE / 2, 314, ICON_DRAW_SIZE / 2.0f, FL_INK);
        draw_padded(s_assets.icons, s_assets.cards[fresh], (float)x, 300.0f,
                    (float)ICON_DRAW_SIZE);
        DrawText(news, x + ICON_DRAW_SIZE + 8, 308, 12,
                 blink ? FL_INK : FL_PAPER);
    }
    static const char *const labels[FL_PANEL_BTN_COUNT] = {"MAP", "RETRY", "NEXT"};
    const bool enabled[FL_PANEL_BTN_COUNT] = {true, true, has_next};
    draw_panel_buttons(labels, enabled,
                       has_next ? FL_PANEL_BTN_NEXT : FL_PANEL_BTN_RETRY,
                       model->tick);
}

static void draw_pause(const fl_state_t *model)
{
    if (!model->paused || model->phase == FL_PHASE_WON
            || model->phase == FL_PHASE_LOST) {
        return;
    }
    DrawRectangle(64, 200, 352, 192, FL_INK);
    DrawRectangleLines(64, 200, 352, 192, FL_PAPER);
    draw_text_centered("PAUSED", FL_VIEW_WIDTH / 2, 224, 28, FL_PAPER);
    draw_text_centered(current_level(model)->name, FL_VIEW_WIDTH / 2, 260, 16,
                       FL_PAPER);
    draw_text_centered(TextFormat("WAVE %02u/%02u", (unsigned)model->wave,
                                  (unsigned)fl_wave_count(model)),
                       FL_VIEW_WIDTH / 2, 286, 14, FL_PAPER);
    static const char *const labels[FL_PANEL_BTN_COUNT] = {"MAP", "RETRY", "RESUME"};
    const bool enabled[FL_PANEL_BTN_COUNT] = {true, true, true};
    draw_panel_buttons(labels, enabled, FL_PANEL_BTN_NEXT, model->tick);
}

/* ------------------------------------------------------------------ */
/* Campaign map                                                        */
/* ------------------------------------------------------------------ */

/* One trail dot: ink with a paper rim, so it reads on snow and on forest.
 * A walked stretch has bolder steps than the way ahead. */
static void draw_trail_dot(int x, int y, bool bold)
{
    int size = bold ? 3 : 2;
    DrawRectangle(x - 1, y - 1, size + 2, size + 2, FL_PAPER);
    DrawRectangle(x, y, size, size, FL_INK);
}

/* The trail from one stage to the next bows sideways like a footpath.
 * Walked stretches are dotted densely; the way ahead is sparse, and the
 * stretch to the newest open stage marches towards it. */
static void draw_trail(fl_point_t a, fl_point_t b, int spacing, int march,
                       bool bold)
{
    float mx = (float)(a.x + b.x) * 0.5f;
    float my = (float)(a.y + b.y) * 0.5f;
    float dx = (float)(b.x - a.x);
    float dy = (float)(b.y - a.y);
    /* The bend: a control point pushed off the straight line. */
    float cx = mx - dy * 0.18f;
    float cy = my + dx * 0.18f;
    float length = sqrtf(dx * dx + dy * dy);
    int steps = (int)(length / 1.0f);
    float walked = 0.0f;
    float px = (float)a.x;
    float py = (float)a.y;
    int last_bucket = (int)floorf((float)march / (float)spacing);
    for (int i = 1; i <= steps; ++i) {
        float t = (float)i / (float)steps;
        float u = 1.0f - t;
        float x = u * u * (float)a.x + 2.0f * u * t * cx + t * t * (float)b.x;
        float y = u * u * (float)a.y + 2.0f * u * t * cy + t * t * (float)b.y;
        walked += sqrtf((x - px) * (x - px) + (y - py) * (y - py));
        px = x;
        py = y;
        /* Leave the medallions themselves clear. */
        float da = (x - a.x) * (x - a.x) + (y - a.y) * (y - a.y);
        float db = (x - b.x) * (x - b.x) + (y - b.y) * (y - b.y);
        /* A dot every time the walked distance crosses the next step. */
        int bucket = (int)floorf((walked + (float)march) / (float)spacing);
        if (bucket == last_bucket) {
            continue;
        }
        last_bucket = bucket;
        if (da >= 20.0f * 20.0f && db >= 20.0f * 20.0f) {
            draw_trail_dot((int)x, (int)y, bold);
        }
    }
}

static void draw_node(const fl_state_t *model, uint8_t i)
{
    fl_point_t node = FL_LEVELS[i].node;
    bool open = fl_level_unlocked(model, i);
    bool picked = model->menu_level == i;
    /* The picked token bobs gently and has a ring pulsing out under it. */
    int bob = 0;
    if (picked) {
        bob = ((model->tick / 8u) % 4u == 0u) ? -1 : 0;
        int pulse = (int)((model->tick / 2u) % 16u);
        DrawCircleLines(node.x, node.y + 2, 16 + pulse / 2, FL_PAPER);
        DrawCircleLines(node.x, node.y + 2, 15 + pulse / 2, FL_INK);
    }
    /* A paper plate under the token lifts it off busy ground. */
    DrawCircle(node.x, node.y + bob, 17, FL_PAPER);
    DrawCircleLines(node.x, node.y + bob, 18, FL_INK);
    draw_padded(s_assets.props, open ? s_assets.nodes[i] : s_assets.node_locked,
                (float)(node.x - 20), (float)(node.y - 18 + bob), PAD_DRAW_SIZE);
    if (picked) {
        /* A banner planted beside the token, the same cloth as the gate's. */
        int pole_x = node.x + 14;
        int pole_top = node.y - 34 + bob;
        DrawRectangle(pole_x - 1, pole_top, 3, 26, FL_PAPER);
        DrawRectangle(pole_x, pole_top + 1, 1, 24, FL_INK);
        int flag = (int)((model->tick / 5u) % CASTLE_FLAG_FRAMES);
        draw_padded(s_assets.castle_fx, s_assets.flag[flag],
                    (float)(pole_x - (int)CASTLE_FX_SIZE + 2),
                    (float)(pole_top - 2), CASTLE_FX_SIZE);
    }
    /* Little pips under a cleared stage: one per star earned. */
    for (int s = 0; s < model->level_stars[i] && s < 3; ++s) {
        int px = node.x - 8 + s * 7;
        int py = node.y + 15;
        DrawRectangle(px - 1, py - 1, 5, 5, FL_INK);
        DrawRectangle(px, py, 3, 3, FL_PAPER);
    }
}

static void draw_menu_card(const fl_state_t *model)
{
    uint8_t i = model->menu_level < FL_LEVEL_COUNT ? model->menu_level : 0;
    const fl_level_t *level = &FL_LEVELS[i];
    int x = FL_MENU_CARD_X;
    int y = FL_MENU_CARD_Y;
    DrawRectangle(x, y, FL_MENU_CARD_W, FL_MENU_CARD_H, FL_INK);
    DrawRectangleLines(x, y, FL_MENU_CARD_W, FL_MENU_CARD_H, FL_PAPER);
    DrawRectangle(x, y + 2, FL_MENU_CARD_W, 1, FL_PAPER);

    draw_padded(s_assets.props, s_assets.nodes[i], (float)(x + 6),
                (float)(y + 10), PAD_DRAW_SIZE);
    DrawText(TextFormat("STAGE %u", (unsigned)i + 1), x + 52, y + 8, 12, FL_PAPER);
    DrawText(level->name, x + 52, y + 22, 16, FL_PAPER);
    if (model->level_stars[i] > 0) {
        for (int s = 0; s < 3; ++s) {
            int px = x + 54 + s * 10;
            int py = y + 46;
            if (s < model->level_stars[i]) {
                DrawRectangle(px, py, 6, 6, FL_PAPER);
            } else {
                DrawRectangleLines(px, py, 6, 6, FL_PAPER);
            }
        }
        DrawText(TextFormat("BEST %lu", (unsigned long)model->level_best[i]),
                 x + 90, y + 44, 12, FL_PAPER);
    } else {
        DrawText(TextFormat("%u WAVES", (unsigned)level->wave_count),
                 x + 52, y + 44, 12, FL_PAPER);
        /* A stage that brings a new tower says so, portrait and all. */
        fl_tower_kind_t fresh = fl_new_tower(i);
        if (fresh != FL_TOWER_NONE) {
            int nx = x + 52 + MeasureText("00 WAVES", 12) + 12;
            DrawText(TextFormat("NEW %s", FL_TOWER_LABELS[fresh]), nx + 24,
                     y + 44, 12, FL_PAPER);
            draw_padded(s_assets.icons, s_assets.cards[fresh], (float)(nx - 2),
                        (float)(y + 36), (float)ICON_DRAW_SIZE);
        }
    }
    draw_button(FL_MENU_GO_X, FL_MENU_GO_Y, FL_MENU_GO_W, FL_MENU_GO_H, "FIGHT",
                true, true, model->tick);
}

static void draw_menu(const fl_state_t *model)
{
    draw_full(s_assets.terrain, s_assets.campaign_map, 0.0f, 0.0f, 480.0f);

    /* Snow drifts across the map the way it does over a battlefield. */
    for (int i = 0; i < 18; ++i) {
        uint32_t speed = 1u + (uint32_t)(i % 3);
        int x = (int)((i * 67u + model->tick * speed) % 400u) + 40;
        int y = (int)((i * 41u + model->tick * (speed + 1u)) % 300u) + 50;
        if (corner_cut(x, y)) {
            continue;
        }
        DrawRectangle(x, y, 1, 1, FL_PAPER);
        DrawRectangle(x + 1, y, 1, 1, FL_INK);
    }

    uint8_t count = fl_level_count();
    for (uint8_t i = 0; i + 1 < count; ++i) {
        bool walked = model->level_stars[i] > 0;
        bool frontier = walked && !(model->level_stars[i + 1] > 0);
        int march = frontier ? (int)(model->tick / 3u) : 0;
        draw_trail(FL_LEVELS[i].node, FL_LEVELS[i + 1].node,
                   walked ? 7 : 10, march, walked);
    }
    for (uint8_t i = 0; i < count; ++i) {
        draw_node(model, i);
    }
    draw_menu_card(model);
}

void frostline_view_draw(const fl_state_t *model)
{
    if (!model) {
        return;
    }
    if (model->phase == FL_PHASE_MENU) {
        draw_menu(model);
        return;
    }
    draw_background(model);
    draw_world_motion(model);
    draw_build_pads(model);
    draw_zones(model);
    draw_selection_reach(model);
    draw_actors(model);
    draw_projectiles(model);
    draw_arcs(model);
    draw_ring(model);
    draw_hud(model);
    if (model->selected_tower >= 0) {
        draw_upgrade_panel(model);
    }
    draw_pause(model);
    draw_overlay(model);
}

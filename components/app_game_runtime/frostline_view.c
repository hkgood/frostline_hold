/* Rendering for Frostline Hold.
 *
 * The view owns every drawing decision; the model owns every rule. All colour
 * is ink or paper so the panel keeps the 1-bit look, and all tone comes from
 * dithered textures baked at build time.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "private/frostline_view_internal.h"
#include "include/frostline_text.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib_lite_2d.h"
#include "raylib_lite_assets.h"
#include "raylib_lite_raylib.h"

/* Where the moving parts hang on the castle plate baked into the map. These
 * mirror CASTLE_* in assets_src/artgen/ai_terrain.py and must agree. */
#define CASTLE_ORIGIN_X (FL_GOAL.x - 92)
#define CASTLE_ORIGIN_Y (FL_GOAL.y + 16 - 100)
#define CASTLE_POLE_X (CASTLE_ORIGIN_X + 93)
#define CASTLE_POLE_TOP (CASTLE_ORIGIN_Y + 12)
static const int CASTLE_BRAZIERS[2][2] = {{66, 89}, {112, 91}};

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

view_assets_t fv_assets;

/* A missing atlas stops the game at start-up, so name it on the console. */
static raylib_lite_atlas_t load_atlas(const char *name)
{
    raylib_lite_atlas_t atlas = raylib_lite_atlas_load(name);
    if (atlas.texture.id == 0) {
        printf("frostline: cannot load %s\n", name);
    }
    return atlas;
}

void frostline_view_init(void)
{
    memset(&fv_assets, 0, sizeof(fv_assets));
    fv_assets.terrain = fl_terrain_init();
    if (!fv_assets.terrain) {
        printf("frostline: no memory for the terrain\n");
    }
    fv_assets.units = load_atlas("units.atlas");
    fv_assets.heroes = load_atlas("heroes.atlas");
    fv_assets.props = load_atlas("props.atlas");
    fv_assets.props_husk = load_atlas("props_husk.atlas");
    fv_assets.props_titan = load_atlas("props_titan.atlas");
    fv_assets.fx = load_atlas("fx.atlas");
    fv_assets.icons = load_atlas("icons.atlas");
    fv_assets.castle_fx = load_atlas("castle_fx.atlas");
    fv_assets.title = load_atlas("title.atlas");
    fv_assets.title_backdrop = raylib_lite_asset_id("title_backdrop");
    fv_assets.ending_backdrop = raylib_lite_asset_id("ending_backdrop");
    fv_assets.logo[FL_LANG_ZH] = raylib_lite_asset_id("logo_zh");
    fv_assets.logo[FL_LANG_EN] = raylib_lite_asset_id("logo_en");

    for (int i = 0; i < FL_STAGE_COUNT; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "node_%d", i);
        fv_assets.nodes[i] = raylib_lite_asset_id(name);
    }
    fv_assets.node_locked = raylib_lite_asset_id("node_locked");
    towers_init();
    for (int kind = FL_TOWER_GUARD; kind < FL_TOWER_KIND_COUNT; ++kind) {
        char name[32];
        snprintf(name, sizeof(name), "card_%s", FL_TOWER_KEYS[kind]);
        fv_assets.cards[kind] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < EMBER_FLAME_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "ember_pot_%d", i);
        fv_assets.ember_pot[i] = raylib_lite_asset_id(name);
        snprintf(name, sizeof(name), "ember_flame_%d", i);
        fv_assets.ember_flame[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < 6; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "spark_%d", i);
        fv_assets.spark[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < SOLDIER_WALK_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "soldier_%d", i);
        fv_assets.soldier_walk[i] = raylib_lite_asset_id(name);
    }
    fv_assets.soldier_strike = raylib_lite_asset_id("soldier_strike");
    for (int kind = 0; kind < FL_ENEMY_KIND_COUNT; ++kind) {
        const char *key = FL_ENEMY_KEYS[kind];
        const char *alt = ENEMY_LOOKS[kind].alt;
        for (int frame = 0; frame < ENEMY_FRAMES; ++frame) {
            char name[40];
            snprintf(name, sizeof(name), "enemy_%s_%d", key, frame);
            fv_assets.enemies[kind][frame] = raylib_lite_asset_id(name);
            if (alt != NULL) {
                snprintf(name, sizeof(name), "enemy_%s_%s_%d", key, alt, frame);
            }
            fv_assets.enemies_alt[kind][frame] = raylib_lite_asset_id(name);
        }
    }
    for (int i = 0; i < 2; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "build_pad_%d", i);
        fv_assets.pads[i] = raylib_lite_asset_id(name);
    }
    fv_assets.projectile = raylib_lite_asset_id("projectile_0_0");
    fv_assets.icon_heart = raylib_lite_asset_id("icon_heart");
    fv_assets.icon_coin = raylib_lite_asset_id("icon_coin");
    fv_assets.icon_score = raylib_lite_asset_id("icon_score");
    fv_assets.icon_nova = raylib_lite_asset_id("icon_nova");
    fv_assets.icon_star = raylib_lite_asset_id("icon_star");
    fv_assets.card_hero = raylib_lite_asset_id("card_hero");
    for (int i = 0; i < 4; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "hero_warden_%d", i);
        fv_assets.hero[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < CASTLE_FLAG_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "castle_flag_%d", i);
        fv_assets.flag[i] = raylib_lite_asset_id(name);
    }
    for (int i = 0; i < CASTLE_FLAME_FRAMES; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "castle_flame_%d", i);
        fv_assets.flame[i] = raylib_lite_asset_id(name);
    }
}

void frostline_view_shutdown(void)
{
    fl_terrain_shutdown();
    towers_shutdown();
    raylib_lite_atlas_unload(fv_assets.units);
    raylib_lite_atlas_unload(fv_assets.heroes);
    raylib_lite_atlas_unload(fv_assets.props);
    raylib_lite_atlas_unload(fv_assets.props_husk);
    raylib_lite_atlas_unload(fv_assets.props_titan);
    raylib_lite_atlas_unload(fv_assets.fx);
    raylib_lite_atlas_unload(fv_assets.icons);
    raylib_lite_atlas_unload(fv_assets.castle_fx);
    raylib_lite_atlas_unload(fv_assets.title);
    memset(&fv_assets, 0, sizeof(fv_assets));
}

bool frostline_view_ready(void)
{
    return fv_assets.terrain && fv_assets.units.texture.id != 0
           && towers_ready()
           && fv_assets.heroes.texture.id != 0
           && fv_assets.props.texture.id != 0 && fv_assets.fx.texture.id != 0
           && fv_assets.props_husk.texture.id != 0
           && fv_assets.props_titan.texture.id != 0
           && fv_assets.icons.texture.id != 0
           && fv_assets.castle_fx.texture.id != 0
           && fv_assets.title.texture.id != 0;
}

/* ------------------------------------------------------------------ */

void draw_padded(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
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

void draw_full(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
                      float x, float y, float size)
{
    const raylib_lite_sprite_frame_t *frame = raylib_lite_atlas_get_frame(atlas, id);
    if (!frame) {
        return;
    }
    DrawTexturePro(atlas.texture, frame->source, (Rectangle){x, y, size, size},
                   (Vector2){0.0f, 0.0f}, 0.0f, TINT_NONE);
}

/* A padded frame at its own size: for sheets whose cells differ. */
bool draw_native(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
                 float x, float y)
{
    const raylib_lite_sprite_frame_t *frame = raylib_lite_atlas_get_frame(atlas, id);
    if (!frame) {
        return false;
    }
    Rectangle source = frame->source;
    source.x += SPRITE_PAD;
    source.y += SPRITE_PAD;
    source.width -= SPRITE_PAD * 2;
    source.height -= SPRITE_PAD * 2;
    DrawTexturePro(atlas.texture, source,
                   (Rectangle){x, y, source.width, source.height},
                   (Vector2){0.0f, 0.0f}, 0.0f, TINT_NONE);
    return true;
}

static const uint8_t BAYER4[4][4] = {
    {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5},
};

void draw_dither_ellipse(int cx, int cy, int rx, int ry, float density,
                         Color color)
{
    if (rx <= 0 || ry <= 0 || density <= 0.0f) {
        return;
    }
    int level = (int)(density * 16.0f);
    for (int dy = -ry; dy <= ry; ++dy) {
        float v = (float)dy / (float)ry;
        int half = (int)((float)rx * sqrtf(1.0f - v * v));
        int y = cy + dy;
        const uint8_t *row = BAYER4[y & 3];
        /* Paint runs of the cells the pattern lets through. */
        int x = cx - half;
        while (x <= cx + half) {
            while (x <= cx + half && row[x & 3] >= level) {
                ++x;
            }
            int start = x;
            while (x <= cx + half && row[x & 3] < level) {
                ++x;
            }
            if (start < x) {
                DrawRectangle(start, y, x - start, 1, color);
            }
        }
    }
}

void draw_dither_disc(int cx, int cy, int r, float density, Color color)
{
    draw_dither_ellipse(cx, cy, r, r, density, color);
}

/* Five points, ink case and paper face, so it reads on either tone. */
void draw_star(int cx, int cy, int r, bool filled)
{
    Vector2 outer[5], inner[5];
    for (int k = 0; k < 5; ++k) {
        float a = -1.5707963f + (float)k * 1.2566371f;
        float b = a + 0.6283185f;
        outer[k] = (Vector2){(float)cx + cosf(a) * (float)r,
                             (float)cy + sinf(a) * (float)r};
        inner[k] = (Vector2){(float)cx + cosf(b) * (float)r * 0.45f,
                             (float)cy + sinf(b) * (float)r * 0.45f};
    }
    Vector2 c = {(float)cx, (float)cy};
    if (filled) {
        for (int k = 0; k < 5; ++k) {
            DrawTriangle(outer[k], c, inner[k], FL_PAPER);
            DrawTriangle(inner[(k + 4) % 5], c, outer[k], FL_PAPER);
        }
    }
    for (int k = 0; k < 5; ++k) {
        DrawLineEx(outer[k], inner[k], 2.0f, FL_INK);
        DrawLineEx(inner[(k + 4) % 5], outer[k], 2.0f, FL_INK);
    }
    if (!filled) {
        /* A paper hairline inside the ink, so the empty star reads on ink. */
        for (int k = 0; k < 5; ++k) {
            DrawLineV(outer[k], inner[k], FL_PAPER);
            DrawLineV(inner[(k + 4) % 5], outer[k], FL_PAPER);
        }
    }
}

const char *level_name(uint8_t level)
{
    if (level >= FL_LEVEL_COUNT) {
        level = 0;
    }
    return fl_tr((fl_text_id_t)(FL_TXT_LEVEL_FIRST + level));
}

const char *tower_name(int kind)
{
    if (kind < 0 || kind >= FL_TOWER_KIND_COUNT) {
        kind = 0;
    }
    return fl_tr((fl_text_id_t)(FL_TXT_TOWER_NONE + kind));
}

const char *upgrade_name(int kind, int stage, int option)
{
    if (kind <= FL_TOWER_NONE || kind >= FL_TOWER_KIND_COUNT || stage < 0
            || stage > 1 || option < 0 || option > 1) {
        return "";
    }
    return fl_tr((fl_text_id_t)(FL_TXT_UPGRADE_FIRST + (kind - 1) * 4
                                + stage * 2 + option));
}

const enemy_look_t *enemy_look(int kind)
{
    return &ENEMY_LOOKS[kind >= 0 && kind < FL_ENEMY_KIND_COUNT ? kind : 0];
}

raylib_lite_atlas_t enemy_sheet(int kind, float *size)
{
    const enemy_look_t *look = enemy_look(kind);
    if (look->sheet == SHEET_HUSK) {
        *size = HUSK_DRAW_SIZE;
        return fv_assets.props_husk;
    }
    if (look->sheet == SHEET_TITAN) {
        *size = TITAN_DRAW_SIZE;
        return fv_assets.props_titan;
    }
    *size = ENEMY_DRAW_SIZE;
    return fv_assets.props;
}

void draw_panel(int x, int y, int w, int h, bool inverted)
{
    DrawRectangle(x, y, w, h, inverted ? FL_PAPER : FL_INK);
    DrawRectangleLines(x, y, w, h, inverted ? FL_INK : FL_PAPER);
}

/* --- stone plates ------------------------------------------------------ */

/* Where the finger is, so the plate under it can sink. */
static struct {
    int x, y;
    bool down;
} s_touch;

void frostline_view_set_pointer(int x, int y, bool down)
{
    s_touch.x = x;
    s_touch.y = y;
    s_touch.down = down;
}

static bool touch_inside(int x, int y, int w, int h)
{
    return s_touch.down && s_touch.x >= x && s_touch.x < x + w
           && s_touch.y >= y && s_touch.y < y + h;
}

/* The corners step in one pixel per row, like a chiselled tablet. */
void fill_plate(int x, int y, int w, int h, int cut, Color color)
{
    DrawRectangle(x, y + cut, w, h - 2 * cut, color);
    for (int i = 0; i < cut; ++i) {
        int inset = cut - i;
        DrawRectangle(x + inset, y + i, w - 2 * inset, 1, color);
        DrawRectangle(x + inset, y + h - 1 - i, w - 2 * inset, 1, color);
    }
}

void outline_plate(int x, int y, int w, int h, int cut, Color color)
{
    DrawRectangle(x + cut, y, w - 2 * cut, 1, color);
    DrawRectangle(x + cut, y + h - 1, w - 2 * cut, 1, color);
    DrawRectangle(x, y + cut, 1, h - 2 * cut, color);
    DrawRectangle(x + w - 1, y + cut, 1, h - 2 * cut, color);
    for (int i = 1; i < cut; ++i) {
        DrawPixel(x + cut - i, y + i, color);
        DrawPixel(x + w - 1 - cut + i, y + i, color);
        DrawPixel(x + cut - i, y + h - 1 - i, color);
        DrawPixel(x + w - 1 - cut + i, y + h - 1 - i, color);
    }
}

void draw_frame_panel(int x, int y, int w, int h)
{
    fill_plate(x, y, w, h, 10, FL_INK);
    outline_plate(x, y, w, h, 10, FL_PAPER);
    outline_plate(x + 4, y + 4, w - 8, h - 8, 8, FL_PAPER);
    /* A rivet in each cut corner, between the two lines. */
    DrawRectangle(x + 6, y + 6, 2, 2, FL_PAPER);
    DrawRectangle(x + w - 8, y + 6, 2, 2, FL_PAPER);
    DrawRectangle(x + 6, y + h - 8, 2, 2, FL_PAPER);
    DrawRectangle(x + w - 8, y + h - 8, 2, 2, FL_PAPER);
}

/* A small lozenge, the stud either side of a wide plate's label. */
static void draw_stud(int cx, int cy, Color color)
{
    DrawRectangle(cx - 1, cy - 2, 3, 5, color);
    DrawRectangle(cx - 2, cy - 1, 5, 3, color);
}

bool draw_plate(int x, int y, int w, int h, plate_style_t style, int *face_y,
                int *face_h)
{
    /* The plate keeps to its rectangle: the stone's thickness takes the
     * bottom rows, and a pressed plate sinks into them. */
    const int depth = h >= 36 ? PLATE_DEPTH : PLATE_DEPTH - 1;
    const int cut = h >= 36 ? PLATE_CUT : PLATE_CUT - 1;
    bool pressed = style != PLATE_OFF && touch_inside(x, y, w, h);
    int top = pressed ? y + depth : y;
    int fh = h - depth;
    if (style == PLATE_OFF) {
        /* Not on offer: only the engraved outline, no stone to press. */
        outline_plate(x, top, w, fh, cut, FL_PAPER);
    } else if (style == PLATE_DARK) {
        /* An ink plate on the ink bar: a paper rim and a paper lip. */
        if (!pressed) {
            fill_plate(x, y + depth, w, fh, cut, FL_PAPER);
        }
        fill_plate(x, top, w, fh, cut, FL_PAPER);
        fill_plate(x + 1, top + 1, w - 2, fh - 2, cut - 1, FL_INK);
    } else {
        if (!pressed) {
            fill_plate(x, y + depth, w, fh, cut, FL_PAPER);
        }
        fill_plate(x, top, w, fh, cut, FL_PAPER);
        outline_plate(x + 1, top + 1, w - 2, fh - 2, cut - 1, FL_INK);
        if (style == PLATE_GRAND) {
            outline_plate(x + 4, top + 4, w - 8, fh - 8, cut - 2, FL_INK);
        }
    }
    if (face_y) {
        *face_y = top;
    }
    if (face_h) {
        *face_h = fh;
    }
    return pressed;
}

/* A right-pointing play triangle of half-height r, centred on (cx, cy). */
static void draw_play_icon(int cx, int cy, int r, Color color)
{
    DrawTriangle((Vector2){(float)(cx - r * 3 / 4), (float)(cy - r)},
                 (Vector2){(float)(cx - r * 3 / 4), (float)(cy + r)},
                 (Vector2){(float)(cx + r), (float)cy}, color);
}

/* Numbers keep the engine's big digits in either language. */
static void draw_text_centered_engine(const char *text, int center_x, int y,
                                      int size, Color color)
{
    DrawText(text, center_x - MeasureText(text, size) / 2, y, size, color);
}

/* ------------------------------------------------------------------ */

const fl_level_t *current_level(const fl_state_t *model)
{
    return &FL_LEVELS[model->level < FL_LEVEL_COUNT ? model->level : 0];
}

static void draw_background(const fl_state_t *model)
{
    uint8_t level = model->level < FL_LEVEL_COUNT ? model->level : 0;
    fl_terrain_draw_level(level);
}

bool corner_cut(int x, int y)
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

/* A travelling glint on the road, and the gate's cloth and fire.
 * The stone of the castle is baked into the map; only the parts that move
 * are drawn here. */
static void draw_world_motion(const fl_state_t *model)
{
    uint32_t tick = model->tick;
    const fl_level_t *level = current_level(model);
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
    draw_padded(fv_assets.castle_fx, fv_assets.flag[flag],
                (float)(CASTLE_POLE_X - (int)CASTLE_FX_SIZE + 2),
                (float)(CASTLE_POLE_TOP - 2), CASTLE_FX_SIZE);

    /* The braziers burn out of step with each other. */
    for (int i = 0; i < 2; ++i) {
        int flame = (int)((tick / 3u + (unsigned)i * 2u) % CASTLE_FLAME_FRAMES);
        int bx = CASTLE_ORIGIN_X + CASTLE_BRAZIERS[i][0];
        int by = CASTLE_ORIGIN_Y + CASTLE_BRAZIERS[i][1];
        draw_padded(fv_assets.castle_fx, fv_assets.flame[flame],
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
        draw_padded(fv_assets.props, fv_assets.pads[frame],
                    (float)(spot->x - PAD_DRAW_SIZE / 2),
                    (float)(spot->y - PAD_DRAW_SIZE / 2), PAD_DRAW_SIZE);
    }
    for (int i = 0; i < level->post_count && i < FL_MAX_HEROES; ++i) {
        if (model->heroes[i].active) {
            continue;
        }
        const fl_point_t *post = &level->posts[i];
        int frame = model->ring_post == i ? breathe : 0;
        draw_padded(fv_assets.props, fv_assets.pads[frame],
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
void draw_dotted_circle(int cx, int cy, int radius, int march)
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
    draw_padded(fv_assets.icons, portrait, (float)(x - ICON_DRAW_SIZE / 2),
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
        return fl_tr(FL_TXT_UI_HERO_NAME);
    }
    return entry < FL_TOWER_KIND_COUNT ? tower_name(entry) : "";
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
            entry == FL_RING_HERO ? fv_assets.card_hero : fv_assets.cards[entry];
        draw_medallion(sx, sy, portrait, pick == i, affordable, model->tick);
        draw_price_tag(sx, sy + FL_RING_ICON_R - 4, cost, affordable);
    }

    /* The picked tower's name on a plate under the ring, or over it when
     * the ring sits low on the glass; the second tap builds it. */
    if (pick >= 0 && open >= 1.0f) {
        const char *name = ring_label(fl_ring_entry(model, pick));
        const char *hint = fl_tr(FL_TXT_UI_TAP_AGAIN);
        int w = fl_text_width(name, FL_FONT_LARGE)
                + fl_text_width(hint, FL_FONT_SMALL) + 26;
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
        fl_text_draw(name, x + 8, y + (h - fl_text_height(FL_FONT_LARGE)) / 2,
                     FL_FONT_LARGE, FL_PAPER);
        fl_text_draw(hint, x + w - 8 - fl_text_width(hint, FL_FONT_SMALL),
                     y + (h - fl_text_height(FL_FONT_SMALL)) / 2, FL_FONT_SMALL,
                     FL_PAPER);
    }
}

static void draw_hero(const fl_state_t *model, int i)
{
    const fl_hero_t *hero = &model->heroes[i];
    int frame = (int)((model->tick / 8u) % 4u);
    float x = (float)hero->x - HERO_DRAW_SIZE / 2.0f;
    float y = (float)hero->y - HERO_DRAW_SIZE + SPRITE_FOOT;
    draw_padded(fv_assets.heroes, fv_assets.hero[frame], x, y, HERO_DRAW_SIZE);
    /* Health bar: an ink trough with a paper core that drains. */
    int bar_x = hero->x - 14;
    int bar_y = (int)y - 4;
    DrawRectangle(bar_x, bar_y, 28, 4, FL_INK);
    int filled = hero->max_hp ? (int)((int)hero->hp * 26 / (int)hero->max_hp) : 0;
    DrawRectangle(bar_x + 1, bar_y + 1, filled, 2, FL_PAPER);
}

/* The shadow a flyer casts on the road: a dithered oval, ink on every other
 * pixel, so the ground still shows through it. */
void draw_ground_shadow(int cx, int cy, int rx, int ry)
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

/* A champion's crown: three paper points on an ink band, sat on its bar. */
static void draw_crown(int cx, int bottom)
{
    DrawRectangle(cx - 6, bottom - 3, 13, 4, FL_INK);
    DrawRectangle(cx - 5, bottom - 2, 11, 2, FL_PAPER);
    for (int k = -1; k <= 1; ++k) {
        int px = cx + k * 5;
        DrawRectangle(px - 1, bottom - 8, 3, 6, FL_INK);
        DrawRectangle(px, bottom - 7, 1, 5, FL_PAPER);
    }
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
    raylib_lite_atlas_t sheet = fv_assets.props;
    if (look->sheet == SHEET_HUSK) {
        size = HUSK_DRAW_SIZE;
        sheet = fv_assets.props_husk;
    } else if (look->sheet == SHEET_TITAN) {
        size = TITAN_DRAW_SIZE;
        sheet = fv_assets.props_titan;
    }
    /* Each enemy walks on its own beat, so a column does not march in step. */
    int frame = (int)((model->tick / look->step + (unsigned)i * 2u) % ENEMY_FRAMES);
    /* The second look: a golem out of its shell, a burrower under the road. */
    bool alt = (kind == FL_ENEMY_GOLEM && enemy->stage > 0) || enemy->burrowed;
    raylib_lite_asset_id_t id = alt ? fv_assets.enemies_alt[kind][frame]
                                    : fv_assets.enemies[kind][frame];
    /* Whole pixels only: a fractional position would smear the dither. */
    int ex = (int)enemy->x;
    int ey = (int)enemy->y;
    int jx, jy;
    fx_enemy_jolt(i, &jx, &jy);
    ex += jx;
    ey += jy;
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
    bool boss = look->sheet == SHEET_TITAN || enemy->champion;
    float ratio = enemy->max_hp > 0.0f ? enemy->hp / enemy->max_hp : 0.0f;
    if (!boss && ratio >= 1.0f && enemy->slow_ticks == 0) {
        return;
    }
    int bar_w = boss ? 40 : (size > ENEMY_DRAW_SIZE ? 26 : 18);
    int bar_x = ex - bar_w / 2;
    int bar_y = fl_enemy_flies(enemy) ? (int)y + 8 : (int)y - 4;
    DrawRectangle(bar_x - 1, bar_y - 1, bar_w + 2, 4, FL_INK);
    DrawRectangle(bar_x, bar_y, (int)(ratio * (float)bar_w), 2, FL_PAPER);
    if (enemy->champion) {
        draw_crown(ex, bar_y - 2);
    }
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
        id = fv_assets.soldier_strike;
    } else if (soldier->moving) {
        id = fv_assets.soldier_walk[(model->tick / 4u + (unsigned)i)
                                   % SOLDIER_WALK_FRAMES];
    } else {
        /* At its post it shifts its weight now and then. */
        id = fv_assets.soldier_walk[((model->tick / 24u + (unsigned)i) % 4u) == 0u
                                       ? 1 : 0];
    }
    int sx = (int)soldier->x;
    int sy = (int)soldier->y;
    float x = (float)sx - SOLDIER_DRAW_SIZE / 2.0f;
    float y = (float)sy - SOLDIER_DRAW_SIZE + SPRITE_FOOT;
    draw_padded(fv_assets.props, id, x, y, SOLDIER_DRAW_SIZE);
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
            towers_draw(model, actors[i].index);
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

/* A cheap integer hash, so every bolt jags its own way frame to frame
 * without any state in the view. */
uint32_t jag_hash(uint32_t a)
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
void draw_shard(int x, int y, int r)
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
                draw_padded(fv_assets.fx, fv_assets.spark[frame],
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
            draw_padded(fv_assets.fx, fv_assets.ember_flame[frame],
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
    int face_y, face_h;
    draw_plate(FL_SKILL_BTN_X, FL_SKILL_BTN_Y, FL_SKILL_BTN_W, FL_SKILL_BTN_H,
               ready ? PLATE_PLAIN : PLATE_DARK, &face_y, &face_h);
    if (ready) {
        draw_padded(fv_assets.icons, fv_assets.icon_nova,
                    (float)(FL_SKILL_BTN_X + 1),
                    (float)(face_y + face_h / 2 - ICON_DRAW_SIZE / 2),
                    (float)ICON_DRAW_SIZE);
    } else if (hero_placed) {
        DrawText("...", FL_SKILL_BTN_X + 10, face_y + 8, 14, FL_PAPER);
    } else {
        /* No hero yet: a thin snowflake marks the nova's slot, not a hole. */
        const int cx = FL_SKILL_BTN_X + FL_SKILL_BTN_W / 2;
        const int cy = face_y + face_h / 2;
        DrawLine(cx, cy - 8, cx, cy + 8, FL_PAPER);
        DrawLine(cx - 7, cy - 4, cx + 7, cy + 4, FL_PAPER);
        DrawLine(cx - 7, cy + 4, cx + 7, cy - 4, FL_PAPER);
    }

    /* The icons are shaded solids with paper highlights, so they read on
     * the ink bar without a plate. The cluster ends before the pause button. */
    const float icon_y = (float)(FL_HUD_Y + 2);
    draw_padded(fv_assets.icons, fv_assets.icon_heart, 78.0f, icon_y,
                (float)ICON_DRAW_SIZE);
    DrawText(TextFormat("%u", (unsigned)model->lives), 106, 14, 16, FL_PAPER);
    draw_padded(fv_assets.icons, fv_assets.icon_coin, 140.0f, icon_y,
                (float)ICON_DRAW_SIZE);
    DrawText(TextFormat("%u", (unsigned)model->gold), 168, 14, 16, FL_PAPER);

    /* Points read as a third resource so the run has a running tally. */
    draw_padded(fv_assets.icons, fv_assets.icon_score, 212.0f, icon_y,
                (float)ICON_DRAW_SIZE);
    DrawText(TextFormat("%lu", (unsigned long)model->score), 240, 14, 16, FL_PAPER);

    fl_text_draw(fl_tr(FL_TXT_UI_HUD_WAVE), 304,
                 fl_text_language() == FL_LANG_ZH ? 6 : 8, FL_FONT_SMALL, FL_PAPER);
    DrawText(TextFormat("%02u/%02u", (unsigned)model->wave,
                        (unsigned)fl_wave_count(model)),
             304, 20, 14, FL_PAPER);

    /* The pause button shows what a tap does: two bars while the battle
     * runs, a lit play triangle while it waits. */
    draw_plate(FL_PAUSE_BTN_X, FL_BTN_Y, FL_PAUSE_BTN_W, FL_BTN_H,
               model->paused ? PLATE_PLAIN : PLATE_DARK, &face_y, &face_h);
    const int pause_mid = face_y + face_h / 2;
    if (model->paused) {
        draw_play_icon(FL_PAUSE_BTN_X + FL_PAUSE_BTN_W / 2 + 1, pause_mid, 7,
                       FL_INK);
    } else {
        DrawRectangle(FL_PAUSE_BTN_X + 12, pause_mid - 6, 5, 12, FL_PAPER);
        DrawRectangle(FL_PAUSE_BTN_X + 20, pause_mid - 6, 5, 12, FL_PAPER);
    }
    draw_plate(FL_SPEED_BTN_X, FL_BTN_Y, FL_SPEED_BTN_W, FL_BTN_H, PLATE_DARK,
               &face_y, &face_h);
    draw_text_centered_engine(TextFormat("%ux", (unsigned)model->speed),
                              FL_SPEED_BTN_X + FL_SPEED_BTN_W / 2,
                              face_y + face_h / 2 - 7, 16, FL_PAPER);
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

    /* The top line: name and level, the numbers, the sell price. Every
     * piece is centred on one baseline band whatever the language. */
    const int line_mid = FL_PANEL_TOP + 12;
    const char *label = tower_name(tower->kind);
    int label_h = fl_text_height(FL_FONT_LARGE);
    fl_text_draw(label, FL_PANEL_X + 8, line_mid - label_h / 2, FL_FONT_LARGE,
                 FL_PAPER);
    int label_end = FL_PANEL_X + 8 + fl_text_width(label, FL_FONT_LARGE);
    int small_y = line_mid - fl_text_height(FL_FONT_SMALL) / 2;
    const char *level = fl_fmt("LV%u", (unsigned)tower->level);
    fl_text_draw(level, label_end + 6, small_y, FL_FONT_SMALL, FL_PAPER);
    int numbers_x = label_end + 6 + fl_text_width(level, FL_FONT_SMALL) + 10;

    /* The numbers that matter for this kind of tower. */
    fl_tower_stats_t stat = fl_tower_stats(tower);
    const char *numbers;
    switch (tower->kind) {
    case FL_TOWER_STORM:
        numbers = fl_trf(FL_TXT_UI_STAT_STORM, stat.damage, stat.range,
                         stat.extra);
        break;
    case FL_TOWER_BARRACKS:
        numbers = fl_trf(FL_TXT_UI_STAT_BARRACKS, stat.extra, stat.hp,
                         stat.damage);
        break;
    case FL_TOWER_EMBER:
        numbers = fl_trf(FL_TXT_UI_STAT_EMBER, stat.damage, stat.range,
                         stat.extra / 30);
        break;
    default:
        numbers = fl_trf(FL_TXT_UI_STAT_DEFAULT, stat.damage, stat.range,
                         (int)stat.cooldown);
        break;
    }
    fl_text_draw(numbers, numbers_x, small_y, FL_FONT_SMALL, FL_PAPER);
    const char *sell = fl_trf(FL_TXT_UI_SELL_N, (unsigned)(tower->invested / 2));
    fl_text_draw(sell, FL_PANEL_X + FL_PANEL_W - 8 - fl_text_width(sell, FL_FONT_SMALL),
                 small_y, FL_FONT_SMALL, FL_PAPER);

    if (tower->level >= FL_TOWER_LEVEL_COUNT) {
        /* Show what the tower ended up as instead of an empty panel. */
        const char *line = fl_fmt("%s + %s",
                                  upgrade_name(tower->kind, 0,
                                               fl_tower_option(tower, 0)),
                                  upgrade_name(tower->kind, 1,
                                               fl_tower_option(tower, 1)));
        fl_text_draw_centered(line, FL_VIEW_WIDTH / 2,
                              FL_UPGRADE_CARD_Y + 20 - label_h / 2,
                              FL_FONT_LARGE, FL_PAPER);
        return;
    }

    int stage = (int)tower->level - 1;
    uint16_t cost = fl_upgrade_cost(model, model->selected_tower);
    bool affordable = model->gold >= cost;
    for (int i = 0; i < FL_TOWER_OPTION_COUNT; ++i) {
        int card_x = FL_UPGRADE_CARD_X0
                     + i * (FL_UPGRADE_CARD_W + FL_UPGRADE_CARD_GAP);
        int face_y, face_h;
        draw_plate(card_x, FL_UPGRADE_CARD_Y, FL_UPGRADE_CARD_W,
                   FL_UPGRADE_CARD_H, PLATE_PLAIN, &face_y, &face_h);
        fl_text_draw(upgrade_name(tower->kind, stage, i), card_x + 8,
                     face_y + 3, FL_FONT_LARGE, FL_INK);
        fl_text_draw(fl_fmt("LV%u", (unsigned)tower->level + 1), card_x + 8,
                     face_y + face_h - 3 - fl_text_height(FL_FONT_SMALL),
                     FL_FONT_SMALL, FL_INK);
        const char *price = TextFormat("%u", (unsigned)cost);
        int price_w = MeasureText(price, BUTTON_TEXT);
        int price_x = card_x + FL_UPGRADE_CARD_W - 9 - price_w;
        int price_y = face_y + 18;
        DrawText(price, price_x, price_y, BUTTON_TEXT, FL_INK);
        if (!affordable) {
            /* Strike the price through so an unaffordable upgrade reads. */
            DrawLine(price_x - 3, price_y + BUTTON_TEXT_HEIGHT - 2,
                     price_x + price_w + 3, price_y + 2, FL_INK);
        }
    }
}

void draw_button(int x, int y, int w, int h, const char *label,
                 bool enabled, bool beckon, uint32_t tick)
{
    draw_icon_button(x, y, w, h, label, false, enabled, beckon, tick);
}

void draw_icon_button(int x, int y, int w, int h, const char *label,
                      bool play_icon, bool enabled, bool beckon, uint32_t tick)
{
    /* Wide plates carry the inner line and the studs; a narrow one has no
     * room for them beside its label. */
    const int icon_r = 6;
    const int icon_w = play_icon ? icon_r * 2 + 6 : 0;
    const int label_w = fl_text_width(label, FL_FONT_LARGE) + icon_w;
    const bool grand = w >= 80;
    plate_style_t style = !enabled ? PLATE_OFF : grand ? PLATE_GRAND : PLATE_PLAIN;
    int face_y, face_h;
    draw_plate(x, y, w, h, style, &face_y, &face_h);
    int text_y = face_y + (face_h - fl_text_height(FL_FONT_LARGE)) / 2;
    if (!enabled) {
        fl_text_draw_centered(label, x + w / 2, text_y, FL_FONT_LARGE, FL_PAPER);
        return;
    }
    if (beckon && (tick / 15u) % 2u == 0u) {
        /* The button the player most likely wants breathes a second frame. */
        outline_plate(x - 3, y - 3, w + 6, h + 6, PLATE_CUT + 2, FL_PAPER);
    }
    if (grand && label_w + 48 <= w) {
        draw_stud(x + 11, face_y + face_h / 2, FL_INK);
        draw_stud(x + w - 12, face_y + face_h / 2, FL_INK);
    }
    /* The icon leads the label; the pair is centred as one. */
    const int left = x + (w - label_w) / 2;
    if (play_icon) {
        draw_play_icon(left + icon_r, face_y + face_h / 2, icon_r, FL_INK);
    }
    fl_text_draw(label, left + icon_w, text_y, FL_FONT_LARGE, FL_INK);
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

/* Three stars in a row: earned ones are the faceted star, the rest the
 * outline of the star that would sit there. */
static void draw_star_row(int x, int y, int stars)
{
    for (int i = 0; i < 3; ++i) {
        int sx = x + i * 34;
        if (i < stars) {
            draw_padded(fv_assets.icons, fv_assets.icon_star, (float)(sx - 1),
                        (float)y, (float)ICON_DRAW_SIZE);
        } else {
            draw_star(sx + ICON_DRAW_SIZE / 2 - 1, y + ICON_DRAW_SIZE / 2 + 1,
                      11, false);
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
    draw_frame_panel(64, 88, 352, 304);
    bool won = model->phase == FL_PHASE_WON;
    const char *title = fl_tr(won ? FL_TXT_UI_WON_TITLE : FL_TXT_UI_LOST_TITLE);
    fl_text_draw_centered(title, FL_VIEW_WIDTH / 2, 100, FL_FONT_HUGE, FL_PAPER);
    fl_text_draw_centered(level_name(model->level), FL_VIEW_WIDTH / 2, 138,
                          FL_FONT_LARGE, FL_PAPER);

    /* The points total, with the breakdown underneath it. */
    draw_text_centered_engine(TextFormat("%lu", (unsigned long)model->score),
                              FL_VIEW_WIDTH / 2, 162, 32, FL_PAPER);
    draw_star_row(FL_VIEW_WIDTH / 2 - 48, 196, fl_star_rating(model));

    fl_text_draw_centered(fl_trf(FL_TXT_UI_END_KILLS, (unsigned)model->kills,
                                 (unsigned long)model->score_waves),
                          FL_VIEW_WIDTH / 2, 234, FL_FONT_SMALL, FL_PAPER);
    fl_text_draw_centered(fl_trf(FL_TXT_UI_END_LIVES, (unsigned)model->lives,
                                 (unsigned long)model->score_bonus),
                          FL_VIEW_WIDTH / 2, 254, FL_FONT_SMALL, FL_PAPER);
    /* The wave counter steps past the last wave once the run is won. */
    unsigned waves = (unsigned)fl_wave_count(model);
    unsigned shown_wave = (unsigned)model->wave;
    if (shown_wave > waves) {
        shown_wave = waves;
    }
    fl_text_draw_centered(fl_trf(FL_TXT_UI_END_WAVE, shown_wave, waves,
                                 (unsigned long)model->level_best[model->level]),
                          FL_VIEW_WIDTH / 2, 274, FL_FONT_SMALL, FL_PAPER);

    /* Map, retry, and after a win either the next stage or, after the last
     * one, the ending. */
    bool last = model->level + 1 >= fl_level_count();
    bool has_next = won && !last;

    /* The reward for pushing on: the tower the next stage brings, its
     * portrait on a paper plate beside the news. */
    fl_tower_kind_t fresh = has_next ? fl_new_tower((uint8_t)(model->level + 1))
                                     : FL_TOWER_NONE;
    if (fresh != FL_TOWER_NONE) {
        const char *news = fl_trf(FL_TXT_UI_END_UNLOCK, tower_name(fresh));
        int w = fl_text_width(news, FL_FONT_SMALL) + ICON_DRAW_SIZE + 14;
        int x = FL_VIEW_WIDTH / 2 - w / 2;
        int blink = (model->tick / 20u) % 2u == 0u;
        DrawRectangle(x - 4, 296, w + 8, 28, blink ? FL_PAPER : FL_INK);
        DrawRectangleLines(x - 4, 296, w + 8, 28, FL_PAPER);
        DrawCircle(x + ICON_DRAW_SIZE / 2, 310, ICON_DRAW_SIZE / 2.0f, FL_INK);
        draw_padded(fv_assets.icons, fv_assets.cards[fresh], (float)x, 296.0f,
                    (float)ICON_DRAW_SIZE);
        fl_text_draw(news, x + ICON_DRAW_SIZE + 8,
                     310 - fl_text_height(FL_FONT_SMALL) / 2, FL_FONT_SMALL,
                     blink ? FL_INK : FL_PAPER);
    }
    const char *labels[FL_PANEL_BTN_COUNT] = {
        fl_tr(FL_TXT_UI_BTN_MAP), fl_tr(FL_TXT_UI_BTN_RETRY),
        fl_tr(won && last ? FL_TXT_UI_BTN_ENDING : FL_TXT_UI_BTN_NEXT),
    };
    const bool enabled[FL_PANEL_BTN_COUNT] = {true, true, won};
    draw_panel_buttons(labels, enabled,
                       won ? FL_PANEL_BTN_NEXT : FL_PANEL_BTN_RETRY,
                       model->tick);
}

static void draw_pause(const fl_state_t *model)
{
    if (!model->paused || model->phase == FL_PHASE_WON
            || model->phase == FL_PHASE_LOST) {
        return;
    }
    draw_frame_panel(64, 200, 352, 192);
    fl_text_draw_centered(fl_tr(FL_TXT_UI_PAUSED), FL_VIEW_WIDTH / 2, 218,
                          FL_FONT_HUGE, FL_PAPER);
    fl_text_draw_centered(level_name(model->level), FL_VIEW_WIDTH / 2, 258,
                          FL_FONT_LARGE, FL_PAPER);
    fl_text_draw_centered(fl_trf(FL_TXT_UI_WAVE_OF, (unsigned)model->wave,
                                 (unsigned)fl_wave_count(model)),
                          FL_VIEW_WIDTH / 2, 284, FL_FONT_SMALL, FL_PAPER);
    const char *labels[FL_PANEL_BTN_COUNT] = {
        fl_tr(FL_TXT_UI_BTN_MAP), fl_tr(FL_TXT_UI_BTN_RETRY),
        fl_tr(FL_TXT_UI_BTN_RESUME),
    };
    /* Resume is the likely tap: the same play triangle as the HUD button
     * leads its label, so the two read as one action. */
    for (int i = 0; i < FL_PANEL_BTN_COUNT; ++i) {
        int x, y, w, h;
        fl_panel_button_rect(i, &x, &y, &w, &h);
        draw_icon_button(x, y, w, h, labels[i], i == FL_PANEL_BTN_NEXT, true,
                         i == FL_PANEL_BTN_NEXT, model->tick);
    }
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
static void draw_trail(fl_point_t a, fl_point_t b, int clear_a, int clear_b,
                       int spacing, int march, bool bold)
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
        if (da >= (float)(clear_a * clear_a) && db >= (float)(clear_b * clear_b)) {
            draw_trail_dot((int)x, (int)y, bold);
        }
    }
}

/* How far the trail keeps off a node: a medallion is wider than a marker. */
static int node_clearance(uint8_t i)
{
    return FL_LEVELS[i].interlude ? 13 : 20;
}

/* A level on the screen, its node moved by the scroll. */
static fl_point_t node_on_screen(const fl_state_t *model, uint8_t i)
{
    fl_point_t node = FL_LEVELS[i].node;
    node.x = (int16_t)(node.x - model->menu_scroll);
    return node;
}

/* An interlude's marker: a waystone on the road, not a landmark. Open, it
 * is an ink stone with a paper heart; locked, a hollow ring of dots. */
static void draw_waystone(fl_point_t node, int bob, bool open)
{
    DrawCircle(node.x, node.y + bob, 10, FL_PAPER);
    DrawCircleLines(node.x, node.y + bob, 11, FL_INK);
    if (open) {
        DrawCircle(node.x, node.y + bob, 7, FL_INK);
        DrawCircle(node.x, node.y + bob, 3, FL_PAPER);
        DrawRectangle(node.x - 1, node.y + bob - 1, 2, 2, FL_INK);
    } else {
        draw_dotted_circle(node.x, node.y + bob, 6, 0);
    }
}

static void draw_node(const fl_state_t *model, uint8_t i)
{
    fl_point_t node = node_on_screen(model, i);
    bool interlude = FL_LEVELS[i].interlude;
    bool open = fl_level_unlocked(model, i);
    bool picked = model->menu_level == i;
    int radius = interlude ? 10 : 17;
    /* The picked token bobs gently and has a ring pulsing out under it. */
    int bob = 0;
    if (picked) {
        bob = ((model->tick / 8u) % 4u == 0u) ? -1 : 0;
        int pulse = (int)((model->tick / 2u) % 16u);
        DrawCircleLines(node.x, node.y + 2, radius - 1 + pulse / 2, FL_PAPER);
        DrawCircleLines(node.x, node.y + 2, radius - 2 + pulse / 2, FL_INK);
    }
    if (interlude) {
        draw_waystone(node, bob, open);
    } else {
        /* A paper plate under the token lifts it off busy ground. */
        DrawCircle(node.x, node.y + bob, 17, FL_PAPER);
        DrawCircleLines(node.x, node.y + bob, 18, FL_INK);
        draw_padded(fv_assets.props,
                    open ? fv_assets.nodes[FL_LEVELS[i].stage] : fv_assets.node_locked,
                    (float)(node.x - 20), (float)(node.y - 18 + bob), PAD_DRAW_SIZE);
    }
    if (picked) {
        /* A banner planted beside the token, the same cloth as the gate's. */
        int pole_x = node.x + radius - 3;
        int pole_top = node.y - radius - 17 + bob;
        DrawRectangle(pole_x - 1, pole_top, 3, 26, FL_PAPER);
        DrawRectangle(pole_x, pole_top + 1, 1, 24, FL_INK);
        int flag = (int)((model->tick / 5u) % CASTLE_FLAG_FRAMES);
        draw_padded(fv_assets.castle_fx, fv_assets.flag[flag],
                    (float)(pole_x - (int)CASTLE_FX_SIZE + 2),
                    (float)(pole_top - 2), CASTLE_FX_SIZE);
    }
    /* Little pips under a cleared stage: one per star earned. */
    for (int s = 0; s < model->level_stars[i] && s < 3; ++s) {
        int px = node.x - 8 + s * 7;
        int py = node.y + radius - 2;
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
    fill_plate(x, y, FL_MENU_CARD_W, FL_MENU_CARD_H, 8, FL_INK);
    outline_plate(x, y, FL_MENU_CARD_W, FL_MENU_CARD_H, 8, FL_PAPER);
    outline_plate(x + 3, y + 3, FL_MENU_CARD_W - 6, FL_MENU_CARD_H - 6, 6,
                  FL_PAPER);

    draw_padded(fv_assets.props, fv_assets.nodes[level->stage], (float)(x + 6),
                (float)(y + 10), PAD_DRAW_SIZE);
    /* Three lines: the stage number, its name, and its record or news. */
    bool zh = fl_text_language() == FL_LANG_ZH;
    int line1 = y + (zh ? 4 : 8);
    int line2 = y + (zh ? 18 : 21);
    int line3 = y + (zh ? 47 : 44);
    fl_text_draw(fl_trf(FL_TXT_UI_STAGE_N, (unsigned)i + 1), x + 52, line1,
                 FL_FONT_SMALL, FL_PAPER);
    fl_text_draw(level_name(i), x + 52, line2, FL_FONT_LARGE, FL_PAPER);
    int mid3 = line3 + fl_text_height(FL_FONT_SMALL) / 2;
    if (model->level_stars[i] > 0) {
        for (int s = 0; s < 3; ++s) {
            int px = x + 58 + s * 13;
            if (s < model->level_stars[i]) {
                draw_star(px, mid3, 5, true);
            } else {
                draw_star(px, mid3, 5, false);
            }
        }
        fl_text_draw(fl_trf(FL_TXT_UI_BEST_N, (unsigned long)model->level_best[i]),
                     x + 98, line3, FL_FONT_SMALL, FL_PAPER);
    } else {
        const char *waves = fl_trf(FL_TXT_UI_WAVES_N, (unsigned)level->wave_count);
        fl_text_draw(waves, x + 52, line3, FL_FONT_SMALL, FL_PAPER);
        /* A stage that brings a new tower says so, portrait and all. */
        fl_tower_kind_t fresh = fl_new_tower(i);
        if (fresh != FL_TOWER_NONE) {
            int nx = x + 52 + fl_text_width(waves, FL_FONT_SMALL) + 12;
            draw_padded(fv_assets.icons, fv_assets.cards[fresh], (float)(nx - 2),
                        (float)(mid3 - ICON_DRAW_SIZE / 2), (float)ICON_DRAW_SIZE);
            fl_text_draw(fl_trf(FL_TXT_UI_NEW_TOWER, tower_name(fresh)), nx + 26,
                         line3, FL_FONT_SMALL, FL_PAPER);
        }
    }
    draw_button(FL_MENU_GO_X, FL_MENU_GO_Y, FL_MENU_GO_W, FL_MENU_GO_H,
                fl_tr(FL_TXT_UI_BTN_FIGHT), true, true, model->tick);
}

/* An arrow at the screen's edge where more of the map lies, nudging out and
 * back so it reads as "drag this way". */
static void draw_scroll_hint(int x, int dir, uint32_t tick)
{
    int y = 214;
    int nudge = ((tick / 10u) % 4u == 0u) ? dir : 0;
    x += nudge;
    fill_plate(x - 9, y - 16, 18, 32, 3, FL_PAPER);
    outline_plate(x - 9, y - 16, 18, 32, 3, FL_INK);
    /* A chevron of stacked bars, its point towards dir. */
    for (int row = -6; row <= 6; ++row) {
        int reach = 6 - (row < 0 ? -row : row);
        int tip = x + dir * (reach - 3);
        DrawRectangle(dir > 0 ? tip - 2 : tip, y + row, 3, 1, FL_INK);
    }
}

static bool node_visible(const fl_state_t *model, uint8_t i, int margin)
{
    int x = FL_LEVELS[i].node.x - model->menu_scroll;
    return x > -margin && x < FL_VIEW_WIDTH + margin;
}

static void draw_menu(const fl_state_t *model)
{
    fl_terrain_draw_campaign(model->menu_scroll);

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
        if (!node_visible(model, i, 120) && !node_visible(model, i + 1, 120)) {
            continue;
        }
        bool walked = model->level_stars[i] > 0;
        bool frontier = walked && !(model->level_stars[i + 1] > 0);
        int march = frontier ? (int)(model->tick / 3u) : 0;
        draw_trail(node_on_screen(model, i), node_on_screen(model, i + 1),
                   node_clearance(i), node_clearance((uint8_t)(i + 1)),
                   walked ? 7 : 10, march, walked);
    }
    for (uint8_t i = 0; i < count; ++i) {
        if (node_visible(model, i, 40)) {
            draw_node(model, i);
        }
    }
    if (model->menu_scroll > 0) {
        draw_scroll_hint(14, -1, model->tick);
    }
    if (model->menu_scroll < FL_CAMPAIGN_WIDTH - FL_VIEW_WIDTH) {
        draw_scroll_hint(FL_VIEW_WIDTH - 14, 1, model->tick);
    }
    draw_menu_card(model);
    draw_button(FL_MENU_BACK_X, FL_MENU_BACK_Y, FL_MENU_BACK_W, FL_MENU_BACK_H,
                fl_tr(FL_TXT_UI_BTN_MENU), true, false, model->tick);
}

/* The battle: the map and its life, the actors and their effects shaken
 * together, then the controls and banners steady on top. */
static void draw_battle(const fl_state_t *model)
{
    fx_update(model);
    screens_update_battle(model);
    int dx, dy;
    fx_shake_offset(&dx, &dy);
    bool shaking = dx != 0 || dy != 0;
    if (shaking) {
        Camera2D camera = {0};
        camera.offset = (Vector2){(float)dx, (float)dy};
        camera.zoom = 1.0f;
        BeginMode2D(camera);
    }
    draw_background(model);
    scenery_draw_under(model);
    draw_world_motion(model);
    draw_build_pads(model);
    fx_draw_ground(model);
    draw_zones(model);
    draw_selection_reach(model);
    draw_actors(model);
    fx_draw_status(model);
    fx_draw_projectiles(model);
    draw_arcs(model);
    fx_draw_air(model);
    scenery_draw_over(model);
    if (shaking) {
        EndMode2D();
    }
    draw_ring(model);
    draw_hud(model);
    if (model->selected_tower >= 0) {
        draw_upgrade_panel(model);
    }
    screens_draw_battle(model);
    draw_pause(model);
    draw_overlay(model);
}

void frostline_view_draw(const fl_state_t *model)
{
    if (!model) {
        return;
    }
    fl_text_set_language(model->settings.lang);
    switch (model->phase) {
    case FL_PHASE_TITLE:
        fx_reset();
        screens_draw_title(model);
        return;
    case FL_PHASE_SETTINGS:
        fx_reset();
        screens_draw_settings(model);
        return;
    case FL_PHASE_ENDING:
        fx_reset();
        screens_draw_ending(model);
        return;
    case FL_PHASE_MENU:
        fx_reset();
        draw_menu(model);
        return;
    default:
        draw_battle(model);
        return;
    }
}

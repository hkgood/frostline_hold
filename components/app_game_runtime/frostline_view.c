/* Rendering for Frostline Hold.
 *
 * The view owns every drawing decision; the model owns every rule. All colour
 * is pure black or white so the panel keeps the 1-bit look, and all tone comes
 * from dithered textures baked at build time.
 *
 * SPDX-License-Identifier: Apache-2.0 */
#include "include/frostline_view.h"
#include "../app_game/data/frostline_level.h"

#include <stdio.h>
#include <string.h>

#include "raylib_lite_2d.h"
#include "raylib_lite_assets.h"
#include "raylib_lite_raylib.h"

/* The packer puts the source art centred in a cell that is four pixels
 * larger, so every sprite frame carries two pixels of padding per side. */
#define SPRITE_PAD 2

#define TOWER_DRAW_SIZE 48.0f
#define ENEMY_DRAW_SIZE 32.0f
#define PAD_DRAW_SIZE 24.0f
#define PROJECTILE_DRAW_SIZE 16.0f
#define ICON_DRAW_SIZE 16.0f

static const char *const TOWER_NAMES[9] = {
    "tower_guard_lv1", "tower_guard_lv2", "tower_guard_lv3",
    "tower_frost_lv1", "tower_frost_lv2", "tower_frost_lv3",
    "tower_siege_lv1", "tower_siege_lv2", "tower_siege_lv3",
};

static const char *const TOWER_LABELS[3] = {"GUARD", "FROST", "SIEGE"};
static const int TOWER_COSTS[3] = {70, 120, 160};

typedef struct {
    raylib_lite_atlas_t terrain;
    raylib_lite_atlas_t units;
    raylib_lite_atlas_t props;
    raylib_lite_atlas_t fx;
    raylib_lite_asset_id_t background;
    raylib_lite_asset_id_t towers[9];
    raylib_lite_asset_id_t wolf[4];
    raylib_lite_asset_id_t raider[4];
    raylib_lite_asset_id_t pads[2];
    raylib_lite_asset_id_t projectile;
    raylib_lite_asset_id_t icon_heart;
    raylib_lite_asset_id_t icon_coin;
    raylib_lite_asset_id_t icon_score;
} view_assets_t;

static view_assets_t s_assets;

void frostline_view_init(void)
{
    memset(&s_assets, 0, sizeof(s_assets));
    s_assets.terrain = raylib_lite_atlas_load("terrain.atlas");
    s_assets.units = raylib_lite_atlas_load("units.atlas");
    s_assets.props = raylib_lite_atlas_load("props.atlas");
    s_assets.fx = raylib_lite_atlas_load("fx.atlas");

    s_assets.background = raylib_lite_asset_id("map_background");
    for (int i = 0; i < 9; ++i) {
        s_assets.towers[i] = raylib_lite_asset_id(TOWER_NAMES[i]);
    }
    for (int i = 0; i < 4; ++i) {
        char name[32];
        snprintf(name, sizeof(name), "enemy_wolf_%d", i);
        s_assets.wolf[i] = raylib_lite_asset_id(name);
        snprintf(name, sizeof(name), "enemy_raider_%d", i);
        s_assets.raider[i] = raylib_lite_asset_id(name);
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
}

void frostline_view_shutdown(void)
{
    raylib_lite_atlas_unload(s_assets.terrain);
    raylib_lite_atlas_unload(s_assets.units);
    raylib_lite_atlas_unload(s_assets.props);
    raylib_lite_atlas_unload(s_assets.fx);
    memset(&s_assets, 0, sizeof(s_assets));
}

bool frostline_view_ready(void)
{
    return s_assets.terrain.texture.id != 0 && s_assets.units.texture.id != 0
           && s_assets.props.texture.id != 0 && s_assets.fx.texture.id != 0;
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
                   (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
}

static void draw_full(raylib_lite_atlas_t atlas, raylib_lite_asset_id_t id,
                      float x, float y, float size)
{
    const raylib_lite_sprite_frame_t *frame = raylib_lite_atlas_get_frame(atlas, id);
    if (!frame) {
        return;
    }
    DrawTexturePro(atlas.texture, frame->source, (Rectangle){x, y, size, size},
                   (Vector2){0.0f, 0.0f}, 0.0f, WHITE);
}

static void draw_text_centered(const char *text, int center_x, int y, int size,
                               Color color)
{
    int width = MeasureText(text, size);
    DrawText(text, center_x - width / 2, y, size, color);
}

static void draw_panel(int x, int y, int w, int h, bool inverted)
{
    DrawRectangle(x, y, w, h, inverted ? WHITE : BLACK);
    DrawRectangleLines(x, y, w, h, inverted ? BLACK : WHITE);
}

/* ------------------------------------------------------------------ */

static void draw_background(void)
{
    draw_full(s_assets.terrain, s_assets.background, 0.0f, 0.0f, 480.0f);
}

static void draw_build_pads(const fl_state_t *model)
{
    if (model->selected_kind == FL_TOWER_NONE) {
        return;
    }
    int frame = (int)((model->tick / 12u) % 2u);
    for (int i = 0; i < FL_BUILD_SPOT_COUNT; ++i) {
        bool occupied = false;
        for (int t = 0; t < FL_MAX_TOWERS; ++t) {
            if (model->towers[t].active
                    && model->towers[t].x == FL_BUILD_SPOTS[i].x
                    && model->towers[t].y == FL_BUILD_SPOTS[i].y) {
                occupied = true;
                break;
            }
        }
        if (occupied) {
            continue;
        }
        draw_padded(s_assets.props, s_assets.pads[frame],
                    (float)(FL_BUILD_SPOTS[i].x - PAD_DRAW_SIZE / 2),
                    (float)(FL_BUILD_SPOTS[i].y - PAD_DRAW_SIZE / 2),
                    PAD_DRAW_SIZE);
    }
}

static void draw_towers(const fl_state_t *model)
{
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        const fl_tower_t *tower = &model->towers[i];
        if (!tower->active) {
            continue;
        }
        int index = (int)tower->kind * 3 + (int)tower->level - 1;
        if (index < 0 || index > 8) {
            continue;
        }
        float x = (float)tower->x - TOWER_DRAW_SIZE / 2.0f;
        float y = (float)tower->y - TOWER_DRAW_SIZE + 8.0f;
        bool selected = model->selected_tower == (int8_t)i;
        draw_padded(s_assets.units, s_assets.towers[index], x, y, TOWER_DRAW_SIZE);

        /* Level pips under the tower. */
        for (int level = 0; level < 3; ++level) {
            int px = tower->x - 9 + level * 7;
            bool filled = level < tower->level;
            DrawRectangle(px, tower->y + 2, 5, 4, filled ? BLACK : WHITE);
            DrawRectangleLines(px, tower->y + 2, 5, 4, BLACK);
        }
        if (selected) {
            DrawRectangleLines((int)x - 2, (int)y - 2, (int)TOWER_DRAW_SIZE + 4,
                               (int)TOWER_DRAW_SIZE + 4, WHITE);
        }
    }
}

static void draw_enemies(const fl_state_t *model)
{
    int frame = (int)((model->tick / 5u) % 4u);
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &model->enemies[i];
        if (!enemy->alive) {
            continue;
        }
        raylib_lite_asset_id_t id = (enemy->kind == FL_ENEMY_RAIDER)
            ? s_assets.raider[frame] : s_assets.wolf[frame];
        float x = (float)enemy->x - ENEMY_DRAW_SIZE / 2.0f;
        float y = (float)enemy->y - ENEMY_DRAW_SIZE + 6.0f;
        draw_padded(s_assets.props, id, x, y, ENEMY_DRAW_SIZE);

        /* Health bar: black trough, white fill. */
        float ratio = enemy->max_hp > 0.0f ? enemy->hp / enemy->max_hp : 0.0f;
        int bar_width = (int)(ratio * 18.0f);
        int bar_x = (int)enemy->x - 9;
        int bar_y = (int)y - 5;
        DrawRectangle(bar_x, bar_y, 18, 3, WHITE);
        DrawRectangle(bar_x, bar_y, bar_width, 3, BLACK);
        if (enemy->slow_ticks > 0) {
            DrawRectangle(bar_x, bar_y + 4, 18, 1, BLACK);
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
        draw_padded(s_assets.fx, s_assets.projectile,
                    (float)shot->x - PROJECTILE_DRAW_SIZE / 2.0f,
                    (float)shot->y - PROJECTILE_DRAW_SIZE / 2.0f,
                    PROJECTILE_DRAW_SIZE);
    }
}

static void draw_hud(const fl_state_t *model)
{
    DrawRectangle(0, 0, FL_VIEW_WIDTH, FL_HUD_HEIGHT, BLACK);
    DrawRectangle(0, FL_HUD_HEIGHT - 2, FL_VIEW_WIDTH, 2, WHITE);

    /* The icon art is black, so it needs a white plate to read on the bar. */
    DrawRectangle(8, 12, 20, 20, WHITE);
    draw_padded(s_assets.fx, s_assets.icon_heart, 10.0f, 14.0f, ICON_DRAW_SIZE);
    DrawText(TextFormat("%u", (unsigned)model->lives), 32, 10, 16, WHITE);
    DrawRectangle(68, 12, 20, 20, WHITE);
    draw_padded(s_assets.fx, s_assets.icon_coin, 70.0f, 14.0f, ICON_DRAW_SIZE);
    DrawText(TextFormat("%u", (unsigned)model->gold), 92, 10, 16, WHITE);

    /* Points read as a third resource so the run has a running tally. */
    DrawRectangle(140, 12, 20, 20, WHITE);
    draw_padded(s_assets.fx, s_assets.icon_score, 142.0f, 14.0f, ICON_DRAW_SIZE);
    DrawText(TextFormat("%lu", (unsigned long)model->score), 164, 10, 16, WHITE);

    DrawText("WAVE", 228, 6, 12, WHITE);
    DrawText(TextFormat("%02u/%02d", (unsigned)model->wave, FL_WAVE_COUNT),
             228, 22, 16, WHITE);

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
                       FL_SPEED_BTN_X + FL_SPEED_BTN_W / 2, FL_BTN_Y + 7, 16, WHITE);
}

static void draw_build_bar(const fl_state_t *model)
{
    DrawRectangle(0, FL_BAR_TOP, FL_VIEW_WIDTH, FL_BAR_HEIGHT, BLACK);
    DrawRectangle(0, FL_BAR_TOP, FL_VIEW_WIDTH, 2, WHITE);

    if (model->selected_tower >= 0) {
        const fl_tower_t *tower = &model->towers[model->selected_tower];
        if (!tower->active) {
            return;
        }
        draw_panel(6, FL_BAR_TOP + 6, FL_VIEW_WIDTH - 12, FL_TOWER_CARD_H, false);
        DrawText(TOWER_LABELS[tower->kind - 1], 18, FL_BAR_TOP + 12, 16, WHITE);
        DrawText(TextFormat("LV %u", (unsigned)tower->level), 18, FL_BAR_TOP + 32, 14, WHITE);
        DrawText(TextFormat("SELL %u", (unsigned)(tower->invested / 2)),
                 120, FL_BAR_TOP + 22, 14, WHITE);
        DrawText("TAP ELSEWHERE TO CLOSE", 210, FL_BAR_TOP + 22, 12, WHITE);
        return;
    }

    for (int i = 0; i < FL_TOWER_CARD_COUNT; ++i) {
        int x = 8 + i * FL_TOWER_CARD_GAP;
        bool armed = model->selected_kind == (uint8_t)(FL_TOWER_GUARD + i);
        /* Cards are white plates; the armed one flips to black. */
        draw_panel(x, FL_TOWER_CARD_Y, FL_TOWER_CARD_W, FL_TOWER_CARD_H, !armed);
        int sprite = i * 3;
        float size = 38.0f;
        draw_padded(s_assets.units, s_assets.towers[sprite],
                    (float)(x + (FL_TOWER_CARD_W - (int)size) / 2),
                    (float)(FL_TOWER_CARD_Y + 2), size);
        DrawText(TextFormat("%d", TOWER_COSTS[i]), x + 6, FL_TOWER_CARD_Y + 42, 14,
                 armed ? WHITE : BLACK);
    }
}

static void draw_overlay(const fl_state_t *model)
{
    if (model->phase != FL_PHASE_WON && model->phase != FL_PHASE_LOST) {
        return;
    }
    DrawRectangle(0, 0, FL_VIEW_WIDTH, FL_VIEW_HEIGHT, BLACK);
    bool won = model->phase == FL_PHASE_WON;
    const char *title = won ? "HOLD SECURED" : "THE GATE FALLS";
    draw_text_centered(title, FL_VIEW_WIDTH / 2, 120, 28, WHITE);

    /* The points total, with the breakdown underneath it. */
    draw_text_centered("SCORE", FL_VIEW_WIDTH / 2, 172, 14, WHITE);
    draw_text_centered(TextFormat("%lu", (unsigned long)model->score),
                       FL_VIEW_WIDTH / 2, 192, 32, WHITE);

    draw_text_centered(TextFormat("KILLS %u      WAVE BONUS %lu",
                                  (unsigned)model->kills,
                                  (unsigned long)model->score_waves),
                       FL_VIEW_WIDTH / 2, 244, 14, WHITE);
    draw_text_centered(TextFormat("LIVES %u      BONUS %lu",
                                  (unsigned)model->lives,
                                  (unsigned long)model->score_bonus),
                       FL_VIEW_WIDTH / 2, 266, 14, WHITE);
    /* The wave counter steps past the last wave once the run is won. */
    unsigned shown_wave = (unsigned)model->wave;
    if (shown_wave > (unsigned)FL_WAVE_COUNT) {
        shown_wave = (unsigned)FL_WAVE_COUNT;
    }
    draw_text_centered(TextFormat("WAVE %02u/%02d", shown_wave, FL_WAVE_COUNT),
                       FL_VIEW_WIDTH / 2, 296, 14, WHITE);
    draw_text_centered("TAP TO FIGHT AGAIN", FL_VIEW_WIDTH / 2, 340, 14, WHITE);
}

void frostline_view_draw(const fl_state_t *model)
{
    if (!model) {
        return;
    }
    draw_background();
    draw_build_pads(model);
    draw_towers(model);
    draw_enemies(model);
    draw_projectiles(model);
    draw_hud(model);
    draw_build_bar(model);
    draw_overlay(model);
}

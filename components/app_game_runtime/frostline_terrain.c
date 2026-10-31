/* The ground under everything: a level's battlefield or the campaign scroll.
 *
 * The terrain ships as one bit per pixel (data/frostline_terrain_data.h).
 * Only one map is ever on screen, so a single RGB565 buffer holds it: the
 * map is expanded into it when the screen asks for a different one, and
 * registered with the renderer as an ordinary texture. The buffer is sized
 * for the widest map, the campaign scroll.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "private/frostline_view_internal.h"

#include <stdlib.h>

#include "raylib_lite_2d.h"
#if RAYLIB_LITE_GAME_NATIVE
#include "esp_heap_caps.h"
#endif

#include "data/frostline_terrain_data.h"

_Static_assert(FL_TERRAIN_MAP_COUNT == FL_LEVEL_COUNT,
               "regenerate the terrain: python3 assets_src/artgen/generate_terrain.py");
_Static_assert(FL_TERRAIN_CAMPAIGN_W == FL_CAMPAIGN_WIDTH,
               "regenerate the terrain: python3 assets_src/artgen/generate_terrain.py");

/* Which map the buffer holds: a level index, or the campaign. */
#define SHOWING_NONE (-2)
#define SHOWING_CAMPAIGN (-1)

static struct {
    uint16_t *pixels;
    Texture2D texture;
    int showing;
} s_terrain = {.showing = SHOWING_NONE};

static uint16_t to_rgb565(Color c)
{
    return (uint16_t)(((c.r & 0xF8u) << 8) | ((c.g & 0xFCu) << 3) | (c.b >> 3));
}

bool fl_terrain_init(void)
{
    const size_t bytes = (size_t)FL_TERRAIN_CAMPAIGN_W * FL_TERRAIN_SIZE * sizeof(uint16_t);
#if RAYLIB_LITE_GAME_NATIVE
    s_terrain.pixels = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
#else
    s_terrain.pixels = malloc(bytes);
#endif
    s_terrain.showing = SHOWING_NONE;
    return s_terrain.pixels != NULL;
}

void fl_terrain_shutdown(void)
{
    if (s_terrain.texture.id) {
        raylib_lite_2d_unload_texture(s_terrain.texture);
    }
    free(s_terrain.pixels);
    s_terrain.pixels = NULL;
    s_terrain.texture = (Texture2D){0};
    s_terrain.showing = SHOWING_NONE;
}

/* Expand a 1-bit map into the buffer and hand it to the renderer. */
static void show(int which, const uint8_t *bits, int width)
{
    if (s_terrain.showing == which || !s_terrain.pixels) {
        return;
    }
    const uint16_t ink = to_rgb565(FL_INK);
    const uint16_t paper = to_rgb565(FL_PAPER);
    const size_t count = (size_t)width * FL_TERRAIN_SIZE;
    uint16_t *out = s_terrain.pixels;
    for (size_t i = 0; i < count / 8; ++i) {
        const uint8_t byte = bits[i];
        for (int bit = 0; bit < 8; ++bit) {
            *out++ = (byte & (0x80u >> bit)) ? ink : paper;
        }
    }
    /* The texture records its size, so a map of another width needs a new
     * registration; it costs a slot lookup, not a copy. */
    if (s_terrain.texture.id) {
        raylib_lite_2d_unload_texture(s_terrain.texture);
    }
    s_terrain.texture = raylib_lite_2d_register_rgb565(s_terrain.pixels, width, FL_TERRAIN_SIZE);
    s_terrain.showing = s_terrain.texture.id ? which : SHOWING_NONE;
}

void fl_terrain_draw_level(int level)
{
    if (level < 0 || level >= FL_TERRAIN_MAP_COUNT) {
        return;
    }
    show(level, FL_TERRAIN_MAPS[level], FL_TERRAIN_SIZE);
    if (s_terrain.texture.id) {
        DrawTexture(s_terrain.texture, 0, 0, TINT_NONE);
    }
}

void fl_terrain_draw_campaign(int scroll)
{
    show(SHOWING_CAMPAIGN, FL_TERRAIN_CAMPAIGN, FL_TERRAIN_CAMPAIGN_W);
    if (!s_terrain.texture.id) {
        return;
    }
    if (scroll < 0) {
        scroll = 0;
    } else if (scroll > FL_TERRAIN_CAMPAIGN_W - FL_TERRAIN_SIZE) {
        scroll = FL_TERRAIN_CAMPAIGN_W - FL_TERRAIN_SIZE;
    }
    const Rectangle source = {(float)scroll, 0.0f, (float)FL_TERRAIN_SIZE, (float)FL_TERRAIN_SIZE};
    DrawTextureRec(s_terrain.texture, source, (Vector2){0.0f, 0.0f}, TINT_NONE);
}

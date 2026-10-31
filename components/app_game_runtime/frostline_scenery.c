/* The living details of each map: the weather, the wildlife, the light.
 *
 * A map is a still engraving; this module keeps it breathing. Every level
 * picks from a set of small effects - snow in two depths, gusts, birds,
 * snow slipping off pines, glints on ice, mist, drips, reeds, lamps,
 * smoke, a waterfall, an aurora - and places them where its plate shows
 * the trees, the lake or the cliffs. Nothing here touches the rules, and
 * everything is drawn from the tick alone, so there is no state to keep.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "private/frostline_view_internal.h"

#include <math.h>
#include <string.h>

typedef struct {
    int16_t x, y;
} spot_t;

typedef struct {
    int16_t x, y, w, h;
} box_t;

#define MAX_SPOTS 12

typedef struct {
    const char *key;          /* the level it dresses */
    uint8_t snow;             /* flakes in the near layer */
    uint8_t wind;             /* 0 calm .. 3 blizzard: drift and gusts */
    bool birds;
    bool eagle;
    bool aurora;              /* only on maps with open sky at the top */
    /* Pines that shed clumps of snow. */
    spot_t pines[MAX_SPOTS];
    uint8_t pine_count;
    /* Ice that glints: an ellipse of lake or glacier. */
    box_t ice;
    /* Mist drifting over a low band. */
    box_t mist;
    /* Ledges that drip. */
    spot_t drips[MAX_SPOTS];
    uint8_t drip_count;
    /* Reeds that sway at a shore. */
    spot_t reeds[MAX_SPOTS];
    uint8_t reed_count;
    /* Lamps and torches that flicker. */
    spot_t lamps[MAX_SPOTS];
    uint8_t lamp_count;
    /* Chimneys or vents that smoke. */
    spot_t smoke[4];
    uint8_t smoke_count;
    /* A frozen fall with water still running inside it. */
    box_t fall;
    /* Rune motes that hover, for the ruins. */
    box_t motes;
    /* The path a hare runs now and then: from, to. */
    spot_t hare[2];
} scenery_t;

static const scenery_t SCENERY[] = {
    {
        .key = "pass", .snow = 26, .wind = 1, .birds = true,
        .pines = {{36, 64}, {104, 40}, {176, 72}, {246, 112}, {58, 206},
                  {28, 300}, {92, 352}, {140, 420}, {204, 96}, {290, 60}},
        .pine_count = 10,
        .hare = {{232, 438}, {336, 412}},
    },
    {
        .key = "mere", .snow = 20, .wind = 1, .birds = true,
        .pines = {{20, 200}, {440, 40}, {452, 160}, {60, 420}, {170, 444},
                  {120, 24}, {300, 26}, {454, 250}},
        .pine_count = 8,
        .ice = {232, 214, 132, 62},
        .mist = {96, 196, 272, 50},
        .reeds = {{238, 132}, {206, 294}, {300, 282}, {108, 244}, {380, 200},
                  {88, 172}},
        .reed_count = 6,
    },
    {
        .key = "ridge", .snow = 30, .wind = 2, .eagle = true,
        .pines = {{20, 210}, {460, 190}, {420, 270}, {150, 402}, {240, 402}},
        .pine_count = 5,
        .drips = {{96, 196}, {162, 192}, {248, 266}, {306, 270}, {184, 342},
                  {262, 346}, {336, 204}, {120, 268}, {228, 196}},
        .drip_count = 9,
    },
    {
        /* The pit is sheltered: little snow, no wind, lanterns at the
         * tunnel mouth and dust hanging in its draught. */
        .key = "mine", .snow = 10, .wind = 0,
        .lamps = {{180, 62}, {287, 64}, {42, 204}},
        .lamp_count = 3,
        .motes = {200, 48, 90, 50},
    },
    {
        /* Open ice: the hardest wind, glints on the upper tarn and cold
         * vapour off the lower one. */
        .key = "glacier", .snow = 36, .wind = 3,
        .ice = {240, 136, 96, 28},
        .mist = {150, 188, 190, 56},
    },
    {
        .key = "ruins", .snow = 18, .wind = 1, .birds = true,
        .pines = {{24, 96}, {70, 92}, {120, 88}, {170, 90}, {220, 86},
                  {270, 82}, {320, 64}},
        .pine_count = 7,
        .lamps = {{399, 66}},
        .lamp_count = 1,
        .motes = {176, 284, 86, 70},
    },
    {
        .key = "falls", .snow = 20, .wind = 1, .eagle = true,
        .fall = {206, 48, 74, 168},
        .mist = {136, 236, 200, 60},
        .drips = {{70, 94}, {155, 89}, {291, 94}, {328, 94}, {385, 108},
                  {47, 197}, {422, 169}},
        .drip_count = 7,
        .pines = {{84, 22}, {403, 28}, {441, 46}},
        .pine_count = 3,
    },
    {
        /* Torches at the citadel's two gates, frost sparkling round the
         * throne, and the aurora the king's power lights. */
        .key = "citadel", .snow = 32, .wind = 2, .aurora = true,
        .lamps = {{186, 92}, {218, 92}, {259, 92}, {291, 92}},
        .lamp_count = 4,
        .motes = {196, 196, 90, 74},
    },
};

#define SCENERY_COUNT (sizeof(SCENERY) / sizeof(SCENERY[0]))

static const scenery_t *scenery_for(const fl_state_t *model)
{
    const char *key = current_level(model)->key;
    for (size_t i = 0; i < SCENERY_COUNT; ++i) {
        if (strcmp(SCENERY[i].key, key) == 0) {
            return &SCENERY[i];
        }
    }
    return &SCENERY[0];
}

static uint32_t hash2(uint32_t a, uint32_t b)
{
    return jag_hash(a * 0x9E3779B1u ^ (b + 0x7F4A7C15u));
}

/* ------------------------------------------------------------------------
 * Weather
 * --------------------------------------------------------------------- */

/* Snow in two depths: far flakes are single slow pixels, near flakes are
 * small crosses that fall faster and sway, and the wind leans them all. */
static void draw_snow(const scenery_t *s, uint32_t tick)
{
    float lean = (float)s->wind * 0.6f;
    int far = s->snow + s->snow / 2;
    for (int i = 0; i < far; ++i) {
        uint32_t h = hash2((uint32_t)i, 17u);
        uint32_t fall = 1u + (h >> 20) % 2u;
        int x = (int)((h % 480u) + (uint32_t)((float)tick * lean * 0.5f)) % 480;
        int y = (int)((h >> 9) % 480u + tick * fall / 2u) % 480;
        if (corner_cut(x, y)) {
            continue;
        }
        DrawRectangle(x, y, 1, 1, ((h >> 4) & 1u) ? FL_PAPER : FL_INK);
    }
    for (int i = 0; i < s->snow; ++i) {
        uint32_t h = hash2((uint32_t)i, 91u);
        uint32_t fall = 2u + (h >> 22) % 2u;
        float sway = sinf((float)tick * 0.06f + (float)(h % 628u) * 0.01f) * 3.0f;
        int x = (int)((float)(h % 480u) + (float)tick * lean + sway) % 480;
        if (x < 0) {
            x += 480;
        }
        int y = (int)((h >> 9) % 480u + tick * fall) % 480;
        if (corner_cut(x, y)) {
            continue;
        }
        /* An ink rim round a paper heart reads on snow and on forest. */
        DrawRectangle(x - 1, y, 3, 1, FL_INK);
        DrawRectangle(x, y - 1, 1, 3, FL_INK);
        DrawRectangle(x, y, 1, 1, FL_PAPER);
        if (((h >> 3) & 3u) == 0u) {
            DrawRectangle(x - 1, y, 1, 1, FL_PAPER);
        }
    }
}

/* Gusts: every few seconds a band of streaks races across the map. */
static void draw_gusts(const scenery_t *s, uint32_t tick)
{
    if (s->wind == 0) {
        return;
    }
    uint32_t period = s->wind >= 3 ? 70u : (s->wind == 2 ? 110u : 170u);
    uint32_t cycle = tick / period;
    uint32_t t = tick % period;
    const uint32_t span = 34u;
    if (t >= span) {
        return;
    }
    uint32_t h = hash2(cycle, 5u);
    int band = 70 + (int)(h % 300u);
    int streaks = 4 + (int)s->wind * 3;
    for (int k = 0; k < streaks; ++k) {
        uint32_t g = hash2(cycle, (uint32_t)k + 40u);
        int y = band + (int)(g % 40u) - 20;
        int len = 10 + (int)((g >> 8) % 22u);
        int x = -40 + (int)(t * (14u + (g >> 16) % 6u)) + (int)((g >> 4) % 60u);
        if (x > 480) {
            continue;
        }
        DrawRectangle(x, y, len, 1, FL_PAPER);
        DrawRectangle(x + len / 3, y + 1, len / 2, 1, FL_INK);
    }
}

/* A flock crosses the sky now and then, wings beating out of step. */
void scenery_draw_birds(uint32_t tick)
{
    const uint32_t period = 520u;
    uint32_t cycle = tick / period;
    uint32_t t = tick % period;
    if (t > 200u) {
        return;
    }
    uint32_t h = hash2(cycle, 3u);
    bool rightwards = h & 1u;
    int base_y = 60 + (int)((h >> 4) % 120u);
    for (int k = 0; k < 5; ++k) {
        int offset_x = k * 14 - (k % 2) * 6;
        int offset_y = (k % 2) * 8 + k * 2;
        int x = rightwards ? -30 + (int)(t * 3u) - offset_x
                           : 510 - (int)(t * 3u) + offset_x;
        int y = base_y + offset_y + (int)(sinf((float)(t + (unsigned)k * 7u) * 0.08f) * 3.0f);
        if (x < 0 || x >= 480 || corner_cut(x, y)) {
            continue;
        }
        bool up = ((t / 4u) + (unsigned)k) % 2u == 0u;
        int wing = up ? -2 : 1;
        DrawLine(x - 3, y + wing, x, y, FL_INK);
        DrawLine(x, y, x + 3, y + wing, FL_INK);
        DrawRectangle(x, y, 1, 1, FL_INK);
    }
}

/* An eagle wheels slowly high over the ridge, its shadow far below. */
static void draw_eagle(uint32_t tick)
{
    float a = (float)tick * 0.012f;
    int x = 240 + (int)(cosf(a) * 150.0f);
    int y = 120 + (int)(sinf(a) * 46.0f);
    float dx = cosf(a + 1.5707963f);
    /* Glides most of the time, beats its wings once in a while. */
    bool beat = (tick / 5u) % 12u < 2u;
    int span = 7;
    int lift = beat ? -3 : -1;
    DrawLine(x - span, y + lift, x - 2, y, FL_INK);
    DrawLine(x + span, y + lift, x + 2, y, FL_INK);
    DrawLine(x - span + 1, y + lift - 1, x - 3, y - 1, FL_INK);
    DrawLine(x + span - 1, y + lift - 1, x + 3, y - 1, FL_INK);
    DrawRectangle(x - 2, y - 1, 5, 2, FL_INK);
    DrawRectangle(x + (int)lroundf(dx * 3.0f), y, 1, 1, FL_PAPER);
}

/* ------------------------------------------------------------------------
 * Ground life
 * --------------------------------------------------------------------- */

/* A pine lets go of its load: a clump drops and bursts into powder. */
static void draw_pine_snow(const scenery_t *s, uint32_t tick)
{
    for (int i = 0; i < s->pine_count; ++i) {
        uint32_t period = 150u + (uint32_t)(hash2((uint32_t)i, 9u) % 140u);
        uint32_t t = (tick + (uint32_t)i * 37u) % period;
        if (t > 22u) {
            continue;
        }
        int x = s->pines[i].x;
        int y = s->pines[i].y;
        if (t < 10u) {
            int fy = y + (int)(t * t / 6u);
            DrawRectangle(x - 1, fy - 1, 3, 3, FL_INK);
            DrawRectangle(x, fy, 1, 1, FL_PAPER);
        } else {
            int spread = (int)(t - 10u);
            int py = y + 16 + spread / 3;
            for (int k = 0; k < 5; ++k) {
                int px = x + (k - 2) * (spread / 2 + 1);
                if ((k + (int)t) % 3 != 0) {
                    DrawRectangle(px, py - (k % 2), 1, 1, FL_PAPER);
                }
            }
        }
    }
}

/* Light catches the ice in quick four-point glints. */
static void draw_ice_glints(const scenery_t *s, uint32_t tick)
{
    if (s->ice.w <= 0) {
        return;
    }
    for (int k = 0; k < 5; ++k) {
        uint32_t cycle = (tick + (uint32_t)k * 13u) / 24u;
        uint32_t t = (tick + (uint32_t)k * 13u) % 24u;
        uint32_t h = hash2(cycle, (uint32_t)k + 70u);
        float a = (float)(h % 628u) * 0.01f;
        float r = (float)((h >> 10) % 100u) / 100.0f;
        int x = s->ice.x + (int)(cosf(a) * (float)s->ice.w * r);
        int y = s->ice.y + (int)(sinf(a) * (float)s->ice.h * r);
        int size = t < 4u ? (int)t : (t < 10u ? 4 - (int)(t - 4u) / 2 : 0);
        if (size <= 0) {
            continue;
        }
        DrawRectangle(x - size, y, size * 2 + 1, 1, FL_PAPER);
        DrawRectangle(x, y - size, 1, size * 2 + 1, FL_PAPER);
        DrawRectangle(x - 1, y - 1, 1, 1, FL_INK);
        DrawRectangle(x + 1, y + 1, 1, 1, FL_INK);
    }
}

/* Mist: thin broken strands of paper drifting along a band. */
static void draw_mist(const scenery_t *s, uint32_t tick)
{
    if (s->mist.w <= 0) {
        return;
    }
    for (int k = 0; k < 9; ++k) {
        uint32_t h = hash2((uint32_t)k, 33u);
        int y = s->mist.y + (int)(h % (uint32_t)s->mist.h);
        int len = 18 + (int)((h >> 8) % 40u);
        int drift = (int)((tick / (3u + (h >> 20) % 3u) + (h >> 4)) % (uint32_t)(s->mist.w + len));
        int x = s->mist.x - len + drift;
        /* Broken strands: dash, gap, dash. */
        for (int dx = 0; dx < len; dx += 5) {
            int px = x + dx;
            if (px < s->mist.x || px > s->mist.x + s->mist.w) {
                continue;
            }
            if (((unsigned)dx / 5u + h) % 3u != 0u) {
                DrawRectangle(px, y, 3, 1, FL_PAPER);
            }
        }
    }
}

/* Meltwater drips from the ice on the ledges and splashes below. */
static void draw_drips(const scenery_t *s, uint32_t tick)
{
    for (int i = 0; i < s->drip_count; ++i) {
        uint32_t period = 60u + (uint32_t)(hash2((uint32_t)i, 12u) % 50u);
        uint32_t t = (tick + (uint32_t)i * 19u) % period;
        int x = s->drips[i].x;
        int y = s->drips[i].y;
        if (t < 18u) {
            /* The bead swells at the icicle's tip. */
            if (t > 8u) {
                DrawRectangle(x, y, 1, 2, FL_PAPER);
            }
        } else if (t < 26u) {
            int fy = y + (int)((t - 18u) * (t - 18u) / 2u);
            DrawRectangle(x - 1, fy - 1, 3, 3, FL_INK);
            DrawRectangle(x, fy, 1, 1, FL_PAPER);
        } else if (t < 32u) {
            int fy = y + 32;
            int r = (int)(t - 26u);
            DrawRectangle(x - r, fy, 1, 1, FL_PAPER);
            DrawRectangle(x + r, fy, 1, 1, FL_PAPER);
            DrawRectangle(x - r / 2, fy - 1, 1, 1, FL_PAPER);
            DrawRectangle(x + r / 2, fy - 1, 1, 1, FL_PAPER);
        }
    }
}

/* Reeds lean with the wind, each on its own phase. */
static void draw_reeds(const scenery_t *s, uint32_t tick)
{
    for (int i = 0; i < s->reed_count; ++i) {
        int x = s->reeds[i].x;
        int y = s->reeds[i].y;
        for (int k = 0; k < 3; ++k) {
            float sway = sinf((float)tick * 0.07f + (float)(i * 3 + k)) * 2.0f;
            int h = 7 + k * 2;
            int tipx = x + k * 2 - 2 + (int)lroundf(sway);
            DrawLine(x + k * 2 - 2, y, tipx, y - h, FL_INK);
            DrawRectangle(tipx, y - h - 1, 1, 2, FL_INK);
        }
    }
}

/* A lamp: a flame that flickers in a soft halo of dither. */
static void draw_lamps(const scenery_t *s, uint32_t tick)
{
    for (int i = 0; i < s->lamp_count; ++i) {
        int x = s->lamps[i].x;
        int y = s->lamps[i].y;
        uint32_t h = hash2(tick / 3u, (uint32_t)i);
        int glow = 7 + (int)(h % 3u);
        draw_dither_disc(x, y, glow, 0.32f, FL_PAPER);
        int frame = (int)((tick / 3u + (unsigned)i) % CASTLE_FLAME_FRAMES);
        draw_padded(fv_assets.castle_fx, fv_assets.flame[frame], (float)(x - 14),
                    (float)(y - 18), CASTLE_FX_SIZE);
    }
}

/* Smoke rising from chimneys and vents, leaning downwind. */
static void draw_smoke(const scenery_t *s, uint32_t tick)
{
    for (int i = 0; i < s->smoke_count; ++i) {
        for (int k = 0; k < 6; ++k) {
            uint32_t t = (tick + (uint32_t)k * 12u + (uint32_t)i * 7u) % 72u;
            int x = s->smoke[i].x + (int)(t / 5u) * (int)(s->wind + 1) / 2
                    + (int)(sinf((float)(t + (unsigned)k) * 0.15f) * 2.0f);
            int y = s->smoke[i].y - (int)(t / 2u);
            int r = 2 + (int)(t / 14u);
            float fade = 1.0f - (float)t / 72.0f;
            draw_dither_disc(x, y, r, 0.55f * fade, FL_INK);
            if (t < 30u) {
                draw_dither_disc(x - 1, y - 1, r / 2, 0.5f * fade, FL_PAPER);
            }
        }
    }
}

/* A frozen fall: water still runs in streaks behind the ice, and spray
 * hangs at the foot. */
static void draw_fall(const scenery_t *s, uint32_t tick)
{
    if (s->fall.w <= 0) {
        return;
    }
    const box_t *f = &s->fall;
    for (int col = 0; col < f->w; col += 3) {
        uint32_t h = hash2((uint32_t)col, 61u);
        int speed = 3 + (int)(h % 3u);
        int len = 6 + (int)((h >> 6) % 10u);
        for (int start = -(int)((tick * (uint32_t)speed + h) % 24u); start < f->h;
             start += 24) {
            int y0 = f->y + start;
            int y1 = y0 + len;
            if (y1 < f->y || y0 > f->y + f->h) {
                continue;
            }
            if (y0 < f->y) {
                y0 = f->y;
            }
            if (y1 > f->y + f->h) {
                y1 = f->y + f->h;
            }
            DrawRectangle(f->x + col, y0, 1, y1 - y0, FL_PAPER);
        }
    }
    /* Spray at the foot. */
    for (int k = 0; k < 10; ++k) {
        uint32_t t = (tick + (uint32_t)k * 5u) % 30u;
        uint32_t h = hash2((uint32_t)k, tick / 30u);
        int x = f->x + (int)(h % (uint32_t)(f->w + 1));
        int y = f->y + f->h - (int)(t / 2u);
        draw_dither_disc(x, y, 2 + (int)(t / 8u), 0.5f * (1.0f - (float)t / 30.0f),
                         FL_PAPER);
    }
}

/* Motes of old magic hovering over the ruins, pulsing in turn. */
static void draw_motes(const scenery_t *s, uint32_t tick)
{
    if (s->motes.w <= 0) {
        return;
    }
    for (int k = 0; k < 10; ++k) {
        uint32_t h = hash2((uint32_t)k, 77u);
        float phase = (float)tick * 0.03f + (float)(h % 628u) * 0.01f;
        int x = s->motes.x + (int)(h % (uint32_t)s->motes.w)
                + (int)(sinf(phase) * 6.0f);
        int y = s->motes.y + (int)((h >> 9) % (uint32_t)s->motes.h)
                + (int)(cosf(phase * 0.7f) * 5.0f);
        bool bright = ((tick / 6u) + (uint32_t)k) % 5u == 0u;
        DrawRectangle(x - 1, y - 1, 3, 3, FL_INK);
        DrawRectangle(x, y, 1, 1, FL_PAPER);
        if (bright) {
            DrawRectangle(x - 2, y, 5, 1, FL_PAPER);
            DrawRectangle(x, y - 2, 1, 5, FL_PAPER);
        }
    }
}

/* Every so often a hare bolts across open ground in long bounds. */
static void draw_hare(const scenery_t *s, uint32_t tick)
{
    if (s->hare[0].x == 0 && s->hare[1].x == 0) {
        return;
    }
    const uint32_t period = 640u;
    const uint32_t run = 90u;
    uint32_t t = tick % period;
    if (t >= run) {
        return;
    }
    float u = (float)t / (float)run;
    int x = s->hare[0].x + (int)((float)(s->hare[1].x - s->hare[0].x) * u);
    int y = s->hare[0].y + (int)((float)(s->hare[1].y - s->hare[0].y) * u);
    /* A bound every eight ticks. */
    int hop = (int)(fabsf(sinf((float)t * 0.39f)) * 5.0f);
    bool facing_right = s->hare[1].x > s->hare[0].x;
    int dir = facing_right ? 1 : -1;
    int by = y - hop;
    DrawRectangle(x - 3, by - 2, 6, 3, FL_INK);           /* body */
    DrawRectangle(x + dir * 3, by - 4, 2, 2, FL_INK);     /* head */
    DrawRectangle(x + dir * 3, by - 7, 1, 3, FL_INK);     /* ears */
    DrawRectangle(x + dir * 4, by - 6, 1, 2, FL_INK);
    DrawRectangle(x - dir * 4, by - 2, 1, 1, FL_PAPER);   /* white scut */
    if (hop > 2) {
        DrawRectangle(x - dir * 2, by + 1, 1, 2, FL_INK); /* legs stretched */
        DrawRectangle(x + dir * 2, by + 1, 1, 2, FL_INK);
    }
    /* Little puffs where it last touched down. */
    if (hop == 0) {
        DrawRectangle(x - 1, y + 1, 1, 1, FL_PAPER);
        DrawRectangle(x + 1, y + 1, 1, 1, FL_PAPER);
    }
}

/* The aurora: curtains of light rippling across the sky. Each column of
 * the band is a run of dithered paper whose length and offset follow two
 * slow waves, so the curtain folds as it moves. */
void scenery_draw_aurora(int top, int height, uint32_t tick, float strength)
{
    for (int x = 0; x < 480; x += 2) {
        float fx = (float)x;
        float wave = sinf(fx * 0.018f + (float)tick * 0.021f) * 0.6f
                     + sinf(fx * 0.047f - (float)tick * 0.013f) * 0.4f;
        float lift = sinf(fx * 0.011f + (float)tick * 0.008f);
        int y0 = top + (int)((lift + 1.0f) * (float)height * 0.18f);
        int len = (int)((float)height * (0.35f + 0.3f * (wave + 1.0f) * 0.5f));
        float bright = strength * (0.45f + 0.55f * (wave + 1.0f) * 0.5f);
        /* Bright at the hem, thinning upwards: paper on every n-th row. */
        for (int y = 0; y < len; y += 1) {
            float fall = 1.0f - (float)y / (float)len;
            float d = bright * fall * fall;
            int level = (int)(d * 16.0f);
            int py = y0 + len - y;
            static const uint8_t BAYER[4][4] = {
                {0, 8, 2, 10}, {12, 4, 14, 6}, {3, 11, 1, 9}, {15, 7, 13, 5},
            };
            if (BAYER[py & 3][x & 3] < level) {
                DrawRectangle(x, py, 1, 1, FL_PAPER);
            }
            if (BAYER[py & 3][(x + 1) & 3] < level) {
                DrawRectangle(x + 1, py, 1, 1, FL_PAPER);
            }
        }
        /* The hem itself: a bright folded line. */
        if (bright > 0.5f) {
            DrawRectangle(x, y0 + len, 2, 1, FL_PAPER);
        }
    }
}

void scenery_draw_under(const fl_state_t *model)
{
    const scenery_t *s = scenery_for(model);
    uint32_t tick = model->tick;
    draw_fall(s, tick);
    draw_ice_glints(s, tick);
    draw_mist(s, tick);
    draw_reeds(s, tick);
    draw_drips(s, tick);
    draw_motes(s, tick);
    draw_hare(s, tick);
    draw_pine_snow(s, tick);
}

void scenery_draw_over(const fl_state_t *model)
{
    const scenery_t *s = scenery_for(model);
    uint32_t tick = model->tick;
    if (s->aurora) {
        scenery_draw_aurora(40, 34, tick, 0.45f);
    }
    draw_lamps(s, tick);
    draw_smoke(s, tick);
    draw_snow(s, tick);
    draw_gusts(s, tick);
    if (s->birds) {
        scenery_draw_birds(tick);
    }
    if (s->eagle) {
        draw_eagle(tick);
    }
}

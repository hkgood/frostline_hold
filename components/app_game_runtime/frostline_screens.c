/* The screens around the battle, and the banners over it.
 *
 *   title     the fortress by night: aurora, stars, blowing snow, the
 *             braziers on its towers, and the logo carved out of ice
 *   settings  the same night behind a panel of rows
 *   ending    the fortress at dawn, the sun's rays turning, the story told
 *   battle    the wave call, a boss's entrance, the card that introduces
 *             each enemy the first time it walks on
 *
 * The title and ending plates are AI engravings (title_art.py); everything
 * that moves is drawn here, at the points those plates show.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "private/frostline_view_internal.h"
#include "include/frostline_text.h"

#include <math.h>
#include <string.h>

#define PI_F 3.14159265f

/* Frames drawn on the screens around the battle: the clock of everything
 * that only decorates, so it runs on while the model stands still. */
static uint32_t s_frame;

static uint32_t hash2(uint32_t a, uint32_t b)
{
    return jag_hash(a * 0x9E3779B1u ^ (b + 0x7F4A7C15u));
}

static float clamp01(float v)
{
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

/* Fast at first, settling at the end. */
static float ease_out(float t)
{
    t = 1.0f - clamp01(t);
    return 1.0f - t * t * t;
}

/* Text with an ink rim, so it reads over any part of a plate. */
static void draw_text_rimmed(const char *text, int cx, int y, fl_font_t font,
                             Color fill, Color rim)
{
    static const int OFFSETS[8][2] = {{-1, -1}, {0, -1}, {1, -1}, {-1, 0},
                                      {1, 0}, {-1, 1}, {0, 1}, {1, 1}};
    for (int i = 0; i < 8; ++i) {
        fl_text_draw_centered(text, cx + OFFSETS[i][0], y + OFFSETS[i][1], font, rim);
    }
    fl_text_draw_centered(text, cx, y, font, fill);
}

/* The first `count` characters of a UTF-8 string: a typewriter. */
static const char *first_chars(const char *text, int count)
{
    static char buffer[192];
    const char *cursor = text;
    for (int i = 0; i < count && *cursor; ++i) {
        fl_text_next_codepoint(&cursor);
    }
    size_t length = (size_t)(cursor - text);
    if (length >= sizeof(buffer)) {
        length = sizeof(buffer) - 1;
    }
    memcpy(buffer, text, length);
    buffer[length] = '\0';
    return buffer;
}

static int char_count(const char *text)
{
    int count = 0;
    while (*text) {
        fl_text_next_codepoint(&text);
        ++count;
    }
    return count;
}

/* A small diamond, the ornament every rule and ribbon ends in. */
static void draw_diamond(int cx, int cy, int r, Color color)
{
    DrawTriangle((Vector2){(float)cx, (float)(cy - r)},
                 (Vector2){(float)(cx - r), (float)cy},
                 (Vector2){(float)(cx + r), (float)cy}, color);
    DrawTriangle((Vector2){(float)(cx - r), (float)cy},
                 (Vector2){(float)cx, (float)(cy + r)},
                 (Vector2){(float)(cx + r), (float)cy}, color);
}

/* A rule with a diamond at each end, for under a heading. */
static void draw_rule(int cx, int y, int half, Color color)
{
    DrawRectangle(cx - half, y, half * 2, 1, color);
    draw_diamond(cx - half - 4, y, 3, color);
    draw_diamond(cx + half + 4, y, 3, color);
    draw_diamond(cx, y, 2, color);
}

/* A four-point twinkle. */
static void draw_twinkle(int x, int y, int arm, Color color)
{
    DrawRectangle(x - arm, y, arm * 2 + 1, 1, color);
    DrawRectangle(x, y - arm, 1, arm * 2 + 1, color);
    if (arm >= 3) {
        DrawRectangle(x - 1, y - 1, 3, 3, color);
    }
}

/* ------------------------------------------------------------------------
 * The night sky
 * --------------------------------------------------------------------- */

/* The title plate's braziers: the base of each fire (title_art.py). */
static const int TITLE_BRAZIERS[2][2] = {{158, 199}, {291, 221}};
/* The band of open sky above the fortress. */
#define SKY_TOP 6
#define SKY_HEIGHT 88

/* A few bright stars breathe in and out, and now and then one falls. */
static void draw_star_twinkles(uint32_t frame)
{
    for (int i = 0; i < 22; ++i) {
        uint32_t h = hash2((uint32_t)i, 301u);
        int x = 40 + (int)(h % 400u);
        int y = SKY_TOP + (int)((h >> 9) % 120u);
        if (corner_cut(x, y)) {
            continue;
        }
        float phase = (float)frame * (0.05f + (float)((h >> 20) % 5u) * 0.01f)
                      + (float)(h % 628u) * 0.01f;
        float glow = sinf(phase);
        if (glow > 0.75f) {
            draw_twinkle(x, y, 3, FL_PAPER);
        } else if (glow > 0.2f) {
            draw_twinkle(x, y, 1, FL_PAPER);
        } else {
            DrawRectangle(x, y, 1, 1, FL_PAPER);
        }
    }

    /* A shooting star every eight seconds or so. */
    const uint32_t period = 250u;
    uint32_t t = frame % period;
    if (t < 16u) {
        uint32_t h = hash2(frame / period, 77u);
        float sx = 120.0f + (float)(h % 260u);
        float sy = 14.0f + (float)((h >> 8) % 40u);
        float x = sx - (float)t * 9.0f;
        float y = sy + (float)t * 4.0f;
        int tail = t < 4u ? (int)t * 5 : 20;
        DrawLine((int)x, (int)y, (int)(x + (float)tail * 0.9f),
                 (int)(y - (float)tail * 0.4f), FL_PAPER);
        draw_twinkle((int)x, (int)y, t < 10u ? 2 : 1, FL_PAPER);
    }
}

/* Snow blown across the night in three depths. The whole field sways as
 * the wind rises and falls; the near flakes are big and fast. */
static void draw_night_snow(uint32_t frame, int amount)
{
    float t = (float)frame;
    /* The drift is the integral of a gusting wind, so it never jumps. */
    float drift = t * 1.1f - cosf(t * 0.021f) * 38.0f;
    for (int layer = 0; layer < 3; ++layer) {
        int count = layer == 0 ? amount * 2 : (layer == 1 ? amount : amount / 4);
        float fall = layer == 0 ? 0.6f : (layer == 1 ? 1.4f : 2.8f);
        float lean = layer == 0 ? 0.4f : (layer == 1 ? 0.9f : 1.6f);
        for (int i = 0; i < count; ++i) {
            uint32_t h = hash2((uint32_t)i, 500u + (uint32_t)layer);
            float speed = fall * (0.8f + (float)((h >> 24) % 5u) * 0.1f);
            float sway = sinf(t * 0.05f + (float)(h % 628u) * 0.01f) * 4.0f;
            int x = ((int)((float)(h % 480u) + drift * lean + sway) % 480 + 480) % 480;
            int y = (int)((float)((h >> 9) % 480u) + t * speed) % 480;
            if (corner_cut(x, y)) {
                continue;
            }
            if (layer == 0) {
                DrawRectangle(x, y, 1, 1, FL_PAPER);
            } else if (layer == 1) {
                DrawRectangle(x - 1, y, 3, 1, FL_INK);
                DrawRectangle(x, y - 1, 1, 3, FL_INK);
                DrawRectangle(x, y, 1, 1, FL_PAPER);
            } else {
                draw_diamond(x, y, 3, FL_INK);
                draw_diamond(x, y, 2, FL_PAPER);
            }
        }
    }
}

/* A brazier: a halo of dithered light, three tongues of fire leaning out
 * of step, a hot core, and sparks lifting into the dark. */
static void draw_brazier(int cx, int base, float scale, uint32_t frame,
                         uint32_t seed)
{
    float t = (float)frame;
    float s = (float)seed;
    float pulse = 0.16f + 0.05f * sinf(t * 0.31f + s);
    draw_dither_ellipse(cx, base - (int)(12.0f * scale), (int)(30.0f * scale),
                        (int)(26.0f * scale), pulse, FL_PAPER);

    for (int k = 0; k < 3; ++k) {
        float sway = sinf(t * 0.36f + (float)k * 2.1f + s) * 3.0f * scale;
        float height = (22.0f + 6.0f * sinf(t * 0.52f + (float)k * 1.7f + s * 0.3f)
                        - (float)k * 5.0f) * scale;
        float half = (7.0f - (float)k * 1.5f) * scale;
        float offset = (k == 1 ? -4.0f : (k == 2 ? 4.0f : 0.0f)) * scale;
        float bx = (float)cx + offset;
        DrawTriangle((Vector2){bx - half, (float)base},
                     (Vector2){bx + sway, (float)base - height},
                     (Vector2){bx + half, (float)base}, FL_PAPER);
        /* A rounded belly under each tongue. */
        draw_dither_ellipse((int)bx, base - 2, (int)half, 3, 1.0f, FL_PAPER);
    }
    /* Ink strokes of heat running up the fire. */
    for (int k = 0; k < 3; ++k) {
        int phase = (int)((frame + (uint32_t)k * 3u) % 9u);
        int x = cx - 3 + k * 3;
        int top = base - 6 - phase * 2;
        DrawLine(x, base - 2, x + (int)(sinf(t * 0.4f + (float)k) * 2.0f), top, FL_INK);
    }
    /* Sparks rise, sway and go out. */
    for (int k = 0; k < 6; ++k) {
        uint32_t life = 34u;
        uint32_t age = (frame + (uint32_t)k * 7u + seed * 13u) % life;
        uint32_t h = hash2((frame + (uint32_t)k * 7u + seed * 13u) / life, (uint32_t)k + seed);
        float drift = ((float)(h % 21u) - 10.0f) * 0.12f;
        int x = cx + (int)(drift * (float)age + sinf((float)age * 0.3f + (float)k) * 2.0f);
        int y = base - (int)(18.0f * scale) - (int)((float)age * 1.4f);
        if (age < 12u) {
            DrawRectangle(x, y, 2, 2, FL_PAPER);
        } else if (age < 28u) {
            DrawRectangle(x, y, 1, 1, FL_PAPER);
        }
    }
}

static void draw_night(uint32_t frame, float aurora, int snow)
{
    draw_full(fv_assets.title, fv_assets.title_backdrop, 0.0f, 0.0f, 480.0f);
    scenery_draw_aurora(SKY_TOP, SKY_HEIGHT, frame, aurora);
    draw_star_twinkles(frame);
    draw_brazier(TITLE_BRAZIERS[0][0], TITLE_BRAZIERS[0][1], 1.25f, frame, 1u);
    draw_brazier(TITLE_BRAZIERS[1][0], TITLE_BRAZIERS[1][1], 1.0f, frame, 4u);
    draw_night_snow(frame, snow);
}

/* ------------------------------------------------------------------------
 * The logo
 * --------------------------------------------------------------------- */

/* The lettering is laid out once per language as a small bitmap of the
 * font's own pixels, then drawn large: an extruded block of ice with an
 * engraved lower half, snow on its tops and icicles under it. */
#define LOGO_MAX_W 96
#define LOGO_MAX_H 20

static struct {
    uint8_t lang;          /* the language laid out, 0xFF before any */
    int width, height;     /* in font pixels */
    int scale;             /* screen pixels per font pixel */
    uint8_t bits[LOGO_MAX_H][LOGO_MAX_W];
    uint8_t shade[LOGO_MAX_H];   /* rows engraved with hatching */
} s_logo = {.lang = 0xFF};

static void logo_put_latin(const char *text, int x, int y)
{
    for (; *text; ++text, x += 6) {
        const uint8_t *rows = fl_text_latin_glyph(*text);
        if (rows == NULL) {
            continue;
        }
        for (int r = 0; r < 7; ++r) {
            for (int c = 0; c < 5; ++c) {
                if ((rows[r] >> (4 - c)) & 1u && x + c < LOGO_MAX_W
                        && y + r < LOGO_MAX_H) {
                    s_logo.bits[y + r][x + c] = 1;
                }
            }
        }
    }
}

static void logo_layout(uint8_t lang)
{
    memset(&s_logo, 0, sizeof(s_logo));
    s_logo.lang = lang;
    if (lang == FL_LANG_ZH) {
        /* The title font's own glyphs, a pixel of air between them. */
        const char *text = fl_tr(FL_TXT_UI_GAME_TITLE);
        int x = 0;
        int height = 16;
        while (*text) {
            uint32_t cp = fl_text_next_codepoint(&text);
            const uint16_t *rows;
            int advance;
            if (!fl_text_title_glyph(cp, &rows, &height, &advance)) {
                continue;
            }
            for (int r = 0; r < height && r < LOGO_MAX_H; ++r) {
                for (int c = 0; c < 16 && x + c < LOGO_MAX_W; ++c) {
                    if ((rows[r] >> (15 - c)) & 1u) {
                        s_logo.bits[r][x + c] = 1;
                    }
                }
            }
            x += advance + 1;
        }
        s_logo.height = height;
        s_logo.scale = 4;
        for (int r = height * 11 / 20; r < height; ++r) {
            s_logo.shade[r] = 1;
        }
    } else {
        /* Two lines of the 5x7 face: FROSTLINE over HOLD. */
        const char *top = "FROSTLINE";
        const char *bottom = "HOLD";
        int top_w = (int)strlen(top) * 6 - 1;
        int bottom_w = (int)strlen(bottom) * 6 - 1;
        logo_put_latin(top, 0, 0);
        logo_put_latin(bottom, (top_w - bottom_w) / 2, 9);
        s_logo.height = 16;
        s_logo.scale = 5;
        for (int r = 4; r < 7; ++r) {
            s_logo.shade[r] = 1;
            s_logo.shade[r + 9] = 1;
        }
    }
    /* Trim to the pixels actually set. */
    int width = 0;
    for (int r = 0; r < LOGO_MAX_H; ++r) {
        for (int c = 0; c < LOGO_MAX_W; ++c) {
            if (s_logo.bits[r][c] && c + 1 > width) {
                width = c + 1;
            }
        }
    }
    s_logo.width = width;
}

static bool logo_bit(int r, int c)
{
    return r >= 0 && c >= 0 && r < s_logo.height && c < s_logo.width
           && s_logo.bits[r][c];
}

/* Every horizontal run of set pixels, drawn as one grown rectangle. */
static void logo_runs(int ox, int oy, int dx, int dy, int grow, Color color)
{
    int s = s_logo.scale;
    for (int r = 0; r < s_logo.height; ++r) {
        int c = 0;
        while (c < s_logo.width) {
            if (!s_logo.bits[r][c]) {
                ++c;
                continue;
            }
            int start = c;
            while (c < s_logo.width && s_logo.bits[r][c]) {
                ++c;
            }
            DrawRectangle(ox + start * s + dx - grow, oy + r * s + dy - grow,
                          (c - start) * s + grow * 2, s + grow * 2, color);
        }
    }
}

/* Draws the logo centred on `cx`, revealed out to `reveal` (0..1) from the
 * middle, and returns its height on screen. */
static int draw_logo(int cx, int top, float reveal, uint32_t frame)
{
    uint8_t lang = fl_text_language();
    if (s_logo.lang != lang) {
        logo_layout(lang);
    }
    int s = s_logo.scale;
    int w = s_logo.width * s;
    int h = s_logo.height * s;
    int ox = cx - w / 2;
    int oy = top;
    int half = (int)((float)(w / 2 + 16) * clamp01(reveal));
    if (half <= 0) {
        return h;
    }
    BeginScissorMode(cx - half, 0, half * 2, oy + h + 24);

    /* A dark halo parts the lettering from the aurora behind it. */
    logo_runs(ox, oy, 2, 2, 6, FL_INK);
    /* The block of ice: a solid side trailing down and right, and the face
     * outlined in ink. Hanzi strokes sit a font pixel apart, too close for
     * a side, so they keep a flat face. */
    if (s > 4) {
        for (int d = 4; d >= 2; --d) {
            logo_runs(ox, oy, d, d, 0, FL_PAPER);
        }
        logo_runs(ox, oy, 0, 0, 1, FL_INK);
    }
    logo_runs(ox, oy, 0, 0, 0, FL_PAPER);

    for (int r = 0; r < s_logo.height; ++r) {
        for (int c = 0; c < s_logo.width; ++c) {
            if (!s_logo.bits[r][c]) {
                continue;
            }
            int px = ox + c * s;
            int py = oy + r * s;
            /* The lower half is engraved, a hairline across each row. */
            if (s_logo.shade[r] && s > 3) {
                DrawRectangle(px, py + s - 2, s, 1, FL_INK);
            }
            /* Snow lies on every top edge, heaped here and there. */
            if (!logo_bit(r - 1, c) && !logo_bit(r - 2, c)) {
                DrawRectangle(px - 1, py - 3, s + 2, 2, FL_PAPER);
                if (hash2((uint32_t)c, (uint32_t)r + 9u) % 3u == 0u) {
                    DrawRectangle(px, py - 4, s, 1, FL_PAPER);
                }
            }
            /* Icicles hang from some bottom edges and drip now and then. */
            if (!logo_bit(r + 1, c) && !logo_bit(r + 2, c)) {
                uint32_t hh = hash2((uint32_t)c, (uint32_t)r + 31u);
                if (hh % 3u == 0u) {
                    int length = 4 + (int)(hh >> 8) % 8;
                    int ix = px + s / 2;
                    int iy = py + s + 2;
                    DrawTriangle((Vector2){(float)(ix - 2), (float)iy},
                                 (Vector2){(float)ix, (float)(iy + length + 1)},
                                 (Vector2){(float)(ix + 2), (float)iy}, FL_INK);
                    DrawTriangle((Vector2){(float)(ix - 1), (float)iy},
                                 (Vector2){(float)ix, (float)(iy + length)},
                                 (Vector2){(float)(ix + 1), (float)iy}, FL_PAPER);
                    uint32_t drip = (frame + (hh >> 4) % 90u) % 90u;
                    if ((hh >> 12) % 4u == 0u && drip < 14u) {
                        DrawRectangle(ix, iy + length + 2 + (int)(drip * drip / 6u),
                                      1, 2, FL_PAPER);
                    }
                }
            }
        }
    }
    EndScissorMode();

    /* While it opens, a seam of light runs out from the middle. */
    if (reveal < 1.0f) {
        for (int side = -1; side <= 1; side += 2) {
            int x = cx + side * half;
            DrawRectangle(x, oy - 6, 1, h + 14, FL_PAPER);
            for (int k = 0; k < 3; ++k) {
                uint32_t g = hash2(frame, (uint32_t)(k + side * 7 + 20));
                draw_twinkle(x + side * (int)(g % 6u), oy + (int)((g >> 8) % (uint32_t)h),
                             (int)(g >> 16) % 3 + 1, FL_PAPER);
            }
        }
        return h;
    }

    /* Once whole, a glint skims along the tops every four seconds. */
    const uint32_t period = 120u;
    uint32_t t = frame % period;
    if (t < 28u) {
        int col = (int)((float)s_logo.width * (float)t / 27.0f);
        for (int r = 0; r < s_logo.height; ++r) {
            if (logo_bit(r, col)) {
                draw_twinkle(ox + col * s + s / 2, oy + r * s - 2, t % 4u < 2u ? 4 : 3,
                             FL_PAPER);
                DrawRectangle(ox + col * s + s / 2, oy + r * s - 2, 1, 1, FL_INK);
                break;
            }
        }
    }
    return h;
}

/* ------------------------------------------------------------------------
 * Title
 * --------------------------------------------------------------------- */

#define TITLE_LOGO_TOP 22

void screens_draw_title(const fl_state_t *model)
{
    uint32_t frame = ++s_frame;
    float t = (float)model->screen_ticks;

    draw_night(frame, clamp01(t / 60.0f), 30);

    /* The logo opens from the middle, then the subtitle types itself. */
    int logo_h = draw_logo(240, TITLE_LOGO_TOP, ease_out((t - 8.0f) / 30.0f), frame);
    int sub_y = TITLE_LOGO_TOP + logo_h + 14;
    if (t > 34.0f) {
        const char *subtitle = fl_tr(FL_TXT_UI_GAME_SUBTITLE);
        int shown = (int)((t - 34.0f) / 2.0f);
        int width = fl_text_width(subtitle, FL_FONT_SMALL);
        draw_text_rimmed(first_chars(subtitle, shown), 240, sub_y, FL_FONT_SMALL,
                         FL_PAPER, FL_INK);
        if (shown >= char_count(subtitle)) {
            int rule_y = sub_y + fl_text_height(FL_FONT_SMALL) / 2;
            DrawRectangle(240 - width / 2 - 40, rule_y, 30, 1, FL_PAPER);
            DrawRectangle(240 + width / 2 + 10, rule_y, 30, 1, FL_PAPER);
            draw_diamond(240 - width / 2 - 44, rule_y, 2, FL_PAPER);
            draw_diamond(240 + width / 2 + 44, rule_y, 2, FL_PAPER);
        }
    }

    /* The buttons rise into place one after the other. */
    static const fl_text_id_t LABELS[2] = {FL_TXT_UI_TITLE_START,
                                           FL_TXT_UI_TITLE_SETTINGS};
    static const int TOPS[2] = {FL_TITLE_START_Y, FL_TITLE_SETTINGS_Y};
    for (int i = 0; i < 2; ++i) {
        float rise = ease_out((t - 40.0f - (float)i * 6.0f) / 14.0f);
        if (rise <= 0.0f) {
            continue;
        }
        int y = TOPS[i] + (int)((1.0f - rise) * 140.0f);
        /* A dark plinth under each button lifts it off the busy plate. */
        fill_plate(FL_TITLE_BTN_X - 4, y - 4, FL_TITLE_BTN_W + 8,
                   FL_TITLE_BTN_H + 8, PLATE_CUT + 3, FL_INK);
        draw_button(FL_TITLE_BTN_X, y, FL_TITLE_BTN_W, FL_TITLE_BTN_H,
                    fl_tr(LABELS[i]), true, i == 0 && rise >= 1.0f, frame);
        if (i == 0 && rise >= 1.0f) {
            /* Little flames point at the start button from both sides. */
            int bob = (int)(sinf((float)frame * 0.2f) * 3.0f);
            int my = y + FL_TITLE_BTN_H / 2;
            draw_diamond(FL_TITLE_BTN_X - 18 + bob, my, 4, FL_PAPER);
            draw_diamond(FL_TITLE_BTN_X + FL_TITLE_BTN_W + 18 - bob, my, 4, FL_PAPER);
        }
    }

    /* The curtain: the night opens like an eye. */
    float open = ease_out(t / 18.0f);
    if (open < 1.0f) {
        int bar = (int)(240.0f * (1.0f - open));
        DrawRectangle(0, 0, 480, bar, FL_INK);
        DrawRectangle(0, 480 - bar, 480, bar, FL_INK);
        DrawRectangle(0, bar, 480, 1, FL_PAPER);
        DrawRectangle(0, 479 - bar, 480, 1, FL_PAPER);
    }
}

/* ------------------------------------------------------------------------
 * Settings
 * --------------------------------------------------------------------- */

#define SET_PANEL_X 48
#define SET_PANEL_Y 34
#define SET_PANEL_W 384
#define SET_PANEL_H 414

/* A volume: minus, a staircase of bars, plus. */
static void draw_volume_row(int row, uint8_t level, uint32_t frame)
{
    int mx, my, mw, mh, px, py, pw, ph;
    fl_settings_button_rect(row, 0, &mx, &my, &mw, &mh);
    fl_settings_button_rect(row, 1, &px, &py, &pw, &ph);
    draw_button(mx, my, mw, mh, "-", level > 0, false, frame);
    draw_button(px, py, pw, ph, "+", level < FL_VOLUME_MAX, false, frame);
    int left = mx + mw + 8;
    int span = px - 8 - left;
    int gap = 4;
    int bar_w = (span - gap * (FL_VOLUME_MAX - 1)) / FL_VOLUME_MAX;
    int bottom = my + mh - 6;
    for (int i = 0; i < FL_VOLUME_MAX; ++i) {
        int bh = 10 + i * 6;
        int bx = left + i * (bar_w + gap);
        if (i < level) {
            DrawRectangle(bx, bottom - bh, bar_w, bh, FL_PAPER);
        } else {
            DrawRectangleLines(bx, bottom - bh, bar_w, bh, FL_PAPER);
        }
    }
    if (level == 0) {
        /* Muted: a stroke through the stairs. */
        DrawLine(left - 2, bottom + 2, left + span + 2, bottom - 30, FL_PAPER);
    }
}

void screens_draw_settings(const fl_state_t *model)
{
    uint32_t frame = ++s_frame;
    const fl_settings_t *s = &model->settings;
    draw_night(frame, 1.0f, 18);

    /* The panel, with a second frame inside the first. */
    draw_frame_panel(SET_PANEL_X, SET_PANEL_Y, SET_PANEL_W, SET_PANEL_H);
    fl_text_draw_centered(fl_tr(FL_TXT_UI_SETTINGS), 240, SET_PANEL_Y + 18,
                          FL_FONT_HUGE, FL_PAPER);
    draw_rule(240, FL_SET_ROW_Y0 - 10, 120, FL_PAPER);

    static const fl_text_id_t LABELS[FL_SET_ROW_COUNT] = {
        FL_TXT_UI_SET_MUSIC, FL_TXT_UI_SET_SOUND, FL_TXT_UI_SET_HAPTICS,
        FL_TXT_UI_SET_LANGUAGE, FL_TXT_UI_SET_THEME, FL_TXT_UI_SET_RESET,
    };
    for (int row = 0; row < FL_SET_ROW_COUNT; ++row) {
        int y = FL_SET_ROW_Y0 + row * FL_SET_ROW_GAP;
        if (row != FL_SET_ROW_RESET) {
            fl_text_draw(fl_tr(LABELS[row]), FL_SET_LABEL_X,
                         y + (FL_SET_ROW_H - fl_text_height(FL_FONT_LARGE)) / 2,
                         FL_FONT_LARGE, FL_PAPER);
        }
        int bx, by, bw, bh;
        switch (row) {
        case FL_SET_ROW_MUSIC:
            draw_volume_row(row, s->music, frame);
            break;
        case FL_SET_ROW_SOUND:
            draw_volume_row(row, s->sound, frame);
            break;
        case FL_SET_ROW_HAPTICS:
            fl_settings_button_rect(row, 0, &bx, &by, &bw, &bh);
            draw_button(bx, by, bw, bh, fl_tr(s->haptics ? FL_TXT_UI_ON : FL_TXT_UI_OFF),
                        s->haptics != 0, false, frame);
            break;
        case FL_SET_ROW_LANGUAGE:
            fl_settings_button_rect(row, 0, &bx, &by, &bw, &bh);
            draw_button(bx, by, bw, bh,
                        fl_fmt("< %s >", fl_tr(s->lang == FL_LANG_ZH ? FL_TXT_LANG_ZH
                                                                     : FL_TXT_LANG_EN)),
                        true, false, frame);
            break;
        case FL_SET_ROW_THEME:
            fl_settings_button_rect(row, 0, &bx, &by, &bw, &bh);
            draw_button(bx, by, bw, bh,
                        fl_fmt("< %s >", fl_tr(s->theme == FL_THEME_NIGHT
                                               ? FL_TXT_UI_THEME_NIGHT
                                               : FL_TXT_UI_THEME_DAY)),
                        true, false, frame);
            break;
        case FL_SET_ROW_RESET:
            /* Reset spans the row: its label is its button. */
            fl_settings_button_rect(row, 0, &bx, &by, &bw, &bh);
            if (model->reset_armed == 1) {
                draw_button(bx, by, bw, bh, fl_tr(FL_TXT_UI_SET_CONFIRM),
                            (frame / 8u) % 2u == 0u, true, frame);
            } else if (model->reset_armed == 2) {
                draw_button(bx, by, bw, bh, fl_tr(FL_TXT_UI_SET_CLEARED), false,
                            false, frame);
            } else {
                draw_button(bx, by, bw, bh, fl_tr(FL_TXT_UI_SET_RESET), true, false,
                            frame);
            }
            break;
        default:
            break;
        }
    }
    draw_button(FL_SET_BACK_X, FL_SET_BACK_Y, FL_SET_BACK_W, FL_SET_BACK_H,
                fl_tr(FL_TXT_UI_BTN_BACK), true, false, frame);
}

/* ------------------------------------------------------------------------
 * Ending
 * --------------------------------------------------------------------- */

/* The sun rising between the peaks of the dawn plate. */
#define SUN_X 362
#define SUN_Y 146
#define SUN_HORIZON 156
/* Smoke from the braziers, now only embers. */
static const int ENDING_SMOKE[2][2] = {{160, 124}, {292, 146}};

#define ENDING_LINE_START 34
#define ENDING_LINE_GAP 30

static void draw_sun(uint32_t frame)
{
    BeginScissorMode(0, 0, 480, SUN_HORIZON);
    float turn = (float)frame * 0.004f;
    for (int i = 0; i < 20; ++i) {
        float a = turn + (float)i * (2.0f * PI_F / 20.0f);
        float dx = cosf(a);
        float dy = sinf(a);
        float length = 70.0f + (float)(hash2((uint32_t)i, 9u) % 70u)
                       + sinf((float)frame * 0.05f + (float)i) * 10.0f;
        /* Engraved rays: a dotted line, longer rays a little bolder. */
        for (float r = 26.0f; r < length; r += 4.0f) {
            int x = SUN_X + (int)(dx * r);
            int y = SUN_Y + (int)(dy * r);
            DrawRectangle(x, y, 1, 1, FL_INK);
            if (i % 2 == 0 && r < length * 0.6f) {
                DrawRectangle(x + 1, y, 1, 1, FL_INK);
            }
        }
    }
    DrawCircleLines(SUN_X, SUN_Y, 17.0f, FL_INK);
    DrawCircleLines(SUN_X, SUN_Y, 18.0f, FL_INK);
    float breathe = 21.0f + sinf((float)frame * 0.08f) * 1.5f;
    for (int k = 0; k < 24; k += 2) {
        float a = (float)k * (2.0f * PI_F / 24.0f) + turn * 3.0f;
        DrawRectangle(SUN_X + (int)(cosf(a) * breathe), SUN_Y + (int)(sinf(a) * breathe),
                      1, 1, FL_INK);
    }
    EndScissorMode();
}

static void draw_ending_smoke(uint32_t frame)
{
    for (int b = 0; b < 2; ++b) {
        for (int k = 0; k < 5; ++k) {
            uint32_t life = 90u;
            uint32_t age = (frame + (uint32_t)k * 18u + (uint32_t)b * 40u) % life;
            float f = (float)age / (float)life;
            int x = ENDING_SMOKE[b][0] + (int)(f * 26.0f + sinf(f * 6.0f + (float)k) * 3.0f);
            int y = ENDING_SMOKE[b][1] - (int)(f * 46.0f);
            int r = 3 + (int)(f * 8.0f);
            draw_dither_disc(x, y, r, 0.45f * (1.0f - f), FL_INK);
        }
    }
}

void screens_draw_ending(const fl_state_t *model)
{
    uint32_t frame = ++s_frame;
    float t = (float)model->screen_ticks;

    draw_full(fv_assets.title, fv_assets.ending_backdrop, 0.0f, 0.0f, 480.0f);
    draw_sun(frame);
    draw_ending_smoke(frame);
    scenery_draw_birds(frame + 300u);
    scenery_draw_birds(frame + 40u);

    /* The heading unrolls on an ink ribbon. */
    float unroll = ease_out((t - 6.0f) / 20.0f);
    if (unroll > 0.0f) {
        int half = (int)(150.0f * unroll);
        int ry = 26;
        int rh = fl_text_height(FL_FONT_HUGE) + 18;
        DrawRectangle(240 - half, ry, half * 2, rh, FL_INK);
        DrawRectangle(240 - half, ry + 3, half * 2, 1, FL_PAPER);
        DrawRectangle(240 - half, ry + rh - 4, half * 2, 1, FL_PAPER);
        draw_diamond(240 - half, ry + rh / 2, rh / 2, FL_INK);
        draw_diamond(240 + half, ry + rh / 2, rh / 2, FL_INK);
        BeginScissorMode(240 - half, 0, half * 2, 120);
        fl_text_draw_centered(fl_tr(FL_TXT_UI_ENDING_TITLE), 240, ry + 9, FL_FONT_HUGE,
                              FL_PAPER);
        EndScissorMode();
    }

    /* The story panel rises over the forest. */
    float rise = ease_out((t - 14.0f) / 18.0f);
    if (rise <= 0.0f) {
        return;
    }
    int panel_y = 292 + (int)((1.0f - rise) * 200.0f);
    draw_frame_panel(60, panel_y, 360, 164);

    int line_h = fl_text_height(FL_FONT_SMALL) + 6;
    for (int i = 0; i < FL_TXT_ENDING_COUNT; ++i) {
        float start = (float)(ENDING_LINE_START + i * ENDING_LINE_GAP);
        if (t < start) {
            break;
        }
        const char *line = fl_tr((fl_text_id_t)(FL_TXT_ENDING_0 + i));
        int shown = (int)((t - start) * 1.2f) + 1;
        fl_text_draw_centered(first_chars(line, shown), 240, panel_y + 14 + i * line_h,
                              FL_FONT_SMALL, FL_PAPER);
    }
    float done = (float)(ENDING_LINE_START + FL_TXT_ENDING_COUNT * ENDING_LINE_GAP);
    int thanks_y = panel_y + 14 + FL_TXT_ENDING_COUNT * line_h + 4;
    if (t >= done) {
        fl_text_draw_centered(fl_tr(FL_TXT_UI_ENDING_THANKS), 240, thanks_y,
                              FL_FONT_LARGE, FL_PAPER);
        int w = fl_text_width(fl_tr(FL_TXT_UI_ENDING_THANKS), FL_FONT_LARGE);
        int cy = thanks_y + fl_text_height(FL_FONT_LARGE) / 2;
        draw_twinkle(240 - w / 2 - 14, cy, (frame / 6u) % 2u ? 3 : 2, FL_PAPER);
        draw_twinkle(240 + w / 2 + 14, cy, (frame / 6u) % 2u ? 2 : 3, FL_PAPER);
    }
    if (t >= done + 20.0f && t >= (float)FL_ENDING_MIN_TICKS && (frame / 15u) % 2u == 0u) {
        fl_text_draw_centered(fl_tr(FL_TXT_UI_ENDING_HINT), 240,
                              thanks_y + fl_text_height(FL_FONT_LARGE) + 6,
                              FL_FONT_SMALL, FL_PAPER);
    }
}

/* ------------------------------------------------------------------------
 * Banners over the battle
 * --------------------------------------------------------------------- */

#define WAVE_BANNER_FRAMES 64
#define BOSS_BANNER_FRAMES 100
#define CARD_FRAMES 150
#define CARD_QUEUE 8

static struct {
    bool ready;
    uint16_t run_serial;
    uint32_t tick;           /* the model tick last seen */
    uint32_t tick_rest;      /* ticks not yet worth a whole frame at speed */
    uint8_t phase;
    uint8_t wave;
    uint8_t bosses;          /* bosses alive last frame */
    uint16_t wave_timer;
    uint8_t wave_shown;
    uint16_t boss_timer;
    uint8_t boss_kind;
    uint32_t seen;           /* kinds already introduced */
    uint8_t queue[CARD_QUEUE];
    uint8_t queued;
    uint16_t card_timer;
    uint8_t card_kind;
} s_banner;

void frostline_view_set_seen_enemies(uint32_t mask)
{
    s_banner.seen = mask;
}

uint32_t frostline_view_seen_enemies(void)
{
    return s_banner.seen;
}

static bool is_boss(const fl_enemy_t *enemy)
{
    return enemy->champion || enemy->kind == FL_ENEMY_WYRM
           || enemy->kind == FL_ENEMY_KING;
}

void screens_update_battle(const fl_state_t *model)
{
    if (!s_banner.ready || s_banner.run_serial != model->run_serial) {
        uint32_t seen = s_banner.seen;
        memset(&s_banner, 0, sizeof(s_banner));
        s_banner.seen = seen;
        s_banner.ready = true;
        s_banner.run_serial = model->run_serial;
        s_banner.phase = model->phase;
        s_banner.wave = model->wave;
        s_banner.tick = model->tick;
    }

    /* Banners run on the battle's clock, brought back to real time at 2x
     * and 3x, and stand still while the battle is paused. Time is spent
     * before new banners start, so a banner always gets its full run. */
    uint32_t speed = model->speed > 0 ? model->speed : 1u;
    s_banner.tick_rest += model->tick - s_banner.tick;
    s_banner.tick = model->tick;
    uint32_t elapsed = s_banner.tick_rest / speed;
    s_banner.tick_rest %= speed;
    s_banner.wave_timer = s_banner.wave_timer > elapsed ? (uint16_t)(s_banner.wave_timer - elapsed) : 0;
    s_banner.boss_timer = s_banner.boss_timer > elapsed ? (uint16_t)(s_banner.boss_timer - elapsed) : 0;
    s_banner.card_timer = s_banner.card_timer > elapsed ? (uint16_t)(s_banner.card_timer - elapsed) : 0;

    /* The wave call: a wave starts, or the next one follows straight on. */
    if (model->phase == FL_PHASE_WAVE
            && (s_banner.phase != FL_PHASE_WAVE || s_banner.wave != model->wave)) {
        s_banner.wave_timer = WAVE_BANNER_FRAMES;
        s_banner.wave_shown = model->wave;
    }
    s_banner.phase = model->phase;
    s_banner.wave = model->wave;

    uint8_t bosses = 0;
    int boss_kind = -1;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *enemy = &model->enemies[i];
        if (!enemy->alive) {
            continue;
        }
        int kind = (int)enemy->kind;
        if (is_boss(enemy)) {
            ++bosses;
            boss_kind = kind;
        }
        /* The first sight of a kind queues its card. */
        uint32_t bit = 1u << kind;
        if (!(s_banner.seen & bit)) {
            s_banner.seen |= bit;
            if (s_banner.queued < CARD_QUEUE) {
                s_banner.queue[s_banner.queued++] = (uint8_t)kind;
            }
        }
    }
    if (bosses > s_banner.bosses && boss_kind >= 0) {
        s_banner.boss_timer = BOSS_BANNER_FRAMES;
        s_banner.boss_kind = (uint8_t)boss_kind;
        fx_shake(4, 16);
    }
    s_banner.bosses = bosses;

    if (s_banner.card_timer == 0 && s_banner.queued > 0) {
        s_banner.card_kind = s_banner.queue[0];
        memmove(s_banner.queue, s_banner.queue + 1, (size_t)(s_banner.queued - 1));
        --s_banner.queued;
        s_banner.card_timer = CARD_FRAMES;
    }
}

/* How far a banner is open: rises over `in` frames, holds, closes over the
 * last `out` frames. */
static float banner_open(int age, int total, int in, int out)
{
    float a = ease_out((float)age / (float)in);
    float b = ease_out((float)(total - age) / (float)out);
    return a < b ? a : b;
}

static void draw_wave_banner(const fl_state_t *model, uint32_t frame)
{
    int age = WAVE_BANNER_FRAMES - s_banner.wave_timer;
    float open = banner_open(age, WAVE_BANNER_FRAMES, 8, 8);
    int half = (int)(170.0f * open);
    if (half <= 2) {
        return;
    }
    int cy = 182;
    int rh = fl_text_height(FL_FONT_HUGE) + 20;
    int top = cy - rh / 2;
    /* A ribbon with swallowtails, two paper threads along it. */
    DrawRectangle(240 - half, top, half * 2, rh, FL_INK);
    for (int side = -1; side <= 1; side += 2) {
        float edge = (float)(240 + side * half);
        float tip = edge + (float)(side * 26);
        DrawTriangle((Vector2){edge, (float)top}, (Vector2){tip, (float)top},
                     (Vector2){edge, (float)cy}, FL_INK);
        DrawTriangle((Vector2){edge, (float)cy}, (Vector2){tip, (float)(top + rh)},
                     (Vector2){edge, (float)(top + rh)}, FL_INK);
    }
    DrawRectangle(240 - half, top + 3, half * 2, 1, FL_PAPER);
    DrawRectangle(240 - half, top + rh - 4, half * 2, 1, FL_PAPER);
    BeginScissorMode(240 - half, top, half * 2, rh);
    const char *text = fl_trf(FL_TXT_UI_WAVE_OF, (unsigned)s_banner.wave_shown,
                              (unsigned)fl_wave_count(model));
    fl_text_draw_centered(text, 240, top + 10, FL_FONT_HUGE, FL_PAPER);
    EndScissorMode();
    /* Sparks fly off both ends while it holds. */
    if (open >= 1.0f) {
        for (int k = 0; k < 4; ++k) {
            uint32_t g = hash2(frame, (uint32_t)k + 60u);
            int side = (k % 2) ? 1 : -1;
            int x = 240 + side * (half + 20 + (int)(g % 30u));
            int y = top + (int)((g >> 8) % (uint32_t)rh);
            draw_twinkle(x, y, (int)(g >> 16) % 2 + 1, FL_PAPER);
        }
    }
}

static void draw_boss_banner(uint32_t frame)
{
    int age = BOSS_BANNER_FRAMES - s_banner.boss_timer;
    float open = banner_open(age, BOSS_BANNER_FRAMES, 10, 12);
    /* Letterbox bars close in like a stage. */
    int bar = (int)(34.0f * open);
    if (bar > 0) {
        DrawRectangle(0, 0, 480, bar, FL_INK);
        DrawRectangle(0, 480 - bar, 480, bar, FL_INK);
        DrawRectangle(0, bar, 480, 1, FL_PAPER);
        DrawRectangle(0, 479 - bar, 480, 1, FL_PAPER);
    }
    if (open <= 0.05f) {
        return;
    }
    int cy = 232;
    int half = (int)(180.0f * open);
    int rh = 72;
    DrawRectangle(240 - half, cy - rh / 2, half * 2, rh, FL_INK);
    DrawRectangle(240 - half, cy - rh / 2 + 3, half * 2, 1, FL_PAPER);
    DrawRectangle(240 - half, cy + rh / 2 - 4, half * 2, 1, FL_PAPER);
    BeginScissorMode(240 - half, cy - rh / 2, half * 2, rh);
    if ((frame / 6u) % 3u != 0u) {
        fl_text_draw_centered(fl_tr(FL_TXT_UI_BOSS_COMES), 240, cy - rh / 2 + 10,
                              FL_FONT_SMALL, FL_PAPER);
    }
    const char *name = fl_tr((fl_text_id_t)(FL_TXT_ENEMY_FIRST + s_banner.boss_kind));
    fl_text_draw_centered(name, 240, cy - 4, FL_FONT_HUGE, FL_PAPER);
    EndScissorMode();
    /* Chevrons march inwards on both sides of the name. */
    if (open >= 1.0f) {
        int w = fl_text_width(name, FL_FONT_HUGE);
        for (int k = 0; k < 3; ++k) {
            int step = (int)((frame / 3u + (uint32_t)k) % 3u);
            int off = w / 2 + 20 + (2 - k) * 12 + step;
            int y = cy + 8;
            for (int side = -1; side <= 1; side += 2) {
                int x = 240 + side * off;
                DrawLine(x + side * 5, y - 6, x, y, FL_PAPER);
                DrawLine(x, y, x + side * 5, y + 6, FL_PAPER);
            }
        }
    }
}

static void draw_enemy_card(uint32_t frame)
{
    int age = CARD_FRAMES - s_banner.card_timer;
    float open = banner_open(age, CARD_FRAMES, 10, 10);
    if (open <= 0.0f) {
        return;
    }
    const int w = 344;
    const int h = 88;
    int x = 240 - w / 2;
    int y = -h + (int)((float)(h + 50) * open);
    int kind = s_banner.card_kind;

    draw_frame_panel(x, y, w, h);

    /* The portrait: the creature walking in a paper window. */
    int bx = x + 12;
    int by = y + 12;
    const int box = 64;
    DrawRectangle(bx, by, box, box, FL_PAPER);
    DrawRectangleLines(bx - 2, by - 2, box + 4, box + 4, FL_PAPER);
    float size;
    raylib_lite_atlas_t sheet = enemy_sheet(kind, &size);
    const enemy_look_t *look = enemy_look(kind);
    int f = (int)((frame / (look->step ? look->step : 4u)) % ENEMY_FRAMES);
    BeginScissorMode(bx, by, box, box);
    draw_ground_shadow(bx + box / 2, by + box - 9, 12, 3);
    draw_padded(sheet, fv_assets.enemies[kind][f], (float)(bx + box / 2) - size / 2.0f,
                (float)(by + box - 6) - size + SPRITE_FOOT, size);
    EndScissorMode();

    int tx = bx + box + 12;
    if (age > 12 || (frame / 4u) % 2u == 0u) {
        fl_text_draw(fl_tr(FL_TXT_UI_NEW_ENEMY), tx, y + 10, FL_FONT_SMALL, FL_PAPER);
    }
    int tag_w = fl_text_width(fl_tr(FL_TXT_UI_NEW_ENEMY), FL_FONT_SMALL);
    draw_diamond(tx + tag_w + 8, y + 10 + fl_text_height(FL_FONT_SMALL) / 2, 3, FL_PAPER);
    int name_y = y + 14 + fl_text_height(FL_FONT_SMALL);
    fl_text_draw(fl_tr((fl_text_id_t)(FL_TXT_ENEMY_FIRST + kind)), tx, name_y,
                 FL_FONT_LARGE, FL_PAPER);
    fl_text_draw_wrapped(fl_tr((fl_text_id_t)(FL_TXT_ENEMY_NOTE_FIRST + kind)), tx,
                         name_y + fl_text_height(FL_FONT_LARGE) + 5, x + w - 12 - tx,
                         FL_FONT_SMALL, 3, FL_PAPER, true);
    /* A fuse along the bottom burns down while the card stays. */
    int fuse = (int)((float)(w - 16) * (float)s_banner.card_timer / (float)CARD_FRAMES);
    DrawRectangle(x + 8, y + h - 6, fuse, 1, FL_PAPER);
}

void screens_draw_battle(const fl_state_t *model)
{
    uint32_t frame = model->tick + s_frame;
    if (s_banner.card_timer > 0) {
        draw_enemy_card(frame);
    }
    if (s_banner.wave_timer > 0 && s_banner.boss_timer == 0) {
        draw_wave_banner(model, frame);
    }
    if (s_banner.boss_timer > 0) {
        draw_boss_banner(frame);
    }
}

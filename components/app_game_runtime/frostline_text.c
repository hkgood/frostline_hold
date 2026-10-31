/* The glyph tables are compiled here and nowhere else. */
#define FROSTLINE_TEXT_TABLES
#include "include/frostline_text.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static uint8_t s_lang = FL_LANG_ZH;

void fl_text_set_language(uint8_t lang)
{
    s_lang = lang == FL_LANG_EN ? FL_LANG_EN : FL_LANG_ZH;
}

uint8_t fl_text_language(void)
{
    return s_lang;
}

const char *fl_tr(fl_text_id_t id)
{
    if ((int)id < 0 || id >= FL_TXT_COUNT) {
        return "";
    }
    return FL_TEXT[s_lang][id];
}

/* ------------------------------------------------------------------------
 * Formatting
 * --------------------------------------------------------------------- */

#define FMT_BUFFERS 4
#define FMT_SIZE 160

static char *next_buffer(void)
{
    static char buffers[FMT_BUFFERS][FMT_SIZE];
    static unsigned index;
    return buffers[index++ % FMT_BUFFERS];
}

const char *fl_fmt(const char *format, ...)
{
    char *out = next_buffer();
    va_list args;
    va_start(args, format);
    vsnprintf(out, FMT_SIZE, format ? format : "", args);
    va_end(args);
    return out;
}

const char *fl_trf(fl_text_id_t id, ...)
{
    char *out = next_buffer();
    va_list args;
    va_start(args, id);
    vsnprintf(out, FMT_SIZE, fl_tr(id), args);
    va_end(args);
    return out;
}

/* ------------------------------------------------------------------------
 * The 5x7 Latin face: the engine's letters and digits plus the punctuation
 * the English strings use.
 * --------------------------------------------------------------------- */

static const uint8_t LATIN_DIGITS[10][7] = {
    {14,17,19,21,25,17,14},{4,12,4,4,4,4,14},{14,17,1,2,4,8,31},
    {30,1,1,14,1,1,30},{2,6,10,18,31,2,2},{31,16,16,30,1,1,30},
    {14,16,16,30,17,17,14},{31,1,2,4,8,8,8},{14,17,17,14,17,17,14},
    {14,17,17,15,1,1,14}
};

static const uint8_t LATIN_LETTERS[26][7] = {
    {14,17,17,31,17,17,17},{30,17,17,30,17,17,30},{14,17,16,16,16,17,14},
    {30,17,17,17,17,17,30},{31,16,16,30,16,16,31},{31,16,16,30,16,16,16},
    {14,17,16,23,17,17,15},{17,17,17,31,17,17,17},{14,4,4,4,4,4,14},
    {7,2,2,2,18,18,12},{17,18,20,24,20,18,17},{16,16,16,16,16,16,31},
    {17,27,21,21,17,17,17},{17,25,21,19,17,17,17},{14,17,17,17,17,17,14},
    {30,17,17,30,16,16,16},{14,17,17,17,21,18,13},{30,17,17,30,20,18,17},
    {15,16,16,14,1,1,30},{31,4,4,4,4,4,4},{17,17,17,17,17,17,14},
    {17,17,17,17,17,10,4},{17,17,17,21,21,21,10},{17,17,10,4,10,17,17},
    {17,17,10,4,4,4,4},{31,1,2,4,8,16,31}
};

static const struct {
    char ch;
    uint8_t rows[7];
} LATIN_MARKS[] = {
    {'/', {1,2,2,4,8,8,16}},  {'-', {0,0,0,31,0,0,0}},
    {':', {0,4,4,0,4,4,0}},   {'.', {0,0,0,0,0,6,6}},
    {',', {0,0,0,0,6,2,4}},   {'!', {4,4,4,4,4,0,4}},
    {'+', {0,4,4,31,4,4,0}},  {'%', {24,25,2,4,8,19,3}},
    {'\'', {4,4,8,0,0,0,0}},  {'(', {2,4,8,8,8,4,2}},
    {')', {8,4,2,2,2,4,8}},   {'?', {14,17,1,2,4,0,4}},
    {'<', {2,4,8,16,8,4,2}},  {'>', {8,4,2,1,2,4,8}},
};

static const uint8_t *latin_glyph(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return LATIN_DIGITS[ch - '0'];
    }
    if (ch >= 'A' && ch <= 'Z') {
        return LATIN_LETTERS[ch - 'A'];
    }
    if (ch >= 'a' && ch <= 'z') {
        return LATIN_LETTERS[ch - 'a'];
    }
    for (size_t i = 0; i < sizeof(LATIN_MARKS) / sizeof(LATIN_MARKS[0]); ++i) {
        if (LATIN_MARKS[i].ch == ch) {
            return LATIN_MARKS[i].rows;
        }
    }
    return NULL;
}

/* Pixel scale of the Latin face per font size: 7, 14 and 21 px tall. */
static int latin_scale(fl_font_t font)
{
    return font == FL_FONT_HUGE ? 3 : font == FL_FONT_LARGE ? 2 : 1;
}

/* ------------------------------------------------------------------------
 * The pixel fonts
 * --------------------------------------------------------------------- */

typedef struct {
    int count;
    int height;
    const uint16_t *codes;
    const uint8_t *advance;
    const uint16_t *rows;   /* count x height */
} pixel_font_t;

static const pixel_font_t BODY_FONT = {
    FL_FONT_BODY_COUNT, FL_FONT_BODY_HEIGHT, FL_FONT_BODY_CODES,
    FL_FONT_BODY_ADVANCE, &FL_FONT_BODY_ROWS[0][0],
};

static const pixel_font_t TITLE_FONT = {
    FL_FONT_TITLE_COUNT, FL_FONT_TITLE_HEIGHT, FL_FONT_TITLE_CODES,
    FL_FONT_TITLE_ADVANCE, &FL_FONT_TITLE_ROWS[0][0],
};

/* Small text uses the 12px font; titles the 16px one, doubled for
 * headlines. */
static const pixel_font_t *pixel_font(fl_font_t font)
{
    return font == FL_FONT_SMALL ? &BODY_FONT : &TITLE_FONT;
}

static int pixel_scale(fl_font_t font)
{
    return font == FL_FONT_HUGE ? 2 : 1;
}

static int find_glyph(const pixel_font_t *font, uint32_t codepoint)
{
    int lo = 0;
    int hi = font->count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        uint32_t code = font->codes[mid];
        if (code == codepoint) {
            return mid;
        }
        if (code < codepoint) {
            lo = mid + 1;
        } else {
            hi = mid - 1;
        }
    }
    return -1;
}

/* Decode one UTF-8 character; advances *text. Bad bytes read as '?'. */
static uint32_t next_codepoint(const char **text)
{
    const uint8_t *s = (const uint8_t *)*text;
    uint32_t cp = s[0];
    int extra = 0;
    if (cp >= 0xF0) {
        cp &= 0x07;
        extra = 3;
    } else if (cp >= 0xE0) {
        cp &= 0x0F;
        extra = 2;
    } else if (cp >= 0xC0) {
        cp &= 0x1F;
        extra = 1;
    } else if (cp >= 0x80) {
        *text += 1;
        return '?';
    }
    for (int i = 1; i <= extra; ++i) {
        if ((s[i] & 0xC0) != 0x80) {
            *text += i;
            return '?';
        }
        cp = (cp << 6) | (s[i] & 0x3F);
    }
    *text += 1 + extra;
    return cp;
}

/* English mode draws ASCII in the Latin face; anything else (and all of
 * Chinese mode) uses the pixel fonts. */
static bool uses_latin(uint32_t codepoint)
{
    return s_lang == FL_LANG_EN && codepoint < 0x80;
}

static int glyph_advance(uint32_t codepoint, fl_font_t font)
{
    if (uses_latin(codepoint)) {
        return 6 * latin_scale(font);
    }
    const pixel_font_t *pf = pixel_font(font);
    int index = find_glyph(pf, codepoint);
    int advance = index >= 0 ? pf->advance[index] : pf->height / 2;
    return advance * pixel_scale(font);
}

int fl_text_height(fl_font_t font)
{
    if (s_lang == FL_LANG_EN) {
        return 7 * latin_scale(font);
    }
    return pixel_font(font)->height * pixel_scale(font);
}

int fl_text_width(const char *text, fl_font_t font)
{
    if (!text || !*text) {
        return 0;
    }
    int width = 0;
    bool last_latin = false;
    while (*text) {
        uint32_t cp = next_codepoint(&text);
        width += glyph_advance(cp, font);
        last_latin = uses_latin(cp);
    }
    /* The Latin face's advance includes a trailing gap column. */
    return last_latin ? width - latin_scale(font) : width;
}

static void draw_latin(const uint8_t *rows, int x, int y, int scale,
                       Color color)
{
    for (int yy = 0; yy < 7; ++yy) {
        int xx = 0;
        while (xx < 5) {
            while (xx < 5 && !(rows[yy] & (1U << (4 - xx)))) {
                ++xx;
            }
            int start = xx;
            while (xx < 5 && (rows[yy] & (1U << (4 - xx)))) {
                ++xx;
            }
            if (start < xx) {
                DrawRectangle(x + start * scale, y + yy * scale,
                              (xx - start) * scale, scale, color);
            }
        }
    }
}

static void draw_pixel_glyph(const pixel_font_t *pf, int index, int x, int y,
                             int scale, Color color)
{
    const uint16_t *rows = pf->rows + index * pf->height;
    for (int yy = 0; yy < pf->height; ++yy) {
        uint16_t bits = rows[yy];
        int xx = 0;
        while (bits && xx < 16) {
            while (xx < 16 && !(bits & (0x8000U >> xx))) {
                ++xx;
            }
            int start = xx;
            while (xx < 16 && (bits & (0x8000U >> xx))) {
                bits &= (uint16_t)~(0x8000U >> xx);
                ++xx;
            }
            if (start < xx) {
                DrawRectangle(x + start * scale, y + yy * scale,
                              (xx - start) * scale, scale, color);
            }
        }
    }
}

/* Draw one character with its top at `y`; returns its advance. Latin
 * glyphs sit centred on a pixel-font line when the two faces mix. */
static int draw_codepoint(uint32_t cp, int x, int y, fl_font_t font,
                          Color color)
{
    if (uses_latin(cp)) {
        const uint8_t *rows = latin_glyph((char)cp);
        if (rows) {
            draw_latin(rows, x, y, latin_scale(font), color);
        }
        return 6 * latin_scale(font);
    }
    const pixel_font_t *pf = pixel_font(font);
    int scale = pixel_scale(font);
    int index = find_glyph(pf, cp);
    if (index < 0) {
        return pf->height / 2 * scale;
    }
    int top = y;
    if (s_lang == FL_LANG_EN) {
        /* A Chinese word inside English text: centre it on the Latin line. */
        top = y + (7 * latin_scale(font) - pf->height * scale) / 2;
    }
    draw_pixel_glyph(pf, index, x, top, scale, color);
    return pf->advance[index] * scale;
}

void fl_text_draw(const char *text, int x, int y, fl_font_t font, Color color)
{
    if (!text) {
        return;
    }
    while (*text) {
        uint32_t cp = next_codepoint(&text);
        x += draw_codepoint(cp, x, y, font, color);
    }
}

void fl_text_draw_centered(const char *text, int cx, int y, fl_font_t font,
                           Color color)
{
    fl_text_draw(text, cx - fl_text_width(text, font) / 2, y, font, color);
}

/* ------------------------------------------------------------------------
 * Wrapping
 * --------------------------------------------------------------------- */

/* Chinese punctuation that must not start a line. */
static bool no_line_start(uint32_t cp)
{
    switch (cp) {
    case 0xFF0C: case 0x3002: case 0xFF01: case 0xFF1A: case 0xFF1B:
    case 0x3001: case 0xFF09: case 0xFF1F: case ',': case '.': case '!':
    case ':':
        return true;
    default:
        return false;
    }
}

/* Draw `len` bytes of `text` as one line. */
static void draw_span(const char *text, size_t len, int x, int y,
                      fl_font_t font, Color color)
{
    const char *end = text + len;
    while (text < end && *text) {
        uint32_t cp = next_codepoint(&text);
        x += draw_codepoint(cp, x, y, font, color);
    }
}

int fl_text_draw_wrapped(const char *text, int x, int y, int width,
                         fl_font_t font, int line_gap, Color color, bool draw)
{
    if (!text || !*text) {
        return 0;
    }
    int lines = 0;
    int step = fl_text_height(font) + line_gap;
    const char *line = text;
    while (*line) {
        while (*line == ' ') {
            ++line;
        }
        if (!*line) {
            break;
        }
        /* Walk forward until the line overflows, remembering the last place
         * it could break: after a space, or before any Chinese character. */
        const char *cursor = line;
        const char *last_break = NULL;
        int used = 0;
        while (*cursor) {
            const char *before = cursor;
            uint32_t cp = next_codepoint(&cursor);
            if (cp >= 0x80 && before != line && !no_line_start(cp)) {
                last_break = before;
            }
            int advance = glyph_advance(cp, font);
            if (used + advance > width && before != line) {
                cursor = before;
                break;
            }
            used += advance;
            if (cp == ' ') {
                last_break = cursor;
            }
        }
        const char *end = cursor;
        if (*cursor && last_break && last_break > line) {
            end = last_break;
        }
        if (draw) {
            size_t len = (size_t)(end - line);
            while (len > 0 && line[len - 1] == ' ') {
                --len;
            }
            draw_span(line, len, x, y + lines * step, font, color);
        }
        ++lines;
        line = end;
    }
    return lines;
}

/* ------------------------------------------------------------------------
 * Raw glyphs
 * --------------------------------------------------------------------- */

bool fl_text_title_glyph(uint32_t codepoint, const uint16_t **rows,
                         int *height, int *advance)
{
    int index = find_glyph(&TITLE_FONT, codepoint);
    if (index < 0) {
        return false;
    }
    *rows = TITLE_FONT.rows + index * TITLE_FONT.height;
    *height = TITLE_FONT.height;
    *advance = TITLE_FONT.advance[index];
    return true;
}

const uint8_t *fl_text_latin_glyph(char ch)
{
    return latin_glyph(ch);
}

uint32_t fl_text_next_codepoint(const char **text)
{
    return next_codepoint(text);
}

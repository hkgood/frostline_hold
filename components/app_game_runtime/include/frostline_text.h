#ifndef FROSTLINE_TEXT_H
#define FROSTLINE_TEXT_H

/* Bilingual text for Frostline Hold.
 *
 * Strings live in the generated frostline_text_data.h (source of truth:
 * assets_src/artgen/strings.py). English is drawn with a 5x7 capital face
 * matching the engine's, Chinese with pixel fonts whose glyphs are compiled
 * in. Everything is drawn as horizontal runs of rectangles, the way the
 * engine draws its own text, so no texture is needed. */

#include <stdbool.h>
#include <stdint.h>

#include "raylib_lite_raylib.h"
#include "frostline_text_data.h"

#define FL_LANG_ZH 0
#define FL_LANG_EN 1

/* Three sizes: body text, titles and buttons, and headlines. */
typedef enum {
    FL_FONT_SMALL = 0,
    FL_FONT_LARGE,
    FL_FONT_HUGE,
} fl_font_t;

void fl_text_set_language(uint8_t lang);
uint8_t fl_text_language(void);

/* The string for `id` in the current language. */
const char *fl_tr(fl_text_id_t id);

/* printf into one of a few rotating buffers: valid until four more calls. */
const char *fl_trf(fl_text_id_t id, ...);
const char *fl_fmt(const char *format, ...);

/* Line height and width in pixels of `text` drawn in `font`. */
int fl_text_height(fl_font_t font);
int fl_text_width(const char *text, fl_font_t font);

void fl_text_draw(const char *text, int x, int y, fl_font_t font, Color color);
void fl_text_draw_centered(const char *text, int cx, int y, fl_font_t font,
                           Color color);

/* Raw glyphs, for drawing lettering larger than the fonts go: the logo.
 * A title-font glyph is `height` rows of 16 bits, bit 15 the leftmost
 * pixel; a Latin glyph is 7 rows of 5 bits, bit 4 the leftmost. */
bool fl_text_title_glyph(uint32_t codepoint, const uint16_t **rows,
                         int *height, int *advance);
const uint8_t *fl_text_latin_glyph(char ch);
/* Decode one UTF-8 character and step past it. */
uint32_t fl_text_next_codepoint(const char **text);

/* Wraps at `width` (between words in English, between characters in
 * Chinese) and returns the number of lines drawn. With `draw` false it only
 * counts them. */
int fl_text_draw_wrapped(const char *text, int x, int y, int width,
                         fl_font_t font, int line_gap, Color color, bool draw);

#endif

/* Rendering layer for Frostline Hold.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#pragma once
#include <stdbool.h>

#include "../../app_game/include/frostline_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/* The panel's only two tones: warm ink and paper. The sprite sheets are baked
 * in the same pair, so every pixel on screen is one or the other. */
#define FL_INK ((Color){0x31, 0x2F, 0x28, 255})
#define FL_PAPER ((Color){0xB1, 0xAF, 0xA8, 255})

/** Load every atlas. Safe to call once per run. */
void frostline_view_init(void);
/** Release the atlases. */
void frostline_view_shutdown(void);
/** True when every atlas loaded and the scene can be drawn. */
bool frostline_view_ready(void);
/** Draw one full frame of the current game state. */
void frostline_view_draw(const fl_state_t *model);
/** The enemies already introduced, one bit per kind. The view shows a card
 * for every kind the first time it walks on; the runtime keeps the mask in
 * the record. */
void frostline_view_set_seen_enemies(uint32_t mask);
/** Where the finger is, so the button under it is drawn pressed in. */
void frostline_view_set_pointer(int x, int y, bool down);
uint32_t frostline_view_seen_enemies(void);

#ifdef __cplusplus
}
#endif

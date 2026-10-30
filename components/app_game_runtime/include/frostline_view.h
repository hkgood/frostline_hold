/* Rendering layer for Frostline Hold.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#pragma once
#include <stdbool.h>

#include "../../app_game/include/frostline_model.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Load every atlas. Safe to call once per run. */
void frostline_view_init(void);
/** Release the atlases. */
void frostline_view_shutdown(void);
/** True when every atlas loaded and the scene can be drawn. */
bool frostline_view_ready(void);
/** Draw one full frame of the current game state. */
void frostline_view_draw(const fl_state_t *model);

#ifdef __cplusplus
}
#endif

/* Steam, when the build links Steamworks (FROSTLINE_STEAM); otherwise these
 * do nothing, so the rest of the shell never asks which build it is.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PC_STEAM_OK,      /* running, under Steam or without it */
    PC_STEAM_RESTART, /* Steam is relaunching the game: exit now */
    PC_STEAM_FAILED,  /* a Steam build that could not reach Steam */
} pc_steam_status_t;

pc_steam_status_t pc_steam_init(void);
void pc_steam_update(void);
void pc_steam_shutdown(void);

#ifdef __cplusplus
}
#endif

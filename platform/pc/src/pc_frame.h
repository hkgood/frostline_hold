/* The framed presentation (FROSTLINE_FRAME, the Android build): the board in
 * an engraved border on a night sky, snowy peaks along the bottom, filling
 * whatever screen it is on. Without it these do nothing and the shell
 * letterboxes the board in plain ink.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#pragma once

#include <stdbool.h>

#include <SDL3/SDL.h>

/* Loads the frame's art from asset_root/frame; false when the build has no
 * frame or the art is missing, and the shell then letterboxes. */
bool pc_frame_init(SDL_Renderer *renderer, const char *asset_root,
                   int board_width, int board_height);

/* Draws the backdrop, the border and the board texture over the whole
 * output, keeping the board inside the window's safe area. */
void pc_frame_render(SDL_Renderer *renderer, SDL_Window *window, SDL_Texture *board);

/* Render coordinates (output pixels) to board pixels, by the last layout. */
void pc_frame_to_board(float *x, float *y);

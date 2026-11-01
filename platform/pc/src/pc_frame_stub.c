/* The frame when the build has none: the shell letterboxes instead.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "pc_frame.h"

bool pc_frame_init(SDL_Renderer *renderer, const char *asset_root,
                   int board_width, int board_height)
{
    (void)renderer;
    (void)asset_root;
    (void)board_width;
    (void)board_height;
    return false;
}

void pc_frame_render(SDL_Renderer *renderer, SDL_Window *window, SDL_Texture *board)
{
    (void)renderer;
    (void)window;
    (void)board;
}

void pc_frame_to_board(float *x, float *y)
{
    (void)x;
    (void)y;
}

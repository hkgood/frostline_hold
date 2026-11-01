/* The framed presentation: see pc_frame.h. The art comes from
 * assets_src/artgen/generate_frame.py, one image pixel per game pixel, and
 * is scaled with the board so every dot on screen is the same size.
 *
 *   sky.png     pinned to the top of the screen, centred on the board
 *   ground.png  pinned to the bottom, centred on the board
 *   border.png  round the board, PC_FRAME_MARGIN game pixels each side
 *
 * Whatever the art does not reach stays the ink the renderer was cleared to.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "pc_frame.h"

#include <stdio.h>

/* generate_frame.py's MARGIN: the border's reach beyond the board. */
#define PC_FRAME_MARGIN 16

static struct {
    SDL_Texture *sky;
    SDL_Texture *ground;
    SDL_Texture *border;
    int board_width;
    int board_height;
    /* The last layout, in output pixels. */
    SDL_FRect board;
    float scale;
} s_frame;

static SDL_Texture *load_art(SDL_Renderer *renderer, const char *root, const char *name)
{
    char path[1100];
    snprintf(path, sizeof(path), "%s/frame/%s", root, name);
    SDL_Surface *surface = SDL_LoadPNG(path);
    if (!surface) {
        SDL_Log("frame: %s: %s", path, SDL_GetError());
        return NULL;
    }
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (texture) {
        SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_PIXELART);
    }
    return texture;
}

bool pc_frame_init(SDL_Renderer *renderer, const char *asset_root,
                   int board_width, int board_height)
{
    s_frame.sky = load_art(renderer, asset_root, "sky.png");
    s_frame.ground = load_art(renderer, asset_root, "ground.png");
    s_frame.border = load_art(renderer, asset_root, "border.png");
    s_frame.board_width = board_width;
    s_frame.board_height = board_height;
    return s_frame.sky && s_frame.ground && s_frame.border;
}

/* The safe area (clear of notches and rounded corners) in output pixels. */
static SDL_FRect safe_area(SDL_Window *window, int output_w, int output_h)
{
    SDL_Rect safe;
    int window_w = 0;
    SDL_GetWindowSize(window, &window_w, NULL);
    if (!SDL_GetWindowSafeArea(window, &safe) || window_w <= 0) {
        return (SDL_FRect){0.0f, 0.0f, (float)output_w, (float)output_h};
    }
    float density = (float)output_w / (float)window_w;
    return (SDL_FRect){safe.x * density, safe.y * density,
                       safe.w * density, safe.h * density};
}

/* The largest board, with its border, that fits the safe area, centred in
 * it and placed on whole pixels. */
static void layout(SDL_Window *window, int output_w, int output_h)
{
    SDL_FRect safe = safe_area(window, output_w, output_h);
    float span_w = (float)(s_frame.board_width + PC_FRAME_MARGIN * 2);
    float span_h = (float)(s_frame.board_height + PC_FRAME_MARGIN * 2);
    float scale = SDL_min(safe.w / span_w, safe.h / span_h);
    float board_w = SDL_floorf(s_frame.board_width * scale);
    scale = board_w / (float)s_frame.board_width;
    float board_h = SDL_floorf(s_frame.board_height * scale);
    s_frame.scale = scale;
    s_frame.board = (SDL_FRect){
        SDL_roundf(safe.x + (safe.w - board_w) * 0.5f),
        SDL_roundf(safe.y + (safe.h - board_h) * 0.5f),
        board_w, board_h,
    };
}

/* A texture at the frame's scale, centred on the board across, pinned to
 * the top or the bottom of the output. */
static void draw_pinned(SDL_Renderer *renderer, SDL_Texture *texture,
                        bool at_bottom, int output_h)
{
    float w = 0.0f;
    float h = 0.0f;
    SDL_GetTextureSize(texture, &w, &h);
    w *= s_frame.scale;
    h *= s_frame.scale;
    float centre = s_frame.board.x + s_frame.board.w * 0.5f;
    SDL_FRect dst = {SDL_roundf(centre - w * 0.5f),
                     at_bottom ? (float)output_h - h : 0.0f, w, h};
    SDL_RenderTexture(renderer, texture, NULL, &dst);
}

void pc_frame_render(SDL_Renderer *renderer, SDL_Window *window, SDL_Texture *board)
{
    int output_w = 0;
    int output_h = 0;
    if (!SDL_GetCurrentRenderOutputSize(renderer, &output_w, &output_h) ||
        output_w <= 0 || output_h <= 0) {
        return;
    }
    layout(window, output_w, output_h);

    draw_pinned(renderer, s_frame.sky, false, output_h);
    draw_pinned(renderer, s_frame.ground, true, output_h);
    float reach = PC_FRAME_MARGIN * s_frame.scale;
    SDL_FRect border = {s_frame.board.x - reach, s_frame.board.y - reach,
                        s_frame.board.w + reach * 2.0f, s_frame.board.h + reach * 2.0f};
    SDL_RenderTexture(renderer, s_frame.border, NULL, &border);
    SDL_RenderTexture(renderer, board, NULL, &s_frame.board);
}

void pc_frame_to_board(float *x, float *y)
{
    if (s_frame.scale <= 0.0f) {
        return;
    }
    *x = (*x - s_frame.board.x) / s_frame.scale;
    *y = (*y - s_frame.board.y) / s_frame.scale;
}

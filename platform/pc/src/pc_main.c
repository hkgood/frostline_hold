/* Frostline Hold on a PC: macOS, Windows, Linux and the browser; and on
 * Android, through the same shell.
 *
 * The game is the same module the device and the simulator run; the Engine
 * renders it into a 480x480 RGB565 frame. This shell owns everything around
 * that frame: the window, the scaling (letterboxed, or framed by pc_frame.c),
 * the mouse and touch, the clock, the player's data folder, and Steam when
 * the build links it.
 *
 * SDL's callback entry points drive the loop, so the browser (which owns its
 * own frame loop) and the desktop share one code path.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <stdio.h>
#include <stdlib.h>

#include "raylib_lite_host_game.h"
#include "raylib_lite_game_audio.h"
#include "../../../components/app_game_runtime/include/frostline_record.h"
#include "pc_frame.h"
#include "pc_steam.h"

#define PC_TITLE "Frostline Hold"
#define PC_ORG "hkgood"
#define PC_SAVE_FILE "frostline_record.bin"
/* The letterbox around the square board: the game's ink, FL_INK, as it
 * shows after RGB565 so the bars meet the board without a seam. */
#define PC_INK_R 0x31
#define PC_INK_G 0x2C
#define PC_INK_B 0x29
/* After a stall (a dragged window, a breakpoint) the game resumes rather
 * than racing through the missed ticks. */
#define PC_MAX_CATCH_UP_TICKS 4
/* --smoke-test FILE.bmp: run this many ticks, save the frame, quit. */
#define PC_SMOKE_TICKS 90

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    void *game;
    uint16_t *pixels;
    int width;
    int height;
    Uint64 tick_ns;
    Uint64 last_ns;
    Uint64 lag_ns;
    bool vsync;
    bool framed;
    bool pointer_down;
    bool minimized;
    const char *smoke_path;
    unsigned ticks;
} pc_app_t;

/* Where the packed assets are: FROSTLINE_ASSETS when set (development),
 * else the assets folder SDL reports beside the program (Resources inside a
 * macOS bundle). On Android a relative path names a folder inside the APK. */
static void find_asset_root(char *out, size_t capacity)
{
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    snprintf(out, capacity, "/assets");
#elif defined(SDL_PLATFORM_ANDROID)
    snprintf(out, capacity, "assets");
#else
    const char *override = SDL_getenv("FROSTLINE_ASSETS");
    if (override && *override) {
        snprintf(out, capacity, "%s", override);
        return;
    }
    const char *base = SDL_GetBasePath();
    snprintf(out, capacity, "%sassets", base ? base : "./");
#endif
}

/* The record lives in the player's data folder; on the web, in the folder
 * the page mounts on IndexedDB (web/frostline_pre.js). */
static void set_save_path(void)
{
    char path[1024];
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    snprintf(path, sizeof(path), "/save/%s", PC_SAVE_FILE);
#else
    char *folder = SDL_GetPrefPath(PC_ORG, PC_TITLE);
    if (!folder) {
        SDL_Log("no data folder (%s); progress will not be kept", SDL_GetError());
        return;
    }
    snprintf(path, sizeof(path), "%s%s", folder, PC_SAVE_FILE);
    SDL_free(folder);
#endif
    frostline_record_set_host_path(path);
}

/* A window that fills most of the screen's height, square like the board. */
static int initial_window_size(void)
{
    int size = 720;
#if !defined(SDL_PLATFORM_EMSCRIPTEN)
    SDL_Rect usable;
    if (SDL_GetDisplayUsableBounds(SDL_GetPrimaryDisplay(), &usable)) {
        size = usable.h * 4 / 5;
    }
#endif
    return SDL_clamp(size, 480, 1440);
}

static void send_pointer(pc_app_t *app, float x, float y, bool pressed)
{
    raylib_lite_host_input_v1_t input = {
        .type = RAYLIB_LITE_HOST_INPUT_POINTER,
        .x = (int32_t)SDL_clamp(x, 0.0f, (float)(app->width - 1)),
        .y = (int32_t)SDL_clamp(y, 0.0f, (float)(app->height - 1)),
        .track_id = 0,
        .pressed = pressed,
    };
    raylib_lite_host_game_input_v1(app->game, &input);
}

static bool inside_board(const pc_app_t *app, float x, float y)
{
    return x >= 0.0f && y >= 0.0f && x < (float)app->width && y < (float)app->height;
}

static void toggle_fullscreen(pc_app_t *app)
{
#if !defined(SDL_PLATFORM_EMSCRIPTEN)
    bool full = (SDL_GetWindowFlags(app->window) & SDL_WINDOW_FULLSCREEN) != 0;
    SDL_SetWindowFullscreen(app->window, !full);
#else
    (void)app;
#endif
}

/* Saves the current frame for --smoke-test; true when it was written. */
static bool save_frame(const pc_app_t *app)
{
    SDL_Surface *surface = SDL_CreateSurfaceFrom(app->width, app->height,
                                                 SDL_PIXELFORMAT_RGB565, app->pixels,
                                                 app->width * (int)sizeof(uint16_t));
    bool saved = surface && SDL_SaveBMP(surface, app->smoke_path);
    SDL_DestroySurface(surface);
    if (!saved) {
        SDL_Log("smoke test: cannot write %s (%s)", app->smoke_path, SDL_GetError());
    }
    return saved;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    const char *smoke_path = NULL;
    for (int i = 1; i + 1 < argc; ++i) {
        if (SDL_strcmp(argv[i], "--smoke-test") == 0) {
            smoke_path = argv[i + 1];
        }
    }
    /* A smoke test runs outside Steam, in CI or from a terminal. */
    switch (smoke_path ? PC_STEAM_OK : pc_steam_init()) {
    case PC_STEAM_RESTART: return SDL_APP_SUCCESS;
    case PC_STEAM_FAILED: return SDL_APP_FAILURE;
    case PC_STEAM_OK: break;
    }

    SDL_SetAppMetadata(PC_TITLE, FROSTLINE_VERSION, "com.hkgood.frostlinehold");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        SDL_Log("SDL_Init: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    pc_app_t *app = SDL_calloc(1, sizeof(*app));
    if (!app) {
        return SDL_APP_FAILURE;
    }
    *appstate = app;
    app->smoke_path = smoke_path;

    const raylib_lite_host_game_descriptor_v1_t *game = raylib_lite_host_game_v1();
    if (!game || game->abi_version != RAYLIB_LITE_HOST_GAME_ABI_V1 || !game->tick_hz) {
        SDL_Log("the game module does not match this shell");
        return SDL_APP_FAILURE;
    }
    app->width = game->width;
    app->height = game->height;
    app->tick_ns = SDL_NS_PER_SECOND / game->tick_hz;

    int size = initial_window_size();
    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#if defined(SDL_PLATFORM_EMSCRIPTEN)
    /* The canvas fills the page (itch's frame, or its fullscreen view). */
    flags |= SDL_WINDOW_FILL_DOCUMENT;
#elif defined(SDL_PLATFORM_ANDROID)
    /* Held upright in one hand, the system bars hidden. */
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "Portrait");
    flags |= SDL_WINDOW_FULLSCREEN;
#endif
    if (smoke_path) {
        flags |= SDL_WINDOW_HIDDEN;
    }
    if (!SDL_CreateWindowAndRenderer(PC_TITLE, size, size, flags,
                                     &app->window, &app->renderer)) {
        SDL_Log("window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    SDL_SetWindowMinimumSize(app->window, app->width, app->height);
    app->vsync = SDL_SetRenderVSync(app->renderer, 1);
    char assets[1024];
    find_asset_root(assets, sizeof(assets));
    app->framed = pc_frame_init(app->renderer, assets, app->width, app->height);
    if (!app->framed) {
        SDL_SetRenderLogicalPresentation(app->renderer, app->width, app->height,
                                         SDL_LOGICAL_PRESENTATION_LETTERBOX);
    }
    app->texture = SDL_CreateTexture(app->renderer, SDL_PIXELFORMAT_RGB565,
                                     SDL_TEXTUREACCESS_STREAMING, app->width, app->height);
    app->pixels = SDL_calloc((size_t)app->width * (size_t)app->height, sizeof(uint16_t));
    if (!app->texture || !app->pixels) {
        SDL_Log("frame: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }
    /* Crisp at any scale, without the uneven pixels of plain nearest. */
    SDL_SetTextureScaleMode(app->texture, SDL_SCALEMODE_PIXELART);

    set_save_path();
    app->game = raylib_lite_host_game_create_v1(assets);
    if (!app->game) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, PC_TITLE,
                                 "The game data could not be loaded.", app->window);
        return SDL_APP_FAILURE;
    }
    app->last_ns = SDL_GetTicksNS();
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    pc_app_t *app = appstate;
    switch (event->type) {
    case SDL_EVENT_QUIT:
        return SDL_APP_SUCCESS;

    case SDL_EVENT_WINDOW_MINIMIZED:
        app->minimized = true;
        break;
    case SDL_EVENT_WINDOW_RESTORED:
    case SDL_EVENT_WINDOW_MAXIMIZED:
        app->minimized = false;
        app->last_ns = SDL_GetTicksNS();
        break;

    case SDL_EVENT_KEY_DOWN:
        if (event->key.repeat) {
            break;
        }
        if (event->key.key == SDLK_F11 ||
            (event->key.key == SDLK_RETURN && (event->key.mod & SDL_KMOD_ALT)) ||
            (event->key.key == SDLK_F && (event->key.mod & SDL_KMOD_GUI) &&
             (event->key.mod & SDL_KMOD_CTRL))) {
            toggle_fullscreen(app);
        }
        break;

    /* Touch arrives as these too: SDL turns the first finger into the mouse. */
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
    case SDL_EVENT_MOUSE_MOTION:
        if (!app->game) {
            break;
        }
        SDL_ConvertEventToRenderCoordinates(app->renderer, event);
        if (event->type == SDL_EVENT_MOUSE_MOTION) {
            if (app->pointer_down) {
                float x = event->motion.x;
                float y = event->motion.y;
                if (app->framed) {
                    pc_frame_to_board(&x, &y);
                }
                send_pointer(app, x, y, true);
            }
        } else if (event->button.button == SDL_BUTTON_LEFT) {
            float x = event->button.x;
            float y = event->button.y;
            if (app->framed) {
                pc_frame_to_board(&x, &y);
            }
            if (event->type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                /* A press on the letterbox or the frame is not a press on the board. */
                if (inside_board(app, x, y)) {
                    app->pointer_down = true;
                    send_pointer(app, x, y, true);
                }
            } else if (app->pointer_down) {
                app->pointer_down = false;
                send_pointer(app, x, y, false);
            }
        }
        break;

    default:
        break;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    pc_app_t *app = appstate;
    pc_steam_update();

    Uint64 now = SDL_GetTicksNS();
    app->lag_ns += now - app->last_ns;
    app->last_ns = now;
    if (app->minimized) {
        app->lag_ns = 0;
        SDL_Delay(50);
        return SDL_APP_CONTINUE;
    }
    if (app->lag_ns > app->tick_ns * PC_MAX_CATCH_UP_TICKS) {
        app->lag_ns = app->tick_ns;
    }

    bool advanced = false;
    while (app->lag_ns >= app->tick_ns) {
        raylib_lite_host_game_update_v1(app->game);
        app->lag_ns -= app->tick_ns;
        app->ticks++;
        advanced = true;
    }
    if (advanced &&
        raylib_lite_host_game_render_rgb565_v1(app->game, app->pixels,
                                               (size_t)app->width) == 0) {
        SDL_UpdateTexture(app->texture, NULL, app->pixels,
                          app->width * (int)sizeof(uint16_t));
    }
    if (app->smoke_path && app->ticks >= PC_SMOKE_TICKS) {
        SDL_Log("smoke test: %u ticks, audio %s", app->ticks,
                raylib_lite_game_audio_ready() ? "ready" : "silent");
        return save_frame(app) ? SDL_APP_SUCCESS : SDL_APP_FAILURE;
    }

    SDL_SetRenderDrawColor(app->renderer, PC_INK_R, PC_INK_G, PC_INK_B, 255);
    SDL_RenderClear(app->renderer);
    if (app->framed) {
        pc_frame_render(app->renderer, app->window, app->texture);
    } else {
        SDL_RenderTexture(app->renderer, app->texture, NULL, NULL);
    }
    SDL_RenderPresent(app->renderer);

    /* Without vsync the present returns at once; sleep to the next tick. */
    if (!app->vsync) {
        Uint64 spent = SDL_GetTicksNS() - app->last_ns;
        Uint64 remaining = app->tick_ns - app->lag_ns;
        if (remaining > spent) {
            SDL_DelayNS(remaining - spent);
        }
    }
    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    (void)result;
    pc_app_t *app = appstate;
    if (app) {
        if (app->game) {
            raylib_lite_host_game_destroy_v1(app->game);
        }
        SDL_free(app->pixels);
        SDL_free(app);
    }
    pc_steam_shutdown();
}

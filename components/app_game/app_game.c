// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
#include <stdlib.h>
#include <string.h>
#include "include/app_game.h"

struct app_game {
    app_game_config_t config;
    app_game_snapshot_t state;
    fl_state_t model;
};

app_game_err_t app_game_create(const app_game_config_t *config, app_game_handle_t *ret_handle)
{
    if (!ret_handle) {
        return APP_GAME_INVALID_ARGUMENT;
    }
    *ret_handle = NULL;
    if (!config || !config->width || !config->height) {
        return APP_GAME_INVALID_ARGUMENT;
    }
    app_game_handle_t game = calloc(1, sizeof(*game));
    if (!game) {
        return APP_GAME_NO_MEMORY;
    }
    game->config = *config;
    fl_state_reset(&game->model, 0x464C5354u);
    fl_state_open_title(&game->model);
    *ret_handle = game;
    return APP_GAME_OK;
}

void app_game_destroy(app_game_handle_t *handle)
{
    if (handle) {
        free(*handle);
        *handle = NULL;
    }
}

void app_game_reset(app_game_handle_t game)
{
    if (game) {
        game->state = (app_game_snapshot_t) {
            0
        };
        /* A reset starts over from the title but keeps what was won and
         * what was set. */
        uint8_t stars[FL_MAX_LEVELS];
        uint32_t best[FL_MAX_LEVELS];
        memcpy(stars, game->model.level_stars, sizeof(stars));
        memcpy(best, game->model.level_best, sizeof(best));
        fl_settings_t settings = game->model.settings;
        fl_state_reset(&game->model, 0x464C5354u);
        app_game_set_progress(game, stars, best, FL_MAX_LEVELS);
        game->model.settings = settings;
        fl_state_open_title(&game->model);
    }
}

void app_game_set_progress(app_game_handle_t game, const uint8_t *stars,
                           const uint32_t *best, int count)
{
    if (!game || !stars || !best) {
        return;
    }
    for (int i = 0; i < count && i < FL_MAX_LEVELS; ++i) {
        game->model.level_stars[i] = stars[i] > 3 ? 3 : stars[i];
        game->model.level_best[i] = best[i];
    }
    if (game->model.phase == FL_PHASE_MENU) {
        fl_state_open_menu(&game->model);   /* re-aim at the newest level */
    }
}

void app_game_set_settings(app_game_handle_t game, const fl_settings_t *settings)
{
    if (!game || !settings) {
        return;
    }
    fl_settings_t s = *settings;
    s.music = s.music > FL_VOLUME_MAX ? FL_VOLUME_MAX : s.music;
    s.sound = s.sound > FL_VOLUME_MAX ? FL_VOLUME_MAX : s.sound;
    s.haptics = s.haptics ? 1 : 0;
    s.lang = s.lang ? 1 : 0;
    game->model.settings = s;
}

void app_game_set_paused(app_game_handle_t game, bool paused)
{
    if (game) {
        game->state.paused = paused;
        game->model.paused = paused;
    }
}

void app_game_set_pointer(app_game_handle_t game, int32_t x, int32_t y, bool pressed)
{
    if (!game) {
        return;
    }
    game->state.pointer_x = x < 0 ? 0 : (x >= game->config.width ? game->config.width - 1 : x);
    game->state.pointer_y = y < 0 ? 0 : (y >= game->config.height ? game->config.height - 1 : y);
    game->state.pointer_down = pressed;
    fl_set_pointer(&game->model, x, y, pressed);
}

void app_game_update(app_game_handle_t game)
{
    if (!game || game->state.paused) {
        return;
    }
    ++game->state.tick;
    fl_state_update(&game->model);
}

app_game_snapshot_t app_game_read(app_game_handle_t game)
{
    return game ? game->state : (app_game_snapshot_t) {
        0
    };
}

void app_game_get_model(app_game_handle_t handle, fl_state_t *out)
{
    if (!handle || !out) {
        return;
    }
    *out = handle->model;
}

uint32_t app_game_take_events(app_game_handle_t game)
{
    if (!game) {
        return 0;
    }
    return fl_events_take(&game->model);
}

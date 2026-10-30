// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
#include <stdio.h>
#include "include/app_game_runtime.h"
/* Engine Host accepts source lists, not component include paths. Reference only
 * the model's public header so these translation units work on both targets. */
#include "../app_game/include/app_game.h"
#include "include/frostline_view.h"
#include "include/frostline_record.h"
#include "include/frostline_audio.h"
#include "private/game_config.h"
#include "raylib_lite_raylib.h"
#if !defined(RAYLIB_LITE_GAME_NATIVE)
#include "host_asset_runtime.h"
#endif

typedef struct {
    app_game_handle_t game;
    frostline_record_t record;
    frostline_audio_t audio;
    bool recorded;   /* the finished run has already been written down */
} host_state_t;

static int initialize(void *value, const char *asset_root)
{
    host_state_t *state = value;
#if !defined(RAYLIB_LITE_GAME_NATIVE)
    /* The Host serves the packed atlases from the generated asset folder. */
    raylib_lite_host_assets_set_root(asset_root);
    char path[512];
    snprintf(path, sizeof(path), "%s/frostline_record.bin", asset_root);
    frostline_record_set_host_path(path);
#else
    (void)asset_root;
#endif
    const app_game_config_t config = {GAME_WIDTH, GAME_HEIGHT};
    if (app_game_create(&config, &state->game) != APP_GAME_OK) {
        return -1;
    }
    frostline_view_init();
    if (!frostline_view_ready()) {
        return -1;
    }
    frostline_record_load(&state->record);
    frostline_audio_init(&state->audio);
    InitWindow(GAME_WIDTH, GAME_HEIGHT, GAME_NAME);
    SetTargetFPS(GAME_TICK_HZ);
    return 0;
}

static void shutdown(void *value)
{
    host_state_t *state = value;
    frostline_audio_shutdown(&state->audio);
    frostline_view_shutdown();
    app_game_destroy(&state->game);
}

static void input(void *value, const raylib_lite_game_input_v1_t *event)
{
    host_state_t *state = value;
    if (!event) {
        return;
    }
    if (event->type == RAYLIB_LITE_GAME_INPUT_POINTER && event->track_id == 0) {
        app_game_set_pointer(state->game, event->x, event->y, event->pressed);
        if (event->pressed) {
            frostline_audio_tap(&state->audio);
        }
    } else if (event->type == RAYLIB_LITE_GAME_INPUT_CONTROL) {
        switch (event->code) {
        case RAYLIB_LITE_GAME_CONTROL_PAUSE: app_game_set_paused(state->game, true); break;
        case RAYLIB_LITE_GAME_CONTROL_RESUME: app_game_set_paused(state->game, false); break;
        case RAYLIB_LITE_GAME_CONTROL_RESET:
            app_game_reset(state->game);
            state->recorded = false;
            break;
        default: break;
        }
    } else if (event->type == RAYLIB_LITE_GAME_INPUT_ACTION && event->pressed) {
        if (event->code == GAME_ACTION_RESET) {
            app_game_reset(state->game);
            state->recorded = false;
        }
        if (event->code == GAME_ACTION_PAUSE) {
            app_game_set_paused(state->game, !app_game_read(state->game).paused);
        }
    }
}

static void update(void *value)
{
    host_state_t *state = value;
    app_game_update(state->game);
    /* What the simulation did this frame drives the sound and the motor. */
    fl_state_t model;
    app_game_get_model(state->game, &model);
    fl_audio_frame_t frame;
    frame.events = app_game_take_events(state->game);
    frame.tick = model.tick;
    frame.wave_active = (model.phase == FL_PHASE_WAVE) ? 1u : 0u;
    frame.run_over = (model.phase == FL_PHASE_WON || model.phase == FL_PHASE_LOST)
                     ? 1u : 0u;
    frame.kill_kind = model.last_kill_kind;
    frostline_audio_update(&state->audio, &frame);
    /* When a run ends, write it down if it beat the record. */
    if (model.phase != FL_PHASE_WON || state->recorded) {
        return;
    }
    state->recorded = true;
    int stars = fl_star_rating(&model);
    if (model.score > state->record.best_score) {
        state->record.best_score = model.score;
        state->record.best_wave = model.wave;
        state->record.best_stars = (uint8_t)stars;
        frostline_record_store(&state->record);
    } else if (stars > state->record.best_stars) {
        state->record.best_stars = (uint8_t)stars;
        frostline_record_store(&state->record);
    }
}

static int render(void *value)
{
    if (!((host_state_t *)value)->game) {
        return -1;
    }
    BeginDrawing();
    if (!raylib_lite_raylib_frame_available()) {
        EndDrawing();
        return -1;
    }
    ClearBackground(BLACK);
    host_state_t *state = value;
    fl_state_t model;
    app_game_get_model(state->game, &model);
    model.best_score = state->record.best_score;
    model.best_stars = state->record.best_stars;
    frostline_view_draw(&model);
    EndDrawing();
    return 0;
}

static uint32_t state_hash(const void *value)
{
    const app_game_snapshot_t state = app_game_read(((const host_state_t *)value)->game);
    /* Hash named fields, avoiding platform-dependent struct padding. */
    uint32_t hash = state.tick;
    hash = hash * 31U + (uint32_t)state.pointer_x;
    hash = hash * 31U + (uint32_t)state.pointer_y;
    hash = hash * 31U + state.pointer_down;
    return hash * 31U + state.paused;
}

static int state_json(const void *value, char *output, size_t capacity)
{
    if (!output || !capacity) {
        return -1;
    }
    fl_state_t model;
    const host_state_t *state = value;
    app_game_get_model(state->game, &model);
    int size = fl_state_json_sfx(&model, frostline_audio_name(&state->audio),
                                 output, capacity);
    if (size < 0) {
        return -1;
    }
    return size;
}

static const raylib_lite_game_module_v1_t s_module = {
    .descriptor = {
        RAYLIB_LITE_GAME_MODULE_ABI_V1, GAME_NAME, GAME_NAME,
        GAME_WIDTH, GAME_HEIGHT, GAME_TICK_HZ, 1
    },
    .state_size = sizeof(host_state_t),
    .initialize = initialize, .shutdown = shutdown, .input = input,
    .update = update, .render = render,
    .state_hash = state_hash, .state_json = state_json,
};

const raylib_lite_game_module_v1_t *app_game_runtime_module(void)
{
    return &s_module;
}

// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
#include <stdio.h>
#include <string.h>
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
    uint16_t recorded_run; /* run_serial of the last run written down */
    uint16_t settings_serial; /* the model's settings_serial last stored */
    uint16_t reset_serial;    /* the model's reset_serial last acted on */
    uint32_t seen_enemies;    /* the introductions last stored */
    bool pointer_held;  /* one contact owns the pointer, whatever its id */
    int32_t pointer_track;
} host_state_t;

static int initialize(void *value, const char *asset_root)
{
    host_state_t *state = value;
#if !defined(RAYLIB_LITE_GAME_NATIVE)
    /* The Host serves the packed atlases from the generated asset folder. */
    raylib_lite_host_assets_set_root(asset_root);
#if !defined(FROSTLINE_PC)
    /* The PC build sets the path itself, in the player's data folder: the
     * assets may sit in a read-only app bundle. */
    char path[512];
    snprintf(path, sizeof(path), "%s/frostline_record.bin", asset_root);
    frostline_record_set_host_path(path);
#endif
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
    app_game_set_progress(state->game, state->record.stars,
                          state->record.best_score, FROSTLINE_RECORD_LEVELS);
    fl_settings_t settings = fl_default_settings();
    if (state->record.has_settings) {
        settings.music = state->record.music;
        settings.sound = state->record.sound;
        settings.haptics = state->record.haptics;
        settings.lang = state->record.lang;
    }
    app_game_set_settings(state->game, &settings);
    state->seen_enemies = state->record.seen_enemies;
    frostline_view_set_seen_enemies(state->seen_enemies);
    frostline_audio_init(&state->audio);
    fl_state_t model;
    app_game_get_model(state->game, &model);
    frostline_audio_configure(&state->audio, model.settings.music,
                              model.settings.sound, model.settings.haptics != 0);
    state->settings_serial = model.settings_serial;
    state->reset_serial = model.reset_serial;
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
    if (event->type == RAYLIB_LITE_GAME_INPUT_POINTER) {
        /* The browser names the mouse pointer 1. The device names its primary
         * touch 0. Own whichever contact arrives first and follow only that
         * one, so a second finger cannot cancel the first. */
        bool mine = state->pointer_held && state->pointer_track == event->track_id;
        if (event->pressed && (!state->pointer_held || mine)) {
            if (!state->pointer_held) {
                frostline_audio_tap(&state->audio);
            }
            state->pointer_held = true;
            state->pointer_track = event->track_id;
            app_game_set_pointer(state->game, event->x, event->y, true);
            frostline_view_set_pointer(event->x, event->y, true);
        } else if (!event->pressed && (!state->pointer_held || mine)) {
            state->pointer_held = false;
            app_game_set_pointer(state->game, event->x, event->y, false);
            frostline_view_set_pointer(event->x, event->y, false);
        }
    } else if (event->type == RAYLIB_LITE_GAME_INPUT_CONTROL) {
        switch (event->code) {
        case RAYLIB_LITE_GAME_CONTROL_PAUSE: app_game_set_paused(state->game, true); break;
        case RAYLIB_LITE_GAME_CONTROL_RESUME: app_game_set_paused(state->game, false); break;
        case RAYLIB_LITE_GAME_CONTROL_RESET:
            app_game_reset(state->game);
            state->recorded_run = 0;
            state->pointer_held = false;
            break;
        default: break;
        }
    } else if (event->type == RAYLIB_LITE_GAME_INPUT_ACTION && event->pressed) {
        if (event->code == GAME_ACTION_RESET) {
            app_game_reset(state->game);
            state->recorded_run = 0;
            state->pointer_held = false;
        }
        if (event->code == GAME_ACTION_PAUSE) {
            app_game_set_paused(state->game, !app_game_read(state->game).paused);
        }
    }
}

/* Which theme the screen calls for: the title's outside battle, the boss
 * track while a boss walks the road, the battle drum during a wave. */
static uint8_t music_for(const fl_state_t *model)
{
    switch (model->phase) {
    case FL_PHASE_TITLE:
    case FL_PHASE_MENU:
    case FL_PHASE_SETTINGS:
    case FL_PHASE_ENDING:
        return FL_MUSIC_TITLE;
    case FL_PHASE_WAVE:
        for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
            const fl_enemy_t *enemy = &model->enemies[i];
            if (enemy->alive && (enemy->champion
                                 || enemy->behaviour == FL_BEHAVIOUR_WYRM
                                 || enemy->behaviour == FL_BEHAVIOUR_KING)) {
                return FL_MUSIC_BOSS;
            }
        }
        return FL_MUSIC_BATTLE;
    default:
        return FL_MUSIC_NONE;
    }
}

/* Keep the record in step with what the player changed: settings, a wiped
 * campaign, the enemies introduced so far. */
static void sync_record(host_state_t *state, const fl_state_t *model)
{
    bool dirty = false;
    if (model->settings_serial != state->settings_serial) {
        state->settings_serial = model->settings_serial;
        state->record.music = model->settings.music;
        state->record.sound = model->settings.sound;
        state->record.haptics = model->settings.haptics;
        state->record.lang = model->settings.lang;
        state->record.has_settings = 1;
        frostline_audio_configure(&state->audio, model->settings.music,
                                  model->settings.sound,
                                  model->settings.haptics != 0);
        dirty = true;
    }
    if (model->reset_serial != state->reset_serial) {
        state->reset_serial = model->reset_serial;
        memset(state->record.best_score, 0, sizeof(state->record.best_score));
        memset(state->record.stars, 0, sizeof(state->record.stars));
        state->record.seen_enemies = 0;
        state->seen_enemies = 0;
        frostline_view_set_seen_enemies(0);
        dirty = true;
    }
    uint32_t seen = frostline_view_seen_enemies();
    if (seen != state->seen_enemies) {
        state->seen_enemies = seen;
        state->record.seen_enemies = seen;
        dirty = true;
    }
    if (dirty) {
        frostline_record_store(&state->record);
    }
}

static void update(void *value)
{
    host_state_t *state = value;
    app_game_update(state->game);
    /* What the simulation did this frame drives the sound and the motor. */
    fl_state_t model;
    app_game_get_model(state->game, &model);
    sync_record(state, &model);
    fl_audio_frame_t frame;
    frame.events = app_game_take_events(state->game);
    frame.tick = model.tick;
    frame.wave_active = (model.phase == FL_PHASE_WAVE) ? 1u : 0u;
    frame.music = music_for(&model);
    frame.run_over = (model.phase == FL_PHASE_WON || model.phase == FL_PHASE_LOST)
                     ? 1u : 0u;
    frame.kill_kind = model.last_kill_kind;
    frostline_audio_update(&state->audio, &frame);
    /* A won run has already raised the level's entry in the model; copy it
     * into the record once and store it if anything improved. */
    if (model.phase != FL_PHASE_WON || state->recorded_run == model.run_serial) {
        return;
    }
    state->recorded_run = model.run_serial;
    uint8_t level = model.level;
    if (level >= FROSTLINE_RECORD_LEVELS) {
        return;
    }
    bool better = model.level_stars[level] > state->record.stars[level]
                  || model.level_best[level] > state->record.best_score[level];
    if (better) {
        state->record.stars[level] = model.level_stars[level];
        state->record.best_score[level] = model.level_best[level];
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
    ClearBackground(FL_INK);
    host_state_t *state = value;
    fl_state_t model;
    app_game_get_model(state->game, &model);
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

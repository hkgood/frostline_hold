/* The Raylib-style audio calls of the game, played through SDL.
 *
 * The Engine's mixer does the work, as on the device: it decodes the packed
 * .sound files and mixes 24 kHz mono. SDL pulls from it on its audio thread
 * (on the web, from the browser's audio callback) and converts to whatever
 * the output device wants.
 *
 * A Sound or Music hands the game a pointer into s_slots; every call checks
 * that the pointer is one of ours before using it.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include <string.h>

#include <SDL3/SDL.h>

#include "raylib_lite_assets.h"
#include "raylib_lite_audio_mixer.h"
#include "raylib_lite_game_audio.h"

/* The mixer's clip table: every effect and theme, with room to spare. */
#define PC_AUDIO_CLIPS 48
#define PC_AUDIO_VOICES 12
/* The device's default theme loudness, so both targets sound alike. */
#define PC_MUSIC_DEFAULT_GAIN 0.28f
/* Frames mixed per pass of the pull callback. */
#define PC_MIX_CHUNK 512

typedef struct {
    raylib_lite_audio_clip_t clip;
    bool used;
} pc_audio_slot_t;

static raylib_lite_audio_mixer_t *s_mixer;
static SDL_AudioStream *s_stream;
static SDL_Mutex *s_lock;
static pc_audio_slot_t s_slots[PC_AUDIO_CLIPS];
static float s_master_volume = 1.0f;

static void lock_mixer(void *context)
{
    SDL_LockMutex(context);
}

static void unlock_mixer(void *context)
{
    SDL_UnlockMutex(context);
}

static raylib_lite_result_t open_asset(void *context, const char *path,
                                       raylib_lite_audio_asset_t *out_asset)
{
    (void)context;
    raylib_lite_asset_view_t view;
    if (!out_asset || raylib_lite_asset_open(path, &view) != RAYLIB_LITE_OK) {
        return RAYLIB_LITE_IO_ERROR;
    }
    *out_asset = (raylib_lite_audio_asset_t){.data = view.data, .size = view.size};
    return RAYLIB_LITE_OK;
}

/* The asset store keeps every file until exit, so there is nothing to free. */
static void close_asset(void *context, raylib_lite_audio_asset_t *asset)
{
    (void)context;
    if (asset) {
        memset(asset, 0, sizeof(*asset));
    }
}

static void SDLCALL pull_audio(void *userdata, SDL_AudioStream *stream,
                               int additional_amount, int total_amount)
{
    (void)userdata;
    (void)total_amount;
    int16_t frames[PC_MIX_CHUNK];
    int wanted = additional_amount / (int)sizeof(int16_t);
    while (wanted > 0) {
        int count = wanted < PC_MIX_CHUNK ? wanted : PC_MIX_CHUNK;
        if (raylib_lite_audio_mixer_mix(s_mixer, frames, (size_t)count) != RAYLIB_LITE_OK) {
            memset(frames, 0, sizeof(frames));
        }
        SDL_PutAudioStreamData(stream, frames, count * (int)sizeof(int16_t));
        wanted -= count;
    }
}

void raylib_lite_game_audio_init(void)
{
    if (s_mixer) {
        return;
    }
    /* Without threads (a web build) SDL may hand back no mutex; the mixer
     * then runs unlocked, which is safe on one thread. */
    s_lock = SDL_CreateMutex();
    raylib_lite_audio_mixer_config_t config = {
        .max_clips = PC_AUDIO_CLIPS,
        .max_sfx_voices = PC_AUDIO_VOICES,
        .assets = {.open = open_asset, .close = close_asset},
    };
    if (s_lock) {
        config.sync = (raylib_lite_audio_sync_t){s_lock, lock_mixer, unlock_mixer};
    }
    if (raylib_lite_audio_mixer_create(&config, &s_mixer) != RAYLIB_LITE_OK) {
        s_mixer = NULL;
        raylib_lite_game_audio_close();
        return;
    }
    raylib_lite_audio_mixer_set_music_volume(s_mixer, 1.0f);
    raylib_lite_audio_mixer_set_master_volume(s_mixer, s_master_volume);

    const SDL_AudioSpec spec = {
        SDL_AUDIO_S16, (int)RAYLIB_LITE_MIXER_CHANNELS, (int)RAYLIB_LITE_MIXER_SAMPLE_RATE,
    };
    s_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec,
                                         pull_audio, NULL);
    if (!s_stream) {
        SDL_Log("audio: no output device (%s); the game runs silent", SDL_GetError());
        raylib_lite_game_audio_close();
        return;
    }
    SDL_ResumeAudioStreamDevice(s_stream);
}

void raylib_lite_game_audio_close(void)
{
    /* Destroying the stream stops and joins the pull callback, so the
     * mixer is quiet before it goes. */
    if (s_stream) {
        SDL_DestroyAudioStream(s_stream);
        s_stream = NULL;
    }
    if (s_mixer) {
        raylib_lite_audio_mixer_destroy(s_mixer);
        s_mixer = NULL;
    }
    if (s_lock) {
        SDL_DestroyMutex(s_lock);
        s_lock = NULL;
    }
    memset(s_slots, 0, sizeof(s_slots));
}

bool raylib_lite_game_audio_ready(void)
{
    return s_mixer && s_stream;
}

static pc_audio_slot_t *slot_from(const void *handle)
{
    const pc_audio_slot_t *slot = handle;
    if (!s_mixer || slot < s_slots || slot >= s_slots + PC_AUDIO_CLIPS) {
        return NULL;
    }
    return s_slots[slot - s_slots].used ? &s_slots[slot - s_slots] : NULL;
}

/* Loads a clip into a free slot; NULL when the mixer or the table is full. */
static pc_audio_slot_t *load_clip(const char *path)
{
    if (!s_mixer) {
        return NULL;
    }
    for (unsigned i = 0; i < PC_AUDIO_CLIPS; ++i) {
        if (s_slots[i].used) {
            continue;
        }
        if (raylib_lite_audio_mixer_load(s_mixer, path, &s_slots[i].clip) != RAYLIB_LITE_OK) {
            SDL_Log("audio: cannot load %s", path);
            return NULL;
        }
        s_slots[i].used = true;
        return &s_slots[i];
    }
    return NULL;
}

static void unload_clip(pc_audio_slot_t *slot)
{
    if (slot) {
        (void)raylib_lite_audio_mixer_unload(s_mixer, slot->clip);
        slot->used = false;
    }
}

static AudioStream stream_of(pc_audio_slot_t *slot)
{
    return (AudioStream){
        .buffer = (rAudioBuffer *)slot,
        .sampleRate = RAYLIB_LITE_MIXER_SAMPLE_RATE,
        .sampleSize = 16,
        .channels = RAYLIB_LITE_MIXER_CHANNELS,
    };
}

Sound raylib_lite_game_audio_load_sound(const char *asset_path)
{
    pc_audio_slot_t *slot = load_clip(asset_path);
    if (!slot) {
        return (Sound){0};
    }
    return (Sound){
        .stream = stream_of(slot),
        .frameCount = raylib_lite_audio_mixer_clip_frames(s_mixer, slot->clip),
    };
}

void raylib_lite_game_audio_unload_sound(Sound sound)
{
    unload_clip(slot_from(sound.stream.buffer));
}

void raylib_lite_game_audio_play_sound(Sound sound)
{
    pc_audio_slot_t *slot = slot_from(sound.stream.buffer);
    if (slot) {
        (void)raylib_lite_audio_mixer_play_sound(s_mixer, slot->clip);
    }
}

void raylib_lite_game_audio_stop_sound(Sound sound)
{
    pc_audio_slot_t *slot = slot_from(sound.stream.buffer);
    if (slot) {
        (void)raylib_lite_audio_mixer_stop_sound(s_mixer, slot->clip);
    }
}

bool raylib_lite_game_audio_is_sound_playing(Sound sound)
{
    pc_audio_slot_t *slot = slot_from(sound.stream.buffer);
    return slot && raylib_lite_audio_mixer_is_sound_playing(s_mixer, slot->clip);
}

void raylib_lite_game_audio_set_sound_volume(Sound sound, float volume)
{
    pc_audio_slot_t *slot = slot_from(sound.stream.buffer);
    if (slot) {
        (void)raylib_lite_audio_mixer_set_sound_volume(s_mixer, slot->clip, volume);
    }
}

Music raylib_lite_game_audio_load_music(const char *asset_path)
{
    pc_audio_slot_t *slot = load_clip(asset_path);
    if (!slot) {
        return (Music){0};
    }
    (void)raylib_lite_audio_mixer_set_music_clip_volume(s_mixer, slot->clip,
                                                        PC_MUSIC_DEFAULT_GAIN);
    return (Music){
        .stream = stream_of(slot),
        .frameCount = raylib_lite_audio_mixer_clip_frames(s_mixer, slot->clip),
        .looping = true,
        .ctxData = slot,
    };
}

void raylib_lite_game_audio_unload_music(Music music)
{
    unload_clip(slot_from(music.ctxData));
}

void raylib_lite_game_audio_play_music(Music music)
{
    pc_audio_slot_t *slot = slot_from(music.ctxData);
    if (slot) {
        (void)raylib_lite_audio_mixer_play_music(s_mixer, slot->clip);
    }
}

/* The mixer streams on its own; nothing to feed per frame. */
void raylib_lite_game_audio_update_music(Music music)
{
    (void)music;
}

void raylib_lite_game_audio_stop_music(Music music)
{
    pc_audio_slot_t *slot = slot_from(music.ctxData);
    if (slot) {
        (void)raylib_lite_audio_mixer_stop_music_clip(s_mixer, slot->clip);
    }
}

void raylib_lite_game_audio_set_music_volume(Music music, float volume)
{
    pc_audio_slot_t *slot = slot_from(music.ctxData);
    if (slot) {
        (void)raylib_lite_audio_mixer_set_music_clip_volume(s_mixer, slot->clip, volume);
    }
}

void raylib_lite_game_audio_set_master_volume(float volume)
{
    s_master_volume = volume < 0.0f ? 0.0f : (volume > 1.0f ? 1.0f : volume);
    if (s_mixer) {
        raylib_lite_audio_mixer_set_master_volume(s_mixer, s_master_volume);
    }
}

void raylib_lite_game_audio_get_stats(raylib_lite_game_audio_stats_t *stats)
{
    if (!stats) {
        return;
    }
    memset(stats, 0, sizeof(*stats));
    if (!s_mixer) {
        return;
    }
    raylib_lite_audio_mixer_stats_t mixer;
    raylib_lite_audio_mixer_get_stats(s_mixer, &mixer);
    stats->voice_steals = mixer.voice_steals;
    stats->active_sfx_voices = (uint8_t)mixer.active_sfx_voices;
    stats->music_playing = mixer.music_playing;
}

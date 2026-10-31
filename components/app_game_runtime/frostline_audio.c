// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
#include "include/frostline_audio.h"

#include <string.h>

/* The event bits live with the model. The include is relative because the
 * Host simulator compiles these files without component include paths. */
#include "../app_game/include/app_game.h"

#if defined(RAYLIB_LITE_GAME_NATIVE)
#include "raylib_lite_raylib_audio.h"
#include "native_feedback.h"
#endif

/* The asset names the packer produced from the WAV files. Only the device
 * loads them; the Host names the sound in the state JSON instead. */
#if defined(RAYLIB_LITE_GAME_NATIVE)
static const char *const SFX_ASSETS[FL_SFX_COUNT] = {
    "ui_tap.sound",  "ui_deny.sound",  "build.sound",   "upgrade.sound",
    "sell.sound",    "shot_guard.sound", "shot_frost.sound", "shot_siege.sound",
    "shot_storm.sound", "shot_rune.sound", "shot_ember.sound", "clash.sound",
    "impact.sound",  "kill.sound",     "leak.sound",    "wave_start.sound",
    "wave_clear.sound", "hero_hit.sound", "hero_down.sound", "nova.sound",
    "win.sound",     "lose.sound",
    "crack.sound",   "split.sound",    "burrow.sound",  "freeze.sound",
    "boss.sound",
};

#define MUSIC_ASSET "music_battle.sound"
#endif

/* The browser preview only plays a sound when the name changes, so the Host
 * rotates through two spellings of the same file. The second spelling exists
 * as a copy of the first in assets_src and never reaches the device. */
static const char *const SFX_HOST_NAMES[FL_SFX_COUNT] = {
    "ui_tap",  "ui_deny",  "build",      "upgrade",
    "sell",    "shot_guard", "shot_frost", "shot_siege",
    "shot_storm", "shot_rune", "shot_ember", "clash",
    "impact",  "kill",     "leak",       "wave_start",
    "wave_clear", "hero_hit", "hero_down", "nova",
    "win",     "lose",
    "crack",   "split",    "burrow",     "freeze",
    "boss",
};

/* Which event makes which sound, and how loud the motor gets. The first
 * entry that matches a frame wins, so the order is the priority: a nova
 * must never be buried under a shot. */
typedef struct {
    uint32_t event;
    uint8_t cue;
    uint16_t gap;        /* ticks that must pass before this cue fires again */
    uint8_t haptic;      /* motor strength, 0 for no haptics */
    uint16_t haptic_ms;
    uint8_t haptic_two;  /* second pulse strength, 0 for a single pulse */
    uint16_t haptic_gap_ms;
    uint16_t haptic_two_ms;
} fl_cue_rule_t;

/* The rules are written out top to bottom, so the array order is the
 * priority: a nova must never be buried under a shot. */
static const fl_cue_rule_t RULES[] = {
    {FL_EVENT_WON,        FL_SFX_WIN,         0,   60, 60,  100, 140, 90},
    {FL_EVENT_LOST,       FL_SFX_LOSE,        0,   90, 120,  40, 160, 70},
    {FL_EVENT_BOSS,       FL_SFX_BOSS,        30, 100, 90,  80, 120, 60},
    {FL_EVENT_NOVA,       FL_SFX_NOVA,        12,  95, 50,  70,  45, 30},
    {FL_EVENT_FREEZE,     FL_SFX_FREEZE,      12,  70, 40,   0,   0,  0},
    {FL_EVENT_LEAK,       FL_SFX_LEAK,        10,  85, 60,  95,  70, 40},
    {FL_EVENT_WAVE_START, FL_SFX_WAVE_START,  10,  45, 70,   0,   0,  0},
    {FL_EVENT_WAVE_CLEAR, FL_SFX_WAVE_CLEAR,  10,  40, 40,   0,   0,  0},
    {FL_EVENT_HERO_DOWN,  FL_SFX_HERO_DOWN,   12,  65, 40,  90,  60, 30},
    {FL_EVENT_HERO_HIT,   FL_SFX_HERO_HIT,    8,   35, 25,   0,   0,  0},
    {FL_EVENT_CRACK,      FL_SFX_CRACK,       8,   60, 30,   0,   0,  0},
    {FL_EVENT_SPLIT,      FL_SFX_SPLIT,       8,   35, 25,   0,   0,  0},
    {FL_EVENT_IMPACT,     FL_SFX_IMPACT,      8,   55, 30,   0,   0,  0},
    {FL_EVENT_KILLED,     FL_SFX_KILL,        5,    0,  0,   0,   0,  0},
    {FL_EVENT_SHOT_STORM, FL_SFX_SHOT_STORM,  6,   30, 20,   0,   0,  0},
    {FL_EVENT_SHOT_SIEGE, FL_SFX_SHOT_SIEGE,  5,   20, 18,   0,   0,  0},
    {FL_EVENT_SHOT_RUNE,  FL_SFX_SHOT_RUNE,   6,   18, 16,   0,   0,  0},
    {FL_EVENT_SHOT_EMBER, FL_SFX_SHOT_EMBER,  6,   20, 18,   0,   0,  0},
    {FL_EVENT_SHOT_FROST, FL_SFX_SHOT_FROST,  5,   18, 16,   0,   0,  0},
    {FL_EVENT_SHOT_GUARD, FL_SFX_SHOT_GUARD,  5,   20, 18,   0,   0,  0},
    {FL_EVENT_HERO_SHOT,  FL_SFX_SHOT_FROST,  6,    0,  0,   0,   0,  0},
    {FL_EVENT_CLASH,      FL_SFX_CLASH,       7,    0,  0,   0,   0,  0},
    {FL_EVENT_BURROW,     FL_SFX_BURROW,      10,   0,  0,   0,   0,  0},
    {FL_EVENT_UPGRADED,   FL_SFX_UPGRADE,     6,   45, 30,  75,  40, 25},
    {FL_EVENT_BUILT,      FL_SFX_BUILD,       6,   60, 40,   0,   0,  0},
    {FL_EVENT_HERO_PLACED, FL_SFX_BUILD,      6,   60, 40,   0,   0,  0},
    {FL_EVENT_SOLD,       FL_SFX_SELL,        6,   30, 25,   0,   0,  0},
    {FL_EVENT_DENIED,     FL_SFX_UI_DENY,     8,   75, 60,   0,   0,  0},
};

#define RULE_COUNT (sizeof(RULES) / sizeof(RULES[0]))

#if defined(RAYLIB_LITE_GAME_NATIVE)
static Sound s_sounds[FL_SFX_COUNT];
static Music s_music;
#endif

static void play_cue(frostline_audio_t *audio, uint8_t cue)
{
    if (cue >= FL_SFX_COUNT) {
        return;
    }
    /* The Host browser only reacts to a change of name, so the same sound
     * gets a second spelling on alternate plays. */
    const char *base = SFX_HOST_NAMES[cue];
    if (audio->variant) {
        size_t length = strlen(base);
        if (length + 2u < sizeof(audio->name)) {
            memcpy(audio->name, base, length);
            audio->name[length] = '_';
            audio->name[length + 1u] = 'b';
            audio->name[length + 2u] = '\0';
        }
    } else {
        size_t length = strlen(base);
        if (length + 1u < sizeof(audio->name)) {
            memcpy(audio->name, base, length + 1u);
        }
    }
    audio->variant = (uint8_t)((audio->variant + 1u) & 1u);

#if defined(RAYLIB_LITE_GAME_NATIVE)
    if (audio->ready) {
        PlaySound(s_sounds[cue]);
    }
#else
    (void)audio;
#endif
}

static void fire_haptics(const fl_cue_rule_t *rule)
{
#if defined(RAYLIB_LITE_GAME_NATIVE)
    if (!rule->haptic) {
        return;
    }
    if (rule->haptic_two) {
        raylib_lite_native_feedback_pattern(rule->haptic, rule->haptic_ms,
                                            rule->haptic_two,
                                            rule->haptic_gap_ms,
                                            rule->haptic_two_ms);
    } else {
        raylib_lite_native_feedback_pulse(rule->haptic, rule->haptic_ms);
    }
#else
    (void)rule;
#endif
}

void frostline_audio_init(frostline_audio_t *audio)
{
    if (!audio) {
        return;
    }
    memset(audio, 0, sizeof(*audio));
#if defined(RAYLIB_LITE_GAME_NATIVE)
    InitAudioDevice();
    if (IsAudioDeviceReady()) {
        for (unsigned i = 0; i < FL_SFX_COUNT; ++i) {
            s_sounds[i] = LoadSound(SFX_ASSETS[i]);
        }
        s_music = LoadMusicStream(MUSIC_ASSET);
        SetMusicVolume(s_music, 0.26f);
        audio->ready = true;
    }
    raylib_lite_native_feedback_init();
#else
    audio->ready = true;
#endif
}

void frostline_audio_shutdown(frostline_audio_t *audio)
{
    if (!audio) {
        return;
    }
#if defined(RAYLIB_LITE_GAME_NATIVE)
    if (audio->ready) {
        for (unsigned i = 0; i < FL_SFX_COUNT; ++i) {
            if (s_sounds[i].frameCount) {
                UnloadSound(s_sounds[i]);
            }
        }
        if (s_music.frameCount) {
            StopMusicStream(s_music);
            UnloadMusicStream(s_music);
        }
        CloseAudioDevice();
    }
    raylib_lite_native_feedback_stop();
#endif
    audio->ready = false;
    audio->music_playing = false;
}

void frostline_audio_tap(frostline_audio_t *audio)
{
    if (audio) {
        audio->pending_tap = true;
    }
}

void frostline_audio_update(frostline_audio_t *audio,
                            const fl_audio_frame_t *frame)
{
    if (!audio || !frame) {
        return;
    }

    /* Let the cooldowns run down. */
    for (unsigned i = 0; i < FL_SFX_COUNT; ++i) {
        if (audio->cooldown[i]) {
            audio->cooldown[i]--;
        }
    }

    int chosen = -1;
    const fl_cue_rule_t *chosen_rule = NULL;
    for (unsigned i = 0; i < RULE_COUNT; ++i) {
        const fl_cue_rule_t *rule = &RULES[i];
        if (!(frame->events & rule->event)) {
            continue;
        }
        if (audio->cooldown[rule->cue]) {
            continue;
        }
        chosen = rule->cue;
        chosen_rule = rule;
        break;
    }

    if (chosen >= 0) {
        audio->pending_tap = false;
        audio->cooldown[chosen] = chosen_rule->gap;
        play_cue(audio, (uint8_t)chosen);
        fire_haptics(chosen_rule);
    } else if (audio->pending_tap) {
        audio->pending_tap = false;
        if (!audio->cooldown[FL_SFX_UI_TAP]) {
            audio->cooldown[FL_SFX_UI_TAP] = 3;
            play_cue(audio, FL_SFX_UI_TAP);
        }
    }

    /* The theme runs under a wave and stops when the wave is over. */
    audio->music_wanted = frame->wave_active != 0;
#if defined(RAYLIB_LITE_GAME_NATIVE)
    if (audio->ready && s_music.frameCount) {
        if (audio->music_wanted && !audio->music_playing) {
            PlayMusicStream(s_music);
            audio->music_playing = true;
        } else if (!audio->music_wanted && audio->music_playing) {
            StopMusicStream(s_music);
            audio->music_playing = false;
        }
    }
#endif
}

const char *frostline_audio_name(const frostline_audio_t *audio)
{
    return audio ? audio->name : "";
}

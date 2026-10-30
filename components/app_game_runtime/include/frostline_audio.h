/* Sound and haptics for Frostline Hold.
 *
 * The model only says what happened; this module decides what that sounds
 * like and how hard the board shakes. It sits in the runtime component, so
 * the pure model never learns about speakers or motors.
 *
 * On the device the sounds go through the engine's mixer and the haptics
 * drive the vibration motor. On the Host the sounds are named in the state
 * JSON, which the browser preview turns into Web Audio calls.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* Every sound the game knows about. The order matches the asset names. */
typedef enum {
    FL_SFX_UI_TAP = 0,
    FL_SFX_UI_DENY,
    FL_SFX_BUILD,
    FL_SFX_UPGRADE,
    FL_SFX_SELL,
    FL_SFX_SHOT_GUARD,
    FL_SFX_SHOT_FROST,
    FL_SFX_SHOT_SIEGE,
    FL_SFX_IMPACT,
    FL_SFX_KILL,
    FL_SFX_LEAK,
    FL_SFX_WAVE_START,
    FL_SFX_WAVE_CLEAR,
    FL_SFX_HERO_HIT,
    FL_SFX_HERO_DOWN,
    FL_SFX_NOVA,
    FL_SFX_WIN,
    FL_SFX_LOSE,
    FL_SFX_COUNT,
} fl_sfx_t;

/* What one frame of the simulation did. */
typedef struct {
    uint32_t events;
    uint32_t tick;
    uint8_t wave_active;  /* a wave is running right now */
    uint8_t run_over;     /* the run ended on this frame */
    uint8_t kill_kind;    /* which enemy died last */
} fl_audio_frame_t;

typedef struct {
    bool ready;                /* the audio device came up */
    uint16_t cooldown[FL_SFX_COUNT]; /* ticks until this cue may fire again */
    char name[24];             /* the last sound, for the Host browser */
    uint8_t variant;           /* rotates the Host name so repeats still play */
    bool pending_tap;          /* a finger went down during this frame */
    bool music_wanted;         /* the theme should be running */
    bool music_playing;
} frostline_audio_t;

/* Bring up the audio device and load every clip. Safe to call twice. */
void frostline_audio_init(frostline_audio_t *audio);
/* Stop everything and release the clips. */
void frostline_audio_shutdown(frostline_audio_t *audio);
/* Note that a finger went down; the tap sound only plays if nothing
 * more interesting happened on the same frame. */
void frostline_audio_tap(frostline_audio_t *audio);
/* Turn one frame of model events into sound and haptics. */
void frostline_audio_update(frostline_audio_t *audio,
                            const fl_audio_frame_t *frame);
/* The name of the last sound that played, or an empty string. */
const char *frostline_audio_name(const frostline_audio_t *audio);

/* Tests for the sound and haptics layer.
 *
 * The model says what happened; this layer decides what that sounds like.
 * These tests drive the model, take its events and check that the right
 * sound comes out, that repeats still make a sound, and that the loud
 * moments are not crowded out by the frequent ones.
 *
 * Build and run:
 *   gcc -std=c11 -Wall -Wextra -Iinclude -I../app_game/include -Idata \
 *       -o /tmp/test_audio test/test_audio.c frostline_audio.c \
 *       ../app_game/frostline_model.c -lm && /tmp/test_audio
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "frostline_audio.h"
#include "../../app_game/include/frostline_model.h"

/* The Host spells a sound two ways so that repeats still make a noise. */
static int name_is(const char *base, const char *actual)
{
    size_t length = strlen(base);
    if (strncmp(base, actual, length) != 0) {
        return 0;
    }
    return actual[length] == '\0' || (actual[length] == '_' && actual[length + 1] == 'b');
}

static void feed(frostline_audio_t *audio, fl_state_t *state)
{
    fl_audio_frame_t frame;
    frame.events = fl_events_take(state);
    frame.tick = state->tick;
    frame.wave_active = (state->phase == FL_PHASE_WAVE) ? 1u : 0u;
    frame.run_over = (state->phase == FL_PHASE_WON ||
                      state->phase == FL_PHASE_LOST) ? 1u : 0u;
    frame.kill_kind = state->last_kill_kind;
    frostline_audio_update(audio, &frame);
}

static void test_building_makes_the_build_sound(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    feed(&audio, &state);
    assert(name_is("build", frostline_audio_name(&audio)));
    frostline_audio_shutdown(&audio);
}

static void test_the_host_name_rotates_so_repeats_still_play(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    feed(&audio, &state);
    assert(name_is("build", frostline_audio_name(&audio)));
    /* The same sound again comes out under the second spelling. The
     * cooldown runs down on audio frames, so the loop has to feed the
     * sound layer as well as the model. */
    for (int i = 0; i < 8; ++i) {
        fl_state_update(&state);
        feed(&audio, &state);
    }
    assert(fl_build_tower(&state, 50, 192, FL_TOWER_GUARD));
    feed(&audio, &state);
    assert(strcmp(frostline_audio_name(&audio), "build_b") == 0);
    frostline_audio_shutdown(&audio);
}

static void test_not_enough_gold_makes_the_refusal_sound(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.gold = 5;
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    assert(!fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    feed(&audio, &state);
    assert(name_is("ui_deny", frostline_audio_name(&audio)));
    frostline_audio_shutdown(&audio);
}

static void test_a_leak_makes_the_dark_bell(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    bool leaked = false;
    for (int i = 0; i < 4000 && !leaked; ++i) {
        fl_state_update(&state);
        uint32_t events = fl_events_take(&state);
        if (events & FL_EVENT_LEAK) {
            fl_audio_frame_t frame;
            frame.events = events;
            frame.tick = state.tick;
            frame.wave_active = 1;
            frame.run_over = 0;
            frame.kill_kind = 0;
            frostline_audio_update(&audio, &frame);
            leaked = true;
        }
    }
    assert(leaked);
    assert(name_is("leak", frostline_audio_name(&audio)));
    frostline_audio_shutdown(&audio);
}

static void test_a_tower_shot_makes_its_own_sound(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    assert(fl_build_tower(&state, 58, 108, FL_TOWER_FROST));
    fl_events_take(&state);
    bool fired = false;
    for (int i = 0; i < 900 && !fired; ++i) {
        fl_state_update(&state);
        uint32_t events = fl_events_take(&state);
        if (events & FL_EVENT_SHOT_FROST) {
            fl_audio_frame_t frame;
            frame.events = events;
            frame.tick = state.tick;
            frame.wave_active = 1;
            frame.run_over = 0;
            frame.kill_kind = 0;
            frostline_audio_update(&audio, &frame);
            fired = true;
        }
    }
    assert(fired);
    assert(name_is("shot_frost", frostline_audio_name(&audio)));
    frostline_audio_shutdown(&audio);
}

static void test_the_loud_moment_wins_over_the_frequent_one(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    /* A shot and a leak in the same frame: the leak is what matters. */
    fl_audio_frame_t frame;
    frame.events = FL_EVENT_SHOT_GUARD | FL_EVENT_LEAK;
    frame.tick = 10;
    frame.wave_active = 1;
    frame.run_over = 0;
    frame.kill_kind = 0;
    frostline_audio_update(&audio, &frame);
    assert(name_is("leak", frostline_audio_name(&audio)));
    frostline_audio_shutdown(&audio);
}

static void test_a_repeated_cue_waits_for_its_cooldown(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    fl_audio_frame_t frame;
    frame.events = FL_EVENT_KILLED;
    frame.tick = 10;
    frame.wave_active = 1;
    frame.run_over = 0;
    frame.kill_kind = 0;
    frostline_audio_update(&audio, &frame);
    assert(name_is("kill", frostline_audio_name(&audio)));

    /* Right after, the same cue is held back so the mix stays clear. */
    frame.tick = 11;
    frostline_audio_update(&audio, &frame);
    assert(name_is("kill", frostline_audio_name(&audio)));

    /* Once the cooldown has run out it plays again. */
    for (int i = 0; i < 8; ++i) {
        frame.tick = (uint32_t)(12 + i);
        frostline_audio_update(&audio, &frame);
    }
    assert(strcmp(frostline_audio_name(&audio), "kill_b") == 0);
    frostline_audio_shutdown(&audio);
}

static void test_a_tap_only_sounds_when_nothing_else_happened(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    frostline_audio_tap(&audio);
    fl_audio_frame_t frame;
    frame.events = 0;
    frame.tick = 3;
    frame.wave_active = 0;
    frame.run_over = 0;
    frame.kill_kind = 0;
    frostline_audio_update(&audio, &frame);
    assert(name_is("ui_tap", frostline_audio_name(&audio)));

    /* A tap that also builds a tower makes the build sound, not the tap. */
    frostline_audio_tap(&audio);
    frame.events = FL_EVENT_BUILT;
    frostline_audio_update(&audio, &frame);
    assert(name_is("build", frostline_audio_name(&audio)));
    frostline_audio_shutdown(&audio);
}

static void test_the_theme_follows_the_wave(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    frostline_audio_t audio;
    frostline_audio_init(&audio);

    fl_audio_frame_t frame;
    frame.events = 0;
    frame.tick = 1;
    frame.wave_active = 1;
    frame.run_over = 0;
    frame.kill_kind = 0;
    frostline_audio_update(&audio, &frame);
    assert(audio.music_wanted);

    frame.wave_active = 0;
    frostline_audio_update(&audio, &frame);
    assert(!audio.music_wanted);
    frostline_audio_shutdown(&audio);
}

int main(void)
{
    test_building_makes_the_build_sound();
    test_the_host_name_rotates_so_repeats_still_play();
    test_not_enough_gold_makes_the_refusal_sound();
    test_a_leak_makes_the_dark_bell();
    test_a_tower_shot_makes_its_own_sound();
    test_the_loud_moment_wins_over_the_frequent_one();
    test_a_repeated_cue_waits_for_its_cooldown();
    test_a_tap_only_sounds_when_nothing_else_happened();
    test_the_theme_follows_the_wave();
    printf("audio tests passed\n");
    return 0;
}

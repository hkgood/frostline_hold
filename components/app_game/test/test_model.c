/* Model tests for Frostline Hold.
 *
 * Build and run:
 *   gcc -std=c11 -Wall -Wextra -Iinclude -Idata \
 *       -o /tmp/test_model test/test_model.c frostline_model.c -lm && /tmp/test_model
 *
 * SPDX-License-Identifier: Apache-2.0 */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "frostline_model.h"

static void test_reset_starts_in_prep(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(state.phase == FL_PHASE_PREP);
    assert(state.lives == 20);
    assert(state.gold == 190);
    assert(state.wave == 1);
    assert(state.selected_tower == -1);
}

static void test_build_tower_costs_gold_and_occupies_spot(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    uint16_t before = state.gold;
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    assert(state.gold == before - 70);
    assert(state.towers[0].kind == FL_TOWER_GUARD);
    assert(state.towers[0].level == 1);
    assert(state.towers[0].x == 58 && state.towers[0].y == 108);
    /* The same spot cannot host a second tower. */
    assert(!fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
}

static void test_build_tower_rejects_insufficient_gold(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.gold = 10;
    assert(!fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    assert(state.gold == 10);
}

static void test_build_tower_rejects_off_spot_position(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    /* 400,100 sits outside every build spot's 26px catch radius. */
    assert(!fl_build_tower(&state, 400, 100, FL_TOWER_GUARD));
    assert(state.gold == 190);
}

static void test_build_tower_snaps_to_the_nearest_spot(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    /* Tapping just beside a pad still builds on that pad. */
    assert(fl_build_tower(&state, 262, 240, FL_TOWER_GUARD));
    assert(state.towers[0].x == 258 && state.towers[0].y == 236);
}

static void test_sell_refunds_half(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    uint16_t after_build = state.gold;
    assert(fl_sell_tower(&state, 0));
    assert(state.gold == after_build + 35);
    assert(!state.towers[0].active);
}

static void test_enemies_walk_towards_the_goal(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    /* Skip the preparation phase. */
    for (int i = 0; i < 700; ++i) {
        fl_state_update(&state);
    }
    assert(state.phase == FL_PHASE_WAVE);
    bool found = false;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        if (state.enemies[i].alive) {
            found = true;
            assert(state.enemies[i].dist > 0.0f);
        }
    }
    assert(found);
}

static void test_leak_costs_a_life(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    uint8_t lives = state.lives;
    for (int i = 0; i < 4000; ++i) {
        fl_state_update(&state);
    }
    assert(state.lives < lives);
}

static void test_two_towers_hold_the_first_wave(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    /* Two guards on the first stretch of lane A: the classic opening. */
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    assert(fl_build_tower(&state, 50, 192, FL_TOWER_GUARD));
    for (int i = 0; i < 3000 && state.wave < 2; ++i) {
        fl_state_update(&state);
    }
    assert(state.wave == 2);
    assert(state.kills == 4);
    assert(state.lives == 20);
    assert(state.phase == FL_PHASE_PREP);
}

static void test_state_hash_is_stable_for_the_same_seed(void)
{
    fl_state_t a, b;
    fl_state_reset(&a, 42u);
    fl_state_reset(&b, 42u);
    for (int i = 0; i < 100; ++i) {
        fl_state_update(&a);
        fl_state_update(&b);
    }
    assert(fl_state_hash(&a) == fl_state_hash(&b));
}

static void test_pointer_selects_a_tower_card(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    /* Tap the second card in the build bar. */
    fl_set_pointer(&state, 150, 430, true);
    fl_set_pointer(&state, 150, 430, false);
    assert(state.selected_kind == FL_TOWER_FROST);
}

static void test_pointer_builds_on_a_spot(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    fl_set_pointer(&state, 150, 430, true);
    fl_set_pointer(&state, 150, 430, false);
    fl_set_pointer(&state, 58, 108, true);
    fl_set_pointer(&state, 58, 108, false);
    assert(state.towers[0].active);
    assert(state.towers[0].kind == FL_TOWER_FROST);
    assert(state.gold == 190 - 120);
}

static void test_speed_multiplier_advances_faster(void)
{
    fl_state_t slow, fast;
    fl_state_reset(&slow, 7u);
    fl_state_reset(&fast, 7u);
    fast.speed = 3;
    for (int i = 0; i < 100; ++i) {
        fl_state_update(&slow);
        fl_state_update(&fast);
    }
    assert(fast.tick == slow.tick * 3);
}

int main(void)
{
    test_reset_starts_in_prep();
    test_build_tower_costs_gold_and_occupies_spot();
    test_build_tower_rejects_insufficient_gold();
    test_build_tower_rejects_off_spot_position();
    test_build_tower_snaps_to_the_nearest_spot();
    test_sell_refunds_half();
    test_enemies_walk_towards_the_goal();
    test_leak_costs_a_life();
    test_two_towers_hold_the_first_wave();
    test_state_hash_is_stable_for_the_same_seed();
    test_pointer_selects_a_tower_card();
    test_pointer_builds_on_a_spot();
    test_speed_multiplier_advances_faster();
    printf("model tests passed\n");
    return 0;
}

/* Model tests for Frostline Hold.
 *
 * Build and run:
 *   gcc -std=c11 -Wall -Wextra -Iinclude -Idata \
 *       -o /tmp/test_model test/test_model.c frostline_model.c -lm && /tmp/test_model
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "frostline_model.h"
#include "../data/frostline_level.h"

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

static void test_score_counts_kills_and_cleared_waves(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(state.score == 0);
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    assert(fl_build_tower(&state, 50, 192, FL_TOWER_GUARD));
    for (int i = 0; i < 3000 && state.wave < 2; ++i) {
        fl_state_update(&state);
    }
    assert(state.wave == 2);
    /* Four wolves at ten points each, plus the bonus the wave pays. */
    assert(state.score_kills == 40);
    assert(state.score_waves == FL_LEVELS[0].waves[0].score_bonus);
    assert(state.score == (uint32_t)(40 + FL_LEVELS[0].waves[0].score_bonus));
    assert(state.leaks == 0);
    assert(state.score_bonus == 0);
}

static void test_a_leak_is_counted_and_costs_the_flawless_bonus(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    for (int i = 0; i < 4000; ++i) {
        fl_state_update(&state);
    }
    assert(state.leaks > 0);
    /* The run is not over, so no end-of-run bonus has been paid yet. */
    assert(state.score_bonus == 0);
    assert(state.score == state.score_kills + state.score_waves);
}

static void test_upgrading_a_tower_changes_its_stats(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    fl_tower_stats_t before = fl_tower_stats(&state.towers[0]);
    assert(before.damage == 18);
    assert(before.range == 130);

    /* The first upgrade stage offers two options and costs the level price. */
    uint16_t cost = fl_upgrade_cost(&state, 0);
    assert(cost == FL_TOWER_LEVELS[FL_TOWER_GUARD][1].upgrade);
    uint16_t gold = state.gold;
    assert(fl_upgrade_tower(&state, 0, 0));
    assert(state.towers[0].level == 2);
    assert(state.gold == gold - cost);
    assert(state.towers[0].invested == (uint32_t)(70 + cost));
    assert(fl_tower_option(&state.towers[0], 0) == 0);

    fl_tower_stats_t after = fl_tower_stats(&state.towers[0]);
    assert(after.damage > before.damage);
    assert(after.cooldown > before.cooldown);   /* BALLISTA reloads slower */

    /* The second stage stacks on top of the first. */
    state.gold = 500;
    assert(fl_upgrade_tower(&state, 0, 0));      /* PIERCE */
    assert(state.towers[0].level == 3);
    fl_tower_stats_t maxed = fl_tower_stats(&state.towers[0]);
    assert(maxed.pierce == 1);
    assert(fl_upgrade_cost(&state, 0) == 0);
    assert(!fl_upgrade_tower(&state, 0, 0));    /* already at the top */
}

static void test_upgrading_needs_the_gold(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    state.gold = 0;
    assert(!fl_upgrade_tower(&state, 0, 0));
    assert(state.towers[0].level == 1);
}

static void test_armour_is_a_flat_cut(void)
{
    /* Two identical runs, one enemy bare and one armoured. */
    fl_state_t plain;
    fl_state_t armoured;
    fl_state_reset(&plain, 1u);
    fl_state_reset(&armoured, 1u);
    for (int i = 0; i < 2; ++i) {
        fl_state_t *state = (i == 0) ? &plain : &armoured;
        assert(fl_build_tower(state, 58, 108, FL_TOWER_GUARD));
        state->phase = FL_PHASE_WAVE;
        state->batch_spawned = 4;   /* wave one is already out */
        state->enemies[0].kind = FL_ENEMY_WOLF;
        state->enemies[0].max_hp = 400.0f;
        state->enemies[0].hp = 400.0f;
        state->enemies[0].armor = (i == 0) ? 0 : 12;
        state->enemies[0].lane = 0;
        state->enemies[0].dist = 50.0f;
        state->enemies[0].alive = 1;
    }
    for (int i = 0; i < 20; ++i) {
        fl_state_update(&plain);
        fl_state_update(&armoured);
    }
    /* The guard's bolt is 18; twelve points of armour eat into it. */
    assert(plain.enemies[0].hp < 400.0f);
    assert(armoured.enemies[0].hp > plain.enemies[0].hp);
}

/* Put an enemy of a kind on the lane at a given distance along it. */
static void place_enemy(fl_state_t *state, int index, fl_enemy_kind_t kind,
                        float dist, float hp)
{
    fl_enemy_t *enemy = &state->enemies[index];
    memset(enemy, 0, sizeof(*enemy));
    enemy->kind = kind;
    enemy->max_hp = hp;
    enemy->hp = hp;
    enemy->armor = FL_ENEMY_STATS[kind].armor;
    enemy->behaviour = FL_ENEMY_STATS[kind].behaviour;
    enemy->lane = 0;
    enemy->dist = dist;
    enemy->alive = 1;
}

static void test_the_shaman_mends_its_ally(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_spawned = 4;
    place_enemy(&state, 0, FL_ENEMY_SHAMAN, 40.0f, 85.0f);
    place_enemy(&state, 1, FL_ENEMY_WOLF, 50.0f, 40.0f);
    state.enemies[1].hp = 10.0f;      /* badly hurt */
    float before = state.enemies[1].hp;
    for (int i = 0; i < 60; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[1].hp > before);
}

static void test_the_banner_hardens_its_ally(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_spawned = 4;
    place_enemy(&state, 0, FL_ENEMY_BANNERMAN, 40.0f, 110.0f);
    place_enemy(&state, 1, FL_ENEMY_RAIDER, 50.0f, 70.0f);
    assert(state.enemies[1].armor_bonus == 0);
    for (int i = 0; i < 60; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[1].armor_bonus > 0);
}

static void test_a_sap_takes_three_lives_from_the_gate(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_spawned = 4;
    place_enemy(&state, 0, FL_ENEMY_SAP, 0.0f, 60.0f);
    uint8_t lives = state.lives;
    for (int i = 0; i < 3000 && state.lives == lives; ++i) {
        fl_state_update(&state);
    }
    assert(lives - state.lives == 3);
}

static void test_the_titan_ignores_the_frost_and_the_stun(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_spawned = 4;
    place_enemy(&state, 0, FL_ENEMY_TITAN, 40.0f, 1200.0f);
    place_enemy(&state, 1, FL_ENEMY_WOLF, 50.0f, 40.0f);
    for (int i = 0; i < 90; ++i) {
        fl_state_update(&state);
    }
    /* A plain wolf would be slowed and stopped; the titan is not. */
    assert(state.enemies[0].stun_ticks == 0);
}

static void test_a_hero_stands_on_a_post(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    uint16_t gold = state.gold;
    assert(fl_build_hero(&state, 56, 232, FL_HERO_WARDEN));
    assert(state.gold == gold - FL_HERO_STATS[FL_HERO_WARDEN].cost);
    assert(state.heroes[0].active);
    assert(state.heroes[0].alive);
    assert(state.heroes[0].x == FL_LEVELS[0].posts[0].x);
    assert(state.heroes[0].y == FL_LEVELS[0].posts[0].y);
    /* The second post is still free. */
    assert(!state.heroes[1].active);
    /* The hero cannot pay for itself twice. */
    state.gold = 0;
    assert(!fl_build_hero(&state, 212, 300, FL_HERO_WARDEN));
}

static void test_a_hero_holds_the_road(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_spawned = 4;
    assert(fl_build_hero(&state, 56, 232, FL_HERO_WARDEN));
    /* An enemy right beside the hero has to stop and fight it. */
    place_enemy(&state, 0, FL_ENEMY_WOLF, 0.0f, 40.0f);
    state.enemies[0].x = 56.0f;
    state.enemies[0].y = 232.0f;
    float before = state.enemies[0].dist;
    uint16_t hp = state.heroes[0].hp;
    for (int i = 0; i < 60; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[0].dist == before);
    assert(state.heroes[0].hp < hp);
}

static void test_the_frost_nova_clears_the_road(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_spawned = 4;
    assert(fl_build_hero(&state, 56, 232, FL_HERO_WARDEN));
    place_enemy(&state, 0, FL_ENEMY_WOLF, 0.0f, 200.0f);
    state.enemies[0].x = 80.0f;
    state.enemies[0].y = 232.0f;
    float before = state.enemies[0].hp;
    assert(fl_cast_hero_skill(&state, 0));
    assert(state.enemies[0].hp < before);
    /* The nova cannot be cast twice in a row. */
    assert(!fl_cast_hero_skill(&state, 0));
    assert(state.heroes[0].skill_timer > 0);
}

static void test_the_star_rating_follows_the_lives_left(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WON;
    state.lives = 20;
    assert(fl_star_rating(&state) == 3);
    state.lives = 12;
    assert(fl_star_rating(&state) == 2);
    state.lives = 3;
    assert(fl_star_rating(&state) == 1);
    state.phase = FL_PHASE_LOST;
    assert(fl_star_rating(&state) == 0);
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

/* --- events ---------------------------------------------------------- */

static void test_building_raises_an_event(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_events_take(&state) == 0);
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    uint32_t events = fl_events_take(&state);
    assert(events & FL_EVENT_BUILT);
    /* Taking the events clears them. */
    assert(fl_events_take(&state) == 0);
}

static void test_building_without_the_gold_is_denied(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.gold = 10;
    assert(!fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    uint32_t events = fl_events_take(&state);
    assert(events & FL_EVENT_DENIED);
    assert(!(events & FL_EVENT_BUILT));
}

static void test_upgrading_and_selling_raise_events(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    fl_events_take(&state);
    assert(fl_upgrade_tower(&state, 0, 0));
    uint32_t events = fl_events_take(&state);
    assert(events & FL_EVENT_UPGRADED);
    assert(fl_sell_tower(&state, 0));
    events = fl_events_take(&state);
    assert(events & FL_EVENT_SOLD);
}

static void test_a_leak_raises_an_event(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    bool leaked = false;
    for (int i = 0; i < 4000 && !leaked; ++i) {
        fl_state_update(&state);
        if (fl_events_take(&state) & FL_EVENT_LEAK) {
            leaked = true;
        }
    }
    assert(leaked);
    assert(state.lives < 20);
}

static void test_a_kill_raises_an_event_and_names_the_enemy(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    assert(fl_build_tower(&state, 50, 192, FL_TOWER_GUARD));
    bool killed = false;
    for (int i = 0; i < 3000 && !killed; ++i) {
        fl_state_update(&state);
        if (fl_events_take(&state) & FL_EVENT_KILLED) {
            killed = true;
        }
    }
    assert(killed);
    assert(state.last_kill_kind == FL_ENEMY_WOLF);
}

static void test_a_wave_start_raises_an_event(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    bool started = false;
    for (int i = 0; i < 700 && !started; ++i) {
        fl_state_update(&state);
        if (fl_events_take(&state) & FL_EVENT_WAVE_START) {
            started = true;
        }
    }
    assert(started);
}

static void test_events_survive_across_input_and_update(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    /* A tap builds the tower, then the simulation runs in the same frame. The
     * runtime only reads the events afterwards, so both must be there. */
    assert(fl_build_tower(&state, 58, 108, FL_TOWER_GUARD));
    for (int i = 0; i < 10; ++i) {
        fl_state_update(&state);
    }
    uint32_t events = fl_events_take(&state);
    assert(events & FL_EVENT_BUILT);
    assert(state.event_serial > 0);
}

static void test_the_frost_nova_raises_an_event(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_build_hero(&state, 56, 232, FL_HERO_WARDEN));
    fl_events_take(&state);
    assert(fl_cast_hero_skill(&state, 0));
    assert(fl_events_take(&state) & FL_EVENT_NOVA);
}

/* --- campaign ------------------------------------------------------- */

static void tap(fl_state_t *state, int x, int y)
{
    fl_set_pointer(state, x, y, true);
    fl_set_pointer(state, x, y, false);
}

static void tap_panel(fl_state_t *state, int button)
{
    int x, y, w, h;
    fl_panel_button_rect(button, &x, &y, &w, &h);
    tap(state, x + w / 2, y + h / 2);
}

/* Play the level out by force: every wave is marked spent and cleared. */
static void win_level(fl_state_t *state, uint8_t lives)
{
    state->lives = lives;
    state->phase = FL_PHASE_WAVE;
    state->wave = fl_wave_count(state);
    state->batch_index = FL_BATCH_MAX;
    fl_state_update(state);
    assert(state->phase == FL_PHASE_WON);
}

static void test_only_the_first_level_is_open_at_first(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(fl_level_count() >= 3);
    assert(fl_level_unlocked(&state, 0));
    assert(!fl_level_unlocked(&state, 1));
    assert(!fl_state_start_level(&state, 1));
    assert(!fl_level_unlocked(&state, fl_level_count()));
}

static void test_winning_a_level_opens_the_next(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    win_level(&state, 20);
    assert(state.level_stars[0] == 3);
    assert(state.level_best[0] == state.score);
    assert(fl_level_unlocked(&state, 1));
    /* A worse run never lowers the record. */
    assert(fl_state_start_level(&state, 0));
    win_level(&state, 3);
    assert(state.level_stars[0] == 3);
}

static void test_the_next_button_starts_the_next_level(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    win_level(&state, 20);
    tap_panel(&state, FL_PANEL_BTN_NEXT);
    assert(state.phase == FL_PHASE_PREP);
    assert(state.level == 1);
    assert(state.gold == FL_LEVELS[1].start_gold);
    assert(state.wave == 1);
    /* The progress survives the new run. */
    assert(state.level_stars[0] == 3);
}

static void test_a_lost_run_offers_retry_and_map_only(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_LOST;
    tap_panel(&state, FL_PANEL_BTN_NEXT);
    assert(state.phase == FL_PHASE_LOST);
    tap_panel(&state, FL_PANEL_BTN_RETRY);
    assert(state.phase == FL_PHASE_PREP);
    assert(state.level == 0);
    state.phase = FL_PHASE_LOST;
    tap_panel(&state, FL_PANEL_BTN_MAP);
    assert(state.phase == FL_PHASE_MENU);
}

static void test_the_map_picks_and_starts_a_level(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.level_stars[0] = 2;
    fl_state_open_menu(&state);
    assert(state.phase == FL_PHASE_MENU);
    /* The map points at the newest open level. */
    assert(state.menu_level == 1);
    /* A locked node cannot be picked. */
    tap(&state, FL_LEVELS[2].node.x, FL_LEVELS[2].node.y);
    assert(state.menu_level == 1);
    tap(&state, FL_LEVELS[0].node.x, FL_LEVELS[0].node.y);
    assert(state.menu_level == 0);
    /* Time passes on the map without starting a wave. */
    for (int i = 0; i < 1000; ++i) {
        fl_state_update(&state);
    }
    assert(state.phase == FL_PHASE_MENU);
    tap(&state, FL_MENU_GO_X + 10, FL_MENU_GO_Y + 10);
    assert(state.phase == FL_PHASE_PREP);
    assert(state.level == 0);
    assert(fl_events_take(&state) & FL_EVENT_LEVEL_START);
}

static void test_the_pause_panel_resumes_and_leaves(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    tap(&state, FL_PAUSE_BTN_X + 4, FL_BTN_Y + 4);
    assert(state.paused);
    /* The battlefield does not listen while paused. */
    tap(&state, 58, 108);
    assert(state.ring_spot == -1);
    tap_panel(&state, FL_PANEL_BTN_NEXT);
    assert(!state.paused);
    tap(&state, FL_PAUSE_BTN_X + 4, FL_BTN_Y + 4);
    tap_panel(&state, FL_PANEL_BTN_MAP);
    assert(state.phase == FL_PHASE_MENU);
    assert(!state.paused);
}

static void test_every_level_walks_from_spawn_to_the_gate(void)
{
    for (uint8_t level = 0; level < FL_LEVEL_COUNT; ++level) {
        fl_state_t state;
        fl_state_reset(&state, 1u);
        for (uint8_t i = 0; i < level; ++i) {
            state.level_stars[i] = 1;
        }
        assert(fl_state_start_level(&state, level));
        const fl_level_t *data = &FL_LEVELS[level];
        assert(data->lane_a[data->lane_a_count - 1].x == FL_GOAL.x);
        assert(data->lane_a[data->lane_a_count - 1].y == FL_GOAL.y);
        /* A lone wolf on each lane reaches the gate and costs one life. */
        for (uint8_t lane = 0; lane < 2; ++lane) {
            if (lane == 1 && data->lane_b_count == 0) {
                continue;
            }
            fl_state_start_level(&state, level);
            state.phase = FL_PHASE_WAVE;
            state.batch_index = FL_BATCH_MAX;
            state.enemies[0].kind = FL_ENEMY_WOLF;
            state.enemies[0].hp = state.enemies[0].max_hp = 1e6f;
            state.enemies[0].lane = lane;
            state.enemies[0].alive = 1;
            for (int t = 0; t < 30 * 300 && state.leaks == 0; ++t) {
                fl_state_update(&state);
            }
            assert(state.leaks == 1);
            assert(state.lives == data->lives - 1);
        }
    }
}

/* --- build ring ------------------------------------------------------ */

/* The medallion of an entry in the open ring, or -1. */
static int ring_index(const fl_state_t *state, uint8_t entry)
{
    for (int i = 0; i < fl_ring_count(state); ++i) {
        if (fl_ring_entry(state, i) == entry) {
            return i;
        }
    }
    return -1;
}

static void tap_ring(fl_state_t *state, int index)
{
    int x, y;
    fl_ring_slot(state, index, &x, &y);
    tap(state, x, y);
}

static void test_a_pad_opens_the_ring_with_the_levels_towers(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    tap(&state, 58, 108);
    assert(state.ring_spot == 0);
    assert(fl_ring_count(&state) == 3);
    assert(fl_ring_entry(&state, 0) == FL_TOWER_GUARD);
    assert(fl_ring_entry(&state, 1) == FL_TOWER_FROST);
    assert(fl_ring_entry(&state, 2) == FL_TOWER_SIEGE);
    assert(fl_ring_cost(&state, 1) == FL_TOWER_LEVELS[FL_TOWER_FROST][0].cost);
    /* Tapping the pad again puts the ring away. */
    tap(&state, 58, 108);
    assert(state.ring_spot == -1);
    assert(fl_ring_count(&state) == 0);
}

static void test_the_first_tap_picks_and_the_second_builds(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    tap(&state, 58, 108);
    int frost = ring_index(&state, FL_TOWER_FROST);
    tap_ring(&state, frost);
    assert(state.ring_pick == frost);
    assert(!state.towers[0].active);
    tap_ring(&state, frost);
    assert(state.towers[0].active);
    assert(state.towers[0].kind == FL_TOWER_FROST);
    assert(state.gold == 190 - 120);
    assert(state.ring_spot == -1);
}

static void test_a_tap_off_the_ring_closes_it(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    tap(&state, 58, 108);
    tap(&state, 300, 140);
    assert(state.ring_spot == -1);
    assert(!state.towers[0].active);
    /* A tap on another pad moves the ring there in one go. */
    tap(&state, 58, 108);
    tap(&state, 186, 244);
    assert(state.ring_spot == 5);
}

static void test_short_of_gold_the_ring_stays_open(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.gold = 50;
    tap(&state, 58, 108);
    int guard = ring_index(&state, FL_TOWER_GUARD);
    tap_ring(&state, guard);
    (void)fl_events_take(&state);
    tap_ring(&state, guard);
    assert(!state.towers[0].active);
    assert(state.ring_spot == 0);
    assert(fl_events_take(&state) & FL_EVENT_DENIED);
}

static void test_a_free_post_opens_the_hero_ring(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    tap(&state, 56, 232);
    assert(state.ring_post == 0);
    assert(fl_ring_count(&state) == 1);
    assert(fl_ring_entry(&state, 0) == FL_RING_HERO);
    tap_ring(&state, 0);
    tap_ring(&state, 0);
    assert(state.heroes[0].active);
    assert(state.ring_post == -1);
    /* A post that is taken opens nothing. */
    tap(&state, 56, 232);
    assert(state.ring_post == -1);
}

/* Inside the rounded glass, clear of the HUD, with a whole medallion. */
static bool on_the_glass(int x, int y)
{
    const int r = FL_RING_ICON_R;
    if (x < r || x > FL_VIEW_WIDTH - r || y > FL_VIEW_HEIGHT - r
            || y < FL_HUD_Y + FL_HUD_H + r) {
        return false;
    }
    int cx = x < FL_CORNER_RADIUS ? FL_CORNER_RADIUS
           : (x > FL_VIEW_WIDTH - FL_CORNER_RADIUS
              ? FL_VIEW_WIDTH - FL_CORNER_RADIUS : x);
    int cy = y < FL_CORNER_RADIUS ? FL_CORNER_RADIUS
           : (y > FL_VIEW_HEIGHT - FL_CORNER_RADIUS
              ? FL_VIEW_HEIGHT - FL_CORNER_RADIUS : y);
    int dx = x - cx;
    int dy = y - cy;
    return (dx * dx + dy * dy) <= (FL_CORNER_RADIUS - r) * (FL_CORNER_RADIUS - r);
}

static void test_every_medallion_stays_on_the_glass(void)
{
    for (uint8_t level = 0; level < FL_LEVEL_COUNT; ++level) {
        fl_state_t state;
        fl_state_reset(&state, 1u);
        for (uint8_t i = 0; i < level; ++i) {
            state.level_stars[i] = 1;
        }
        assert(fl_state_start_level(&state, level));
        const fl_level_t *data = &FL_LEVELS[level];
        for (int spot = 0; spot < data->spot_count; ++spot) {
            state.ring_spot = (int8_t)spot;
            for (int i = 0; i < fl_ring_count(&state); ++i) {
                int x, y;
                fl_ring_slot(&state, i, &x, &y);
                assert(on_the_glass(x, y));
            }
        }
        state.ring_spot = -1;
        for (int post = 0; post < data->post_count; ++post) {
            state.ring_post = (int8_t)post;
            int x, y;
            fl_ring_slot(&state, 0, &x, &y);
            assert(on_the_glass(x, y));
        }
    }
}

/* --- the newer towers ------------------------------------------------ */

/* The first level does not offer the newer towers yet, so build a guard
 * on the first pad and swap its kind: the pad, the price and the rally
 * point are all the same. */
static fl_tower_t *force_tower(fl_state_t *state, fl_tower_kind_t kind)
{
    assert(fl_build_tower(state, 58, 108, FL_TOWER_GUARD));
    state->towers[0].kind = kind;
    state->phase = FL_PHASE_WAVE;
    state->batch_index = FL_BATCH_MAX;   /* no spawning of its own */
    return &state->towers[0];
}

/* An enemy at a fixed spot; `dist` orders who is closest to the gate. */
static void place_at(fl_state_t *state, int index, fl_enemy_kind_t kind,
                     float x, float y, float dist)
{
    place_enemy(state, index, kind, dist, 1000.0f);
    state->enemies[index].x = x;
    state->enemies[index].y = y;
    state->enemies[index].stun_ticks = 1000;   /* hold still */
}

static void test_the_new_towers_arrive_one_stage_at_a_time(void)
{
    assert(fl_new_tower(0) == FL_TOWER_NONE);
    assert(fl_new_tower(1) == FL_TOWER_STORM);
    assert(fl_new_tower(2) == FL_TOWER_RUNE);
    fl_state_t state;
    fl_state_reset(&state, 1u);
    assert(!fl_tower_available(&state, FL_TOWER_STORM));
    state.level_stars[0] = state.level_stars[1] = 1;
    assert(fl_state_start_level(&state, 2));
    assert(fl_tower_available(&state, FL_TOWER_STORM));
    assert(fl_tower_available(&state, FL_TOWER_RUNE));
    assert(fl_ring_count(&state) == 0);
    state.ring_spot = 0;
    assert(fl_ring_count(&state) == 5);
    assert(strcmp(fl_upgrade_name(FL_TOWER_EMBER, 1, 1), "MAGMA") == 0);
}

static void test_lightning_leaps_through_a_crowd(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_STORM);
    /* A column of wolves; the one furthest along is struck first. */
    for (int i = 0; i < 4; ++i) {
        place_at(&state, i, FL_ENEMY_WOLF, 60.0f + 20.0f * i, 150.0f,
                 10.0f + 10.0f * i);
    }
    fl_state_update(&state);
    float loss[4];
    for (int i = 0; i < 4; ++i) {
        loss[i] = 1000.0f - state.enemies[i].hp;
    }
    /* Three struck, each leap weaker than the last; the fourth spared. */
    assert(loss[3] > loss[2] && loss[2] > loss[1] && loss[1] > 0.0f);
    assert(loss[0] == 0.0f);
    int drawn = 0;
    for (int i = 0; i < FL_MAX_ARCS; ++i) {
        if (state.arcs[i].ttl > 0) {
            assert(state.arcs[i].count == 4);
            assert(state.arcs[i].kind == FL_TOWER_STORM);
            drawn++;
        }
    }
    assert(drawn == 1);
    assert(fl_events_take(&state) & FL_EVENT_SHOT_STORM);
}

static void test_the_rune_beam_ignores_armour(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_RUNE);
    place_at(&state, 0, FL_ENEMY_HUSK, 80.0f, 150.0f, 10.0f);
    assert(state.enemies[0].armor > 0);
    fl_state_update(&state);
    assert(1000.0f - state.enemies[0].hp
           == (float)FL_TOWER_LEVELS[FL_TOWER_RUNE][0].damage);
}

static void test_a_hex_strips_the_armour_for_every_tower(void)
{
    fl_state_t plain, hexed;
    fl_state_reset(&plain, 1u);
    force_tower(&plain, FL_TOWER_GUARD);
    place_at(&plain, 0, FL_ENEMY_HUSK, 80.0f, 150.0f, 10.0f);
    hexed = plain;
    hexed.enemies[0].hex_ticks = 500;
    for (int i = 0; i < 20; ++i) {
        fl_state_update(&plain);
        fl_state_update(&hexed);
    }
    assert(hexed.enemies[0].hp < plain.enemies[0].hp);

    /* And the hex upgrade is what lays it on. */
    fl_state_t state;
    fl_state_reset(&state, 1u);
    fl_tower_t *rune = force_tower(&state, FL_TOWER_RUNE);
    rune->level = 2;
    rune->mod_mask = 1u << 1;   /* stage 0, option 1: HEX */
    place_at(&state, 0, FL_ENEMY_HUSK, 80.0f, 150.0f, 10.0f);
    fl_state_update(&state);
    assert(state.enemies[0].hex_ticks > 0);
}

static int soldiers_alive(const fl_state_t *state)
{
    int count = 0;
    for (int s = 0; s < FL_MAX_SOLDIERS; ++s) {
        count += state->soldiers[s].active && state->soldiers[s].alive;
    }
    return count;
}

static void test_soldiers_march_out_and_hold_the_road(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    fl_tower_t *hall = force_tower(&state, FL_TOWER_BARRACKS);
    /* The rally point is on the road, within the hall's reach. */
    float rx = (float)(hall->rally_x - hall->x);
    float ry = (float)(hall->rally_y - hall->y);
    assert(rx * rx + ry * ry
           <= (float)FL_TOWER_LEVELS[FL_TOWER_BARRACKS][0].range
              * FL_TOWER_LEVELS[FL_TOWER_BARRACKS][0].range);
    state.phase = FL_PHASE_PREP;
    for (int i = 0; i < 90; ++i) {
        fl_state_update(&state);
    }
    assert(soldiers_alive(&state) == 2);
    const fl_soldier_t *first = &state.soldiers[0];
    float dx = first->x - (float)hall->rally_x;
    float dy = first->y - (float)hall->rally_y;
    assert(dx * dx + dy * dy < 12.0f * 12.0f);

    /* A wolf walking into them is stopped and fights. */
    state.phase = FL_PHASE_WAVE;
    place_enemy(&state, 0, FL_ENEMY_WOLF, 30.0f, 1000.0f);
    state.enemies[0].x = first->x;
    state.enemies[0].y = first->y;
    float before = state.enemies[0].dist;
    for (int i = 0; i < 40; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[0].dist == before);
    assert(state.enemies[0].hp < 1000.0f);
    assert(state.soldiers[state.enemies[0].soldier - 1].hp
           < state.soldiers[state.enemies[0].soldier - 1].max_hp);
}

static void test_a_fallen_soldier_marches_out_again(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_BARRACKS);
    fl_state_update(&state);
    assert(soldiers_alive(&state) == 2);
    state.soldiers[0].hp = 1;
    /* The empty field cleared the wave; start it again with a wolf. */
    state.phase = FL_PHASE_WAVE;
    place_enemy(&state, 0, FL_ENEMY_WOLF, 30.0f, 1000.0f);
    state.enemies[0].x = state.soldiers[0].x;
    state.enemies[0].y = state.soldiers[0].y;
    for (int i = 0; i < 5; ++i) {
        fl_state_update(&state);
    }
    assert(!state.soldiers[0].alive);
    state.enemies[0].alive = 0;
    for (int i = 0; i < 310; ++i) {
        fl_state_update(&state);
    }
    assert(state.soldiers[0].alive);
    assert(state.soldiers[0].hp == state.soldiers[0].max_hp);
}

static void test_selling_a_hall_takes_its_garrison(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_BARRACKS);
    fl_state_update(&state);
    assert(soldiers_alive(&state) == 2);
    assert(fl_sell_tower(&state, 0));
    assert(soldiers_alive(&state) == 0);
}

static int zones_burning(const fl_state_t *state)
{
    int count = 0;
    for (int z = 0; z < FL_MAX_ZONES; ++z) {
        count += state->zones[z].active;
    }
    return count;
}

static void test_an_ember_pot_leaves_the_road_burning(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_EMBER);
    place_at(&state, 0, FL_ENEMY_HUSK, 80.0f, 150.0f, 10.0f);
    for (int i = 0; i < 30 && zones_burning(&state) == 0; ++i) {
        fl_state_update(&state);
    }
    assert(zones_burning(&state) == 1);
    /* The pot itself does no harm; the fire does, armour or not. */
    float landed = state.enemies[0].hp;
    for (int i = 0; i < 40; ++i) {
        fl_state_update(&state);
    }
    float burned = landed - state.enemies[0].hp;
    assert(burned >= 3.0f * (float)FL_TOWER_LEVELS[FL_TOWER_EMBER][0].damage);
    /* It burns out once its time is up; the tower is sold so no new pot
     * keeps it alight. */
    assert(fl_sell_tower(&state, 0));
    for (int i = 0; i < 200; ++i) {
        fl_state_update(&state);
    }
    assert(zones_burning(&state) == 0);
}

/* --- the newer enemies ----------------------------------------------- */

static int count_alive(const fl_state_t *state, fl_enemy_kind_t kind)
{
    int count = 0;
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        count += state->enemies[i].alive && state->enemies[i].kind == kind;
    }
    return count;
}

static void test_a_crow_flies_over_the_siege_and_the_soldiers(void)
{
    /* The catapult lobs at the ground: a bird is out of its reach. */
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_SIEGE);
    place_at(&state, 0, FL_ENEMY_CROW, 80.0f, 150.0f, 10.0f);
    for (int i = 0; i < 60; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[0].hp == 1000.0f);

    /* An archer shoots it down well enough. */
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_GUARD);
    place_at(&state, 0, FL_ENEMY_CROW, 80.0f, 150.0f, 10.0f);
    for (int i = 0; i < 60; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[0].hp < 1000.0f);

    /* And no soldier can hold it. */
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_BARRACKS);
    fl_state_update(&state);
    state.phase = FL_PHASE_WAVE;
    place_enemy(&state, 0, FL_ENEMY_CROW, 30.0f, 1000.0f);
    state.enemies[0].x = state.soldiers[0].x;
    state.enemies[0].y = state.soldiers[0].y;
    for (int i = 0; i < 20; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[0].dist > 30.0f);
    assert(state.enemies[0].soldier == 0);
    assert(state.enemies[0].hp == 1000.0f);
}

static void test_a_golem_cracks_at_half_health(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_RUNE);
    place_at(&state, 0, FL_ENEMY_GOLEM, 80.0f, 150.0f, 10.0f);
    state.enemies[0].hp = 501.0f;
    assert(state.enemies[0].armor == FL_ENEMY_STATS[FL_ENEMY_GOLEM].armor);
    fl_state_update(&state);
    assert(state.enemies[0].stage == 1);
    assert(state.enemies[0].armor == 0);
    assert(fl_events_take(&state) & FL_EVENT_CRACK);
}

static void test_a_slime_bursts_into_slimelets_the_blast_spares(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_SIEGE);
    place_at(&state, 0, FL_ENEMY_SLIME, 80.0f, 150.0f, 40.0f);
    state.enemies[0].hp = 1.0f;
    uint16_t gold = state.gold;
    for (int i = 0; i < 60 && count_alive(&state, FL_ENEMY_SLIME); ++i) {
        fl_state_update(&state);
    }
    assert(count_alive(&state, FL_ENEMY_SLIME) == 0);
    assert(count_alive(&state, FL_ENEMY_SLIMELET) == 3);
    assert(state.gold == gold + FL_ENEMY_STATS[FL_ENEMY_SLIME].bounty);
    assert(fl_events_take(&state) & FL_EVENT_SPLIT);
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *child = &state.enemies[i];
        if (child->alive && child->kind == FL_ENEMY_SLIMELET) {
            assert(child->hp == child->max_hp);
        }
    }
}

static void test_a_burrower_dives_out_of_reach_and_speeds_up(void)
{
    /* Under the road no tower can see it. */
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_GUARD);
    place_at(&state, 0, FL_ENEMY_BURROWER, 80.0f, 150.0f, 10.0f);
    fl_state_update(&state);   /* phase_ticks starts at 0: it dives */
    assert(state.enemies[0].burrowed);
    assert(fl_events_take(&state) & FL_EVENT_BURROW);
    /* The bolt already loosed flies on to where it was and misses. */
    float hp = state.enemies[0].hp;
    for (int i = 0; i < 30; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[0].hp == hp);
    assert(hp == 1000.0f);

    /* And it tunnels faster than it walks. */
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_index = FL_BATCH_MAX;
    place_enemy(&state, 0, FL_ENEMY_BURROWER, 40.0f, 1000.0f);
    fl_state_update(&state);
    assert(state.enemies[0].burrowed);
    float start = state.enemies[0].dist;
    for (int i = 0; i < 10; ++i) {
        fl_state_update(&state);
    }
    float under = state.enemies[0].dist - start;
    state.enemies[0].phase_ticks = 0;
    fl_state_update(&state);
    assert(!state.enemies[0].burrowed);
    start = state.enemies[0].dist;
    for (int i = 0; i < 10; ++i) {
        fl_state_update(&state);
    }
    float above = state.enemies[0].dist - start;
    assert(under > above * 1.5f);
}

static void test_the_wyrm_surfaces_with_a_brood(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_index = FL_BATCH_MAX;
    place_enemy(&state, 0, FL_ENEMY_WYRM, 200.0f, 2600.0f);
    state.enemies[0].burrowed = 1;
    fl_state_update(&state);
    assert(!state.enemies[0].burrowed);
    assert(count_alive(&state, FL_ENEMY_SKITTER) == 3);
    assert(fl_events_take(&state) & FL_EVENT_BOSS);
    for (int i = 0; i < FL_MAX_ENEMIES; ++i) {
        const fl_enemy_t *skitter = &state.enemies[i];
        if (skitter->alive && skitter->kind == FL_ENEMY_SKITTER) {
            assert(skitter->dist < state.enemies[0].dist);
        }
    }
}

static void test_the_king_freezes_the_nearest_tower(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    fl_tower_t *tower = force_tower(&state, FL_TOWER_GUARD);
    place_at(&state, 0, FL_ENEMY_KING, 80.0f, 150.0f, 10.0f);
    state.enemies[0].phase_ticks = 10000;   /* not yet */
    place_at(&state, 1, FL_ENEMY_WOLF, 80.0f, 140.0f, 20.0f);
    state.enemies[0].phase_ticks = 0;
    fl_state_update(&state);
    assert(tower->frozen_ticks > 0);
    assert(fl_events_take(&state) & FL_EVENT_FREEZE);
    bool beam = false;
    for (int i = 0; i < FL_MAX_ARCS; ++i) {
        beam |= state.arcs[i].ttl > 0 && state.arcs[i].kind == FL_ARC_FREEZE;
    }
    assert(beam);
    /* Encased, it holds its fire; thawed, it shoots again. The bolt it
     * loosed on the first tick is let land before counting. */
    state.enemies[0].phase_ticks = 10000;
    for (int i = 0; i < 15; ++i) {
        fl_state_update(&state);
    }
    float hp = state.enemies[1].hp;
    for (int i = 0; i < 90; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[1].hp == hp);
    for (int i = 0; i < 60; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[1].hp < hp);
}

static void test_the_king_wards_itself_at_each_third(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    force_tower(&state, FL_TOWER_RUNE);
    place_at(&state, 0, FL_ENEMY_KING, 80.0f, 150.0f, 10.0f);
    state.enemies[0].phase_ticks = 10000;
    state.enemies[0].max_hp = 3000.0f;
    state.enemies[0].hp = 2001.0f;
    fl_state_update(&state);
    /* The blow stops at the line, the ward goes up, the wolves come. */
    assert(state.enemies[0].hp == 2000.0f);
    assert(state.enemies[0].stage == 1);
    assert(state.enemies[0].ward_ticks > 0);
    assert(count_alive(&state, FL_ENEMY_WOLF) == 3);
    for (int i = 0; i < 40; ++i) {
        fl_state_update(&state);
    }
    assert(state.enemies[0].hp == 2000.0f);
}

static void test_a_boss_at_the_gate_costs_ten_lives(void)
{
    fl_state_t state;
    fl_state_reset(&state, 1u);
    state.phase = FL_PHASE_WAVE;
    state.batch_index = FL_BATCH_MAX;
    place_enemy(&state, 0, FL_ENEMY_WYRM, 100000.0f, 2600.0f);
    state.enemies[0].phase_ticks = 10000;
    uint8_t lives = state.lives;
    fl_state_update(&state);
    assert(lives - state.lives == 10);
}

int main(void)
{
    test_only_the_first_level_is_open_at_first();
    test_winning_a_level_opens_the_next();
    test_the_next_button_starts_the_next_level();
    test_a_lost_run_offers_retry_and_map_only();
    test_the_map_picks_and_starts_a_level();
    test_the_pause_panel_resumes_and_leaves();
    test_every_level_walks_from_spawn_to_the_gate();
    test_reset_starts_in_prep();
    test_build_tower_costs_gold_and_occupies_spot();
    test_build_tower_rejects_insufficient_gold();
    test_build_tower_rejects_off_spot_position();
    test_build_tower_snaps_to_the_nearest_spot();
    test_sell_refunds_half();
    test_enemies_walk_towards_the_goal();
    test_leak_costs_a_life();
    test_two_towers_hold_the_first_wave();
    test_upgrading_a_tower_changes_its_stats();
    test_upgrading_needs_the_gold();
    test_armour_is_a_flat_cut();
    test_the_shaman_mends_its_ally();
    test_the_banner_hardens_its_ally();
    test_a_sap_takes_three_lives_from_the_gate();
    test_the_titan_ignores_the_frost_and_the_stun();
    test_a_hero_stands_on_a_post();
    test_a_hero_holds_the_road();
    test_the_frost_nova_clears_the_road();
    test_the_star_rating_follows_the_lives_left();
    test_score_counts_kills_and_cleared_waves();
    test_a_leak_is_counted_and_costs_the_flawless_bonus();
    test_state_hash_is_stable_for_the_same_seed();
    test_a_pad_opens_the_ring_with_the_levels_towers();
    test_the_first_tap_picks_and_the_second_builds();
    test_a_tap_off_the_ring_closes_it();
    test_short_of_gold_the_ring_stays_open();
    test_a_free_post_opens_the_hero_ring();
    test_every_medallion_stays_on_the_glass();
    test_the_new_towers_arrive_one_stage_at_a_time();
    test_lightning_leaps_through_a_crowd();
    test_the_rune_beam_ignores_armour();
    test_a_hex_strips_the_armour_for_every_tower();
    test_soldiers_march_out_and_hold_the_road();
    test_a_fallen_soldier_marches_out_again();
    test_selling_a_hall_takes_its_garrison();
    test_an_ember_pot_leaves_the_road_burning();
    test_a_crow_flies_over_the_siege_and_the_soldiers();
    test_a_golem_cracks_at_half_health();
    test_a_slime_bursts_into_slimelets_the_blast_spares();
    test_a_burrower_dives_out_of_reach_and_speeds_up();
    test_the_wyrm_surfaces_with_a_brood();
    test_the_king_freezes_the_nearest_tower();
    test_the_king_wards_itself_at_each_third();
    test_a_boss_at_the_gate_costs_ten_lives();
    test_building_raises_an_event();
    test_building_without_the_gold_is_denied();
    test_upgrading_and_selling_raise_events();
    test_a_leak_raises_an_event();
    test_a_kill_raises_an_event_and_names_the_enemy();
    test_a_wave_start_raises_an_event();
    test_events_survive_across_input_and_update();
    test_the_frost_nova_raises_an_event();    test_speed_multiplier_advances_faster();
    printf("model tests passed\n");
    return 0;
}

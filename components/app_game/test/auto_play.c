/* Balance probe for Frostline Hold.
 *
 * Plays the level with a simple greedy shopper and reports how far it gets.
 * It is a tuning tool, not part of the game: nothing here ships.
 *
 * Build: gcc -std=c11 -O2 -Iinclude -Idata -o /tmp/auto /tmp/auto.c \
 *            frostline_model.c -lm
 * Usage: /tmp/auto [shop_order] [branch_mask]
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "frostline_model.h"
#include "../data/frostline_level.h"

#define TOWER_COUNT 14

/* Which branch to take at each stage, per tower kind. Index 0 is stage 0. */
static int branch_pick[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};

/* The spots in the order the probe builds them: the early ones cover more of
 * the road because the waves start at the north-west gate. */
/* Spots 0-8 sit on lane A and 9-13 on lane B; a player who ignores the
 * second lane loses every enemy that walks down it. */
static const int SPOT_ORDER[] = {0, 9, 1, 10, 2, 11, 3, 12, 4, 13, 5, 6, 7, 8};
#define SPOT_COUNT ((int)(sizeof(SPOT_ORDER) / sizeof(SPOT_ORDER[0])))

static int tower_at_spot(const fl_state_t *state, int spot)
{
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        const fl_tower_t *tower = &state->towers[i];
        if (tower->active && tower->x == FL_BUILD_SPOTS[spot].x
                && tower->y == FL_BUILD_SPOTS[spot].y) {
            return i;
        }
    }
    return -1;
}

/* 0 = upgrade first, 1 = build first. */
static int g_kind_mask = 7;   /* bit 0 guard, bit 1 frost, bit 2 siege */

static int try_build(fl_state_t *state)
{
    for (int s = 0; s < SPOT_COUNT; ++s) {
        int spot = SPOT_ORDER[s];
        if (tower_at_spot(state, spot) >= 0) {
            continue;
        }
        /* The cheapest tower first; a frost tower joins the mix later. */
        int built = 0;
        for (int i = 0; i < FL_MAX_TOWERS; ++i) {
            if (state->towers[i].active) {
                built++;
            }
        }
        /* A mixed board: two cheap guards first, then a rotation. */
        static const fl_tower_kind_t MIX[4] = {
            FL_TOWER_GUARD, FL_TOWER_GUARD, FL_TOWER_FROST, FL_TOWER_SIEGE
        };
        fl_tower_kind_t kind = MIX[built % 4];
        if (!(g_kind_mask & (1 << (kind - 1)))) {
            kind = (g_kind_mask & 1) ? FL_TOWER_GUARD
                 : ((g_kind_mask & 2) ? FL_TOWER_FROST : FL_TOWER_SIEGE);
        }
        if (fl_build_tower(state, FL_BUILD_SPOTS[spot].x,
                           FL_BUILD_SPOTS[spot].y, kind)) {
            return 1;
        }
        return 0;
    }
    return 0;
}

static int try_upgrade(fl_state_t *state)
{
    int best = -1;
    uint16_t best_cost = 0xFFFF;
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        if (!state->towers[i].active
                || state->towers[i].level >= FL_TOWER_LEVEL_COUNT) {
            continue;
        }
        uint16_t cost = fl_upgrade_cost(state, i);
        if (cost == 0) {
            continue;   /* already at the top level */
        }
        if (cost < best_cost && state->gold >= cost) {
            best_cost = cost;
            best = i;
        }
    }
    if (best < 0) {
        return 0;
    }
    int kind = state->towers[best].kind;
    int stage = state->towers[best].level - 1;
    return fl_upgrade_tower(state, best, branch_pick[kind][stage]) ? 1 : 0;
}

static void shop(fl_state_t *state, int shop_order)
{
    for (int round = 0; round < 40; ++round) {
        bool spent = false;
        if (shop_order == 0) {
            spent = try_build(state) != 0;
            spent = (try_upgrade(state) != 0) || spent;
        } else {
            spent = (try_upgrade(state) != 0);
            spent = (try_build(state) != 0) || spent;
        }
        if (!spent) {
            break;
        }
    }
}

int main(int argc, char **argv)
{
    int shop_order = (argc > 1) ? atoi(argv[1]) : 0;
    if (argc > 3) {
        g_kind_mask = atoi(argv[3]);
    }
    if (argc > 2) {
        int mask = atoi(argv[2]);
        for (int kind = 1; kind <= 3; ++kind) {
            branch_pick[kind][0] = (mask >> (kind - 1) * 2) & 1;
            branch_pick[kind][1] = (mask >> ((kind - 1) * 2 + 1)) & 1;
        }
    }

    fl_state_t state;
    fl_state_reset(&state, 7u);
    int last_wave = 1;
    bool shopping = true;
    for (int tick = 0; tick < 200000 && state.phase != FL_PHASE_WON
            && state.phase != FL_PHASE_LOST; ++tick) {
        if (state.phase == FL_PHASE_PREP && shopping) {
            shop(&state, shop_order);
            shopping = false;
        }
        if (state.phase == FL_PHASE_WAVE) {
            shopping = true;
        }
        fl_state_update(&state);
        if ((int)state.wave != last_wave) {
            printf("  wave %2d: lives %2u  gold %4u  kills %3u\n", last_wave,
                   (unsigned)state.lives, (unsigned)state.gold,
                   (unsigned)state.kills);
            last_wave = state.wave;
        }
    }

    int built = 0;
    int level_sum = 0;
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        if (state.towers[i].active) {
            built++;
            level_sum += state.towers[i].level;
        }
    }
    printf("levels:");
    for (int i = 0; i < FL_MAX_TOWERS; ++i) {
        if (state.towers[i].active) {
            printf(" %u/m%d", (unsigned)state.towers[i].level, state.towers[i].kind);
        }
    }
    printf("\n");
    printf("shop=%d mask=%d -> %s wave=%u lives=%u kills=%u gold=%u "
           "score=%u towers=%d avg_lv=%.1f ticks=%u\n",
           shop_order, (argc > 2) ? atoi(argv[2]) : 0,
           fl_phase_name(&state), (unsigned)state.wave, (unsigned)state.lives,
           (unsigned)state.kills, (unsigned)state.gold, (unsigned)state.score,
           built, built ? (double)level_sum / built : 0.0,
           (unsigned)state.tick);
    return 0;
}

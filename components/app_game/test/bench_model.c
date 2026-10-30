/* Balance benchmark for Frostline Hold.
 *
 * Runs the model headlessly and reports how far the battle gets with no
 * towers and with a standard opening layout. The numbers form the baseline
 * that later tuning is measured against.
 *
 * Build and run:
 *   gcc -std=c11 -O2 -Iinclude -Idata -o /tmp/bench test/bench_model.c \
 *       frostline_model.c -lm && /tmp/bench
 *
 * SPDX-License-Identifier: Apache-2.0 */
#include <stdio.h>

#include "frostline_model.h"

#define MAX_TICKS 30000

static const int16_t STANDARD_LAYOUT[][2] = {
    {58, 108}, {50, 192}, {118, 178}, {118, 250}, {186, 186}, {186, 244},
};

static int run(const char *label, const int16_t (*spots)[2], size_t count)
{
    fl_state_t state;
    fl_state_reset(&state, 0x464C5354u);
    for (size_t i = 0; i < count; ++i) {
        fl_build_tower(&state, spots[i][0], spots[i][1], FL_TOWER_GUARD);
    }

    int ticks = 0;
    while (ticks < MAX_TICKS && state.phase != FL_PHASE_WON
            && state.phase != FL_PHASE_LOST) {
        fl_state_update(&state);
        ++ticks;
    }

    printf("%-16s towers=%zu waves=%2u lives=%2u kills=%3lu gold=%3u ticks=%5d result=%s\n",
           label, count, (unsigned)state.wave, (unsigned)state.lives,
           (unsigned long)state.kills, (unsigned)state.gold, ticks,
           fl_phase_name(&state));
    return (int)state.wave;
}

int main(void)
{
    printf("Frostline Hold balance baseline (480x480, 15 waves, 20 lives)\n");
    int bare = run("no towers", NULL, 0);
    int standard = run("standard layout", STANDARD_LAYOUT,
                       sizeof(STANDARD_LAYOUT) / sizeof(STANDARD_LAYOUT[0]));
    printf("BASELINE bare_waves=%d standard_waves=%d\n", bare, standard);
    return 0;
}

// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The best run the player has managed on this device. */
typedef struct {
    uint32_t best_score;
    uint32_t best_wave;
    uint8_t best_stars;
    uint8_t version;
} frostline_record_t;

/* Load the stored record. A missing or damaged record reads as zeroes. */
bool frostline_record_load(frostline_record_t *record);

/* Store the record. On the device this goes to NVS, on the Host to a file
 * next to the packed assets. */
bool frostline_record_store(const frostline_record_t *record);

/* The Host keeps its record in a file; this sets where. The device ignores
 * it and uses NVS. */
void frostline_record_set_host_path(const char *path);

#ifdef __cplusplus
}
#endif

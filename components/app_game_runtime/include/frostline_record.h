// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FROSTLINE_RECORD_VERSION 2
#define FROSTLINE_RECORD_LEVELS 8   /* matches FL_MAX_LEVELS in the model */

/* The campaign as the player left it on this device: per level, the stars
 * earned (0 = not cleared yet) and the best score. */
typedef struct {
    uint32_t version;
    uint32_t best_score[FROSTLINE_RECORD_LEVELS];
    uint8_t stars[FROSTLINE_RECORD_LEVELS];
} frostline_record_t;

/* Load the stored record. A missing or damaged record reads as zeroes; a
 * record from the single-level game becomes the first level's entry. */
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

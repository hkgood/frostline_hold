// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
#pragma once
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FROSTLINE_RECORD_VERSION 3
#define FROSTLINE_RECORD_LEVELS 8   /* matches FL_MAX_LEVELS in the model */

/* The campaign as the player left it on this device: per level, the stars
 * earned (0 = not cleared yet) and the best score; which enemies the
 * player has been introduced to; and the settings. */
typedef struct {
    uint32_t version;
    uint32_t best_score[FROSTLINE_RECORD_LEVELS];
    uint8_t stars[FROSTLINE_RECORD_LEVELS];
    /* Version 3 from here on. */
    uint32_t seen_enemies;   /* one bit per enemy kind */
    uint8_t music;           /* 0..4 */
    uint8_t sound;           /* 0..4 */
    uint8_t haptics;
    uint8_t lang;            /* 0 = Chinese, 1 = English */
    uint8_t has_settings;    /* 0 until the settings were first stored */
    uint8_t reserved[3];
} frostline_record_t;

/* Load the stored record. A missing or damaged record reads as zeroes; a
 * record from the single-level game becomes the first level's entry, and a
 * version 2 record keeps its campaign with default settings. */
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

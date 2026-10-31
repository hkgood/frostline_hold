// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
/* Where the campaign progress is kept.
 *
 * On the device the record lives in NVS, so it survives a reboot and a
 * reflash of the application partition. On the Host it is a small file next
 * to the packed assets, which keeps the simulator honest: the same code path
 * reads and writes a record, and the automation can inspect it.
 *
 * Version 1 held a single best run. Version 2 holds one entry per level; a
 * version 1 record is read as the first level's entry so nothing is lost.
 */
#include <stdio.h>
#include <string.h>

#include "include/frostline_record.h"

#define FROSTLINE_NVS_NAMESPACE "frostline"
#define FROSTLINE_NVS_KEY "campaign"

static void frostline_record_clear(frostline_record_t *record)
{
    memset(record, 0, sizeof(*record));
    record->version = FROSTLINE_RECORD_VERSION;
}

/* A version 1 record: the best score, the wave it reached, its stars. */
static void frostline_record_from_v1(frostline_record_t *record,
                                     uint32_t score, uint8_t stars)
{
    frostline_record_clear(record);
    record->best_score[0] = score;
    record->stars[0] = stars > 3 ? 3 : stars;
}

#if defined(RAYLIB_LITE_GAME_NATIVE)
#include "nvs_flash.h"

static bool frostline_nvs_open(nvs_handle_t *handle)
{
    esp_err_t error = nvs_open(FROSTLINE_NVS_NAMESPACE, NVS_READWRITE, handle);
    return error == ESP_OK;
}

bool frostline_record_load(frostline_record_t *record)
{
    if (!record) {
        return false;
    }
    frostline_record_clear(record);
    nvs_handle_t handle;
    if (!frostline_nvs_open(&handle)) {
        return false;
    }
    size_t size = sizeof(*record);
    esp_err_t error = nvs_get_blob(handle, FROSTLINE_NVS_KEY, record, &size);
    if (error == ESP_OK && size == sizeof(*record)
            && record->version == FROSTLINE_RECORD_VERSION) {
        nvs_close(handle);
        return true;
    }
    /* No campaign yet: carry over the single-level record if there is one. */
    uint32_t score = 0;
    uint8_t stars = 0;
    error = nvs_get_u32(handle, "score", &score);
    nvs_get_u8(handle, "stars", &stars);
    nvs_close(handle);
    if (error != ESP_OK) {
        frostline_record_clear(record);
        return false;
    }
    frostline_record_from_v1(record, score, stars);
    return true;
}

bool frostline_record_store(const frostline_record_t *record)
{
    if (!record) {
        return false;
    }
    nvs_handle_t handle;
    if (!frostline_nvs_open(&handle)) {
        return false;
    }
    nvs_set_blob(handle, FROSTLINE_NVS_KEY, record, sizeof(*record));
    esp_err_t error = nvs_commit(handle);
    nvs_close(handle);
    return error == ESP_OK;
}

void frostline_record_set_host_path(const char *path)
{
    (void)path;
}

#else /* Host */

static char g_host_path[512];

void frostline_record_set_host_path(const char *path)
{
    if (!path) {
        return;
    }
    snprintf(g_host_path, sizeof(g_host_path), "%s", path);
}

/* The version 1 file was the raw struct: score, wave, stars, version. */
typedef struct {
    uint32_t best_score;
    uint32_t best_wave;
    uint8_t best_stars;
    uint8_t version;
} frostline_record_v1_t;

bool frostline_record_load(frostline_record_t *record)
{
    if (!record) {
        return false;
    }
    frostline_record_clear(record);
    if (!g_host_path[0]) {
        return false;
    }
    FILE *file = fopen(g_host_path, "rb");
    if (!file) {
        return false;
    }
    unsigned char bytes[sizeof(frostline_record_t) + 1];
    size_t size = fread(bytes, 1, sizeof(bytes), file);
    fclose(file);
    if (size == sizeof(frostline_record_t)) {
        memcpy(record, bytes, sizeof(*record));
        if (record->version == FROSTLINE_RECORD_VERSION) {
            return true;
        }
    } else if (size == sizeof(frostline_record_v1_t)) {
        frostline_record_v1_t old;
        memcpy(&old, bytes, sizeof(old));
        if (old.version == 1) {
            frostline_record_from_v1(record, old.best_score, old.best_stars);
            return true;
        }
    }
    frostline_record_clear(record);
    return false;
}

bool frostline_record_store(const frostline_record_t *record)
{
    if (!record || !g_host_path[0]) {
        return false;
    }
    FILE *file = fopen(g_host_path, "wb");
    if (!file) {
        return false;
    }
    bool ok = fwrite(record, sizeof(*record), 1, file) == 1;
    fclose(file);
    return ok;
}

#endif

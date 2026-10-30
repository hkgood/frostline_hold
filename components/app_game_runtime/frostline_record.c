// SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary
/* Where the best run is kept.
 *
 * On the device the record lives in NVS, so it survives a reboot and a
 * reflash of the application partition. On the Host it is a small file next
 * to the packed assets, which keeps the simulator honest: the same code path
 * reads and writes a record, and the automation can inspect it.
 */
#include <stdio.h>
#include <string.h>

#include "include/frostline_record.h"

#define FROSTLINE_NVS_NAMESPACE "frostline"
#define FROSTLINE_NVS_KEY "best"

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
    memset(record, 0, sizeof(*record));
    record->version = 1;
    nvs_handle_t handle;
    if (!frostline_nvs_open(&handle)) {
        return false;
    }
    uint32_t score = 0;
    uint32_t wave = 0;
    uint8_t stars = 0;
    esp_err_t error = nvs_get_u32(handle, "score", &score);
    if (error != ESP_OK) {
        nvs_close(handle);
        return false;
    }
    nvs_get_u32(handle, "wave", &wave);
    nvs_get_u8(handle, "stars", &stars);
    nvs_close(handle);
    record->best_score = score;
    record->best_wave = wave;
    record->best_stars = stars;
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
    nvs_set_u32(handle, "score", record->best_score);
    nvs_set_u32(handle, "wave", record->best_wave);
    nvs_set_u8(handle, "stars", record->best_stars);
    esp_err_t error = nvs_commit(handle);
    nvs_close(handle);
    return error == ESP_OK;
}

void frostline_record_set_host_path(const char *path)
{
    (void)path;
}

#else /* Host */

#include <stdlib.h>

static char g_host_path[512];

void frostline_record_set_host_path(const char *path)
{
    if (!path) {
        return;
    }
    snprintf(g_host_path, sizeof(g_host_path), "%s", path);
}

bool frostline_record_load(frostline_record_t *record)
{
    if (!record) {
        return false;
    }
    memset(record, 0, sizeof(*record));
    record->version = 1;
    if (!g_host_path[0]) {
        return false;
    }
    FILE *file = fopen(g_host_path, "rb");
    if (!file) {
        return false;
    }
    bool ok = fread(record, sizeof(*record), 1, file) == 1;
    fclose(file);
    if (!ok || record->version != 1) {
        memset(record, 0, sizeof(*record));
        record->version = 1;
        return false;
    }
    return true;
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

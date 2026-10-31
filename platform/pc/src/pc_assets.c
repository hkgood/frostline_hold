/* The asset store of the PC build: the packed files sit in one folder (the
 * app bundle's Resources on macOS, beside the .exe on Windows, the preloaded
 * /assets folder on the web). Each file is read once, on first use, and kept
 * for the life of the process: the game holds views into it.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "raylib_lite_assets.h"
#include "host_asset_runtime.h"

/* Every atlas, sound and theme the game ships, with room to spare. */
#define PC_ASSET_SLOTS 64

typedef struct {
    char name[64];
    uint8_t *data;
    size_t size;
} pc_asset_t;

static char s_root[1024];
static pc_asset_t s_assets[PC_ASSET_SLOTS];

void raylib_lite_host_assets_set_root(const char *root)
{
    snprintf(s_root, sizeof(s_root), "%s", root ? root : "");
}

/* FNV-1a, the hash the asset packer uses for its ids. */
raylib_lite_asset_id_t raylib_lite_asset_id(const char *name)
{
    uint32_t value = 2166136261U;
    if (!name) {
        return 0;
    }
    while (*name) {
        value = (value ^ (uint8_t)*name++) * 16777619U;
    }
    return value;
}

raylib_lite_result_t raylib_lite_assets_mount(const raylib_lite_asset_store_config_t *config)
{
    (void)config;
    return RAYLIB_LITE_OK;
}

void raylib_lite_assets_unmount(void)
{
}

bool raylib_lite_assets_is_mounted(void)
{
    return true;
}

static void view_of(const pc_asset_t *asset, raylib_lite_asset_view_t *out)
{
    *out = (raylib_lite_asset_view_t){
        raylib_lite_asset_id(asset->name), asset->name, asset->data, asset->size,
    };
}

/* Reads the whole file into a fresh buffer; NULL when it cannot. */
static uint8_t *read_file(const char *path, size_t *out_size)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        return NULL;
    }
    uint8_t *data = NULL;
    long length = -1;
    if (fseek(file, 0, SEEK_END) == 0) {
        length = ftell(file);
    }
    if (length > 0 && fseek(file, 0, SEEK_SET) == 0) {
        data = malloc((size_t)length);
        if (data && fread(data, 1, (size_t)length, file) != (size_t)length) {
            free(data);
            data = NULL;
        }
    }
    fclose(file);
    if (data) {
        *out_size = (size_t)length;
    }
    return data;
}

raylib_lite_result_t raylib_lite_asset_open(const char *name, raylib_lite_asset_view_t *out)
{
    if (!name || !out || strlen(name) >= sizeof(s_assets[0].name)) {
        return RAYLIB_LITE_INVALID_ARGUMENT;
    }
    pc_asset_t *free_slot = NULL;
    for (unsigned i = 0; i < PC_ASSET_SLOTS; ++i) {
        pc_asset_t *asset = &s_assets[i];
        if (asset->data && strcmp(asset->name, name) == 0) {
            view_of(asset, out);
            return RAYLIB_LITE_OK;
        }
        if (!asset->data && !free_slot) {
            free_slot = asset;
        }
    }
    if (!free_slot) {
        return RAYLIB_LITE_NO_MEMORY;
    }
    char path[1200];
    if (snprintf(path, sizeof(path), "%s/%s", s_root, name) >= (int)sizeof(path)) {
        return RAYLIB_LITE_INVALID_ARGUMENT;
    }
    size_t size = 0;
    uint8_t *data = read_file(path, &size);
    if (!data) {
        return RAYLIB_LITE_NOT_FOUND;
    }
    snprintf(free_slot->name, sizeof(free_slot->name), "%s", name);
    free_slot->data = data;
    free_slot->size = size;
    view_of(free_slot, out);
    return RAYLIB_LITE_OK;
}

/* Views stay valid until exit, so a release only forgets the pointer. */
void raylib_lite_asset_release(raylib_lite_asset_view_t *view)
{
    if (view) {
        view->data = NULL;
        view->size = 0;
    }
}

/* Only files opened by name can be found by id: an id cannot name a file. */
raylib_lite_result_t raylib_lite_asset_open_id(raylib_lite_asset_id_t id,
                                               raylib_lite_asset_view_t *out)
{
    if (!out) {
        return RAYLIB_LITE_INVALID_ARGUMENT;
    }
    for (unsigned i = 0; i < PC_ASSET_SLOTS; ++i) {
        if (s_assets[i].data && raylib_lite_asset_id(s_assets[i].name) == id) {
            view_of(&s_assets[i], out);
            return RAYLIB_LITE_OK;
        }
    }
    return RAYLIB_LITE_NOT_FOUND;
}

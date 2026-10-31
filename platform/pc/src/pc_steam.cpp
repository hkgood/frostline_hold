/* Steamworks for the Steam build. The SDK headers are C++, hence this one
 * C++ file behind the C interface of pc_steam.h.
 *
 * Saves need no code here: Steam Auto-Cloud syncs the record file from the
 * player's data folder once the app's Steamworks settings name it.
 *
 * SPDX-License-Identifier: LicenseRef-Frostline-Hold-Proprietary */
#include "pc_steam.h"

#include <cstdio>

#include "steam/steam_api.h"

#ifndef FROSTLINE_STEAM_APPID
#error "set FROSTLINE_STEAM_APPID to the game's Steam App ID"
#endif

static bool s_running = false;

pc_steam_status_t pc_steam_init(void)
{
    /* Started outside Steam: Steam relaunches the game through itself. */
    if (SteamAPI_RestartAppIfNecessary(FROSTLINE_STEAM_APPID)) {
        return PC_STEAM_RESTART;
    }
    SteamErrMsg message = {0};
    if (SteamAPI_InitEx(&message) != k_ESteamAPIInitResult_OK) {
        std::fprintf(stderr, "steam: %s\n", message);
        return PC_STEAM_FAILED;
    }
    s_running = true;
    return PC_STEAM_OK;
}

void pc_steam_update(void)
{
    if (s_running) {
        SteamAPI_RunCallbacks();
    }
}

void pc_steam_shutdown(void)
{
    if (s_running) {
        SteamAPI_Shutdown();
        s_running = false;
    }
}

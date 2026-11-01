# Frostline Hold for macOS, Windows, Linux and the web (Emscripten), and the
# native library of the Android app (platform/android builds it through here).
#
#   cmake -S platform/pc -B build-pc && cmake --build build-pc
#
# The game sources come from game.sim.json, the list the simulator compiles,
# so the two never drift. The Engine sources come from the ESP-Mosaic
# checkout, found the same way as the firmware build finds it.
cmake_minimum_required(VERSION 3.24)
project(frostline_hold_pc VERSION 0.1.1 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(FROSTLINE_STEAM "Link Steamworks (needs STEAMWORKS_SDK and FROSTLINE_STEAM_APPID)" OFF)
set(STEAMWORKS_SDK "" CACHE PATH "The unpacked Steamworks SDK folder (the one holding public/ and redistributable_bin/)")
set(FROSTLINE_STEAM_APPID "" CACHE STRING "The game's Steam App ID")
set(FROSTLINE_BUNDLE_ID "com.hkgood.frostlinehold" CACHE STRING "macOS bundle identifier")
# The App Store wants a larger CFBundleVersion on every upload, even of the
# same version; package_mac.sh --mas passes one. Empty: the version.
set(FROSTLINE_BUILD_NUMBER "" CACHE STRING "macOS bundle build number (CFBundleVersion)")
# The engraved border and night backdrop round the board (src/pc_frame.c),
# made for tall phone screens; on a desktop it previews the Android look.
if(ANDROID)
    set(FROSTLINE_FRAME_DEFAULT ON)
else()
    set(FROSTLINE_FRAME_DEFAULT OFF)
endif()
option(FROSTLINE_FRAME "Frame the board for tall screens (the Android look)" ${FROSTLINE_FRAME_DEFAULT})

get_filename_component(GAME_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

# ---------------------------------------------------------------- ESP-Mosaic
if(DEFINED ENV{MOSAICO_ROOT})
    set(MOSAICO_ROOT "$ENV{MOSAICO_ROOT}")
elseif(EXISTS "${GAME_ROOT}/../../submodule/raylib-lite-engine")
    set(MOSAICO_ROOT "${GAME_ROOT}/../..")
elseif(EXISTS "${GAME_ROOT}/../ESP-Mosaic/submodule/raylib-lite-engine")
    set(MOSAICO_ROOT "${GAME_ROOT}/../ESP-Mosaic")
else()
    message(FATAL_ERROR "ESP-Mosaic not found: set the MOSAICO_ROOT environment variable")
endif()
get_filename_component(ENGINE_ROOT "${MOSAICO_ROOT}/submodule/raylib-lite-engine" ABSOLUTE)
if(NOT EXISTS "${ENGINE_ROOT}/host/host_module_bridge.c")
    message(FATAL_ERROR "the Engine at ${ENGINE_ROOT} has no host/ folder (a release package lacks it; use a git checkout)")
endif()

# ---------------------------------------------------------------------- SDL3
include(FetchContent)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_Declare(SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.18
    GIT_SHALLOW TRUE
    EXCLUDE_FROM_ALL)
FetchContent_MakeAvailable(SDL3)

# ------------------------------------------------------------------- sources
file(READ "${GAME_ROOT}/game.sim.json" SIM_MANIFEST)
string(JSON SIM_SCHEMA GET "${SIM_MANIFEST}" schema)
if(NOT SIM_SCHEMA STREQUAL "raylib-lite-game-sim/v1")
    message(FATAL_ERROR "game.sim.json: unsupported schema ${SIM_SCHEMA}")
endif()
string(JSON SIM_COUNT LENGTH "${SIM_MANIFEST}" sources)
set(GAME_SOURCES "")
math(EXPR SIM_LAST "${SIM_COUNT} - 1")
foreach(index RANGE ${SIM_LAST})
    string(JSON source GET "${SIM_MANIFEST}" sources ${index})
    list(APPEND GAME_SOURCES "${GAME_ROOT}/${source}")
endforeach()
# Re-read the list whenever it changes.
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${GAME_ROOT}/game.sim.json")

# The simulator's Engine list, with the Host asset table swapped for the
# PC one (src/pc_assets.c) and the audio mixer added.
set(ENGINE_SOURCES
    "${ENGINE_ROOT}/host/host_module_bridge.c"
    "${ENGINE_ROOT}/host/host_video_backend.c"
    "${ENGINE_ROOT}/host/host_clock.c"
    "${ENGINE_ROOT}/src/runtime/raylib_lite_raylib_port.c"
    "${ENGINE_ROOT}/src/renderer/raylib_lite_renderer.c"
    "${ENGINE_ROOT}/src/renderer/raylib_lite_renderer_raylib.c"
    "${ENGINE_ROOT}/src/renderer/raylib_lite_rgb565.c"
    "${ENGINE_ROOT}/src/renderer/raylib_lite_raylib_impl.c"
    "${ENGINE_ROOT}/src/fx/raylib_lite_fx.c"
    "${ENGINE_ROOT}/src/renderer/raylib_lite_tilemap.c"
    "${ENGINE_ROOT}/src/audio/raylib_lite_audio_decode.c"
    "${ENGINE_ROOT}/src/audio/raylib_lite_audio_mixer.c")

set(SHELL_SOURCES
    src/pc_main.c
    src/pc_assets.c
    src/pc_audio.c)
if(FROSTLINE_STEAM)
    enable_language(CXX)
    list(APPEND SHELL_SOURCES src/pc_steam.cpp)
else()
    list(APPEND SHELL_SOURCES src/pc_steam_stub.c)
endif()
if(FROSTLINE_FRAME)
    list(APPEND SHELL_SOURCES src/pc_frame.c)
else()
    list(APPEND SHELL_SOURCES src/pc_frame_stub.c)
endif()

# -------------------------------------------------------------------- assets
# Only the files the game opens: the atlases and the sounds.
file(GLOB GAME_ASSETS CONFIGURE_DEPENDS
    "${GAME_ROOT}/assets/generated/*.atlas"
    "${GAME_ROOT}/assets/generated/*.sound")
if(NOT GAME_ASSETS)
    message(FATAL_ERROR "no packed assets in assets/generated: run the simulator or the prepare_*.py scripts once")
endif()
set(ASSET_STAGE "${CMAKE_BINARY_DIR}/assets")
set(STAGED_ASSETS "")
foreach(asset ${GAME_ASSETS})
    get_filename_component(name "${asset}" NAME)
    add_custom_command(OUTPUT "${ASSET_STAGE}/${name}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${asset}" "${ASSET_STAGE}/${name}"
        DEPENDS "${asset}" VERBATIM)
    list(APPEND STAGED_ASSETS "${ASSET_STAGE}/${name}")
endforeach()
# The frame's art (assets_src/artgen/generate_frame.py), in assets/frame.
set(FRAME_ASSETS "")
if(FROSTLINE_FRAME)
    file(GLOB FRAME_ASSETS CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/frame/*.png")
    if(NOT FRAME_ASSETS)
        message(FATAL_ERROR "no frame art in platform/pc/frame: run assets_src/artgen/generate_frame.py")
    endif()
    foreach(asset ${FRAME_ASSETS})
        get_filename_component(name "${asset}" NAME)
        add_custom_command(OUTPUT "${ASSET_STAGE}/frame/${name}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${asset}" "${ASSET_STAGE}/frame/${name}"
            DEPENDS "${asset}" VERBATIM)
        list(APPEND STAGED_ASSETS "${ASSET_STAGE}/frame/${name}")
    endforeach()
endif()
add_custom_target(frostline_assets DEPENDS ${STAGED_ASSETS})

# -------------------------------------------------------------------- target
set(TARGET_NAME "Frostline Hold")
if(EMSCRIPTEN)
    set(TARGET_NAME index)
endif()
if(ANDROID)
    # SDLActivity loads libmain.so and calls its SDL_main.
    add_library(frostline SHARED ${SHELL_SOURCES} ${GAME_SOURCES} ${ENGINE_SOURCES})
    set(TARGET_NAME main)
else()
    add_executable(frostline WIN32 MACOSX_BUNDLE
        ${SHELL_SOURCES} ${GAME_SOURCES} ${ENGINE_SOURCES})
endif()
set_target_properties(frostline PROPERTIES OUTPUT_NAME "${TARGET_NAME}")
add_dependencies(frostline frostline_assets)

target_include_directories(frostline PRIVATE
    # First, so "raylib.h" is the PC header that adds the audio types.
    "${CMAKE_CURRENT_LIST_DIR}/include"
    "${ENGINE_ROOT}/host/include"
    "${ENGINE_ROOT}/host"
    "${ENGINE_ROOT}/examples/common_components/examples_common/include"
    "${ENGINE_ROOT}/include/raylib_lite"
    "${ENGINE_ROOT}/compat/raylib/include"
    "${GAME_ROOT}/main"
    "${GAME_ROOT}/assets/generated")
target_compile_definitions(frostline PRIVATE
    FROSTLINE_PC=1
    RAYLIB_LITE_HOST_SIMULATION=1
    "FROSTLINE_VERSION=\"${PROJECT_VERSION}\""
    "FROSTLINE_HOST_RAYLIB_H=\"${ENGINE_ROOT}/host/include/raylib.h\"")
if(MSVC)
    target_compile_options(frostline PRIVATE /W3 /utf-8)
else()
    target_compile_options(frostline PRIVATE -Wall -Wextra -funroll-loops)
endif()
target_link_libraries(frostline PRIVATE SDL3::SDL3-static)
if(NOT MSVC AND NOT APPLE AND NOT EMSCRIPTEN)
    target_link_libraries(frostline PRIVATE m)
endif()
if(ANDROID)
    target_link_libraries(frostline PRIVATE android log)
endif()

# --------------------------------------------------------------------- Steam
if(FROSTLINE_STEAM)
    if(NOT STEAMWORKS_SDK OR NOT EXISTS "${STEAMWORKS_SDK}/public/steam/steam_api.h")
        message(FATAL_ERROR "FROSTLINE_STEAM needs STEAMWORKS_SDK, the folder holding public/steam/steam_api.h")
    endif()
    if(NOT FROSTLINE_STEAM_APPID MATCHES "^[0-9]+$")
        message(FATAL_ERROR "FROSTLINE_STEAM needs FROSTLINE_STEAM_APPID (480, Spacewar, for a first test)")
    endif()
    set_target_properties(frostline PROPERTIES CXX_STANDARD 17)
    target_include_directories(frostline PRIVATE "${STEAMWORKS_SDK}/public")
    target_compile_definitions(frostline PRIVATE FROSTLINE_STEAM=1
        FROSTLINE_STEAM_APPID=${FROSTLINE_STEAM_APPID})
    set(STEAM_BIN "${STEAMWORKS_SDK}/redistributable_bin")
    if(APPLE)
        set(STEAM_RUNTIME "${STEAM_BIN}/osx/libsteam_api.dylib")
        target_link_libraries(frostline PRIVATE "${STEAM_RUNTIME}")
        set_target_properties(frostline PROPERTIES BUILD_RPATH "@executable_path"
                                                    INSTALL_RPATH "@executable_path")
    elseif(WIN32)
        set(STEAM_RUNTIME "${STEAM_BIN}/win64/steam_api64.dll")
        target_link_libraries(frostline PRIVATE "${STEAM_BIN}/win64/steam_api64.lib")
    else()
        set(STEAM_RUNTIME "${STEAM_BIN}/linux64/libsteam_api.so")
        target_link_libraries(frostline PRIVATE "${STEAM_RUNTIME}")
        set_target_properties(frostline PROPERTIES BUILD_RPATH "$ORIGIN" INSTALL_RPATH "$ORIGIN")
    endif()
    # The Steam runtime sits beside the program. steam_appid.txt lets a
    # development build start outside Steam; the packaging scripts leave it out.
    add_custom_command(TARGET frostline POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${STEAM_RUNTIME}" "$<TARGET_FILE_DIR:frostline>"
        COMMAND ${CMAKE_COMMAND} -E echo_append "${FROSTLINE_STEAM_APPID}" > "$<TARGET_FILE_DIR:frostline>/steam_appid.txt")
endif()

# ------------------------------------------------------------------ platform
if(EMSCRIPTEN)
    set(WEB_DIR "${CMAKE_CURRENT_LIST_DIR}/web")
    target_link_options(frostline PRIVATE
        "--preload-file=${ASSET_STAGE}@/assets"
        "--pre-js=${WEB_DIR}/frostline_pre.js"
        "--shell-file=${WEB_DIR}/shell.html"
        -sALLOW_MEMORY_GROWTH=1
        -sINITIAL_MEMORY=64MB
        -sSTACK_SIZE=1MB
        -lidbfs.js)
    set_target_properties(frostline PROPERTIES SUFFIX ".html"
        LINK_DEPENDS "${WEB_DIR}/frostline_pre.js;${WEB_DIR}/shell.html;${STAGED_ASSETS}")
elseif(ANDROID)
    # Gradle packs the assets into the APK (platform/android/app/build.gradle.kts).
elseif(APPLE)
    # The assets go into the bundle's Resources, where SDL_GetBasePath points.
    set_source_files_properties(${GAME_ASSETS} PROPERTIES
        MACOSX_PACKAGE_LOCATION Resources/assets)
    target_sources(frostline PRIVATE ${GAME_ASSETS})
    if(FRAME_ASSETS)
        set_source_files_properties(${FRAME_ASSETS} PROPERTIES
            MACOSX_PACKAGE_LOCATION Resources/assets/frame)
        target_sources(frostline PRIVATE ${FRAME_ASSETS})
    endif()
    set(ICON "${CMAKE_CURRENT_LIST_DIR}/mac/FrostlineHold.icns")
    if(EXISTS "${ICON}")
        set_source_files_properties("${ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        target_sources(frostline PRIVATE "${ICON}")
        set(MACOSX_BUNDLE_ICON_FILE FrostlineHold.icns)
    endif()
    # macOS 26 takes the Icon Composer file, compiled into Assets.car, over
    # the .icns; only actool 26 or later reads it. Its own .icns stays out:
    # the hand-drawn small sizes are better.
    set(ICON_COMPOSER "${CMAKE_CURRENT_LIST_DIR}/mac/FrostlineHold.icon")
    execute_process(COMMAND xcrun actool --version
        OUTPUT_VARIABLE ACTOOL_VERSION RESULT_VARIABLE ACTOOL_FAILED ERROR_QUIET)
    string(REGEX MATCH "short-bundle-version</key>[ \t\r\n]*<string>([0-9]+)" ACTOOL_MATCH "${ACTOOL_VERSION}")
    if(ACTOOL_FAILED OR NOT ACTOOL_MATCH OR CMAKE_MATCH_1 LESS 26)
        message(WARNING "actool 26 or later not found: macOS 26 will show the .icns icon")
    elseif(NOT EXISTS "${ICON_COMPOSER}/icon.json")
        message(FATAL_ERROR "no ${ICON_COMPOSER}: run assets_src/artgen/generate_icon.py")
    else()
        set(ICON_OUT "${CMAKE_BINARY_DIR}/icon")
        file(GLOB_RECURSE ICON_COMPOSER_FILES CONFIGURE_DEPENDS "${ICON_COMPOSER}/*")
        add_custom_command(OUTPUT "${ICON_OUT}/Assets.car"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${ICON_OUT}"
            COMMAND xcrun actool "${ICON_COMPOSER}" --compile "${ICON_OUT}"
                --platform macosx --minimum-deployment-target 11.0
                --app-icon FrostlineHold --output-partial-info-plist "${ICON_OUT}/partial.plist"
                --errors --warnings --output-format human-readable-text
            DEPENDS ${ICON_COMPOSER_FILES} VERBATIM)
        set_source_files_properties("${ICON_OUT}/Assets.car" PROPERTIES
            GENERATED TRUE MACOSX_PACKAGE_LOCATION Resources)
        target_sources(frostline PRIVATE "${ICON_OUT}/Assets.car")
        set(MACOSX_BUNDLE_ICON_NAME FrostlineHold)
    endif()
    if(FROSTLINE_BUILD_NUMBER)
        set(BUNDLE_BUILD_NUMBER "${FROSTLINE_BUILD_NUMBER}")
    else()
        set(BUNDLE_BUILD_NUMBER "${PROJECT_VERSION}")
    endif()
    set_target_properties(frostline PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/mac/Info.plist.in"
        MACOSX_BUNDLE_GUI_IDENTIFIER "${FROSTLINE_BUNDLE_ID}"
        MACOSX_BUNDLE_BUNDLE_NAME "Frostline Hold"
        MACOSX_BUNDLE_BUNDLE_VERSION "${BUNDLE_BUILD_NUMBER}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
else()
    # Windows and Linux: the assets folder beside the program.
    add_custom_command(TARGET frostline POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${ASSET_STAGE}" "$<TARGET_FILE_DIR:frostline>/assets")
    if(MINGW)
        # One self-contained .exe, without the MinGW runtime DLLs.
        target_link_options(frostline PRIVATE -static -static-libgcc)
    endif()
    if(WIN32)
        # The icon and the version Explorer shows.
        enable_language(RC)
        set(RC_FILE "${CMAKE_CURRENT_LIST_DIR}/windows/frostline.rc")
        set_source_files_properties("${RC_FILE}" PROPERTIES
            COMPILE_DEFINITIONS "FL_VERSION_MAJOR=${PROJECT_VERSION_MAJOR};FL_VERSION_MINOR=${PROJECT_VERSION_MINOR};FL_VERSION_PATCH=${PROJECT_VERSION_PATCH}"
            OBJECT_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/windows/frostline.ico")
        target_sources(frostline PRIVATE "${RC_FILE}")
    endif()
endif()

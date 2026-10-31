idf_component_register(SRCS "app_game_runtime.c" "frostline_view.c"
    "frostline_record.c"
    "frostline_audio.c"
    "frostline_text.c"
    "frostline_fx.c"
    "frostline_scenery.c"
    "frostline_screens.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private"
    REQUIRES examples_common
    PRIV_REQUIRES app_game raylib-lite-engine nvs_flash)
target_compile_definitions(${COMPONENT_LIB} PRIVATE RAYLIB_LITE_GAME_NATIVE=1)

# Development notes

Frostline Hold is a Raylib Lite game for the ESP-Mosaico board (ESP32-S31,
480×480 display). It lives in an ESP-Mosaic workspace as
`projects/frostline_hold`; every command below runs from the workspace root.

```sh
python mosaico.py game sim --project projects/frostline_hold          # browser simulator
python mosaico.py game build --project projects/frostline_hold        # firmware
python mosaico.py iris system-update --project projects/frostline_hold  # install
```

Run `python mosaico.py recover` before the first installation on a blank or
unverified device. `recover` and the device tools need `pyserial` in the Python
that runs `mosaico.py`.

## Layout

- `components/app_game`: pure C state and gameplay rules, no graphics dependency.
- `components/app_game_runtime`: Engine input/lifecycle adapter, rendering, audio
  and JSON.
- `components/app_game_runtime/private/game_config.h`: identity, pixel dimensions
  and tick frequency; never included by another component.
- `main/game_entry.c`: selects the adapter for the unchanged Engine module ABI.
- `game.sim.json`: explicit list of the same sources used by the Host module.
- `assets_src/`: the art, sound, level and text generators (`artgen/`) and their
  sources; the build runs the `prepare_*.py` hooks and packs `assets/generated/`.

Dependencies flow `main -> app_game_runtime -> app_game`, with the Engine as a
platform dependency of the runtime. The model never depends on its caller or on
rendering. Public headers live under each component's `include/`; internal state
and configuration stay private.

Keep new application components cohesive and prefixed `app_`; keep dependencies
acyclic and entry points limited to composition/startup. Exchange handles and
value snapshots, never extern business globals or private structures. Pins and
workspace overrides live in `main/idf_component.yml` and the root CMake
configuration; never edit `managed_components/` in place.

## Generated data

Levels, text and art are generated; edit the sources, then regenerate:

```sh
cd assets_src
python artgen/generate_level.py      # components/app_game/data/frostline_level.h
python artgen/generate_text.py       # components/app_game_runtime/include/frostline_text_data.h
python artgen/generate_terrain.py    # components/app_game_runtime/data/frostline_terrain_data.h
python artgen/generate_tower_rig.py  # components/app_game/data/frostline_tower_rig.h
python prepare_sprites.py            # atlas sources
python prepare_sfx.py                # sound effects and music
```

Each generator has a `--check` mode used by the build and the tests.

Levels live in `artgen/levels.py` (the 8 chapters) and `artgen/interludes.py`
(the 2 interludes after each chapter); `levels.LEVELS` puts them in play order.
The battlefields and the campaign scroll (`CAMPAIGN_PLATES`, one 480 px plate
per third) are baked as 1-bit maps into the firmware, about 30 KB each, and
the runtime expands only the map on screen into one RGB565 buffer. Rerun
`generate_terrain.py` after changing a plate, a lane or a node.

Towers are drawn to one rig, `artgen/tower_rig.py`: muzzles, the catapult's
arm and bucket, the ballista's mount, the kiln's spare pots, orbits and the
hall's door. `artgen/tower_art.py` draws the frames from it and
`generate_tower_rig.py` writes the same points for the model, which fires
from them, and for `frostline_towers.c`, which hangs the moving parts on them.
`artgen/test_towers.py` checks that every muzzle lands on the art. Rerun the
generator after moving anything on a tower.

## Tests

```sh
# gameplay model
clang -std=c11 -Wall -Wextra -Werror -Icomponents/app_game/include \
  -Icomponents/app_game -o /tmp/test_model \
  components/app_game/test/test_model.c components/app_game/frostline_model.c -lm
/tmp/test_model

# asset pipeline, levels, art (needs RAYLIB_LITE_ENGINE when the engine is elsewhere)
cd assets_src && python -m unittest discover -s artgen -p "test_*.py" -t .
```

`components/app_game/test/auto_play.c` is a balance probe: a greedy bot that
plays a level and reports how far it gets.

## Device limits

The firmware embeds every packed file and registers it in fixed tables. Past
their capacity a file fails silently, so `sdkconfig.defaults` raises them and
`test_pipeline.py` guards them:

| Setting | Value | Holds |
| --- | --- | --- |
| `CONFIG_RAYLIB_LITE_MAX_ASSETS` | 64 | every atlas and sound file |
| `CONFIG_ESP_MOSAICO_AUDIO_CLIPS` | 32 | every sound effect and music theme |

`CONFIG_RAYLIB_LITE_MAX_ASSETS` needs a raylib-lite-engine that reads it; older
engines register 32 files and the game cannot load all its atlases. The native
asset pack is limited to 10 MiB.

Host state exposes `phase`, `tick`, `pointer_x`, `pointer_y`, `pointer_down` and
`state_hash` for automation; pause/resume/step/reset are wired for Host controls
and replay.

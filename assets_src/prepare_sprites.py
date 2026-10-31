"""Build hook: regenerate every sprite sheet before packing.

This runs in both the Host simulator and the ESP-IDF build, so the PNGs in
assets_src are always reproducible from code.

Atlas sizing rules taken from the engine packer:
  * binary alpha: output_cell must be exactly source_cell + 4, otherwise the
    packer shrinks the crop with LANCZOS and the 1-bit art gains grey levels
  * opaque alpha: the source cell must equal output_cell exactly
"""
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from artgen import sprites  # noqa: E402
from artgen import tower_art  # noqa: E402
from artgen.canvas import Canvas  # noqa: E402
from artgen import title_art  # noqa: E402

MAP_SIZE = 480


def _write(source_name, config_name, canvas, frames, alpha_mode, output_cell,
           animations, columns=8):
    canvas.to_image().save(HERE / source_name)
    rows = (len(frames) + columns - 1) // columns
    config = {
        "image": source_name,
        "columns": columns,
        "rows": rows,
        "output_cell": output_cell,
        "output_columns": columns,
        "alpha_mode": alpha_mode,
        "frames": frames,
        "animations": animations,
    }
    (HERE / config_name).write_text(json.dumps(config, indent=2) + "\n",
                                    encoding="utf-8")


def main():
    # The terrain is not a sheet: generate_terrain.py bakes it into the
    # firmware as 1-bit data. The title sheet: the fortress by night, and at
    # dawn for the ending.
    plates = [("title_backdrop", title_art.build_title_backdrop()),
              ("ending_backdrop", title_art.build_ending_backdrop())]
    sheet = Canvas(MAP_SIZE * len(plates), MAP_SIZE)
    for index, (_name, canvas) in enumerate(plates):
        sheet.blit(canvas, index * MAP_SIZE, 0)
    sheet.to_image().save(HERE / "title_atlas_source.png")
    (HERE / "atlas_title.json").write_text(json.dumps({
        "image": "title_atlas_source.png",
        "columns": len(plates), "rows": 1,
        "output_cell": MAP_SIZE, "output_columns": len(plates),
        "alpha_mode": "opaque",
        "frames": [{"name": name} for name, _canvas in plates],
        "animations": [],
    }, indent=2) + "\n", encoding="utf-8")

    canvas, frames = sprites.build_units()
    _write("units_atlas_source.png", "atlas_units.json", canvas, frames,
           "binary", sprites.UNIT_ATLAS_CELL, [])

    canvas, frames = sprites.build_parts()
    _write("parts_atlas_source.png", "atlas_parts.json", canvas, frames,
           "binary", tower_art.PART_ATLAS_CELL, [])

    canvas, frames = sprites.build_props()
    _write("props_atlas_source.png", "atlas_props.json", canvas, frames,
           "binary", sprites.PROP_ATLAS_CELL, sprites.PROPS_ANIMATIONS)

    canvas, frames = sprites.build_props_husk()
    _write("props_husk_atlas_source.png", "atlas_props_husk.json", canvas,
           frames, "binary", sprites.HUSK_ATLAS_CELL, sprites.HUSK_ANIMATIONS,
           columns=sprites.WALK_FRAMES)

    canvas, frames = sprites.build_props_titan()
    _write("props_titan_atlas_source.png", "atlas_props_titan.json", canvas,
           frames, "binary", sprites.TITAN_ATLAS_CELL,
           sprites.TITAN_ANIMATIONS, columns=sprites.WALK_FRAMES)

    canvas, frames = sprites.build_heroes()
    _write("heroes_atlas_source.png", "atlas_heroes.json", canvas, frames,
           "binary", sprites.HERO_ATLAS_CELL, [], columns=4)

    canvas, frames = sprites.build_fx()
    _write("fx_atlas_source.png", "atlas_fx.json", canvas, frames,
           "binary", sprites.FX_ATLAS_CELL, sprites.FX_ANIMATIONS)

    canvas, frames = sprites.build_icons()
    _write("icons_atlas_source.png", "atlas_icons.json", canvas, frames,
           "binary", sprites.ICON_ATLAS_CELL, [])

    canvas, frames = sprites.build_castle_fx()
    _write("castle_fx_atlas_source.png", "atlas_castle_fx.json", canvas,
           frames, "binary", sprites.CASTLE_FX_ATLAS_CELL,
           sprites.CASTLE_FX_ANIMATIONS)

    print("sprite sheets regenerated")


if __name__ == "__main__":
    main()

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
from artgen.ai_terrain import build_background  # noqa: E402


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
    background = build_background()
    background.to_image().save(HERE / "terrain_atlas_source.png")
    (HERE / "atlas_terrain.json").write_text(json.dumps({
        "image": "terrain_atlas_source.png",
        "columns": 1, "rows": 1,
        "output_cell": 480, "output_columns": 1,
        "alpha_mode": "opaque",
        "frames": [{"name": "map_background"}],
        "animations": [],
    }, indent=2) + "\n", encoding="utf-8")

    canvas, frames = sprites.build_units()
    _write("units_atlas_source.png", "atlas_units.json", canvas, frames,
           "binary", sprites.UNIT_ATLAS_CELL, [])

    canvas, frames = sprites.build_props()
    _write("props_atlas_source.png", "atlas_props.json", canvas, frames,
           "binary", sprites.PROP_ATLAS_CELL, sprites.PROPS_ANIMATIONS)

    canvas, frames = sprites.build_fx()
    _write("fx_atlas_source.png", "atlas_fx.json", canvas, frames,
           "binary", sprites.FX_ATLAS_CELL, sprites.FX_ANIMATIONS)

    print("sprite sheets regenerated")


if __name__ == "__main__":
    main()

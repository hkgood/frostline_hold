"""Preview helper: composite generated sprites onto the real map.

Not part of the game; it exists so art can be judged in context the way a
player sees it, instead of as an isolated checkerboard sheet.
"""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from PIL import Image

from artgen import level_geom as geom
from artgen import sprites
from artgen.terrain import build_background


def _sheet(builder):
    canvas, frames = builder()
    image = canvas.to_image()
    names = [frame["name"] for frame in frames]
    return image, names, canvas.width // 8  # source cell size


def _cell(image, names, cell, name):
    index = names.index(name)
    col, row = index % 8, index // 8
    return image.crop((col * cell, row * cell, col * cell + cell, row * cell + cell))


def compose(output):
    background = build_background().to_image()
    units_image, unit_names, unit_cell = _sheet(sprites.build_units)
    props_image, prop_names, prop_cell = _sheet(sprites.build_props)

    towers = ["tower_guard_lv1", "tower_frost_lv2", "tower_siege_lv1",
              "tower_guard_lv3_pierce"]
    for index, spot in enumerate(geom.BUILD_SPOTS[:4]):
        name = towers[index]
        sprite = _cell(units_image, unit_names, unit_cell, name)
        sprite = sprite.crop((0, 0, sprites.UNIT_CELL, sprites.UNIT_CELL))
        background.alpha_composite(sprite, (spot[0] - 24, spot[1] - 40))

    enemies = [("enemy_wolf_0", (150, 214)), ("enemy_raider_0", (228, 268))]
    for name, position in enemies:
        sprite = _cell(props_image, prop_names, prop_cell, name)
        sprite = sprite.crop((0, 0, sprites.PROP_CELL, sprites.PROP_CELL))
        background.alpha_composite(sprite, (position[0] - 16, position[1] - 22))

    background.convert("RGB").save(output)
    print("wrote", output)


if __name__ == "__main__":
    compose(sys.argv[1] if len(sys.argv) > 1 else "/workspace/output/scene.png")

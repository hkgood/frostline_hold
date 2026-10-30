"""Pipeline tests: the packed atlases must stay strictly 1-bit.

The engine packer resamples a source crop whenever the source cell is larger
than output_cell - 4, and LANCZOS turns pure black and white into grey. That
silently breaks the art style, so the packed RGB565 payload is decoded and
checked here.
"""
import struct
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ASSETS = Path(__file__).resolve().parent.parent
PROJECT = ASSETS.parent
ENGINE = PROJECT.parent.parent / "submodule" / "raylib-lite-engine"

ATLAS_HEADER = struct.Struct("<4sHHHHII")
ATLAS_FRAME = struct.Struct("<IHHHHhh")

WHITE = 0xFFFF
BLACK = 0x0000


def _atlas_colours(path):
    data = path.read_bytes()
    magic, width, height, frames, flags, rgb_len, alpha_len = ATLAS_HEADER.unpack_from(data, 0)
    assert magic == b"MSA1", magic
    start = ATLAS_HEADER.size + frames * ATLAS_FRAME.size
    rgb = data[start:start + rgb_len]
    colours = {rgb[i] | (rgb[i + 1] << 8) for i in range(0, len(rgb), 2)}
    return width, height, frames, alpha_len, colours


class TestAssetPipeline(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory()
        cls.output = Path(cls.tmp.name)
        subprocess.run([sys.executable, str(ASSETS / "prepare_sprites.py")],
                       check=True, capture_output=True)
        subprocess.run(
            [sys.executable, str(ENGINE / "tools" / "pack_game_assets.py"),
             "--source", str(ASSETS), "--output", str(cls.output)],
            check=True, capture_output=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_every_atlas_is_strictly_one_bit(self):
        for name in ("terrain.atlas", "units.atlas", "props.atlas", "fx.atlas"):
            path = self.output / name
            self.assertTrue(path.exists(), name)
            _w, _h, _frames, _alpha, colours = _atlas_colours(path)
            self.assertTrue(
                colours <= {BLACK, WHITE},
                "%s has %d colours, e.g. %s" % (name, len(colours),
                                                 [hex(c) for c in sorted(colours)[:6]]),
            )

    def test_terrain_atlas_is_opaque_with_no_alpha_channel(self):
        _w, _h, _frames, alpha_len, colours = _atlas_colours(self.output / "terrain.atlas")
        self.assertEqual(alpha_len, 0)
        self.assertEqual(colours, {BLACK, WHITE})

    def test_sprite_atlases_use_binary_alpha(self):
        for name in ("units.atlas", "props.atlas", "fx.atlas"):
            _w, _h, _frames, alpha_len, _colours = _atlas_colours(self.output / name)
            self.assertGreater(alpha_len, 0, name)

    def test_source_cells_are_four_pixels_smaller_than_atlas_cells(self):
        from artgen import sprites
        self.assertEqual(sprites.UNIT_ATLAS_CELL, sprites.UNIT_CELL + 4)
        self.assertEqual(sprites.PROP_ATLAS_CELL, sprites.PROP_CELL + 4)
        self.assertEqual(sprites.FX_ATLAS_CELL, sprites.FX_CELL + 4)

    def test_generated_sources_contain_only_black_white_and_transparent(self):
        from PIL import Image
        for name in ("units_atlas_source.png", "props_atlas_source.png",
                     "fx_atlas_source.png", "terrain_atlas_source.png"):
            image = Image.open(ASSETS / name).convert("RGBA")
            colours = set(image.getdata())
            self.assertTrue(
                colours <= {(0, 0, 0, 0), (0, 0, 0, 255), (255, 255, 255, 255)},
                "%s -> %s" % (name, sorted(colours)[:6]),
            )


if __name__ == "__main__":
    unittest.main()

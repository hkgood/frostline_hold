"""Store art for the Mac App Store and Google Play, from the game's own frames.

  mac/<lang>/NN_<shot>.png      2880x1800  the board as the Mac app shows it:
                                           scaled to the window's height,
                                           the ink either side
  android/<lang>/NN_<shot>.png  1080x1920  the board in the Android frame,
                                           laid out as src/pc_frame.c does
  android/feature_graphic.png   1024x500   Play's banner: the logo over the
                                           aurora, as on the itch.io banner

The frames come from tools/promo/capture.py (run it with and without
--lang en first); the frame art from generate_frame.py. Everything is
written to dist/store/, which git ignores: the screenshots are large and
are rebuilt from the captures at will. Play's 512x512 icon is
platform/pc/icon/itch_square_512.png.

Run: cd assets_src && python3 artgen/generate_store_art.py
"""

import math
import sys
from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from artgen import title_art  # noqa: E402
from artgen.canvas import Canvas, WHITE  # noqa: E402
from artgen.generate_itch_page import (  # noqa: E402
    BANNER_FRAME,
    INK,
    Logo,
    draw_aurora,
    draw_subtitle,
    draw_twinkle,
    enlarge,
    to_rgb,
)

PROJECT = HERE.parent.parent
CLIPS = PROJECT / "tools" / "promo" / "film" / "public"
FRAME_ART = PROJECT / "platform" / "pc" / "frame"
OUT_DIR = PROJECT / "dist" / "store"
LANGS = {"en": CLIPS / "clips_en", "zh": CLIPS / "clips"}

GAME = 480
# pc_frame.c's PC_FRAME_MARGIN: the border's reach beyond the board.
FRAME_MARGIN = 16
# The shots, in store order: (name, capture clip, frame).
SHOTS = (
    ("title", "title", 200),
    ("campaign", "campaign", 62),
    ("towers", "towers", 310),
    ("wyrm", "wyrm", 300),
    ("king", "king", 250),
    ("victory", "victory", 110),
)


def scaled(image, width, height):
    """`image` at a non-integer scale as SDL's pixel-art mode draws it: each
    game pixel a block, the blocks' edges blended."""
    big = image.resize((image.width * 4, image.height * 4), Image.NEAREST)
    return big.resize((max(1, round(width)), max(1, round(height))), Image.BOX)


def load_frame(lang, clip, index):
    path = LANGS[lang] / clip / f"{index:04d}.png"
    if not path.exists():
        flag = " --lang en" if lang == "en" else ""
        raise SystemExit(f"{path} is missing: run python3 tools/promo/capture.py{flag}")
    return Image.open(path).convert("RGB")


# ---------------------------------------------------------------------- Mac

MAC_W, MAC_H = 2880, 1800


def mac_shot(board):
    """SDL's letterbox: the largest square that fits, centred on the ink."""
    side = min(MAC_W, MAC_H)
    image = Image.new("RGB", (MAC_W, MAC_H), INK)
    image.paste(scaled(board, side, side), ((MAC_W - side) // 2, (MAC_H - side) // 2))
    return image


# ------------------------------------------------------------------ Android

PHONE_W, PHONE_H = 1080, 1920


def phone_shot(board, art):
    """pc_frame.c's layout() and pc_frame_render() on a full-screen phone."""
    span = GAME + FRAME_MARGIN * 2
    scale = min(PHONE_W / span, PHONE_H / span)
    board_w = math.floor(GAME * scale)
    scale = board_w / GAME
    board_h = math.floor(GAME * scale)
    bx = round((PHONE_W - board_w) * 0.5)
    by = round((PHONE_H - board_h) * 0.5)
    centre = bx + board_w * 0.5

    image = Image.new("RGBA", (PHONE_W, PHONE_H), INK + (255,))

    def pinned(layer, at_bottom):
        w, h = layer.width * scale, layer.height * scale
        x = round(centre - w * 0.5)
        y = round(PHONE_H - h) if at_bottom else 0
        image.alpha_composite(scaled(layer, w, h).convert("RGBA"), (x, y))

    pinned(art["sky"], False)
    pinned(art["ground"], True)
    reach = FRAME_MARGIN * scale
    border = scaled(art["border"], board_w + reach * 2, board_h + reach * 2).convert("RGBA")
    image.alpha_composite(border, (round(bx - reach), round(by - reach)))
    image.paste(scaled(board, board_w, board_h), (bx, by))
    return image.convert("RGB")


# ---------------------------------------------------------- feature graphic

FEATURE_W, FEATURE_H = 512, 250  # game pixels, stored at 2x: 1024x500
FEATURE_STARS = (
    (40, 34, 3),
    (96, 200, 1),
    (132, 22, 1),
    (420, 26, 1),
    (470, 150, 3),
    (380, 214, 1),
    (22, 120, 1),
    (492, 230, 1),
)


def build_feature_graphic():
    canvas = Canvas(FEATURE_W, FEATURE_H)
    title_art._starfield(canvas, 0, FEATURE_H, 0x1CE5)
    draw_aurora(canvas, 20, 150, BANNER_FRAME, 1.0, taper=80)
    for x, y, arm in FEATURE_STARS:
        draw_twinkle(canvas, x, y, arm, WHITE)
    logo = Logo()
    # The logo and the subtitle below it, centred as one block.
    block = logo.height * logo.SCALE + 14 + 7
    top = (FEATURE_H - block) // 2
    height = logo.draw(canvas, FEATURE_W // 2, top, BANNER_FRAME)
    draw_subtitle(canvas, FEATURE_W // 2, top + height + 14)
    return enlarge(to_rgb(canvas), 2)


# ------------------------------------------------------------------- export


def save(image, relative):
    path = OUT_DIR / relative
    path.parent.mkdir(parents=True, exist_ok=True)
    image.save(path, optimize=True)
    print(f"wrote {path.relative_to(PROJECT)} {image.width}x{image.height}")


def main():
    art = {name: Image.open(FRAME_ART / f"{name}.png") for name in ("sky", "ground", "border")}
    for lang in LANGS:
        for number, (name, clip, index) in enumerate(SHOTS, 1):
            board = load_frame(lang, clip, index)
            save(mac_shot(board), f"mac/{lang}/{number:02d}_{name}.png")
            save(phone_shot(board, art), f"android/{lang}/{number:02d}_{name}.png")
    save(build_feature_graphic(), "android/feature_graphic.png")


if __name__ == "__main__":
    main()

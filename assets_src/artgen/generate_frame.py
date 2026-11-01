"""Art for the framed shell (the Android build): what fills a tall screen
around the square board.

  sky.png     1024x1280  night sky, opaque: dim stars and an aurora along the
                         top. The shell pins it to the top of the screen.
  ground.png   640x360   the snowy peaks and pines of
                         ai/page_background_source.jpg, its sky cut away along
                         the ridge line. The shell pins it to the bottom.
  border.png   512x512   the engraved double rule round the 480x480 board,
                         its corner and middle diamonds, clear in the middle.

All three are one image pixel per game pixel; the shell scales them with the
board, so the dots of the backdrop and of the game stay the same size. The
backdrop's paper is dimmed, as on the itch.io page, so it never competes
with the board; the border uses the full paper.

Run: cd assets_src && python3 artgen/generate_frame.py
"""
import sys
from pathlib import Path

from PIL import Image, ImageFilter

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from artgen.ai_terrain import AI_DIR, _dither  # noqa: E402
from artgen.canvas import BLACK, WHITE, Canvas  # noqa: E402
from artgen import shade as S  # noqa: E402
from artgen.generate_itch_page import (BAYER4, DIM, INK, PAPER,  # noqa: E402
                                       draw_aurora, draw_diamond, draw_twinkle)

PROJECT = HERE.parent.parent
OUT_DIR = PROJECT / "platform" / "pc" / "frame"

BOARD = 480
# Game pixels of border on each side of the board, room for the corner
# diamonds; the shell's PC_FRAME_MARGIN.
MARGIN = 16

SKY_W, SKY_H = 1024, 1280
GROUND_W, GROUND_H = 640, 360
GROUND_SOURCE = AI_DIR / "page_background_source.jpg"
GROUND_EDGE = 40      # columns of dithered fade at the left and right ends


# ------------------------------------------------------------------- export

def to_rgba(canvas, paper):
    """Unset pixels clear, ink and paper opaque."""
    image = Image.new("RGBA", (canvas.width, canvas.height))
    clear = (0, 0, 0, 0)
    ink = INK + (255,)
    lit = paper + (255,)
    image.putdata([lit if canvas.get(x, y) == WHITE else
                   ink if canvas.get(x, y) == BLACK else clear
                   for y in range(canvas.height) for x in range(canvas.width)])
    return image


def save(image, name):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / name
    image.save(path, optimize=True)
    print(f"wrote {path.relative_to(PROJECT)} {image.width}x{image.height}")


# ---------------------------------------------------------------------- sky

def build_sky():
    canvas = Canvas(SKY_W, SKY_H)
    canvas.fill_rect(0, 0, SKY_W, SKY_H, BLACK)
    # Scattered stars, fewer towards the bottom where the peaks rise.
    for y in range(SKY_H):
        density = 0.005 * (1.0 - 0.5 * y / SKY_H)
        for x in range(SKY_W):
            h = S.hash01(x, y, 0x5C1E)
            if h < density:
                canvas.set(x, y, WHITE)
            elif h < density + 0.0005:
                draw_twinkle(canvas, x, y, 2, WHITE)
    draw_aurora(canvas, 6, 140, 260, 0.8, taper=200)
    image = to_rgba(canvas, DIM)

    # A few bright stars in full paper, the only ones that catch the eye.
    bright = Canvas(SKY_W, SKY_H)
    for i in range(36):
        h = S.hash01(i, 7, 0xB417)
        g = S.hash01(i, 13, 0xB417)
        x, y = 16 + int(h * (SKY_W - 32)), 16 + int(g * g * (SKY_H - 32))
        draw_twinkle(bright, x, y, (3, 1, 1, 0)[i % 4], WHITE)
    overlay = to_rgba(bright, PAPER)
    image.alpha_composite(overlay)
    return image.convert("RGB")


# ------------------------------------------------------------------- ground

def ridge_line(grey):
    """For each column, the first row of the mountains below the dark sky.

    A heavy blur wipes out the stars, so the first bright row is the ridge.
    """
    soft = grey.filter(ImageFilter.GaussianBlur(3)).load()
    ridge = []
    for x in range(grey.width):
        row = next((y for y in range(grey.height) if soft[x, y] > 70), grey.height)
        ridge.append(max(0, row - 2))
    return ridge


def build_ground():
    source = Image.open(GROUND_SOURCE).convert("L")
    grey = source.resize((GROUND_W, GROUND_H), Image.BOX)
    grey = grey.filter(ImageFilter.UnsharpMask(radius=1.2, percent=90, threshold=2))
    plate = _dither(grey)
    ridge = ridge_line(grey)
    canvas = Canvas(GROUND_W, GROUND_H)
    for x in range(GROUND_W):
        edge = min(1.0, x / GROUND_EDGE, (GROUND_W - 1 - x) / GROUND_EDGE)
        for y in range(ridge[x], GROUND_H):
            # The ends fade out by an ordered threshold, so they stay dots.
            if (BAYER4[y & 3][x & 3] + 0.5) / 16.0 < edge:
                canvas.set(x, y, plate.get(x, y))
    return to_rgba(canvas, DIM)


# ------------------------------------------------------------------- border

# Distances outward from the board's edge, in game pixels.
THIN_RULE = 4         # one pixel
THICK_RULE = 7        # two pixels, 7 and 8
MAT = 9               # ink from the board out to here, under the rules


def ring(canvas, d, width, value):
    """A square outline from `d` pixels outside the board, `width` thick."""
    for i in range(width):
        a, b = MARGIN - (d + i), MARGIN + BOARD - 1 + (d + i)
        canvas.hline(a, a, b - a + 1, value)
        canvas.hline(a, b, b - a + 1, value)
        canvas.vline(a, a, b - a + 1, value)
        canvas.vline(b, a, b - a + 1, value)


def ornament(canvas, cx, cy, r):
    """An engraved diamond: an ink halo, the paper face, an ink eye."""
    draw_diamond(canvas, cx, cy, r + 2, BLACK)
    draw_diamond(canvas, cx, cy, r, WHITE)
    draw_diamond(canvas, cx, cy, max(1, r // 3), BLACK)


def build_border():
    size = BOARD + MARGIN * 2
    canvas = Canvas(size, size)
    canvas.fill_rect(MARGIN - MAT, MARGIN - MAT, BOARD + MAT * 2, BOARD + MAT * 2, BLACK)
    ring(canvas, THIN_RULE, 1, WHITE)
    ring(canvas, THICK_RULE, 2, WHITE)
    # On the thick rule's outer pixel, mirrored across the image.
    lo, hi = MARGIN - THICK_RULE - 1, MARGIN + BOARD + THICK_RULE
    for cx in (lo, hi):
        for cy in (lo, hi):
            ornament(canvas, cx, cy, 6)
    mid = size // 2
    for cx, cy in ((mid, lo), (mid, hi), (lo, mid), (hi, mid)):
        ornament(canvas, cx, cy, 4)
    # The board shows through.
    canvas.fill_rect(MARGIN, MARGIN, BOARD, BOARD, None)
    return to_rgba(canvas, PAPER)


def main():
    save(build_sky(), "sky.png")
    save(build_ground(), "ground.png")
    save(build_border(), "border.png")


if __name__ == "__main__":
    main()

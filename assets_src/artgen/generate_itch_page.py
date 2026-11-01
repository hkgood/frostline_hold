"""Art for the web build's itch.io page (Edit theme > Images).

  banner.png      1920x576   the logo over the aurora; itch shows it 960 wide,
                             so every game pixel is two screen pixels, sharp
                             on high-density screens too
  background.png  2560x1440  snowy peaks either side of the page column
                             (ai/page_background_source.jpg), dimmed so they
                             never compete with the text
  embed_bg.png    720x720    the title screen without its buttons, behind
                             "Run game" in the 720x720 game frame

All three use the two inks as the browser shows them (the game's RGB565
output), so the page, the frame and the running game meet without a seam.
The logo is drawn by the same steps as draw_logo() in frostline_screens.c;
the embed backdrop is one of the game's own title frames, captured by
tools/promo/capture.py --lang en.

Run: cd assets_src && python3 artgen/generate_itch_page.py
"""
import math
import sys
from pathlib import Path

from PIL import Image, ImageFilter

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE.parent))

from artgen.ai_terrain import AI_DIR, _dither  # noqa: E402
from artgen.canvas import BLACK, WHITE, Canvas  # noqa: E402
from artgen import strings as S  # noqa: E402
from artgen import title_art  # noqa: E402

PROJECT = HERE.parent.parent
OUT_DIR = PROJECT / "platform" / "pc" / "itch"
TITLE_FRAMES = PROJECT / "tools" / "promo" / "film" / "public" / "clips_en" / "title"

# The inks as the game shows them, after RGB565.
INK = (49, 44, 41)
PAPER = (180, 174, 172)
# The page background's paper: the peaks seen through the dark.
DIM = (84, 78, 74)

# ----------------------------------------------------------------- the font

# The 5x7 Latin face of frostline_text.c, the rows' low five bits.
LATIN = {
    "A": (14, 17, 17, 31, 17, 17, 17), "B": (30, 17, 17, 30, 17, 17, 30),
    "C": (14, 17, 16, 16, 16, 17, 14), "D": (30, 17, 17, 17, 17, 17, 30),
    "E": (31, 16, 16, 30, 16, 16, 31), "F": (31, 16, 16, 30, 16, 16, 16),
    "G": (14, 17, 16, 23, 17, 17, 15), "H": (17, 17, 17, 31, 17, 17, 17),
    "I": (14, 4, 4, 4, 4, 4, 14), "J": (7, 2, 2, 2, 18, 18, 12),
    "K": (17, 18, 20, 24, 20, 18, 17), "L": (16, 16, 16, 16, 16, 16, 31),
    "M": (17, 27, 21, 21, 17, 17, 17), "N": (17, 25, 21, 19, 17, 17, 17),
    "O": (14, 17, 17, 17, 17, 17, 14), "P": (30, 17, 17, 30, 16, 16, 16),
    "Q": (14, 17, 17, 17, 21, 18, 13), "R": (30, 17, 17, 30, 20, 18, 17),
    "S": (15, 16, 16, 14, 1, 1, 30), "T": (31, 4, 4, 4, 4, 4, 4),
    "U": (17, 17, 17, 17, 17, 17, 14), "V": (17, 17, 17, 17, 17, 10, 4),
    "W": (17, 17, 17, 21, 21, 21, 10), "X": (17, 17, 10, 4, 10, 17, 17),
    "Y": (17, 17, 10, 4, 4, 4, 4), "Z": (31, 1, 2, 4, 8, 16, 31),
}


def ui_text(key):
    """The English text of a strings.py UI entry."""
    return next(en for name, en, _ in S.UI if name == key)


def latin_bits(text, x, y, bits):
    """Set the font pixels of `text` (6 px advance) into a {(r, c)} set."""
    for ch in text:
        rows = LATIN.get(ch)
        if rows:
            for r, row in enumerate(rows):
                for c in range(5):
                    if (row >> (4 - c)) & 1:
                        bits.add((y + r, x + c))
        x += 6


def draw_latin(canvas, text, x, y, value):
    bits = set()
    latin_bits(text, x, y, bits)
    for r, c in bits:
        canvas.set(c, r, value)


def latin_width(text):
    return len(text) * 6 - 1


# ------------------------------------------------- frostline_screens.c ports

def jag_hash(a):
    a &= 0xFFFFFFFF
    a ^= a >> 16
    a = (a * 0x7FEB352D) & 0xFFFFFFFF
    a ^= a >> 15
    a = (a * 0x846CA68B) & 0xFFFFFFFF
    a ^= a >> 16
    return a


def hash2(a, b):
    return jag_hash(((a * 0x9E3779B1) & 0xFFFFFFFF) ^ ((b + 0x7F4A7C15) & 0xFFFFFFFF))


def draw_twinkle(canvas, x, y, arm, value):
    canvas.fill_rect(x - arm, y, arm * 2 + 1, 1, value)
    canvas.fill_rect(x, y - arm, 1, arm * 2 + 1, value)
    if arm >= 3:
        canvas.fill_rect(x - 1, y - 1, 3, 3, value)


def draw_diamond(canvas, cx, cy, r, value):
    canvas.triangle(cx, cy - r, cx - r, cy, cx + r, cy, value)
    canvas.triangle(cx - r, cy, cx, cy + r, cx + r, cy, value)


BAYER4 = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))


def draw_aurora(canvas, top, height, tick, strength, taper=0):
    """scenery_draw_aurora(): curtains of dithered light on two slow waves.

    `taper` columns at each end fade the curtain out instead of cutting it.
    """
    for x in range(0, canvas.width, 2):
        wave = (math.sin(x * 0.018 + tick * 0.021) * 0.6
                + math.sin(x * 0.047 - tick * 0.013) * 0.4)
        lift = math.sin(x * 0.011 + tick * 0.008)
        y0 = top + int((lift + 1.0) * height * 0.18)
        length = int(height * (0.35 + 0.3 * (wave + 1.0) * 0.5))
        bright = strength * (0.45 + 0.55 * (wave + 1.0) * 0.5)
        if taper:
            bright *= min(1.0, x / taper, (canvas.width - 2 - x) / taper)
        for y in range(length):
            fall = 1.0 - y / length
            level = int(bright * fall * fall * 16.0)
            py = y0 + length - y
            for dx in (0, 1):
                if BAYER4[py & 3][(x + dx) & 3] < level:
                    canvas.set(x + dx, py, WHITE)
        if bright > 0.5:
            canvas.fill_rect(x, y0 + length, 2, 1, WHITE)


class Logo:
    """draw_logo() for English: FROSTLINE over HOLD, five pixels a font pixel."""

    SCALE = 5

    def __init__(self):
        top, bottom = "FROSTLINE", "HOLD"
        self.bits = set()
        latin_bits(top, 0, 0, self.bits)
        latin_bits(bottom, (latin_width(top) - latin_width(bottom)) // 2, 9, self.bits)
        self.height = 16
        self.width = max(c for _, c in self.bits) + 1
        self.shade = {4, 5, 6, 13, 14, 15}

    def bit(self, r, c):
        return (r, c) in self.bits

    def runs(self, canvas, ox, oy, dx, dy, grow, value):
        s = self.SCALE
        for r in range(self.height):
            c = 0
            while c < self.width:
                if not self.bit(r, c):
                    c += 1
                    continue
                start = c
                while c < self.width and self.bit(r, c):
                    c += 1
                canvas.fill_rect(ox + start * s + dx - grow, oy + r * s + dy - grow,
                                 (c - start) * s + grow * 2, s + grow * 2, value)

    def draw(self, canvas, cx, top, frame):
        """The whole logo at rest; returns its height."""
        s = self.SCALE
        ox = cx - self.width * s // 2
        oy = top
        # The dark halo, the block of ice's side, its outline and its face.
        self.runs(canvas, ox, oy, 2, 2, 6, BLACK)
        for d in (4, 3, 2):
            self.runs(canvas, ox, oy, d, d, 0, WHITE)
        self.runs(canvas, ox, oy, 0, 0, 1, BLACK)
        self.runs(canvas, ox, oy, 0, 0, 0, WHITE)
        for r in range(self.height):
            for c in range(self.width):
                if not self.bit(r, c):
                    continue
                px, py = ox + c * s, oy + r * s
                if r in self.shade:
                    canvas.fill_rect(px, py + s - 2, s, 1, BLACK)
                if not self.bit(r - 1, c) and not self.bit(r - 2, c):
                    canvas.fill_rect(px - 1, py - 3, s + 2, 2, WHITE)
                    if hash2(c, r + 9) % 3 == 0:
                        canvas.fill_rect(px, py - 4, s, 1, WHITE)
                if not self.bit(r + 1, c) and not self.bit(r + 2, c):
                    hh = hash2(c, r + 31)
                    if hh % 3 == 0:
                        length = 4 + (hh >> 8) % 8
                        ix, iy = px + s // 2, py + s + 2
                        canvas.triangle(ix - 2, iy, ix, iy + length + 1, ix + 2, iy, BLACK)
                        canvas.triangle(ix - 1, iy, ix, iy + length, ix + 1, iy, WHITE)
                        drip = (frame + (hh >> 4) % 90) % 90
                        if (hh >> 12) % 4 == 0 and drip < 14:
                            canvas.fill_rect(ix, iy + length + 2 + drip * drip // 6, 1, 2, WHITE)
        # The glint that skims along the tops.
        t = frame % 120
        if t < 28:
            col = int(self.width * t / 27.0)
            for r in range(self.height):
                if self.bit(r, col):
                    x, y = ox + col * s + s // 2, oy + r * s - 2
                    draw_twinkle(canvas, x, y, 4 if t % 4 < 2 else 3, WHITE)
                    canvas.set(x, y, BLACK)
                    break
        return self.height * s


def draw_text_rimmed(canvas, text, cx, y):
    x = cx - latin_width(text) // 2
    for dx in (-1, 0, 1):
        for dy in (-1, 0, 1):
            if dx or dy:
                draw_latin(canvas, text, x + dx, y + dy, BLACK)
    draw_latin(canvas, text, x, y, WHITE)


def draw_subtitle(canvas, cx, y):
    """The subtitle between two rules ending in diamonds, as on the title."""
    text = ui_text("game_subtitle")
    width = latin_width(text)
    # Clear night behind the line, so no star sits between the words.
    canvas.fill_rect(cx - width // 2 - 48, y - 3, width + 96, 13, BLACK)
    draw_text_rimmed(canvas, text, cx, y)
    rule_y = y + 7 // 2
    canvas.fill_rect(cx - width // 2 - 40, rule_y, 30, 1, WHITE)
    canvas.fill_rect(cx + width // 2 + 10, rule_y, 30, 1, WHITE)
    draw_diamond(canvas, cx - width // 2 - 44, rule_y, 2, WHITE)
    draw_diamond(canvas, cx + width // 2 + 44, rule_y, 2, WHITE)


# ------------------------------------------------------------------- export

def to_rgb(canvas, ink=INK, paper=PAPER):
    image = Image.new("RGB", (canvas.width, canvas.height))
    image.putdata([paper if canvas.is_white(x, y) else ink
                   for y in range(canvas.height) for x in range(canvas.width)])
    return image


def enlarge(image, factor):
    return image.resize((image.width * factor, image.height * factor), Image.NEAREST)


def save(image, name):
    OUT_DIR.mkdir(parents=True, exist_ok=True)
    path = OUT_DIR / name
    image.save(path, optimize=True)
    print(f"wrote {path.relative_to(PROJECT)} {image.width}x{image.height}")


# ------------------------------------------------------------------- banner

BANNER_W, BANNER_H = 480, 144   # game pixels, shown at 2x and stored at 4x
BANNER_FRAME = 130              # the clock for the aurora, drips and glint
LOGO_TOP = 24
# Bright stars clear of the lettering: (x, y, arm).
BANNER_STARS = ((34, 30, 3), (82, 106, 1), (118, 18, 1), (402, 22, 1),
                (444, 74, 3), (378, 116, 1), (20, 86, 1), (462, 128, 1))


def build_banner():
    canvas = Canvas(BANNER_W, BANNER_H)
    title_art._starfield(canvas, 0, BANNER_H, 0x1CE5)
    draw_aurora(canvas, 6, 88, BANNER_FRAME, 1.0, taper=72)
    for x, y, arm in BANNER_STARS:
        draw_twinkle(canvas, x, y, arm, WHITE)
    height = Logo().draw(canvas, BANNER_W // 2, LOGO_TOP, BANNER_FRAME)
    draw_subtitle(canvas, BANNER_W // 2, LOGO_TOP + height + 14)
    return enlarge(to_rgb(canvas), 4)


# --------------------------------------------------------------- background

# An AI engraving in the title plate's style: peaks on the left and right
# thirds, an empty valley in the middle where the page column covers it.
BG_SOURCE = AI_DIR / "page_background_source.jpg"
BG_DOT = 3                       # screen pixels per dot
BG_W, BG_H = 2560, 1440
BG_FADE = 70                     # dots of dithered fade at the bottom
BG_EDGE = 40                     # dots of fade at the far left and right


def build_background():
    cols, rows = -(-BG_W // BG_DOT), -(-BG_H // BG_DOT)
    source = Image.open(BG_SOURCE).convert("L")
    scale = cols / source.width
    grey = source.resize((cols, round(source.height * scale)), Image.BOX)
    grey = grey.filter(ImageFilter.UnsharpMask(radius=1.2, percent=90, threshold=2))
    plate = _dither(grey)
    canvas = Canvas(cols, rows)
    for y in range(rows):
        for x in range(cols):
            # Fade to ink by an ordered threshold, so the edge stays dots.
            keep = min(1.0, (rows - 1 - y) / BG_FADE, x / BG_EDGE, (cols - 1 - x) / BG_EDGE)
            white = y < plate.height and plate.is_white(x, y)
            if white and (BAYER4[y & 3][x & 3] + 0.5) / 16.0 < keep:
                canvas.set(x, y, WHITE)
            else:
                canvas.set(x, y, BLACK)
    image = enlarge(to_rgb(canvas, paper=DIM), BG_DOT)
    return image.crop((0, 0, BG_W, BG_H))


# --------------------------------------------------------------- embed bg

EMBED_SIZE = 720
EMBED_FRAME = "0200.png"
EMBED_AURORA_TICK = 400
# The buttons, their plinths and the flames beside START (frostline_model.h).
BUTTONS = (118, 360, 364, 472)
# The sky the logo and the aurora fill: the banner above already names the
# game, so the frame shows the hold under the aurora alone.
SKY_ROWS = 132


def build_embed_bg():
    path = TITLE_FRAMES / EMBED_FRAME
    if not path.exists():
        raise SystemExit(f"{path} is missing: run python3 tools/promo/capture.py --lang en")
    frame = Image.open(path).convert("RGB")
    plate = title_art.build_title_backdrop()
    sky = Canvas(480, SKY_ROWS)
    for y in range(SKY_ROWS):
        for x in range(480):
            sky.set(x, y, plate.get(x, y))
    draw_aurora(sky, 6, 88, EMBED_AURORA_TICK, 1.0)
    for i in range(22):
        # draw_star_twinkles() at rest: its bright stars, some breathing.
        h = hash2(i, 301)
        x, y = 40 + h % 400, 6 + (h >> 9) % 120
        draw_twinkle(sky, x, y, (3, 1, 0)[i % 3], WHITE)
    pixels = frame.load()
    for y in range(SKY_ROWS):
        for x in range(480):
            pixels[x, y] = PAPER if sky.is_white(x, y) else INK
    x0, y0, x1, y1 = BUTTONS
    for y in range(y0, y1):
        for x in range(x0, x1):
            pixels[x, y] = PAPER if plate.is_white(x, y) else INK
    # As the browser shows the game at 1.5x: each game pixel spread over its
    # share of screen pixels, the edges between them blended.
    return enlarge(frame, 3).resize((EMBED_SIZE, EMBED_SIZE), Image.BOX)


def main():
    save(build_banner(), "banner.png")
    save(build_background(), "background.png")
    save(build_embed_bg(), "embed_bg.png")


if __name__ == "__main__":
    main()

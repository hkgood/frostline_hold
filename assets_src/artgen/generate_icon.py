"""The app icon of the PC builds: the macOS .icns, the Windows .ico and the
store pictures.

Two drawings, one look. From 128 px up the icon is an AI plate
(ai/icon_source.jpg: a snow-capped watchtower under the stars) dithered to
ink and paper like every plate in the game, on a coarse dot grid so it reads
as the game does on screen, framed by the double rule of the game's panels.
Below that a dithered plate is only noise, so 16, 24 and 32 px are drawn by
hand, pixel by pixel; 48 and 64 px double the 24 and 32.

    python artgen/generate_icon.py      # from assets_src/; needs macOS for iconutil
"""
import io
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from artgen.canvas import INK_RGB, PAPER_RGB  # noqa: E402
from artgen.shade import BAYER4  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "assets_src" / "ai" / "icon_source.jpg"
PC = ROOT / "platform" / "pc"
ICNS = PC / "mac" / "FrostlineHold.icns"
ICO = PC / "windows" / "frostline.ico"
STORE = PC / "icon"

INK = INK_RGB + (255,)
PAPER = PAPER_RGB + (255,)
CLEAR = (0, 0, 0, 0)

# The plate's dot grid: 206 dots across the body, 4 px each on the 1024 px
# master, close to the game's 480 px screen shown at twice its size.
GRID = 206
# The part of the AI picture the plate shows (left, top, right, bottom):
# the tower large, the top of its crag, sky above the pennant. Negative
# values reach past the picture into more night.
CROP = (2, -40, 1022, 980)
# macOS draws an icon as a rounded square 824 px wide inside 1024, with
# room around it for the shadow; the corners are 22.4 % of the side.
MAC_BODY = 824 / 1024
CORNER = 0.224
# The frame: two rules in from the edge, in dots, like the game's panels.
RULES = (5, 8)

# Stars in the sky, in dots: (x, y, arm length). The AI picture's own faint
# stars are cleared: at this grid they dither to stray dots.
STARS = [(24, 40, 2), (178, 52, 2), (30, 112, 1), (176, 108, 1),
         (186, 150, 1), (20, 160, 1), (150, 26, 1), (52, 22, 1)]

# The small sizes, by hand. '#' paper, ':' paper and ink in a checker (the
# game's half tone, for the shaded side), anything else the ink plate.
# INSET and RADIUS shape the plate around them.
SMALL = {
    16: (1, 3, [
        "................",
        "........###.....",
        "........#.......",
        ".......##.......",
        "......##:#......",
        ".....######.....",
        "....#.####.#....",
        "....########....",
        ".....####:#.....",
        ".....##..:#.....",
        ".....##..:#.....",
        ".....####:#.....",
        "....########....",
        "..##::::::::##..",
        ".::::::::::::::.",
        "................",
    ]),
    24: (1, 4, [
        "........................",
        "........................",
        "............###.........",
        "............####........",
        "............#...........",
        "...........##...........",
        "..........####..........",
        "..........##:#..........",
        ".........###::#.........",
        "........########........",
        ".........####:#.........",
        ".......##.#..#.##.......",
        ".......##########.......",
        ".......#######::#.......",
        "........######:#........",
        "........###..#:#........",
        "........###..#:#........",
        "........###..#:#........",
        "........######:#........",
        "........######:#........",
        ".......##########.......",
        "....###::::::::::###....",
        "..::::::::::::::::::::..",
        "........................",
    ]),
    32: (2, 6, [
        "................................",
        "................................",
        "................................",
        "................#####...........",
        "................######..........",
        "................####............",
        "................#...............",
        "...............##...............",
        "......#.......####..............",
        "..............##:#..............",
        ".............###::#.............",
        ".............###::#.......#.....",
        "............####:::#............",
        "...........##########...........",
        "............#####::#............",
        "............#####::#............",
        "..........##.##..##.##..........",
        "..........############..........",
        "..........########:::#..........",
        "...........#.#.##.#.#...........",
        "...........#######::#...........",
        "...........#######::#...........",
        "...........####..#::#...........",
        "...........####..#::#...........",
        "...........####..#::#...........",
        "...........####..#::#...........",
        "...........#######::#...........",
        "..........############..........",
        ".......###::::::::::::###.......",
        "....::::::::::::::::::::::::....",
        "................................",
        "................................",
    ]),
}


def rounded_mask(size, inset, radius):
    """A rounded square, antialiased, `inset` px in from every edge."""
    scale = 4
    mask = Image.new("L", (size * scale, size * scale), 0)
    ImageDraw.Draw(mask).rounded_rectangle(
        (inset * scale, inset * scale,
         (size - inset) * scale - 1, (size - inset) * scale - 1),
        radius=radius * scale, fill=255)
    return mask.resize((size, size), Image.LANCZOS)


# ----------------------------------------------------------- large plate

def dithered_plate():
    """The AI picture as ink and paper dots, GRID across, framed."""
    picture = Image.open(SOURCE).convert("L")
    night = Image.new("L", (CROP[2] - CROP[0], CROP[3] - CROP[1]), 0)
    night.paste(picture, (-CROP[0], -CROP[1]))
    grey = night.resize((GRID, GRID), Image.BOX)
    grey = grey.filter(ImageFilter.UnsharpMask(radius=1.2, percent=90, threshold=2))
    plate = Image.new("RGBA", (GRID, GRID))
    tones = grey.load()
    dots = plate.load()
    for y in range(GRID):
        row = BAYER4[y % 4]
        for x in range(GRID):
            # Lifted mid tones: the stonework reads as a lit tower against
            # the night rather than a grey shape in it.
            tone = min(1.0, max(0.0, (tones[x, y] / 255.0 - 0.10) / 0.60))
            dots[x, y] = PAPER if tone > (row[x % 4] + 0.5) / 16.0 else INK

    # Clear the sky: a paper dot with little paper around it is one of the
    # picture's stars, and the band under the frame stays plain night.
    paper = [[dots[x, y] == PAPER for x in range(GRID)] for y in range(GRID)]
    band = RULES[1] + 2
    for y in range(GRID):
        for x in range(GRID):
            if not paper[y][x]:
                continue
            near = sum(paper[j][i]
                       for j in range(max(0, y - 4), min(GRID, y + 5))
                       for i in range(max(0, x - 4), min(GRID, x + 5)))
            edge = min(x, y, GRID - 1 - x) < band
            if near <= 9 or (edge and y < GRID - band):
                dots[x, y] = INK

    for sx, sy, arm in STARS:
        for d in range(-arm, arm + 1):
            dots[sx + d, sy] = PAPER
            dots[sx, sy + d] = PAPER

    # The rules run unbroken: paper over the night, ink across the crag.
    rules = Image.new("L", (GRID, GRID), 0)
    draw = ImageDraw.Draw(rules)
    radius = round(GRID * CORNER)
    for inset in RULES:
        draw.rounded_rectangle((inset, inset, GRID - 1 - inset, GRID - 1 - inset),
                               radius=radius - inset, outline=255, width=1)
    marks = rules.load()
    for y in range(GRID):
        for x in range(GRID):
            if marks[x, y]:
                dots[x, y] = INK if dots[x, y] == PAPER else PAPER
    return plate


def large_icon(plate, size, body):
    """The plate on a rounded square `body` px wide, centred in `size`."""
    whole = max(1, body // GRID) * GRID
    art = plate.resize((whole, whole), Image.NEAREST)
    if whole != body:
        art = art.resize((body, body), Image.LANCZOS)
    inset = (size - body) // 2
    icon = Image.new("RGBA", (size, size), CLEAR)
    icon.paste(art, (inset, inset))
    icon.putalpha(rounded_mask(size, inset, round(body * CORNER)))
    return icon


# ----------------------------------------------------------- small sizes

def small_icon(size):
    """A hand-drawn size, or twice one: 48 from 24, 64 from 32."""
    if size not in SMALL:
        return small_icon(size // 2).resize((size, size), Image.NEAREST)
    inset, radius, rows = SMALL[size]
    assert len(rows) == size and all(len(row) == size for row in rows), size
    icon = Image.new("RGBA", (size, size), INK)
    dots = icon.load()
    for y, row in enumerate(rows):
        for x, mark in enumerate(row):
            if mark == "#" or (mark == ":" and (x + y) % 2 == 0):
                dots[x, y] = PAPER
    icon.putalpha(rounded_mask(size, inset, radius))
    return icon


# ---------------------------------------------------------------- icons

def with_shadow(icon):
    """The soft shadow a macOS icon stands on."""
    size = icon.width
    shadow = Image.new("RGBA", (size, size), (0, 0, 0, 255))
    shadow.putalpha(icon.getchannel("A").point(lambda v: int(v * 0.4)))
    shadow = shadow.filter(ImageFilter.GaussianBlur(size / 90))
    framed = Image.new("RGBA", (size, size), CLEAR)
    framed.alpha_composite(shadow, (0, round(size / 110)))
    framed.alpha_composite(icon)
    return framed


def mac_icon(plate, size):
    if size < 128:
        return small_icon(size)
    return with_shadow(large_icon(plate, size, round(size * MAC_BODY)))


def windows_icon(plate, size):
    if size < 128:
        return small_icon(size)
    return large_icon(plate, size, size - 2 * round(size * 0.02))


def write_icns(plate):
    with tempfile.TemporaryDirectory() as folder:
        iconset = Path(folder) / "FrostlineHold.iconset"
        iconset.mkdir()
        for points in (16, 32, 128, 256, 512):
            mac_icon(plate, points).save(iconset / f"icon_{points}x{points}.png")
            mac_icon(plate, points * 2).save(iconset / f"icon_{points}x{points}@2x.png")
        ICNS.parent.mkdir(parents=True, exist_ok=True)
        subprocess.run(["iconutil", "-c", "icns", str(iconset), "-o", str(ICNS)], check=True)


def write_ico(plate):
    """A Vista-style .ico, one PNG per size; Windows picks one per DPI."""
    sizes = (16, 24, 32, 48, 64, 256)
    blobs = []
    for size in sizes:
        data = io.BytesIO()
        windows_icon(plate, size).save(data, "PNG")
        blobs.append(data.getvalue())
    header = struct.pack("<HHH", 0, 1, len(sizes))
    offset = 6 + 16 * len(sizes)
    entries = b""
    for size, blob in zip(sizes, blobs):
        # A 256 px entry is written as 0 in the one-byte size fields.
        entries += struct.pack("<BBBBHHII", size % 256, size % 256, 0, 0, 1, 32,
                               len(blob), offset)
        offset += len(blob)
    ICO.parent.mkdir(parents=True, exist_ok=True)
    ICO.write_bytes(header + entries + b"".join(blobs))


def write_store(plate):
    """The 1024 px master, Steam's 184 px community icon, a square for the
    itch.io page, and a sheet of every size for review."""
    STORE.mkdir(parents=True, exist_ok=True)
    mac_icon(plate, 1024).save(STORE / "frostline_icon_1024.png")
    square = plate.resize((GRID * 4, GRID * 4), Image.NEAREST).convert("RGB")
    square.resize((184, 184), Image.LANCZOS).save(STORE / "steam_community_184.jpg", quality=95)
    square.resize((512, 512), Image.LANCZOS).save(STORE / "itch_square_512.png")

    sizes = (16, 24, 32, 48, 64, 128, 256)
    sheet = Image.new("RGBA", (sum(sizes) + 16 * (len(sizes) + 1), 256 + 32), PAPER)
    x = 16
    for size in sizes:
        sheet.alpha_composite(mac_icon(plate, size), (x, 256 + 16 - size))
        x += size + 16
    sheet.save(STORE / "icon_sizes_preview.png")


def main():
    plate = dithered_plate()
    write_icns(plate)
    write_ico(plate)
    write_store(plate)
    for path in (ICNS, ICO, STORE):
        print(path.relative_to(ROOT))


if __name__ == "__main__":
    main()

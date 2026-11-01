"""The app icon of the PC builds: the macOS .icns, the Windows .ico and the
store pictures; and the Android launcher icon's layers.

Two drawings, one look. From 128 px up the icon is an AI plate
(ai/icon_source.jpg: a snow-capped watchtower under the stars) dithered to
ink and paper like every plate in the game, on a coarse dot grid so it reads
as the game does on screen, framed by the double rule of the game's panels.
Below that a dithered plate is only noise, so 16, 24 and 32 px are drawn by
hand, pixel by pixel; 48 and 64 px double the 24 and 32.

macOS 26 draws its icons from an Icon Composer file instead (.icon, built into
Assets.car by actool): the same plate as one layer, which the system masks,
scales and shades itself. The .icns stays for older systems.

    python artgen/generate_icon.py      # from assets_src/; needs macOS for iconutil
"""
import io
import json
import math
import shutil
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
ICON = PC / "mac" / "FrostlineHold.icon"
ICO = PC / "windows" / "frostline.ico"
STORE = PC / "icon"
ANDROID_RES = ROOT / "platform" / "android" / "app" / "src" / "main" / "res"
# Pixels per dp of each launcher density.
ANDROID_DENSITIES = {"mdpi": 1, "hdpi": 1.5, "xhdpi": 2, "xxhdpi": 3, "xxxhdpi": 4}

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
# macOS 26 masks an Icon Composer layer to the whole canvas, but its corners
# are not circular: each is a superellipse of this power, this share of the
# side long. The frame follows them.
SQUIRCLE_CORNER = 0.342
SQUIRCLE_POWER = 2.8
ICON_CANVAS = 1024

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

def rounded_rules():
    """The frame's rules on the rounded square of the .icns and the .ico."""
    rules = Image.new("L", (GRID, GRID), 0)
    draw = ImageDraw.Draw(rules)
    radius = round(GRID * CORNER)
    for inset in RULES:
        draw.rounded_rectangle((inset, inset, GRID - 1 - inset, GRID - 1 - inset),
                               radius=radius - inset, outline=255, width=1)
    return rules


def squircle_outline(low, high, corner, steps=64):
    """The points round a square from `low` to `high` on both axes, its
    corners superelliptic quarters `corner` long."""
    centres = ((high - corner, high - corner), (low + corner, high - corner),
               (low + corner, low + corner), (high - corner, low + corner))
    points = []
    for quarter, (cx, cy) in enumerate(centres):
        for step in range(steps + 1):
            angle = (quarter + step / steps) * math.pi / 2
            c, s = math.cos(angle), math.sin(angle)
            points.append((cx + corner * math.copysign(abs(c) ** (2 / SQUIRCLE_POWER), c),
                           cy + corner * math.copysign(abs(s) ** (2 / SQUIRCLE_POWER), s)))
    return points


def squircle_rules():
    """The frame's rules on the shape macOS 26 masks the icon to."""
    rules = Image.new("L", (GRID, GRID), 0)
    draw = ImageDraw.Draw(rules)
    for inset in RULES:
        outline = squircle_outline(inset, GRID - 1 - inset, GRID * SQUIRCLE_CORNER - inset)
        draw.line(outline + outline[:1], fill=255, width=1)
    return rules


def dithered_plate(rules=rounded_rules):
    """The AI picture as ink and paper dots, GRID across, framed by `rules`."""
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
    marks = rules().load()
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


def write_icon_composer():
    """The macOS 26 icon: the ink is the file's fill and the paper dots the
    one layer over it, with Liquid Glass off so the dots stay crisp. Edits
    made in Icon Composer are lost on the next run."""
    plate = dithered_plate(squircle_rules)
    plate.putalpha(plate.convert("L").point(lambda v: 255 if v > 128 else 0))
    if ICON.exists():
        shutil.rmtree(ICON)
    (ICON / "Assets").mkdir(parents=True)
    plate.resize((ICON_CANVAS, ICON_CANVAS), Image.NEAREST).save(ICON / "Assets" / "plate.png")
    ink = ",".join(f"{c / 255:.5f}" for c in INK_RGB)
    document = {
        "fill": {"solid": f"srgb:{ink},1.00000"},
        "groups": [{
            "layers": [{"glass": False, "image-name": "plate.png", "name": "plate"}],
            "shadow": {"kind": "none", "opacity": 0.5},
            "specular": False,
            "translucency": {"enabled": False, "value": 0.5},
        }],
        "supported-platforms": {"squares": ["macOS"]},
    }
    # Icon Composer's own layout, so a file saved there diffs cleanly.
    text = json.dumps(document, indent=2, separators=(",", " : "), sort_keys=True)
    (ICON / "icon.json").write_text(text + "\n")


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


def write_android():
    """The Android adaptive icon's layers, one PNG per density. The launcher
    masks the icon to its own shape, so the plate has no rules; it fills the
    72 dp viewport in the middle of the 108 dp layer, and the ink
    background (res/values/colors.xml) shows past it when the launcher
    shifts the layer. The monochrome layer is the paper dots alone, which
    Android 13 tints for themed icons."""
    plate = dithered_plate(lambda: Image.new("L", (GRID, GRID), 0))
    for name, density in ANDROID_DENSITIES.items():
        layer, view = round(108 * density), round(72 * density)
        whole = max(1, view // GRID) * GRID
        art = plate.resize((whole, whole), Image.NEAREST)
        if whole != view:
            art = art.resize((view, view), Image.LANCZOS)
        inset = (layer - view) // 2
        foreground = Image.new("RGBA", (layer, layer), CLEAR)
        foreground.paste(art, (inset, inset))
        lit = art.convert("L").point(lambda v: 255 if v > 128 else 0)
        monochrome = Image.new("RGBA", (layer, layer), CLEAR)
        monochrome.paste(Image.new("RGBA", (view, view), (255, 255, 255, 255)),
                         (inset, inset), lit)
        folder = ANDROID_RES / f"mipmap-{name}"
        folder.mkdir(parents=True, exist_ok=True)
        foreground.save(folder / "ic_launcher_foreground.png", optimize=True)
        monochrome.save(folder / "ic_launcher_monochrome.png", optimize=True)


def main():
    plate = dithered_plate()
    write_icns(plate)
    write_icon_composer()
    write_ico(plate)
    write_store(plate)
    write_android()
    for path in (ICNS, ICON, ICO, STORE, ANDROID_RES):
        print(path.relative_to(ROOT))


if __name__ == "__main__":
    main()

"""Level backgrounds assembled from AI-drawn engravings plus procedural detail.

Every level has an AI plate under assets_src/ai/, a 1-bit style engraving
generated once from that level's layout guide and committed as a source
asset. This module thresholds a plate to a strict two-colour bitmap and then
makes the road pixel exact:

  * an "engraving" plate already shows the road; the trail is only re-cut
    where the drawing runs dark, so a black enemy never vanishes into it
  * a "carve" plate was drawn without a road; the road is cut into it with
    the same pen language - a trodden band, inked banks and wheel ruts
"""
import math
import random
from pathlib import Path

from PIL import Image, ImageFilter

from artgen.canvas import BLACK, WHITE, Canvas
from artgen import level_geom as geom
from artgen import levels as campaign
from artgen import shade as S
from artgen.shade import BAYER4

AI_DIR = Path(__file__).resolve().parent.parent / "ai"
AI_SOURCE = AI_DIR / "terrain_source.jpg"

ROAD_WIDTH_A = 16
ROAD_WIDTH_B = 12
CORRIDOR_MARGIN = 20


def _plate_grey(source, size=480):
    """An AI plate as a sharpened greyscale image, `size` px square."""
    grey = Image.open(source).convert("L").resize((size, size), Image.BOX)
    return grey.filter(ImageFilter.UnsharpMask(radius=1.2, percent=90,
                                               threshold=2))


def _dither(grey):
    """A greyscale image to a strict two-state bitmap, keeping its mid tones.

    A hard threshold throws away the engraving's grey hatching. Ordered
    dithering after sharpening keeps every pen line crisp while the forest
    and cliff shading survive as fine dot patterns.
    """
    pixels = grey.load()
    canvas = Canvas(grey.width, grey.height)
    for y in range(grey.height):
        row = BAYER4[y % 4]
        for x in range(grey.width):
            tone = pixels[x, y] / 255.0
            # Paper stays paper and ink stays ink; only the middle dithers.
            tone = min(1.0, max(0.0, (tone - 0.14) / 0.74))
            threshold = (row[x % 4] + 0.5) / 16.0
            canvas.set(x, y, WHITE if tone > threshold else BLACK)
    return canvas


def _threshold(source, size=480):
    """AI art to a strict two-state bitmap."""
    return _dither(_plate_grey(source, size))


# --- The castle at the end of the road ----------------------------------------
# An AI engraving (ai/castle_source.jpg), drawn bold so it survives being
# shrunk to ~100 px, seen from the upper left with its gate in the west face
# of the gatehouse, where every road arrives. The plate is a box at
# CASTLE_ORIGIN; the view hangs the moving banner and brazier fire on it and
# keeps a copy of these offsets, so they have to agree.
CASTLE_SOURCE = AI_DIR / "castle_source.jpg"
CASTLE_SCALE = 0.13
CASTLE_SOURCE_GATE = (685, 715)   # foot of the gate in the source art, px
CASTLE_W = 122
CASTLE_H = 106
CASTLE_ORIGIN = (geom.GOAL[0] - 92, geom.GOAL[1] + 16 - 100)
CASTLE_GATE = (90, 92)            # foot of the gate, local
CASTLE_GATE_LEFT = 84             # west jamb of the gate, local x
CASTLE_ROAD = (76, 92)            # local rows the road runs through
CASTLE_POLE_TOP = (99, 21)
CASTLE_BRAZIERS = ((76, 74), (78, 99))

# Which look each chapter's castle has, by stage: the journey gets colder,
# and at the end the hold has been fought over.
CASTLE_LOOK_BY_STAGE = ("plain", "plain", "plain", "plain",
                        "frozen", "frozen", "battered", "battered")
# Where icicles hang: (x0, y0, x1, y1) lines along the eaves, local px.
CASTLE_EAVES = ((81, 55, 99, 63), (101, 62, 119, 52), (21, 49, 50, 50),
                (50, 60, 80, 62))


def _castle_tone(value, x, y):
    """Ink or paper for a grey level: engraved lines, not dots.

    The darkest greys print solid; the middle ones become diagonal hatching
    that thins out towards the light, the way the maps' pen shading does.
    """
    if value < 45:
        return BLACK
    if value < 100:
        return BLACK if (x + y) % 3 != 0 else WHITE
    if value < 165:
        return BLACK if (x + y) % 3 == 0 else WHITE
    return WHITE


def _castle_silhouette(source):
    """Which source pixels are castle: everything not reached from the border.

    The paper round the drawing and the grey road the AI drew in front of
    the gate are both light, so a flood fill through light pixels from the
    edge takes them away and leaves the building and its cast shadow.
    """
    width, height = source.size
    pixels = source.load()
    outside = bytearray(width * height)
    stack = [(x, y) for x in range(width) for y in (0, height - 1)]
    stack += [(x, y) for y in range(height) for x in (0, width - 1)]
    while stack:
        x, y = stack.pop()
        if not (0 <= x < width and 0 <= y < height):
            continue
        index = y * width + x
        if outside[index] or pixels[x, y] < 150:
            continue
        outside[index] = 1
        stack.extend(((x + 1, y), (x - 1, y), (x, y + 1), (x, y - 1)))
    return Image.frombytes("L", source.size, bytes(0 if o else 255 for o in outside))


def _castle_base():
    """The castle as a bitmap plus its mask, both CASTLE_W x CASTLE_H."""
    source = Image.open(CASTLE_SOURCE).convert("L")
    mask = _castle_silhouette(source)
    # Thicken the pen lines first, so they still close at a seventh of the size.
    inked = source.filter(ImageFilter.MinFilter(3))
    size = (round(source.width * CASTLE_SCALE), round(source.height * CASTLE_SCALE))
    grey = inked.resize(size, Image.BOX).filter(
        ImageFilter.UnsharpMask(radius=0.8, percent=120, threshold=2))
    small_mask = mask.resize(size, Image.BOX)
    ox = CASTLE_GATE[0] - round(CASTLE_SOURCE_GATE[0] * CASTLE_SCALE)
    oy = CASTLE_GATE[1] - round(CASTLE_SOURCE_GATE[1] * CASTLE_SCALE)

    art = Canvas(CASTLE_W, CASTLE_H)
    solid = [[False] * CASTLE_W for _ in range(CASTLE_H)]
    gp, mp = grey.load(), small_mask.load()
    road_top, road_bottom = CASTLE_ROAD
    for y in range(CASTLE_H):
        for x in range(CASTLE_W):
            sx, sy = x - ox, y - oy
            if not (0 <= sx < size[0] and 0 <= sy < size[1]) or mp[sx, sy] < 128:
                continue
            # West of the gate the road runs in front of the keep and the
            # curtain, so it hides their feet.
            if x < CASTLE_GATE_LEFT and road_top <= y <= road_bottom + 6:
                continue
            solid[y][x] = True
            art.set(x, y, _castle_tone(gp[sx, sy], x, y))
    return art, solid


def _outline(art, solid):
    """Ink round the silhouette, paper one pixel outside it."""
    def inside(x, y):
        return 0 <= x < CASTLE_W and 0 <= y < CASTLE_H and solid[y][x]
    halo = []
    for y in range(CASTLE_H):
        for x in range(CASTLE_W):
            edge = [inside(x + dx, y + dy) for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1))]
            if solid[y][x] and not all(edge):
                art.set(x, y, BLACK)
            elif not solid[y][x] and any(edge):
                halo.append((x, y))
    return halo


def _icicles(art, solid, rng):
    """Icicles under the eaves: 3-7 px drips in the colour the wall is not."""
    for x0, y0, x1, y1 in CASTLE_EAVES:
        steps = max(1, abs(x1 - x0))
        for i in range(0, steps + 1, 2):
            x = x0 + (x1 - x0) * i // steps
            y = y0 + (y1 - y0) * i // steps
            if not solid[y][x]:
                continue
            ink = art.get(x, y + 3) == WHITE
            length = 3 + int(rng.random() * 5)
            for k in range(length):
                if y + k < CASTLE_H and solid[y + k][x]:
                    art.set(x, y + k, BLACK if ink else WHITE)
                    # The root of a long icicle is two pixels thick.
                    if k < length // 2 and length > 4 and solid[y + k][x + 1]:
                        art.set(x + 1, y + k, BLACK if ink else WHITE)


def _battle_scars(art, solid, rng):
    """Cracks across the lit stone, a breach in the curtain, soot over the gate."""
    # Zigzag cracks, two pixels wide, down the lit faces of keep and gatehouse.
    for x, y, length in ((32, 56, 18), (24, 66, 10), (92, 64, 8)):
        cx = x
        for k in range(length):
            cx += (1 if (k // 3) % 2 else -1) if k % 3 == 0 else 0
            for dx in (0, 1):
                if solid[y + k][cx + dx]:
                    art.set(cx + dx, y + k, BLACK)
    # A breach knocked out of the curtain wall's battlement: a ragged notch.
    for y in range(54, 66):
        for x in range(58, 70):
            if y - 54 < 12 - abs(x - 64) * 1.6:
                solid[y][x] = False
    # Soot licked up the wall over the gate.
    for y in range(58, 72):
        for x in range(CASTLE_GATE_LEFT - 1, CASTLE_GATE_LEFT + 13):
            if solid[y][x] and rng.random() < 0.15 + 0.04 * (y - 58):
                art.set(x, y, BLACK)


def _braziers(art, solid):
    """Iron fire-baskets on posts either side of the road. The view lights them."""
    for bx, by in CASTLE_BRAZIERS:
        for y in range(by - 4, min(by + 9, CASTLE_H)):
            for x in range(bx - 4, min(bx + 5, CASTLE_W)):
                bowl = by - 3 <= y <= by and abs(x - bx) <= 3 - (y - by + 3) // 3
                post = y > by and abs(x - bx) <= 1
                if bowl or post:
                    solid[y][x] = True
                    art.set(x, y, BLACK)
                elif (by - 4 <= y <= by + 1 and abs(x - bx) <= 4
                      or y > by and abs(x - bx) <= 2):
                    art.set(x, y, WHITE)
                    solid[y][x] = True
        art.set(bx - 1, by - 3, WHITE)          # a lit rim
        art.set(bx, by - 3, WHITE)


def castle_plate(look="plain"):
    """The castle for one look, as (bitmap, mask) in its CASTLE_W box."""
    rng = random.Random(0xCA57)
    art, solid = _castle_base()
    if look in ("frozen", "battered"):
        _icicles(art, solid, rng)
    if look == "battered":
        _battle_scars(art, solid, rng)
    _braziers(art, solid)
    halo = _outline(art, solid)
    return art, solid, halo


def draw_gatehouse(canvas, look="plain"):
    """Bake the castle into the map at the end of the road."""
    ox, oy = CASTLE_ORIGIN
    art, solid, halo = castle_plate(look)
    for x, y in halo:
        if 0 <= ox + x < canvas.width and 0 <= oy + y < canvas.height:
            canvas.set(ox + x, oy + y, WHITE)
    for y in range(CASTLE_H):
        for x in range(CASTLE_W):
            if solid[y][x] and 0 <= ox + x < canvas.width:
                canvas.set(ox + x, oy + y, art.get(x, y))


def clear_disc(canvas, cx, cy, radius, margin, rng):
    """Open a clearing with a stippled fade, so a built object reads."""
    for y in range(max(0, cy - radius - margin), min(canvas.height, cy + radius + margin + 1)):
        for x in range(max(0, cx - radius - margin), min(canvas.width, cx + radius + margin + 1)):
            distance = ((x - cx) ** 2 + (y - cy) ** 2) ** 0.5
            if distance <= radius:
                canvas.set(x, y, WHITE)
            elif distance <= radius + margin:
                fade = (radius + margin - distance) / float(margin)
                if rng.random() < fade * fade:
                    canvas.set(x, y, WHITE)


def _dark_ratio(canvas, cx, cy, radius):
    """How much of the neighbourhood is ink; 1.0 means solid black."""
    dark = 0
    total = 0
    for y in range(max(0, cy - radius), min(canvas.height, cy + radius + 1)):
        for x in range(max(0, cx - radius), min(canvas.width, cx + radius + 1)):
            total += 1
            if canvas.get(x, y) == 0:
                dark += 1
    return (dark / float(total)) if total else 0.0


def clear_dark_track(canvas, points, radius, rng, jitter=1.2):
    """Open a trail only where the lane runs through ink.

    The engraving already draws the road as a pale band with a dark edge.
    Clearing that band would throw the drawing away, so the trail is cut
    only where the ground is genuinely black - the forest at the entrance,
    for instance - which is exactly where a black enemy would disappear.
    """
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        steps = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
        for step in range(steps + 1):
            t = step / float(steps)
            cx = int(x0 + (x1 - x0) * t)
            cy = int(y0 + (y1 - y0) * t)
            if _dark_ratio(canvas, cx, cy, 9) < 0.45:
                continue
            ox = rng.uniform(-jitter, jitter)
            oy = rng.uniform(-jitter, jitter)
            for dy in range(-radius - 2, radius + 3):
                for dx in range(-radius - 2, radius + 3):
                    distance = (dx * dx + dy * dy) ** 0.5
                    px, py = int(cx + ox) + dx, int(cy + oy) + dy
                    if not (0 <= px < canvas.width and 0 <= py < canvas.height):
                        continue
                    if distance <= radius:
                        canvas.set(px, py, WHITE)
                    elif distance <= radius + 2:
                        if rng.random() < 0.6:
                            canvas.set(px, py, WHITE)


def _road_field(routes, reach):
    """Distance to the nearest road for every pixel within reach of one.

    Returns {(x, y): (distance, side, along)} where side is the sign of the
    cross product (which bank the pixel is on) and along is the arc length,
    so the banks can be dashed and the ruts broken up.
    """
    field = {}
    for points in routes:
        along = 0.0
        for index in range(len(points) - 1):
            x0, y0 = points[index]
            x1, y1 = points[index + 1]
            dx, dy = x1 - x0, y1 - y0
            length = math.hypot(dx, dy)
            if length < 1e-6:
                continue
            for py in range(int(min(y0, y1) - reach), int(max(y0, y1) + reach) + 2):
                for px in range(int(min(x0, x1) - reach), int(max(x0, x1) + reach) + 2):
                    cx, cy = px + 0.5 - x0, py + 0.5 - y0
                    t = max(0.0, min(1.0, (cx * dx + cy * dy) / (length * length)))
                    ex, ey = cx - t * dx, cy - t * dy
                    distance = math.hypot(ex, ey)
                    if distance > reach:
                        continue
                    best = field.get((px, py))
                    if best is None or distance < best[0]:
                        side = 1 if (dx * cy - dy * cx) > 0 else -1
                        field[(px, py)] = (distance, side, along + t * length)
            along += length
    return field


def carve_road(canvas, routes, rng, half_width=10):
    """Cut a trodden road into a plate that was drawn without one.

    The band is cleared to snow, the banks are inked as broken pen lines -
    heavier on the bank that faces away from the light, as the engraving
    shades its rocks - and two rows of sparse dots mark the wheel ruts.
    """
    field = _road_field(routes, half_width + 6)
    for (x, y), (distance, side, along) in field.items():
        if not (0 <= x < canvas.width and 0 <= y < canvas.height):
            continue
        if distance <= half_width:
            canvas.set(x, y, WHITE)
            # Wheel ruts: two broken dotted lines either side of the centre.
            rut = abs(distance - half_width * 0.45)
            if rut < 0.6 and S.hash01(int(along) // 2, side, 17) < 0.3:
                canvas.set(x, y, BLACK)
            continue
        bank = distance - half_width
        # The light comes from the top left, so the right / lower bank of
        # every stretch is the shadowed one and takes the heavier line.
        shadowed = side > 0
        gap = S.hash01(int(along) // 7, side, 5) < 0.12
        if bank <= 1.0 and not gap:
            canvas.set(x, y, BLACK)
        elif bank <= 2.0 and shadowed and not gap:
            canvas.set(x, y, BLACK)
        elif bank <= 6.0:
            # A stippled shoulder melts the bank into the drawing around it.
            fade = (6.0 - bank) / 4.0
            if rng.random() < fade * 0.8:
                canvas.set(x, y, WHITE)
            elif shadowed and bank <= 3.5 and S.hash01(x, y, 41) < 0.35:
                canvas.set(x, y, BLACK)


def _clear_sites(canvas, level, rng):
    """Clearings so every build pad and hero post reads as open ground."""
    for x, y in level["spots"]:
        clear_disc(canvas, x, y, 14, 12, rng)
        # A tower stands up to 44 px tall over its spot; clear the snow
        # behind its body too, or a drawn boulder swallows the sprite. On a
        # cliff plate that would punch holes in the rock, so it is optional.
        if level.get("clear_behind", True):
            clear_disc(canvas, x, y - 20, 14, 10, rng)
    for x, y in level["posts"]:
        clear_disc(canvas, x, y, 11, 10, rng)


def build_level_background(level):
    """The finished 480x480 battlefield of one level."""
    rng = random.Random(0xF1E1D)
    canvas = _threshold(AI_DIR / level["plate"])
    routes = [route for route in campaign.routes(level) if route]

    if level["art"] == "carve":
        carve_road(canvas, routes, rng)
    else:
        # The lanes decide where the enemies walk, so the trail follows them.
        for route in routes:
            clear_dark_track(canvas, route, 9, rng)
        # Where the drawn road stops short of the gate, cut the rest.
        if "carve_from" in level:
            fx, fy = level["carve_from"]
            lane_a = routes[0]
            start = min(range(len(lane_a)),
                        key=lambda i: (lane_a[i][0] - fx) ** 2 + (lane_a[i][1] - fy) ** 2)
            carve_road(canvas, [lane_a[start:]], rng)

    _clear_sites(canvas, level, rng)

    # The castle sits at the end of the road. Clearing round it takes the
    # road's last stretch too, so cut that again: it runs in front of the
    # keep and into the gate.
    gx, gy = geom.GOAL
    clear_disc(canvas, gx - 8, gy - 6, 58, 16, rng)
    carve_road(canvas, [[(gx - 100, gy), (gx, gy)]], rng)
    look = CASTLE_LOOK_BY_STAGE[level.get("stage", 0)]
    draw_gatehouse(canvas, look)
    return canvas


# How far each side of a seam the two plates fade into open snow, px.
SEAM_MELT = 40


def _melt(grey, seam):
    """Blend two plates where they meet, so no cut shows.

    Each side of the seam cross-fades into its neighbour's edge mirrored
    across the seam, reaching half and half on the seam itself, so tone
    and snow texture run straight through it. A light lift towards paper
    thins the mirrored ghost of any landmark near the edge, and the blend's
    width wanders down the seam so it reads as drifts, not a ruled band.
    """
    src = grey.copy().load()
    pixels = grey.load()
    for y in range(grey.height):
        # Smooth wobble: interpolate a random width every 16 rows.
        block, frac = divmod(y, 16)
        a = S.hash01(block, seam, 31)
        b = S.hash01(block + 1, seam, 31)
        reach = SEAM_MELT * (0.65 + 0.7 * (a + (b - a) * frac / 16.0))
        for x in range(max(0, seam - int(reach)),
                       min(grey.width, seam + int(reach))):
            mirror = 2 * seam - 1 - x
            d = min(1.0, abs(x + 0.5 - seam) / reach)
            near = 1.0 - d * d * (3.0 - 2.0 * d)   # 1 at the seam, 0 at reach
            tone = src[x, y] * (1.0 - 0.5 * near) + src[mirror, y] * 0.5 * near
            pixels[x, y] = int(255 - (255 - tone) * (1.0 - 0.35 * near))


def build_campaign_background():
    """The campaign scroll: the journey plates with a clearing at every node.

    The plates are laid side by side in greyscale and melted together at
    each seam before one dither pass, so the scroll reads as one drawing.
    The trail and the medallions are drawn by the view, so a locked stage
    can look different from a cleared one. A chapter's own level has a
    medallion and an interlude a small marker, so their clearings differ.
    """
    rng = random.Random(0xCA4)
    size = 480
    grey = Image.new("L", (campaign.CAMPAIGN_WIDTH, size), 255)
    for index, plate in enumerate(campaign.CAMPAIGN_PLATES):
        grey.paste(_plate_grey(AI_DIR / plate, size), (index * size, 0))
    for index in range(1, len(campaign.CAMPAIGN_PLATES)):
        _melt(grey, index * size)
    canvas = _dither(grey)
    for level in campaign.LEVELS:
        x, y = level["node"]
        if level["interlude"]:
            clear_disc(canvas, x, y, 11, 7, rng)
        else:
            clear_disc(canvas, x, y + 2, 17, 9, rng)
    return canvas


def build_background():
    """The first level's battlefield, kept for the existing tests."""
    return build_level_background(campaign.LEVELS[0])

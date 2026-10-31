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


def _threshold(source, size=480):
    """AI art to a strict two-state bitmap, keeping its mid tones.

    A hard threshold throws away the engraving's grey hatching. Sharpening
    first and then ordered-dithering keeps every pen line crisp while the
    forest and cliff shading survive as fine dot patterns.
    """
    grey = Image.open(source).convert("L").resize((size, size), Image.BOX)
    grey = grey.filter(ImageFilter.UnsharpMask(radius=1.2, percent=90,
                                               threshold=2))
    pixels = grey.load()
    canvas = Canvas(size, size)
    for y in range(size):
        row = BAYER4[y % 4]
        for x in range(size):
            tone = pixels[x, y] / 255.0
            # Paper stays paper and ink stays ink; only the middle dithers.
            tone = min(1.0, max(0.0, (tone - 0.14) / 0.74))
            threshold = (row[x % 4] + 0.5) / 16.0
            canvas.set(x, y, WHITE if tone > threshold else BLACK)
    return canvas


# The castle plate: its own origin relative to the goal, and where the view
# hangs the moving banner and brazier fire on it. The view keeps a copy of
# these offsets; they have to agree.
CASTLE_W = 118
CASTLE_H = 106
CASTLE_GATE_X = 92        # gate centre, local x; sits on the goal point
CASTLE_BASE_Y = 100       # ground line, local y
CASTLE_ORIGIN = (geom.GOAL[0] - CASTLE_GATE_X, geom.GOAL[1] + 16 - CASTLE_BASE_Y)
CASTLE_POLE_TOP = (CASTLE_GATE_X + 1, 12)
CASTLE_BRAZIERS = ((66, CASTLE_BASE_Y - 8), (112, CASTLE_BASE_Y - 6))


def _crenels(tc, x0, x1, top, depth, albedo=0.95):
    """A battlement: merlons with snow on their heads and dark embrasures."""
    x = x0
    while x + 4 <= x1:
        S.box(tc, x, top - 5, 4, 5, depth, albedo, top_albedo=1.35)
        x += 7


def _round_tower(tc, cx, top, base, r, roof, seed):
    """A drum tower with coursed stone, a slit window and a slate cone."""
    S.cylinder(tc, cx, top, base, r, 0.95,
               tex=S.masonry(r, course=6, blocks=7, seed=seed))
    S.cylinder(tc, cx, top - 4, top + 1, r + 2, 0.9,
               tex=S.masonry(r + 2, course=5, blocks=9, seed=seed + 1))
    for u in (-0.9, -0.45, 0.0, 0.45, 0.9):
        bx = cx + u * (r + 1)
        S.capsule(tc, bx, top + 1, bx - u, top + 4, 1.4, 0.55)
    S.cone(tc, cx, top - roof, top - 3, r + 4, 0.78,
           tex=S.shingles(rows=6, depth=0.4))
    S.capsule(tc, cx, top - roof - 6, cx, top - roof + 1, 1.2, 0.6)
    S.glint(tc, int(cx), int(top - roof - 7), 0)
    for wy in range(int(top) + 8, int(base) - 10, 14):
        _gate_dark(tc, cx - r * 0.35, wy, 2, 6)


def _gate_dark(tc, x, y, w, h, tone=0.03):
    for yy in range(int(y), int(y + h)):
        for xx in range(int(x), int(x + w)):
            tc.put(xx, yy, tone)
    # A lit sill under the opening.
    for xx in range(int(x) - 1, int(x + w) + 1):
        tc.put(xx, int(y + h), 1.0)


def _castle_plate():
    """The hold, modelled as lit stone like the towers the player builds."""
    tc = S.ToneCanvas(CASTLE_W, CASTLE_H)
    base = CASTLE_BASE_Y
    gate = CASTLE_GATE_X
    S.shadow_ellipse(tc, 70, base + 2, 52, 5, 0.55)

    # Left keep: the tallest mass, a round tower with a slate spire.
    _round_tower(tc, 22, 40, base, 15, 26, seed=31)

    # Curtain wall with a wall-walk and battlements.
    wall = S.flat_masonry(course=6, block=9, seed=5)
    # Set back between the towers, so it sits a step darker than they do.
    S.box(tc, 36, 58, 42, base - 58, 6, 0.72, tex=wall, top_albedo=1.2)
    _crenels(tc, 37, 78, 58, 3, albedo=0.8)
    # A buttress on the curtain, so the long wall has a rhythm.
    S.box(tc, 50, 70, 6, base - 70, 4, 0.85, tex=wall, top_albedo=1.25)
    _gate_dark(tc, 62, 68, 3, 7)

    # Gatehouse: a square tower astride the road.
    S.box(tc, gate - 15, 44, 30, base - 44, 8, 0.95,
          tex=S.flat_masonry(course=6, block=8, seed=9), top_albedo=1.2)
    _crenels(tc, gate - 15, gate + 15, 44, 3)
    _gate_dark(tc, gate - 9, 54, 3, 8)
    _gate_dark(tc, gate + 6, 54, 3, 8)

    # The gate arch: voussoirs round a dark mouth, the portcullis half up.
    arch_top = base - 26
    for y in range(arch_top - 3, base):
        for x in range(gate - 10, gate + 11):
            dx = (x + 0.5 - gate) / 10.0
            dy = (y + 0.5 - (arch_top + 7)) / 10.0
            inside_outer = dy > 0 or dx * dx + dy * dy <= 1.0
            if not inside_outer:
                continue
            dx2 = (x + 0.5 - gate) / 7.0
            dy2 = (y + 0.5 - (arch_top + 7)) / 7.0
            if dy2 > 0 and abs(dx2) <= 1.0 or dx2 * dx2 + dy2 * dy2 <= 1.0:
                # The opening: dark, with the iron grille hanging in it.
                bar = (x - gate) % 3 == 0 and y < arch_top + 14
                rail = y in (arch_top + 6, arch_top + 11)
                tc.put(x, y, 0.55 if (bar or rail) else 0.02)
            else:
                # Wedge stones round the arch, each with its own tone.
                angle = math.atan2(dy, dx)
                stone = int((angle + math.pi) / math.pi * 9)
                tc.put(x, y, 0.9 - 0.25 * (stone % 2) - (0.4 if dx > 0.3 else 0.0))

    # Right flanking tower, slimmer, kept clear of the glass corner.
    _round_tower(tc, 108, 58, base, 8, 16, seed=17)

    # Brazier stands either side of the approach. Fire is drawn by the view.
    for bx, by in CASTLE_BRAZIERS:
        S.capsule(tc, bx, by, bx, by + 8, 1.8, 0.6)
        S.cylinder(tc, bx, by - 3, by, 4.5, 0.75, cap=0.15, spec=0.4)

    # The banner pole on the gatehouse roof. The cloth moves; the view adds it.
    S.capsule(tc, CASTLE_POLE_TOP[0], 44, CASTLE_POLE_TOP[0],
              CASTLE_POLE_TOP[1], 1.6, 0.7, spec=0.4)
    S.ellipsoid(tc, CASTLE_POLE_TOP[0], CASTLE_POLE_TOP[1] - 1, 2, 2, 1.0,
                spec=0.9)

    # Snow drifted against the foot of every wall.
    S.occlude(tc, base, 10, 0.35)
    for x0, x1 in ((8, 36), (36, 78), (gate + 11, 118)):
        for x in range(x0, x1):
            lift = int(1.5 + 1.5 * math.sin(x * 0.7))
            for dy in range(lift):
                if tc.get(x, base - dy) is not None:
                    tc.put(x, base - dy, 1.0 - dy * 0.05)
    return tc


def draw_gatehouse(canvas):
    """Bake the shaded castle into the map at the end of the road."""
    ox, oy = CASTLE_ORIGIN
    plate = _castle_plate().to_canvas(origin=(ox, oy))
    canvas.blit(plate, ox, oy)


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

    _clear_sites(canvas, level, rng)

    # The gatehouse sits at the end of the road.
    gx, gy = geom.GOAL
    clear_disc(canvas, gx - 8, gy - 6, 58, 16, rng)
    draw_gatehouse(canvas)
    return canvas


def build_campaign_background():
    """The campaign map: the journey plate with a clearing at every node.

    The trail and the medallions are drawn by the view, so a locked stage
    can look different from a cleared one.
    """
    rng = random.Random(0xCA4)
    canvas = _threshold(AI_DIR / campaign.CAMPAIGN_PLATE)
    # Only the stages that exist get a clearing; the rest of the journey
    # stays drawn until its level is added.
    for x, y in campaign.CAMPAIGN_NODES[:len(campaign.LEVELS)]:
        clear_disc(canvas, x, y + 2, 17, 9, rng)
    return canvas


def build_background():
    """The first level's battlefield, kept for the existing tests."""
    return build_level_background(campaign.LEVELS[0])

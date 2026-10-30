"""Snowfield background generator.

The reference look is built from hard-edged regions, not soft gradients:
  1. clean white snow base
  2. irregular tilled patches, ordered-dither filled with crisp boundaries
  3. snow tracks that scour through the patches
  4. the eastern cliff as a dithered wall with a hard black lip
  5. conifer stands, crags, bushes, stones, stumps and fence posts
  6. the two roads with dark shoulders
  7. the gatehouse at the goal

Everything is strictly black and white; tone comes only from Bayer dithering.
"""
import random

from artgen.canvas import (BLACK, WHITE, Canvas, dither_polygon, dither_rect)
from artgen import level_geom as geom

ROAD_WIDTH_A = 15
ROAD_WIDTH_B = 11
TREE_MIN_ROAD_CLEARANCE = 15
TREE_MIN_SPOT_CLEARANCE = 14

# Tilled snow patches: angular polygons with hard boundaries.
# Kept few on purpose: the battlefield must stay readable under the units.
# (vertices, density, dither offset, furrow lines)
PATCHES = [
    ([(24, 52), (104, 48), (118, 76), (96, 104), (24, 94)],
     0.26, (0, 0), [((30, 66), (108, 60)), ((32, 82), (104, 76))]),
    ([(352, 56), (424, 52), (452, 82), (440, 124), (352, 128)],
     0.28, (1, 7), [((358, 76), (442, 72)), ((358, 98), (446, 94))]),
    ([(18, 234), (64, 228), (74, 262), (52, 296), (18, 286)],
     0.24, (4, 1), [((24, 250), (66, 246))]),
]

# Tracks that cut through the tilled patches.
TRACKS = [
    (34, 60, 96, 106, 12),
    (160, 58, 240, 96, 10),
    (272, 176, 344, 210, 11),
    (398, 176, 438, 218, 9),
]

# Cliff wall: a dithered band on the eastern edge with a hard black lip.
CLIFF_POLY = [(424, 44), (480, 44), (480, 268), (438, 258), (416, 186),
              (424, 118)]
CLIFF_LIP = [(424, 44), (416, 118), (424, 186), (414, 252), (438, 258)]


def _draw_conifer(canvas, x, y, height, layers=4):
    """A snow-laden spruce: black silhouette with white snow shelves."""
    trunk = max(2, height // 11)
    canvas.fill_rect(x - trunk // 2, y - trunk, trunk, trunk, BLACK)
    for index in range(layers):
        t = index / (layers - 1.0) if layers > 1 else 0.0
        half = max(2, int(height * (0.45 - 0.31 * t)))
        top = y - height + int(height * 0.26 * index)
        bottom = top + int(height * 0.36)
        canvas.triangle(x, top, x - half, bottom, x + half, bottom, BLACK)
        shelf = max(1, int(half * 0.55))
        shelf_top = bottom - max(2, int(height * 0.17))
        canvas.triangle(x, shelf_top, x - shelf, bottom, x + shelf, bottom, WHITE)
    canvas.fill_rect(x - 1, y - height, 2, max(1, height // 8), BLACK)


def _draw_bush(canvas, x, y, size):
    """Low shrub: overlapping black lobes with a snow highlight."""
    canvas.disc(x, y, size, BLACK)
    canvas.disc(x - size // 2, y, size // 2, BLACK)
    canvas.disc(x + size // 2, y - 1, size // 2, BLACK)
    canvas.disc(x - size // 3, y - size // 3, max(1, size // 4), WHITE)


def _draw_stump(canvas, x, y, size):
    canvas.fill_rect(x - size, y - size, size * 2, size, BLACK)
    canvas.fill_rect(x - size + 2, y - size + 1, size * 2 - 4, 2, WHITE)


def _draw_rock(canvas, x, y, size):
    """Solid boulder with a small snow highlight, never a hollow ring."""
    canvas.disc(x, y, size, BLACK)
    canvas.disc(x - size // 3, y - size // 3, max(1, size // 4), WHITE)


def _draw_stone(canvas, x, y):
    canvas.fill_rect(x, y, 2, 2, BLACK)


def _draw_snowdrift(canvas, x, y, width, height):
    """Wind drift: a white mound shadowed only along its lower edge."""
    canvas.ellipse(x, y, width, height, WHITE)
    for offset in range(-width + 2, width - 2, 2):
        lift = int(height * (1.0 - (offset / float(width)) ** 2) ** 0.5)
        if lift <= 0:
            continue
        canvas.set(x + offset, y + lift, BLACK)
        canvas.set(x + offset + 1, y + lift, BLACK)


def _draw_fence(canvas, points):
    """Short post-and-rail fence, used to edge the road near the gate."""
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        canvas.thick_line(x0, y0, x1, y1, 2, BLACK)
    for px, py in points:
        canvas.fill_rect(px - 1, py - 7, 2, 8, BLACK)


def _draw_road(canvas, points, width):
    """Snow road: white body with a thin dark shoulder so it reads on the field."""
    canvas.polyline(points, width + 6, BLACK)
    canvas.polyline(points, width + 2, WHITE)
    canvas.polyline(points, width, WHITE)


def _draw_crags(canvas):
    """A broken rock outcrop along the northern wall."""
    base = 62
    for cx, half, height in ((194, 18, 25), (206, 9, 13), (222, 14, 19),
                              (238, 8, 11), (252, 15, 22), (270, 10, 14)):
        canvas.triangle(cx, base - height, cx - half, base + 2, cx + half, base + 2,
                        BLACK)
        canvas.triangle(cx, base - height + 6, cx - half // 2, base - 2,
                        cx + half // 2, base - 2, WHITE)


def _draw_cairn(canvas, x, y, height):
    """Waymarker cairn: stacked stones narrowing toward the top."""
    stones = max(3, height // 7)
    for index in range(stones):
        t = index / float(max(1, stones - 1))
        half = max(2, int(5 - 3 * t))
        top = y - int(height * (index + 1) / stones)
        bottom = y - int(height * index / stones)
        canvas.ellipse(x, (top + bottom) // 2, half, max(1, (bottom - top) // 2), BLACK)
        canvas.ellipse(x - 1, (top + bottom) // 2 - 1, max(1, half - 2),
                       max(1, (bottom - top) // 2 - 1), WHITE)


def _draw_bridge(canvas, x, y, span):
    """Stone bridge carrying the road across a crevasse."""
    canvas.fill_rect(x - span, y - 9, span * 2, 5, BLACK)
    canvas.fill_rect(x - span, y - 4, span * 2, 4, WHITE)
    for pier in (-span // 2, span // 2):
        canvas.fill_rect(x + pier - 3, y, 6, 13, BLACK)
        canvas.fill_rect(x + pier - 2, y + 1, 4, 12, WHITE)
    for offset in range(-span, span, 8):
        canvas.fill_rect(x + offset, y - 3, 1, 3, BLACK)
    canvas.thick_line(x - span - 4, y + 13, x - span, y + 2, 2, BLACK)
    canvas.thick_line(x + span + 4, y + 13, x + span, y + 2, 2, BLACK)


def _draw_watchtower(canvas, x, y):
    """Ruined stone watchtower on the northern ridge."""
    canvas.fill_rect(x - 13, y - 44, 26, 44, BLACK)
    canvas.fill_rect(x - 10, y - 41, 20, 41, WHITE)
    for row in range(y - 40, y - 2, 8):
        canvas.fill_rect(x - 10, row, 20, 1, BLACK)
    canvas.fill_rect(x - 6, y - 22, 12, 22, BLACK)
    for offset in range(-13, 13, 6):
        canvas.fill_rect(x + offset, y - 50, 4, 7, BLACK)
    canvas.triangle(x - 16, y - 48, x + 16, y - 48, x, y - 62, BLACK)
    canvas.triangle(x - 11, y - 49, x + 11, y - 49, x, y - 58, WHITE)


def _draw_tent(canvas, x, y):
    """Campaign tent: a small triangle with a dark doorway."""
    canvas.triangle(x, y - 20, x - 12, y, x + 12, y, BLACK)
    canvas.triangle(x, y - 16, x - 8, y, x + 8, y, WHITE)
    canvas.fill_rect(x - 3, y - 9, 6, 9, BLACK)


def _draw_well(canvas, x, y):
    """Frozen well with a little roof."""
    canvas.ellipse(x, y, 9, 6, BLACK)
    canvas.ellipse(x, y - 1, 6, 4, WHITE)
    canvas.fill_rect(x - 8, y - 18, 3, 18, BLACK)
    canvas.fill_rect(x + 5, y - 18, 3, 18, BLACK)
    canvas.triangle(x - 12, y - 18, x + 12, y - 18, x, y - 27, BLACK)
    canvas.triangle(x - 8, y - 18, x + 8, y - 18, x, y - 24, WHITE)


def _draw_ridge(canvas):
    """Mountain ridge in line art: crest outline plus hatched shadow faces.

    The HUD covers the top 44px, so the ridge sits low enough for its peaks
    to stay visible. Peaks are drawn as white snow faces, not solid black.
    """
    base = 92
    ridge = [(0, 0), (26, 0), (44, 26), (62, 0), (96, 0), (116, 40),
             (138, 0), (176, 0), (196, 24), (214, 0), (250, 0), (268, 36),
             (290, 0), (330, 0), (346, 22), (362, 0), (400, 0), (420, 30),
             (440, 0), (480, 0)]
    crest = [(x, base - height) for x, height in ridge]
    canvas.polyline(crest, 3, BLACK)

    for index in range(1, len(ridge) - 1, 2):
        cx, height = ridge[index]
        if height <= 0:
            continue
        right_x = ridge[index + 1][0]
        steps = max(3, (right_x - cx) // 12)
        for step in range(1, steps):
            t = step / float(steps)
            sx = cx + (right_x - cx) * t
            sy = base - height + height * t
            canvas.line(sx, sy, sx + 7, sy + 7, BLACK)
            canvas.line(sx + 1, sy + 5, sx + 5, sy + 9, BLACK)


def _draw_cliff(canvas):
    dither_polygon(canvas, CLIFF_POLY, 0.42, offset=(4, 4))
    canvas.polyline(CLIFF_LIP, 3, BLACK)
    for cx, cy in ((438, 92), (452, 140), (436, 196)):
        canvas.line(cx - 6, cy, cx + 4, cy + 12, BLACK)
        canvas.line(cx + 4, cy + 12, cx - 2, cy + 20, BLACK)
        canvas.line(cx - 2, cy + 20, cx + 2, cy + 26, BLACK)


def _draw_gatehouse(canvas):
    """Stone gate at the goal: two towers, an arch mouth and a crenellated top."""
    gx, gy = geom.GOAL
    left, right = gx - 28, gx + 12
    base = gy + 14
    canvas.fill_rect(left, base - 34, 13, 34, BLACK)
    canvas.fill_rect(right - 13, base - 34, 13, 34, BLACK)
    canvas.fill_rect(gx - 7, base - 26, 14, 26, BLACK)
    canvas.fill_rect(left, base - 41, right - left, 8, BLACK)
    for row in range(base - 30, base, 7):
        canvas.fill_rect(left + 2, row, 9, 2, WHITE)
        canvas.fill_rect(right - 11, row, 9, 2, WHITE)
    for x in range(left + 1, right - 2, 6):
        canvas.fill_rect(x, base - 45, 3, 5, BLACK)
    canvas.fill_rect(gx - 5, base - 24, 10, 24, WHITE)
    canvas.fill_rect(gx - 4, base - 23, 8, 23, BLACK)


def _scatter_decor(canvas, rng):
    for x0, y0, x1, y1 in geom.FOREST_ZONES:
        area = (x1 - x0) * (y1 - y0)
        for _ in range(max(1, area // 900)):
            fx = rng.randint(x0, x1)
            fy = rng.randint(y0, y1)
            if geom.distance_to_paths(fx, fy) < TREE_MIN_ROAD_CLEARANCE:
                continue
            if geom.distance_to_spots(fx, fy) < TREE_MIN_SPOT_CLEARANCE:
                continue
            height = rng.randint(20, 40)
            layers = rng.choice((3, 4, 4, 5))
            _draw_conifer(canvas, fx, fy, height, layers)

    placed, attempts = 0, 0
    while placed < 18 and attempts < 500:
        attempts += 1
        rx = rng.randint(8, 470)
        ry = rng.randint(geom.PLAY_TOP + 6, 405)
        if geom.distance_to_paths(rx, ry) < 13:
            continue
        if geom.distance_to_spots(rx, ry) < 12:
            continue
        _draw_rock(canvas, rx, ry, rng.randint(4, 8))
        placed += 1

    bushes, attempts = 0, 0
    while bushes < 14 and attempts < 500:
        attempts += 1
        bx = rng.randint(10, 468)
        by = rng.randint(geom.PLAY_TOP + 8, 404)
        if geom.distance_to_paths(bx, by) < 16:
            continue
        if geom.distance_to_spots(bx, by) < 14:
            continue
        _draw_bush(canvas, bx, by, rng.randint(3, 5))
        bushes += 1

    for _ in range(6):
        sx = rng.randint(24, 452)
        sy = rng.randint(geom.PLAY_TOP + 12, 400)
        if geom.distance_to_paths(sx, sy) < 18:
            continue
        _draw_stump(canvas, sx, sy, rng.randint(2, 4))

    for _ in range(40):
        sx = rng.randint(6, 472)
        sy = rng.randint(geom.PLAY_TOP + 4, 408)
        if geom.distance_to_paths(sx, sy) < 11:
            continue
        _draw_stone(canvas, sx, sy)

    for _ in range(6):
        dx = rng.randint(20, 460)
        dy = rng.randint(geom.PLAY_TOP + 10, 400)
        if geom.distance_to_paths(dx, dy) < 20:
            continue
        _draw_snowdrift(canvas, dx, dy, rng.randint(10, 16), rng.randint(4, 7))

    for _ in range(10):
        cx = rng.randint(20, 456)
        cy = rng.randint(geom.PLAY_TOP + 10, 400)
        if geom.distance_to_paths(cx, cy) < 15:
            continue
        x, y = cx, cy
        for _ in range(rng.randint(2, 4)):
            nx = x + rng.randint(-9, 9)
            ny = y + rng.randint(-7, 7)
            canvas.line(x, y, nx, ny, BLACK)
            x, y = nx, ny


def build_background():
    canvas = Canvas(geom.VIEW_WIDTH, geom.VIEW_HEIGHT)
    canvas.fill_rect(0, 0, geom.VIEW_WIDTH, geom.VIEW_HEIGHT, WHITE)

    # Tilled patches: hard-edged regions of ordered dither.
    for vertices, density, offset, furrows in PATCHES:
        dither_polygon(canvas, vertices, density, offset=offset)
        for (x0, y0), (x1, y1) in furrows:
            canvas.thick_line(x0, y0, x1, y1, 2, WHITE)

    # Tracks scour white through the patches.
    for x0, y0, x1, y1, width in TRACKS:
        canvas.thick_line(x0, y0, x1, y1, width, WHITE)

    _draw_ridge(canvas)
    _draw_cliff(canvas)
    _draw_watchtower(canvas, 132, 96)
    _draw_well(canvas, 84, 330)
    _draw_tent(canvas, 96, 128)
    _draw_tent(canvas, 348, 372)
    _scatter_decor(canvas, rng=random.Random(0x5EED))

    _draw_road(canvas, geom.PATH_LANE_A, ROAD_WIDTH_A)
    _draw_road(canvas, geom.PATH_LANE_B, ROAD_WIDTH_B)
    _draw_bridge(canvas, 150, 214, 26)
    for cx, cy in ((18, 122), (96, 150), (204, 268), (264, 320), (348, 366)):
        _draw_cairn(canvas, cx + 16, cy + 12, 16)
    _draw_fence(canvas, [(356, 344), (372, 340), (388, 336)])
    _draw_gatehouse(canvas)
    return canvas

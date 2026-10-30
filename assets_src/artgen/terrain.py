"""Snowfield background generator.

Built the way the reference art is built: hard-edged regions of worked
ground, contour lines for slopes, hatched rock faces and a ridge of hatched
mountains. Nothing is a soft blob of dots.

Layer order (back to front):
  1. clean white snow base
  2. tilled plots with ploughed rows
  3. contour-lined slopes
  4. the eastern cliff, a hatched rock face
  5. the northern ridge, hatched shadow faces
  6. conifer stands
  7. scattered boulders, bushes, stumps and drifts
  8. the two roads
  9. buildings, cairns, fences and the gatehouse
"""
import math
import random

from artgen.canvas import (BLACK, WHITE, Canvas, fill_polygon, hatch_polygon,
                           outline_polygon, point_in_polygon)
from artgen import level_geom as geom
from artgen import objects as O

ROAD_WIDTH_A = 15
ROAD_WIDTH_B = 11

# Tilled plots: (vertices, row spacing, arc amount).
PLOTS = [
    ([(18, 52), (108, 46), (126, 74), (104, 104), (36, 100), (16, 76)], 6, 0.16),
    ([(196, 50), (268, 46), (286, 72), (262, 100), (206, 96)], 6, -0.14),
    ([(16, 236), (64, 228), (78, 262), (52, 300), (14, 288)], 5, 0.18),
    ([(330, 330), (398, 322), (414, 352), (388, 382), (330, 378)], 6, -0.15),
    ([(120, 296), (190, 288), (206, 316), (176, 346), (122, 340)], 6, 0.12),
]

# Rounded slopes drawn as contour rings: (cx, cy, rx, ry, rings).
HILLS = [
    (250, 352, 62, 40, 6),
    (386, 176, 52, 34, 5),
    (86, 330, 48, 32, 5),
    (300, 210, 40, 26, 4),
]

CLIFF_POLY = [(424, 44), (480, 44), (480, 272), (438, 262), (414, 190),
              (426, 116)]

RIDGE = [(0, 0), (26, 0), (46, 28), (64, 0), (98, 0), (118, 42), (140, 0),
         (178, 0), (198, 26), (216, 0), (252, 0), (270, 38), (292, 0),
         (332, 0), (348, 24), (364, 0), (402, 0), (422, 32), (442, 0),
         (480, 0)]

# Conifer stands: (x0, y0, x1, y1, density).
STANDS = [
    (6, 186, 126, 410, 1 / 520),
    (140, 50, 268, 118, 1 / 620),
    (242, 330, 438, 404, 1 / 560),
    (352, 50, 476, 126, 1 / 640),
    (186, 176, 236, 214, 1 / 700),
]


# ----------------------------------------------------------------------
# Landforms
# ----------------------------------------------------------------------

def _draw_plots(canvas):
    """Tilled ground: ploughed rows inside hard-edged plots."""
    for vertices, spacing, curve in PLOTS:
        fill_polygon(canvas, vertices, WHITE)
        O.furrow_field(canvas, vertices, spacing, BLACK, curve=curve)
        outline_polygon(canvas, vertices, BLACK, 1)


def _draw_hills(canvas):
    """Slopes described by contour rings."""
    for cx, cy, rx, ry, rings in HILLS:
        O.contour_hill(canvas, cx, cy, rx, ry, rings, BLACK)


def _draw_cliff(canvas):
    """Eastern cliff: rock face with striations, cracks and a heavy lip."""
    fill_polygon(canvas, CLIFF_POLY, WHITE)
    hatch_polygon(canvas, CLIFF_POLY, 74.0, 9.0, BLACK, offset=2.0)

    # Horizontal strata across the rock face.
    strata = random.Random(0x57A7A)
    for _ in range(9):
        sy = strata.randint(50, 250)
        sx = strata.randint(424, 452)
        canvas.line(sx, sy, strata.randint(458, 478),
                    sy + strata.randint(-4, 4), BLACK)

    # Vertical rock striations.
    rng = random.Random(0xC11FF)
    for _ in range(18):
        sx = rng.randint(422, 476)
        sy = rng.randint(48, 258)
        length = rng.randint(5, 16)
        canvas.line(sx, sy, sx + rng.randint(-2, 2), sy + length, BLACK)

    # Fractures across the face.
    for cx, cy in ((438, 88), (456, 140), (432, 196), (462, 230)):
        canvas.line(cx - 6, cy, cx + 3, cy + 12, BLACK)
        canvas.line(cx + 3, cy + 12, cx - 2, cy + 21, BLACK)
        canvas.line(cx + 3, cy + 12, cx + 9, cy + 20, BLACK)

    outline_polygon(canvas, CLIFF_POLY, BLACK, 2)


def _draw_ridge(canvas):
    """Northern ridge: outline plus hatched shadow faces."""
    base = 94
    crest = [(x, base - height) for x, height in RIDGE]
    for index in range(1, len(RIDGE) - 1, 2):
        cx, height = RIDGE[index]
        if height <= 0:
            continue
        right_x = RIDGE[index + 1][0]
        steps = max(3, (right_x - cx) // 10)
        for step in range(1, steps):
            t = step / float(steps)
            sx = cx + (right_x - cx) * t
            sy = base - height + height * t
            canvas.line(sx, sy, sx + 6, sy + 6, BLACK)
            canvas.line(sx + 1, sy + 4, sx + 4, sy + 7, BLACK)
    for index in range(len(crest) - 1):
        x0, y0 = crest[index]
        x1, y1 = crest[index + 1]
        canvas.thick_line(x0, y0, x1, y1, 3, BLACK)
    # A bright snow shoulder just under the crest.
    for index in range(len(crest) - 1):
        x0, y0 = crest[index]
        x1, y1 = crest[index + 1]
        canvas.thick_line(x0, y0 + 5, x1, y1 + 5, 2, WHITE)


# ----------------------------------------------------------------------
# Scatter
# ----------------------------------------------------------------------

def _draw_stands(canvas, rng):
    """Dense conifer stands with soft, overlapping edges."""
    for x0, y0, x1, y1, density in STANDS:
        area = (x1 - x0) * (y1 - y0)
        for _ in range(max(1, int(area * density))):
            fx = rng.randint(x0 - 6, x1 + 6)
            fy = rng.randint(y0 - 4, y1 + 4)
            if geom.distance_to_paths(fx, fy) < 16:
                continue
            if geom.distance_to_spots(fx, fy) < 15:
                continue
            O.conifer(canvas, fx, fy, rng.randint(22, 40),
                      rng.randint(0, 9999), tiers=rng.choice((4, 5, 5, 6)))


def _draw_scatter(canvas, rng):
    placed, attempts = 0, 0
    while placed < 22 and attempts < 600:
        attempts += 1
        rx = rng.randint(8, 470)
        ry = rng.randint(geom.PLAY_TOP + 6, 405)
        if geom.distance_to_paths(rx, ry) < 13:
            continue
        if geom.distance_to_spots(rx, ry) < 12:
            continue
        O.boulder(canvas, rx, ry, rng.randint(5, 13), rng.randint(0, 9999))
        placed += 1

    bushes, attempts = 0, 0
    while bushes < 16 and attempts < 600:
        attempts += 1
        bx = rng.randint(10, 468)
        by = rng.randint(geom.PLAY_TOP + 8, 404)
        if geom.distance_to_paths(bx, by) < 15:
            continue
        if geom.distance_to_spots(bx, by) < 13:
            continue
        O.bush(canvas, bx, by, rng.randint(4, 6), rng.randint(0, 9999))
        bushes += 1

    for _ in range(6):
        sx = rng.randint(24, 452)
        sy = rng.randint(geom.PLAY_TOP + 12, 400)
        if geom.distance_to_paths(sx, sy) < 17:
            continue
        O.stump(canvas, sx, sy, rng.randint(3, 5))

    for _ in range(7):
        dx = rng.randint(20, 460)
        dy = rng.randint(geom.PLAY_TOP + 10, 400)
        if geom.distance_to_paths(dx, dy) < 19:
            continue
        O.snowdrift(canvas, dx, dy, rng.randint(11, 17), rng.randint(5, 8))

    for _ in range(58):
        sx = rng.randint(6, 472)
        sy = rng.randint(geom.PLAY_TOP + 4, 408)
        if geom.distance_to_paths(sx, sy) < 11:
            continue
        canvas.fill_rect(sx, sy, 2, 2, BLACK)

    # Grass tufts and pebbles keep the open snow from reading as empty paper.
    for _ in range(150):
        sx = rng.randint(4, 476)
        sy = rng.randint(geom.PLAY_TOP + 2, 410)
        if geom.distance_to_paths(sx, sy) < 13:
            continue
        if rng.random() < 0.45:
            canvas.fill_rect(sx, sy, 2, 2, BLACK)
        else:
            canvas.line(sx, sy, sx, sy - 3, BLACK)
            canvas.line(sx, sy, sx + 1, sy - 4, BLACK)


# ----------------------------------------------------------------------
# Road
# ----------------------------------------------------------------------

def _draw_road(canvas, points, width, rng):
    """A trodden snow road: dark shoulder, packed body, two cart ruts."""
    canvas.polyline(points, width + 5, BLACK)
    canvas.polyline(points, width + 2, WHITE)
    canvas.polyline(points, width, WHITE)

    # Two continuous ruts, slightly off centre.
    for side in (-1, 1):
        offset = width * 0.22 * side
        for index in range(len(points) - 1):
            x0, y0 = points[index]
            x1, y1 = points[index + 1]
            length = ((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5
            if length <= 0:
                continue
            dx = (x1 - x0) / length
            dy = (y1 - y0) / length
            nx, ny = -dy, dx
            canvas.line(x0 + nx * offset, y0 + ny * offset,
                        x1 + nx * offset, y1 + ny * offset, BLACK)

    # Sparse snow lumps breaking the shoulder line.
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        length = int(((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5)
        if length <= 0:
            continue
        dx = (x1 - x0) / float(length)
        dy = (y1 - y0) / float(length)
        nx, ny = -dy, dx
        for step in range(6, length - 6, 17):
            px = x0 + dx * step
            py = y0 + dy * step
            side = 1 if rng.random() < 0.5 else -1
            canvas.disc(px + nx * side * (width / 2 + 3),
                        py + ny * side * (width / 2 + 3), 2, BLACK)


# ----------------------------------------------------------------------
# Buildings
# ----------------------------------------------------------------------

def _draw_gatehouse(canvas):
    """The goal: a walled gate with two towers."""
    gx, gy = geom.GOAL
    left, right = gx - 30, gx + 14
    base = gy + 16

    # Curtain wall.
    O._stone_wall(canvas, left + 11, base - 26, right - left - 22, 26, course=6)
    # Towers.
    for tx in (left, right - 13):
        O._stone_wall(canvas, tx, base - 40, 13, 40, course=5)
        for offset in range(0, 13, 5):
            canvas.fill_rect(tx + offset, base - 46, 3, 6, BLACK)
    # Arch mouth.
    canvas.fill_rect(gx - 7, base - 26, 14, 26, BLACK)
    canvas.fill_rect(gx - 5, base - 24, 10, 24, WHITE)
    canvas.ellipse(gx, base - 24, 7, 5, WHITE)
    # Banner.
    canvas.fill_rect(gx + 1, base - 46, 2, 16, BLACK)
    canvas.fill_rect(gx + 3, base - 45, 9, 6, BLACK)


def build_background():
    canvas = Canvas(geom.VIEW_WIDTH, geom.VIEW_HEIGHT)
    canvas.fill_rect(0, 0, geom.VIEW_WIDTH, geom.VIEW_HEIGHT, WHITE)

    _draw_plots(canvas)
    _draw_hills(canvas)
    _draw_cliff(canvas)
    _draw_ridge(canvas)

    rng = random.Random(0x5EED)
    _draw_stands(canvas, rng)
    _draw_scatter(canvas, rng)

    _draw_road(canvas, geom.PATH_LANE_A, ROAD_WIDTH_A, rng)
    _draw_road(canvas, geom.PATH_LANE_B, ROAD_WIDTH_B, rng)

    # Structures along the route.
    O.watchtower(canvas, 132, 98, 52)
    O.hut(canvas, 92, 322, 24, 16)
    O.hut(canvas, 420, 120, 20, 14)
    O.well(canvas, 74, 196)
    O.tented_camp(canvas, 404, 336)
    O.cairn(canvas, 36, 122, 16)
    O.cairn(canvas, 244, 242, 15)
    O.cairn(canvas, 366, 344, 15)
    O.bridge(canvas, 150, 214, 26)
    O.fence(canvas, [(300, 380), (316, 376), (332, 372)])
    O.fence(canvas, [(66, 262), (80, 268), (94, 274)])
    _draw_gatehouse(canvas)
    return canvas

"""Detailed 1-bit object library.

Every object here follows the same rules, matching the reference art:

  * the silhouette is a closed black shape with a 2px contour
  * interior detail is carried by hatching, facet lines and material breaks
  * no feature thinner than 2px, because a pixel is 0.081mm on the panel
  * shading uses one hatch family per material, never a grey value
"""
import math
import random

from artgen.canvas import (BLACK, WHITE, Canvas, _bounds, blob_points,
                           fill_polygon, hatch_polygon, outline_polygon,
                           point_in_polygon)


def _rng(seed):
    return random.Random(seed)


# ----------------------------------------------------------------------
# Vegetation
# ----------------------------------------------------------------------


def conifer(canvas, x, y, height, seed=0, tiers=5):
    """A snow-laden spruce.

    The silhouette is a jagged cone; white scalloped lines carve the bough
    tiers into it and a lit edge runs down the left side. That combination
    is what reads as a drawn conifer rather than a black triangle.
    """
    rng = _rng(seed)
    top = y - height
    half = max(6, int(height * 0.34))

    # Jagged cone silhouette: a saw edge on both flanks.
    left, right = [], []
    steps = max(4, tiers * 2)
    for index in range(steps):
        f = (index + 1) / float(steps)
        yy = top + height * f
        w = half * f
        jitter = rng.uniform(0.0, height * 0.03)
        left.append((x - w, yy - height * 0.045 + jitter))
        left.append((x - w * 0.78, yy))
        right.append((x + w * 0.78, yy))
        right.append((x + w, yy - height * 0.045 + jitter))
    poly = ([(x, top)] + left + [(x - half * 0.45, y), (x + half * 0.45, y)]
            + list(reversed(right)))
    fill_polygon(canvas, poly, BLACK)

    # Bough tiers carved in white.
    for index in range(1, tiers):
        f = index / float(tiers)
        ty = top + int(height * f * 0.90)
        w = int(half * f * 0.92)
        canvas.thick_line(x - w, ty, x, ty + 3, 2, WHITE)
        canvas.thick_line(x, ty + 3, x + w, ty, 2, WHITE)

    # Lit edge down the left flank and a trunk shadow.
    canvas.thick_line(x - 3, top + 5, x - half + 4, y - 3, 2, WHITE)
    canvas.fill_rect(x - 1, y - 4, 3, 4, WHITE)


def dead_tree(canvas, x, y, height, seed=0):
    """A bare trunk with a few broken branches: used at map edges."""
    rng = _rng(seed)
    canvas.fill_rect(x - 1, y - height, 3, height, BLACK)
    for _ in range(rng.randint(2, 4)):
        by = y - rng.randint(height // 4, height - 4)
        direction = rng.choice((-1, 1))
        length = rng.randint(4, 9)
        canvas.line(x, by, x + direction * length, by - length // 2, BLACK)


def bush(canvas, x, y, size, seed=0):
    """Low shrub: three overlapping lobes with a hatched underside."""
    rng = _rng(seed)
    lobes = []
    for index in range(3):
        offset = (index - 1) * size * 0.6
        lobes.append(blob_points(x + offset, y - (index == 1) * size * 0.3,
                                  size * (0.9 if index == 1 else 0.7),
                                  7, 0.3, rng.random() * 6.0))
    for lobe in lobes:
        fill_polygon(canvas, lobe, BLACK)
    for lobe in lobes:
        outline_polygon(canvas, lobe, WHITE, 1)
    canvas.disc(x - size // 3, y - size // 2, max(1, size // 3), WHITE)


def stump(canvas, x, y, size):
    """Cut trunk with visible growth rings."""
    canvas.ellipse(x, y, size, size * 0.6, BLACK)
    canvas.ellipse(x, y - 1, size - 1, size * 0.6 - 1, WHITE)
    canvas.ellipse(x, y - 1, max(1, size // 2), max(1, int(size * 0.3)), BLACK)


# ----------------------------------------------------------------------
# Rock and ice
# ----------------------------------------------------------------------


def boulder(canvas, x, y, size, seed=0):
    """Faceted boulder: solid mass, two facet breaks, optional shadow hatch."""
    rng = _rng(seed)
    sides = rng.randint(6, 8)
    shape = blob_points(x, y, size, sides, 0.24, rng.random() * 6.0, 0.8)
    fill_polygon(canvas, shape, BLACK)

    # Two facet breaks running from the light source.
    lx, ly = x - size * 0.28, y - size * 0.30
    picks = sorted(rng.sample(range(len(shape)), 2))
    for index in picks:
        px, py = shape[index]
        canvas.line(lx, ly, px, py, WHITE)

    # Larger rocks carry a hatched shadow face.
    if size >= 13:
        shadow = [shape[index] for index in picks + [picks[0]]]
        hatch_polygon(canvas, shadow, 45.0, 4.0, WHITE, offset=1.0)


def ice_block(canvas, x, y, size, seed=0):
    """Angular ice shard with a bright edge."""
    rng = _rng(seed)
    shape = blob_points(x, y, size, 5, 0.34, rng.random() * 6.0, 1.5)
    fill_polygon(canvas, shape, BLACK)
    outline_polygon(canvas, shape, WHITE, 1)
    canvas.line(shape[0][0], shape[0][1], shape[2][0], shape[2][1], WHITE)


def snowdrift(canvas, x, y, width, height):
    """Wind drift: a white mound with a hatched lee side."""
    shape = blob_points(x, y, width, 9, 0.16, 0.7, 0.42)
    fill_polygon(canvas, shape, WHITE)
    lee = [(px, py + height) for px, py in shape]
    hatch_polygon(canvas, lee, 60.0, 2.0, BLACK, offset=1.0)
    outline_polygon(canvas, shape, BLACK, 1)


# ----------------------------------------------------------------------
# Structures
# ----------------------------------------------------------------------


def _stone_wall(canvas, x, y, width, height, seed=0, course=5):
    """A wall of individual blocks with staggered joints."""
    rng = _rng(seed)
    fill_polygon(canvas, [(x, y), (x + width, y), (x + width, y + height),
                          (x, y + height)], BLACK)
    row = 0
    offset = 0
    while offset < height:
        row_height = min(course, height - offset)
        cursor = x - (course if row % 2 else 0)
        while cursor < x + width:
            left = max(cursor, x)
            right = min(cursor + course - 1, x + width)
            if right > left:
                canvas.fill_rect(left + 1, y + offset + 1,
                                 right - left - 1, row_height - 1, WHITE)
            cursor += course
        offset += course
        row += 1
    _ = rng


def watchtower(canvas, x, y, height=54):
    """Stone watchtower with a timber upper storey and a shingled roof."""
    base = y
    half = 11
    # Stone shaft.
    _stone_wall(canvas, x - half, base - height + 14, half * 2, height - 14, course=5)
    # Timber storey.
    top = base - height + 14
    canvas.fill_rect(x - half + 1, top - 12, half * 2 - 2, 12, BLACK)
    canvas.fill_rect(x - half + 3, top - 10, half * 2 - 6, 8, WHITE)
    for post in range(-half + 3, half - 3, 6):
        canvas.fill_rect(x + post, top - 10, 2, 8, BLACK)
    # Conical shingle roof.
    canvas.triangle(x, top - 26, x - half - 2, top - 12, x + half + 2, top - 12, BLACK)
    for row in range(top - 22, top - 12, 3):
        canvas.hline(x - half + 1 + (top - 20 - row), row, (half - 1) * 2, WHITE)
    canvas.fill_rect(x - 3, base - 12, 6, 12, BLACK)   # doorway
    canvas.fill_rect(x - 2, base - 11, 4, 11, WHITE)
    canvas.fill_rect(x + 4, base - 22, 4, 5, BLACK)    # window


def hut(canvas, x, y, width=22, height=16):
    """Small gabled dwelling with a door and a shuttered window."""
    wall_top = y - height
    _stone_wall(canvas, x - width // 2, wall_top, width, height, course=4)
    canvas.triangle(x, wall_top - 12, x - width // 2 - 3, wall_top,
                    x + width // 2 + 3, wall_top, BLACK)
    for row in range(wall_top - 9, wall_top, 3):
        canvas.hline(x - width // 2 - 2 + (row - (wall_top - 9)), row,
                     width + 4 - 2 * (row - (wall_top - 9)), WHITE)
    canvas.fill_rect(x - 3, y - 9, 6, 9, BLACK)
    canvas.fill_rect(x - 2, y - 8, 4, 8, WHITE)
    canvas.fill_rect(x + 4, y - 14, 4, 4, BLACK)


def well(canvas, x, y):
    """Frozen well with a little shingled roof."""
    canvas.ellipse(x, y, 10, 6, BLACK)
    canvas.ellipse(x, y - 1, 8, 4, WHITE)
    canvas.ellipse(x, y - 1, 4, 2, BLACK)
    canvas.fill_rect(x - 9, y - 20, 3, 18, BLACK)
    canvas.fill_rect(x + 6, y - 20, 3, 18, BLACK)
    canvas.triangle(x, y - 30, x - 13, y - 19, x + 13, y - 19, BLACK)
    canvas.hline(x - 10, y - 20, 20, WHITE)


def tent(canvas, x, y, width=14, height=20):
    """Campaign tent with a dark doorway."""
    canvas.triangle(x, y - height, x - width, y, x + width, y, BLACK)
    canvas.triangle(x, y - height + 3, x - width + 3, y, x + width - 3, y, WHITE)
    canvas.fill_rect(x - 3, y - 9, 6, 9, BLACK)
    canvas.line(x, y - height + 3, x - width + 3, y, BLACK)


def cairn(canvas, x, y, height=18):
    """Waymarker: stacked stones, each one a small faceted blob."""
    stones = max(3, height // 6)
    for index in range(stones):
        f = index / float(max(1, stones - 1))
        half = max(2, int(5 - 3 * f))
        cy = y - int(height * (index + 0.5) / stones)
        shape = blob_points(x, cy, half, 6, 0.22, index * 1.7)
        fill_polygon(canvas, shape, BLACK)
        outline_polygon(canvas, shape, WHITE, 1)


def bridge(canvas, x, y, span=26):
    """Stone bridge carrying the road over a crevasse."""
    canvas.fill_rect(x - span, y - 9, span * 2, 5, BLACK)
    canvas.fill_rect(x - span + 1, y - 8, span * 2 - 2, 3, WHITE)
    for offset in range(-span, span, 8):
        canvas.fill_rect(x + offset, y - 7, 1, 3, BLACK)
    for pier in (-span // 2, span // 2):
        canvas.fill_rect(x + pier - 3, y, 6, 14, BLACK)
        canvas.fill_rect(x + pier - 2, y + 1, 4, 12, WHITE)
    for offset in range(-span + 4, span - 3, 7):
        canvas.fill_rect(x + offset, y - 12, 2, 4, BLACK)
    canvas.thick_line(x - span - 4, y + 14, x - span, y + 2, 2, BLACK)
    canvas.thick_line(x + span + 4, y + 14, x + span, y + 2, 2, BLACK)


def fence(canvas, points):
    """Post and rail fence."""
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        canvas.thick_line(x0, y0, x1, y1, 2, BLACK)
    for px, py in points:
        canvas.fill_rect(px - 1, py - 7, 2, 8, BLACK)


def tented_camp(canvas, x, y):
    """A tiny camp: tent, fire ring and a crate."""
    tent(canvas, x, y, 12, 16)
    canvas.ellipse(x + 16, y - 2, 6, 3, BLACK)
    canvas.ellipse(x + 16, y - 3, 4, 2, WHITE)
    canvas.fill_rect(x - 16, y - 9, 8, 9, BLACK)
    canvas.fill_rect(x - 15, y - 8, 6, 7, WHITE)
    canvas.line(x - 15, y - 8, x - 10, y - 2, BLACK)


def contour_hill(canvas, cx, cy, rx, ry, rings, value, fade=True):
    """Concentric contour lines suggesting a rounded slope.

    The rings are spaced so the outermost sits on the hill's edge and the
    spacing stays even, which is what makes it read as terrain rather than
    as a target.
    """
    rng = _rng(int(cx) * 31 + int(cy))
    for index in range(rings, 0, -1):
        f = index / float(rings)
        jitter_x = rng.uniform(-rx * 0.06, rx * 0.06)
        jitter_y = rng.uniform(-ry * 0.06, ry * 0.06)
        ox = cx + jitter_x
        oy = cy + jitter_y
        # Rings are drawn as broken arcs so they never close into a target.
        gaps = rng.randint(1, 3)
        for gap in range(gaps):
            start = rng.random() * math.tau
            span = math.tau / gaps * rng.uniform(0.55, 0.9)
            steps = max(6, int(rx * f / 2))
            for step in range(steps + 1):
                t = step / float(steps)
                angle = start + span * t
                px = ox + math.cos(angle) * rx * f
                py = oy + math.sin(angle) * ry * f
                canvas.set(int(px), int(py), value)
        if fade and index == rings:
            canvas.ellipse_outline(cx, cy, rx * f + 1, ry * f + 1, value)


def furrow_field(canvas, polygon, spacing, value, curve=0.18, phase=0.0):
    """Tilled rows inside a bounded plot.

    Rows follow a shallow arc and are clipped to the plot, so the plot reads
    as worked ground rather than a field of random dots.
    """
    import math
    min_x, min_y, max_x, max_y = _bounds(polygon)
    span = (max_x - min_x) + (max_y - min_y)
    if span <= 0:
        return
    cx = (min_x + max_x) / 2.0
    rows = int(span / spacing) + 2
    for index in range(-rows, rows + 1):
        base = index * spacing + phase
        points = []
        steps = 16
        for step in range(steps + 1):
            f = step / float(steps)
            px = min_x - 4 + (max_x - min_x + 8) * f
            py = base + math.sin(f * math.pi) * span * curve
            points.append((px, py))
        for step in range(len(points) - 1):
            x0, y0 = points[step]
            x1, y1 = points[step + 1]
            if not point_in_polygon((x0 + x1) / 2.0, (y0 + y1) / 2.0, polygon):
                continue
            canvas.line(x0, y0, x1, y1, value)

"""Background assembled from the AI-drawn engraving plus procedural detail.

The AI source (assets_src/ai/terrain_source.jpg) is a 1-bit style engraving of
the pass, generated once from the level layout guide and committed as a source
asset. This module thresholds it to a strict two-colour bitmap and then
re-cuts the road corridor so the gameplay path is pixel exact and blends with
the engraving instead of fighting it.
"""
import random
from pathlib import Path

from PIL import Image

from artgen.canvas import BLACK, WHITE, Canvas
from artgen import level_geom as geom
from artgen import objects as O

AI_SOURCE = Path(__file__).resolve().parent.parent / "ai" / "terrain_source.jpg"

ROAD_WIDTH_A = 16
ROAD_WIDTH_B = 12
CORRIDOR_MARGIN = 20

GATE = O


def _threshold(source, size=480):
    """AI art to a strict black and white bitmap."""
    grey = Image.open(source).convert("L").resize((size, size), Image.LANCZOS)
    grey = grey.point(lambda value: 255 if value > 128 else 0)
    canvas = Canvas(size, size)
    for y in range(size):
        for x in range(size):
            canvas.set(x, y, WHITE if grey.getpixel((x, y)) > 128 else BLACK)
    return canvas


def clear_corridor(canvas, points, radius, margin, rng):
    """Erase the artwork along the route with a stippled fade at the edge."""
    for y in range(max(0, int(min(p[1] for p in points) - radius - margin)),
                   min(canvas.height, int(max(p[1] for p in points) + radius + margin) + 1)):
        for x in range(max(0, int(min(p[0] for p in points) - radius - margin)),
                       min(canvas.width, int(max(p[0] for p in points) + radius + margin) + 1)):
            distance = geom.distance_to_paths(x, y)
            if distance <= radius:
                canvas.set(x, y, WHITE)
            elif distance <= radius + margin:
                fade = (radius + margin - distance) / float(margin)
                if rng.random() < fade * fade * 1.15:
                    canvas.set(x, y, WHITE)


def draw_road(canvas, points, width, rng):
    """A packed snow road: bright body, soft shoulder, thin contour."""
    canvas.polyline(points, width + 2, WHITE)

    # Soft, broken shoulder that fades into the snow.
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        length = int(((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5)
        if length <= 0:
            continue
        dx = (x1 - x0) / float(length)
        dy = (y1 - y0) / float(length)
        nx, ny = -dy, dx
        for step in range(3, length - 3, 6):
            px = x0 + dx * step
            py = y0 + dy * step
            for side in (-1, 1):
                if rng.random() < 0.45:
                    continue
                ox = px + nx * side * (width / 2 + 1)
                oy = py + ny * side * (width / 2 + 1)
                canvas.set(int(ox), int(oy), BLACK)
                canvas.set(int(ox + dx), int(oy + dy), BLACK)

    # Sparse wheel marks, kept faint so the road does not read as a diagram.
    for index in range(len(points) - 1):
        x0, y0 = points[index]
        x1, y1 = points[index + 1]
        length = int(((x1 - x0) ** 2 + (y1 - y0) ** 2) ** 0.5)
        if length <= 0:
            continue
        dx = (x1 - x0) / float(length)
        dy = (y1 - y0) / float(length)
        nx, ny = -dy, dx
        for step in range(4, length - 4, 13):
            px = x0 + dx * step
            py = y0 + dy * step
            if rng.random() < 0.45:
                continue
            side = 1 if rng.random() < 0.5 else -1
            canvas.set(int(px + nx * side * 3), int(py + ny * side * 3), BLACK)


def draw_gatehouse(canvas):
    """The goal: a curtain wall with two towers and a lit gate mouth."""
    gx, gy = geom.GOAL
    left, right = gx - 34, gx + 10
    base = gy + 18

    # Curtain wall.
    O._stone_wall(canvas, left + 15, base - 30, right - left - 30, 30, course=7)
    # Towers.
    for tx in (left, right - 15):
        O._stone_wall(canvas, tx, base - 46, 15, 46, course=6)
        for offset in range(0, 15, 6):
            canvas.fill_rect(tx + offset, base - 53, 4, 7, BLACK)
    # Gate mouth: an arch cut into the wall.
    canvas.fill_rect(gx - 9, base - 30, 18, 30, BLACK)
    canvas.fill_rect(gx - 6, base - 27, 12, 27, WHITE)
    canvas.ellipse(gx, base - 27, 8, 6, WHITE)
    canvas.ellipse(gx, base - 27, 5, 4, BLACK)
    # Snow on the tower tops.
    for tx in (left, right - 15):
        canvas.hline(tx - 1, base - 53, 17, WHITE)
    # Banner.
    canvas.fill_rect(gx + 2, base - 60, 2, 18, BLACK)
    canvas.fill_rect(gx + 4, base - 59, 11, 7, BLACK)


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


def build_background():
    """The AI engraving already contains the road, so nothing is drawn over it.

    Only the build clearings and the gatehouse are added, and both use the
    same stippled fade so they sit inside the illustration instead of on top
    of it.
    """
    rng = random.Random(0xF1E1D)
    canvas = _threshold(AI_SOURCE)

    # Clearings so every build pad reads as open ground.
    for x, y in geom.BUILD_SPOTS:
        clear_disc(canvas, x, y, 14, 12, rng)
    for x, y in geom.HERO_POSTS:
        clear_disc(canvas, x, y, 11, 10, rng)

    # The gatehouse sits at the end of the road.
    gx, gy = geom.GOAL
    clear_disc(canvas, gx - 10, gy - 10, 34, 16, rng)
    draw_gatehouse(canvas)
    return canvas

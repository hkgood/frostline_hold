"""Tone-first rendering for the sprite sheets.

Every shape here is lit as a solid. A surface gets a continuous tone between
ink (0.0) and paper (1.0) from one light coming from the upper left, and only
at the very end is that tone ordered-dithered down to the two-colour palette.
That is how a 1-bit sprite shows a curved cylinder, a domed shield or a
faceted crystal instead of a flat silhouette.

Coordinates are pixels in the target cell; x grows right, y grows down and z
points out of the screen towards the viewer.
"""
import math

from artgen.canvas import BLACK, WHITE, Canvas, point_in_polygon

BAYER4 = [
    [0, 8, 2, 10],
    [12, 4, 14, 6],
    [3, 11, 1, 9],
    [15, 7, 13, 5],
]


def _normalise(x, y, z):
    length = math.sqrt(x * x + y * y + z * z) or 1.0
    return x / length, y / length, z / length


LIGHT = _normalise(-0.55, -0.68, 0.50)
# Half vector between the light and a viewer looking straight in.
HALF = _normalise(LIGHT[0], LIGHT[1], LIGHT[2] + 1.0)


def lit(nx, ny, nz, albedo, ambient=0.08, spec=0.0, shine=14):
    """Wrapped diffuse plus an optional Blinn highlight.

    Plain Lambert sends every surface past the terminator to solid ink, which
    at 1 bit reads as a hole. Wrapping the falloff keeps a readable ramp all
    the way round a cylinder.
    """
    nx, ny, nz = _normalise(nx, ny, nz)
    facing = nx * LIGHT[0] + ny * LIGHT[1] + nz * LIGHT[2]
    # Lit faces saturate to paper and the dither band stays narrow, which is
    # what keeps a 1-bit sprite crisp instead of a grey checkerboard.
    f = max(0.0, min(1.0, (facing + 0.4) / 1.1))
    diffuse = f * f * (3.0 - 2.0 * f)
    tone = albedo * (ambient + (1.0 - ambient) * diffuse)
    if spec:
        facing = max(0.0, nx * HALF[0] + ny * HALF[1] + nz * HALF[2])
        tone += spec * facing ** shine
    return tone


def hash01(x, y, seed=0):
    """Stable pseudo-random value in [0, 1) for a grid cell."""
    value = (int(x) * 374761393 + int(y) * 668265263 + seed * 2147483647)
    value = (value ^ (value >> 13)) * 1274126177
    return ((value ^ (value >> 16)) & 0xFFFF) / 65536.0


class ToneCanvas:
    """A buffer of tones. None is transparent.

    A separate shadow layer holds cast shadows: they only land on pixels the
    object itself leaves transparent, and they dither to ink over the map.
    """

    def __init__(self, width, height):
        self.width = int(width)
        self.height = int(height)
        self.tone = [None] * (self.width * self.height)
        self.shadow = [0.0] * (self.width * self.height)

    def _index(self, x, y):
        if 0 <= x < self.width and 0 <= y < self.height:
            return y * self.width + x
        return -1

    def put(self, x, y, tone):
        index = self._index(int(x), int(y))
        if index >= 0:
            self.tone[index] = min(1.0, max(0.0, tone))

    def get(self, x, y):
        index = self._index(int(x), int(y))
        return self.tone[index] if index >= 0 else None

    def scale(self, x, y, factor):
        """Darken or lighten a pixel that is already painted."""
        index = self._index(int(x), int(y))
        if index >= 0 and self.tone[index] is not None:
            self.tone[index] = min(1.0, max(0.0, self.tone[index] * factor))

    def cast(self, x, y, density):
        index = self._index(int(x), int(y))
        if index >= 0:
            self.shadow[index] = max(self.shadow[index], density)

    def resized(self, width, height):
        """Box-filter the tones to a new size, before any dithering.

        Shrinking a dithered sprite destroys its pattern; shrinking the tones
        and dithering afterwards keeps the shading readable at icon size.
        """
        out = ToneCanvas(width, height)
        sx = self.width / float(width)
        sy = self.height / float(height)
        for y in range(height):
            for x in range(width):
                total = 0.0
                covered = 0
                samples = 0
                y0, y1 = int(y * sy), max(int(y * sy) + 1, int((y + 1) * sy))
                x0, x1 = int(x * sx), max(int(x * sx) + 1, int((x + 1) * sx))
                for yy in range(y0, min(y1, self.height)):
                    for xx in range(x0, min(x1, self.width)):
                        samples += 1
                        tone = self.tone[yy * self.width + xx]
                        if tone is not None:
                            total += tone
                            covered += 1
                if samples and covered * 2 >= samples:
                    out.tone[y * width + x] = total / covered
        return out

    def to_canvas(self, outline=True, origin=(0, 0), halo=True):
        """Dither to ink and paper; ring the silhouette with ink.

        origin shifts the Bayer grid, so a sprite baked into the map uses the
        same pattern phase as the map pixels around it.

        halo adds a one pixel paper ring outside the ink outline. On snow it
        disappears; over forest, rock or cliff hatching it lifts the sprite
        off the engraving. The cast shadow under the feet is left alone.
        """
        ox, oy = origin
        canvas = Canvas(self.width, self.height)
        for y in range(self.height):
            row = BAYER4[(y + oy) % 4]
            for x in range(self.width):
                index = y * self.width + x
                tone = self.tone[index]
                threshold = (row[(x + ox) % 4] + 0.5) / 16.0
                if tone is None:
                    if self.shadow[index] > threshold:
                        canvas.set(x, y, BLACK)
                    continue
                canvas.set(x, y, WHITE if tone > threshold else BLACK)
        if outline:
            for y in range(self.height):
                for x in range(self.width):
                    if self.tone[y * self.width + x] is None:
                        continue
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        if self.get(x + dx, y + dy) is None:
                            canvas.set(x, y, BLACK)
                            break
        if halo:
            for y in range(self.height):
                for x in range(self.width):
                    index = y * self.width + x
                    if self.tone[index] is not None or self.shadow[index] > 0.05:
                        continue
                    for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                        if self.get(x + dx, y + dy) is not None:
                            canvas.set(x, y, WHITE)
                            break
        return canvas


# ----------------------------------------------------------------------
# Lit solids
# ----------------------------------------------------------------------


def ellipsoid(tc, cx, cy, rx, ry, albedo, spec=0.0, depth=1.0, tex=None):
    """A sphere or a squashed sphere. depth < 1 flattens it towards a disc."""
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
            u = (x + 0.5 - cx) / rx
            v = (y + 0.5 - cy) / ry
            d2 = u * u + v * v
            if d2 > 1.0:
                continue
            nz = math.sqrt(1.0 - d2) / max(0.05, depth)
            tone = lit(u, v, nz, albedo, spec=spec)
            if tex:
                tone += tex(x, y, u, v)
            tc.put(x, y, tone)


def cylinder(tc, cx, top, bottom, r, albedo, tex=None, lid=0.32,
             cap=None, spec=0.0):
    """An upright cylinder seen from slightly above.

    tex receives the pixel and the angular position (-1 left .. 1 right) so a
    texture can wrap: masonry blocks narrow towards the silhouette edge.
    """
    ry = r * lid
    for y in range(int(top), int(bottom + ry) + 1):
        for x in range(int(cx - r), int(cx + r) + 1):
            u = (x + 0.5 - cx) / r
            if abs(u) > 1.0:
                continue
            arc = math.sqrt(1.0 - u * u)
            if y + 0.5 > bottom + ry * arc:
                continue
            tone = lit(u, 0.05, arc, albedo, spec=spec)
            if tex:
                tone += tex(x, y, u, (y - top) / max(1.0, bottom - top))
            tc.put(x, y, tone)
    if cap is not None:
        for y in range(int(top - ry) - 1, int(top + ry) + 2):
            for x in range(int(cx - r), int(cx + r) + 1):
                u = (x + 0.5 - cx) / r
                v = (y + 0.5 - top) / max(0.5, ry)
                if u * u + v * v > 1.0:
                    continue
                tc.put(x, y, lit(u * 0.25, -1.0, 0.45, cap))


def cone(tc, cx, apex, base, r, albedo, tex=None, lid=0.32, spec=0.0):
    """An upright cone, apex up. Used for roofs, cloaks and ice spires."""
    height = max(1.0, base - apex)
    ry = r * lid
    slope = r / height
    for y in range(int(apex), int(base + ry) + 1):
        f = min(1.0, max(0.0, (y + 0.5 - apex) / height))
        radius = max(0.6, r * f)
        for x in range(int(cx - radius) - 1, int(cx + radius) + 2):
            u = (x + 0.5 - cx) / radius
            if abs(u) > 1.0:
                continue
            arc = math.sqrt(1.0 - u * u)
            if f >= 1.0 and y + 0.5 > base + ry * arc:
                continue
            tone = lit(u, -slope, arc, albedo, spec=spec)
            if tex:
                tone += tex(x, y, u, f)
            tc.put(x, y, tone)


def capsule(tc, x0, y0, x1, y1, width, albedo, spec=0.0, tex=None):
    """A rounded limb, beam or haft, shaded across its width."""
    half = width / 2.0
    dx, dy = x1 - x0, y1 - y0
    length2 = dx * dx + dy * dy
    # The side normal of the stroke, in screen space.
    length = math.sqrt(length2) or 1.0
    px, py = -dy / length, dx / length
    for y in range(int(min(y0, y1) - half) - 1, int(max(y0, y1) + half) + 2):
        for x in range(int(min(x0, x1) - half) - 1, int(max(x0, x1) + half) + 2):
            cx, cy = x + 0.5, y + 0.5
            t = 0.0 if length2 == 0 else max(0.0, min(
                1.0, ((cx - x0) * dx + (cy - y0) * dy) / length2))
            ex, ey = x0 + dx * t - cx, y0 + dy * t - cy
            distance = math.sqrt(ex * ex + ey * ey)
            if distance > half:
                continue
            across = -(ex * px + ey * py) / max(0.5, half)
            nz = math.sqrt(max(0.0, 1.0 - across * across))
            tone = lit(px * across, py * across, nz, albedo, spec=spec)
            if tex:
                tone += tex(x, y, across, t)
            tc.put(x, y, tone)


def polygon(tc, points, normal, albedo, spec=0.0, tex=None):
    """A flat facet with one normal: a plank face, a crystal side, a blade."""
    xs = [p[0] for p in points]
    ys = [p[1] for p in points]
    tone = lit(normal[0], normal[1], normal[2], albedo, spec=spec)
    for y in range(int(min(ys)), int(max(ys)) + 1):
        for x in range(int(min(xs)), int(max(xs)) + 1):
            if point_in_polygon(x + 0.5, y + 0.5, points):
                value = tone + (tex(x, y, 0.0, 0.0) if tex else 0.0)
                tc.put(x, y, value)


def box(tc, x, y, w, h, depth, albedo, tex=None, top_albedo=None):
    """A block seen from the front and a little above the right shoulder.

    Front face, lit top, and a shaded right side, so a crenel or a beam end
    reads as a solid with three planes.
    """
    dx = depth * 0.55
    dy = depth * 0.5
    side = [(x + w, y), (x + w + dx, y - dy), (x + w + dx, y + h - dy),
            (x + w, y + h)]
    polygon(tc, side, (1.0, 0.1, 0.55), albedo, tex=tex)
    top = [(x, y), (x + w, y), (x + w + dx, y - dy), (x + dx, y - dy)]
    polygon(tc, top, (-0.1, -1.0, 0.55),
            top_albedo if top_albedo is not None else albedo)
    front = [(x, y), (x + w, y), (x + w, y + h), (x, y + h)]
    polygon(tc, front, (-0.15, 0.0, 1.0), albedo, tex=tex)


def shadow_ellipse(tc, cx, cy, rx, ry, density=0.5):
    """A soft contact shadow, denser at the middle."""
    for y in range(int(cy - ry) - 1, int(cy + ry) + 2):
        for x in range(int(cx - rx) - 1, int(cx + rx) + 2):
            u = (x + 0.5 - cx) / rx
            v = (y + 0.5 - cy) / ry
            d2 = u * u + v * v
            if d2 <= 1.0:
                tc.cast(x, y, density * (1.0 - 0.55 * d2))


def occlude(tc, foot_y, height, strength=0.55):
    """Ambient occlusion: everything painted near the ground darkens."""
    for y in range(int(foot_y - height), int(foot_y) + 3):
        f = 1.0 - max(0.0, min(1.0, (foot_y - y) / float(height)))
        for x in range(tc.width):
            tc.scale(x, y, 1.0 - strength * f * f)


def glint(tc, x, y, size=1):
    """A pure paper spark, the brightest thing in a sprite."""
    tc.put(x, y, 1.0)
    for step in range(1, size + 1):
        tc.put(x - step, y, 0.92)
        tc.put(x + step, y, 0.92)
        tc.put(x, y - step, 0.92)
        tc.put(x, y + step, 0.92)


# ----------------------------------------------------------------------
# Material textures. Each returns a tone offset for one pixel.
# ----------------------------------------------------------------------


def masonry(r, course=6, blocks=6, seed=0, depth=0.5):
    """Coursed stone for a cylinder: mortar, lit top bevels, per-block tone."""
    def tex(x, y, u, _v):
        row, inner = divmod(int(y), course)
        angle = math.asin(max(-1.0, min(1.0, u)))
        position = (angle / math.pi + 0.5) * blocks + (0.5 if row % 2 else 0.0)
        column = math.floor(position)
        # How many pixels one block is wide at this point on the curve.
        width_px = max(1.0, (math.pi / blocks) * r * math.cos(angle))
        if inner == 0 or (position - column) * width_px < 1.0:
            return -depth
        offset = (hash01(column, row, seed) - 0.5) * 0.08
        if inner == 1:
            offset += 0.10
        elif inner == course - 1:
            offset -= 0.06
        return offset
    return tex


def flat_masonry(course=6, block=9, seed=0, depth=0.5):
    """Coursed stone for a flat wall face."""
    def tex(x, y, _u, _v):
        row, inner = divmod(int(y), course)
        column, along = divmod(int(x) + (block // 2 if row % 2 else 0), block)
        if inner == 0 or along == 0:
            return -depth
        offset = (hash01(column, row, seed) - 0.5) * 0.08
        if inner == 1:
            offset += 0.10
        elif inner == course - 1:
            offset -= 0.06
        if along == 1:
            offset += 0.05
        return offset
    return tex


def wood(seed=0, spacing=3, depth=0.4, vertical=False):
    """Plank grain: dark seams with a little knot noise."""
    def tex(x, y, _u, _v):
        lane = int(x) if vertical else int(y)
        if lane % spacing == 0:
            return -depth
        return (hash01(x // 3, y, seed) - 0.5) * 0.12
    return tex


def folds(count=6, depth=0.16):
    """Cloth falling in vertical folds."""
    def tex(_x, _y, u, _v):
        return depth * math.sin(u * math.pi * count * 0.5)
    return tex


def fur(seed=0, depth=0.08):
    """Short strands, so a coat is never a flat ramp."""
    def tex(x, y, _u, _v):
        return (hash01(x, y // 2, seed) - 0.5) * depth * 2.0
    return tex


def shingles(rows=4, depth=0.22):
    """Rows of slates on a cone; the lower lip of each row is in shade."""
    def tex(_x, _y, _u, f):
        band = (f * rows) % 1.0
        return -depth if band > 0.82 else (0.06 if band < 0.2 else 0.0)
    return tex

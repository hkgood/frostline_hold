"""Strict 1-bit pixel canvas.

A pixel is one of three states: unset (transparent), black, or white. No grey
value is ever produced, so dithering is the only way to express tone.
"""
from PIL import Image

BAYER8 = [
    [0, 32, 8, 40, 2, 34, 10, 42],
    [48, 16, 56, 24, 50, 18, 58, 26],
    [12, 44, 4, 36, 14, 46, 6, 38],
    [60, 28, 52, 20, 62, 30, 54, 22],
    [3, 35, 11, 43, 1, 33, 9, 41],
    [51, 19, 59, 27, 49, 17, 57, 25],
    [15, 47, 7, 39, 13, 45, 5, 37],
    [63, 31, 55, 23, 61, 29, 53, 21],
]

WHITE = 255
BLACK = 0
UNSET = -1

_STATE_UNSET = 0
_STATE_BLACK = 1
_STATE_WHITE = 2


class Canvas:
    """1-bit canvas backed by a bytearray of three-valued pixels."""

    def __init__(self, width, height):
        self.width = int(width)
        self.height = int(height)
        self.pixels = bytearray(self.width * self.height)

    # -- pixel access ----------------------------------------------------
    def set(self, x, y, value):
        """value: 255 = white, 0 = black, None = clear to transparent."""
        if not (0 <= x < self.width and 0 <= y < self.height):
            return
        index = y * self.width + x
        if value is None:
            self.pixels[index] = _STATE_UNSET
        elif value:
            self.pixels[index] = _STATE_WHITE
        else:
            self.pixels[index] = _STATE_BLACK

    def get(self, x, y):
        """255 = white, 0 = black, -1 = unset."""
        if not (0 <= x < self.width and 0 <= y < self.height):
            return UNSET
        state = self.pixels[y * self.width + x]
        if state == _STATE_WHITE:
            return WHITE
        if state == _STATE_BLACK:
            return BLACK
        return UNSET

    def is_set(self, x, y):
        if not (0 <= x < self.width and 0 <= y < self.height):
            return False
        return self.pixels[y * self.width + x] != _STATE_UNSET

    def is_white(self, x, y):
        return self.get(x, y) == WHITE

    def count_set(self):
        return sum(1 for state in self.pixels if state != _STATE_UNSET)

    # -- primitives ------------------------------------------------------
    def fill_rect(self, x, y, w, h, value):
        for row in range(y, y + h):
            for col in range(x, x + w):
                self.set(col, row, value)

    def hline(self, x, y, w, value):
        self.fill_rect(x, y, w, 1, value)

    def vline(self, x, y, h, value):
        self.fill_rect(x, y, 1, h, value)

    def line(self, x0, y0, x1, y1, value):
        x0, y0, x1, y1 = int(x0), int(y0), int(x1), int(y1)
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx + dy
        while True:
            self.set(x0, y0, value)
            if x0 == x1 and y0 == y1:
                break
            err2 = 2 * err
            if err2 >= dy:
                err += dy
                x0 += sx
            if err2 <= dx:
                err += dx
                y0 += sy

    def disc(self, cx, cy, r, value):
        cx, cy, r = int(cx), int(cy), int(r)
        if r <= 0:
            self.set(cx, cy, value)
            return
        for row in range(cy - r, cy + r + 1):
            for col in range(cx - r, cx + r + 1):
                if (col - cx) ** 2 + (row - cy) ** 2 <= r * r:
                    self.set(col, row, value)

    def ellipse(self, cx, cy, rx, ry, value):
        cx, cy = int(cx), int(cy)
        rx, ry = float(rx), float(ry)
        if rx <= 0 or ry <= 0:
            self.set(cx, cy, value)
            return
        for row in range(cy - int(ry), cy + int(ry) + 1):
            for col in range(cx - int(rx), cx + int(rx) + 1):
                if ((col - cx) / rx) ** 2 + ((row - cy) / ry) ** 2 <= 1.0:
                    self.set(col, row, value)

    def triangle(self, x0, y0, x1, y1, x2, y2, value):
        """Filled triangle using the sign test on each edge."""
        x0, y0, x1, y1, x2, y2 = int(x0), int(y0), int(x1), int(y1), int(x2), int(y2)
        min_x = max(0, int(min(x0, x1, x2)))
        max_x = min(self.width - 1, int(max(x0, x1, x2)))
        min_y = max(0, int(min(y0, y1, y2)))
        max_y = min(self.height - 1, int(max(y0, y1, y2)))

        def edge(ax, ay, bx, by, cx, cy):
            return (ax - cx) * (by - cy) - (bx - cx) * (ay - cy)

        for row in range(min_y, max_y + 1):
            for col in range(min_x, max_x + 1):
                d1 = edge(col, row, x0, y0, x1, y1)
                d2 = edge(col, row, x1, y1, x2, y2)
                d3 = edge(col, row, x2, y2, x0, y0)
                has_neg = d1 < 0 or d2 < 0 or d3 < 0
                has_pos = d1 > 0 or d2 > 0 or d3 > 0
                if not (has_neg and has_pos):
                    self.set(col, row, value)

    def rotated_ellipse(self, cx, cy, rx, ry, angle_deg, value):
        """Ellipse rotated about its centre."""
        import math
        radians = math.radians(angle_deg)
        cos_a, sin_a = math.cos(radians), math.sin(radians)
        bound = int(max(rx, ry)) + 1
        for row in range(cy - bound, cy + bound + 1):
            for col in range(cx - bound, cx + bound + 1):
                dx, dy = col - cx, row - cy
                lx = dx * cos_a + dy * sin_a
                ly = -dx * sin_a + dy * cos_a
                if (lx / rx) ** 2 + (ly / ry) ** 2 <= 1.0:
                    self.set(col, row, value)

    def polygon(self, points, value):
        """Filled convex polygon given as a list of (x, y) vertices."""
        if len(points) < 3:
            return
        for index in range(1, len(points) - 1):
            ax, ay = points[0]
            bx, by = points[index]
            cx, cy = points[index + 1]
            self.triangle(ax, ay, bx, by, cx, cy, value)

    def quad(self, x0, y0, x1, y1, x2, y2, x3, y3, value):
        """Filled quadrilateral, split into two triangles."""
        self.triangle(x0, y0, x1, y1, x2, y2, value)
        self.triangle(x0, y0, x2, y2, x3, y3, value)

    def thick_line(self, x0, y0, x1, y1, width, value):
        """Line of the given pixel width: a quad plus round end caps."""
        x0, y0, x1, y1 = int(x0), int(y0), int(x1), int(y1)
        half = width / 2.0
        dx, dy = x1 - x0, y1 - y0
        length = (dx * dx + dy * dy) ** 0.5
        if length == 0.0:
            self.disc(x0, y0, int(half), value)
            return
        nx, ny = -dy / length * half, dx / length * half
        self.quad(x0 + nx, y0 + ny, x1 + nx, y1 + ny,
                  x1 - nx, y1 - ny, x0 - nx, y0 - ny, value)
        self.disc(x0, y0, int(half), value)
        self.disc(x1, y1, int(half), value)

    def polyline(self, points, width, value):
        """Thick polyline: every segment is drawn with the requested width."""
        for index in range(len(points) - 1):
            x0, y0 = points[index]
            x1, y1 = points[index + 1]
            self.thick_line(x0, y0, x1, y1, width, value)

    def blit(self, other, x, y):
        """Copy another canvas over this one; unset pixels do not overwrite."""
        for row in range(other.height):
            for col in range(other.width):
                if other.is_set(col, row):
                    self.set(x + col, y + row, other.get(col, row))

    # -- export ----------------------------------------------------------
    def to_image(self):
        img = Image.new("RGBA", (self.width, self.height), (0, 0, 0, 0))
        data = []
        for state in self.pixels:
            if state == _STATE_WHITE:
                data.append((255, 255, 255, 255))
            elif state == _STATE_BLACK:
                data.append((0, 0, 0, 255))
            else:
                data.append((0, 0, 0, 0))
        img.putdata(data)
        return img


def dither_value(x, y, offset=(0, 0)):
    ox, oy = offset
    return BAYER8[(y + oy) % 8][(x + ox) % 8]


def dither_rect(canvas, x, y, w, h, density, offset=(0, 0)):
    """Fill a region with an ordered-dither pattern of the given density.

    density 0.0 leaves the region untouched; 1.0 fills it solid black.
    """
    limit = density * 64.0
    for row in range(y, y + h):
        for col in range(x, x + w):
            if dither_value(col, row, offset) < limit:
                canvas.set(col, row, BLACK)


def dither_disc(canvas, cx, cy, r, density, offset=(0, 0)):
    """Same as dither_rect but clipped to a disc."""
    limit = density * 64.0
    for row in range(cy - r, cy + r + 1):
        for col in range(cx - r, cx + r + 1):
            if (col - cx) ** 2 + (row - cy) ** 2 > r * r:
                continue
            if dither_value(col, row, offset) < limit:
                canvas.set(col, row, BLACK)


def point_in_polygon(px, py, points):
    """Even-odd ray casting test."""
    inside = False
    j = len(points) - 1
    for i in range(len(points)):
        xi, yi = points[i]
        xj, yj = points[j]
        if (yi > py) != (yj > py):
            crossing = (xj - xi) * (py - yi) / (yj - yi) + xi
            if px < crossing:
                inside = not inside
        j = i
    return inside


def dither_polygon(canvas, points, density, offset=(0, 0)):
    """Fill an irregular polygon with ordered dithering.

    Polygons keep a hard edge, which is what separates a tilled field from a
    soft circle of dots.
    """
    limit = density * 64.0
    xs = [point[0] for point in points]
    ys = [point[1] for point in points]
    for row in range(int(min(ys)), int(max(ys)) + 1):
        for col in range(int(min(xs)), int(max(xs)) + 1):
            if not point_in_polygon(col, row, points):
                continue
            if dither_value(col, row, offset) < limit:
                canvas.set(col, row, BLACK)

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
        if r <= 0:
            self.set(cx, cy, value)
            return
        for row in range(cy - r, cy + r + 1):
            for col in range(cx - r, cx + r + 1):
                if (col - cx) ** 2 + (row - cy) ** 2 <= r * r:
                    self.set(col, row, value)

    def ellipse(self, cx, cy, rx, ry, value):
        if rx <= 0 or ry <= 0:
            self.set(cx, cy, value)
            return
        for row in range(cy - ry, cy + ry + 1):
            for col in range(cx - rx, cx + rx + 1):
                if ((col - cx) / rx) ** 2 + ((row - cy) / ry) ** 2 <= 1.0:
                    self.set(col, row, value)

    def polyline(self, points, width, value):
        """Thick polyline: one line per segment plus a disc at every joint."""
        half = max(0, width // 2)
        for index in range(len(points) - 1):
            x0, y0 = points[index]
            x1, y1 = points[index + 1]
            self.line(x0, y0, x1, y1, value)
        for px, py in points:
            self.disc(px, py, half, value)

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

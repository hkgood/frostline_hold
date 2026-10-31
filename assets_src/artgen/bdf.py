"""A minimal BDF bitmap-font reader, and the subsetter that keeps the
repository copy of each font small.

Only what the game needs is read: each glyph's advance, its bounding box and
its rows of bits, plus the font's ascent and descent. Glyphs come back as
lists of row strings of "0"/"1", top row first, already placed on the font's
line box (ascent + descent rows tall), so a caller can blit them without
caring about per-glyph offsets.
"""
from pathlib import Path


class BdfFont:
    def __init__(self, ascent, descent, glyphs):
        self.ascent = ascent
        self.descent = descent
        self.glyphs = glyphs   # codepoint -> (advance, rows)

    @property
    def line_height(self):
        return self.ascent + self.descent

    def has(self, codepoint):
        return codepoint in self.glyphs

    def glyph(self, codepoint):
        return self.glyphs.get(codepoint)


def _place(bitmap_hex, bbx, ascent, descent):
    """Turn one glyph's hex rows into full-line rows of bits."""
    width, height, x_off, y_off = bbx
    line = ascent + descent
    rows = [[0] * max(width + max(x_off, 0), 1) for _ in range(line)]
    top = ascent - (height + y_off)
    for r, value in enumerate(bitmap_hex):
        bits = bin(int(value, 16))[2:].zfill(len(value) * 4)
        y = top + r
        if not 0 <= y < line:
            continue
        for c in range(width):
            if bits[c] == "1":
                x = c + x_off
                if x >= 0:
                    while x >= len(rows[y]):
                        rows[y].append(0)
                    rows[y][x] = 1
    return rows


def load(path, keep=None):
    """Read a BDF file. `keep` (a set of codepoints) skips everything else."""
    ascent = descent = 0
    glyphs = {}
    encoding = advance = None
    bbx = None
    bitmap = None
    with open(path, encoding="utf-8", errors="ignore") as handle:
        for raw in handle:
            line = raw.strip()
            if line.startswith("FONT_ASCENT "):
                ascent = int(line.split()[1])
            elif line.startswith("FONT_DESCENT "):
                descent = int(line.split()[1])
            elif line.startswith("STARTCHAR"):
                encoding = advance = bbx = None
                bitmap = None
            elif line.startswith("ENCODING "):
                encoding = int(line.split()[1])
            elif line.startswith("DWIDTH "):
                advance = int(line.split()[1])
            elif line.startswith("BBX "):
                bbx = tuple(int(v) for v in line.split()[1:5])
            elif line == "BITMAP":
                bitmap = []
            elif line == "ENDCHAR":
                if (bitmap is not None and encoding is not None
                        and (keep is None or encoding in keep)):
                    glyphs[encoding] = (advance, bitmap, bbx)
                bitmap = None
            elif bitmap is not None:
                bitmap.append(line)
    placed = {}
    for codepoint, (advance, bitmap, bbx) in glyphs.items():
        placed[codepoint] = (advance, _place(bitmap, bbx, ascent, descent))
    return BdfFont(ascent, descent, placed)


def gb2312_codepoints():
    """Every character GB2312 encodes, plus printable ASCII: the set the
    repository copies of the fonts keep."""
    keep = set(range(0x20, 0x7F))
    for high in range(0xA1, 0xF8):
        for low in range(0xA1, 0xFF):
            try:
                text = bytes((high, low)).decode("gb2312")
            except UnicodeDecodeError:
                continue
            keep.add(ord(text))
    # Full-width forms and CJK punctuation used in running text.
    keep.update(range(0x3000, 0x3020))
    keep.update(range(0xFF01, 0xFF5F))
    keep.update((0x2014, 0x2026, 0x00B7, 0x2018, 0x2019, 0x201C, 0x201D))
    return keep


def subset(source, destination, keep):
    """Copy a BDF file keeping only the glyphs in `keep`."""
    out = []
    block = []
    inside = False
    count = 0
    with open(source, encoding="utf-8", errors="ignore") as handle:
        for raw in handle:
            line = raw.rstrip("\n")
            if line.startswith("STARTCHAR"):
                inside = True
                block = [line]
                continue
            if inside:
                block.append(line)
                if line == "ENDCHAR":
                    inside = False
                    encoding = next(int(b.split()[1]) for b in block
                                    if b.startswith("ENCODING "))
                    if encoding in keep:
                        out.extend(block)
                        count += 1
                continue
            if line.startswith("CHARS "):
                out.append("CHARS @COUNT@")
                continue
            if line.startswith("ENDFONT"):
                break
            out.append(line)
    out.append("ENDFONT")
    text = "\n".join(out).replace("CHARS @COUNT@", f"CHARS {count}") + "\n"
    Path(destination).write_text(text, encoding="utf-8")
    return count

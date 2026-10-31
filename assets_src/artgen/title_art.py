"""The title and ending backdrops: the fortress by night and at dawn.

Both plates are AI engravings of the same fortress from the same viewpoint
(assets_src/ai/title_backdrop_source.jpg and ending_backdrop_source.jpg),
thresholded to the panel's two tones like the battle maps. The title plate
is lowered so its night sky gains a band for the logo; the stars in that
band are placed by a hash so the plate is reproducible.

The view animates the parts that move - aurora, snow, the braziers and the
sun's rays - at the points listed here and mirrored in frostline_screens.c.
"""
from artgen.ai_terrain import AI_DIR, _threshold
from artgen.canvas import BLACK, WHITE, Canvas
from artgen import shade as S

SIZE = 480
TITLE_SOURCE = "title_backdrop_source.jpg"
ENDING_SOURCE = "ending_backdrop_source.jpg"

# How far the title plate is lowered to open sky for the logo.
TITLE_DROP = 72


def _starfield(canvas, top, bottom, seed):
    """Dark sky with scattered stars, a few of them crossed."""
    for y in range(top, bottom):
        for x in range(SIZE):
            canvas.set(x, y, BLACK)
    for y in range(top, bottom):
        for x in range(SIZE):
            h = S.hash01(x, y, seed)
            if h < 0.006:
                canvas.set(x, y, WHITE)
            elif h < 0.0068 and 2 <= x < SIZE - 2 and top + 2 <= y < bottom - 2:
                for d in (-2, -1, 1, 2):
                    canvas.set(x + d, y, WHITE)
                    canvas.set(x, y + d, WHITE)
                canvas.set(x, y, WHITE)


def build_title_backdrop():
    plate = _threshold(AI_DIR / TITLE_SOURCE)
    canvas = Canvas(SIZE, SIZE)
    _starfield(canvas, 0, TITLE_DROP + 4, 0x57A7)
    for y in range(TITLE_DROP, SIZE):
        for x in range(SIZE):
            canvas.set(x, y, plate.get(x, y - TITLE_DROP))
    return canvas


def build_ending_backdrop():
    return _threshold(AI_DIR / ENDING_SOURCE)

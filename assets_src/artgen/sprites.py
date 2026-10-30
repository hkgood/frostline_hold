"""Procedural 1-bit sprite sheets for Frostline Hold.

The sprites share the engraving language of the background: a solid black
silhouette for the mass, with white lines carved into it for structure,
texture and light. Nothing is a flat geometric blob and nothing is a white
block with an outline.

Every sprite is authored at its final pixel size, so the packer never
rescales anything and the two-colour rule survives. Each sheet is a uniform
grid; the atlas cell is four pixels larger than the source cell so the
packer's thumbnail step never triggers a LANCZOS resample.
"""
from artgen.canvas import (BLACK, WHITE, Canvas, fill_polygon, hatch_polygon,
                           outline_polygon)

UNIT_CELL = 48
PROP_CELL = 32
FX_CELL = 16

UNIT_ATLAS_CELL = UNIT_CELL + 4
PROP_ATLAS_CELL = PROP_CELL + 4
FX_ATLAS_CELL = FX_CELL + 4


def _canvas(size):
    return Canvas(size, size)


def _carve_masonry(canvas, x0, x1, y0, y1, courses=4, stagger=True):
    """Carve mortar joints into a black stone mass."""
    step = max(3, (y1 - y0) // courses)
    row = 0
    for y in range(y0 + step, y1, step):
        if stagger and row % 2 == 0:
            canvas.hline(x0 + 3, y, x1 - x0 - 3, WHITE)
            canvas.set(x0 + 1, y, WHITE)
        else:
            canvas.hline(x0 + 1, y, x1 - x0 - 1, WHITE)
        row += 1


# ---------------------------------------------------------------- towers
def _stone_block_rows(canvas, x0, x1, y0, y1, rows=2):
    """Carve a few stone joints, broken into blocks instead of full stripes."""
    step = max(4, (y1 - y0) // rows)
    for index, y in enumerate(range(y0 + step, y1, step)):
        if index % 2 == 0:
            canvas.hline(x0 + 4, y, x1 - x0 - 4, WHITE)
            canvas.set(x0 + 1, y, WHITE)
        else:
            canvas.hline(x0 + 2, y, x1 - x0 - 2, WHITE)
            canvas.set(x1 - 2, y, WHITE)


def _guard_tower(level, branch=None):
    """Ballista tower: coursed stone shaft, timber deck, crossbow."""
    c = _canvas(UNIT_CELL)
    base = 46
    half = 14 + level
    top = base - (22 + level * 4)

    # Stone shaft: one solid black mass with a lit left edge.
    fill_polygon(c, [(24 - half, base), (24 + half, base),
                      (24 + half - 3, top), (24 - half + 3, top)], BLACK)
    c.vline(24 - half + 2, top + 3, base - top - 4, WHITE)
    c.vline(24 + half - 4, top + 3, base - top - 4, WHITE)
    _stone_block_rows(c, 24 - half, 24 + half, top + 3, base - 1, 2)
    # Arrow slit and a band of snow along the base.
    c.fill_rect(23, top + 6, 2, 6, WHITE)
    c.fill_rect(24 - half + 1, base - 3, half * 2 - 2, 1, WHITE)

    # Timber deck with merlons.
    c.fill_rect(24 - half - 2, top - 5, half * 2 + 4, 6, BLACK)
    c.fill_rect(24 - half, top - 3, half * 2, 1, WHITE)
    for post in (-half + 1, -4, half - 8):
        c.fill_rect(24 + post, top - 10, 5, 6, BLACK)

    # Crossbow: limbs, string and bolt, all fully inside the cell.
    pivot_y = top - 6
    c.thick_line(24, pivot_y, 24 - 10, pivot_y - 6, 3, BLACK)
    c.thick_line(24, pivot_y, 24 + 10, pivot_y - 6, 3, BLACK)
    c.line(24 - 10, pivot_y - 6, 24, pivot_y - 2, WHITE)
    c.line(24 + 10, pivot_y - 6, 24, pivot_y - 2, WHITE)
    c.fill_rect(23, pivot_y - 15, 3, 17, BLACK)
    c.fill_rect(24, pivot_y - 13, 1, 13, WHITE)

    if branch == "pierce":
        c.polygon([(20, pivot_y - 9), (28, pivot_y - 9), (24, pivot_y - 18)], BLACK)
        c.fill_rect(23, pivot_y - 7, 2, 9, WHITE)
    elif branch == "rapid":
        c.disc(24, pivot_y + 9, 5, BLACK)
        c.disc(24, pivot_y + 9, 3, WHITE)
        c.fill_rect(24 - 13, pivot_y - 8, 4, 8, BLACK)
        c.fill_rect(24 + 9, pivot_y - 8, 4, 8, BLACK)
    elif level >= 3:
        c.fill_rect(24 + half + 2, top - 4, 3, 17, BLACK)
        c.fill_rect(24 + half + 2, top - 4, 3, 12, WHITE)
    return c


def _frost_tower(level, branch=None):
    """Frost tower: coursed stone, a faceted ice roof, a crystal finial."""
    c = _canvas(UNIT_CELL)
    base = 46
    half = 14 + level
    top = base - (22 + level * 4)

    fill_polygon(c, [(24 - half, base), (24 + half, base),
                      (24 + half - 3, top + 5), (24 - half + 3, top + 5)], BLACK)
    c.vline(24 - half + 2, top + 7, base - top - 8, WHITE)
    c.vline(24 + half - 4, top + 7, base - top - 8, WHITE)
    _stone_block_rows(c, 24 - half, 24 + half, top + 7, base - 1, 2)
    c.fill_rect(23, top + 10, 2, 6, WHITE)
    c.fill_rect(24 - half + 1, base - 3, half * 2 - 2, 1, WHITE)

    # Ice roof: a white cone split by black facets.
    roof = [(24, top - 14), (24 + half - 1, top + 6), (24 - half + 1, top + 6)]
    fill_polygon(c, roof, BLACK)
    fill_polygon(c, [(24, top - 8), (24 + half - 6, top + 5),
                     (24 - half + 6, top + 5)], WHITE)
    c.line(24, top - 8, 24, top + 5, BLACK)
    c.line(24 - 8, top - 3, 24 - 3, top + 2, BLACK)
    c.line(24 + 8, top - 3, 24 + 3, top + 2, BLACK)
    c.line(24 - 12, top + 1, 24 - 7, top + 4, BLACK)

    # Crystal finial.
    head_y = top - 17
    c.polygon([(24, head_y - 6), (24 + 6, head_y), (24, head_y + 6),
               (24 - 6, head_y)], BLACK)
    c.polygon([(24, head_y - 3), (24 + 3, head_y), (24, head_y + 3),
               (24 - 3, head_y)], WHITE)
    c.line(24, head_y - 3, 24 + 3, head_y, BLACK)

    if branch == "nova":
        import math
        for angle in range(0, 360, 45):
            c.set(24 + int(math.cos(math.radians(angle)) * 12),
                  head_y + int(math.sin(math.radians(angle)) * 12), BLACK)
    elif branch == "ray":
        c.fill_rect(22, head_y - 18, 4, 16, BLACK)
        c.fill_rect(23, head_y - 16, 2, 12, WHITE)
    return c


def _siege_tower(level, branch=None):
    """Siege engine: a timber trebuchet with a spoked wheel and a stone."""
    c = _canvas(UNIT_CELL)
    ground = 46
    arm_y = 20 - level * 2

    # Spoked wheels, drawn first so the frame overlaps them.
    for wx in (13, 31):
        c.disc(wx, ground - 6, 8, BLACK)
        c.disc(wx, ground - 6, 6, WHITE)
        c.disc(wx, ground - 6, 2, BLACK)
        for angle in (20, 70, 110, 160):
            import math
            c.line(wx, ground - 6,
                   wx + int(math.cos(math.radians(angle)) * 6),
                   ground - 6 + int(math.sin(math.radians(angle)) * 6), BLACK)

    # Ground beam.
    c.fill_rect(6, ground - 12, 34, 5, BLACK)
    for plank in range(9, 38, 6):
        c.fill_rect(plank, ground - 11, 1, 3, WHITE)

    # A-frame legs and crossbar.
    c.thick_line(22, ground - 12, 13, arm_y + 8, 4, BLACK)
    c.thick_line(22, ground - 12, 31, arm_y + 8, 4, BLACK)
    c.thick_line(13, arm_y + 8, 31, arm_y + 8, 3, BLACK)
    c.thick_line(16, ground - 14, 26, ground - 16, 3, BLACK)

    # Throwing arm with the stone in the sling.
    c.thick_line(11, arm_y + 2, 38, arm_y - 8, 4, BLACK)
    c.thick_line(11, arm_y + 2, 38, arm_y - 8, 1, WHITE)
    c.disc(40, arm_y - 9, 6 + level, BLACK)
    c.disc(40, arm_y - 9, 4 + level, WHITE)
    c.disc(38, arm_y - 11, 2, BLACK)
    c.line(34, arm_y - 3, 40, arm_y - 9, WHITE)
    # Counterweight.
    c.disc(9, arm_y + 4, 5, BLACK)
    c.disc(9, arm_y + 4, 3, WHITE)
    c.line(9, arm_y + 9, 10, ground - 14, BLACK)

    if branch == "fire":
        c.disc(40, arm_y - 9, 2, BLACK)
        c.line(36, arm_y - 16, 45, arm_y - 18, BLACK)
        c.line(37, arm_y - 12, 44, arm_y - 11, BLACK)
    elif branch == "concussion":
        c.hline(35, arm_y - 17, 11, BLACK)
        c.hline(35, arm_y - 13, 11, BLACK)
    return c


# --------------------------------------------------------------- enemies
def _enemy_wolf(frame):
    """Frost wolf: a low black mass with a white belly and notched fur."""
    c = _canvas(PROP_CELL)
    bob = (0, -1, 0, 1)[frame % 4]
    body_y = 18 + bob
    stride = (2, 0, -2, 0)[frame % 4]

    # Tail sweeping back and down.
    fill_polygon(c, [(9, body_y - 1), (5, body_y - 4), (0, body_y + 2),
                     (4, body_y + 3), (9, body_y + 2)], BLACK)
    c.line(6, body_y, 2, body_y + 1, WHITE)

    # Body: a sloped back, a deep chest, a tucked waist.
    fill_polygon(c, [(8, body_y - 1), (11, body_y - 4), (16, body_y - 5),
                     (20, body_y - 2), (23, body_y + 1), (22, body_y + 5),
                     (10, body_y + 5), (8, body_y + 2)], BLACK)
    # White belly and flank carving.
    fill_polygon(c, [(12, body_y + 3), (19, body_y + 2), (22, body_y + 3),
                     (21, body_y + 5), (12, body_y + 5)], WHITE)
    c.line(12, body_y - 2, 15, body_y + 1, WHITE)
    c.line(17, body_y - 3, 19, body_y, WHITE)

    # Neck and head.
    fill_polygon(c, [(20, body_y - 2), (22, body_y - 8), (25, body_y - 10),
                     (29, body_y - 8), (31, body_y - 5), (28, body_y - 3),
                     (24, body_y - 4), (21, body_y - 1)], BLACK)
    c.fill_rect(27, body_y - 8, 2, 2, WHITE)        # eye
    c.fill_rect(29, body_y - 5, 2, 1, WHITE)        # tooth
    fill_polygon(c, [(22, body_y - 8), (22, body_y - 13), (25, body_y - 9)], BLACK)
    c.line(23, body_y - 10, 24, body_y - 9, WHITE)  # inner ear
    c.line(24, body_y - 6, 27, body_y - 6, WHITE)   # jaw

    # Legs with a knee and a paw.
    for x, extra in ((10, 0), (13, stride), (19, -stride), (22, 0)):
        length = 6 - abs(extra) // 2
        c.fill_rect(x, body_y + 4, 3, length, BLACK)
        c.fill_rect(x - 1 + extra // 2, body_y + 4 + length, 4, 2, BLACK)
        c.set(x + 1, body_y + 5, WHITE)

    # Snow along the spine.
    c.hline(12, body_y - 4, 5, WHITE)
    c.hline(15, body_y - 5, 4, WHITE)
    return c


def _enemy_raider(frame):
    """Raider: hooded, cloaked, carrying a broad axe."""
    c = _canvas(PROP_CELL)
    bob = (0, 1, 0, -1)[frame % 4]
    top = 5 + bob

    # Axe behind the figure: haft, then a broad blade with a lit edge.
    c.fill_rect(21, top + 3, 2, 16, BLACK)
    c.fill_rect(22, top + 6, 1, 10, WHITE)
    fill_polygon(c, [(20, top), (29, top + 2), (31, top + 7), (27, top + 12),
                     (20, top + 10)], BLACK)
    fill_polygon(c, [(22, top + 2), (27, top + 4), (28, top + 7), (25, top + 9),
                     (22, top + 8)], WHITE)

    # Cloak: a solid black mass, wide at the hem.
    fill_polygon(c, [(8, top + 18), (10, top + 10), (13, top + 6),
                     (19, top + 6), (21, top + 11), (22, top + 18)], BLACK)
    c.line(12, top + 9, 10, top + 17, WHITE)
    c.line(16, top + 8, 16, top + 17, WHITE)
    c.line(19, top + 10, 20, top + 17, WHITE)

    # Hood with a carved face opening and two eyes.
    fill_polygon(c, [(10, top + 9), (12, top + 2), (17, top + 1),
                     (20, top + 6), (19, top + 10)], BLACK)
    fill_polygon(c, [(13, top + 8), (14, top + 4), (18, top + 4),
                     (18, top + 8)], WHITE)
    c.fill_rect(14, top + 6, 2, 2, BLACK)
    c.fill_rect(17, top + 6, 1, 2, BLACK)
    c.line(12, top + 9, 19, top + 9, WHITE)

    # Belt and legs.
    c.fill_rect(8, top + 17, 14, 3, BLACK)
    c.fill_rect(9, top + 18, 12, 1, WHITE)
    c.fill_rect(11, top + 20, 4, 6, BLACK)
    c.fill_rect(17, top + 20, 4, 6, BLACK)
    c.fill_rect(12, top + 21, 1, 4, WHITE)
    c.fill_rect(18, top + 21, 1, 4, WHITE)
    return c


def _build_pad(frame):
    """Build spot: a stone disc with a breathing cross."""
    c = _canvas(PROP_CELL)
    c.disc(16, 16, 12, BLACK)
    c.disc(16, 16, 10, WHITE)
    c.disc(16, 16, 8, BLACK)
    c.disc(16, 16, 7, WHITE)
    if frame == 0:
        c.fill_rect(9, 15, 14, 3, BLACK)
        c.fill_rect(15, 9, 3, 14, BLACK)
    else:
        c.fill_rect(10, 15, 12, 2, BLACK)
        c.fill_rect(15, 10, 2, 12, BLACK)
    return c


def _projectile(frame):
    """Arrow in flight; the fletching flickers between frames."""
    c = _canvas(FX_CELL)
    c.fill_rect(3, 7, 10, 2, BLACK)
    c.fill_rect(4, 7, 8, 1, WHITE)
    if frame == 1:
        fill_polygon(c, [(1, 8), (5, 5), (5, 11)], BLACK)
    elif frame == 2:
        fill_polygon(c, [(0, 8), (4, 4), (4, 12)], BLACK)
    return c


def _spark(frame):
    """Impact spark: a growing ring with radiating flecks."""
    c = _canvas(FX_CELL)
    radius = 2 + frame
    c.disc(8, 8, radius, BLACK)
    c.disc(8, 8, max(0, radius - 2), WHITE)
    if frame > 3:
        import math
        for angle in range(0, 360, 60):
            c.set(8 + int(math.cos(math.radians(angle)) * radius),
                  8 + int(math.sin(math.radians(angle)) * radius), BLACK)
    return c


def _icon_heart():
    c = _canvas(FX_CELL)
    c.disc(6, 6, 3, BLACK)
    c.disc(10, 6, 3, BLACK)
    fill_polygon(c, [(3, 7), (13, 7), (8, 13)], BLACK)
    return c


def _icon_coin():
    c = _canvas(FX_CELL)
    c.disc(8, 8, 6, BLACK)
    c.disc(8, 8, 4, WHITE)
    c.fill_rect(7, 5, 2, 6, BLACK)
    return c


def _icon_score():
    """Score glyph: three ascending bars, so it never reads as a coin."""
    c = _canvas(FX_CELL)
    c.fill_rect(2, 9, 4, 5, BLACK)
    c.fill_rect(6, 6, 4, 8, BLACK)
    c.fill_rect(10, 2, 4, 12, BLACK)
    c.fill_rect(3, 10, 2, 3, WHITE)
    c.fill_rect(7, 7, 2, 6, WHITE)
    c.fill_rect(11, 3, 2, 10, WHITE)
    return c


def _icon_flag():
    c = _canvas(FX_CELL)
    c.fill_rect(3, 2, 2, 12, BLACK)
    fill_polygon(c, [(5, 3), (13, 5), (5, 9)], BLACK)
    return c


def _build_grid(cell, entries, columns=8):
    """Lay sprites into a uniform grid of source-sized cells.

    The source cell is deliberately four pixels smaller than the atlas cell:
    the packer reserves output_cell - 4 for its crop, so matching the source
    to that size keeps the thumbnail step from resampling the art.
    """
    rows = (len(entries) + columns - 1) // columns
    canvas = Canvas(columns * cell, rows * cell)
    configs = []
    for index, (name, sprite) in enumerate(entries):
        col, row = index % columns, index // columns
        canvas.blit(sprite, col * cell, row * cell)
        configs.append({"name": name})
    return canvas, configs


def build_units():
    entries = []
    for level in (1, 2, 3):
        entries.append((f"tower_guard_lv{level}", _guard_tower(level)))
    for level in (1, 2, 3):
        entries.append((f"tower_frost_lv{level}", _frost_tower(level)))
    for level in (1, 2, 3):
        entries.append((f"tower_siege_lv{level}", _siege_tower(level)))
    entries.append(("tower_guard_lv3_pierce", _guard_tower(3, "pierce")))
    entries.append(("tower_guard_lv3_rapid", _guard_tower(3, "rapid")))
    entries.append(("tower_frost_lv3_nova", _frost_tower(3, "nova")))
    entries.append(("tower_frost_lv3_ray", _frost_tower(3, "ray")))
    entries.append(("tower_siege_lv3_fire", _siege_tower(3, "fire")))
    entries.append(("tower_siege_lv3_concussion", _siege_tower(3, "concussion")))
    return _build_grid(UNIT_CELL, entries)


def build_props():
    entries = []
    for frame in range(4):
        entries.append((f"enemy_wolf_{frame}", _enemy_wolf(frame)))
    for frame in range(4):
        entries.append((f"enemy_raider_{frame}", _enemy_raider(frame)))
    entries.append(("build_pad_0", _build_pad(0)))
    entries.append(("build_pad_1", _build_pad(1)))
    return _build_grid(PROP_CELL, entries)


def build_fx():
    entries = []
    for frame in range(3):
        entries.append((f"projectile_0_{frame}", _projectile(frame)))
    for frame in range(6):
        entries.append((f"spark_{frame}", _spark(frame)))
    entries.append(("icon_heart", _icon_heart()))
    entries.append(("icon_coin", _icon_coin()))
    entries.append(("icon_flag", _icon_flag()))
    entries.append(("icon_score", _icon_score()))
    return _build_grid(FX_CELL, entries)


PROPS_ANIMATIONS = [
    {"name": "wolf_walk",
     "frames": ["enemy_wolf_0", "enemy_wolf_1", "enemy_wolf_2", "enemy_wolf_3"],
     "frame_ticks": 5},
    {"name": "raider_walk",
     "frames": ["enemy_raider_0", "enemy_raider_1", "enemy_raider_2",
                "enemy_raider_3"],
     "frame_ticks": 5},
    {"name": "pad_pulse", "frames": ["build_pad_0", "build_pad_1"],
     "frame_ticks": 15},
]

FX_ANIMATIONS = [
    {"name": "spark", "frames": [f"spark_{i}" for i in range(6)], "frame_ticks": 2},
]

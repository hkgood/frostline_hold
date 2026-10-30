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
HERO_CELL = 40
HUSK_CELL = 40
TITAN_CELL = 64
FX_CELL = 16

UNIT_ATLAS_CELL = UNIT_CELL + 4
PROP_ATLAS_CELL = PROP_CELL + 4
HERO_ATLAS_CELL = HERO_CELL + 4
HUSK_ATLAS_CELL = HUSK_CELL + 4
TITAN_ATLAS_CELL = TITAN_CELL + 4
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
    elif branch == "marksman":
        # a scoped single shot: one long limb and a sighting tube
        c.thick_line(24, pivot_y, 24 - 11, pivot_y - 7, 3, BLACK)
        c.thick_line(24, pivot_y, 24 + 11, pivot_y - 7, 3, BLACK)
        c.line(24 - 11, pivot_y - 7, 24, pivot_y - 2, WHITE)
        c.line(24 + 11, pivot_y - 7, 24, pivot_y - 2, WHITE)
        c.fill_rect(21, pivot_y - 17, 6, 4, BLACK)
        c.fill_rect(22, pivot_y - 16, 4, 2, WHITE)
        c.fill_rect(23, pivot_y - 14, 2, 17, BLACK)
        c.fill_rect(24, pivot_y - 12, 1, 13, WHITE)
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
    # A wedge blade with a lit edge, not a blob.
    fill_polygon(c, [(20, top + 1), (29, top + 3), (29, top + 11),
                     (20, top + 9)], BLACK)
    c.fill_rect(21, top + 3, 7, 1, WHITE)
    c.fill_rect(21, top + 7, 6, 1, WHITE)

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


def _enemy_skitter(frame):
    """Ice skitter: a low six-legged crawler that scuttles."""
    c = _canvas(PROP_CELL)
    bob = (0, -1, 0, 1)[frame % 4]
    body_y = 17 + bob
    stride = (2, 0, -2, 0)[frame % 4]

    # Six legs, drawn as thin white strokes off a black body.
    for index, (x, spread) in enumerate(((6, -1), (9, 0), (13, 1), (17, 0),
                                         (21, -1), (24, 0))):
        reach = stride if index % 2 == 0 else -stride
        c.line(x, body_y + 2, x + spread * 4, body_y + 9 + reach, BLACK)
        c.line(x + spread * 4, body_y + 9 + reach,
               x + spread * 6, body_y + 13 + reach, BLACK)

    # Body: a dark carapace with a pale seam.
    fill_polygon(c, [(7, body_y - 2), (13, body_y - 5), (21, body_y - 4),
                     (26, body_y), (21, body_y + 4), (12, body_y + 4)], BLACK)
    c.line(12, body_y - 1, 20, body_y - 2, WHITE)
    c.line(13, body_y + 1, 19, body_y, WHITE)
    # Eyes and mandibles.
    c.fill_rect(23, body_y - 2, 2, 2, WHITE)
    c.fill_rect(26, body_y - 1, 2, 1, WHITE)
    c.line(27, body_y + 1, 30, body_y + 2, BLACK)
    c.line(27, body_y + 2, 30, body_y + 4, BLACK)
    return c


def _enemy_husk(frame):
    """Frost husk: a broad armoured brute that barely hurries."""
    c = _canvas(40)
    bob = (0, -1, 0, 1)[frame % 4]
    base = 37 + bob
    stride = (2, 0, -2, 0)[frame % 4]

    # Legs.
    for x, extra in ((10, 0), (15, stride), (24, -stride), (29, 0)):
        c.fill_rect(x, base - 8, 5, 9, BLACK)
        c.fill_rect(x - 1 + extra // 2, base + 1, 7, 3, BLACK)
        c.fill_rect(x + 1, base - 7, 1, 7, WHITE)

    # Torso: a heavy black mass with plate seams.
    fill_polygon(c, [(6, base - 8), (34, base - 8), (31, base - 24),
                     (24, base - 28), (16, base - 28), (9, base - 24)], BLACK)
    c.hline(9, base - 20, 22, WHITE)
    c.hline(11, base - 14, 18, WHITE)
    c.vline(20, base - 26, 16, WHITE)
    # Pauldrons.
    c.fill_rect(2, base - 26, 10, 8, BLACK)
    c.fill_rect(28, base - 26, 10, 8, BLACK)
    c.fill_rect(4, base - 24, 6, 1, WHITE)
    c.fill_rect(30, base - 24, 6, 1, WHITE)
    # A small head sunk between the shoulders.
    fill_polygon(c, [(15, base - 28), (25, base - 28), (23, base - 34),
                     (17, base - 34)], BLACK)
    c.fill_rect(18, base - 31, 2, 2, WHITE)
    c.fill_rect(21, base - 31, 2, 2, WHITE)
    return c


def _enemy_shaman(frame):
    """Shaman: a hooded figure dragging a rune staff."""
    c = _canvas(PROP_CELL)
    bob = (0, 1, 0, -1)[frame % 4]
    top = 5 + bob

    # Staff with a carved head.
    c.fill_rect(25, top - 1, 2, 20, BLACK)
    c.fill_rect(26, top + 1, 1, 16, WHITE)
    c.disc(26, top - 3, 4, BLACK)
    c.disc(26, top - 3, 2, WHITE)
    c.fill_rect(25, top - 6, 2, 2, BLACK)

    # Cloak.
    fill_polygon(c, [(7, top + 18), (10, top + 9), (14, top + 6),
                     (19, top + 7), (21, top + 14), (21, top + 18)], BLACK)
    c.line(12, top + 10, 10, top + 17, WHITE)
    c.line(16, top + 9, 16, top + 17, WHITE)
    # Hood and the dark under it.
    fill_polygon(c, [(10, top + 9), (12, top + 2), (17, top + 2),
                     (20, top + 7), (19, top + 10)], BLACK)
    c.fill_rect(13, top + 5, 2, 2, WHITE)
    c.fill_rect(16, top + 5, 2, 2, WHITE)
    # Arm reaching for the staff.
    c.thick_line(18, top + 11, 25, top + 8, 2, BLACK)
    return c


def _enemy_banner(frame):
    """Banner man: the hardening comes from the flag over his head."""
    c = _canvas(PROP_CELL)
    bob = (0, 1, 0, -1)[frame % 4]
    top = 6 + bob

    # Pole and flag.
    c.fill_rect(24, 1, 2, 24, BLACK)
    fill_polygon(c, [(26, 2), (31, 4), (28, 9), (26, 8)], BLACK)
    c.fill_rect(27, 3, 2, 3, WHITE)
    c.fill_rect(25, 1, 4, 2, BLACK)

    # Body.
    fill_polygon(c, [(7, top + 17), (9, top + 9), (13, top + 5),
                     (18, top + 6), (20, top + 17)], BLACK)
    c.line(11, top + 10, 10, top + 16, WHITE)
    c.line(15, top + 9, 15, top + 16, WHITE)
    # Hood.
    fill_polygon(c, [(10, top + 8), (12, top + 1), (17, top + 2),
                     (19, top + 7)], BLACK)
    c.fill_rect(13, top + 4, 2, 2, WHITE)
    c.fill_rect(16, top + 4, 1, 2, WHITE)
    # Arm on the pole.
    c.thick_line(18, top + 10, 25, top + 8, 2, BLACK)
    return c


def _enemy_sap(frame):
    """Sap: a runner with a lit charge on its back."""
    c = _canvas(PROP_CELL)
    bob = (0, -1, 0, 1)[frame % 4]
    body_y = 17 + bob
    stride = (3, 0, -3, 0)[frame % 4]

    # The charge on its back: a black ball with a white fuse.
    c.disc(13, body_y - 7, 6, BLACK)
    c.disc(13, body_y - 7, 4, WHITE)
    c.fill_rect(12, body_y - 8, 3, 3, BLACK)
    c.line(16, body_y - 11, 20, body_y - 14, BLACK)
    c.set(21, body_y - 15, BLACK)
    c.set(22, body_y - 16, BLACK)

    # Hunched body and head.
    fill_polygon(c, [(10, body_y - 2), (16, body_y - 4), (22, body_y - 2),
                     (24, body_y + 4), (12, body_y + 5)], BLACK)
    fill_polygon(c, [(20, body_y - 6), (25, body_y - 7), (28, body_y - 3),
                     (25, body_y - 1), (21, body_y - 2)], BLACK)
    c.fill_rect(25, body_y - 6, 2, 2, WHITE)
    c.line(14, body_y - 1, 18, body_y - 2, WHITE)

    # Legs.
    for x, extra in ((12, 0), (16, stride), (21, -stride)):
        length = 6 - abs(extra) // 2
        c.fill_rect(x, body_y + 4, 3, length, BLACK)
        c.fill_rect(x - 1 + extra // 2, body_y + 4 + length, 4, 2, BLACK)
    return c


def _enemy_titan(frame):
    """The frost titan: one creature, a heavy head and a cracked carapace."""
    c = _canvas(64)
    bob = (0, -1, 0, 1)[frame % 4]
    base = 60 + bob
    stride = (2, 0, -2, 0)[frame % 4]

    # Legs.
    for x, extra in ((14, 0), (22, stride), (40, -stride), (48, 0)):
        c.fill_rect(x, base - 14, 9, 15, BLACK)
        c.fill_rect(x - 2 + extra // 2, base + 1, 13, 4, BLACK)
        c.fill_rect(x + 2, base - 13, 2, 12, WHITE)

    # Torso: a heavy black mass, narrower at the top.
    fill_polygon(c, [(9, base - 12), (55, base - 12), (48, base - 36),
                     (38, base - 42), (26, base - 42), (16, base - 36)], BLACK)
    # Fractures in the ice.
    c.line(20, base - 16, 26, base - 26, WHITE)
    c.line(26, base - 26, 22, base - 33, WHITE)
    c.line(26, base - 26, 33, base - 29, WHITE)
    c.line(40, base - 18, 45, base - 28, WHITE)
    c.line(45, base - 28, 41, base - 37, WHITE)

    # Shoulders that sit on the torso rather than beside it.
    fill_polygon(c, [(9, base - 12), (22, base - 14), (24, base - 30),
                     (14, base - 34)], BLACK)
    fill_polygon(c, [(55, base - 12), (42, base - 14), (40, base - 30),
                     (50, base - 34)], BLACK)
    c.line(12, base - 16, 21, base - 18, WHITE)
    c.line(52, base - 16, 43, base - 18, WHITE)

    # One big head, set straight on the shoulders.
    fill_polygon(c, [(23, base - 40), (41, base - 40), (39, base - 54),
                     (25, base - 54)], BLACK)
    c.fill_rect(26, base - 50, 4, 4, WHITE)
    c.fill_rect(35, base - 50, 4, 4, WHITE)
    c.hline(29, base - 45, 7, WHITE)
    # Horns of ice.
    c.polygon([(24, base - 52), (16, base - 63), (28, base - 56)], BLACK)
    c.polygon([(40, base - 52), (48, base - 63), (36, base - 56)], BLACK)

    # Arms hanging at the sides.
    for x, extra in ((8, stride), (50, -stride)):
        c.thick_line(x + 4, base - 30, x + extra, base - 14, 7, BLACK)
        c.thick_line(x + 4, base - 30, x + extra, base - 14, 2, WHITE)
    return c


def _hero_warden(frame):
    """The frost warden: a shield bearer who holds the road."""
    c = _canvas(40)
    bob = (0, -1, 0, 1)[frame % 4]
    base = 37 + bob
    stride = (2, 0, -2, 0)[frame % 4]

    # Legs.
    for x, extra in ((12, 0), (17, stride), (24, -stride), (28, 0)):
        c.fill_rect(x, base - 11, 5, 11, BLACK)
        c.fill_rect(x - 1 + extra // 2, base, 7, 3, BLACK)
        c.fill_rect(x + 1, base - 10, 1, 9, WHITE)

    # Cloak and torso.
    fill_polygon(c, [(9, base - 4), (11, base - 22), (15, base - 28),
                     (26, base - 28), (30, base - 22), (31, base - 4)], BLACK)
    c.line(16, base - 26, 15, base - 6, WHITE)
    c.line(22, base - 26, 23, base - 6, WHITE)
    c.fill_rect(14, base - 16, 13, 2, WHITE)

    # A big shield on the near arm.
    fill_polygon(c, [(1, base - 26), (11, base - 28), (11, base - 8),
                     (1, base - 6)], BLACK)
    c.line(3, base - 22, 9, base - 23, WHITE)
    c.line(3, base - 17, 9, base - 18, WHITE)
    c.fill_rect(5, base - 20, 2, 7, WHITE)

    # Hooded head with a lit face.
    fill_polygon(c, [(16, base - 28), (18, base - 36), (25, base - 36),
                     (27, base - 28)], BLACK)
    fill_polygon(c, [(19, base - 31), (20, base - 34), (24, base - 34),
                     (24, base - 31)], WHITE)
    c.fill_rect(21, base - 33, 1, 1, BLACK)

    # The maul, raised.
    c.fill_rect(33, base - 34, 3, 26, BLACK)
    c.fill_rect(34, base - 32, 1, 20, WHITE)
    c.fill_rect(29, base - 38, 11, 7, BLACK)
    c.fill_rect(31, base - 36, 7, 1, WHITE)
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


def _icon_star():
    """Star glyph for the end panel rating."""
    c = _canvas(FX_CELL)
    points = [(8, 1), (10, 6), (15, 6), (11, 9), (13, 14),
              (8, 11), (3, 14), (5, 9), (1, 6), (6, 6)]
    fill_polygon(c, points, BLACK)
    return c


def _icon_nova():
    """Frost nova glyph: a burst of shards around a core."""
    c = _canvas(FX_CELL)
    c.disc(8, 8, 3, BLACK)
    c.disc(8, 8, 1, WHITE)
    import math
    for angle in range(0, 360, 45):
        x = 8 + int(math.cos(math.radians(angle)) * 6)
        y = 8 + int(math.sin(math.radians(angle)) * 6)
        c.set(x, y, BLACK)
        c.set(8 + int(math.cos(math.radians(angle)) * 7),
              8 + int(math.sin(math.radians(angle)) * 7), BLACK)
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

    `columns` has to be the same number the atlas manifest declares, because
    the packer cuts the source sheet apart by that count.
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
    """Four frames per tower kind: level 1, level 2, and the two level 3
    branches. The order matches the view's [kind][variant] table."""
    entries = []
    entries.append(("tower_guard_lv1", _guard_tower(1)))
    entries.append(("tower_guard_lv2", _guard_tower(2)))
    entries.append(("tower_guard_lv3a", _guard_tower(3, "pierce")))
    entries.append(("tower_guard_lv3b", _guard_tower(3, "marksman")))
    entries.append(("tower_frost_lv1", _frost_tower(1)))
    entries.append(("tower_frost_lv2", _frost_tower(2)))
    entries.append(("tower_frost_lv3a", _frost_tower(3, "nova")))
    entries.append(("tower_frost_lv3b", _frost_tower(3, "ray")))
    entries.append(("tower_siege_lv1", _siege_tower(1)))
    entries.append(("tower_siege_lv2", _siege_tower(2)))
    entries.append(("tower_siege_lv3a", _siege_tower(3, "fire")))
    entries.append(("tower_siege_lv3b", _siege_tower(3, "concussion")))
    return _build_grid(UNIT_CELL, entries)


# The roster, in the order the level data lists them. The husk and the titan
# are bigger than the rank and file, so they live in their own sheets: every
# cell in a sheet has to be the same size or the packer rescales the art.
SMALL_ENEMY_BUILDERS = [
    ("wolf", _enemy_wolf),
    ("raider", _enemy_raider),
    ("skitter", _enemy_skitter),
    ("shaman", _enemy_shaman),
    ("banner", _enemy_banner),
    ("sap", _enemy_sap),
]
ENEMY_BUILDERS = SMALL_ENEMY_BUILDERS + [("husk", _enemy_husk),
                                         ("titan", _enemy_titan)]


def build_props():
    """The rank and file, all on one 32 pixel grid."""
    entries = []
    for name, builder in SMALL_ENEMY_BUILDERS:
        for frame in range(4):
            entries.append((f"enemy_{name}_{frame}", builder(frame)))
    entries.append(("build_pad_0", _build_pad(0)))
    entries.append(("build_pad_1", _build_pad(1)))
    return _build_grid(PROP_CELL, entries)


def build_props_husk():
    """The armoured brute, on its own 40 pixel grid."""
    entries = [(f"enemy_husk_{frame}", _enemy_husk(frame)) for frame in range(4)]
    return _build_grid(HUSK_CELL, entries, columns=4)


def build_props_titan():
    """The boss, on its own 64 pixel grid."""
    entries = [(f"enemy_titan_{frame}", _enemy_titan(frame))
               for frame in range(4)]
    return _build_grid(TITAN_CELL, entries, columns=4)


def build_heroes():
    """The hero sheet, on its own 40 pixel grid."""
    entries = [("hero_warden_0", _hero_warden(0)),
               ("hero_warden_1", _hero_warden(1)),
               ("hero_warden_2", _hero_warden(2)),
               ("hero_warden_3", _hero_warden(3))]
    return _build_grid(HERO_CELL, entries, columns=4)


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
    entries.append(("icon_nova", _icon_nova()))
    entries.append(("icon_star", _icon_star()))
    return _build_grid(FX_CELL, entries)


PROPS_ANIMATIONS = [
    {"name": f"{name}_walk",
     "frames": [f"enemy_{name}_{i}" for i in range(4)],
     "frame_ticks": 5}
    for name, _builder in SMALL_ENEMY_BUILDERS
] + [
    {"name": "pad_pulse", "frames": ["build_pad_0", "build_pad_1"],
     "frame_ticks": 15},
]

HUSK_ANIMATIONS = [
    {"name": "husk_walk",
     "frames": [f"enemy_husk_{i}" for i in range(4)],
     "frame_ticks": 8},
]

TITAN_ANIMATIONS = [
    {"name": "titan_walk",
     "frames": [f"enemy_titan_{i}" for i in range(4)],
     "frame_ticks": 10},
]

FX_ANIMATIONS = [
    {"name": "spark", "frames": [f"spark_{i}" for i in range(6)], "frame_ticks": 2},
]

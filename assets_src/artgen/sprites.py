"""Procedural 1-bit sprite sheets for Frostline Hold.

Every sprite is drawn at its final pixel size, so the packer never rescales
anything and the two-colour rule survives. Each sheet is a uniform grid; the
atlas cell is four pixels larger than the source cell so the packer's
thumbnail step never triggers a LANCZOS resample.

Drawing rules learned from the reference art:
  * every form carries a 2px black silhouette, so shapes read at 2.16 inch
  * interiors stay white and carry the detail, never 1px hairlines
  * no feature thinner than 2px survives the 0.081mm pixel pitch
"""
from artgen.canvas import BLACK, WHITE, Canvas

UNIT_CELL = 48
PROP_CELL = 32
FX_CELL = 16

# The packer reserves output_cell - 4 for the source crop; matching the source
# cell to that size keeps the sprite pixel-exact.
UNIT_ATLAS_CELL = UNIT_CELL + 4
PROP_ATLAS_CELL = PROP_CELL + 4
FX_ATLAS_CELL = FX_CELL + 4


def _canvas(size):
    return Canvas(size, size)


def _plinth(canvas, cx, cy, radius, blocks=3):
    """Stone footing every tower stands on."""
    canvas.ellipse(cx, cy, radius, radius // 3, BLACK)
    canvas.ellipse(cx, cy - 1, radius - 2, max(1, radius // 3 - 2), WHITE)
    for index in range(1, blocks + 1):
        offset = int(radius * index / (blocks + 1))
        canvas.fill_rect(cx - offset, cy + 1, offset * 2, 1, BLACK)


# ---------------------------------------------------------------- towers
def _guard_tower(level, branch=None):
    """Ballista tower: wide stone shaft, crenellated top, small crossbow."""
    c = _canvas(UNIT_CELL)
    top = 20 - level * 3
    half_bot = 12 + level
    half_top = 10 + level

    _plinth(c, 24, 42, 18)

    # Stone shaft: taller than wide so it reads as a tower, not a table.
    c.polygon([(24 - half_bot, 43), (24 + half_bot, 43),
               (24 + half_top, top + 5), (24 - half_top, top + 5)], BLACK)
    c.polygon([(24 - half_bot + 2, 41), (24 + half_bot - 2, 41),
               (24 + half_top - 2, top + 7), (24 - half_top + 2, top + 7)],
              WHITE)
    c.hline(24 - half_bot + 3, 33, (half_bot - 3) * 2, BLACK)
    c.fill_rect(22, top + 13, 5, 10, BLACK)          # arrow slit
    c.fill_rect(24 - half_top + 2, top + 9, 4, 8, BLACK)

    # Crenellated parapet.
    c.fill_rect(24 - half_top - 1, top, half_top * 2 + 2, 4, BLACK)
    for x in range(24 - half_top, 24 + half_top - 4, 7):
        c.fill_rect(x, top - 4, 5, 5, BLACK)

    # Crossbow seen from the front: two angled limbs, a string and a bolt.
    bow_y = top - 6
    c.thick_line(24, bow_y + 5, 24 - 7, bow_y - 3, 3, BLACK)
    c.thick_line(24, bow_y + 5, 24 + 7, bow_y - 3, 3, BLACK)
    c.line(24 - 7, bow_y - 3, 24, bow_y + 3, BLACK)
    c.line(24 + 7, bow_y - 3, 24, bow_y + 3, BLACK)
    c.fill_rect(23, bow_y - 9, 2, 11, BLACK)        # bolt
    c.fill_rect(24, bow_y - 7, 1, 8, WHITE)
    if branch == "pierce":
        c.polygon([(22, bow_y - 15), (27, bow_y - 15), (24, bow_y - 21)], BLACK)
        c.fill_rect(23, bow_y - 13, 2, 7, WHITE)
    elif branch == "rapid":
        c.disc(24, bow_y + 9, 4, BLACK)
        c.disc(24, bow_y + 9, 2, WHITE)
    elif level >= 3:
        c.fill_rect(24 + half_top - 1, top - 3, 3, 16, BLACK)   # banner
        c.fill_rect(24 + half_top - 1, top - 3, 3, 9, WHITE)
    return c


def _frost_tower(level, branch=None):
    """Frost spire: stone shaft, faceted ice roof, crystal finial."""
    c = _canvas(UNIT_CELL)
    top = 18 - level * 3
    half = 12 + level

    _plinth(c, 24, 42, 18)

    # Stone shaft with visible courses.
    c.polygon([(24 - half, 43), (24 + half, 43), (24 + half - 4, 22),
               (24 - half + 4, 22)], BLACK)
    c.polygon([(24 - half + 2, 41), (24 + half - 2, 41), (24 + half - 6, 24),
               (24 - half + 6, 24)], WHITE)
    c.hline(24 - half + 4, 34, (half - 4) * 2, BLACK)
    c.fill_rect(22, 28, 5, 10, BLACK)                # arrow slit

    # Faceted ice roof.
    c.triangle(24, top, 24 - half + 6, 23, 24 + half - 6, 23, BLACK)
    c.triangle(24, top + 4, 24 - half + 9, 23, 24 + half - 9, 23, WHITE)
    c.line(24, top + 3, 24, 23, BLACK)
    c.line(24 - 5, top + 10, 24 - 2, top + 10, BLACK)

    # Crystal finial seated on the roof tip.
    head_y = top + 1
    c.polygon([(24, head_y - 9), (24 + 7, head_y), (24, head_y + 9),
               (24 - 7, head_y)], BLACK)
    c.polygon([(24, head_y - 6), (24 + 4, head_y), (24, head_y + 6),
               (24 - 4, head_y)], WHITE)
    c.line(24, head_y - 6, 24 + 4, head_y, BLACK)
    c.line(24, head_y - 6, 24 - 4, head_y, BLACK)

    if branch == "nova":
        import math
        for angle in range(0, 360, 45):
            c.set(24 + int(math.cos(math.radians(angle)) * 14),
                  head_y + int(math.sin(math.radians(angle)) * 14), BLACK)
    elif branch == "ray":
        c.fill_rect(22, head_y - 20, 4, 20, BLACK)
        c.fill_rect(23, head_y - 18, 2, 16, WHITE)
    return c


def _siege_tower(level, branch=None):
    """Catapult: heavy wheeled platform, A-frame, throwing arm and boulder."""
    c = _canvas(UNIT_CELL)
    arm_y = 28 - level * 3
    half = 13 + level

    # Platform and wheels.
    c.fill_rect(24 - half, 34, half * 2, 9, BLACK)
    c.fill_rect(24 - half + 2, 36, half * 2 - 4, 5, WHITE)
    c.disc(24 - half + 4, 44, 5, BLACK)
    c.disc(24 - half + 4, 44, 2, WHITE)
    c.disc(24 + half - 4, 44, 5, BLACK)
    c.disc(24 + half - 4, 44, 2, WHITE)

    # A-frame.
    c.polygon([(24 - 9, 34), (24 + 9, 34), (24 + 3, arm_y + 6),
               (24 - 3, arm_y + 6)], BLACK)
    c.polygon([(24 - 6, 33), (24 + 6, 33), (24 + 2, arm_y + 7),
               (24 - 2, arm_y + 7)], WHITE)
    c.hline(24 - 6, 30, 13, BLACK)

    # Throwing arm with a boulder at the tip.
    c.thick_line(24, arm_y + 8, 24 + 12, arm_y - 6, 5, BLACK)
    c.thick_line(24, arm_y + 8, 24 + 12, arm_y - 6, 2, WHITE)
    c.disc(24 + 13, arm_y - 7, 6 + level, BLACK)
    c.disc(24 + 13, arm_y - 7, 4 + level, WHITE)
    if branch == "fire":
        c.disc(24 + 13, arm_y - 7, 2, BLACK)
        c.line(24 + 9, arm_y - 12, 24 + 17, arm_y - 14, BLACK)
    elif branch == "concussion":
        c.hline(24 + 8, arm_y - 14, 11, BLACK)
        c.hline(24 + 8, arm_y - 10, 11, BLACK)
    return c


# --------------------------------------------------------------- enemies
def _enemy_wolf(frame):
    """Frost wolf in side view: low body, pointed ears, snow on the back."""
    c = _canvas(PROP_CELL)
    bob = (0, -1, 0, 1)[frame % 4]
    body_y = 19 + bob
    stride = (2, 0, -2, 0)[frame % 4]

    # Body.
    c.ellipse(15, body_y, 10, 6, BLACK)
    c.ellipse(15, body_y - 1, 8, 3, WHITE)
    # Head with snout and ear.
    c.disc(24, body_y - 4, 6, BLACK)
    c.disc(24, body_y - 4, 4, WHITE)
    c.polygon([(27, body_y - 5), (31, body_y - 2), (27, body_y + 1)], BLACK)
    c.polygon([(20, body_y - 8), (21, body_y - 12), (25, body_y - 8)], BLACK)
    c.fill_rect(25, body_y - 6, 2, 2, BLACK)          # eye
    # Tail.
    c.polygon([(6, body_y - 2), (1, body_y - 7), (6, body_y + 2)], BLACK)
    # Legs: two pairs offset by the stride.
    for x, extra in ((9, 0), (13, stride), (19, -stride), (22, 0)):
        c.fill_rect(x, body_y + 4, 3, 6 - abs(extra) // 2 + 2, BLACK)
    # Snow on the back.
    c.hline(9, body_y - 5, 9, WHITE)
    c.hline(12, body_y - 6, 5, WHITE)
    return c


def _enemy_raider(frame):
    """Armoured raider: hooded figure with a heavy two-handed axe."""
    c = _canvas(PROP_CELL)
    bob = (0, 1, 0, -1)[frame % 4]
    top = 5 + bob

    # Hood.
    c.disc(14, top + 3, 5, BLACK)
    c.disc(14, top + 3, 3, WHITE)
    c.fill_rect(13, top + 2, 2, 2, BLACK)             # eye slit
    # Shoulders and cloak.
    c.polygon([(8, top + 8), (20, top + 8), (22, top + 17), (6, top + 17)],
              BLACK)
    c.polygon([(10, top + 9), (18, top + 9), (19, top + 15), (9, top + 15)],
              WHITE)
    # Torso and belt.
    c.fill_rect(8, top + 16, 12, 7, BLACK)
    c.fill_rect(10, top + 17, 8, 5, WHITE)
    c.fill_rect(9, top + 20, 10, 2, BLACK)
    # Legs.
    c.fill_rect(9, top + 23, 4, 6, BLACK)
    c.fill_rect(15, top + 23, 4, 6, BLACK)
    # Axe: haft plus a broad blade.
    c.fill_rect(21, top + 4, 2, 15, BLACK)
    c.polygon([(20, top + 2), (27, top + 5), (27, top + 12), (20, top + 11)],
              BLACK)
    c.polygon([(21, top + 4), (25, top + 6), (25, top + 10), (21, top + 9)],
              WHITE)
    return c


def _build_pad(frame):
    """Build spot: a stone disc with a breathing cross."""
    c = _canvas(PROP_CELL)
    c.disc(16, 16, 12, BLACK)
    c.disc(16, 16, 10, WHITE)
    c.disc(16, 16, 8, BLACK)
    c.disc(16, 16, 7, WHITE)
    if frame == 0:
        c.fill_rect(10, 15, 12, 3, BLACK)
        c.fill_rect(15, 10, 3, 13, BLACK)
    else:
        c.fill_rect(11, 15, 10, 2, BLACK)
        c.fill_rect(15, 11, 2, 12, BLACK)
    return c


def _projectile(frame):
    """Arrow in flight; the tail flickers between frames."""
    c = _canvas(FX_CELL)
    c.fill_rect(3, 7, 10, 2, BLACK)
    c.fill_rect(4, 7, 8, 1, WHITE)
    if frame == 1:
        c.polygon([(1, 8), (5, 5), (5, 11)], BLACK)
    elif frame == 2:
        c.polygon([(0, 8), (4, 4), (4, 12)], BLACK)
    return c


def _spark(frame):
    """Impact spark: a growing ring."""
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
    c.polygon([(3, 7), (13, 7), (8, 13)], BLACK)
    return c


def _icon_coin():
    c = _canvas(FX_CELL)
    c.disc(8, 8, 6, BLACK)
    c.disc(8, 8, 4, WHITE)
    c.fill_rect(7, 5, 2, 6, BLACK)
    return c


def _icon_flag():
    c = _canvas(FX_CELL)
    c.fill_rect(3, 2, 2, 12, BLACK)
    c.polygon([(5, 3), (13, 5), (5, 9)], BLACK)
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

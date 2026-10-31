"""The towers: bodies, the frames they move through, and the parts the view
moves on its own.

Every tower has four looks (artgen/tower_rig.py VARIANTS). Most are one
still body. Three of them move as they fight:

  siege   the catapult throws: five poses of its arm, facing either way
  guard   the keep stays still; its ballista is a separate head that turns
          to eight directions and loads, looses and draws again
  ember   the fire in the kiln's cauldron flickers, and surges as a pot
          is thrown

The parts sheet holds what the view places and moves by itself: the
ballista heads, the spare fire pots, the stones and shards that orbit a
tower, and the open door of a barracks. Every point those parts hang on
comes from artgen/tower_rig.py, the same table the model fires from.
"""
import math

from artgen import shade as S
from artgen import tower_rig as rig
from artgen.art_kit import (arch_door, crystal, phase, rect, snow_cap,
                            stone_plinth, wheel)
from artgen.canvas import WHITE

CELL = rig.CELL
CX = rig.CX
FOOT_ROW = rig.FOOT_ROW
ATLAS_CELL = CELL + 4

PART_CELL = 32
PART_ATLAS_CELL = PART_CELL + 4
PART_CENTER = PART_CELL // 2
# A barracks' open door is drawn with its bottom centre on this pixel.
DOOR_ANCHOR = (PART_CENTER, PART_CENTER + 8)

VARIANT_NAMES = ("lv1", "lv2", "lv3a", "lv3b")
EMBER_FLICKER_FRAMES = 4
FACINGS = (("r", 1), ("l", -1))


def _tower_canvas():
    return S.ToneCanvas(CELL, CELL)


def _part_canvas():
    return S.ToneCanvas(PART_CELL, PART_CELL)


# ----------------------------------------------------------------------
# Guard: a keep with a turning ballista
# ----------------------------------------------------------------------


def _guard_body(variant):
    """Round stone keep with a wooden fighting gallery; the ballista that
    turns on top of it is a part of its own (guard_head)."""
    level, branch = rig.VARIANTS[variant]
    tc = _tower_canvas()
    cx, base = CX, FOOT_ROW
    r, top = rig.guard_body(level)
    height = base - top
    S.shadow_ellipse(tc, cx + 6, base + 1, r + 9, 4.5, 0.5)

    stone_plinth(tc, cx, base, r + 3, seed=level)
    S.cylinder(tc, cx, top, base - 5, r, 0.96,
               tex=S.masonry(r, seed=level + 4))
    arch_door(tc, cx - 2, base - 4, 6, 9)
    # Arrow slits on the lit face, so the wall shows its thickness.
    rect(tc, cx - 6, top + 5, cx - 4, top + 10, 0.05)
    if height > 24:
        rect(tc, cx + 2, top + 13, cx + 4, top + 18, 0.05)

    if branch == 1:
        # The marksman: a tall lookout with a shingled spire and a pennant;
        # its arbalest looks out of the window under the eaves.
        S.cylinder(tc, cx, top - 6, top, r + 2, 0.9,
                   tex=S.wood(seed=7, spacing=3, vertical=True))
        S.cone(tc, cx, top - 20, top - 5, r + 4, 0.8,
               tex=S.shingles(rows=5, depth=0.4))
        S.capsule(tc, cx, top - 24, cx, top - 18, 1.5, 0.6)
        S.polygon(tc, [(cx + 1, top - 24), (cx + 10, top - 22),
                       (cx + 1, top - 19)], (-0.2, -0.2, 1.0), 0.85)
        rect(tc, cx - 4, top - 4, cx + 4, top - 1, 0.04)
        S.occlude(tc, base - 5, 8, 0.3)
        return tc.to_canvas()

    if level == 1:
        # Plain crenellated parapet.
        S.cylinder(tc, cx, top - 4, top, r + 2, 0.94,
                   tex=S.masonry(r + 2, course=4, blocks=8, seed=2),
                   cap=0.45)
        for u in (-0.85, -0.3, 0.25, 0.8):
            mx = cx + u * (r + 1)
            S.box(tc, mx - 2, top - 9, 4, 5, 2, 0.94, top_albedo=1.3)
    else:
        # A timber gallery hung out over the wall, roofed in snow.
        S.cylinder(tc, cx, top - 7, top, r + 3, 0.82,
                   tex=S.wood(seed=level, spacing=3, vertical=True), cap=0.5)
        for u in (-0.9, -0.45, 0.0, 0.45, 0.9):
            bx = cx + u * (r + 2)
            S.capsule(tc, bx, top, bx - u * 2, top + 4, 1.6, 0.6)
        snow_cap(tc, cx, top - 7, r + 3, depth=2)

    # The pivot post the ballista turns on.
    deck = top - (5 if level == 1 else 8)
    _mx, my = rig.guard_mount(variant)
    S.cylinder(tc, cx, my + 1, deck, 2.5, 0.6, tex=S.wood(seed=11, spacing=2),
               cap=0.9)
    if branch == 0:
        # Iron-tipped bolts racked beside the engine, and a war banner.
        for i in range(3):
            S.capsule(tc, cx - r + i * 2, deck - 1, cx - r + i * 2 + 2,
                      deck - 9, 1.0, 0.95, spec=0.5)
        S.capsule(tc, cx + r, deck + 2, cx + r, deck - 14, 1.5, 0.55)
        S.polygon(tc, [(cx + r + 1, deck - 14), (cx + r + 9, deck - 11),
                       (cx + r + 1, deck - 7)], (-0.3, -0.1, 1.0), 0.85,
                  tex=S.folds(4))
    S.occlude(tc, base - 5, 8, 0.3)
    return tc.to_canvas()


# The three heads: the plain crossbow of level 1, the heavy ballista and
# the twin-grooved repeater of the first upgrade.
GUARD_HEAD_SPECS = {
    "crossbow": {"back": 5.5, "span": 7.5, "bow": 2.0, "stock": 3.0, "bolts": 1},
    "ballista": {"back": 6.0, "span": 9.0, "bow": 2.2, "stock": 3.2, "bolts": 1},
    "repeater": {"back": 6.0, "span": 7.0, "bow": 1.8, "stock": 3.0, "bolts": 2},
}
GUARD_HEAD_FRONT = 5.0    # the bow sits this far ahead of the mount


def _thin_line(tc, x0, y0, x1, y1, tone):
    steps = int(max(abs(x1 - x0), abs(y1 - y0))) + 1
    for s in range(steps + 1):
        t = s / float(steps)
        tc.put(int(round(x0 + (x1 - x0) * t)), int(round(y0 + (y1 - y0) * t)),
               tone)


def guard_head(head, state, direction):
    """The ballista seen from above and in front, pointing one of eight ways.

    loaded  the bow drawn back and a bolt in the groove
    loosed  the arms snapped forward, the string straight, the stock kicked
            back a pixel
    drawn   winched back again, the groove still empty
    """
    spec = GUARD_HEAD_SPECS[head]
    tc = _part_canvas()
    a = direction * 2.0 * math.pi / rig.GUARD_DIRECTIONS
    dx, dy = math.cos(a), math.sin(a)
    px, py = -dy, dx
    mx, my = float(PART_CENTER), float(PART_CENTER)
    if state == "loosed":
        mx -= dx
        my -= dy

    def at(along, side=0.0):
        return mx + dx * along + px * side, my + dy * along + py * side

    back, span = spec["back"], spec["span"]
    bend = -0.6 if state == "loosed" else 2.6
    root = at(GUARD_HEAD_FRONT - 1.0)
    tips = [at(GUARD_HEAD_FRONT - 1.0 - bend, side) for side in (-span, span)]
    nut = at(-(back - 1.5))

    # The yoke it turns in.
    S.ellipsoid(tc, PART_CENTER, PART_CENTER + 1.5, 3.2, 1.8, 0.45)

    def stock():
        S.capsule(tc, *at(-back), *at(GUARD_HEAD_FRONT), spec["stock"], 0.72,
                  tex=S.wood(seed=11, spacing=2))
        if head == "repeater":
            # The magazine box that feeds both grooves.
            cx, cy = at(-2.0)
            S.box(tc, cx - 2, cy - 2.5, 4, 3, 2, 0.8, top_albedo=1.1)

    def bow():
        for tip in tips:
            S.capsule(tc, *root, *tip, spec["bow"], 0.9, spec=0.3)

    def string():
        if state == "loosed":
            _thin_line(tc, *tips[0], *tips[1], 0.1)
        else:
            for tip in tips:
                _thin_line(tc, *tip, *nut, 0.1)

    def bolts():
        if state != "loaded":
            return
        for k in range(spec["bolts"]):
            side = (k - (spec["bolts"] - 1) / 2.0) * 1.8
            S.capsule(tc, *at(-(back - 2.0), side), *at(rig.GUARD_REACH - 2.0, side),
                      1.3, 1.0, spec=0.6)
            S.polygon(tc, [at(rig.GUARD_REACH - 2.5, side - 1.3),
                           at(rig.GUARD_REACH + 0.5, side),
                           at(rig.GUARD_REACH - 2.5, side + 1.3)],
                      (0.0, -0.4, 1.0), 1.1, spec=0.8)

    # Pointing away, the bow is the far end and goes down first.
    if dy < -0.35:
        order = (bow, string, stock, bolts)
    else:
        order = (stock, string, bow, bolts)
    for draw in order:
        draw()
    # Outlined like every sprite, or it melts into the roof under it.
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Frost: an ice spire
# ----------------------------------------------------------------------


def _frost_tower(variant):
    """An ice spire grown out of a rune-cut stone plinth. The nova's
    orbiting shards are parts the view turns round it."""
    level, branch = rig.VARIANTS[variant]
    tc = _tower_canvas()
    cx, base = CX, FOOT_ROW
    S.shadow_ellipse(tc, cx + 6, base + 1, 19, 4.5, 0.5)

    # Plinth: two stepped drums of dressed stone.
    stone_plinth(tc, cx, base, 15, height=5, seed=3)
    S.cylinder(tc, cx, base - 10, base - 5, 12, 0.9,
               tex=S.masonry(12, course=5, blocks=6, seed=9), cap=0.85)
    # Runes, as dark notches round the drum.
    for u in (-0.6, -0.1, 0.4):
        rect(tc, cx + u * 12, base - 8, cx + u * 12 + 2, base - 6, 0.1)

    height = rig.frost_height(variant)
    _tx, top = rig.frost_tip(variant)
    half = 6 + level
    if branch == 1:
        half = 5 + level // 2
    crystal(tc, cx, top, base - 10, half, lean=-1)
    if level >= 2:
        # Smaller shards breaking out of the plinth.
        crystal(tc, cx - half - 4, base - 10 - height * 0.5, base - 9, 3.5,
                lean=-2)
        crystal(tc, cx + half + 4, base - 10 - height * 0.42, base - 8, 3,
                lean=2)
    if branch == 1:
        # A lens of clear ice over the tip, held by a cold iron ring.
        S.ellipsoid(tc, cx - 1, top - 1, 4, 4, 1.0, spec=0.9)
        S.glint(tc, int(cx - 3), int(top - 3), 1)
    S.glint(tc, int(cx - half * 0.4), int(top + half * 2 + 4), 1)
    S.occlude(tc, base - 5, 6, 0.25)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Siege: a catapult that throws
# ----------------------------------------------------------------------


def _siege_frame(variant, pose, facing, with_load=False):
    """A timber catapult on a stone emplacement, its arm in one pose.

    The frame faces its target: facing 1 throws to the right, -1 to the
    left. The light stays in the upper left either way, so the mirrored
    frames are drawn, not flipped. The shot itself is not part of the
    frame: the view draws it in the bucket, exactly as it then flies.
    """
    level, branch = rig.VARIANTS[variant]
    f = facing
    tc = _tower_canvas()
    cx, base = CX, FOOT_ROW

    def mx(x):
        return cx + f * (x - cx)

    S.shadow_ellipse(tc, cx + 6, base + 1, 22, 5, 0.5)
    stone_plinth(tc, cx, base, 18, height=6, seed=5)
    if branch == 1:
        # Shatter: a heap of ready stones on the emplacement.
        for sx, sy in ((15, -8), (18.5, -7), (16.5, -10.5)):
            S.ellipsoid(tc, mx(cx + sx), base + sy, 2.3, 2.0, 0.9, spec=0.2,
                        tex=S.fur(seed=13, depth=0.12))

    width = 22 + level * 2
    left = cx - width // 2
    deck = base - 13
    iron = branch == 0
    # The bed: a heavy timber frame with a dark side and a snow-dusted top.
    S.box(tc, left, deck, width, 5, 5, 0.85,
          tex=S.wood(seed=level, spacing=3), top_albedo=1.2)
    for wx in (left + 3, left + width - 2):
        wheel(tc, mx(wx), deck + 6, 6, albedo=0.55 if iron else 0.7)

    px, py = rig.siege_pivot(level)
    # The winch at the back, its rope wound round it.
    winch = (mx(cx - 9), deck - 2)
    wheel(tc, winch[0], winch[1], 3, albedo=0.6)
    # The A-frame: the rear post meets the axle; the front post runs on up
    # to the padded stop the arm slams into.
    stop_x, stop_y = mx(cx + 4), py - 10
    rear, front = mx(cx - 7), mx(cx + 7)
    S.capsule(tc, rear, deck - 1, px + (rear - cx) * 0.3, py - 2, 3.4,
              0.8 if rear < cx else 0.55,
              tex=S.wood(seed=3, spacing=4, vertical=True))
    S.capsule(tc, front, deck - 1, stop_x, stop_y + 1, 3.4,
              0.8 if front < cx else 0.55,
              tex=S.wood(seed=9, spacing=4, vertical=True))
    S.capsule(tc, cx - 6, py, cx + 6, py, 3, 0.7, spec=0.3)

    degrees = dict(rig.SIEGE_POSES)[pose]
    ex, ey = rig.siege_arm_end(level, degrees, f)
    length = rig.siege_arm_length(level)
    sx = px - (ex - px) * rig.SIEGE_SHORT_ARM / length
    sy = py - (ey - py) * rig.SIEGE_SHORT_ARM / length

    if pose in ("cocked", "wind"):
        # The rope from the winch to the arm, taut.
        hx = px + (ex - px) * 0.7
        hy = py + (ey - py) * 0.7
        S.capsule(tc, winch[0], winch[1] - 1, hx, hy, 1.0, 0.35)

    # The arm, the bucket at its long end, the hub over the axle.
    S.capsule(tc, sx, sy, ex, ey, 3.4 if iron else 3.2, 0.95,
              tex=S.wood(seed=3, spacing=5))
    if iron:
        for t in (0.35, 0.7):
            bx = sx + (ex - sx) * t
            by = sy + (ey - sy) * t
            nx, ny = -(ey - sy) / (length + rig.SIEGE_SHORT_ARM), \
                (ex - sx) / (length + rig.SIEGE_SHORT_ARM)
            S.capsule(tc, bx - nx * 1.8, by - ny * 1.8, bx + nx * 1.8,
                      by + ny * 1.8, 1.0, 0.15, spec=0.5)
    lx, ly = rig.siege_load(level, degrees, f)
    S.ellipsoid(tc, ex, ey, 3.6, 3.0, 0.95, spec=0.2)
    S.ellipsoid(tc, ex + (lx - ex) * 0.5, ey + (ly - ey) * 0.5, 1.8, 1.1,
                0.06)
    S.ellipsoid(tc, px, py, 2.0, 2.0, 0.5, spec=0.6)
    S.glint(tc, int(px - 1), int(py - 1), 0)
    # The padded stop, in front of the arm that hits it.
    S.ellipsoid(tc, stop_x, stop_y, 2.6, 2.2, 0.75, tex=S.fur(seed=17, depth=0.15))

    if level >= 2:
        # A stone counterweight box hung from the short end; it drops as
        # the arm throws, and lags behind as it swings.
        lag = {"swing": -1.5, "bounce": 1.0}.get(pose, 0.0) * f
        S.capsule(tc, sx, sy, sx + lag, sy + 2.5, 1.4, 0.4)
        S.box(tc, sx + lag - 4, sy + 2.5, 8, 6, 4, 0.8,
              tex=S.flat_masonry(course=3, block=4, seed=8), top_albedo=1.1)
    if with_load:
        S.ellipsoid(tc, lx, ly, 3.8, 3.8, 0.95, spec=0.25,
                    tex=S.fur(seed=13, depth=0.12))
    S.occlude(tc, base - 5, 6, 0.25)
    canvas = tc.to_canvas()
    if pose == "swing":
        _siege_smear(canvas, level, degrees, f)
    return canvas


def _siege_smear(canvas, level, degrees, facing):
    """The bucket's path through the throw, too quick to see: paper flecks
    on the open air behind the arm, thicker towards it, never outlined."""
    px, py = rig.siege_pivot(level)
    length = rig.siege_arm_length(level)
    start = dict(rig.SIEGE_POSES)["cocked"]
    steps = 60
    for step in range(steps):
        t = step / float(steps)
        d = start + (degrees - start) * t
        if d > degrees - 8:
            break
        for radius in (length - 1.0, length, length + 1.0, length + 2.0):
            ex, ey = rig.siege_arm_end(level, d, facing)
            x = int(round(px + (ex - px) * radius / length))
            y = int(round(py + (ey - py) * radius / length))
            dense = t > 0.55 or (x + y) % 2 == 0
            if dense and not canvas.is_set(x, y):
                canvas.set(x, y, WHITE)


# ----------------------------------------------------------------------
# Storm: a spire holding a caught storm
# ----------------------------------------------------------------------


def _copper_coil(tc, cx, y, r):
    """A dark band of wound wire round a column, with a lit top edge."""
    S.cylinder(tc, cx, y, y + 2, r, 0.45, lid=0.3, spec=0.6)
    for x in range(int(cx - r) + 1, int(cx + r)):
        if tc.get(x, y) is not None:
            tc.put(x, y, tc.get(x, y) + 0.35)


def _storm_tower(variant):
    """A slim stone spire wound with copper, an iron cage on top holding a
    caught storm: the orb the lightning leaps from. The static branch's
    charged stones are parts the view floats round the cage."""
    level, branch = rig.VARIANTS[variant]
    tc = _tower_canvas()
    cx, base = CX, FOOT_ROW
    orb_y = rig.STORM_ORB_ROW
    S.shadow_ellipse(tc, cx + 6, base + 1, 17, 4.5, 0.5)

    stone_plinth(tc, cx, base, 13, height=5, seed=21)
    # The shaft tapers: a broad drum, then a narrower one.
    S.cylinder(tc, cx, base - 16, base - 5, 10, 0.95,
               tex=S.masonry(10, course=5, blocks=6, seed=22), cap=0.6)
    top = orb_y + 7
    S.cylinder(tc, cx, top, base - 16, 6 + (level > 1), 0.92,
               tex=S.masonry(6, course=4, blocks=5, seed=23), cap=0.7)
    rect(tc, cx - 3, base - 13, cx - 1, base - 9, 0.05)
    for y in range(top + 4, base - 16, 6):
        _copper_coil(tc, cx, y, 6.5 + (level > 1))

    if level >= 2:
        # Two lightning rods brace the cage, their tips glinting.
        for side in (-1, 1):
            S.capsule(tc, cx + side * 6, top + 10, cx + side * 11, orb_y - 4,
                      1.4, 0.6 if side > 0 else 0.85, spec=0.6)
            S.glint(tc, int(cx + side * 11), int(orb_y - 5), 0)

    # The cage: four iron ribs round the orb, and the orb itself, white hot.
    radius = rig.storm_radius(variant)
    # The back ribs, the orb, then the two ribs that cross in front of it.
    for dx in (-radius * 0.35, radius * 0.35):
        S.capsule(tc, cx + dx, top, cx + dx, orb_y - radius, 1.0, 0.3)
    S.ellipsoid(tc, cx, orb_y, radius, radius, 1.2, spec=0.9)
    S.ellipsoid(tc, cx - 1, orb_y - 1, radius * 0.5, radius * 0.5, 1.4)
    for dx in (-radius - 1, radius + 1):
        S.capsule(tc, cx + dx * 0.8, top, cx + dx, orb_y - radius * 0.3, 1.2,
                  0.4, spec=0.4)
        S.capsule(tc, cx + dx, orb_y - radius * 0.3, cx + dx * 0.3,
                  orb_y - radius - 1, 1.2, 0.4, spec=0.4)
    S.capsule(tc, cx - radius - 1, top, cx + radius + 1, top, 1.6, 0.4)
    S.capsule(tc, cx, orb_y - radius - 1, cx, orb_y - radius - 6, 1.2,
              0.55, spec=0.6)
    S.glint(tc, int(cx - 1), int(orb_y - 1), 1)

    if branch == 1:
        # Tempest: a thunderhead boils over the rod, a bolt hanging from it.
        for dx, dy, r in ((-6, -8, 4.0), (0, -10, 5.0), (6, -8, 4.0),
                          (-2, -6, 3.5)):
            S.ellipsoid(tc, cx + dx, orb_y + dy, r, r * 0.8, 0.55,
                        tex=S.fur(31, depth=0.1))
        for y0, x0, y1, x1 in ((-4, 3, 0, 6), (0, 6, 3, 4)):
            S.capsule(tc, cx + x0, orb_y + y0, cx + x1, orb_y + y1, 1.2, 1.2)
    S.occlude(tc, base - 5, 6, 0.25)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Rune: an obelisk with an eye of light
# ----------------------------------------------------------------------


def _rune_glyph(tc, x, y, kind, tone):
    """A carved rune, three or four strokes deep into the face."""
    strokes = (
        ((0, 0, 0, 5), (0, 1, 2, 3)),
        ((0, 0, 0, 5), (0, 0, 2, 2), (0, 2, 2, 4)),
        ((1, 0, 1, 5), (0, 1, 2, 3), (2, 1, 0, 3)),
    )[kind % 3]
    for x0, y0, x1, y1 in strokes:
        steps = max(abs(x1 - x0), abs(y1 - y0))
        for s in range(steps + 1):
            t = s / max(1, steps)
            tc.put(int(x + x0 + (x1 - x0) * t), int(y + y0 + (y1 - y0) * t),
                   tone)


def _rune_tower(variant):
    """A four-sided obelisk on a stepped base, runes cut down its faces and
    an eye of light near the top: the beam leaves from there, or from the
    prism set on the capstone. The obelisk branch's rune stones are parts
    the view turns round the shaft."""
    level, branch = rig.VARIANTS[variant]
    tc = _tower_canvas()
    cx, base = CX, FOOT_ROW
    eye_y = rig.RUNE_EYE_ROW
    S.shadow_ellipse(tc, cx + 6, base + 1, 18, 4.5, 0.5)

    # Two stepped slabs of dressed stone.
    S.box(tc, cx - 12, base - 5, 22, 5, 5, 0.85,
          tex=S.flat_masonry(course=5, block=7, seed=41), top_albedo=1.2)
    S.box(tc, cx - 9, base - 9, 16, 4, 4, 0.9,
          tex=S.flat_masonry(course=4, block=6, seed=42), top_albedo=1.25)

    half_base = 6.0 + (level > 1)
    half_top = 3.5
    top = rig.rune_top(variant)
    bottom = base - 9
    # The lit west face and the shaded east face of the shaft.
    lit_face = [(cx - half_base, bottom), (cx, bottom + 1), (cx, top + 2),
                (cx - half_top, top + 3)]
    dark_face = [(cx, bottom + 1), (cx + half_base, bottom),
                 (cx + half_top, top + 3), (cx, top + 2)]
    S.polygon(tc, lit_face, (-0.7, -0.1, 0.7), 0.98,
              tex=lambda x, y, u, v: (S.hash01(x, y // 3, 43) - 0.5) * 0.08)
    S.polygon(tc, dark_face, (0.8, -0.1, 0.55), 0.78)
    # The capstone: a little pyramid, gilt bright on the lit side.
    S.polygon(tc, [(cx - half_top, top + 3), (cx, top - 4), (cx, top + 2)],
              (-0.6, -0.6, 0.6), 1.2, spec=0.7)
    S.polygon(tc, [(cx, top + 2), (cx, top - 4), (cx + half_top, top + 3)],
              (0.6, -0.5, 0.6), 0.8)

    # Runes carved down the lit face, glowing faintly on the dark one.
    for i, y in enumerate(range(eye_y + 7, bottom - 5, 7)):
        _rune_glyph(tc, cx - 4, y, i + level, 0.08)
        _rune_glyph(tc, cx + 1, y + 2, i + 1, 1.0)

    # The eye: a lens of light held in a dark socket.
    S.ellipsoid(tc, cx - 0.5, eye_y, 3.2, 2.4, 0.05)
    S.ellipsoid(tc, cx - 0.5, eye_y, 2.2, 1.6, 1.2, spec=0.9)
    S.glint(tc, int(cx - 1), int(eye_y), 0)

    if level >= 2:
        # Two standing stones keep watch either side.
        for side, albedo in ((-1, 0.95), (1, 0.75)):
            sx = cx + side * 14
            S.polygon(tc, [(sx - 2.5, base - 4), (sx + 2.5, base - 4),
                           (sx + 1.5, base - 15), (sx - 1.5, base - 16)],
                      (side * 0.6, -0.1, 0.8), albedo)
            _rune_glyph(tc, sx - 1, base - 13, side + 2, 0.1)
    if branch == 0:
        # A crystal prism sits on the capstone and splits the light.
        crystal(tc, cx, top - 9, top - 1, 3.2, albedo=1.05)
        S.glint(tc, int(cx - 1), int(top - 6), 1)
    S.occlude(tc, base - 5, 6, 0.25)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Barracks: a longhall whose door opens for the garrison
# ----------------------------------------------------------------------


def _barracks_tower(variant):
    """A timber longhall on a stone footing: pitched shingle roof, a broad
    plank door the garrison marches out of, shields hung along the eaves.
    The view opens the door (part "door_open") as soldiers come and go."""
    level, branch = rig.VARIANTS[variant]
    tc = _tower_canvas()
    cx, base = CX, FOOT_ROW
    S.shadow_ellipse(tc, cx + 6, base + 1, 23, 5, 0.5)

    width = 30 + level * 2
    left = cx - width // 2 - 2
    wall_top = base - 14 - level * 2
    # Footing, then walls: stone to the sill on a hall of level 2 and up.
    S.box(tc, left - 1, base - 4, width + 2, 4, 5, 0.85,
          tex=S.flat_masonry(course=4, block=6, seed=51), top_albedo=1.1)
    wall_tex = (S.flat_masonry(course=4, block=7, seed=52) if level > 1
                else S.wood(seed=53, spacing=3, vertical=True))
    S.box(tc, left, wall_top, width, base - 4 - wall_top, 5, 0.9,
          tex=wall_tex)
    # A hipped roof: the long slope faces the viewer, a short hip turns
    # away on the right, and snow lies along the ridge.
    ridge = wall_top - 10 - level
    eave = wall_top + 2
    right = left + width
    inset = 6

    def courses(x, y, _u, _v):
        # Shingle rows with staggered butt joints.
        row = int(y - ridge)
        if row % 3 == 0:
            return -0.35
        return -0.2 if (int(x) + (row // 3) * 2) % 5 == 0 else 0.0
    S.polygon(tc, [(left - 3, eave), (right + 2, eave), (right - inset, ridge),
                   (left + inset - 2, ridge)], (-0.15, -0.75, 0.65), 1.05,
              tex=courses)
    S.polygon(tc, [(right + 2, eave), (right + 5, eave - 3),
                   (right - inset + 1, ridge), (right - inset, ridge)],
              (0.85, -0.4, 0.4), 0.7, tex=courses)
    S.polygon(tc, [(left - 3, eave), (left + inset - 2, ridge),
                   (left + inset - 4, ridge + 1)], (-0.9, -0.3, 0.4), 1.0)
    snow_cap(tc, (left + right) / 2.0, ridge, (right - left) / 2.0 - inset,
             depth=2)
    # Crossed gable horns at both ends of the ridge, the old northern way.
    for gx in (left + inset - 2, right - inset):
        S.capsule(tc, gx - 3, ridge - 4, gx + 1, ridge, 1.3, 0.75)
        S.capsule(tc, gx + 3, ridge - 4, gx - 1, ridge, 1.3, 0.55)
    # The door: a dark frame round a shut leaf of planks with an iron ring.
    door_x, door_bottom = rig.BARRACKS_DOOR
    door_w, door_h = rig.BARRACKS_DOOR_SIZE
    arch_door(tc, door_x, door_bottom, door_w, door_h)
    arch_door(tc, door_x, door_bottom, door_w - 2, door_h - 1, tone=0.62)
    for x in range(int(door_x - door_w / 2 + 1), int(door_x + door_w / 2)):
        for y in range(door_bottom - door_h + 2, door_bottom):
            if tc.get(x, y) is not None and tc.get(x, y) > 0.5 and x % 2 == 0:
                tc.put(x, y, 0.32)
    tc.put(door_x + 1, door_bottom - 5, 1.0)
    S.capsule(tc, cx - 6, base - 15, cx + 4, base - 15, 1.6, 0.6)

    # Round shields along the wall, alternating light and dark paint.
    for i, sx in enumerate((left + 4, left + width - 5)):
        S.ellipsoid(tc, sx, wall_top + 5, 3.2, 3.4, 1.0 if i == 0 else 0.6,
                    depth=0.5, spec=0.3)
        S.ellipsoid(tc, sx, wall_top + 5, 1.0, 1.0, 1.2, spec=0.8)

    if level >= 2:
        # A banner on a pole by the door.
        px = left + width + 1
        S.capsule(tc, px, base - 2, px, ridge - 2, 1.4, 0.6)
        S.polygon(tc, [(px + 1, ridge - 2), (px + 8, ridge),
                       (px + 1, ridge + 6)], (-0.2, -0.1, 1.0), 0.9,
                  tex=S.folds(3))
    if branch == 0:
        # Veterans: a rack of spears leaning by the door, steel catching light.
        for i in range(3):
            x = left + 2 + i * 2
            S.capsule(tc, x, base - 3, x + 2, wall_top - 6, 1.0, 0.7)
            S.glint(tc, int(x + 2), int(wall_top - 7), 0)
    elif branch == 1:
        # Berserk: an elk skull over the door, antlers spread wide.
        sy = wall_top + 1
        S.ellipsoid(tc, cx - 1, sy, 2.6, 3, 1.1, spec=0.4)
        tc.put(int(cx - 2), int(sy), 0.05)
        tc.put(int(cx), int(sy), 0.05)
        for side in (-1, 1):
            S.capsule(tc, cx - 1 + side * 2, sy - 2, cx - 1 + side * 8, sy - 7,
                      1.2, 1.0)
            S.capsule(tc, cx - 1 + side * 5, sy - 4, cx - 1 + side * 5,
                      sy - 9, 1.0, 1.0)
    S.occlude(tc, base - 4, 6, 0.25)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Ember: a kiln whose fire moves
# ----------------------------------------------------------------------


def _ember_tower(variant, frame):
    """A squat stone kiln banded in iron with a fire cauldron on top.

    frame 0..3 is the fire flickering; EMBER_FLICKER_FRAMES is the surge
    as a pot is thrown out of it. The spare pots stacked by a big kiln are
    parts: the view takes one away for a throw and puts it back."""
    level, branch = rig.VARIANTS[variant]
    tc = _tower_canvas()
    cx, base = CX, FOOT_ROW
    S.shadow_ellipse(tc, cx + 6, base + 1, 19, 4.5, 0.5)

    stone_plinth(tc, cx, base, 15, height=5, seed=61)
    # The kiln: a broad stone drum, then a narrower collar the bowl sits in.
    r = rig.ember_radius(level)
    bowl_y = rig.EMBER_BOWL_ROW
    collar = bowl_y + 5
    S.cylinder(tc, cx, collar + 6, base - 5, r, 0.95,
               tex=S.masonry(r, course=5, blocks=7, seed=62))
    S.cylinder(tc, cx, collar, collar + 6, r - 3, 0.9,
               tex=S.masonry(r - 3, course=3, blocks=6, seed=63), cap=0.3)
    # An iron band round the drum.
    for x in range(int(cx - r), int(cx + r) + 1):
        if tc.get(x, collar + 9) is not None:
            tc.put(x, collar + 9, 0.12)
    # Fire slits and the stoke hole, white with heat in the dark stone.
    surge = frame >= EMBER_FLICKER_FRAMES
    arch_door(tc, cx - 2, base - 5, 7, 7, tone=0.02)
    arch_door(tc, cx - 2, base - 6, 3 + (1 if surge else 0), 4, tone=1.0)
    for sx in (cx - 8, cx + 4):
        rect(tc, sx, collar + 11, sx + 2, collar + 15, 1.0)

    # The cauldron: a heavy black iron bowl, its lip catching the
    # firelight, the fire roaring up out of it. The bowl is near black so
    # the fire over it is the brightest thing on the tower.
    bowl_r = 9 + level * 0.5
    S.ellipsoid(tc, cx, bowl_y + 1, bowl_r, 4.5, 0.12)
    for x in range(int(cx - bowl_r) + 1, int(cx + bowl_r)):
        tc.put(x, bowl_y - 2, 0.05)
    S.ellipsoid(tc, cx, bowl_y - 2, bowl_r - 1, 1.4, 1.3)
    S.glint(tc, int(cx - bowl_r + 3), int(bowl_y + 1), 0)
    tongues = ((-4, 7, 3.0), (0, 11 + level * 2, 3.8), (4, 8, 2.8))
    if branch == 0:
        tongues = ((-6, 9, 2.6), (-2, 14, 3.4), (2, 12, 3.2), (6, 8, 2.4))
    # Each tongue rises and falls out of step with the others and leans in
    # the draught; the surge throws them all up and out.
    p = phase(frame % EMBER_FLICKER_FRAMES, EMBER_FLICKER_FRAMES)
    ceiling = bowl_y - 3
    flames = []
    for i, (dx, h, w) in enumerate(tongues):
        if surge:
            grow, lean, w = 1.35, dx * 0.35, w * 1.25
        else:
            grow = 0.85 + 0.22 * math.sin(p + i * 1.9)
            lean = 1.2 * math.sin(p * 0.5 + i * 2.3)
        height = min(h * grow, ceiling)
        flames.append((cx + dx, cx + dx + lean, height, w))
    for root, tip, height, w in flames:
        apex = bowl_y - 2 - height
        S.cone(tc, (root + tip) / 2.0, apex, bowl_y - 2, w, 1.3, lid=0.5)
        S.capsule(tc, (root + tip) / 2.0, apex + 1, tip, apex - 1, 1.2, 1.3)
    # Dark licks inside the blaze, so it reads as tongues, not a cone.
    for root, tip, height, _w in flames:
        x = (root + tip) / 2.0
        for y in range(int(bowl_y - 2 - height * 0.55), int(bowl_y - 3)):
            tc.put(int(x + (1 if x <= cx else -1)), y, 0.05)
    if surge:
        # Sparks thrown up over the blaze.
        for dx, dy in ((-5, -4), (1, -7), (6, -3)):
            tc.put(cx + dx, max(0, int(ceiling - 14 + dy)), 1.0)
    if branch == 1:
        # Magma: molten rock spills over the lip and runs down the stones.
        for dx, length in ((-7, 10), (6, 14), (-1, 8)):
            S.capsule(tc, cx + dx, bowl_y + 2, cx + dx * 1.1, bowl_y + length,
                      2.2, 1.2, spec=0.4)
    S.occlude(tc, base - 5, 6, 0.25)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Parts the view places
# ----------------------------------------------------------------------


def _spare_pot():
    """A clay fire pot waiting by the kiln, plugged with rag."""
    tc = _part_canvas()
    c = PART_CENTER
    S.ellipsoid(tc, c, c, 3, 3.2, 0.95, spec=0.3)
    S.capsule(tc, c, c - 3, c + 1, c - 5, 1.2, 1.0)
    return tc.to_canvas()


def _orbit_shard():
    """One of the nova's ice shards; its foot is the part's centre."""
    tc = _part_canvas()
    crystal(tc, PART_CENTER, PART_CENTER - 6, PART_CENTER, 2.2)
    return tc.to_canvas()


def _orbit_stone():
    """A charged stone floating round the storm cage."""
    tc = _part_canvas()
    S.ellipsoid(tc, PART_CENTER, PART_CENTER, 2.2, 1.8, 0.95, spec=0.4)
    return tc.to_canvas()


def _orbit_rune():
    """A rune-cut block lifted off the ground by the obelisk's spell."""
    tc = _part_canvas()
    c = PART_CENTER
    S.box(tc, c - 2, c - 2, 4, 4, 2, 0.9, top_albedo=1.2)
    tc.put(c, c, 0.05)
    return tc.to_canvas()


def _door_open():
    """The barracks door swung in: a dark doorway with the leaf's edge."""
    tc = _part_canvas()
    x, bottom = DOOR_ANCHOR
    w, h = rig.BARRACKS_DOOR_SIZE
    arch_door(tc, x, bottom, w, h)
    rect(tc, x - w / 2 + 1, bottom - h + 4, x - w / 2 + 3, bottom, 0.62)
    return tc.to_canvas(halo=False)


# ----------------------------------------------------------------------
# Sheets
# ----------------------------------------------------------------------


def siege_frame_name(variant, pose, facing_key):
    return f"tower_siege_{VARIANT_NAMES[variant]}_{pose}_{facing_key}"


def ember_frame_name(variant, frame):
    suffix = "surge" if frame >= EMBER_FLICKER_FRAMES else str(frame)
    return f"tower_ember_{VARIANT_NAMES[variant]}_{suffix}"


def guard_head_name(head, state, direction):
    return f"head_{head}_{state}_{direction}"


STILL_BUILDERS = {
    "guard": _guard_body,
    "frost": _frost_tower,
    "storm": _storm_tower,
    "rune": _rune_tower,
    "barracks": _barracks_tower,
}


def unit_entries():
    """Every tower frame, by name, in the units sheet."""
    entries = []
    for key, builder in STILL_BUILDERS.items():
        for variant, name in enumerate(VARIANT_NAMES):
            entries.append((f"tower_{key}_{name}", builder(variant)))
    for variant in range(len(VARIANT_NAMES)):
        for pose, _degrees in rig.SIEGE_POSES:
            for facing_key, facing in FACINGS:
                entries.append((siege_frame_name(variant, pose, facing_key),
                                _siege_frame(variant, pose, facing)))
    for variant in range(len(VARIANT_NAMES)):
        for frame in range(EMBER_FLICKER_FRAMES + 1):
            entries.append((ember_frame_name(variant, frame),
                            _ember_tower(variant, frame)))
    return entries


def part_entries():
    """The parts sheet: ballista heads, spare pots, orbiters, the door."""
    entries = []
    for head in rig.GUARD_HEADS:
        for state in rig.GUARD_STATES:
            for direction in range(rig.GUARD_DIRECTIONS):
                entries.append((guard_head_name(head, state, direction),
                                guard_head(head, state, direction)))
    entries += [("spare_pot", _spare_pot()), ("orbit_shard", _orbit_shard()),
                ("orbit_stone", _orbit_stone()), ("orbit_rune", _orbit_rune()),
                ("door_open", _door_open())]
    return entries


def _overlay(canvas, part, x, y):
    """Blit a part with its centre on pixel (x, y) of a tower cell."""
    canvas.blit(part, int(round(x)) - PART_CENTER, int(round(y)) - PART_CENTER)


def card(kind, level):
    """A tower whole, as the build ring shows it: body and moving parts
    together, the catapult cocked and loaded, the kiln's spares stacked."""
    variant = level - 1
    if kind == "siege":
        return _siege_frame(variant, "cocked", 1, with_load=True)
    if kind == "ember":
        canvas = _ember_tower(variant, 0)
        for x, y in rig.ember_spares(level):
            _overlay(canvas, _spare_pot(), x, y)
        return canvas
    canvas = STILL_BUILDERS[kind](variant)
    if kind == "guard":
        mx, my = rig.guard_mount(variant)
        head = "crossbow" if level == 1 else "ballista"
        _overlay(canvas, guard_head(head, "loaded", 7), mx, my)
    return canvas

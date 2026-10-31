"""Sprite sheets for Frostline Hold: towers, the enemy roster, the hero,
build pads, projectiles, UI icons and the castle's moving parts.

Every sprite is modelled as lit solids in artgen.shade and dithered at the
end, so each one carries real volume: round towers with coursed stone,
faceted ice, shaded fur and armour. Sizes are the native draw sizes, so the
view never rescales a sheet and the dither pattern stays exact.

Common conventions:
  * light comes from the upper left
  * a sprite stands with its feet FOOT pixels above the bottom of its cell
  * every sprite gets a one pixel ink outline and a dithered cast shadow
"""
import math

from artgen import shade as S
from artgen import tower_art
from artgen.art_kit import crystal, phase, rect
from artgen.canvas import Canvas

UNIT_CELL = 56
PROP_CELL = 40
HERO_CELL = 48
HUSK_CELL = 48
TITAN_CELL = 80
FX_CELL = 16
ICON_CELL = 28
CASTLE_FX_CELL = 28

UNIT_ATLAS_CELL = UNIT_CELL + 4
PROP_ATLAS_CELL = PROP_CELL + 4
HERO_ATLAS_CELL = HERO_CELL + 4
HUSK_ATLAS_CELL = HUSK_CELL + 4
TITAN_ATLAS_CELL = TITAN_CELL + 4
FX_ATLAS_CELL = FX_CELL + 4
ICON_ATLAS_CELL = ICON_CELL + 4
CASTLE_FX_ATLAS_CELL = CASTLE_FX_CELL + 4

# Feet sit this far above the bottom of every cell; the view uses the same
# number to stand a sprite on its map position.
FOOT = 6
WALK_FRAMES = 6


# Shared building blocks, under the names the roster below uses.
_phase = phase
_rect = rect


# ----------------------------------------------------------------------
# The rank and file (40 px cells)
# ----------------------------------------------------------------------


def _enemy_canvas(size=PROP_CELL):
    return S.ToneCanvas(size, size)


def _enemy_wolf(frame):
    """A grey wolf at a lope: pale belly, dark saddle, ears up."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    swing = 4.0 * math.sin(p)
    bob = -1.2 * abs(math.sin(p))
    S.shadow_ellipse(tc, 21, feet + 1, 14, 3, 0.5)

    # Far legs first, darker because they are in the body's shadow.
    S.capsule(tc, 12, 24 + bob, 12 - swing, feet, 3.2, 0.45)
    S.capsule(tc, 28, 24 + bob, 28 + swing, feet, 3.2, 0.45)

    def coat(x, y, u, v):
        saddle = -0.22 if v < -0.15 else 0.0
        belly = 0.3 if v > 0.35 else 0.0
        return saddle + belly + S.fur(1)(x, y, u, v)

    # A bushy tail, then the body, the ruff and the head.
    S.capsule(tc, 8, 19 + bob, 2, 23 + bob - swing * 0.4, 4.5, 0.75,
              tex=S.fur(2))
    S.ellipsoid(tc, 19, 21 + bob, 11.5, 6.5, 0.9, tex=coat)
    S.ellipsoid(tc, 28, 18 + bob, 5.5, 6.5, 0.95, tex=S.fur(4))
    S.ellipsoid(tc, 32, 15 + bob, 5, 4.5, 0.95, tex=S.fur(5))
    S.ellipsoid(tc, 37, 17 + bob, 3.4, 2.2, 1.0)
    tc.put(39, 16 + bob, 0.0)
    S.polygon(tc, [(29, 12 + bob), (30, 6 + bob), (33, 11 + bob)],
              (-0.5, -0.3, 0.8), 0.85)
    S.polygon(tc, [(32, 11 + bob), (34, 6 + bob), (35, 11 + bob)],
              (0.6, -0.3, 0.7), 0.7)
    tc.put(33, 14 + bob, 0.0)
    tc.put(34, 14 + bob, 0.0)

    # Near legs, lit.
    S.capsule(tc, 15, 24 + bob, 15 + swing, feet, 3.2, 0.9)
    S.capsule(tc, 25, 24 + bob, 25 - swing, feet, 3.2, 0.9)
    return tc.to_canvas()


def _legs(tc, hip_x, hip_y, feet, swing, width=3.2, near=0.75, far=0.4):
    """A walking pair: the far leg in shade, the near leg lit."""
    S.capsule(tc, hip_x + 2, hip_y, hip_x + 2 - swing, feet, width, far)
    S.capsule(tc, hip_x - 1, hip_y, hip_x - 1 + swing, feet, width, near)


def _enemy_raider(frame):
    """A northern raider: fur cloak, horned helm, round shield and axe."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    swing = 3.0 * math.sin(p)
    bob = -abs(math.sin(p))
    S.shadow_ellipse(tc, 21, feet + 1, 11, 3, 0.5)

    _legs(tc, 20, 25 + bob, feet, swing, width=3.6, near=0.55, far=0.3)
    # Axe behind the shoulder, so the shield arm overlaps it.
    S.capsule(tc, 25, 26 + bob, 30, 9 + bob, 1.8, 0.6,
              tex=S.wood(seed=2, spacing=3))
    S.polygon(tc, [(29, 8 + bob), (36, 6 + bob), (35, 15 + bob),
                   (30, 13 + bob)], (0.2, -0.2, 1.0), 1.0, spec=0.6)
    # Fur-trimmed body.
    S.ellipsoid(tc, 20, 20 + bob, 7, 8, 0.85, tex=S.fur(3, depth=0.12))
    S.capsule(tc, 14, 15 + bob, 26, 15 + bob, 4, 1.0, tex=S.fur(6))
    S.capsule(tc, 15, 26 + bob, 25, 26 + bob, 2, 0.4)
    # Head and helm, horns catching the light.
    S.ellipsoid(tc, 20, 10 + bob, 4.5, 4.5, 0.95)
    S.ellipsoid(tc, 20, 8 + bob, 5, 3.4, 0.75, spec=0.7)
    S.capsule(tc, 16, 8 + bob, 12, 3 + bob, 1.6, 1.0)
    S.capsule(tc, 24, 8 + bob, 28, 3 + bob, 1.6, 0.85)
    _rect(tc, 17, 11 + bob, 24, 12 + bob, 0.08)
    # Round shield on the near arm: domed boards and an iron boss.
    def boards(x, y, u, v):
        ring = math.sqrt(u * u + v * v)
        if ring > 0.82:
            return -0.3
        return -0.2 if int(x) % 3 == 0 else 0.0
    S.ellipsoid(tc, 13, 21 + bob, 6.5, 7, 0.95, depth=0.5, tex=boards)
    S.ellipsoid(tc, 13, 21 + bob, 2, 2, 1.0, spec=0.8)
    return tc.to_canvas()


def _enemy_skitter(frame):
    """An ice beetle: a glossy domed shell scuttling on six legs."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    S.shadow_ellipse(tc, 21, feet + 1, 15, 3.5, 0.5)
    body_y = feet - 7

    for i, lx in enumerate((10, 19, 28)):
        step = 3.0 * math.sin(p + i * 2.1)
        S.capsule(tc, lx + 2, body_y, lx + 6 + step, feet - 1, 1.8, 0.35)
    def shell(x, y, u, v):
        seam = -0.5 if abs(u) < 0.07 else 0.0
        ridge = -0.18 if 0.45 < math.sqrt(u * u + v * v) < 0.52 else 0.0
        return seam + ridge
    S.ellipsoid(tc, 19, body_y - 3, 12, 8, 0.95, spec=0.9, tex=shell)
    S.ellipsoid(tc, 31, body_y, 4.5, 3.5, 0.5, spec=0.5)
    S.capsule(tc, 34, body_y + 1, 38, body_y - 1, 1.2, 0.8)
    S.capsule(tc, 34, body_y + 2, 38, body_y + 4, 1.2, 0.6)
    for i, lx in enumerate((10, 19, 28)):
        step = 3.0 * math.sin(p + i * 2.1 + math.pi)
        S.capsule(tc, lx, body_y + 1, lx - 3 + step, feet, 1.8, 0.75)
    S.glint(tc, 14, body_y - 8, 1)
    return tc.to_canvas()


def _enemy_shaman(frame):
    """A hooded frost shaman with a glowing staff orb."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    sway = 1.2 * math.sin(p)
    S.shadow_ellipse(tc, 21, feet + 1, 11, 3, 0.5)

    # Staff behind the robe.
    S.capsule(tc, 28, feet, 28 + sway, 6, 1.8, 0.6,
              tex=S.wood(seed=4, spacing=3))
    # Robe: a cone of heavy cloth with folds, hem dragging in the snow.
    S.cone(tc, 20 + sway * 0.4, 8, feet - 1, 10, 0.75, tex=S.folds(7, 0.2))
    S.capsule(tc, 12, feet - 1, 28, feet - 1, 2.6, 0.55)
    # Hood with a dark face and two cold eyes.
    S.ellipsoid(tc, 20 + sway * 0.4, 10, 6, 6, 0.85, tex=S.folds(3, 0.1))
    S.ellipsoid(tc, 21 + sway * 0.4, 12, 3.5, 3.5, 0.04)
    tc.put(20 + sway * 0.4, 12, 1.0)
    tc.put(23 + sway * 0.4, 12, 1.0)
    # The hand on the staff and the orb, which pulses.
    S.ellipsoid(tc, 27 + sway, 20, 2.4, 2.4, 0.9)
    pulse = 3.0 + 0.8 * math.sin(p * 2)
    S.ellipsoid(tc, 28 + sway, 5, pulse, pulse, 1.05, spec=0.9)
    S.glint(tc, int(27 + sway), 4, 1 if frame % 2 == 0 else 0)
    return tc.to_canvas()


def _enemy_banner(frame):
    """A standard bearer: the flag rallies the troops around him."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    swing = 2.6 * math.sin(p)
    bob = -abs(math.sin(p))
    S.shadow_ellipse(tc, 20, feet + 1, 12, 3, 0.5)

    # The pole and a cloth that ripples along its length.
    S.capsule(tc, 24, feet - 4 + bob, 24, 2 + bob, 1.8, 0.6)
    S.ellipsoid(tc, 24, 2 + bob, 1.8, 1.8, 1.0, spec=0.8)
    wave = [math.sin(p + i * 0.9) * 1.8 for i in range(5)]
    top = [(25 + i * 3, 4 + bob + wave[i]) for i in range(5)]
    bottom = [(25 + i * 3, 15 + bob + wave[i] + i * 0.4) for i in range(5)]
    cloth = top + list(reversed(bottom))
    def ripple(x, y, _u, _v):
        return 0.22 * math.sin(p + (x - 25) / 3.0 * 0.9 + 1.2)
    S.polygon(tc, cloth, (-0.2, -0.1, 1.0), 0.85, tex=ripple)
    # The rune on the flag.
    _rect(tc, 30, 7 + bob, 32, 13 + bob, 0.1)

    _legs(tc, 18, 25 + bob, feet, swing, width=3.4, near=0.6, far=0.3)
    S.ellipsoid(tc, 18, 20 + bob, 6, 7.5, 0.8, tex=S.folds(4, 0.14))
    S.ellipsoid(tc, 18, 11 + bob, 4.2, 4.2, 0.95)
    S.ellipsoid(tc, 18, 9 + bob, 4.6, 3, 0.6, spec=0.5)
    _rect(tc, 16, 12 + bob, 21, 13 + bob, 0.08)
    S.ellipsoid(tc, 23, 19 + bob, 2.2, 2.2, 0.9)
    return tc.to_canvas()


def _enemy_sap(frame):
    """A frost slime: a glossy blob that squashes as it creeps."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    squash = math.sin(p)
    rx = 11 + 1.5 * squash
    ry = 8 - 1.2 * squash
    S.shadow_ellipse(tc, 21, feet + 1, rx + 3, 3, 0.5)
    cy = feet - ry + 1

    def bubbles(x, y, u, v):
        for bx, by, br in ((-0.3, 0.2, 0.18), (0.35, 0.35, 0.12),
                           (0.1, -0.1, 0.1)):
            if (u - bx) ** 2 + (v - by) ** 2 < br * br:
                return -0.45
        return 0.0
    S.ellipsoid(tc, 20, cy, rx, ry, 0.9, spec=1.0, tex=bubbles)
    # Eyes riding high on the front of the blob.
    for ex in (23, 27):
        S.ellipsoid(tc, ex, cy - 2, 1.4, 1.9, 0.0)
    S.glint(tc, 14, int(cy - ry * 0.5), 1)
    return tc.to_canvas()


def _enemy_husk(frame):
    """An armoured brute in banded plate, swinging a spiked mace."""
    tc = _enemy_canvas(HUSK_CELL)
    feet = HUSK_CELL - FOOT
    p = _phase(frame)
    swing = 3.0 * math.sin(p)
    bob = -1.5 * abs(math.sin(p))
    S.shadow_ellipse(tc, 25, feet + 1, 16, 4, 0.5)

    _legs(tc, 24, 30 + bob, feet, swing, width=6, near=0.6, far=0.3)

    def plate(x, y, u, v):
        band = int(y - bob) % 5
        return -0.35 if band == 0 else (0.08 if band == 1 else 0.0)
    # Mace arm behind the body.
    S.capsule(tc, 33, 20 + bob, 38, 30 + bob - swing, 3.6, 0.5)
    S.capsule(tc, 38, 30 + bob - swing, 41, 18 + bob - swing, 1.8, 0.6)
    S.ellipsoid(tc, 41, 16 + bob - swing, 4, 4, 0.75, spec=0.6)
    for dx, dy in ((-4, 0), (4, 0), (0, -4)):
        tc.put(41 + dx, 16 + bob - swing + dy, 0.0)
    S.ellipsoid(tc, 24, 24 + bob, 12, 11, 0.9, spec=0.5, tex=plate)
    # Pauldrons, domed and glinting.
    S.ellipsoid(tc, 13, 17 + bob, 5.5, 5, 0.95, spec=0.8)
    S.ellipsoid(tc, 34, 17 + bob, 5, 4.5, 0.85, spec=0.6)
    # Helm with a dark visor slit.
    S.ellipsoid(tc, 24, 11 + bob, 6, 6, 0.9, spec=0.8)
    _rect(tc, 20, 11 + bob, 29, 13 + bob, 0.04)
    S.capsule(tc, 24, 4 + bob, 24, 8 + bob, 1.6, 0.8)
    # Near arm with a gauntlet.
    S.capsule(tc, 13, 21 + bob, 11, 30 + bob + swing * 0.5, 4, 0.8,
              spec=0.4, tex=plate)
    S.ellipsoid(tc, 11, 31 + bob + swing * 0.5, 3, 3, 0.85, spec=0.5)
    return tc.to_canvas()


def _enemy_titan(frame):
    """The ice titan: a hulking frame of rimed stone crowned with shards."""
    tc = _enemy_canvas(TITAN_CELL)
    feet = TITAN_CELL - FOOT
    p = _phase(frame)
    swing = 4.0 * math.sin(p)
    bob = -2.0 * abs(math.sin(p))
    S.shadow_ellipse(tc, 42, feet + 2, 28, 6, 0.5)

    def rock(x, y, u, v):
        # Cracked boulder skin: big cells with dark seams.
        cell = S.hash01(int(x) // 6, int(y) // 5, 21)
        seam = (int(x) + (int(y) // 5) * 3) % 6 == 0 or int(y) % 5 == 0
        return (-0.28 if seam else 0.0) + (cell - 0.5) * 0.12

    # Far arm and leg.
    S.capsule(tc, 48, 48 + bob, 48 - swing, feet, 10, 0.4, tex=rock)
    S.capsule(tc, 58, 28 + bob, 64, 50 + bob - swing, 9, 0.4, tex=rock)
    S.ellipsoid(tc, 65, 53 + bob - swing, 6.5, 6, 0.45, tex=rock)
    # Torso and the hunched back.
    S.ellipsoid(tc, 40, 38 + bob, 21, 18, 0.9, tex=rock)
    S.ellipsoid(tc, 41, 46 + bob, 14, 8, 0.75, tex=rock)
    # Crown of ice on the shoulders.
    crystal(tc, 26, 10 + bob, 26 + bob, 4.5, lean=-3)
    crystal(tc, 33, 4 + bob, 24 + bob, 5, lean=-1)
    crystal(tc, 52, 9 + bob, 25 + bob, 4, lean=2)
    # The head, sunk between the shoulders, eyes burning white.
    S.ellipsoid(tc, 42, 24 + bob, 8, 7, 0.85, tex=rock)
    _rect(tc, 36, 23 + bob, 48, 27 + bob, 0.05)
    S.glint(tc, 39, 25 + bob, 1)
    S.glint(tc, 45, 25 + bob, 1)
    # Near leg and the great near arm.
    S.capsule(tc, 34, 48 + bob, 34 + swing, feet, 10, 0.8, tex=rock)
    S.capsule(tc, 22, 28 + bob, 15, 50 + bob + swing, 10, 0.95, tex=rock)
    S.ellipsoid(tc, 14, 54 + bob + swing, 7.5, 7, 0.95, tex=rock)
    S.occlude(tc, feet, 6, 0.3)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# The deeper roster: birds, golems, slimes, burrowers and the two bosses
# ----------------------------------------------------------------------


CROW_BODY_Y = 22   # the body's row in its cell; the view hangs it there


def _wing(tc, root_x, root_y, span, angle, albedo):
    """A spread wing built the way a bird's is: a broad arm, a hand, and
    primaries fanned behind the hand, all overlapping so the wing reads as
    one solid sail. `angle` is the stroke in radians, 0 straight back,
    negative raised, positive beaten down."""
    def at(length, bend=0.0):
        a = angle + bend
        return root_x - math.cos(a) * length, root_y + math.sin(a) * length

    elbow = at(span * 0.45)
    hand = at(span * 0.8)

    def coverts(x, y, _u, _v):
        return -0.16 if int(x * 0.6 + y) % 3 == 0 else 0.0
    S.capsule(tc, root_x, root_y, elbow[0], elbow[1], 6.5, albedo,
              spec=0.5, tex=coverts)
    S.capsule(tc, elbow[0], elbow[1], hand[0], hand[1], 5, albedo * 0.95,
              tex=coverts)
    # Primaries: each a little longer and swept further back than the last,
    # so a lowered wing never hangs straight down like a leg.
    for i in range(4):
        tip = at(span * (0.95 + i * 0.04), -angle * 0.18 * (i + 1))
        S.capsule(tc, hand[0], hand[1], tip[0], tip[1], 2.6, albedo * 0.85)


def _enemy_crow(frame):
    """A frost crow on the wing, glossy black with a pale beak. The view
    draws it above the road with its shadow on the ground, so the cell
    carries no shadow of its own."""
    tc = _enemy_canvas()
    p = _phase(frame)
    stroke = 0.75 * math.cos(p) - 0.25   # frame 0 is the downbeat
    bob = 1.5 * math.sin(p)       # the body rises as the wings beat down
    cy = CROW_BODY_Y + bob

    # The far wing peeks out above the body, always a beat behind.
    _wing(tc, 22, cy - 2, 15, -0.55 - 0.45 * abs(stroke), 0.2)
    # A wedge tail, then the body, the head and the beak.
    S.polygon(tc, [(12, cy - 2), (3, cy - 4), (2, cy + 1), (4, cy + 4),
                   (12, cy + 2)], (-0.2, 0.3, 0.9), 0.28,
              tex=lambda x, y, _u, _v: -0.2 if int(y) % 2 == 0 else 0.0)
    S.ellipsoid(tc, 19, cy, 10, 5.5, 0.32, spec=1.0, tex=S.fur(8, 0.06))
    S.ellipsoid(tc, 29, cy - 3, 5, 4.4, 0.34, spec=1.0)
    S.polygon(tc, [(32, cy - 4.5), (38, cy - 2), (32, cy - 0.5)],
              (0.2, -0.4, 0.9), 1.0, spec=0.6)
    S.glint(tc, 30, int(cy - 4), 0)
    # Tucked feet under the belly.
    S.capsule(tc, 18, cy + 5, 16, cy + 8, 1.4, 0.7)
    S.capsule(tc, 21, cy + 5, 20, cy + 8, 1.4, 0.6)
    _wing(tc, 21, cy - 1, 17, stroke, 0.48)
    return tc.to_canvas()


def _blob_eyes(tc, cx, cy, spread, size=1.4):
    for ex in (cx - spread, cx + spread):
        S.ellipsoid(tc, ex, cy, size, size * 1.35, 0.0)
        tc.put(ex - 0.5, cy - size * 0.6, 1.0)


def _enemy_slimelet(frame):
    """A slimelet: a fist-sized gob of the big slime, hopping."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    hop = max(0.0, math.sin(p)) * 4.0
    squash = math.cos(p * 2) * 0.12
    rx = 7.0 * (1 + squash)
    ry = 5.5 * (1 - squash)
    S.shadow_ellipse(tc, 20, feet + 1, 8 - hop * 0.5, 2.4, 0.5)
    cy = feet - ry + 1 - hop
    S.ellipsoid(tc, 20, cy, rx, ry, 0.88, spec=1.0,
                tex=lambda x, y, u, v: -0.4 if (u + 0.2) ** 2 + (v - 0.3) ** 2
                < 0.03 else 0.0)
    _blob_eyes(tc, 22, cy - 1, 2, 1.1)
    S.glint(tc, 16, int(cy - ry * 0.5), 0)
    return tc.to_canvas()


def _enemy_slime(frame):
    """The great slime. Three slimelets show through its glassy body, curled
    up and waiting: the player sees what it will burst into."""
    tc = _enemy_canvas(HUSK_CELL)
    feet = HUSK_CELL - FOOT
    p = _phase(frame)
    squash = math.sin(p)
    rx = 17 + 1.8 * squash
    ry = 13 - 1.5 * squash
    S.shadow_ellipse(tc, 25, feet + 1, rx + 3, 3.5, 0.5)
    cy = feet - ry + 1
    # A wobble that rolls over the top of the body.
    def skin(x, y, u, v):
        ripple = 0.1 * math.sin(u * 5 + p * 2) if v < -0.3 else 0.0
        return ripple
    S.ellipsoid(tc, 24, cy, rx, ry, 1.0, spec=0.6, depth=0.9, tex=skin)
    # The brood inside, seen darkly through the jelly: three curled shapes,
    # each with its own pair of pale eyes.
    for i, (bx, by, br) in enumerate(((15, 5, 4.2), (29, 4, 4.6),
                                       (22, -3, 3.8))):
        drift = math.sin(p + i * 2.1) * 1.2
        x = 24 + (bx - 22) * rx / 17 + drift
        y = cy + by * ry / 13
        S.ellipsoid(tc, x, y, br, br * 0.8, 0.22, spec=0.2)
        tc.put(x, y - 1, 0.9)
        tc.put(x + 2, y - 1, 0.9)
    # The near skin over the brood keeps a highlight on top of everything.
    S.ellipsoid(tc, 16, cy - ry * 0.6, 5, 2.2, 1.1, spec=1.0, depth=0.5)
    _blob_eyes(tc, 32, cy - ry * 0.5, 3, 1.6)
    S.glint(tc, 13, int(cy - ry * 0.65), 1)
    # Drips at the hem.
    for dx in (-10, 3, 12):
        S.capsule(tc, 24 + dx, feet - 1, 24 + dx, feet + 1, 2, 0.6)
    return tc.to_canvas()


def _enemy_burrower(frame):
    """A tunnelling grub: a ringed, plated body behind a pair of digging
    claws and a drill snout. The segments ripple as it pulls itself on."""
    tc = _enemy_canvas()
    feet = PROP_CELL - FOOT
    p = _phase(frame)
    S.shadow_ellipse(tc, 21, feet + 1, 15, 3, 0.5)

    def ring(x, y, u, v):
        return -0.35 if abs(u) > 0.78 else (0.12 if v < -0.35 else 0.0)
    # Tail to head, each segment a little bigger and riding its own wave.
    for i, (sx, size) in enumerate(((8, 4.0), (13, 5.0), (19, 6.0), (25, 6.4))):
        rise = 1.5 * math.sin(p - i * 1.1)
        S.ellipsoid(tc, sx, feet - size - 1 + rise, size, size,
                    0.72 + i * 0.05, spec=0.5, tex=ring)
    # Far claw, the head, the drill snout pointing down the road, the near
    # claw. The drill's spiral turns a notch every frame.
    dig = 2.5 * math.sin(p * 2)
    S.capsule(tc, 28, feet - 6, 34 + dig, feet - 1, 2.6, 0.45)
    S.ellipsoid(tc, 30, feet - 10, 5, 4.5, 0.8, spec=0.4)

    def spiral(x, y, _u, _v):
        return -0.45 if int(x * 0.8 - y + frame) % 3 == 0 else 0.0
    S.polygon(tc, [(33, feet - 15), (40, feet - 10), (33, feet - 5)],
              (0.4, -0.5, 0.75), 0.95, spec=0.8, tex=spiral)
    S.glint(tc, 29, feet - 12, 0)
    S.capsule(tc, 29, feet - 7, 35 - dig, feet, 3, 0.95, spec=0.4)
    for cx in (34 - dig, 36 - dig):
        tc.put(cx, feet, 0.05)
    return tc.to_canvas()


def _mound(tc, cx, feet, rx, ry, frame, seed):
    """A ridge of turned earth with clods tossed off its crest."""
    p = _phase(frame)

    def rubble(x, y, _u, _v):
        cell = S.hash01(int(x) // 3, int(y) // 2, seed + frame)
        return (cell - 0.5) * 0.45
    S.shadow_ellipse(tc, cx, feet + 1, rx + 3, ry * 0.6, 0.55)
    S.ellipsoid(tc, cx, feet - ry * 0.5, rx, ry, 0.72, depth=0.6, tex=rubble)
    for i in range(4):
        t = ((frame + i * 1.5) % WALK_FRAMES) / float(WALK_FRAMES)
        x = cx + rx * (0.9 - t * 1.6) * (1 if i % 2 else -1) * 0.6
        y = feet - ry - math.sin(t * math.pi) * ry * 1.4
        S.ellipsoid(tc, x, y, 1.6, 1.4, 0.6 + 0.1 * math.sin(p + i), spec=0.3)
    # The crack along the crest where the thing below pushes through.
    for x in range(int(cx - rx * 0.6), int(cx + rx * 0.6)):
        y = feet - ry * 1.35 + math.sin(x * 0.9 + p) * 0.8
        tc.put(x, y, 0.05)


def _burrower_mound(frame):
    tc = _enemy_canvas()
    _mound(tc, 20, PROP_CELL - FOOT, 11, 5, frame, 71)
    return tc.to_canvas()


def _enemy_golem(frame, cracked=False):
    """A stone golem in a shell of rune-cut slabs. Cracked, the slabs are
    gone and its glowing core shows through the seams."""
    tc = _enemy_canvas(HUSK_CELL)
    feet = HUSK_CELL - FOOT
    p = _phase(frame)
    swing = (3.5 if cracked else 2.5) * math.sin(p)
    bob = -1.6 * abs(math.sin(p))
    S.shadow_ellipse(tc, 24, feet + 1, 17, 4, 0.5)

    def stone(x, y, _u, _v):
        cell = S.hash01(int(x) // 4, int(y) // 4, 33)
        seam = int(x) % 4 == 0 and int(y) % 4 == 0
        return (cell - 0.5) * 0.16 - (0.2 if seam else 0.0)

    def core(x, y, _u, _v):
        # Rune light burning in the cracks of the bare core.
        lit_seam = (int(x * 0.7 + y) % 5 == 0) or (int(x - y * 0.6) % 6 == 0)
        return 0.9 if lit_seam else -0.1

    # Stumpy legs and the far arm.
    _legs(tc, 24, 32 + bob, feet, swing, width=7, near=0.7, far=0.35)
    S.capsule(tc, 34, 18 + bob, 39, 30 + bob - swing, 6, 0.4, tex=stone)
    S.ellipsoid(tc, 39, 32 + bob - swing, 5, 4.5, 0.45, tex=stone)
    if cracked:
        S.ellipsoid(tc, 24, 24 + bob, 11, 11, 0.22, spec=0.3, tex=core)
        # One slab still clings on; chips of the rest drift loose round the
        # shoulders, rising and falling out of step with the stride.
        S.box(tc, 13, 17 + bob, 7, 9, 3, 0.85, tex=stone, top_albedo=1.0)
        for i, (cx, cy) in enumerate(((35, 10), (12, 6), (38, 20))):
            drift = 2 * math.sin(p + i * 2.1)
            S.box(tc, cx, cy + drift, 3, 3, 1, 0.9, top_albedo=1.1)
        S.ellipsoid(tc, 24, 11 + bob, 6, 5.5, 0.7, tex=stone)
    else:
        # The shell: three slabs over the chest and a heavy brow.
        S.ellipsoid(tc, 24, 24 + bob, 13, 12, 0.8, tex=stone)
        S.box(tc, 14, 16 + bob, 9, 10, 3, 0.95, tex=stone, top_albedo=1.15)
        S.box(tc, 24, 16 + bob, 9, 10, 3, 0.88, tex=stone, top_albedo=1.1)
        S.box(tc, 17, 27 + bob, 13, 6, 3, 0.85, tex=stone, top_albedo=1.05)
        # A rune cut into the middle slab.
        for dy in range(3, 8):
            tc.put(28, 16 + bob + dy, 0.08)
        tc.put(27, 18 + bob, 0.08)
        tc.put(29, 21 + bob, 0.08)
        S.ellipsoid(tc, 24, 10 + bob, 7, 6, 0.85, tex=stone)
        _rect(tc, 18, 7 + bob, 31, 9 + bob, 0.6)
    # The eye slit burns either way.
    _rect(tc, 20, 11 + bob, 29, 13 + bob, 0.05)
    S.glint(tc, 22, int(12 + bob), 0)
    S.glint(tc, 27, int(12 + bob), 0)
    # The near arm and its boulder fist.
    S.capsule(tc, 13, 18 + bob, 9, 30 + bob + swing, 7, 0.85, tex=stone)
    S.ellipsoid(tc, 9, 33 + bob + swing, 6, 5.5, 0.9, spec=0.3, tex=stone)
    return tc.to_canvas()


def _enemy_wyrm(frame):
    """The mine's wyrm, surfacing: a plated body arcing out of the road with
    its head reared and its mandibles working. The tail end dives back
    into the earth, so it always reads as half buried."""
    tc = _enemy_canvas(TITAN_CELL)
    feet = TITAN_CELL - FOOT
    p = _phase(frame)
    S.shadow_ellipse(tc, 40, feet + 2, 32, 6, 0.5)

    def plate(x, y, u, v):
        band = -0.4 if abs(u) > 0.8 else 0.0
        return band + (0.12 if v < -0.4 else 0.0)

    # The body: segments along an arch from the burrow to the raised head.
    count = 9
    for i in range(count):
        t = i / float(count - 1)
        x = 12 + t * 44
        arch = math.sin(t * math.pi * 0.5) * 36
        sway = math.sin(p + t * 3.0) * 2.0
        y = feet - 4 - arch + sway * t
        size = 6.5 + t * 2.5
        S.ellipsoid(tc, x, y, size, size * 0.95, 0.62 + t * 0.25, spec=0.4,
                    tex=plate)
        # A dorsal spine on every plate, the tallest over the shoulders.
        S.cone(tc, x - 1, y - size - 3 - 3 * t, y - size + 2, 2.4, 0.95,
               spec=0.4)
    # The head: a plated skull with a swept-back crest, and a pair of long
    # hooked mandibles opening and closing.
    head_x, head_y = 63, feet - 40 + math.sin(p) * 1.5
    S.polygon(tc, [(head_x - 8, head_y - 4), (head_x - 15, head_y - 12),
                   (head_x - 2, head_y - 7)], (-0.2, -0.7, 0.7), 0.9)
    S.ellipsoid(tc, head_x, head_y, 9, 7.5, 0.92, spec=0.6, tex=plate)
    gape = 2 + 3 * (0.5 + 0.5 * math.sin(p * 2))
    for side in (-1, 1):
        root_y = head_y + side * 2.5
        reach_y = head_y + side * (3 + gape)
        S.capsule(tc, head_x + 5, root_y, head_x + 15, reach_y, 3.6,
                  0.65 if side < 0 else 0.9, spec=0.6)
        S.capsule(tc, head_x + 15, reach_y, head_x + 17, reach_y - side * 4,
                  2.2, 0.65 if side < 0 else 0.95, spec=0.6)
    S.ellipsoid(tc, head_x + 8, head_y, 2.5, 1.5 + gape * 0.4, 0.04)
    for ex, ey in ((head_x + 1, head_y - 3), (head_x + 5, head_y - 4)):
        tc.put(ex, ey, 0.02)
        S.glint(tc, ex, int(ey) - 1, 0)
    # Rubble heaped where the tail goes into the ground.
    def rubble(x, y, _u, _v):
        return (S.hash01(int(x) // 3, int(y) // 2, 91) - 0.5) * 0.4
    S.ellipsoid(tc, 12, feet - 2, 12, 5, 0.75, depth=0.6, tex=rubble)
    S.occlude(tc, feet, 8, 0.3)
    return tc.to_canvas()


def _wyrm_mound(frame):
    """The wyrm under the road: a long ridge of heaved earth, its spines
    breaking the surface as it goes."""
    tc = _enemy_canvas(TITAN_CELL)
    feet = TITAN_CELL - FOOT
    p = _phase(frame)
    _mound(tc, 40, feet, 30, 13, frame, 113)
    for i, x in enumerate((24, 36, 48, 58)):
        rise = 2.5 + 2.5 * math.sin(p - i * 1.3)
        S.cone(tc, x, feet - 26 - rise, feet - 17, 3.2, 0.95, spec=0.5)
    return tc.to_canvas()


KING_STAFF_TIP = (54, 12)   # matches the freeze beam in frostline_model.c


def _enemy_king(frame):
    """The frost king: a towering figure in a fur-trimmed cape and rimed
    plate, a crown of ice prisms, and the staff whose crystal freezes the
    towers. He glides more than he walks."""
    tc = _enemy_canvas(TITAN_CELL)
    feet = TITAN_CELL - FOOT
    p = _phase(frame)
    sway = 1.5 * math.sin(p)
    S.shadow_ellipse(tc, 38, feet + 2, 22, 5, 0.55)

    # The staff stands behind the cape, its crystal held high.
    sx, sy = KING_STAFF_TIP
    S.capsule(tc, sx - 2, feet - 1, sx, sy + 8, 2.4, 0.55,
              tex=S.wood(seed=9, spacing=3))
    crystal(tc, sx, sy - 6 + sway * 0.3, sy + 8, 4.2, albedo=1.1)
    S.glint(tc, sx - 1, int(sy - 2), 1)
    # The cape: a long dark cone with deep folds, its train streaming back
    # in the wind and a hem of white fur dragging through the snow.
    trail = 2 * math.sin(p + 1.0)
    S.polygon(tc, [(24, 20), (6, feet - 10 + trail), (4, feet - 2 + trail),
                   (22, feet - 1)], (-0.5, 0.2, 0.85), 0.28,
              tex=S.folds(5, 0.2))
    S.cone(tc, 36 + sway * 0.5, 14, feet - 1, 21 + sway * 0.5, 0.32,
           tex=S.folds(9, 0.2))
    S.capsule(tc, 6, feet - 3 + trail, 22, feet - 2, 3.5, 1.0,
              tex=S.fur(14, 0.2))
    S.capsule(tc, 17, feet - 2, 56, feet - 2, 4, 1.0, tex=S.fur(15, 0.2))
    # Rimed breastplate and the fur mantle over the shoulders.
    S.ellipsoid(tc, 36, 36, 11, 15, 0.9, spec=0.8, tex=S.folds(3, 0.1))
    _rect(tc, 30, 36, 43, 37, 0.15)
    _rect(tc, 30, 42, 43, 43, 0.15)
    S.ellipsoid(tc, 36, 23, 15, 6, 1.0, tex=S.fur(12, 0.18))
    # The head: a dark face under the helm, eyes white-hot.
    S.ellipsoid(tc, 36, 16, 6.5, 7, 0.85, spec=0.9)
    _rect(tc, 32, 15, 41, 20, 0.04)
    S.glint(tc, 34, 17, 0)
    S.glint(tc, 39, 17, 0)
    # The crown of ice.
    for cx, top, half, lean in ((30, 3, 2.2, -1.5), (36, 0, 2.8, 0),
                                (42, 3, 2.2, 1.5)):
        crystal(tc, cx, top + sway * 0.2, 11, half, albedo=1.05, lean=lean)
    # The hand on the staff and the near arm under the mantle.
    S.capsule(tc, 45, 26, 51, 38, 4.5, 0.8, spec=0.5)
    S.ellipsoid(tc, sx - 2, 38, 3, 3, 0.9, spec=0.6)
    S.capsule(tc, 26, 26, 22, 44 + sway, 4.5, 0.85, spec=0.5)
    S.ellipsoid(tc, 22, 46 + sway, 3, 3, 0.9)
    S.occlude(tc, feet, 6, 0.3)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Hero (48 px cells)
# ----------------------------------------------------------------------


def _hero_warden(frame):
    """The warden: a knight in a dark cloak with a spear and kite shield."""
    tc = S.ToneCanvas(HERO_CELL, HERO_CELL)
    feet = HERO_CELL - FOOT
    p = _phase(frame, 4)
    breath = 0.8 * math.sin(p)
    S.shadow_ellipse(tc, 25, feet + 1, 12, 3.5, 0.5)

    # The cloak hangs behind and stirs in the wind.
    S.cone(tc, 24 + breath * 0.5, 12, feet - 1, 11 + breath * 0.5, 0.45,
           tex=S.folds(6, 0.18))
    # Spear: ash haft and a steel head that catches the light.
    S.capsule(tc, 34, feet, 34, 7 + breath, 1.8, 0.65,
              tex=S.wood(seed=6, spacing=3))
    S.polygon(tc, [(34, 2 + breath), (37, 7 + breath), (34, 11 + breath),
                   (31, 7 + breath)], (-0.3, -0.3, 1.0), 1.0, spec=0.9)
    _legs(tc, 23, 30, feet, 0.0, width=4, near=0.7, far=0.35)
    # Breastplate and helm.
    S.ellipsoid(tc, 24, 22 + breath * 0.3, 8, 9, 0.95, spec=0.7,
                tex=S.folds(2, 0.08))
    S.capsule(tc, 17, 30, 31, 30, 2.2, 0.4)
    S.ellipsoid(tc, 24, 11, 5, 5.5, 0.95, spec=0.9)
    _rect(tc, 21, 11, 28, 12, 0.04)
    S.capsule(tc, 24, 5, 20, 1 + breath, 2.2, 1.0, tex=S.fur(8))
    S.ellipsoid(tc, 33, 20, 2.4, 2.4, 0.9)
    # Kite shield with a frost-star device.
    shield = [(10, 16), (20, 16), (20, 26), (15, 34), (10, 26)]
    S.polygon(tc, shield, (-0.4, -0.2, 1.0), 1.0, spec=0.3)
    S.polygon(tc, [(15, 16), (20, 16), (20, 26), (15, 34)],
              (0.25, -0.1, 1.0), 0.95)
    for dx, dy in ((0, 0), (-2, 0), (2, 0), (0, -2), (0, 2)):
        tc.put(15 + dx, 23 + dy, 0.05)
    return tc.to_canvas()


SOLDIER_WALK_FRAMES = 4


def _soldier(frame, strike=False):
    """A barracks footman: nasal helm, paper tabard over mail, a round
    shield on the near arm and a spear. Smaller than a raider on purpose,
    so the garrison reads as the player's own little men."""
    tc = S.ToneCanvas(PROP_CELL, PROP_CELL)
    feet = PROP_CELL - FOOT
    p = _phase(frame, SOLDIER_WALK_FRAMES)
    swing = 0.0 if strike else 2.4 * math.sin(p)
    bob = 0.0 if strike else -0.8 * abs(math.sin(p))
    cx = 20
    S.shadow_ellipse(tc, cx + 1, feet + 1, 8, 2.5, 0.5)

    _legs(tc, cx, 27 + bob, feet, swing, width=3.0, near=0.55, far=0.3)
    # Spear behind the body: level on the march, thrust out on a strike.
    if strike:
        S.capsule(tc, cx - 2, 22, cx + 17, 19, 1.4, 0.65)
        S.polygon(tc, [(cx + 16, 17), (cx + 21, 19), (cx + 16, 21)],
                  (0.0, -0.3, 1.0), 1.0, spec=0.9)
    else:
        S.capsule(tc, cx + 5, feet - 1, cx + 6, 9 + bob, 1.4, 0.6)
        S.polygon(tc, [(cx + 6, 4 + bob), (cx + 8, 9 + bob),
                       (cx + 6, 11 + bob), (cx + 4, 9 + bob)],
                  (-0.3, -0.3, 1.0), 1.0, spec=0.9)
    # Mail body under a pale tabard with a dark frost-star.
    lean = 1.5 if strike else 0.0
    S.ellipsoid(tc, cx + lean, 21 + bob, 5.5, 6.5, 0.6, tex=S.fur(71, 0.1))
    S.polygon(tc, [(cx - 3 + lean, 16 + bob), (cx + 3 + lean, 16 + bob),
                   (cx + 4 + lean, 28 + bob), (cx - 4 + lean, 28 + bob)],
              (-0.2, 0.0, 1.0), 1.1, tex=S.folds(3, 0.1))
    for dx, dy in ((0, 0), (-1, 0), (1, 0), (0, -1), (0, 1)):
        tc.put(int(cx + lean + dx), int(21 + bob + dy), 0.05)
    # Helm with a nasal guard.
    S.ellipsoid(tc, cx + lean, 11 + bob, 4, 4.2, 0.95, spec=0.8)
    _rect(tc, cx - 3 + lean, 12 + bob, cx + 4 + lean, 13 + bob, 0.1)
    _rect(tc, cx + lean, 11 + bob, cx + 1 + lean, 15 + bob, 0.75)
    # The shield on the near arm, braced forward on a strike.
    shield_x = cx - 5 + (2 if strike else 0)
    S.ellipsoid(tc, shield_x, 21 + bob, 4.2, 5, 0.95, depth=0.5, spec=0.3)
    S.ellipsoid(tc, shield_x, 21 + bob, 1.2, 1.2, 1.2, spec=0.8)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Build pads, projectiles and sparks
# ----------------------------------------------------------------------


def _build_pad(frame):
    """A ring of flagstones marking ground that will take a tower.

    Frame 1 lifts the stones' tone so the pads pulse while a card is armed.
    """
    tc = S.ToneCanvas(PROP_CELL, PROP_CELL)
    cx, cy = 20, 20
    lift = 0.12 if frame else 0.0

    def flags(x, y, u, v):
        angle = math.atan2(v, u)
        seam = (angle / (2 * math.pi) * 9) % 1.0 < 0.12
        ring = math.sqrt(u * u + v * v)
        if ring < 0.55:
            return -1.0
        return (-0.4 if seam else 0.0) + lift
    S.ellipsoid(tc, cx, cy, 16, 9, 0.95, depth=0.3, tex=flags)
    # The open centre is trodden snow, a shade below the map paper.
    for y in range(cy - 6, cy + 7):
        for x in range(cx - 10, cx + 11):
            u = (x + 0.5 - cx) / 8.8
            v = (y + 0.5 - cy) / 4.95
            if u * u + v * v <= 1.0:
                tc.put(x, y, 0.82 + lift + (0.0 if v > 0 else -0.25 * (1 + v)))
    if frame:
        for gx, gy in ((cx - 13, cy - 2), (cx + 12, cy + 3), (cx, cy - 8)):
            S.glint(tc, gx, gy, 0)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Campaign map medallions (40 px cells, in the props sheet)
# ----------------------------------------------------------------------
# A stone token marks every stage on the campaign map. The emblem in relief
# names the stage's landscape, in the order of levels.CAMPAIGN_NODES.

MEDALLION_CX, MEDALLION_CY, MEDALLION_R = 20, 18, 15


def _medallion_disc(tc, albedo):
    """A domed stone token with a raised rim, lit from the top left."""
    cx, cy, r = MEDALLION_CX, MEDALLION_CY, MEDALLION_R
    S.shadow_ellipse(tc, cx + 2, cy + r - 1, r, 4, 0.6)

    def rim(x, y, u, v):
        ring = math.sqrt(u * u + v * v)
        if ring > 0.84:
            return 0.15
        if ring > 0.74:
            return -0.55   # the groove inside the rim
        return 0.0
    S.ellipsoid(tc, cx, cy, r, r - 1, albedo, spec=0.25, depth=0.4, tex=rim)


def _emblem_forest(tc):
    for x, top, h in ((15, 10, 15), (25, 12, 13), (20, 7, 18)):
        S.cone(tc, x, top, top + h - 3, 4.5, 0.95, tex=S.fur(x, depth=0.1))
        S.capsule(tc, x, top + h - 3, x, top + h - 1, 1.0, 0.4)


def _emblem_lake(tc):
    S.ellipsoid(tc, 20, 20, 9, 5, 0.25, spec=0.9, depth=0.2)
    for x0, x1, y in ((14, 19, 19), (21, 26, 21)):
        S.capsule(tc, x0, y, x1, y, 0.6, 1.1)
    S.capsule(tc, 12, 14, 13, 10, 0.7, 0.9)   # a reed on the shore
    S.capsule(tc, 27, 15, 28, 11, 0.7, 0.9)


def _emblem_peaks(tc):
    S.polygon(tc, [(9, 26), (17, 10), (21, 18)], (-0.7, -0.3, 0.65), 1.0)
    S.polygon(tc, [(17, 10), (21, 18), (25, 26), (9, 26)], (0.7, 0.1, 0.7), 0.6)
    S.polygon(tc, [(19, 26), (25, 14), (31, 26)], (0.6, 0.0, 0.8), 0.7)
    S.polygon(tc, [(19, 26), (25, 14), (24, 26)], (-0.7, -0.3, 0.65), 1.05)
    S.glint(tc, 17, 10, 0)


def _emblem_mine(tc):
    S.capsule(tc, 12, 26, 27, 11, 1.2, 0.75)      # the handles cross
    S.capsule(tc, 28, 26, 13, 11, 1.2, 0.65)
    S.capsule(tc, 22, 8, 32, 14, 1.6, 1.0, spec=0.6)   # the pick heads
    S.capsule(tc, 8, 14, 18, 8, 1.6, 1.0, spec=0.6)


def _emblem_glacier(tc):
    crystal(tc, 20, 7, 27, 5, albedo=1.05)
    crystal(tc, 13, 14, 27, 3.5, albedo=0.95, lean=-1)
    crystal(tc, 27, 15, 27, 3.5, albedo=0.95, lean=1)


def _emblem_ruins(tc):
    S.cylinder(tc, 15, 10, 27, 3.5, 0.95, tex=S.masonry(3, course=4, blocks=4))
    S.cylinder(tc, 25, 16, 27, 3.5, 0.9, tex=S.masonry(3, course=4, blocks=4,
                                                        seed=3))
    S.box(tc, 11, 8, 9, 3, 2, 1.0, top_albedo=1.2)   # a lintel still in place
    S.ellipsoid(tc, 21, 27, 3, 1.5, 0.7)            # a fallen drum


def _emblem_falls(tc):
    S.box(tc, 10, 9, 20, 4, 3, 0.9, top_albedo=1.25)    # the lip of the cliff
    for x in (13, 16, 19, 22, 25):
        S.capsule(tc, x, 13, x + (x % 2), 25, 0.9, 1.05, spec=0.5)
    S.ellipsoid(tc, 20, 27, 10, 2.5, 0.8, spec=0.4)    # the plunge pool


def _emblem_fortress(tc):
    S.box(tc, 12, 17, 16, 10, 3, 0.85, top_albedo=1.2)
    for x, top in ((12, 12), (20, 7), (28, 12)):
        S.cylinder(tc, x, top + 4, 27, 3, 0.95)
        S.cone(tc, x, top - 4, top + 4, 4, 1.0, spec=0.4)
    _gate_arch(tc, 20, 27, 3, 5)


def _gate_arch(tc, cx, base, half, height):
    for y in range(base - height, base):
        for x in range(cx - half, cx + half):
            tc.put(x, y, 0.05)


def _emblem_lock(tc):
    """A padlock: the stage is not open yet."""
    for i in range(12):
        angle = math.pi + i * math.pi / 11
        x = 20 + math.cos(angle) * 4.5
        y = 15 + math.sin(angle) * 5
        S.capsule(tc, x, y, x, y, 1.1, 0.85, spec=0.5)
    S.box(tc, 13, 15, 14, 11, 2, 0.8, top_albedo=1.1)
    _gate_arch(tc, 20, 23, 1, 5)


MEDALLION_EMBLEMS = (
    _emblem_forest, _emblem_lake, _emblem_peaks, _emblem_mine,
    _emblem_glacier, _emblem_ruins, _emblem_falls, _emblem_fortress,
)


def _medallion(emblem, locked=False):
    """The token with its emblem sunk into the face.

    At 40 px a mid-grey emblem on a mid-grey stone dithers into mush, so the
    face is kept bright and the emblem is pressed down to a dark relief that
    still carries its own lit and shaded sides.
    """
    tc = S.ToneCanvas(PROP_CELL, PROP_CELL)
    _medallion_disc(tc, 0.8 if locked else 1.2)
    relief = S.ToneCanvas(PROP_CELL, PROP_CELL)
    emblem(relief)
    depth = 0.3 if locked else 0.42
    for y in range(PROP_CELL):
        for x in range(PROP_CELL):
            tone = relief.get(x, y)
            if tone is not None:
                tc.put(x, y, min(1.0, tone) * depth)
    return tc.to_canvas()


def _projectile(frame):
    """A crossbow bolt in flight; the fletching flickers between frames."""
    tc = S.ToneCanvas(FX_CELL, FX_CELL)
    S.capsule(tc, 3, 8, 12, 8, 1.6, 0.8)
    S.polygon(tc, [(12, 6), (15, 8), (12, 10)], (0.0, -0.3, 1.0), 1.0,
              spec=0.8)
    spread = 2 + frame % 2
    S.polygon(tc, [(1, 8 - spread), (5, 8), (1, 8 + spread)],
              (-0.4, -0.2, 1.0), 0.9)
    return tc.to_canvas()


def _spark(frame):
    """Impact: a hot flash that breaks into flying flecks."""
    tc = S.ToneCanvas(FX_CELL, FX_CELL)
    if frame < 3:
        radius = 2.5 + frame * 1.5
        S.ellipsoid(tc, 8, 8, radius, radius, 1.1, spec=0.6)
    for i in range(6):
        angle = i / 6.0 * 2 * math.pi + frame * 0.3
        reach = 2.0 + frame * 1.1
        x = 8 + math.cos(angle) * reach
        y = 8 + math.sin(angle) * reach
        S.glint(tc, int(x), int(y), 0)
    return tc.to_canvas(outline=frame < 4)


EMBER_FRAMES = 4


def _ember_pot(frame):
    """A clay fire pot tumbling through the air, its rag wick streaming
    flame. Each frame turns it a quarter round."""
    tc = S.ToneCanvas(FX_CELL, FX_CELL)
    angle = frame / float(EMBER_FRAMES) * 2 * math.pi
    cx, cy = 8, 9
    S.ellipsoid(tc, cx, cy, 4.2, 4.2, 0.6, spec=0.4,
                tex=lambda x, y, u, v: -0.3 if abs(v - 0.2) < 0.12 else 0.0)
    nx, ny = math.cos(angle - math.pi / 2), math.sin(angle - math.pi / 2)
    S.capsule(tc, cx + nx * 3, cy + ny * 3, cx + nx * 6, cy + ny * 6, 2.4,
              1.15)
    S.glint(tc, int(cx + nx * 6), int(cy + ny * 6), 0)
    return tc.to_canvas()


def _ember_flame(frame):
    """A small tongue of fire on burning ground, licking from side to side."""
    tc = S.ToneCanvas(FX_CELL, FX_CELL)
    p = _phase(frame, EMBER_FRAMES)
    cx, base = 8, 14
    lean = 1.4 * math.sin(p)
    height = 10 + 2 * math.sin(p * 2)
    S.cone(tc, cx + lean, base - height, base, 3.8, 0.75, lid=0.5)
    S.cone(tc, cx + lean * 1.3, base - height * 0.65, base, 2.0, 1.25,
           lid=0.5)
    rise = (frame * 3) % 6
    S.glint(tc, int(cx + lean * 2 + 2), int(base - height - rise), 0)
    return tc.to_canvas()


# ----------------------------------------------------------------------
# Icons (28 px cells)
# ----------------------------------------------------------------------


def _icon_canvas():
    return S.ToneCanvas(ICON_CELL, ICON_CELL)


def _icon_heart():
    tc = _icon_canvas()
    S.ellipsoid(tc, 9.5, 10, 6, 6, 0.95, spec=0.8)
    S.ellipsoid(tc, 18.5, 10, 6, 6, 0.9, spec=0.5)
    S.polygon(tc, [(3.6, 12), (24.4, 12), (14, 24)], (0.1, 0.2, 1.0), 0.85)
    S.glint(tc, 7, 7, 1)
    return tc.to_canvas(halo=False)


def _icon_coin():
    """A short stack of coins, the top one face up."""
    tc = _icon_canvas()

    def milled(x, _y, _u, _v):
        return -0.25 if int(x) % 2 == 0 else 0.0
    for i, y in enumerate((22, 18, 14)):
        S.cylinder(tc, 14 - i, y - 3, y, 9, 0.95, tex=milled, lid=0.42,
                   cap=1.0, spec=0.5)
    S.ellipsoid(tc, 12, 11, 4, 1.6, 0.5)
    S.glint(tc, 8, 10, 1)
    return tc.to_canvas(halo=False)


def _icon_score():
    """A trophy cup: score is a prize, never confused with the coin."""
    tc = _icon_canvas()
    S.box(tc, 8, 22, 12, 3, 3, 0.85, top_albedo=1.0)
    S.capsule(tc, 14, 15, 14, 22, 2.4, 0.9, spec=0.4)
    S.ellipsoid(tc, 6, 9, 3, 4, 0.7)
    S.ellipsoid(tc, 22, 9, 3, 4, 0.6)
    S.ellipsoid(tc, 6, 9, 1.4, 2.4, -1.0)
    S.ellipsoid(tc, 22, 9, 1.4, 2.4, -1.0)
    S.cylinder(tc, 14, 4, 10, 8, 1.0, lid=0.35, cap=0.4, spec=0.8)
    S.ellipsoid(tc, 14, 11, 7.5, 5, 1.0, spec=0.8)
    # The handles' holes are cut out after shading.
    for y in range(ICON_CELL):
        for x in range(ICON_CELL):
            if tc.get(x, y) is not None and tc.get(x, y) < 0.01 and (
                    x < 8 or x > 20):
                tc.tone[y * ICON_CELL + x] = None
    S.glint(tc, 10, 8, 1)
    return tc.to_canvas(halo=False)


def _icon_star():
    """A five pointed star cut in facets: each arm has a lit and dark half."""
    tc = _icon_canvas()
    cx, cy = 14, 15
    outer, inner = 12.5, 5.2
    points = []
    for i in range(10):
        angle = -math.pi / 2 + i * math.pi / 5
        radius = outer if i % 2 == 0 else inner
        points.append((cx + math.cos(angle) * radius,
                       cy + math.sin(angle) * radius))
    for i in range(10):
        a = points[i]
        b = points[(i + 1) % 10]
        mid_angle = -math.pi / 2 + (i + 0.5) * math.pi / 5
        normal = (math.cos(mid_angle) * 0.7, math.sin(mid_angle) * 0.7, 0.75)
        S.polygon(tc, [(cx, cy), a, b], normal, 1.05, spec=0.4)
    return tc.to_canvas(halo=False)


def _icon_nova():
    """The frost nova: four ice shards bursting from a bright core."""
    tc = _icon_canvas()
    crystal(tc, 14, 1, 13, 3.5)
    for angle, length in ((0, 11), (math.pi, 11), (math.pi / 2, 10)):
        x = 14 + math.cos(angle) * length
        y = 14 + math.sin(angle) * length
        S.capsule(tc, 14, 14, x, y, 3.4, 0.95, spec=0.6)
    S.ellipsoid(tc, 14, 14, 5, 5, 1.1, spec=0.9)
    S.glint(tc, 12, 12, 1)
    return tc.to_canvas(halo=False)


def _icon_flag():
    tc = _icon_canvas()
    S.capsule(tc, 7, 25, 7, 3, 2, 0.6)
    S.polygon(tc, [(8, 4), (23, 8), (8, 15)], (-0.2, -0.1, 1.0), 0.95,
              tex=S.folds(3))
    return tc.to_canvas(halo=False)


def _card_icon(builder, *args):
    """A sprite rendered at full size, then shrunk in tone before dithering.

    Shrinking the tones keeps the shading; shrinking dithered art would
    turn it into noise.
    """
    full = builder(*args)
    tc = S.ToneCanvas(full.width, full.height)
    for y in range(full.height):
        for x in range(full.width):
            value = full.get(x, y)
            if value >= 0:
                tc.put(x, y, value / 255.0)
    return tc.resized(ICON_CELL, ICON_CELL).to_canvas()


# ----------------------------------------------------------------------
# Castle moving parts (28 px cells)
# ----------------------------------------------------------------------

CASTLE_FLAG_FRAMES = 6
CASTLE_FLAME_FRAMES = 4


def _castle_flag(frame):
    """The hold's banner, rippling. The pole is at the right edge and the
    cloth flies left, out over the open snow where it reads, instead of
    over the cliff hatching."""
    tc = S.ToneCanvas(CASTLE_FX_CELL, CASTLE_FX_CELL)
    p = _phase(frame, CASTLE_FLAG_FRAMES)
    columns = 8
    pole = CASTLE_FX_CELL - 2
    top, bottom = [], []
    for i in range(columns):
        x = pole - i * 3.2
        wave = math.sin(p - i * 0.8) * (0.5 + i * 0.35)
        top.append((x, 3 + wave))
        bottom.append((x, 14 + wave + i * 0.25 - (3 if i == columns - 1 else 0)))
    cloth = top + list(reversed(bottom))

    def ripple(x, _y, _u, _v):
        return 0.35 * math.cos(p - (pole - x) / 3.2 * 0.8)
    S.polygon(tc, cloth, (-0.2, -0.1, 1.0), 0.75, tex=ripple)
    # A paper frost star on the dark cloth.
    star_x = int(pole - 10 + math.sin(p - 3 * 0.8) * 1.2)
    star_y = int(8 + math.sin(p - 3 * 0.8) * 1.4)
    for dx, dy in ((0, 0), (-1, 0), (1, 0), (0, -1), (0, 1)):
        tc.put(star_x + dx, star_y + dy, 1.0)
    return tc.to_canvas()


def _castle_flame(frame):
    """Brazier fire: a licking tongue with a white heart."""
    tc = S.ToneCanvas(CASTLE_FX_CELL, CASTLE_FX_CELL)
    p = _phase(frame, CASTLE_FLAME_FRAMES)
    cx, base = 14, 24
    lean = 1.8 * math.sin(p)
    height = 15 + 2.5 * math.sin(p * 2)
    S.ellipsoid(tc, cx, base - 3, 5.5, 4, 0.8)
    S.cone(tc, cx + lean, base - height, base - 2, 5.5, 0.7, lid=0.5)
    S.cone(tc, cx + lean * 1.3, base - height * 0.72, base - 2, 3.2, 1.2,
           lid=0.5)
    S.ellipsoid(tc, cx, base - 4, 2.5, 2, 1.2)
    # Loose sparks rising off the tip.
    for i in range(2):
        rise = (frame * 3 + i * 6) % 10
        S.glint(tc, int(cx + lean * 2 + (i * 2 - 1) * 2), int(base - height - rise), 0)
    return tc.to_canvas(outline=True)


# ----------------------------------------------------------------------
# Sheet assembly
# ----------------------------------------------------------------------


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
    """Every tower frame (artgen/tower_art.py): the still bodies, the
    catapult's poses facing either way and the kiln's flickering fire."""
    return _build_grid(UNIT_CELL, tower_art.unit_entries())


def build_parts():
    """What the view moves on a tower by itself: the ballista heads, the
    spare fire pots, the orbiting stones and the barracks' open door."""
    return _build_grid(tower_art.PART_CELL, tower_art.part_entries())


# The roster, in the order the level data lists them. The husk and the titan
# are bigger than the rank and file, so they live in their own sheets: every
# cell in a sheet has to be the same size or the packer rescales the art.
# A sprite key is "enemy_<key>_<frame>"; the alternate looks of one enemy
# carry a suffix: "golem_cracked" is the golem out of its shell, "<key>_under"
# the ridge of earth a burrowing enemy pushes up under the road.
SMALL_ENEMY_BUILDERS = [
    ("wolf", _enemy_wolf),
    ("raider", _enemy_raider),
    ("skitter", _enemy_skitter),
    ("shaman", _enemy_shaman),
    ("banner", _enemy_banner),
    ("sap", _enemy_sap),
    ("crow", _enemy_crow),
    ("slimelet", _enemy_slimelet),
    ("burrower", _enemy_burrower),
    ("burrower_under", _burrower_mound),
]
HUSK_ENEMY_BUILDERS = [
    ("husk", _enemy_husk),
    ("golem", _enemy_golem),
    ("golem_cracked", lambda frame: _enemy_golem(frame, cracked=True)),
    ("slime", _enemy_slime),
]
TITAN_ENEMY_BUILDERS = [
    ("titan", _enemy_titan),
    ("wyrm", _enemy_wyrm),
    ("wyrm_under", _wyrm_mound),
    ("king", _enemy_king),
]
ENEMY_BUILDERS = SMALL_ENEMY_BUILDERS + HUSK_ENEMY_BUILDERS + TITAN_ENEMY_BUILDERS


def _enemy_entries(builders):
    return [(f"enemy_{name}_{frame}", builder(frame))
            for name, builder in builders for frame in range(WALK_FRAMES)]


def build_props():
    """The rank and file and the build pads, all on one 40 pixel grid."""
    entries = _enemy_entries(SMALL_ENEMY_BUILDERS[:6])
    entries.append(("build_pad_0", _build_pad(0)))
    entries.append(("build_pad_1", _build_pad(1)))
    for index, emblem in enumerate(MEDALLION_EMBLEMS):
        entries.append((f"node_{index}", _medallion(emblem)))
    entries.append(("node_locked", _medallion(_emblem_lock, locked=True)))
    for frame in range(SOLDIER_WALK_FRAMES):
        entries.append((f"soldier_{frame}", _soldier(frame)))
    entries.append(("soldier_strike", _soldier(0, strike=True)))
    # The deeper roster goes last, so the cells above keep their places.
    entries += _enemy_entries(SMALL_ENEMY_BUILDERS[6:])
    return _build_grid(PROP_CELL, entries)


def build_props_husk():
    """The heavy rank: the husk, the golem and the great slime, on a 48
    pixel grid."""
    return _build_grid(HUSK_CELL, _enemy_entries(HUSK_ENEMY_BUILDERS),
                       columns=WALK_FRAMES)


def build_props_titan():
    """The bosses, on their own 80 pixel grid."""
    return _build_grid(TITAN_CELL, _enemy_entries(TITAN_ENEMY_BUILDERS),
                       columns=WALK_FRAMES)


def build_heroes():
    """The hero sheet, on its own 48 pixel grid."""
    entries = [(f"hero_warden_{frame}", _hero_warden(frame))
               for frame in range(4)]
    return _build_grid(HERO_CELL, entries, columns=4)


def build_fx():
    entries = []
    for frame in range(3):
        entries.append((f"projectile_0_{frame}", _projectile(frame)))
    for frame in range(6):
        entries.append((f"spark_{frame}", _spark(frame)))
    for frame in range(EMBER_FRAMES):
        entries.append((f"ember_pot_{frame}", _ember_pot(frame)))
    for frame in range(EMBER_FRAMES):
        entries.append((f"ember_flame_{frame}", _ember_flame(frame)))
    return _build_grid(FX_CELL, entries)


def build_icons():
    """HUD glyphs and the build card portraits, on a 28 pixel grid."""
    entries = [
        ("icon_heart", _icon_heart()),
        ("icon_coin", _icon_coin()),
        ("icon_score", _icon_score()),
        ("icon_star", _icon_star()),
        ("icon_nova", _icon_nova()),
        ("icon_flag", _icon_flag()),
        ("card_guard", _card_icon(tower_art.card, "guard", 1)),
        ("card_frost", _card_icon(tower_art.card, "frost", 1)),
        ("card_siege", _card_icon(tower_art.card, "siege", 1)),
        ("card_hero", _card_icon(_hero_warden, 0)),
    ]
    for key in ("storm", "rune", "barracks", "ember"):
        entries.append((f"card_{key}", _card_icon(tower_art.card, key, 2)))
    return _build_grid(ICON_CELL, entries)


def build_castle_fx():
    """The castle's banner and brazier fire, which move every frame."""
    entries = [(f"castle_flag_{frame}", _castle_flag(frame))
               for frame in range(CASTLE_FLAG_FRAMES)]
    entries += [(f"castle_flame_{frame}", _castle_flame(frame))
                for frame in range(CASTLE_FLAME_FRAMES)]
    return _build_grid(CASTLE_FX_CELL, entries)


PROPS_ANIMATIONS = [
    {"name": f"{name}_walk",
     "frames": [f"enemy_{name}_{i}" for i in range(WALK_FRAMES)],
     "frame_ticks": 4}
    for name, _builder in SMALL_ENEMY_BUILDERS
] + [
    {"name": "pad_pulse", "frames": ["build_pad_0", "build_pad_1"],
     "frame_ticks": 15},
]

HUSK_ANIMATIONS = [
    {"name": f"{name}_walk",
     "frames": [f"enemy_{name}_{i}" for i in range(WALK_FRAMES)],
     "frame_ticks": 6}
    for name, _builder in HUSK_ENEMY_BUILDERS
]

TITAN_ANIMATIONS = [
    {"name": f"{name}_walk",
     "frames": [f"enemy_{name}_{i}" for i in range(WALK_FRAMES)],
     "frame_ticks": 8}
    for name, _builder in TITAN_ENEMY_BUILDERS
]

FX_ANIMATIONS = [
    {"name": "spark", "frames": [f"spark_{i}" for i in range(6)], "frame_ticks": 2},
]

CASTLE_FX_ANIMATIONS = [
    {"name": "castle_flag",
     "frames": [f"castle_flag_{i}" for i in range(CASTLE_FLAG_FRAMES)],
     "frame_ticks": 5},
    {"name": "castle_flame",
     "frames": [f"castle_flame_{i}" for i in range(CASTLE_FLAME_FRAMES)],
     "frame_ticks": 3},
]

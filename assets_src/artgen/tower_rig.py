"""Where the moving parts of every tower sit: the single source of the
numbers the art is drawn to and the game shoots from.

A shot leaves a tower from a point on its sprite - the ballista's bolt, the
catapult's bucket at the top of its swing, the spire's orb. The art builders
in artgen/tower_art.py place those parts with the functions here, and
artgen/generate_tower_rig.py writes the same points into
components/app_game/data/frostline_tower_rig.h for the model and the view,
so the two cannot drift apart.

Coordinates: a sprite is a 56 px cell whose foot row is FOOT_ROW, and the
tower's map position is column ORIGIN_X of it. A rig point is (dx, h): px
right of the foot and px above it, for a tower facing right. A mirrored
tower (the catapult) flips dx when it faces left.

Every kind has four looks, the view's variants: level 1, level 2, and the
two level 3 branches (option 0 and option 1 of the second upgrade).
"""
import math

CELL = 56
FOOT_ROW = CELL - 6
ORIGIN_X = CELL // 2
# The column most tower art is centred on.
CX = 27

VARIANTS = ((1, None), (2, None), (3, 0), (3, 1))

# The catapult's arm, in degrees from straight up, positive towards the
# target. Each pose is one baked frame; the view picks them by time.
SIEGE_POSES = (
    ("cocked", -105.0),   # pulled back and waiting; the load sits in the bucket
    ("swing", -40.0),     # the throw, smeared
    ("stop", 12.0),       # slammed against the stop beam: the shot leaves here
    ("bounce", -6.0),     # rebounding off the beam
    ("wind", -60.0),      # being winched back down
)
SIEGE_SHORT_ARM = 6.0
# The guard's turning head: eight directions, three states.
GUARD_DIRECTIONS = 8
GUARD_STATES = ("loaded", "loosed", "drawn")
GUARD_HEADS = ("crossbow", "ballista", "repeater")
GUARD_REACH = 8         # the bolt tip, ahead of the mount along the aim


def to_rig(x, y):
    """A sprite pixel as a rig point."""
    return (int(round(x - ORIGIN_X)), int(round(FOOT_ROW - y)))


# ----------------------------------------------------------------------
# Guard: a keep with a turning ballista on top
# ----------------------------------------------------------------------


def guard_body(level):
    """(radius, top) of the keep's wall."""
    return 12 + level, FOOT_ROW - (14 + level * 4)


def guard_mount(variant):
    """The sprite pixel the ballista turns on."""
    level, branch = VARIANTS[variant]
    _r, top = guard_body(level)
    if branch == 1:
        # The marksman's arbalest pokes out of the lookout's window.
        return CX, top - 3
    deck = top - (5 if level == 1 else 8)
    return CX, deck - 3


# ----------------------------------------------------------------------
# Siege: a catapult whose arm swings over its pivot
# ----------------------------------------------------------------------


def siege_pivot(level):
    return CX, 27 - level


def siege_arm_length(level):
    return 15.0 + level


def siege_arm_end(level, degrees, facing=1):
    """The bucket end of the arm for a pose, facing right (1) or left (-1)."""
    px, py = siege_pivot(level)
    a = math.radians(degrees)
    length = siege_arm_length(level)
    return px + facing * math.sin(a) * length, py - math.cos(a) * length


def siege_load(level, degrees, facing=1):
    """Where the shot sits in the bucket: on its open, upper side."""
    ex, ey = siege_arm_end(level, degrees, facing)
    a = math.radians(degrees)
    nx, ny = facing * math.cos(a), math.sin(a)
    if ny > 0.0:
        nx, ny = -nx, -ny
    return ex + nx * 2.5, ey + ny * 2.5


def siege_release(level, facing=1):
    """Where the shot leaves: just past the bucket as the arm hits the stop."""
    degrees = dict(SIEGE_POSES)["stop"]
    ex, ey = siege_arm_end(level, degrees, facing)
    a = math.radians(degrees)
    return ex + facing * math.sin(a) * 2.0, ey - math.cos(a) * 2.0


# ----------------------------------------------------------------------
# Frost: an ice spire
# ----------------------------------------------------------------------


def frost_height(variant):
    return (22, 28, 32, 32)[variant]


def frost_tip(variant):
    return CX - 1, FOOT_ROW - 10 - frost_height(variant)


# ----------------------------------------------------------------------
# Storm, rune and ember: fixed points on a symmetric tower
# ----------------------------------------------------------------------

STORM_ORB_ROW = FOOT_ROW - 34
RUNE_EYE_ROW = FOOT_ROW - 34
EMBER_BOWL_ROW = FOOT_ROW - 30


def storm_radius(variant):
    level, branch = VARIANTS[variant]
    return 4.5 + level * 0.5 + (1.0 if branch == 0 else 0.0)


def rune_top(variant):
    level, branch = VARIANTS[variant]
    return RUNE_EYE_ROW - (7 if branch == 1 else 4) - level


def ember_radius(level):
    return 11 + level


def ember_spares(level):
    """The fire pots stacked by a kiln of level 2 and up."""
    if level < 2:
        return ()
    r = ember_radius(level)
    return ((CX - r - 3, FOOT_ROW - 6), (CX + r + 3, FOOT_ROW - 5))


BARRACKS_DOOR = (CX - 1, FOOT_ROW - 4)   # bottom centre of the doorway
BARRACKS_DOOR_SIZE = (8, 10)


# ----------------------------------------------------------------------
# The rig table
# ----------------------------------------------------------------------


def _rig(kind, variant):
    """Every point of one look, as rig points; absent parts are (0, 0)."""
    level, branch = VARIANTS[variant]
    rig = {"muzzle": (0, 0), "load": (0, 0), "spare": ((0, 0), (0, 0)),
           "door": (0, 0), "orbit": (0, 0, 0, 0), "charge": 0}
    if kind == "guard":
        rig["muzzle"] = to_rig(*guard_mount(variant))
    elif kind == "siege":
        rig["muzzle"] = to_rig(*siege_release(level))
        rig["load"] = to_rig(*siege_load(level, dict(SIEGE_POSES)["cocked"]))
    elif kind == "frost":
        tx, ty = frost_tip(variant)
        if branch == 1:
            ty -= 2          # the lens over the tip
        rig["muzzle"] = to_rig(tx, ty + 3)
        rig["charge"] = 12
        if branch == 0:
            half = 6 + level
            rig["orbit"] = (6, half + 10, 4,
                            int(round(10 + frost_height(variant) * 0.55)))
    elif kind == "storm":
        rig["muzzle"] = to_rig(CX - 1, STORM_ORB_ROW)
        rig["charge"] = 34
        if branch == 0:
            rig["orbit"] = (5, int(round(storm_radius(variant) + 9)), 3,
                            FOOT_ROW - STORM_ORB_ROW - 3)
    elif kind == "rune":
        if branch == 0:
            rig["muzzle"] = to_rig(CX - 1, rune_top(variant) - 6)
        else:
            rig["muzzle"] = to_rig(CX - 1, RUNE_EYE_ROW)
        rig["charge"] = 10
        if branch == 1:
            rig["orbit"] = (4, 13, 4, FOOT_ROW - RUNE_EYE_ROW - 10)
    elif kind == "ember":
        rig["muzzle"] = to_rig(CX - 1, EMBER_BOWL_ROW - 3)
        spares = [to_rig(x, y) for x, y in ember_spares(level)]
        while len(spares) < 2:
            spares.append((0, 0))
        rig["spare"] = tuple(spares)
    elif kind == "barracks":
        rig["door"] = to_rig(*BARRACKS_DOOR)
    return rig


def tower_rigs(kind):
    """The four looks of a kind, in variant order."""
    return [_rig(kind, variant) for variant in range(len(VARIANTS))]


# Kinds that turn to face their target, and how: a head that points in
# GUARD_DIRECTIONS directions, or a whole sprite that faces left or right.
TURNS = {"guard": "head", "siege": "mirror"}
REACH = {"guard": GUARD_REACH}

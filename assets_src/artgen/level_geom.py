"""Level geometry for Frostline Hold.

Coordinates are screen pixels on the 480x480 panel. The HUD occupies the top
44px and the build bar the bottom 68px, so the visible battlefield is
y in [PLAY_TOP, PLAY_BOTTOM).

This module holds what every level shares - the play area, the gate, the
hero and enemy rosters, the wave timing - plus the geometry and waves of the
first level, Frostline Pass. The campaign itself lives in artgen/levels.py;
generate_level.py writes both into components/app_game/data/frostline_level.h.
"""

def fillet_path(points, radius=16, segments=7):
    """Round off every corner with a quadratic arc.

    The corners are what make a road look machine-cut; rounding them lets the
    drawn route and the route the enemies walk stay the same curve.
    """
    if len(points) < 3:
        return list(points)
    out = [points[0]]
    for index in range(1, len(points) - 1):
        p0, p1, p2 = points[index - 1], points[index], points[index + 1]
        d0 = ((p1[0] - p0[0]) ** 2 + (p1[1] - p0[1]) ** 2) ** 0.5
        d1 = ((p2[0] - p1[0]) ** 2 + (p2[1] - p1[1]) ** 2) ** 0.5
        cut = min(radius, d0 * 0.45, d1 * 0.45)
        if cut < 2.0:
            out.append(p1)
            continue
        ax = p1[0] + (p0[0] - p1[0]) / d0 * cut
        ay = p1[1] + (p0[1] - p1[1]) / d0 * cut
        bx = p1[0] + (p2[0] - p1[0]) / d1 * cut
        by = p1[1] + (p2[1] - p1[1]) / d1 * cut
        for step in range(segments + 1):
            t = step / float(segments)
            ox = (1 - t) ** 2
            oy = 2 * (1 - t) * t
            oz = t ** 2
            out.append((ox * ax + oy * p1[0] + oz * bx,
                        ox * ay + oy * p1[1] + oz * by))
    out.append(points[-1])
    return out


PLAY_TOP = 44
PLAY_BOTTOM = 412
VIEW_WIDTH = 480
VIEW_HEIGHT = 480

SPAWN = (14, 146)
GOAL = (450, 366)

# Lane A: the main road entering from the north-west gate and running south.
PATH_LANE_A = [
    (14, 146), (28, 150), (72, 150), (72, 214), (150, 214), (150, 268),
    (228, 268), (228, 320), (300, 320), (300, 366), (388, 366), GOAL,
]

# Lane B: a spur dropping from the eastern cliff that merges into lane A.
MERGE_POINT = (300, 320)
PATH_LANE_B = [
    (462, 150), (396, 150), (396, 250), (330, 250), (330, 300), MERGE_POINT,
]

# The drawn route is the rounded version of the design polyline; the model
# walks exactly this curve so the enemies never leave the painted road.
PATH_LANE_A_ROUTE = fillet_path(PATH_LANE_A)
PATH_LANE_B_ROUTE = fillet_path(PATH_LANE_B)

ALL_PATHS = (PATH_LANE_A_ROUTE, PATH_LANE_B_ROUTE)

# Build pads sit beside the roads and never on them.
BUILD_SPOTS = [
    (58, 108), (50, 192), (118, 178), (118, 250), (186, 186), (186, 244),
    (258, 236), (258, 300), (246, 346), (330, 210), (352, 282), (360, 330),
    (432, 244), (418, 122),
]

# Where a hero may stand. Both are hard against the road, so the traffic
# has to walk past the hero and fight it.
HERO_POSTS = [(56, 232), (212, 300)]

# The hero roster.
# (kind, name, hp, damage, range, cooldown, cost, skill damage, skill radius,
#  skill slow ticks, skill cooldown, respawn ticks)
HERO_WARDEN = 0

HERO_STATS = [
    (HERO_WARDEN, "warden", 220, 16, 95, 20, 150,
     45, 130, 90, 1200, 600),
]

# How close an enemy has to come before it stops and fights the hero.
HERO_BLOCK_RADIUS = 34

# Enemy roster.
# (kind, name, hp, armor, speed px/s, bounty, score, behaviour, size)
#
# The behaviour says what the enemy does beyond walking and hitting:
#   0 plain      - walks and hits the gate
#   1 healer     - mends the wounded ally nearest to it
#   2 banner     - hardens the allies around it
#   3 sap        - blows a hole in the gate instead of walking through it
#   4 titan      - the boss: too big to slow, too heavy to stun
#   5 flyer      - over soldiers and heroes, out of the siege's reach
#   6 golem      - its shell cracks at half health: no armour, faster
#   7 splitter   - bursts into three slimelets when it dies
#   8 burrower   - dives under the road now and then, unseen and faster
#   9 wyrm       - the mine's boss: dives, surfaces with skitters
#  10 king       - the fortress boss: freezes towers, wards itself twice
ENEMY_WOLF = 0
ENEMY_RAIDER = 1
ENEMY_SKITTER = 2
ENEMY_HUSK = 3
ENEMY_SHAMAN = 4
ENEMY_BANNERMAN = 5
ENEMY_SAP = 6
ENEMY_TITAN = 7
ENEMY_CROW = 8
ENEMY_GOLEM = 9
ENEMY_SLIME = 10
ENEMY_SLIMELET = 11
ENEMY_BURROWER = 12
ENEMY_WYRM = 13
ENEMY_KING = 14

ENEMY_BEHAVIOUR_PLAIN = 0
ENEMY_BEHAVIOUR_HEALER = 1
ENEMY_BEHAVIOUR_BANNER = 2
ENEMY_BEHAVIOUR_SAP = 3
ENEMY_BEHAVIOUR_TITAN = 4
ENEMY_BEHAVIOUR_FLYER = 5
ENEMY_BEHAVIOUR_GOLEM = 6
ENEMY_BEHAVIOUR_SPLITTER = 7
ENEMY_BEHAVIOUR_BURROWER = 8
ENEMY_BEHAVIOUR_WYRM = 9
ENEMY_BEHAVIOUR_KING = 10

ENEMY_STATS = [
    (ENEMY_WOLF, "wolf", 40, 0, 52, 5, 10, ENEMY_BEHAVIOUR_PLAIN, 32),
    (ENEMY_RAIDER, "raider", 70, 2, 34, 9, 25, ENEMY_BEHAVIOUR_PLAIN, 32),
    (ENEMY_SKITTER, "skitter", 26, 0, 76, 4, 12, ENEMY_BEHAVIOUR_PLAIN, 32),
    (ENEMY_HUSK, "husk", 190, 18, 20, 16, 45, ENEMY_BEHAVIOUR_PLAIN, 40),
    (ENEMY_SHAMAN, "shaman", 85, 0, 30, 14, 40, ENEMY_BEHAVIOUR_HEALER, 32),
    (ENEMY_BANNERMAN, "banner", 110, 6, 26, 15, 45, ENEMY_BEHAVIOUR_BANNER, 32),
    (ENEMY_SAP, "sap", 60, 0, 46, 12, 35, ENEMY_BEHAVIOUR_SAP, 32),
    (ENEMY_TITAN, "titan", 1200, 10, 16, 120, 400, ENEMY_BEHAVIOUR_TITAN, 64),
    (ENEMY_CROW, "crow", 32, 0, 62, 6, 15, ENEMY_BEHAVIOUR_FLYER, 32),
    (ENEMY_GOLEM, "golem", 330, 28, 18, 24, 70, ENEMY_BEHAVIOUR_GOLEM, 40),
    (ENEMY_SLIME, "slime", 150, 0, 28, 10, 40, ENEMY_BEHAVIOUR_SPLITTER, 40),
    (ENEMY_SLIMELET, "slimelet", 36, 0, 44, 2, 8, ENEMY_BEHAVIOUR_PLAIN, 32),
    (ENEMY_BURROWER, "burrower", 120, 4, 34, 14, 45, ENEMY_BEHAVIOUR_BURROWER,
     32),
    (ENEMY_WYRM, "wyrm", 2600, 8, 14, 200, 800, ENEMY_BEHAVIOUR_WYRM, 64),
    (ENEMY_KING, "king", 4200, 14, 12, 300, 1200, ENEMY_BEHAVIOUR_KING, 64),
]

# Points awarded for clearing a wave, on top of the points from the kills.
# Twice the gold reward keeps the score curve tied to the difficulty curve.
def wave_score_bonus(wave):
    return wave["reward"] * 2


# Wave table. Each wave is a list of batches; a batch spawns `count` enemies of
# one kind every `interval` ticks down one lane.
# (kind, count, interval_ticks, lane)
LANE_A = 0
LANE_B = 1
# Or'ed into the lane: the batch's enemy is a champion, the mini-boss of a
# chapter's opening level (FL_LANE_CHAMPION).
CHAMPION = 0x80

WAVES = [
    # 1-2: wolves only, a gentle teaching ramp.
    {"reward": 50, "hp_mult": 100, "armor_add": 0,
     "batches": [(ENEMY_WOLF, 4, 34, LANE_A)]},
    {"reward": 60, "hp_mult": 100, "armor_add": 0,
     "batches": [(ENEMY_WOLF, 9, 24, LANE_A)]},
    {"reward": 65, "hp_mult": 105, "armor_add": 0,
     "batches": [(ENEMY_WOLF, 8, 24, LANE_A), (ENEMY_RAIDER, 2, 30, LANE_A)]},
    # 4: the raiders arrive.
    {"reward": 70, "hp_mult": 110, "armor_add": 1,
     "batches": [(ENEMY_WOLF, 8, 22, LANE_A), (ENEMY_RAIDER, 4, 26, LANE_A)]},
    # 5: the skitters, fast and thin.
    {"reward": 75, "hp_mult": 115, "armor_add": 2,
     "batches": [(ENEMY_WOLF, 8, 20, LANE_A), (ENEMY_SKITTER, 5, 16, LANE_A)]},
    # 6: the second lane opens.
    {"reward": 80, "hp_mult": 125, "armor_add": 3,
     "batches": [(ENEMY_RAIDER, 6, 24, LANE_A), (ENEMY_WOLF, 8, 18, LANE_A),
                 (ENEMY_RAIDER, 2, 26, LANE_B)]},
    # 7: the husks walk in, slow and armoured.
    {"reward": 90, "hp_mult": 135, "armor_add": 4,
     "batches": [(ENEMY_WOLF, 10, 18, LANE_A), (ENEMY_HUSK, 2, 40, LANE_A),
                 (ENEMY_RAIDER, 4, 24, LANE_B)]},
    # 8: mixed pressure.
    {"reward": 95, "hp_mult": 150, "armor_add": 5,
     "batches": [(ENEMY_WOLF, 12, 16, LANE_A), (ENEMY_RAIDER, 6, 22, LANE_A),
                 (ENEMY_RAIDER, 4, 22, LANE_B)]},
    # 9: the shaman mends the wounded.
    {"reward": 105, "hp_mult": 280, "armor_add": 6,
     "batches": [(ENEMY_RAIDER, 8, 20, LANE_A), (ENEMY_WOLF, 12, 14, LANE_A),
                 (ENEMY_SHAMAN, 1, 60, LANE_A),
                 (ENEMY_RAIDER, 6, 20, LANE_B)]},
    # 10: skitters flood the lane.
    {"reward": 115, "hp_mult": 315, "armor_add": 7,
     "batches": [(ENEMY_SKITTER, 10, 12, LANE_A), (ENEMY_RAIDER, 8, 18, LANE_A),
                 (ENEMY_WOLF, 12, 14, LANE_A),
                 (ENEMY_RAIDER, 8, 18, LANE_B)]},
    # 11: the banner hardens the escort.
    {"reward": 125, "hp_mult": 350, "armor_add": 8,
     "batches": [(ENEMY_WOLF, 14, 12, LANE_A), (ENEMY_RAIDER, 8, 18, LANE_A),
                 (ENEMY_BANNERMAN, 1, 60, LANE_A),
                 (ENEMY_RAIDER, 8, 18, LANE_B)]},
    # 12: husks and skitters together.
    {"reward": 135, "hp_mult": 390, "armor_add": 9,
     "batches": [(ENEMY_HUSK, 3, 40, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_A),
                 (ENEMY_SKITTER, 10, 12, LANE_A),
                 (ENEMY_RAIDER, 10, 16, LANE_B)]},
    # 13: the saps, quick and volatile.
    {"reward": 145, "hp_mult": 435, "armor_add": 10,
     "batches": [(ENEMY_WOLF, 14, 12, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_A),
                 (ENEMY_SAP, 3, 30, LANE_A),
                 (ENEMY_RAIDER, 10, 16, LANE_B)]},
    # 14: the wall of raiders.
    {"reward": 165, "hp_mult": 495, "armor_add": 11,
     "batches": [(ENEMY_RAIDER, 12, 14, LANE_A), (ENEMY_WOLF, 16, 10, LANE_A),
                 (ENEMY_HUSK, 3, 36, LANE_A),
                 (ENEMY_RAIDER, 12, 14, LANE_B)]},
    # 15: the frost titan walks in behind the escort.
    {"reward": 240, "hp_mult": 560, "armor_add": 12,
     "batches": [(ENEMY_RAIDER, 12, 16, LANE_A), (ENEMY_WOLF, 16, 10, LANE_A),
                 (ENEMY_RAIDER, 12, 16, LANE_B), (ENEMY_WOLF, 16, 10, LANE_B)]},
    # 16: the titan itself, at the head of the last push.
    {"reward": 300, "hp_mult": 645, "armor_add": 14,
     "batches": [(ENEMY_BANNERMAN, 1, 60, LANE_A | CHAMPION),
                 (ENEMY_RAIDER, 10, 14, LANE_A),
                 (ENEMY_RAIDER, 10, 14, LANE_B)]},
]

for _wave in WAVES:
    _wave["score_bonus"] = wave_score_bonus(_wave)

BATCH_MAX = 4
WAVE_COUNT = len(WAVES)
FIRST_PREP_TICKS = 20 * 30
PREP_TICKS = 12 * 30

# Forest patches: conifers are scattered here to build up visual density.
# Each zone is (x0, y0, x1, y1) with x1 > x0 and y1 > y0.
FOREST_ZONES = [
    (6, 200, 132, 410),
    (140, 56, 270, 166),
    (238, 336, 438, 406),
    (356, 52, 474, 128),
]


def point_segment_distance(px, py, x0, y0, x1, y1):
    """Shortest distance from a point to a line segment."""
    dx, dy = x1 - x0, y1 - y0
    length_squared = dx * dx + dy * dy
    if length_squared == 0:
        return ((px - x0) ** 2 + (py - y0) ** 2) ** 0.5
    t = ((px - x0) * dx + (py - y0) * dy) / length_squared
    t = max(0.0, min(1.0, t))
    cx, cy = x0 + t * dx, y0 + t * dy
    return ((px - cx) ** 2 + (py - cy) ** 2) ** 0.5


def distance_to_paths(px, py):
    """Shortest distance from a point to any road segment."""
    best = float("inf")
    for lane in ALL_PATHS:
        for index in range(len(lane) - 1):
            x0, y0 = lane[index]
            x1, y1 = lane[index + 1]
            best = min(best, point_segment_distance(px, py, x0, y0, x1, y1))
    return best


def distance_to_spots(px, py):
    best = float("inf")
    for sx, sy in BUILD_SPOTS:
        best = min(best, ((px - sx) ** 2 + (py - sy) ** 2) ** 0.5)
    return best

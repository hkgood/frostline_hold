"""Level geometry for Frostline Hold.

Coordinates are screen pixels on the 480x480 panel. The HUD occupies the top
44px and the build bar the bottom 68px, so the visible battlefield is
y in [PLAY_TOP, PLAY_BOTTOM).

This module is the single source of truth for the art generator. The C model
mirrors these values in components/app_game/data/frostline_level.h, and
artgen/test_level_sync.py asserts the two never drift apart.
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

HERO_POSTS = [(96, 300), (376, 214)]

# Enemy roster. (kind, name, hp, armor, speed px/s, bounty, score)
# `bounty` is the gold paid on a kill, `score` is the points paid on a kill.
ENEMY_WOLF = 0
ENEMY_RAIDER = 1

ENEMY_STATS = [
    (ENEMY_WOLF, "wolf", 40, 0, 52, 3, 10),
    (ENEMY_RAIDER, "raider", 70, 2, 34, 5, 25),
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

WAVES = [
    # 1-3: wolves only, a gentle teaching ramp.
    {"reward": 20, "batches": [(ENEMY_WOLF, 4, 34, LANE_A)]},
    {"reward": 25, "batches": [(ENEMY_WOLF, 10, 24, LANE_A)]},
    {"reward": 30, "batches": [(ENEMY_WOLF, 8, 24, LANE_A),
                               (ENEMY_RAIDER, 3, 30, LANE_A)]},
    # 4-6: raiders and armoured raiders join.
    {"reward": 35, "batches": [(ENEMY_WOLF, 8, 20, LANE_A),
                               (ENEMY_RAIDER, 5, 26, LANE_A)]},
    {"reward": 40, "batches": [(ENEMY_RAIDER, 8, 24, LANE_A),
                               (ENEMY_WOLF, 8, 18, LANE_A)]},
    {"reward": 45, "batches": [(ENEMY_RAIDER, 6, 24, LANE_A),
                               (ENEMY_WOLF, 10, 16, LANE_A),
                               (ENEMY_RAIDER, 4, 24, LANE_B)]},
    # 7: both lanes are used for the first time.
    {"reward": 50, "batches": [(ENEMY_WOLF, 14, 14, LANE_A),
                               (ENEMY_RAIDER, 6, 24, LANE_A),
                               (ENEMY_RAIDER, 6, 24, LANE_B)]},
    # 8: the reinforced wave.
    {"reward": 70, "batches": [(ENEMY_WOLF, 12, 16, LANE_A),
                               (ENEMY_RAIDER, 8, 20, LANE_A),
                               (ENEMY_RAIDER, 6, 20, LANE_B)]},
    # 9-14: mixed pressure.
    {"reward": 55, "batches": [(ENEMY_WOLF, 10, 18, LANE_A),
                               (ENEMY_RAIDER, 8, 20, LANE_A),
                               (ENEMY_RAIDER, 6, 20, LANE_B)]},
    {"reward": 60, "batches": [(ENEMY_RAIDER, 10, 18, LANE_A),
                               (ENEMY_WOLF, 12, 14, LANE_A),
                               (ENEMY_RAIDER, 8, 18, LANE_B)]},
    {"reward": 65, "batches": [(ENEMY_WOLF, 16, 12, LANE_A),
                               (ENEMY_RAIDER, 10, 18, LANE_A),
                               (ENEMY_RAIDER, 8, 18, LANE_B)]},
    {"reward": 70, "batches": [(ENEMY_RAIDER, 12, 16, LANE_A),
                               (ENEMY_WOLF, 14, 12, LANE_A),
                               (ENEMY_RAIDER, 8, 16, LANE_B),
                               (ENEMY_WOLF, 8, 14, LANE_B)]},
    {"reward": 75, "batches": [(ENEMY_WOLF, 18, 10, LANE_A),
                               (ENEMY_RAIDER, 12, 14, LANE_A),
                               (ENEMY_RAIDER, 10, 14, LANE_B),
                               (ENEMY_RAIDER, 8, 14, LANE_B)]},
    {"reward": 80, "batches": [(ENEMY_RAIDER, 14, 14, LANE_A),
                               (ENEMY_WOLF, 18, 10, LANE_A),
                               (ENEMY_RAIDER, 10, 14, LANE_B),
                               (ENEMY_WOLF, 10, 12, LANE_B)]},
    # 15: the boss wave, flanked by raiders.
    {"reward": 150, "batches": [(ENEMY_RAIDER, 12, 16, LANE_A),
                                (ENEMY_RAIDER, 12, 16, LANE_B),
                                (ENEMY_WOLF, 16, 10, LANE_A),
                                (ENEMY_WOLF, 16, 10, LANE_B)]},
]

for _wave in WAVES:
    _wave["score_bonus"] = wave_score_bonus(_wave)

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

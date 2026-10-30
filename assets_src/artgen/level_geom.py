"""Level geometry for Frostline Hold.

Coordinates are screen pixels on the 480x480 panel. The HUD occupies the top
44px and the build bar the bottom 68px, so the visible battlefield is
y in [PLAY_TOP, PLAY_BOTTOM).

This module is the single source of truth for the art generator. The C model
mirrors these values in components/app_game/data/frostline_level.h, and
artgen/test_level_sync.py asserts the two never drift apart.
"""

PLAY_TOP = 44
PLAY_BOTTOM = 412
VIEW_WIDTH = 480
VIEW_HEIGHT = 480

SPAWN = (18, 96)
GOAL = (450, 366)

# Lane A: the main road entering from the north-west gate and running south.
PATH_LANE_A = [
    (18, 96), (18, 150), (72, 150), (72, 214), (150, 214), (150, 268),
    (228, 268), (228, 320), (300, 320), (300, 366), (388, 366), GOAL,
]

# Lane B: a spur dropping from the eastern cliff that merges into lane A.
MERGE_POINT = (300, 320)
PATH_LANE_B = [
    (462, 150), (396, 150), (396, 250), (330, 250), (330, 300), MERGE_POINT,
]

ALL_PATHS = (PATH_LANE_A, PATH_LANE_B)

# Build pads sit beside the roads and never on them.
BUILD_SPOTS = [
    (58, 108), (50, 192), (118, 178), (118, 250), (186, 186), (186, 244),
    (258, 236), (258, 300), (246, 346), (330, 210), (352, 282), (360, 330),
    (432, 244), (418, 122),
]

HERO_POSTS = [(96, 300), (376, 214)]

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

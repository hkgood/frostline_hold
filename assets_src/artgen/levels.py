"""The campaign: every level Frostline Hold ships, in play order.

Each level is a plain dict so a designer can read it top to bottom:

    key          short id; the terrain frame is "map_<key>"
    name         the printed name on the campaign map
    lane_a       design polyline of the main road (spawn first, gate last)
    lane_b       design polyline of the second road, ending on lane A;
                 an empty list means the level has a single road
    spots        build pads beside the roads
    posts        where a hero may stand, hard against a road
    waves        the wave table, same format as level_geom.WAVES
    start_gold   gold at the start of the run
    lives        lives at the start of the run
    towers       tower kinds the build ring offers on this level
    node         where the level sits on the campaign map
    art          how the terrain is drawn:
                   "engraving" - the AI plate already contains the road
                   "carve"     - the road is carved into an AI plate
    plate        the AI terrain plate under assets_src/ai/
    clear_behind optional, default True: also clear the ground behind a
                 tower's body; off for plates where that cuts the rock

All levels share the gate at level_geom.GOAL, so the castle plate stays one
piece of art. Every road reaches it along y == GOAL.y from the left.
"""
from artgen import level_geom as geom
from artgen.level_geom import (
    ENEMY_BANNERMAN, ENEMY_HUSK, ENEMY_RAIDER, ENEMY_SAP, ENEMY_SHAMAN,
    ENEMY_SKITTER, ENEMY_TITAN, ENEMY_WOLF, LANE_A, LANE_B, fillet_path,
    wave_score_bonus,
)
from artgen.tower_data import TOWER_ORDER

GOAL = geom.GOAL

# Where the eight stages of the journey sit on the campaign map
# (assets_src/ai/campaign_map_source.jpg). Each node sits on the landmark the
# plate draws for it. Levels take these in order; the plate already shows
# all eight, so adding a level never means redrawing it. The bottom of the
# screen stays free for the level card.
CAMPAIGN_NODES = [
    (96, 250),    # 1 snow forest
    (122, 318),   # 2 frozen lake
    (356, 318),   # 3 mountain switchbacks
    (408, 140),   # 4 mine
    (250, 206),   # 5 glacier
    (92, 150),    # 6 ruins
    (84, 86),     # 7 frozen falls
    (241, 46),    # 8 ice fortress
]
CAMPAIGN_PLATE = "campaign_map_source.jpg"

# Bit (1 << kind) per tower kind; kinds are 1-based like FL_TOWER_*.
TOWER_KIND = {name: index + 1 for index, name in enumerate(TOWER_ORDER)}

# The armoury grows by one tower per stage from the second on, and a level
# offers everything handed out so far. Each newcomer arrives one stage before
# the enemy it answers, so the player has a level to learn it.
STARTING_TOWERS = ("guard", "frost", "siege")
TOWERS_STAGE_2 = STARTING_TOWERS + ("storm",)
TOWERS_STAGE_3 = TOWERS_STAGE_2 + ("rune",)
TOWERS_STAGE_4 = TOWERS_STAGE_3 + ("barracks",)
TOWERS_STAGE_5 = TOWERS_STAGE_4 + ("ember",)


def _wave(reward, hp_mult, armor_add, *batches):
    wave = {"reward": reward, "hp_mult": hp_mult, "armor_add": armor_add,
            "batches": list(batches)}
    wave["score_bonus"] = wave_score_bonus(wave)
    return wave


# --- 1. Frostline Pass ---------------------------------------------------
# The level the game was built around: a forest road with a cliff spur.
FROSTLINE_PASS = {
    "key": "pass",
    "name": "FROSTLINE PASS",
    "lane_a": geom.PATH_LANE_A,
    "lane_b": geom.PATH_LANE_B,
    "spots": geom.BUILD_SPOTS,
    "posts": geom.HERO_POSTS,
    "waves": geom.WAVES,
    "start_gold": 190,
    "lives": 20,
    "towers": STARTING_TOWERS,
    "node": CAMPAIGN_NODES[0],
    "art": "engraving",
    "plate": "terrain_source.jpg",
}

# --- 2. Glassmere ----------------------------------------------------------
# A frozen lake. The north road loops round the far shore while the south
# road hugs the near one, so both lanes are open almost from the start and
# the pads on the ice cover either shore.
GLASSMERE = {
    "key": "mere",
    "name": "GLASSMERE",
    "lane_a": [
        (14, 100), (84, 100), (140, 78), (250, 74), (350, 92), (404, 140),
        (414, 212), (392, 258), (340, 292), (304, 326), (300, 366),
        (388, 366), GOAL,
    ],
    "lane_b": [
        (14, 318), (64, 318), (100, 360), (190, 378), (260, 372), (300, 366),
    ],
    "spots": [
        (60, 150), (120, 128), (196, 120), (282, 124), (350, 146),
        (360, 214), (318, 246), (256, 318), (190, 336), (120, 314),
        (60, 370), (246, 400), (452, 236), (450, 120), (60, 260),
    ],
    "posts": [(170, 104), (340, 330)],
    "waves": [
        _wave(55, 110, 0, (ENEMY_WOLF, 6, 30, LANE_A)),
        _wave(60, 110, 0, (ENEMY_WOLF, 6, 26, LANE_A),
              (ENEMY_WOLF, 4, 30, LANE_B)),
        _wave(70, 120, 1, (ENEMY_RAIDER, 4, 28, LANE_A),
              (ENEMY_WOLF, 8, 22, LANE_B)),
        _wave(75, 130, 2, (ENEMY_SKITTER, 6, 16, LANE_A),
              (ENEMY_RAIDER, 3, 28, LANE_B)),
        _wave(85, 145, 3, (ENEMY_WOLF, 10, 18, LANE_A),
              (ENEMY_HUSK, 1, 40, LANE_B), (ENEMY_RAIDER, 4, 24, LANE_B)),
        _wave(95, 170, 4, (ENEMY_RAIDER, 8, 20, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_WOLF, 8, 18, LANE_B)),
        _wave(105, 210, 5, (ENEMY_SKITTER, 10, 12, LANE_A),
              (ENEMY_RAIDER, 6, 20, LANE_B), (ENEMY_HUSK, 2, 40, LANE_B)),
        _wave(115, 260, 6, (ENEMY_WOLF, 14, 14, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_RAIDER, 8, 18, LANE_B)),
        _wave(125, 310, 7, (ENEMY_RAIDER, 10, 16, LANE_A),
              (ENEMY_SAP, 3, 30, LANE_B), (ENEMY_WOLF, 10, 14, LANE_B)),
        _wave(135, 360, 8, (ENEMY_HUSK, 3, 36, LANE_A),
              (ENEMY_SKITTER, 10, 12, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_B)),
        _wave(150, 420, 9, (ENEMY_RAIDER, 12, 14, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_WOLF, 14, 12, LANE_B),
              (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(165, 480, 10, (ENEMY_WOLF, 16, 10, LANE_A),
              (ENEMY_SAP, 4, 26, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B)),
        _wave(240, 560, 12, (ENEMY_RAIDER, 12, 14, LANE_A),
              (ENEMY_HUSK, 3, 36, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B),
              (ENEMY_SKITTER, 12, 10, LANE_B)),
        _wave(300, 640, 14, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_RAIDER, 10, 14, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B),
              (ENEMY_WOLF, 14, 10, LANE_B)),
    ],
    "start_gold": 220,
    "lives": 20,
    "towers": TOWERS_STAGE_2,
    "node": CAMPAIGN_NODES[1],
    "art": "carve",
    "plate": "terrain_mere_source.jpg",
}

# --- 3. Hollow Ridge ---------------------------------------------------------
# A switchback down a mountain face. The long main road doubles back on
# itself three times, so a well-placed tower covers two or three rows at once,
# while a goat track from the north-east joins halfway down.
HOLLOW_RIDGE = {
    "key": "ridge",
    "name": "HOLLOW RIDGE",
    "lane_a": [
        (236, 46), (236, 92), (96, 92), (64, 130), (96, 168), (330, 168),
        (380, 200), (364, 240), (150, 240), (116, 280), (150, 318),
        (300, 318), (330, 352), (388, 366), GOAL,
    ],
    "lane_b": [(466, 96), (430, 104), (420, 150), (380, 200)],
    "spots": [
        (150, 134), (300, 128), (390, 128), (52, 206), (200, 210),
        (290, 210), (430, 236), (60, 300), (200, 284), (290, 284),
        (200, 372), (290, 370), (40, 110),
    ],
    "posts": [(176, 186), (230, 340)],
    "waves": [
        _wave(60, 285, 1, (ENEMY_WOLF, 8, 24, LANE_A)),
        _wave(65, 310, 1, (ENEMY_RAIDER, 4, 28, LANE_A),
              (ENEMY_WOLF, 6, 22, LANE_A)),
        _wave(70, 330, 2, (ENEMY_SKITTER, 8, 14, LANE_A),
              (ENEMY_WOLF, 4, 24, LANE_B)),
        _wave(80, 365, 3, (ENEMY_RAIDER, 6, 22, LANE_A),
              (ENEMY_HUSK, 1, 40, LANE_A), (ENEMY_RAIDER, 3, 26, LANE_B)),
        _wave(90, 405, 4, (ENEMY_WOLF, 12, 16, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 6, 14, LANE_B)),
        _wave(100, 475, 5, (ENEMY_RAIDER, 8, 18, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_RAIDER, 6, 20, LANE_B)),
        _wave(110, 560, 6, (ENEMY_HUSK, 3, 36, LANE_A),
              (ENEMY_WOLF, 12, 14, LANE_A), (ENEMY_SAP, 2, 30, LANE_B)),
        _wave(120, 660, 7, (ENEMY_SKITTER, 14, 10, LANE_A),
              (ENEMY_RAIDER, 8, 18, LANE_B)),
        _wave(130, 760, 8, (ENEMY_RAIDER, 10, 16, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_HUSK, 2, 36, LANE_B),
              (ENEMY_WOLF, 10, 14, LANE_B)),
        _wave(140, 870, 9, (ENEMY_WOLF, 16, 12, LANE_A),
              (ENEMY_SAP, 4, 26, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_B)),
        _wave(150, 980, 10, (ENEMY_RAIDER, 12, 14, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 12, 10, LANE_B)),
        _wave(165, 1100, 11, (ENEMY_HUSK, 4, 32, LANE_A),
              (ENEMY_RAIDER, 10, 14, LANE_A), (ENEMY_RAIDER, 10, 14, LANE_B),
              (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(180, 1230, 12, (ENEMY_WOLF, 18, 10, LANE_A),
              (ENEMY_SAP, 4, 24, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B)),
        _wave(200, 1365, 13, (ENEMY_RAIDER, 14, 12, LANE_A),
              (ENEMY_HUSK, 3, 32, LANE_A), (ENEMY_SKITTER, 14, 10, LANE_B),
              (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(250, 1495, 14, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_RAIDER, 12, 14, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B)),
        _wave(320, 1630, 16, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_WOLF, 16, 10, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_RAIDER, 12, 14, LANE_B)),
    ],
    "start_gold": 240,
    "lives": 20,
    "towers": TOWERS_STAGE_3,
    "node": CAMPAIGN_NODES[2],
    "art": "engraving",
    "plate": "terrain_ridge_source.jpg",
    "clear_behind": False,
}

LEVELS = [FROSTLINE_PASS, GLASSMERE, HOLLOW_RIDGE]


def routes(level):
    """The rounded roads the enemies walk, lane A first."""
    lane_a = fillet_path(level["lane_a"])
    lane_b = fillet_path(level["lane_b"]) if level["lane_b"] else []
    return lane_a, lane_b


def tower_mask(level):
    mask = 0
    for name in level["towers"]:
        mask |= 1 << TOWER_KIND[name]
    return mask


def distance_to_roads(level, px, py):
    """Shortest distance from a point to any road of the level."""
    best = float("inf")
    for lane in routes(level):
        for index in range(len(lane) - 1):
            x0, y0 = lane[index]
            x1, y1 = lane[index + 1]
            best = min(best, geom.point_segment_distance(px, py, x0, y0, x1, y1))
    return best

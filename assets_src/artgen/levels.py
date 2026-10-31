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
    art          how the terrain is drawn:
                   "engraving" - the AI plate already contains the road
                   "carve"     - the road is carved into an AI plate
    plate        the AI terrain plate under assets_src/ai/
    clear_behind optional, default True: also clear the ground behind a
                 tower's body; off for plates where that cuts the rock
    carve_from   optional, on an "engraving" plate: the design point of
                 lane A from which the road is carved after all, where the
                 drawn road stops short of the gate

All levels share the gate at level_geom.GOAL, so the castle plate stays one
piece of art. Every road reaches it along y == GOAL.y from the left.
"""
from artgen import level_geom as geom
from artgen.level_geom import (
    ENEMY_BANNERMAN, ENEMY_BURROWER, ENEMY_CROW, ENEMY_GOLEM, ENEMY_HUSK,
    ENEMY_KING, ENEMY_RAIDER, ENEMY_SAP, ENEMY_SHAMAN, ENEMY_SKITTER,
    ENEMY_SLIME, ENEMY_TITAN, ENEMY_WOLF, ENEMY_WYRM, LANE_A, LANE_B,
    fillet_path,
)
from artgen.interludes import INTERLUDES
from artgen.level_kit import (  # noqa: F401  (re-exported for the tools)
    CHAMPION, GOAL, STARTING_TOWERS, TOWER_KIND, TOWERS_STAGE_2,
    TOWERS_STAGE_3, TOWERS_STAGE_4, TOWERS_STAGE_5, wave as _wave,
)


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
        _wave(300, 640, 14, (ENEMY_SHAMAN, 1, 60, LANE_A | CHAMPION),
              (ENEMY_RAIDER, 10, 14, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B),
              (ENEMY_WOLF, 14, 10, LANE_B)),
    ],
    "start_gold": 220,
    "lives": 20,
    "towers": TOWERS_STAGE_2,
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
        _wave(320, 1500, 16, (ENEMY_HUSK, 1, 60, LANE_A | CHAMPION),
              (ENEMY_WOLF, 16, 10, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_RAIDER, 12, 14, LANE_B)),
    ],
    "start_gold": 240,
    "lives": 20,
    "towers": TOWERS_STAGE_3,
    "art": "engraving",
    "plate": "terrain_ridge_source.jpg",
    "clear_behind": False,
}

# --- 4. Ironvein Mine --------------------------------------------------------
# A pit mine. The road leaves the tunnel mouth and doubles back three times
# across the pit floor while a side shaft feeds the second bend. Burrowers
# dive under the spoil here, golems march out of the deep, and the last wave
# wakes the wyrm.
IRONVEIN_MINE = {
    "key": "mine",
    "name": "IRONVEIN MINE",
    "lane_a": [
        (244, 46), (244, 108), (222, 136), (130, 137), (114, 170), (130, 206),
        (258, 207), (282, 240), (258, 286), (130, 288), (114, 330), (130, 383),
        (318, 383), (352, 372), (388, 366), GOAL,
    ],
    "lane_b": [(466, 186), (378, 188), (344, 214), (318, 250), (282, 255)],
    "spots": [
        (200, 176), (296, 208), (336, 296), (184, 240), (284, 312), (256, 176),
        (360, 248), (192, 328), (132, 236), (88, 152), (92, 204), (88, 316),
        (276, 116), (96, 380), (388, 152),
    ],
    "posts": [(246, 227), (126, 398)],
    "waves": [
        _wave(60, 145, 1, (ENEMY_WOLF, 8, 22, LANE_A)),
        _wave(65, 155, 1, (ENEMY_RAIDER, 5, 26, LANE_A),
              (ENEMY_WOLF, 6, 20, LANE_B)),
        _wave(70, 160, 2, (ENEMY_BURROWER, 4, 30, LANE_A),
              (ENEMY_WOLF, 6, 20, LANE_A)),
        _wave(80, 180, 3, (ENEMY_SKITTER, 10, 12, LANE_A),
              (ENEMY_BURROWER, 3, 30, LANE_B)),
        _wave(90, 200, 4, (ENEMY_RAIDER, 8, 18, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_BURROWER, 4, 26, LANE_B)),
        _wave(100, 235, 5, (ENEMY_GOLEM, 2, 50, LANE_A),
              (ENEMY_WOLF, 10, 14, LANE_B)),
        _wave(110, 270, 6, (ENEMY_HUSK, 2, 40, LANE_A),
              (ENEMY_BURROWER, 6, 22, LANE_A), (ENEMY_RAIDER, 6, 18, LANE_B)),
        _wave(120, 315, 7, (ENEMY_GOLEM, 3, 44, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 12, 10, LANE_B)),
        _wave(130, 360, 8, (ENEMY_RAIDER, 12, 14, LANE_A),
              (ENEMY_SAP, 3, 28, LANE_B), (ENEMY_BURROWER, 6, 20, LANE_B)),
        _wave(140, 415, 9, (ENEMY_GOLEM, 3, 40, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_WOLF, 14, 10, LANE_B),
              (ENEMY_HUSK, 2, 36, LANE_B)),
        _wave(150, 470, 10, (ENEMY_BURROWER, 10, 16, LANE_A),
              (ENEMY_RAIDER, 10, 14, LANE_B), (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(165, 525, 11, (ENEMY_GOLEM, 4, 36, LANE_A),
              (ENEMY_SKITTER, 14, 9, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B)),
        _wave(180, 590, 12, (ENEMY_HUSK, 4, 30, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_BURROWER, 8, 16, LANE_B),
              (ENEMY_SAP, 4, 24, LANE_B)),
        _wave(220, 650, 13, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_GOLEM, 3, 36, LANE_A), (ENEMY_RAIDER, 12, 12, LANE_B)),
        _wave(320, 505, 14, (ENEMY_WYRM, 1, 60, LANE_A),
              (ENEMY_BURROWER, 8, 18, LANE_A), (ENEMY_GOLEM, 3, 40, LANE_B),
              (ENEMY_RAIDER, 10, 14, LANE_B)),
    ],
    "start_gold": 260,
    "lives": 20,
    "towers": TOWERS_STAGE_4,
    "art": "engraving",
    "plate": "terrain_mine_source.jpg",
    "carve_from": (318, 383),
}

# --- 5. Whitethroat Glacier --------------------------------------------------
# Two stepped roads cross a glacier past the blue-ice tarns and meet at the
# gate. Crows cut across the ice and slimes split on the narrow steps.
WHITETHROAT_GLACIER = {
    "key": "glacier",
    "name": "WHITETHROAT GLACIER",
    "lane_a": [
        (14, 151), (71, 151), (71, 212), (146, 212), (146, 268), (226, 268),
        (226, 335), (300, 335), (300, 366), (388, 366), GOAL,
    ],
    "lane_b": [
        (466, 149), (398, 149), (398, 246), (332, 246), (332, 326), (368, 344),
        (404, 366),
    ],
    "spots": [
        (296, 284), (352, 400), (404, 272), (272, 368), (152, 184), (88, 244),
        (172, 300), (100, 156), (220, 364), (368, 216), (256, 252), (324, 192),
        (428, 228), (124, 284), (368, 164),
    ],
    "posts": [(160, 247), (319, 343)],
    "waves": [
        _wave(60, 70, 1, (ENEMY_WOLF, 8, 22, LANE_A)),
        _wave(65, 80, 1, (ENEMY_CROW, 6, 24, LANE_A),
              (ENEMY_WOLF, 6, 20, LANE_B)),
        _wave(70, 90, 2, (ENEMY_RAIDER, 6, 22, LANE_A),
              (ENEMY_CROW, 6, 20, LANE_B)),
        _wave(80, 105, 3, (ENEMY_SLIME, 3, 40, LANE_A),
              (ENEMY_SKITTER, 8, 12, LANE_B)),
        _wave(90, 125, 4, (ENEMY_CROW, 10, 14, LANE_A),
              (ENEMY_RAIDER, 6, 20, LANE_B), (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(100, 145, 5, (ENEMY_SLIME, 4, 34, LANE_A),
              (ENEMY_WOLF, 12, 12, LANE_B), (ENEMY_BURROWER, 3, 30, LANE_A)),
        _wave(110, 175, 6, (ENEMY_GOLEM, 2, 44, LANE_A),
              (ENEMY_CROW, 10, 12, LANE_B), (ENEMY_RAIDER, 6, 18, LANE_B)),
        _wave(120, 210, 7, (ENEMY_SLIME, 5, 30, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 12, 10, LANE_B)),
        _wave(130, 250, 8, (ENEMY_CROW, 14, 10, LANE_A),
              (ENEMY_HUSK, 3, 36, LANE_B), (ENEMY_SAP, 3, 28, LANE_B)),
        _wave(140, 295, 9, (ENEMY_GOLEM, 3, 40, LANE_A),
              (ENEMY_CROW, 8, 12, LANE_A), (ENEMY_SLIME, 4, 30, LANE_B),
              (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(150, 345, 10, (ENEMY_RAIDER, 12, 12, LANE_A),
              (ENEMY_CROW, 12, 10, LANE_B), (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(165, 405, 11, (ENEMY_SLIME, 6, 26, LANE_A),
              (ENEMY_HUSK, 3, 34, LANE_A), (ENEMY_CROW, 14, 9, LANE_B)),
        _wave(180, 465, 12, (ENEMY_GOLEM, 4, 34, LANE_A),
              (ENEMY_BURROWER, 6, 18, LANE_B), (ENEMY_CROW, 12, 10, LANE_B)),
        _wave(220, 535, 13, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_SLIME, 5, 26, LANE_A), (ENEMY_CROW, 14, 9, LANE_B)),
        _wave(300, 610, 15, (ENEMY_GOLEM, 1, 60, LANE_A | CHAMPION),
              (ENEMY_CROW, 16, 8, LANE_A), (ENEMY_GOLEM, 3, 36, LANE_B),
              (ENEMY_SLIME, 5, 24, LANE_B)),
    ],
    "start_gold": 280,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "engraving",
    "plate": "terrain_glacier_source.jpg",
}

# --- 6. Sleeping Ruins -------------------------------------------------------
# A forgotten temple. The main road runs between the colonnade and the
# fallen hall, then straight through the hall's rune circle; a second road
# skirts the ruins to the west. Healers and banners walk with every column.
SLEEPING_RUINS = {
    "key": "ruins",
    "name": "SLEEPING RUINS",
    "lane_a": [
        (466, 140), (352, 140), (342, 212), (215, 212), (215, 250), (215, 400),
        (250, 428), (330, 428), (388, 366), GOAL,
    ],
    "lane_b": [(14, 150), (46, 180), (46, 396), (84, 428), (250, 428)],
    "spots": [
        (300, 244), (248, 244), (416, 196), (188, 208), (272, 344), (76, 348),
        (360, 352), (332, 124), (372, 228), (404, 404), (64, 120), (76, 212),
        (172, 300), (168, 352),
    ],
    "posts": [(195, 238), (312, 408)],
    "waves": [
        _wave(60, 60, 2, (ENEMY_RAIDER, 6, 22, LANE_A)),
        _wave(65, 60, 2, (ENEMY_WOLF, 8, 18, LANE_A),
              (ENEMY_CROW, 6, 20, LANE_B)),
        _wave(75, 60, 3, (ENEMY_RAIDER, 6, 20, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 8, 12, LANE_B)),
        _wave(85, 70, 4, (ENEMY_GOLEM, 2, 48, LANE_A),
              (ENEMY_SLIME, 3, 34, LANE_B)),
        _wave(95, 85, 5, (ENEMY_BURROWER, 6, 22, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_RAIDER, 8, 18, LANE_B)),
        _wave(105, 100, 6, (ENEMY_HUSK, 3, 36, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_CROW, 10, 12, LANE_B)),
        _wave(115, 125, 7, (ENEMY_SLIME, 5, 28, LANE_A),
              (ENEMY_GOLEM, 2, 44, LANE_B), (ENEMY_WOLF, 10, 12, LANE_B)),
        _wave(125, 145, 8, (ENEMY_RAIDER, 12, 14, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_SAP, 4, 26, LANE_B),
              (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(135, 175, 9, (ENEMY_GOLEM, 4, 36, LANE_A),
              (ENEMY_BURROWER, 6, 18, LANE_B)),
        _wave(145, 205, 10, (ENEMY_CROW, 14, 9, LANE_A),
              (ENEMY_HUSK, 3, 34, LANE_A), (ENEMY_SLIME, 5, 26, LANE_B)),
        _wave(160, 240, 11, (ENEMY_RAIDER, 14, 12, LANE_A),
              (ENEMY_SHAMAN, 2, 50, LANE_A), (ENEMY_BANNERMAN, 1, 60, LANE_B),
              (ENEMY_GOLEM, 3, 36, LANE_B)),
        _wave(175, 275, 12, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_BURROWER, 8, 16, LANE_B), (ENEMY_SKITTER, 14, 9, LANE_B)),
        _wave(190, 320, 13, (ENEMY_SLIME, 6, 24, LANE_A),
              (ENEMY_GOLEM, 4, 32, LANE_A), (ENEMY_CROW, 14, 9, LANE_B)),
        _wave(210, 365, 14, (ENEMY_HUSK, 5, 28, LANE_A),
              (ENEMY_SHAMAN, 2, 50, LANE_A), (ENEMY_RAIDER, 14, 12, LANE_B),
              (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(240, 420, 15, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_GOLEM, 4, 32, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_SLIME, 5, 24, LANE_B)),
        _wave(330, 280, 16, (ENEMY_WYRM, 1, 60, LANE_A),
              (ENEMY_BURROWER, 8, 16, LANE_A), (ENEMY_GOLEM, 4, 32, LANE_B),
              (ENEMY_CROW, 14, 9, LANE_B)),
    ],
    "start_gold": 300,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "carve",
    "plate": "terrain_ruins_source.jpg",
}

# --- 7. Frozen Falls ---------------------------------------------------------
# A gorge under a frozen waterfall. Two stepped roads come down from either
# rim, close round the plunge pool and meet before the gate; the falls
# themselves fill the north half and the fight is fought in their spray.
FROZEN_FALLS = {
    "key": "falls",
    "name": "FROZEN FALLS",
    "lane_a": [
        (14, 298), (88, 298), (88, 355), (140, 355), (140, 410), (210, 410),
        (232, 422), (300, 422), (330, 422), (388, 366), GOAL,
    ],
    "lane_b": [
        (466, 298), (392, 298), (392, 355), (336, 355), (330, 404), (300, 422),
    ],
    "spots": [
        (292, 388), (392, 404), (352, 324), (172, 364), (224, 384), (128, 324),
        (108, 388), (56, 328), (288, 336), (100, 272), (392, 268), (48, 268),
        (440, 252),
    ],
    "posts": [(159, 392), (250, 440)],
    "waves": [
        _wave(65, 90, 2, (ENEMY_WOLF, 10, 18, LANE_A)),
        _wave(70, 95, 2, (ENEMY_CROW, 8, 18, LANE_A),
              (ENEMY_RAIDER, 5, 22, LANE_B)),
        _wave(80, 110, 3, (ENEMY_SLIME, 4, 32, LANE_A),
              (ENEMY_SKITTER, 10, 10, LANE_B)),
        _wave(90, 130, 4, (ENEMY_GOLEM, 2, 44, LANE_A),
              (ENEMY_CROW, 8, 14, LANE_B), (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(100, 150, 5, (ENEMY_BURROWER, 6, 20, LANE_A),
              (ENEMY_SLIME, 4, 30, LANE_B)),
        _wave(110, 180, 6, (ENEMY_RAIDER, 10, 14, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_CROW, 12, 10, LANE_B)),
        _wave(120, 210, 7, (ENEMY_HUSK, 3, 34, LANE_A),
              (ENEMY_SLIME, 5, 26, LANE_B), (ENEMY_SAP, 3, 26, LANE_B)),
        _wave(130, 245, 8, (ENEMY_GOLEM, 3, 38, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_CROW, 14, 9, LANE_B)),
        _wave(140, 285, 9, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_WOLF, 14, 10, LANE_B), (ENEMY_BURROWER, 6, 18, LANE_B)),
        _wave(150, 330, 10, (ENEMY_SLIME, 6, 24, LANE_A),
              (ENEMY_CROW, 12, 9, LANE_A), (ENEMY_GOLEM, 3, 36, LANE_B)),
        _wave(165, 380, 11, (ENEMY_RAIDER, 14, 12, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_HUSK, 3, 32, LANE_B),
              (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(180, 435, 12, (ENEMY_CROW, 18, 8, LANE_A),
              (ENEMY_SLIME, 6, 22, LANE_B)),
        _wave(195, 495, 13, (ENEMY_GOLEM, 5, 30, LANE_A),
              (ENEMY_BURROWER, 8, 16, LANE_B), (ENEMY_SAP, 4, 22, LANE_B)),
        _wave(215, 565, 14, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_CROW, 14, 9, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_RAIDER, 12, 12, LANE_B)),
        _wave(240, 640, 15, (ENEMY_SLIME, 8, 20, LANE_A),
              (ENEMY_HUSK, 4, 28, LANE_A), (ENEMY_GOLEM, 4, 30, LANE_B),
              (ENEMY_CROW, 14, 9, LANE_B)),
        _wave(340, 435, 16, (ENEMY_WYRM, 1, 60, LANE_A),
              (ENEMY_TITAN, 1, 60, LANE_B), (ENEMY_CROW, 16, 8, LANE_B),
              (ENEMY_SLIME, 6, 22, LANE_A)),
    ],
    "start_gold": 320,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "engraving",
    "plate": "terrain_falls_source.jpg",
    "carve_from": (210, 410),
}

# --- 8. Rime Citadel ---------------------------------------------------------
# The Frost King's own gate. Both hosts march out of the citadel, part round
# the frozen throne and close on the road home; the king himself leads the
# last wave.
RIME_CITADEL = {
    "key": "citadel",
    "name": "RIME CITADEL",
    "lane_a": [
        (202, 96), (202, 150), (184, 172), (132, 172), (112, 196), (112, 338),
        (138, 366), (320, 366), (388, 366), GOAL,
    ],
    "lane_b": [
        (275, 96), (275, 150), (293, 172), (346, 172), (366, 196), (366, 330),
        (346, 356), (320, 366),
    ],
    "spots": [
        (252, 168), (356, 128), (152, 140), (196, 224), (332, 324), (372, 400),
        (144, 208), (300, 228), (336, 268), (300, 396), (84, 192), (396, 236),
        (176, 336), (80, 388), (80, 268),
    ],
    "posts": [(111, 166), (123, 377)],
    "waves": [
        _wave(70, 60, 2, (ENEMY_RAIDER, 8, 20, LANE_A),
              (ENEMY_WOLF, 8, 18, LANE_B)),
        _wave(75, 65, 3, (ENEMY_CROW, 8, 16, LANE_A),
              (ENEMY_SKITTER, 10, 10, LANE_B)),
        _wave(85, 70, 3, (ENEMY_GOLEM, 2, 44, LANE_A),
              (ENEMY_SLIME, 3, 30, LANE_B)),
        _wave(95, 85, 4, (ENEMY_RAIDER, 10, 16, LANE_A),
              (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_BURROWER, 5, 22, LANE_B)),
        _wave(105, 95, 5, (ENEMY_HUSK, 3, 34, LANE_A),
              (ENEMY_CROW, 10, 12, LANE_B), (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(115, 110, 6, (ENEMY_SLIME, 5, 26, LANE_A),
              (ENEMY_GOLEM, 2, 40, LANE_B), (ENEMY_WOLF, 12, 10, LANE_B)),
        _wave(125, 130, 7, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_RAIDER, 10, 14, LANE_B), (ENEMY_SAP, 3, 26, LANE_B)),
        _wave(135, 150, 8, (ENEMY_CROW, 14, 9, LANE_A),
              (ENEMY_GOLEM, 3, 36, LANE_B), (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(145, 175, 9, (ENEMY_BURROWER, 8, 16, LANE_A),
              (ENEMY_HUSK, 3, 32, LANE_A), (ENEMY_SLIME, 5, 24, LANE_B),
              (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(155, 200, 10, (ENEMY_RAIDER, 14, 12, LANE_A),
              (ENEMY_SHAMAN, 2, 50, LANE_A), (ENEMY_CROW, 14, 9, LANE_B)),
        _wave(170, 235, 11, (ENEMY_GOLEM, 4, 32, LANE_A),
              (ENEMY_TITAN, 1, 60, LANE_B), (ENEMY_SKITTER, 14, 9, LANE_B)),
        _wave(260, 160, 12, (ENEMY_WYRM, 1, 60, LANE_A),
              (ENEMY_BURROWER, 8, 16, LANE_A), (ENEMY_RAIDER, 12, 12, LANE_B)),
        _wave(190, 305, 13, (ENEMY_SLIME, 7, 22, LANE_A),
              (ENEMY_CROW, 14, 9, LANE_A), (ENEMY_HUSK, 4, 28, LANE_B),
              (ENEMY_SHAMAN, 1, 60, LANE_B)),
        _wave(205, 345, 14, (ENEMY_GOLEM, 5, 28, LANE_A),
              (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_SAP, 4, 22, LANE_B)),
        _wave(220, 395, 15, (ENEMY_TITAN, 1, 60, LANE_A),
              (ENEMY_CROW, 16, 8, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_SLIME, 6, 22, LANE_B)),
        _wave(240, 445, 16, (ENEMY_RAIDER, 16, 10, LANE_A),
              (ENEMY_SHAMAN, 2, 50, LANE_A), (ENEMY_GOLEM, 5, 28, LANE_B),
              (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        _wave(260, 300, 17, (ENEMY_WYRM, 1, 60, LANE_A),
              (ENEMY_HUSK, 5, 26, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_CROW, 16, 8, LANE_B)),
        _wave(400, 335, 18, (ENEMY_KING, 1, 60, LANE_A),
              (ENEMY_GOLEM, 5, 28, LANE_A), (ENEMY_TITAN, 1, 60, LANE_B),
              (ENEMY_SLIME, 6, 22, LANE_B)),
    ],
    "start_gold": 340,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "engraving",
    "plate": "terrain_citadel_source.jpg",
    "carve_from": (200, 366),
}

# The eight chapters of the journey. Each opens with its own level and,
# except for the last, is followed by two interludes (interludes.py).
CHAPTERS = [FROSTLINE_PASS, GLASSMERE, HOLLOW_RIDGE, IRONVEIN_MINE,
            WHITETHROAT_GLACIER, SLEEPING_RUINS, FROZEN_FALLS, RIME_CITADEL]

# The campaign map is a scroll of square AI plates laid side by side; the
# view drags it sideways. Nodes are in scroll coordinates, one per level in
# play order, and keep clear of the level card at the bottom of the screen.
CAMPAIGN_PLATES = (
    "campaign_map_1_source.jpg",
    "campaign_map_2_source.jpg",
    "campaign_map_3_source.jpg",
)
CAMPAIGN_WIDTH = 480 * len(CAMPAIGN_PLATES)
CAMPAIGN_NODES = [
    # The snow forest, the frozen lake and the foot of the mountains.
    (70, 250), (130, 180), (196, 236), (262, 306), (330, 250), (384, 186),
    (448, 252),
    # The ridge, the mine and the glacier.
    (514, 176), (574, 236), (636, 300), (704, 250), (760, 184), (826, 240),
    (892, 300), (940, 226),
    # The ruins, the falls and the citadel.
    (1012, 170), (1076, 226), (1136, 290), (1204, 236), (1266, 300),
    (1326, 236), (1392, 160),
]


def _play_order():
    order = []
    for stage, chapter in enumerate(CHAPTERS):
        order.append(dict(chapter, stage=stage, interlude=False))
        for interlude in INTERLUDES.get(chapter["key"], ()):
            order.append(dict(interlude, stage=stage, interlude=True))
    assert len(order) <= len(CAMPAIGN_NODES), "every level needs a map node"
    for index, level in enumerate(order):
        level["node"] = CAMPAIGN_NODES[index]
    return order


# Every level in play order, each tagged with its chapter ("stage"), whether
# it is an interlude, and its node on the campaign map.
LEVELS = _play_order()


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

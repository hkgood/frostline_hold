"""The interludes: two shorter levels on the road after each chapter.

An interlude has no boss and brings no new enemy or tower; it plays what
its chapter just taught on fresh ground, so the next chapter's newcomer
arrives after practice rather than straight after the last one. Every one
is a "carve" level: its AI plate was drawn without a road from a layout
guide, and the road is cut into it along the lanes.

Same dict format as levels.py; INTERLUDES maps the key of the chapter an
interlude follows to its pair, in play order.
"""
from artgen.level_geom import (
    ENEMY_BANNERMAN, ENEMY_BURROWER, ENEMY_CROW, ENEMY_GOLEM, ENEMY_HUSK,
    ENEMY_RAIDER, ENEMY_SAP, ENEMY_SHAMAN, ENEMY_SKITTER, ENEMY_SLIME,
    ENEMY_WOLF, LANE_A, LANE_B,
)
from artgen.level_kit import (
    STARTING_TOWERS, TOWERS_STAGE_2, TOWERS_STAGE_3, TOWERS_STAGE_4,
    TOWERS_STAGE_5, wave,
)


# --- Pinewood Trail --------------------------------------------------------
# A forest trail: one road loops through the pines and doubles back
# on itself above the gate.
PINES = {
    "key": "pines",
    "name": "PINEWOOD TRAIL",
    "lane_a": [
        (14, 96), (300, 96), (400, 120), (420, 180), (380, 220),
        (200, 220), (150, 250), (150, 300), (200, 330), (300, 330),
        (330, 366), (388, 366), (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (234, 266), (330, 164), (234, 152), (324, 290), (186, 284),
        (384, 170), (288, 254), (270, 302), (174, 152), (282, 134),
        (300, 374), (90, 128), (360, 338),
    ],
    "posts": [(411, 152), (150, 265)],
    "waves": [
        wave(50, 175, 0, (ENEMY_WOLF, 4, 34, LANE_A)),
        wave(65, 185, 0, (ENEMY_WOLF, 8, 24, LANE_A),
             (ENEMY_RAIDER, 2, 30, LANE_A)),
        wave(75, 200, 2, (ENEMY_WOLF, 8, 20, LANE_A),
             (ENEMY_SKITTER, 5, 16, LANE_A)),
        wave(80, 220, 3, (ENEMY_RAIDER, 6, 24, LANE_A),
             (ENEMY_WOLF, 8, 18, LANE_A), (ENEMY_RAIDER, 2, 26, LANE_A)),
        wave(95, 260, 5, (ENEMY_WOLF, 12, 16, LANE_A),
             (ENEMY_RAIDER, 6, 22, LANE_A), (ENEMY_RAIDER, 4, 22, LANE_A)),
        wave(115, 550, 7, (ENEMY_SKITTER, 10, 12, LANE_A),
             (ENEMY_RAIDER, 8, 18, LANE_A), (ENEMY_WOLF, 12, 14, LANE_A),
             (ENEMY_RAIDER, 8, 18, LANE_A)),
        wave(125, 610, 8, (ENEMY_WOLF, 14, 12, LANE_A),
             (ENEMY_RAIDER, 8, 18, LANE_A), (ENEMY_BANNERMAN, 1, 60, LANE_A),
             (ENEMY_RAIDER, 8, 18, LANE_A)),
        wave(145, 760, 10, (ENEMY_WOLF, 14, 12, LANE_A),
             (ENEMY_RAIDER, 10, 16, LANE_A), (ENEMY_SAP, 3, 30, LANE_A),
             (ENEMY_RAIDER, 10, 16, LANE_A)),
        wave(240, 980, 12, (ENEMY_RAIDER, 12, 16, LANE_A),
             (ENEMY_WOLF, 16, 10, LANE_A), (ENEMY_RAIDER, 12, 16, LANE_A),
             (ENEMY_WOLF, 16, 10, LANE_A)),
    ],
    "start_gold": 190,
    "lives": 20,
    "towers": STARTING_TOWERS,
    "art": "carve",
    "plate": "terrain_pines_source.jpg",
}


# --- Frozen Ford -----------------------------------------------------------
# A frozen river. The north road fords it twice while a forest track
# from the west joins it on the near bank.
FORD = {
    "key": "ford",
    "name": "FROZEN FORD",
    "lane_a": [
        (230, 46), (230, 110), (300, 150), (300, 230), (230, 270),
        (230, 366), (388, 366), (450, 366),
    ],
    "lane_b": [(14, 190), (110, 190), (160, 240), (180, 300), (230, 350)],
    "spots": [
        (258, 290), (300, 320), (204, 266), (234, 206), (192, 356),
        (156, 314), (270, 170), (102, 224), (270, 398), (180, 218),
        (300, 98), (360, 398), (132, 266),
    ],
    "posts": [(288, 236), (257, 366)],
    "waves": [
        wave(50, 110, 0, (ENEMY_WOLF, 4, 34, LANE_A)),
        wave(65, 120, 0, (ENEMY_WOLF, 8, 24, LANE_A),
             (ENEMY_RAIDER, 2, 30, LANE_A)),
        wave(70, 125, 1, (ENEMY_WOLF, 8, 22, LANE_A),
             (ENEMY_RAIDER, 4, 26, LANE_A)),
        wave(80, 140, 3, (ENEMY_RAIDER, 6, 24, LANE_A),
             (ENEMY_WOLF, 8, 18, LANE_A), (ENEMY_RAIDER, 2, 26, LANE_B)),
        wave(90, 150, 4, (ENEMY_WOLF, 10, 18, LANE_A),
             (ENEMY_HUSK, 2, 40, LANE_A), (ENEMY_RAIDER, 4, 24, LANE_B)),
        wave(105, 315, 6, (ENEMY_RAIDER, 8, 20, LANE_A),
             (ENEMY_WOLF, 12, 14, LANE_A), (ENEMY_SHAMAN, 1, 60, LANE_A),
             (ENEMY_RAIDER, 6, 20, LANE_B)),
        wave(115, 355, 7, (ENEMY_SKITTER, 10, 12, LANE_A),
             (ENEMY_RAIDER, 8, 18, LANE_A), (ENEMY_WOLF, 12, 14, LANE_A),
             (ENEMY_RAIDER, 8, 18, LANE_B)),
        wave(135, 435, 9, (ENEMY_HUSK, 3, 40, LANE_A),
             (ENEMY_RAIDER, 10, 16, LANE_A), (ENEMY_SKITTER, 10, 12, LANE_A),
             (ENEMY_RAIDER, 10, 16, LANE_B)),
        wave(145, 485, 10, (ENEMY_WOLF, 14, 12, LANE_A),
             (ENEMY_RAIDER, 10, 16, LANE_A), (ENEMY_SAP, 3, 30, LANE_A),
             (ENEMY_RAIDER, 10, 16, LANE_B)),
        wave(240, 625, 12, (ENEMY_RAIDER, 12, 16, LANE_A),
             (ENEMY_WOLF, 16, 10, LANE_A), (ENEMY_RAIDER, 12, 16, LANE_B),
             (ENEMY_WOLF, 16, 10, LANE_B)),
    ],
    "start_gold": 190,
    "lives": 20,
    "towers": STARTING_TOWERS,
    "art": "carve",
    "plate": "terrain_ford_source.jpg",
}


# --- Reed Shallows ---------------------------------------------------------
# A reed marsh: a single causeway snakes in three long rows past the
# frozen pools, so a tower between two rows covers both.
REEDS = {
    "key": "reeds",
    "name": "REED SHALLOWS",
    "lane_a": [
        (14, 80), (400, 80), (436, 116), (400, 156), (90, 156),
        (54, 192), (90, 228), (300, 228), (330, 258), (330, 336),
        (360, 366), (388, 366), (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (342, 110), (144, 194), (210, 116), (246, 200), (276, 122),
        (102, 128), (156, 110), (300, 200), (396, 116), (54, 152),
        (360, 296), (270, 284), (342, 230),
    ],
    "posts": [(372, 156), (118, 228)],
    "waves": [
        wave(55, 200, 0, (ENEMY_WOLF, 6, 30, LANE_A)),
        wave(70, 220, 1, (ENEMY_RAIDER, 4, 28, LANE_A),
             (ENEMY_WOLF, 8, 22, LANE_A)),
        wave(75, 240, 2, (ENEMY_SKITTER, 6, 16, LANE_A),
             (ENEMY_RAIDER, 3, 28, LANE_A)),
        wave(85, 265, 3, (ENEMY_WOLF, 10, 18, LANE_A),
             (ENEMY_HUSK, 1, 40, LANE_A), (ENEMY_RAIDER, 4, 24, LANE_A)),
        wave(105, 385, 5, (ENEMY_SKITTER, 10, 12, LANE_A),
             (ENEMY_RAIDER, 6, 20, LANE_A), (ENEMY_HUSK, 2, 40, LANE_A)),
        wave(125, 565, 7, (ENEMY_RAIDER, 10, 16, LANE_A),
             (ENEMY_SAP, 3, 30, LANE_A), (ENEMY_WOLF, 10, 14, LANE_A)),
        wave(135, 660, 8, (ENEMY_HUSK, 3, 36, LANE_A),
             (ENEMY_SKITTER, 10, 12, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_A)),
        wave(150, 770, 9, (ENEMY_RAIDER, 12, 14, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_WOLF, 14, 12, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A)),
        wave(240, 1025, 12, (ENEMY_RAIDER, 12, 14, LANE_A),
             (ENEMY_HUSK, 3, 36, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_A),
             (ENEMY_SKITTER, 12, 10, LANE_A)),
    ],
    "start_gold": 220,
    "lives": 20,
    "towers": TOWERS_STAGE_2,
    "art": "carve",
    "plate": "terrain_reeds_source.jpg",
}


# --- Foothill Hamlet -------------------------------------------------------
# A village at the foot of the mountains. The hill road winds between
# the cottages and a farm track from the west joins it at the green.
HAMLET = {
    "key": "hamlet",
    "name": "FOOTHILL HAMLET",
    "lane_a": [
        (140, 46), (140, 130), (200, 170), (300, 170), (340, 210),
        (320, 260), (250, 290), (250, 366), (388, 366), (450, 366),
    ],
    "lane_b": [(14, 300), (120, 300), (170, 330), (250, 350)],
    "spots": [
        (306, 308), (252, 224), (222, 314), (204, 110), (306, 218),
        (228, 374), (174, 296), (360, 314), (300, 398), (174, 362),
        (210, 260), (90, 332), (258, 140),
    ],
    "posts": [(310, 180), (250, 336)],
    "waves": [
        wave(55, 125, 0, (ENEMY_WOLF, 6, 30, LANE_A)),
        wave(60, 125, 0, (ENEMY_WOLF, 6, 26, LANE_A),
             (ENEMY_WOLF, 4, 30, LANE_B)),
        wave(75, 150, 2, (ENEMY_SKITTER, 6, 16, LANE_A),
             (ENEMY_RAIDER, 3, 28, LANE_B)),
        wave(85, 165, 3, (ENEMY_WOLF, 10, 18, LANE_A),
             (ENEMY_HUSK, 1, 40, LANE_B), (ENEMY_RAIDER, 4, 24, LANE_B)),
        wave(95, 195, 4, (ENEMY_RAIDER, 8, 20, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_WOLF, 8, 18, LANE_B)),
        wave(115, 295, 6, (ENEMY_WOLF, 14, 14, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_RAIDER, 8, 18, LANE_B)),
        wave(125, 355, 7, (ENEMY_RAIDER, 10, 16, LANE_A),
             (ENEMY_SAP, 3, 30, LANE_B), (ENEMY_WOLF, 10, 14, LANE_B)),
        wave(135, 410, 8, (ENEMY_HUSK, 3, 36, LANE_A),
             (ENEMY_SKITTER, 10, 12, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_B)),
        wave(165, 545, 10, (ENEMY_WOLF, 16, 10, LANE_A),
             (ENEMY_SAP, 4, 26, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B)),
        wave(240, 640, 12, (ENEMY_RAIDER, 12, 14, LANE_A),
             (ENEMY_HUSK, 3, 36, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B),
             (ENEMY_SKITTER, 12, 10, LANE_B)),
    ],
    "start_gold": 220,
    "lives": 20,
    "towers": TOWERS_STAGE_2,
    "art": "carve",
    "plate": "terrain_hamlet_source.jpg",
}


# --- Eagle Crag ------------------------------------------------------------
# An eagle's crag. The road rings the great rock on three sides, so the
# pads inside the ring reach every stretch of it.
CRAG = {
    "key": "crag",
    "name": "EAGLE CRAG",
    "lane_a": [
        (466, 84), (110, 84), (66, 124), (66, 300), (106, 340),
        (280, 340), (310, 366), (388, 366), (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (120, 272), (126, 128), (204, 368), (234, 116), (342, 116),
        (36, 194), (330, 338), (96, 170), (162, 308), (264, 374),
        (36, 278), (180, 116), (96, 224),
    ],
    "posts": [(93, 99), (139, 340)],
    "waves": [
        wave(60, 115, 1, (ENEMY_WOLF, 8, 24, LANE_A)),
        wave(70, 135, 2, (ENEMY_SKITTER, 8, 14, LANE_A),
             (ENEMY_WOLF, 4, 24, LANE_A)),
        wave(90, 165, 4, (ENEMY_WOLF, 12, 16, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 6, 14, LANE_A)),
        wave(100, 195, 5, (ENEMY_RAIDER, 8, 18, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_RAIDER, 6, 20, LANE_A)),
        wave(120, 270, 7, (ENEMY_SKITTER, 14, 10, LANE_A),
             (ENEMY_RAIDER, 8, 18, LANE_A)),
        wave(140, 355, 9, (ENEMY_WOLF, 16, 12, LANE_A),
             (ENEMY_SAP, 4, 26, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_A)),
        wave(150, 400, 10, (ENEMY_RAIDER, 12, 14, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A),
             (ENEMY_SKITTER, 12, 10, LANE_A)),
        wave(180, 505, 12, (ENEMY_WOLF, 18, 10, LANE_A),
             (ENEMY_SAP, 4, 24, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_A)),
        wave(250, 615, 14, (ENEMY_HUSK, 2, 40, LANE_A),
             (ENEMY_RAIDER, 12, 14, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_A)),
    ],
    "start_gold": 240,
    "lives": 20,
    "towers": TOWERS_STAGE_3,
    "art": "carve",
    "plate": "terrain_crag_source.jpg",
}


# --- Old Haul Road ---------------------------------------------------------
# The old haul road zigzags down the scree in two long switchbacks.
HAUL = {
    "key": "haul",
    "name": "OLD HAUL ROAD",
    "lane_a": [
        (14, 140), (120, 90), (240, 140), (360, 90), (440, 140),
        (420, 210), (300, 250), (180, 210), (80, 260), (80, 330),
        (140, 366), (388, 366), (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (354, 176), (156, 284), (144, 158), (252, 188), (300, 170),
        (198, 176), (300, 296), (396, 146), (114, 314), (246, 314),
        (90, 164), (354, 314), (240, 92),
    ],
    "posts": [(427, 185), (80, 279)],
    "waves": [
        wave(60, 165, 1, (ENEMY_WOLF, 8, 24, LANE_A)),
        wave(70, 190, 2, (ENEMY_SKITTER, 8, 14, LANE_A),
             (ENEMY_WOLF, 4, 24, LANE_A)),
        wave(80, 210, 3, (ENEMY_RAIDER, 6, 22, LANE_A),
             (ENEMY_HUSK, 1, 40, LANE_A), (ENEMY_RAIDER, 3, 26, LANE_A)),
        wave(100, 275, 5, (ENEMY_RAIDER, 8, 18, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_RAIDER, 6, 20, LANE_A)),
        wave(110, 325, 6, (ENEMY_HUSK, 3, 36, LANE_A),
             (ENEMY_WOLF, 12, 14, LANE_A), (ENEMY_SAP, 2, 30, LANE_A)),
        wave(130, 440, 8, (ENEMY_RAIDER, 10, 16, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_HUSK, 2, 36, LANE_A),
             (ENEMY_WOLF, 10, 14, LANE_A)),
        wave(140, 505, 9, (ENEMY_WOLF, 16, 12, LANE_A),
             (ENEMY_SAP, 4, 26, LANE_A), (ENEMY_RAIDER, 10, 16, LANE_A)),
        wave(165, 640, 11, (ENEMY_HUSK, 4, 32, LANE_A),
             (ENEMY_RAIDER, 10, 14, LANE_A), (ENEMY_RAIDER, 10, 14, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A)),
        wave(180, 715, 12, (ENEMY_WOLF, 18, 10, LANE_A),
             (ENEMY_SAP, 4, 24, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_A)),
        wave(250, 865, 14, (ENEMY_HUSK, 2, 40, LANE_A),
             (ENEMY_RAIDER, 12, 14, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_A)),
    ],
    "start_gold": 240,
    "lives": 20,
    "towers": TOWERS_STAGE_3,
    "art": "carve",
    "plate": "terrain_haul_source.jpg",
}


# --- Miners' Camp ----------------------------------------------------------
# The miners' camp. A road from the west and one from the east meet
# among the tents and run down to the gate together.
CAMP = {
    "key": "camp",
    "name": "MINERS' CAMP",
    "lane_a": [
        (14, 110), (160, 110), (200, 150), (200, 260), (240, 300),
        (320, 300), (340, 330), (340, 366), (388, 366), (450, 366),
    ],
    "lane_b": [(466, 110), (330, 110), (290, 150), (290, 200), (200, 244)],
    "spots": [
        (270, 242), (228, 158), (168, 182), (354, 146), (300, 332),
        (312, 272), (180, 278), (126, 152), (270, 128), (360, 308),
        (318, 182), (222, 320), (150, 230),
    ],
    "posts": [(200, 184), (296, 300)],
    "waves": [
        wave(60, 100, 1, (ENEMY_WOLF, 8, 22, LANE_A)),
        wave(70, 110, 2, (ENEMY_BURROWER, 4, 30, LANE_A),
             (ENEMY_WOLF, 6, 20, LANE_A)),
        wave(80, 120, 3, (ENEMY_SKITTER, 10, 12, LANE_A),
             (ENEMY_BURROWER, 3, 30, LANE_B)),
        wave(100, 160, 5, (ENEMY_GOLEM, 2, 50, LANE_A),
             (ENEMY_WOLF, 10, 14, LANE_B)),
        wave(110, 185, 6, (ENEMY_HUSK, 2, 40, LANE_A),
             (ENEMY_BURROWER, 6, 22, LANE_A), (ENEMY_RAIDER, 6, 18, LANE_B)),
        wave(130, 245, 8, (ENEMY_RAIDER, 12, 14, LANE_A),
             (ENEMY_SAP, 3, 28, LANE_B), (ENEMY_BURROWER, 6, 20, LANE_B)),
        wave(150, 320, 10, (ENEMY_BURROWER, 10, 16, LANE_A),
             (ENEMY_RAIDER, 10, 14, LANE_B), (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        wave(165, 355, 11, (ENEMY_GOLEM, 4, 36, LANE_A),
             (ENEMY_SKITTER, 14, 9, LANE_A), (ENEMY_RAIDER, 12, 14, LANE_B)),
        wave(220, 440, 13, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_GOLEM, 3, 36, LANE_A), (ENEMY_RAIDER, 12, 12, LANE_B)),
    ],
    "start_gold": 260,
    "lives": 20,
    "towers": TOWERS_STAGE_4,
    "art": "carve",
    "plate": "terrain_camp_source.jpg",
}


# --- Moraine Slope ---------------------------------------------------------
# A moraine slope: one road weaves down between the boulder ridges.
MORAINE = {
    "key": "moraine",
    "name": "MORAINE SLOPE",
    "lane_a": [
        (466, 74), (300, 74), (200, 120), (90, 120), (56, 160),
        (90, 200), (260, 200), (300, 240), (200, 280), (90, 290),
        (60, 330), (120, 366), (388, 366), (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (150, 152), (210, 236), (150, 320), (156, 236), (246, 170),
        (258, 308), (204, 326), (108, 260), (102, 170), (288, 140),
        (312, 314), (72, 224), (384, 104),
    ],
    "posts": [(124, 200), (85, 297)],
    "waves": [
        wave(60, 195, 1, (ENEMY_WOLF, 8, 22, LANE_A)),
        wave(65, 210, 1, (ENEMY_RAIDER, 5, 26, LANE_A),
             (ENEMY_WOLF, 6, 20, LANE_A)),
        wave(80, 240, 3, (ENEMY_SKITTER, 10, 12, LANE_A),
             (ENEMY_BURROWER, 3, 30, LANE_A)),
        wave(90, 270, 4, (ENEMY_RAIDER, 8, 18, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_BURROWER, 4, 26, LANE_A)),
        wave(110, 360, 6, (ENEMY_HUSK, 2, 40, LANE_A),
             (ENEMY_BURROWER, 6, 22, LANE_A), (ENEMY_RAIDER, 6, 18, LANE_A)),
        wave(120, 420, 7, (ENEMY_GOLEM, 3, 44, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A),
             (ENEMY_SKITTER, 12, 10, LANE_A)),
        wave(140, 555, 9, (ENEMY_GOLEM, 3, 40, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_WOLF, 14, 10, LANE_A),
             (ENEMY_HUSK, 2, 36, LANE_A)),
        wave(150, 630, 10, (ENEMY_BURROWER, 10, 16, LANE_A),
             (ENEMY_RAIDER, 10, 14, LANE_A), (ENEMY_BANNERMAN, 1, 60, LANE_A)),
        wave(180, 790, 12, (ENEMY_HUSK, 4, 30, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_BURROWER, 8, 16, LANE_A),
             (ENEMY_SAP, 4, 24, LANE_A)),
        wave(220, 870, 13, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_GOLEM, 3, 36, LANE_A), (ENEMY_RAIDER, 12, 12, LANE_A)),
    ],
    "start_gold": 260,
    "lives": 20,
    "towers": TOWERS_STAGE_4,
    "art": "carve",
    "plate": "terrain_moraine_source.jpg",
}


# --- Crevasse Bridge -------------------------------------------------------
# A crevasse field. The road edges round the cracks in a long S and
# crosses the ice on plank bridges.
CREVASSE = {
    "key": "crevasse",
    "name": "CREVASSE BRIDGE",
    "lane_a": [
        (466, 120), (360, 120), (320, 80), (140, 80), (100, 120),
        (100, 200), (150, 240), (330, 240), (350, 280), (320, 320),
        (200, 320), (170, 366), (388, 366), (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (264, 290), (210, 290), (342, 338), (246, 398), (156, 158),
        (312, 272), (162, 308), (300, 398), (258, 110), (378, 92),
        (174, 110), (174, 206), (330, 164),
    ],
    "posts": [(103, 199), (275, 320)],
    "waves": [
        wave(60, 150, 1, (ENEMY_WOLF, 8, 22, LANE_A)),
        wave(70, 195, 2, (ENEMY_RAIDER, 6, 22, LANE_A),
             (ENEMY_CROW, 6, 20, LANE_A)),
        wave(80, 225, 3, (ENEMY_SLIME, 3, 40, LANE_A),
             (ENEMY_SKITTER, 8, 12, LANE_A)),
        wave(100, 315, 5, (ENEMY_SLIME, 4, 34, LANE_A),
             (ENEMY_WOLF, 12, 12, LANE_A), (ENEMY_BURROWER, 3, 30, LANE_A)),
        wave(110, 380, 6, (ENEMY_GOLEM, 2, 44, LANE_A),
             (ENEMY_CROW, 10, 12, LANE_A), (ENEMY_RAIDER, 6, 18, LANE_A)),
        wave(130, 540, 8, (ENEMY_CROW, 14, 10, LANE_A),
             (ENEMY_HUSK, 3, 36, LANE_A), (ENEMY_SAP, 3, 28, LANE_A)),
        wave(150, 745, 10, (ENEMY_RAIDER, 12, 12, LANE_A),
             (ENEMY_CROW, 12, 10, LANE_A), (ENEMY_BANNERMAN, 1, 60, LANE_A)),
        wave(165, 875, 11, (ENEMY_SLIME, 6, 26, LANE_A),
             (ENEMY_HUSK, 3, 34, LANE_A), (ENEMY_CROW, 14, 9, LANE_A)),
        wave(220, 1155, 13, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_SLIME, 5, 26, LANE_A), (ENEMY_CROW, 14, 9, LANE_A)),
    ],
    "start_gold": 280,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "carve",
    "plate": "terrain_crevasse_source.jpg",
}


# --- Buried Henge ----------------------------------------------------------
# A buried stone circle. The north road curls round the henge and the
# east road comes down to meet it short of the gate.
HENGE = {
    "key": "henge",
    "name": "BURIED HENGE",
    "lane_a": [
        (240, 46), (240, 100), (150, 140), (130, 220), (170, 300),
        (260, 330), (300, 366), (388, 366), (450, 366),
    ],
    "lane_b": [(466, 130), (380, 130), (340, 200), (320, 280), (272, 341)],
    "spots": [
        (330, 314), (240, 272), (252, 362), (174, 182), (396, 182),
        (294, 260), (186, 248), (168, 98), (318, 398), (204, 344),
        (210, 146), (360, 242), (318, 182),
    ],
    "posts": [(133, 224), (264, 335)],
    "waves": [
        wave(60, 60, 1, (ENEMY_WOLF, 8, 22, LANE_A)),
        wave(65, 65, 1, (ENEMY_CROW, 6, 24, LANE_A),
             (ENEMY_WOLF, 6, 20, LANE_B)),
        wave(80, 85, 3, (ENEMY_SLIME, 3, 40, LANE_A),
             (ENEMY_SKITTER, 8, 12, LANE_B)),
        wave(90, 100, 4, (ENEMY_CROW, 10, 14, LANE_A),
             (ENEMY_RAIDER, 6, 20, LANE_B), (ENEMY_SHAMAN, 1, 60, LANE_B)),
        wave(110, 140, 6, (ENEMY_GOLEM, 2, 44, LANE_A),
             (ENEMY_CROW, 10, 12, LANE_B), (ENEMY_RAIDER, 6, 18, LANE_B)),
        wave(120, 165, 7, (ENEMY_SLIME, 5, 30, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A),
             (ENEMY_SKITTER, 12, 10, LANE_B)),
        wave(140, 235, 9, (ENEMY_GOLEM, 3, 40, LANE_A),
             (ENEMY_CROW, 8, 12, LANE_A), (ENEMY_SLIME, 4, 30, LANE_B),
             (ENEMY_SHAMAN, 1, 60, LANE_B)),
        wave(150, 275, 10, (ENEMY_RAIDER, 12, 12, LANE_A),
             (ENEMY_CROW, 12, 10, LANE_B), (ENEMY_BANNERMAN, 1, 60, LANE_B)),
        wave(180, 365, 12, (ENEMY_GOLEM, 4, 34, LANE_A),
             (ENEMY_BURROWER, 6, 18, LANE_B), (ENEMY_CROW, 12, 10, LANE_B)),
        wave(220, 425, 13, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_SLIME, 5, 26, LANE_A), (ENEMY_CROW, 14, 9, LANE_B)),
    ],
    "start_gold": 280,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "carve",
    "plate": "terrain_henge_source.jpg",
}


# --- Ancient Pass ----------------------------------------------------------
# The old pass road climbs in and out of the gorge in three narrow
# columns before it reaches the gate.
GATEROAD = {
    "key": "gateroad",
    "name": "ANCIENT PASS",
    "lane_a": [
        (60, 46), (60, 310), (105, 350), (150, 310), (150, 120),
        (195, 80), (240, 120), (240, 366), (388, 366), (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (186, 170), (90, 260), (192, 278), (96, 170), (186, 224),
        (120, 218), (120, 110), (294, 302), (108, 308), (174, 326),
        (300, 398), (270, 200), (30, 128),
    ],
    "posts": [(150, 257), (240, 209)],
    "waves": [
        wave(60, 150, 2, (ENEMY_RAIDER, 6, 22, LANE_A)),
        wave(75, 150, 3, (ENEMY_RAIDER, 6, 20, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 8, 12, LANE_A)),
        wave(95, 215, 5, (ENEMY_BURROWER, 6, 22, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_RAIDER, 8, 18, LANE_A)),
        wave(105, 250, 6, (ENEMY_HUSK, 3, 36, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_CROW, 10, 12, LANE_A)),
        wave(125, 365, 8, (ENEMY_RAIDER, 12, 14, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_SAP, 4, 26, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A)),
        wave(145, 515, 10, (ENEMY_CROW, 14, 9, LANE_A),
             (ENEMY_HUSK, 3, 34, LANE_A), (ENEMY_SLIME, 5, 26, LANE_A)),
        wave(160, 600, 11, (ENEMY_RAIDER, 14, 12, LANE_A),
             (ENEMY_SHAMAN, 2, 50, LANE_A), (ENEMY_BANNERMAN, 1, 60, LANE_A),
             (ENEMY_GOLEM, 3, 36, LANE_A)),
        wave(190, 805, 13, (ENEMY_SLIME, 6, 24, LANE_A),
             (ENEMY_GOLEM, 4, 32, LANE_A), (ENEMY_CROW, 14, 9, LANE_A)),
        wave(240, 1055, 15, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_GOLEM, 4, 32, LANE_A), (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_SLIME, 5, 24, LANE_A)),
    ],
    "start_gold": 300,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "carve",
    "plate": "terrain_gateroad_source.jpg",
}


# --- Icicle Wood -----------------------------------------------------------
# An icicle wood. The west road loops round the frozen trees and a
# track from the south-west joins it at the first bend.
ICICLE = {
    "key": "icicle",
    "name": "ICICLE WOOD",
    "lane_a": [
        (14, 250), (120, 250), (170, 200), (170, 120), (230, 80),
        (330, 80), (380, 120), (380, 200), (320, 250), (300, 300),
        (300, 366), (388, 366), (450, 366),
    ],
    "lane_b": [(14, 400), (90, 400), (130, 350), (150, 290), (159, 211)],
    "spots": [
        (96, 278), (360, 296), (126, 200), (324, 146), (78, 326),
        (186, 266), (222, 128), (198, 182), (330, 338), (78, 218),
        (342, 194), (276, 110), (138, 146),
    ],
    "posts": [(251, 80), (340, 233)],
    "waves": [
        wave(60, 90, 2, (ENEMY_RAIDER, 6, 22, LANE_A)),
        wave(75, 90, 3, (ENEMY_RAIDER, 6, 20, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_SKITTER, 8, 12, LANE_B)),
        wave(85, 105, 4, (ENEMY_GOLEM, 2, 48, LANE_A),
             (ENEMY_SLIME, 3, 34, LANE_B)),
        wave(105, 150, 6, (ENEMY_HUSK, 3, 36, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_CROW, 10, 12, LANE_B)),
        wave(115, 185, 7, (ENEMY_SLIME, 5, 28, LANE_A),
             (ENEMY_GOLEM, 2, 44, LANE_B), (ENEMY_WOLF, 10, 12, LANE_B)),
        wave(135, 260, 9, (ENEMY_GOLEM, 4, 36, LANE_A),
             (ENEMY_BURROWER, 6, 18, LANE_B)),
        wave(145, 305, 10, (ENEMY_CROW, 14, 9, LANE_A),
             (ENEMY_HUSK, 3, 34, LANE_A), (ENEMY_SLIME, 5, 26, LANE_B)),
        wave(175, 405, 12, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_BURROWER, 8, 16, LANE_B), (ENEMY_SKITTER, 14, 9, LANE_B)),
        wave(190, 475, 13, (ENEMY_SLIME, 6, 24, LANE_A),
             (ENEMY_GOLEM, 4, 32, LANE_A), (ENEMY_CROW, 14, 9, LANE_B)),
        wave(240, 620, 15, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_GOLEM, 4, 32, LANE_A), (ENEMY_GOLEM, 2, 40, LANE_B),
             (ENEMY_SLIME, 5, 24, LANE_B)),
    ],
    "start_gold": 300,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "carve",
    "plate": "terrain_icicle_source.jpg",
}


# --- Rime Vale -------------------------------------------------------------
# A frozen river valley. The road follows the meanders down the vale.
VALE = {
    "key": "vale",
    "name": "RIME VALE",
    "lane_a": [
        (466, 70), (350, 70), (300, 110), (200, 90), (100, 110),
        (60, 160), (100, 210), (220, 200), (320, 230), (330, 280),
        (250, 310), (120, 300), (80, 340), (120, 366), (388, 366),
        (450, 366),
    ],
    "lane_b": [],
    "spots": [
        (174, 338), (150, 140), (240, 266), (276, 332), (162, 248),
        (234, 158), (324, 314), (114, 176), (114, 272), (294, 254),
        (198, 122), (288, 152), (384, 98),
    ],
    "posts": [(134, 207), (146, 302)],
    "waves": [
        wave(65, 115, 2, (ENEMY_WOLF, 10, 18, LANE_A)),
        wave(80, 140, 3, (ENEMY_SLIME, 4, 32, LANE_A),
             (ENEMY_SKITTER, 10, 10, LANE_A)),
        wave(100, 190, 5, (ENEMY_BURROWER, 6, 20, LANE_A),
             (ENEMY_SLIME, 4, 30, LANE_A)),
        wave(110, 230, 6, (ENEMY_RAIDER, 10, 14, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_CROW, 12, 10, LANE_A)),
        wave(130, 315, 8, (ENEMY_GOLEM, 3, 38, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A), (ENEMY_CROW, 14, 9, LANE_A)),
        wave(150, 420, 10, (ENEMY_SLIME, 6, 24, LANE_A),
             (ENEMY_CROW, 12, 9, LANE_A), (ENEMY_GOLEM, 3, 36, LANE_A)),
        wave(165, 485, 11, (ENEMY_RAIDER, 14, 12, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_HUSK, 3, 32, LANE_A),
             (ENEMY_SHAMAN, 1, 60, LANE_A)),
        wave(195, 635, 13, (ENEMY_GOLEM, 5, 30, LANE_A),
             (ENEMY_BURROWER, 8, 16, LANE_A), (ENEMY_SAP, 4, 22, LANE_A)),
        wave(240, 820, 15, (ENEMY_SLIME, 8, 20, LANE_A),
             (ENEMY_HUSK, 4, 28, LANE_A), (ENEMY_GOLEM, 4, 30, LANE_A),
             (ENEMY_CROW, 14, 9, LANE_A)),
    ],
    "start_gold": 320,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "carve",
    "plate": "terrain_vale_source.jpg",
}


# --- Outer Ward ------------------------------------------------------------
# The outer ward of the citadel. The west road runs between the walls
# and a road from the north-east breaks in through a fallen gate.
OUTER = {
    "key": "outer",
    "name": "OUTER WARD",
    "lane_a": [
        (14, 90), (200, 90), (230, 120), (230, 180), (200, 210),
        (110, 210), (80, 240), (80, 280), (110, 310), (330, 310),
        (340, 366), (388, 366), (450, 366),
    ],
    "lane_b": [
        (466, 60), (410, 60), (380, 90), (380, 150), (290, 150),
        (230, 136),
    ],
    "spots": [
        (150, 140), (162, 260), (198, 158), (300, 116), (264, 176),
        (108, 260), (222, 260), (288, 338), (408, 122), (252, 98),
        (102, 158), (360, 314), (348, 98),
    ],
    "posts": [(179, 210), (196, 310)],
    "waves": [
        wave(65, 100, 2, (ENEMY_WOLF, 10, 18, LANE_A)),
        wave(80, 125, 3, (ENEMY_SLIME, 4, 32, LANE_A),
             (ENEMY_SKITTER, 10, 10, LANE_B)),
        wave(90, 145, 4, (ENEMY_GOLEM, 2, 44, LANE_A),
             (ENEMY_CROW, 8, 14, LANE_B), (ENEMY_SHAMAN, 1, 60, LANE_B)),
        wave(110, 205, 6, (ENEMY_RAIDER, 10, 14, LANE_A),
             (ENEMY_BANNERMAN, 1, 60, LANE_A), (ENEMY_CROW, 12, 10, LANE_B)),
        wave(120, 235, 7, (ENEMY_HUSK, 3, 34, LANE_A),
             (ENEMY_SLIME, 5, 26, LANE_B), (ENEMY_SAP, 3, 26, LANE_B)),
        wave(140, 320, 9, (ENEMY_GOLEM, 2, 40, LANE_A),
             (ENEMY_WOLF, 14, 10, LANE_B), (ENEMY_BURROWER, 6, 18, LANE_B)),
        wave(150, 375, 10, (ENEMY_SLIME, 6, 24, LANE_A),
             (ENEMY_CROW, 12, 9, LANE_A), (ENEMY_GOLEM, 3, 36, LANE_B)),
        wave(180, 490, 12, (ENEMY_CROW, 18, 8, LANE_A),
             (ENEMY_SLIME, 6, 22, LANE_B)),
        wave(195, 560, 13, (ENEMY_GOLEM, 5, 30, LANE_A),
             (ENEMY_BURROWER, 8, 16, LANE_B), (ENEMY_SAP, 4, 22, LANE_B)),
        wave(240, 725, 15, (ENEMY_SLIME, 8, 20, LANE_A),
             (ENEMY_HUSK, 4, 28, LANE_A), (ENEMY_GOLEM, 4, 30, LANE_B),
             (ENEMY_CROW, 14, 9, LANE_B)),
    ],
    "start_gold": 320,
    "lives": 20,
    "towers": TOWERS_STAGE_5,
    "art": "carve",
    "plate": "terrain_outer_source.jpg",
}


INTERLUDES = {
    "pass": (PINES, FORD),
    "mere": (REEDS, HAMLET),
    "ridge": (CRAG, HAUL),
    "mine": (CAMP, MORAINE),
    "glacier": (CREVASSE, HENGE),
    "ruins": (GATEROAD, ICICLE),
    "falls": (VALE, OUTER),
}

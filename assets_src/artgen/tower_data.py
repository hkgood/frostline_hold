"""Tower roster and upgrade paths for Frostline Hold.

Every tower has three levels. Moving up a level is a choice: each stage
offers two specialisations and the ones you pick are permanent, so a fully
upgraded tower carries two modifiers. That is the Kingdom Rush shape - the
interesting decision is which two to take, and the branches stack.

Modifiers are percentages or flat pixel deltas on top of the level stats:

    damage_mult    percent on the level's damage
    range_delta    flat pixels added to the range
    cooldown_mult  percent on the level's reload time
    aoe_delta      flat pixels added to the blast radius
    slow_mult      percent on how hard the frost tower slows
    pierce         the shot ignores armour
    burn           the shot sets the target burning
    stun           the shot stops the target dead
    double_shot    the shot also hits a second enemy

This module is the single source of truth: artgen/generate_level.py writes
the tables into components/app_game/data/frostline_level.h.
"""

# Modifier keys and their defaults.
MODIFIER_KEYS = ("damage_mult", "range_delta", "cooldown_mult", "aoe_delta",
                 "slow_mult", "pierce", "burn", "stun", "double_shot")

TOWER_GUARD = 1
TOWER_FROST = 2
TOWER_SIEGE = 3

TOWER_KIND_COUNT = 4          # index 0 is the empty slot
TOWER_LEVEL_COUNT = 3
TOWER_STAGE_COUNT = 2         # level 1->2 and level 2->3
TOWER_OPTION_COUNT = 2        # choices per stage

# Level stats. "cost" is the price of building it; "upgrade" is the price of
# moving up from the previous level.
TOWER_LEVELS = {
    "guard": [
        {"damage": 18, "range": 130, "cooldown": 30, "aoe": 0, "speed": 300,
         "cost": 70, "upgrade": 0},
        {"damage": 24, "range": 140, "cooldown": 26, "aoe": 0, "speed": 320,
         "cost": 0, "upgrade": 90},
        {"damage": 32, "range": 150, "cooldown": 23, "aoe": 0, "speed": 340,
         "cost": 0, "upgrade": 140},
    ],
    "frost": [
        {"damage": 12, "range": 120, "cooldown": 37, "aoe": 44, "speed": 240,
         "cost": 120, "upgrade": 0},
        {"damage": 16, "range": 126, "cooldown": 33, "aoe": 48, "speed": 255,
         "cost": 0, "upgrade": 150},
        {"damage": 21, "range": 132, "cooldown": 29, "aoe": 54, "speed": 270,
         "cost": 0, "upgrade": 220},
    ],
    "siege": [
        {"damage": 30, "range": 170, "cooldown": 60, "aoe": 52, "speed": 180,
         "cost": 160, "upgrade": 0},
        {"damage": 40, "range": 176, "cooldown": 54, "aoe": 58, "speed": 195,
         "cost": 0, "upgrade": 210},
        {"damage": 52, "range": 184, "cooldown": 47, "aoe": 64, "speed": 210,
         "cost": 0, "upgrade": 320},
    ],
}

# The two choices at each upgrade stage. "stage" 0 is level 1->2 and
# "stage" 1 is level 2->3.
TOWER_MODS = {
    "guard": {
        0: [
            {"name": "BALLISTA", "damage_mult": 215,
             "cooldown_mult": 126},
            {"name": "REPEATER", "damage_mult": 85,
             "cooldown_mult": 78, "double_shot": 1},
        ],
        1: [
            {"name": "PIERCE", "damage_mult": 110, "pierce": 1},
            {"name": "MARKSMAN", "damage_mult": 120,
             "range_delta": 55},
        ],
    },
    "frost": {
        0: [
            {"name": "DEEPFREEZE", "damage_mult": 95,
             "range_delta": -25, "slow_mult": 55},
            {"name": "GLACIER", "damage_mult": 90,
             "aoe_delta": 34},
        ],
        1: [
            {"name": "NOVA", "damage_mult": 105, "aoe_delta": 40},
            {"name": "RAY", "damage_mult": 130, "range_delta": 45},
        ],
    },
    "siege": {
        0: [
            {"name": "HAIL", "damage_mult": 130, "aoe_delta": 30},
            {"name": "FIREPOT", "damage_mult": 105, "burn": 1},
        ],
        1: [
            {"name": "CONCUSSION", "damage_mult": 100, "stun": 1},
            {"name": "SHATTER", "damage_mult": 115,
             "cooldown_mult": 88, "aoe_delta": 30},
        ],
    },
}

TOWER_ORDER = ("guard", "frost", "siege")


def modifier(name, **values):
    """Fill in every modifier key so the generated table stays rectangular."""
    row = {key: 0 for key in MODIFIER_KEYS}
    row["name"] = name
    row.update(values)
    return row


def tower_mods(kind, stage):
    """The two choices for one upgrade stage, fully populated."""
    return [modifier(spec["name"], **{k: v for k, v in spec.items()
                                      if k != "name"})
            for spec in TOWER_MODS[kind][stage]]


def tower_levels(kind):
    """The three level rows, fully populated."""
    rows = []
    for index, spec in enumerate(TOWER_LEVELS[kind]):
        row = dict(spec)
        row["level"] = index + 1
        rows.append(row)
    return rows

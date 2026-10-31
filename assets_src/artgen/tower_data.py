"""Tower roster and upgrade paths for Frostline Hold.

Every tower has three levels. Moving up a level is a choice: each stage
offers two specialisations and the ones you pick are permanent, so a fully
upgraded tower carries two modifiers. That is the Kingdom Rush shape - the
interesting decision is which two to take, and the branches stack.

The roster grows along the campaign (artgen/levels.py says which level
offers which tower), and every newcomer answers a problem the old three
cannot:

    guard     arrows, cheap and steady
    frost     a slowing burst
    siege     a slow, heavy area blast
    storm     lightning that leaps between a crowd     - against swarms
    rune      a beam that ignores armour               - against plate
    barracks  soldiers who stand in the road           - against runners
    ember     a fire pot that leaves the road burning  - against columns

Level rows carry two numbers whose meaning depends on the tower:

    extra   storm: how many enemies one bolt reaches
            barracks: how many soldiers the hall keeps
            ember: how long the fire on the road burns, in ticks
    hp      barracks: each soldier's health

Modifiers are percentages or flat deltas on top of the level stats:

    damage_mult    percent on the level's damage
    range_delta    flat pixels added to the range
    cooldown_mult  percent on the level's reload time
    aoe_delta      flat pixels added to the blast radius
    slow_mult      percent on how hard the frost tower slows
    pierce         the shot ignores armour
    burn           the shot sets the target burning
    stun           the shot stops the target dead
    double_shot    the shot also hits a second enemy
    hp_mult        percent on a soldier's health
    extra_delta    flat amount added to "extra"
    hex            the hit strips the target's armour for a while
    slow           the hit (or the fire it leaves) slows

This module is the single source of truth: artgen/generate_level.py writes
the tables into components/app_game/data/frostline_level.h.
"""

# Modifier keys and their defaults.
MODIFIER_KEYS = ("damage_mult", "range_delta", "cooldown_mult", "aoe_delta",
                 "slow_mult", "pierce", "burn", "stun", "double_shot",
                 "hp_mult", "extra_delta", "hex", "slow")

TOWER_GUARD = 1
TOWER_FROST = 2
TOWER_SIEGE = 3
TOWER_STORM = 4
TOWER_RUNE = 5
TOWER_BARRACKS = 6
TOWER_EMBER = 7

TOWER_KIND_COUNT = 8          # index 0 is the empty slot
TOWER_LEVEL_COUNT = 3
TOWER_STAGE_COUNT = 2         # level 1->2 and level 2->3
TOWER_OPTION_COUNT = 2        # choices per stage

# Level stats. "cost" is the price of building it; "upgrade" is the price of
# moving up from the previous level. A speed of 0 means the tower does not
# fire a projectile: lightning and the rune beam strike at once.
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
    # Each leap costs the bolt some of its bite (FL_CHAIN_FALLOFF), so the
    # first enemy takes the most and a long chain is a crowd answer.
    "storm": [
        {"damage": 21, "range": 122, "cooldown": 45, "aoe": 0, "speed": 0,
         "cost": 125, "upgrade": 0, "extra": 3},
        {"damage": 27, "range": 128, "cooldown": 41, "aoe": 0, "speed": 0,
         "cost": 0, "upgrade": 150, "extra": 3},
        {"damage": 35, "range": 134, "cooldown": 37, "aoe": 0, "speed": 0,
         "cost": 0, "upgrade": 220, "extra": 4},
    ],
    # Slow and heavy; the beam always ignores armour.
    "rune": [
        {"damage": 46, "range": 125, "cooldown": 54, "aoe": 0, "speed": 0,
         "cost": 135, "upgrade": 0},
        {"damage": 62, "range": 130, "cooldown": 50, "aoe": 0, "speed": 0,
         "cost": 0, "upgrade": 170},
        {"damage": 82, "range": 136, "cooldown": 46, "aoe": 0, "speed": 0,
         "cost": 0, "upgrade": 240},
    ],
    # "damage" and "cooldown" are one soldier's blow; "range" is how far
    # from the hall the soldiers may stand on the road.
    "barracks": [
        {"damage": 8, "range": 70, "cooldown": 24, "aoe": 0, "speed": 0,
         "cost": 110, "upgrade": 0, "extra": 2, "hp": 70},
        {"damage": 11, "range": 75, "cooldown": 22, "aoe": 0, "speed": 0,
         "cost": 0, "upgrade": 140, "extra": 2, "hp": 100},
        {"damage": 15, "range": 80, "cooldown": 20, "aoe": 0, "speed": 0,
         "cost": 0, "upgrade": 220, "extra": 3, "hp": 130},
    ],
    # "damage" is what the fire does every pulse (FL_ZONE_PULSE_TICKS) to
    # each enemy standing in it; "aoe" is the radius of the burning patch.
    "ember": [
        {"damage": 6, "range": 115, "cooldown": 80, "aoe": 28, "speed": 200,
         "cost": 130, "upgrade": 0, "extra": 75},
        {"damage": 8, "range": 120, "cooldown": 74, "aoe": 30, "speed": 210,
         "cost": 0, "upgrade": 170, "extra": 90},
        {"damage": 11, "range": 126, "cooldown": 68, "aoe": 34, "speed": 220,
         "cost": 0, "upgrade": 250, "extra": 105},
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
    "storm": {
        0: [
            {"name": "OVERLOAD", "damage_mult": 150},
            {"name": "FORK", "damage_mult": 90, "extra_delta": 2},
        ],
        1: [
            {"name": "STATIC", "damage_mult": 100, "stun": 1},
            {"name": "TEMPEST", "damage_mult": 100,
             "cooldown_mult": 70, "range_delta": 20},
        ],
    },
    "rune": {
        0: [
            {"name": "ARCANE", "damage_mult": 145},
            {"name": "HEX", "damage_mult": 95, "hex": 1},
        ],
        1: [
            {"name": "PRISM", "damage_mult": 100, "double_shot": 1},
            {"name": "OBELISK", "damage_mult": 125, "range_delta": 40},
        ],
    },
    "barracks": {
        0: [
            {"name": "SHIELDWALL", "hp_mult": 170},
            {"name": "SPEARS", "damage_mult": 170},
        ],
        1: [
            {"name": "VETERANS", "hp_mult": 120, "extra_delta": 1},
            {"name": "BERSERK", "damage_mult": 125, "cooldown_mult": 65},
        ],
    },
    "ember": {
        0: [
            {"name": "INFERNO", "damage_mult": 130, "aoe_delta": 12},
            {"name": "TAR", "damage_mult": 90, "slow": 1},
        ],
        1: [
            {"name": "WILDFIRE", "extra_delta": 60},
            {"name": "MAGMA", "damage_mult": 150, "cooldown_mult": 85},
        ],
    },
}

# Index order: TOWER_ORDER[i] is kind i + 1.
TOWER_ORDER = ("guard", "frost", "siege", "storm", "rune", "barracks", "ember")

# What the build ring prints under each tower.
TOWER_LABELS = {
    "guard": "GUARD", "frost": "FROST", "siege": "SIEGE", "storm": "STORM",
    "rune": "RUNE", "barracks": "BARRACKS", "ember": "EMBER",
}


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
        row = {"extra": 0, "hp": 0}
        row.update(spec)
        row["level"] = index + 1
        rows.append(row)
    return rows

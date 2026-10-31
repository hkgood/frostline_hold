"""What every level definition shares: the gate, the armoury, the wave helper.

The chapter levels (levels.py) and the interludes between them
(interludes.py) are both written with these.
"""
from artgen import level_geom as geom
from artgen.level_geom import CHAMPION, wave_score_bonus  # noqa: F401
from artgen.tower_data import TOWER_ORDER

GOAL = geom.GOAL

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


def wave(reward, hp_mult, armor_add, *batches):
    """One wave: gold paid at its end, health and armour scaling, batches."""
    entry = {"reward": reward, "hp_mult": hp_mult, "armor_add": armor_add,
             "batches": list(batches)}
    entry["score_bonus"] = wave_score_bonus(entry)
    return entry

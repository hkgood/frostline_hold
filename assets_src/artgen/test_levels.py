"""Every level has to be playable before it can be pretty.

These checks hold the campaign data to the rules the model and the view rely
on, so a new level that breaks one fails here instead of on the device.
"""
import math
import unittest
from pathlib import Path

from artgen import level_geom as geom
from artgen import levels as campaign

AI_DIR = Path(__file__).resolve().parent.parent / "ai"
CASTLE = (372, 282, 476, 388)   # the plate, less the open ground at its west foot


class LevelDataTest(unittest.TestCase):
    def test_keys_are_unique_and_fit_the_record(self):
        keys = [level["key"] for level in campaign.LEVELS]
        self.assertEqual(len(keys), len(set(keys)))
        # The progress record and the map both have eight slots.
        self.assertLessEqual(len(campaign.LEVELS), len(campaign.CAMPAIGN_NODES))
        self.assertLessEqual(len(campaign.LEVELS), 8)

    def test_every_level_sits_on_its_own_campaign_node(self):
        for index, level in enumerate(campaign.LEVELS):
            self.assertEqual(tuple(level["node"]), campaign.CAMPAIGN_NODES[index])

    def test_every_road_ends_at_the_shared_gate_from_the_left(self):
        for level in campaign.LEVELS:
            with self.subTest(level=level["key"]):
                lane_a = level["lane_a"]
                self.assertEqual(tuple(lane_a[-1]), geom.GOAL)
                # The castle art expects the road to arrive level with it.
                self.assertEqual(lane_a[-2][1], geom.GOAL[1])
                self.assertLess(lane_a[-2][0], geom.GOAL[0])

    def test_lane_b_ends_on_lane_a(self):
        for level in campaign.LEVELS:
            if not level["lane_b"]:
                continue
            with self.subTest(level=level["key"]):
                end = level["lane_b"][-1]
                lane_a, _ = campaign.routes(level)
                # Rounding the corners moves lane A off the design point by
                # a few pixels; the model joins at the nearest sample.
                gap = min(math.dist(end, point) for point in lane_a)
                self.assertLess(gap, 8.0)

    def test_build_spots_are_beside_the_road_and_apart(self):
        for level in campaign.LEVELS:
            spots = level["spots"]
            for index, (x, y) in enumerate(spots):
                with self.subTest(level=level["key"], spot=index):
                    road = campaign.distance_to_roads(level, x, y)
                    self.assertGreaterEqual(road, 18, "a tower would stand on the road")
                    self.assertLessEqual(road, 90, "a tower here reaches no road")
                    # The tower is up to 44 px tall; it must clear the HUD.
                    self.assertGreaterEqual(y - 44, geom.PLAY_TOP)
                    self.assertLessEqual(y, 404)
                    self.assertTrue(20 <= x <= 460)
                    inside_castle = (CASTLE[0] <= x <= CASTLE[2]
                                     and CASTLE[1] <= y <= CASTLE[3])
                    self.assertFalse(inside_castle)
                    nearest = min(math.dist((x, y), other)
                                  for j, other in enumerate(spots) if j != index)
                    self.assertGreaterEqual(nearest, 40)

    def test_hero_posts_hold_the_road(self):
        for level in campaign.LEVELS:
            self.assertLessEqual(len(level["posts"]), 2)
            for x, y in level["posts"]:
                with self.subTest(level=level["key"], post=(x, y)):
                    self.assertLessEqual(campaign.distance_to_roads(level, x, y),
                                         geom.HERO_BLOCK_RADIUS)

    def test_waves_fit_the_model(self):
        for level in campaign.LEVELS:
            with self.subTest(level=level["key"]):
                self.assertGreater(len(level["waves"]), 0)
                for wave in level["waves"]:
                    self.assertLessEqual(len(wave["batches"]), geom.BATCH_MAX)
                    self.assertIn("score_bonus", wave)
                    for kind, count, interval, lane in wave["batches"]:
                        self.assertLess(kind, len(geom.ENEMY_STATS))
                        self.assertTrue(0 < count < 256 and 0 < interval < 256)
                        if not level["lane_b"]:
                            self.assertEqual(lane, geom.LANE_A)

    def test_every_level_has_its_plate_and_offers_a_tower(self):
        for level in campaign.LEVELS:
            with self.subTest(level=level["key"]):
                self.assertTrue((AI_DIR / level["plate"]).exists())
                self.assertIn(level["art"], ("engraving", "carve"))
                self.assertNotEqual(campaign.tower_mask(level), 0)
        self.assertTrue((AI_DIR / campaign.CAMPAIGN_PLATE).exists())


if __name__ == "__main__":
    unittest.main()

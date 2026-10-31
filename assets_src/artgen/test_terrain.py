import unittest

from artgen import level_geom
from artgen.canvas import INK_RGB, PAPER_RGB
from artgen.terrain import build_background


class TestLevelGeometry(unittest.TestCase):
    def test_lane_a_starts_at_spawn_and_ends_at_goal(self):
        self.assertEqual(level_geom.PATH_LANE_A[0], level_geom.SPAWN)
        self.assertEqual(level_geom.PATH_LANE_A[-1], level_geom.GOAL)

    def test_lane_b_merges_into_lane_a(self):
        self.assertEqual(level_geom.PATH_LANE_B[-1], level_geom.MERGE_POINT)
        self.assertIn(level_geom.MERGE_POINT, level_geom.PATH_LANE_A)

    def test_all_path_points_are_on_screen(self):
        for lane in level_geom.ALL_PATHS:
            for x, y in lane:
                self.assertTrue(0 <= x < level_geom.VIEW_WIDTH)
                self.assertTrue(0 <= y < level_geom.VIEW_HEIGHT)

    def test_build_spots_are_inside_play_area(self):
        for x, y in level_geom.BUILD_SPOTS:
            self.assertTrue(0 <= x < level_geom.VIEW_WIDTH)
            self.assertTrue(level_geom.PLAY_TOP <= y < level_geom.PLAY_BOTTOM)

    def test_build_spots_keep_clear_of_the_roads(self):
        for spot in level_geom.BUILD_SPOTS:
            self.assertGreaterEqual(
                level_geom.distance_to_paths(spot[0], spot[1]), 18.0, spot
            )

    def test_build_spots_are_unique(self):
        self.assertEqual(len(set(level_geom.BUILD_SPOTS)), len(level_geom.BUILD_SPOTS))

    def test_hero_posts_are_inside_play_area(self):
        for x, y in level_geom.HERO_POSTS:
            self.assertTrue(level_geom.PLAY_TOP <= y < level_geom.PLAY_BOTTOM)
            self.assertGreaterEqual(level_geom.distance_to_paths(x, y), 14.0)


class TestTerrain(unittest.TestCase):
    def test_background_is_480_square(self):
        canvas = build_background()
        self.assertEqual((canvas.width, canvas.height), (480, 480))

    def test_background_contains_only_ink_and_paper(self):
        image = build_background().to_image()
        colors = {pixel[:3] for pixel in image.getdata() if pixel[3] == 255}
        self.assertEqual(colors, {INK_RGB, PAPER_RGB})

    def test_background_is_fully_opaque(self):
        image = build_background().to_image()
        alphas = {pixel[3] for pixel in image.getdata()}
        self.assertEqual(alphas, {255})

    def test_background_has_real_content(self):
        canvas = build_background()
        black = sum(1 for state in canvas.pixels if state == 1)
        self.assertGreater(black, 8000)
        self.assertLess(black, 480 * 480)


if __name__ == "__main__":
    unittest.main()

import unittest

from artgen import shade as S
from artgen.canvas import BLACK, UNSET, WHITE


class TestShade(unittest.TestCase):
    def test_lit_faces_are_brighter_than_faces_turned_away(self):
        toward = S.lit(S.LIGHT[0], S.LIGHT[1], S.LIGHT[2], 1.0)
        away = S.lit(-S.LIGHT[0], -S.LIGHT[1], -S.LIGHT[2], 1.0)
        self.assertGreater(toward, 0.9)
        self.assertLess(away, 0.15)

    def test_dither_produces_only_two_states_and_transparency(self):
        tc = S.ToneCanvas(24, 24)
        S.ellipsoid(tc, 12, 12, 8, 8, 0.9)
        canvas = tc.to_canvas()
        states = {canvas.get(x, y) for y in range(24) for x in range(24)}
        self.assertEqual(states, {UNSET, BLACK, WHITE})

    def test_sphere_is_lit_from_the_upper_left(self):
        tc = S.ToneCanvas(24, 24)
        S.ellipsoid(tc, 12, 12, 10, 10, 1.0)
        self.assertGreater(tc.get(8, 8), tc.get(16, 16))

    def test_outline_rings_the_silhouette_in_ink(self):
        tc = S.ToneCanvas(16, 16)
        for y in range(4, 12):
            for x in range(4, 12):
                tc.put(x, y, 1.0)
        canvas = tc.to_canvas()
        self.assertEqual(canvas.get(4, 8), BLACK)
        self.assertEqual(canvas.get(8, 8), WHITE)

    def test_cast_shadow_only_lands_on_empty_pixels(self):
        tc = S.ToneCanvas(16, 16)
        S.shadow_ellipse(tc, 8, 8, 6, 3, 1.0)
        tc.put(8, 8, 1.0)
        canvas = tc.to_canvas(outline=False)
        self.assertEqual(canvas.get(8, 8), WHITE)
        self.assertEqual(canvas.get(7, 8), BLACK)

    def test_resize_keeps_tone_and_transparency(self):
        tc = S.ToneCanvas(8, 8)
        for y in range(4):
            for x in range(8):
                tc.put(x, y, 0.5)
        small = tc.resized(4, 4)
        self.assertAlmostEqual(small.get(0, 0), 0.5)
        self.assertIsNone(small.get(0, 3))


if __name__ == "__main__":
    unittest.main()

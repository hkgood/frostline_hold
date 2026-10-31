import unittest

from artgen.canvas import (BLACK, INK_RGB, PAPER_RGB, UNSET, WHITE, BAYER8,
                           Canvas, dither_rect)


class TestCanvas(unittest.TestCase):
    def test_set_get_roundtrip(self):
        c = Canvas(8, 8)
        c.set(3, 4, WHITE)
        self.assertEqual(c.get(3, 4), WHITE)

    def test_unset_reads_as_minus_one(self):
        c = Canvas(8, 8)
        self.assertEqual(c.get(4, 3), UNSET)

    def test_black_is_distinct_from_unset(self):
        c = Canvas(8, 8)
        c.set(1, 1, BLACK)
        self.assertEqual(c.get(1, 1), BLACK)
        self.assertTrue(c.is_set(1, 1))
        self.assertFalse(c.is_set(2, 2))

    def test_out_of_bounds_is_ignored(self):
        c = Canvas(4, 4)
        c.set(-1, 0, WHITE)
        c.set(99, 99, WHITE)
        self.assertEqual(c.count_set(), 0)

    def test_bayer_matrix_is_permutation_of_0_to_63(self):
        flat = sorted(v for row in BAYER8 for v in row)
        self.assertEqual(flat, list(range(64)))

    def test_dither_density_zero_draws_nothing(self):
        c = Canvas(16, 16)
        dither_rect(c, 0, 0, 16, 16, 0.0)
        self.assertEqual(c.count_set(), 0)

    def test_dither_density_one_fills_everything(self):
        c = Canvas(16, 16)
        dither_rect(c, 0, 0, 16, 16, 1.0)
        self.assertEqual(c.count_set(), 16 * 16)

    def test_dither_half_density_lands_near_half(self):
        c = Canvas(64, 64)
        dither_rect(c, 0, 0, 64, 64, 0.5)
        ratio = c.count_set() / (64 * 64)
        self.assertTrue(0.45 < ratio < 0.55, ratio)

    def test_dither_pattern_is_tileable_at_eight_pixels(self):
        c = Canvas(16, 16)
        dither_rect(c, 0, 0, 16, 16, 0.5)
        for y in range(8):
            for x in range(8):
                self.assertEqual(c.get(x, y), c.get(x + 8, y))
                self.assertEqual(c.get(x, y), c.get(x, y + 8))

    def test_to_image_has_only_three_rgba_values(self):
        c = Canvas(8, 8)
        c.set(1, 1, WHITE)
        c.set(2, 2, BLACK)
        img = c.to_image()
        self.assertEqual(
            sorted(set(img.getdata())),
            sorted([(0, 0, 0, 0), INK_RGB + (255,), PAPER_RGB + (255,)]),
        )

    def test_blit_keeps_transparent_source_pixels_out(self):
        dst = Canvas(8, 8)
        dst.fill_rect(0, 0, 8, 8, WHITE)
        src = Canvas(4, 4)
        src.set(1, 1, BLACK)
        dst.blit(src, 2, 2)
        self.assertEqual(dst.get(3, 3), BLACK)
        self.assertEqual(dst.get(2, 2), WHITE)

    def test_line_connects_endpoints(self):
        c = Canvas(16, 16)
        c.line(2, 2, 12, 2, BLACK)
        self.assertEqual(c.get(2, 2), BLACK)
        self.assertEqual(c.get(12, 2), BLACK)


if __name__ == "__main__":
    unittest.main()

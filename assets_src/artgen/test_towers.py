"""The tower art and the points the game fires from must agree.

The model launches every bolt, stone and beam from the rig points in
components/app_game/data/frostline_tower_rig.h; the art is drawn to the same
rig. These tests catch the two drifting apart: a shot leaving thin air, or a
header that no longer matches the art.
"""

import subprocess
import sys
import unittest
from pathlib import Path

from artgen import tower_art as art
from artgen import tower_rig as rig

ARTGEN = Path(__file__).resolve().parent

KINDS = ("guard", "frost", "siege", "storm", "rune", "barracks", "ember")


def _to_cell(point):
    """A rig point (px right of the foot, px above it) in a tower cell."""
    dx, h = point
    return rig.ORIGIN_X + dx, rig.FOOT_ROW - h


def _distance_to_ink_or_paper(canvas, x, y, reach):
    """How far the nearest drawn pixel lies from (x, y), or None past reach."""
    best = None
    for dy in range(-reach, reach + 1):
        for dx in range(-reach, reach + 1):
            px, py = x + dx, y + dy
            if 0 <= px < canvas.width and 0 <= py < canvas.height \
                    and canvas.is_set(px, py):
                d = max(abs(dx), abs(dy))
                best = d if best is None else min(best, d)
    return best


class TestTowerRig(unittest.TestCase):
    def test_the_rig_header_is_up_to_date(self):
        result = subprocess.run(
            [sys.executable, str(ARTGEN / "generate_tower_rig.py"), "--check"],
            capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_every_shot_leaves_something_drawn(self):
        """Each muzzle sits on the art, give or take the glow round it."""
        for kind in KINDS:
            for variant, point in enumerate(r["muzzle"] for r in rig.tower_rigs(kind)):
                if kind == "barracks":
                    continue
                if kind == "siege":
                    frame = art._siege_frame(variant, "stop", 1)
                elif kind == "ember":
                    frame = art._ember_tower(variant, 0)
                elif kind == "guard":
                    continue   # the head carries the bolt; checked below
                else:
                    frame = art.STILL_BUILDERS[kind](variant)
                x, y = _to_cell(point)
                d = _distance_to_ink_or_paper(frame, x, y, 4)
                self.assertIsNotNone(d, "%s variant %d muzzle %s is in thin air"
                                     % (kind, variant, point))

    def test_the_bolt_tip_is_where_the_model_looses_it(self):
        """The head is drawn centred on the mount; the bolt's tip lies
        GUARD_REACH ahead of it in the aimed direction."""
        steps = ((1, 0), (0.7071, 0.7071), (0, 1), (-0.7071, 0.7071),
                 (-1, 0), (-0.7071, -0.7071), (0, -1), (0.7071, -0.7071))
        for head in rig.GUARD_HEADS:
            for direction, (sx, sy) in enumerate(steps):
                part = art.guard_head(head, "loaded", direction)
                x = int(round(art.PART_CENTER + sx * rig.GUARD_REACH))
                y = int(round(art.PART_CENTER + sy * rig.GUARD_REACH))
                d = _distance_to_ink_or_paper(part, x, y, 2)
                self.assertIsNotNone(d, "%s dir %d: no bolt at its tip"
                                     % (head, direction))

    def test_the_waiting_stone_sits_in_the_bucket(self):
        for variant, r in enumerate(rig.tower_rigs("siege")):
            frame = art._siege_frame(variant, "cocked", 1)
            x, y = _to_cell(r["load"])
            self.assertIsNotNone(_distance_to_ink_or_paper(frame, x, y, 3),
                                 "siege variant %d load %s" % (variant, r["load"]))

    def test_the_catapult_faces_both_ways(self):
        """The left frame is the right frame turned round, so a mirrored rig
        point lands on the same part of the machine."""
        for variant in range(len(art.VARIANT_NAMES)):
            right = art._siege_frame(variant, "stop", 1)
            left = art._siege_frame(variant, "stop", -1)
            mx, my = _to_cell(rig.tower_rigs("siege")[variant]["muzzle"])
            mirrored = 2 * rig.ORIGIN_X - mx
            self.assertIsNotNone(_distance_to_ink_or_paper(right, mx, my, 3))
            self.assertIsNotNone(_distance_to_ink_or_paper(left, mirrored, my, 3))

    def test_every_frame_the_view_asks_for_exists(self):
        names = {name for name, _ in art.unit_entries()}
        names |= {name for name, _ in art.part_entries()}
        for kind in ("guard", "frost", "storm", "rune", "barracks"):
            for v in art.VARIANT_NAMES:
                self.assertIn("tower_%s_%s" % (kind, v), names)
        for v in range(len(art.VARIANT_NAMES)):
            for pose, _deg in rig.SIEGE_POSES:
                for facing in ("r", "l"):
                    self.assertIn(art.siege_frame_name(v, pose, facing), names)
            for frame in range(art.EMBER_FLICKER_FRAMES + 1):
                self.assertIn(art.ember_frame_name(v, frame), names)
        for head in rig.GUARD_HEADS:
            for state in rig.GUARD_STATES:
                for direction in range(rig.GUARD_DIRECTIONS):
                    self.assertIn(art.guard_head_name(head, state, direction), names)
        for part in ("spare_pot", "orbit_shard", "orbit_stone", "orbit_rune",
                     "door_open"):
            self.assertIn(part, names)


if __name__ == "__main__":
    unittest.main()

"""Building blocks shared by the tower art and the rest of the roster:
rectangles of tone, doorways, snow on ledges, ice prisms, plinths, wheels.

Coordinates and tones follow artgen.shade: pixels in the target cell, tone
0.0 ink .. 1.0 paper, light from the upper left.
"""
import math

from artgen import shade as S

WALK_FRAMES = 6


def phase(frame, frames=WALK_FRAMES):
    return frame / float(frames) * 2.0 * math.pi


def rect(tc, x0, y0, x1, y1, tone):
    for y in range(int(y0), int(y1)):
        for x in range(int(x0), int(x1)):
            tc.put(x, y, tone)


def arch_door(tc, cx, bottom, width, height, tone=0.03):
    """A dark doorway with a round head, set into a wall."""
    half = width / 2.0
    rect(tc, cx - half, bottom - height + half, cx + half, bottom, tone)
    for y in range(int(bottom - height), int(bottom - height + half) + 1):
        for x in range(int(cx - half), int(cx + half) + 1):
            dx = (x + 0.5 - cx) / half
            dy = (y + 0.5 - (bottom - height + half)) / half
            if dx * dx + dy * dy <= 1.0:
                tc.put(x, y, tone)


def snow_cap(tc, cx, y, half, depth=2):
    """A soft drift of snow sitting on a ledge: the brightest band."""
    for dy in range(depth):
        inset = dy * 0.5
        for x in range(int(cx - half + inset), int(cx + half - inset) + 1):
            u = (x + 0.5 - cx) / max(1.0, half)
            tc.put(x, y - dy, S.lit(u * 0.6, -1.0, 0.5, 1.25))


def crystal(tc, cx, top, bottom, half, albedo=1.0, lean=0.0):
    """A hexagonal ice prism with a pointed tip, three faces visible.

    Each facet has its own normal, so the column reads as cut ice: bright
    left face, mid front, dark right face, and a hard specular on the tip.
    """
    inner = half * 0.38
    shoulder = top + half * 1.5
    ax = cx + lean
    left = [(cx - half, shoulder + 1), (cx - inner, shoulder + 2),
            (cx - inner, bottom + 1), (cx - half, bottom)]
    front = [(cx - inner, shoulder + 2), (cx + inner, shoulder + 2),
             (cx + inner, bottom + 1), (cx - inner, bottom + 1)]
    right = [(cx + inner, shoulder + 2), (cx + half, shoulder + 1),
             (cx + half, bottom), (cx + inner, bottom + 1)]

    def vein(x, y, _u, _v):
        # A diagonal refraction streak through the front face.
        return -0.35 if int(x - cx + y * 0.5) % 9 == 0 else 0.0

    S.polygon(tc, left, (-0.85, -0.1, 0.55), albedo, spec=0.2)
    S.polygon(tc, front, (0.15, 0.0, 1.0), albedo * 0.72, tex=vein)
    S.polygon(tc, right, (0.95, 0.1, 0.3), albedo * 0.8)
    S.polygon(tc, [(ax, top), (cx - half, shoulder + 1),
                   (cx - inner, shoulder + 2)], (-0.6, -0.8, 0.5), albedo,
              spec=0.6)
    S.polygon(tc, [(ax, top), (cx - inner, shoulder + 2),
                   (cx + inner, shoulder + 2)], (0.0, -0.6, 0.8), albedo,
              spec=0.4)
    S.polygon(tc, [(ax, top), (cx + inner, shoulder + 2),
                   (cx + half, shoulder + 1)], (0.7, -0.5, 0.5), albedo)
    S.glint(tc, int(cx - inner), int(shoulder + 3), 1 if half > 4 else 0)


def stone_plinth(tc, cx, base, r, height=5, seed=0):
    """A squat round footing of big blocks under a tower."""
    S.cylinder(tc, cx, base - height, base, r, 0.92,
               tex=S.masonry(r, course=height, blocks=9, seed=seed), lid=0.3)


def wheel(tc, cx, cy, r, albedo=0.7):
    """A spoked cart wheel, seen nearly face on."""
    def spokes(x, y, u, v):
        angle = math.atan2(v, u * 0.8)
        ring = math.sqrt(u * u + v * v)
        if ring > 0.86:
            return 0.1
        if ring > 0.7:
            return -0.7
        return -0.6 if (angle * 6.0 / math.pi) % 2.0 < 0.5 else 0.0

    S.ellipsoid(tc, cx, cy, r * 0.8, r, albedo, depth=0.35, tex=spokes)
    S.ellipsoid(tc, cx, cy, 1.4, 1.6, 0.9, spec=0.5)

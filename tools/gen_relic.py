#!/usr/bin/env python3
"""Geometry for Models/Relic -- the carryable consecrated relic.

A small stone cross, roughly 0.6 m tall, built from two boxes so it reads as a
hand held object rather than one of the cemetery's grave monuments (which are
all 1.3 m and up, on plinths, and pivot at the base of a slab).

Pivot is the base of the upright, horizontally centred, so a world placement
sits it straight on the terrain height with no offset -- unlike the ammo crate,
whose source cube is centred and needs half its height added.

Prints the payload for the traktor MCP create_mesh_from_geometry tool. Winding
is fixed the same way tools/gen_crate.py does it: Traktor wants front faces
clockwise as seen from outside, so a face whose cross(p1 - p0, p2 - p0) points
*along* its outward normal is reversed.
"""

import json

H = 0.60          # overall height
UP_W = 0.085      # upright thickness (square in plan)
ARM_W = 0.34      # crossbar span
ARM_H = 0.10      # crossbar height
ARM_Y = 0.36      # crossbar sits with its base here
BASE_H = 0.07     # slightly wider foot, so it does not look like a floating stick
BASE_W = 0.15


def box(cx, cy, cz, sx, sy, sz):
    """(positions, faces) for an axis aligned box centred on (cx, cy, cz).
    Faces are listed counter clockwise from outside; winding is fixed below."""
    hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
    p = [
        (cx - hx, cy - hy, cz - hz), (cx + hx, cy - hy, cz - hz),
        (cx + hx, cy + hy, cz - hz), (cx - hx, cy + hy, cz - hz),
        (cx - hx, cy - hy, cz + hz), (cx + hx, cy - hy, cz + hz),
        (cx + hx, cy + hy, cz + hz), (cx - hx, cy + hy, cz + hz),
    ]
    faces = [
        ((0, 0, 1),  [4, 5, 6, 7]),
        ((0, 0, -1), [1, 0, 3, 2]),
        ((1, 0, 0),  [5, 1, 2, 6]),
        ((-1, 0, 0), [0, 4, 7, 3]),
        ((0, 1, 0),  [7, 6, 2, 3]),
        ((0, -1, 0), [0, 1, 5, 4]),
    ]
    return p, faces


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


positions, normals, polygons = [], [], []

# Per face vertices throughout: the cross has hard edges everywhere, and shared
# positions would average the normals into a soft, muddy blob at this size.
for cx, cy, cz, sx, sy, sz in (
    (0.0, BASE_H * 0.5, 0.0, BASE_W, BASE_H, BASE_W),                  # foot
    (0.0, H * 0.5, 0.0, UP_W, H, UP_W),                                # upright
    (0.0, ARM_Y + ARM_H * 0.5, 0.0, ARM_W, ARM_H, UP_W),               # crossbar
):
    src, faces = box(cx, cy, cz, sx, sy, sz)
    for n, idx in faces:
        c = [src[i] for i in idx]
        if dot(cross(sub(c[1], c[0]), sub(c[2], c[0])), n) > 0:
            c = [c[0], c[3], c[2], c[1]]
        base = len(positions)
        for v in c:
            positions.append([round(v[0], 4), round(v[1], 4), round(v[2], 4)])
            normals.append(list(n))
        polygons.append([base, base + 1, base + 2, base + 3])

print(json.dumps({"positions": positions, "normals": normals, "polygons": polygons}))

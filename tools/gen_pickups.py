#!/usr/bin/env python3
"""Geometry for the two pickups, so they look like what they give you.

Both were coloured boxes: Models/Pickup (a green cube) for health and
Models/Crate scaled to 0.45 (a blue cube) for ammunition. Nothing about either
said what it was -- you learned it by walking over one.

Each is now one mesh built from two parts in two materials:

  Models/PickupAmmo     an open wooden crate, with seven brass cartridges in it
  Models/PickupHealth   an apothecary flask of something red, with a cork stopper

One mesh, not one per part. A Model carries a material list and a material index
per polygon, so both parts ride a single MeshAsset -- whose materialShaders binds
each material *name* to its shader graph -- and a single MeshComponent draws them.
Splitting them across two components would only cost the entity components and
mean keeping their transforms in step by hand for nothing.

That is also why this does not go through create_mesh_from_geometry, which assigns
one material name to every polygon it is given. The parts are assembled into a
blank model instead (model_open/model_edit/model_save), which is the same Model API
with the material index left in the author's hands.

The pivot is on the ground at the object's centre, so both parts line up by
construction and the entity mounts the mesh at its own origin. Height in the world
is not this file's business: Scene 2's Pickups layer is aligned to the terrain by
an AlignToTerrainOperationData, whose `offset` decides how far above the ground
they ride (0 rests them on it, now that the pivot is the base).

Winding follows tools/gen_relic.py: Traktor wants front faces clockwise as seen
from outside, so any face whose cross(p1 - p0, p2 - p0) points *along* its outward
normal is reversed. Normals are given explicitly everywhere -- flat per face on the
boxes, and smooth around the axis on everything revolved, which is what keeps a
0.03 m cartridge from reading as a heptagonal nut.

Writes both meshes through the traktor MCP bridge. Run with the editor open:

    python3 tools/gen_pickups.py
"""

import json
import math
import urllib.request

BRIDGE = "http://127.0.0.1:13880/"

# The two meshes, and the parts and materials each is assembled from. Material
# colours are set on the model as well as bound to a shader graph: the shader is what
# the game renders, the model's own material is what the editor's mesh preview shows,
# and having them disagree makes the preview a liar.
PICKUPS = {
    "PickupAmmo": {
        "guid": "{7E868FDD-1D35-4004-8DC6-8478019713A5}",
        "parts": [
            ("Wood", "ammo_box", "{FB9E9188-3EC8-4015-A562-2DAE681C1B1C}",
             dict(color=[0.30, 0.17, 0.08, 1.0], roughness=0.9, metalness=0.0)),
            ("Brass", "ammo_rounds", "{B1CCEE00-0000-4000-8000-0000000000B1}",
             dict(color=[0.85, 0.62, 0.22, 1.0], roughness=0.28, metalness=0.5)),
        ],
    },
    "PickupHealth": {
        "guid": "{70DB34A7-71E5-BF40-AA13-4ABEE096FD9B}",
        "parts": [
            ("Glass", "health_bottle", "{130BC3FC-B38D-AC47-843A-DF02406067EF}",
             dict(color=[0.60, 0.045, 0.06, 1.0], roughness=0.18, metalness=0.0)),
            ("Cork", "health_cork", "{B1CCEE00-0000-4000-8000-0000000000C0}",
             dict(color=[0.42, 0.26, 0.13, 1.0], roughness=0.95, metalness=0.0)),
        ],
    },
}

# ---------------------------------------------------------------- ammo crate

CRATE_W = 0.30       # outer width  (x)
CRATE_D = 0.22       # outer depth  (z)
FLOOR_H = 0.025      # thickness of the crate floor
WALL_H = 0.105       # how far the walls stand above that floor
WALL_T = 0.022       # ...and how thick they are

# Cartridges. They stand on the crate floor and deliberately clear the rim: the
# silhouette against the forest is the whole point, and a round that hides inside
# the box only reads from directly above.
CASE_R = 0.0135      # case radius
CASE_TOP = 0.115     # case shoulder, measured from the crate floor
TIP_R = 0.006        # bullet radius where it flattens off
TIP_TOP = 0.16       # ...and where that is
ROUND_SEGS = 8       # sides per cartridge; brass at 3 cm reads round enough at 8

# Hand-packed rather than gridded, but fixed rather than random, so the mesh is the
# same every time it is generated. Offsets are from the crate centre, in metres.
ROUNDS = [
    (-0.092, -0.045), (-0.048, 0.038), (-0.004, -0.052),
    (0.038, 0.041), (0.082, -0.038), (0.100, 0.046),
    (-0.086, 0.050),
]

# --------------------------------------------------------------- health flask

# Profile of the bottle, revolved: (radius, height). Square-shouldered and a bit
# over life size, because it has to be picked out of ferns in the dark.
BOTTLE = [
    (0.000, 0.000), (0.072, 0.000),   # foot
    (0.076, 0.022),                   # heel, flared so it does not read as a tube
    (0.072, 0.150),                   # body, tapering very slightly inward
    (0.052, 0.190),                   # shoulder
    (0.026, 0.215),                   # neck
    (0.026, 0.262),
    (0.032, 0.272),                   # lip
    (0.032, 0.286),
    (0.000, 0.286),                   # mouth, capped so the cork sits on solid glass
]
BOTTLE_SEGS = 12

CORK_R0 = 0.025      # where it disappears into the neck
CORK_R1 = 0.031      # ...and the wider head that is left proud of it
CORK_Y0 = 0.268
CORK_Y1 = 0.318


def cross(a, b):
    return (a[1] * b[2] - a[2] * b[1],
            a[2] * b[0] - a[0] * b[2],
            a[0] * b[1] - a[1] * b[0])


def sub(a, b):
    return (a[0] - b[0], a[1] - b[1], a[2] - b[2])


def dot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2]


def norm(v):
    l = math.sqrt(dot(v, v))
    return (v[0] / l, v[1] / l, v[2] / l) if l > 1e-9 else (0.0, 1.0, 0.0)


class Mesh:
    """Accumulates positions/normals/polygons, fixing winding as faces arrive."""

    def __init__(self):
        self.positions = []
        self.normals = []
        self.polygons = []

    def face(self, verts, normals, outward):
        """One polygon. `outward` is the side it should face; the ring is reversed
        when the winding says it faces the other way."""
        if len(verts) >= 3:
            wound = cross(sub(verts[1], verts[0]), sub(verts[2], verts[0]))
            if dot(wound, outward) > 0.0:
                verts = list(reversed(verts))
                normals = list(reversed(normals))
        base = len(self.positions)
        for p, n in zip(verts, normals):
            self.positions.append([round(p[0], 5), round(p[1], 5), round(p[2], 5)])
            self.normals.append([round(n[0], 5), round(n[1], 5), round(n[2], 5)])
        self.polygons.append(list(range(base, base + len(verts))))

    def box(self, cx, cy, cz, sx, sy, sz):
        """Axis aligned box, hard edges (a vertex per face)."""
        hx, hy, hz = sx * 0.5, sy * 0.5, sz * 0.5
        p = [(cx - hx, cy - hy, cz - hz), (cx + hx, cy - hy, cz - hz),
             (cx + hx, cy + hy, cz - hz), (cx - hx, cy + hy, cz - hz),
             (cx - hx, cy - hy, cz + hz), (cx + hx, cy - hy, cz + hz),
             (cx + hx, cy + hy, cz + hz), (cx - hx, cy + hy, cz + hz)]
        for n, idx in (((0, 0, 1), [4, 5, 6, 7]), ((0, 0, -1), [1, 0, 3, 2]),
                       ((1, 0, 0), [5, 1, 2, 6]), ((-1, 0, 0), [0, 4, 7, 3]),
                       ((0, 1, 0), [7, 6, 2, 3]), ((0, -1, 0), [0, 1, 5, 4])):
            self.face([p[i] for i in idx], [n] * 4, n)

    def revolve(self, profile, segs, ox=0.0, oz=0.0, heading=0.0):
        """Revolve a (radius, height) profile about +Y at (ox, oz).

        Normals are the true surface normals -- smooth around the axis, and hard
        wherever the profile changes slope, since every face carries its own
        vertices. Zero-radius ends are emitted as triangles rather than degenerate
        quads. `heading` only turns the facets, which matters when several copies
        stand side by side and would otherwise line up like a machined set."""
        for (r0, y0), (r1, y1) in zip(profile, profile[1:]):
            if abs(r1 - r0) < 1e-9 and abs(y1 - y0) < 1e-9:
                continue
            # Outward normal of this segment in the (radius, height) plane.
            nr, ny = norm((y1 - y0, -(r1 - r0), 0.0))[:2]
            for s in range(segs):
                a0 = heading + 2.0 * math.pi * s / segs
                a1 = heading + 2.0 * math.pi * (s + 1) / segs
                ring = []
                for r, y, a in ((r0, y0, a0), (r0, y0, a1), (r1, y1, a1), (r1, y1, a0)):
                    ring.append((
                        (ox + r * math.cos(a), y, oz + r * math.sin(a)),
                        (nr * math.cos(a), ny, nr * math.sin(a)),
                    ))
                # Drop the duplicated vertex at a pole, so a cap comes out a triangle.
                if r0 < 1e-9:
                    ring = [ring[0], ring[2], ring[3]]
                elif r1 < 1e-9:
                    ring = [ring[0], ring[1], ring[2]]
                verts = [v for v, _ in ring]
                normals = [n for _, n in ring]
                out = norm((sum(n[0] for n in normals), sum(n[1] for n in normals),
                            sum(n[2] for n in normals)))
                self.face(verts, normals, out)


def ammo_box():
    """An open crate: a floor and four walls, pivot on the ground at its centre."""
    m = Mesh()
    m.box(0.0, FLOOR_H * 0.5, 0.0, CRATE_W, FLOOR_H, CRATE_D)
    wy = FLOOR_H + WALL_H * 0.5
    # Long walls run the full width; the short ones fit between them, so the corners
    # are a lap joint rather than two boxes fighting over the same volume.
    m.box(0.0, wy, (CRATE_D - WALL_T) * 0.5, CRATE_W, WALL_H, WALL_T)
    m.box(0.0, wy, -(CRATE_D - WALL_T) * 0.5, CRATE_W, WALL_H, WALL_T)
    inner_d = CRATE_D - 2.0 * WALL_T
    m.box((CRATE_W - WALL_T) * 0.5, wy, 0.0, WALL_T, WALL_H, inner_d)
    m.box(-(CRATE_W - WALL_T) * 0.5, wy, 0.0, WALL_T, WALL_H, inner_d)
    return m


def ammo_rounds():
    """Cartridges standing on the crate floor: a case, a shoulder, a blunt tip."""
    m = Mesh()
    y0 = FLOOR_H
    profile = [
        (0.000, y0), (CASE_R, y0),                     # base
        (CASE_R, y0 + CASE_TOP * 0.62),                # case wall
        (CASE_R * 0.93, y0 + CASE_TOP),                # slight neck in at the shoulder
        (TIP_R, y0 + TIP_TOP - 0.012),                 # bullet
        (TIP_R * 0.7, y0 + TIP_TOP),
        (0.000, y0 + TIP_TOP),                         # flattened off, not needle sharp
    ]
    for i, (ox, oz) in enumerate(ROUNDS):
        m.revolve(profile, ROUND_SEGS, ox, oz, heading=0.31 * i)
    return m


def health_bottle():
    m = Mesh()
    m.revolve(BOTTLE, BOTTLE_SEGS)
    return m


def health_cork():
    m = Mesh()
    m.revolve([
        (0.000, CORK_Y0), (CORK_R0, CORK_Y0),
        (CORK_R1, CORK_Y1 - 0.008),
        (CORK_R1 * 0.88, CORK_Y1),
        (0.000, CORK_Y1),
    ], BOTTLE_SEGS)
    return m


def call(name, args, timeout=300):
    body = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "tools/call",
                       "params": {"name": name, "arguments": args}}).encode()
    req = urllib.request.Request(BRIDGE, data=body,
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        res = json.loads(r.read().decode())
    text = json.dumps(res)
    if '"isError":true' in text or '\\"error\\"' in text:
        raise SystemExit(f"{name} failed: {text[:400]}")
    # Tool results come back as a JSON document inside a text content block.
    for c in res.get("result", {}).get("content", []):
        if c.get("type") == "text":
            try:
                return json.loads(c["text"])
            except ValueError:
                return c["text"]
    return res


def edits_for(mesh, material_index, first_position, first_vertex):
    """Model edits appending one part's geometry under a single material index.

    Positions, normals and vertices are added in order, so their indices run from
    the counts already in the model -- which is what lets the polygons below name
    their vertices without reading anything back."""
    edits = []
    for p in mesh.positions:
        edits.append({"op": "addPosition", "value": p})
    for n in mesh.normals:
        edits.append({"op": "addNormal", "value": n})
    for i in range(len(mesh.positions)):
        edits.append({"op": "addVertex",
                      "position": first_position + i,
                      "normal": first_position + i})
    for poly in mesh.polygons:
        edits.append({"op": "addPolygon",
                      "material": material_index,
                      "vertices": [first_vertex + i for i in poly]})
    return edits


BUILDERS = {
    "ammo_box": ammo_box,
    "ammo_rounds": ammo_rounds,
    "health_bottle": health_bottle,
    "health_cork": health_cork,
}


def main():
    for name, spec in PICKUPS.items():
        handle = call("model_open", {"blank": True})["handle"]
        edits = []
        bindings = {}
        positions = 0
        for index, (material, builder, shader, props) in enumerate(spec["parts"]):
            edits.append(dict(op="addMaterial", name=material, **props))
            bindings[material] = shader
            m = BUILDERS[builder]()
            edits += edits_for(m, index, positions, positions)
            positions += len(m.positions)

        call("model_edit", {"handle": handle, "edits": edits})
        call("model_apply_operation", {"handle": handle,
                                       "operations": [{"name": "Triangulate"}]})
        summary = call("model_inspect", {"handle": handle})
        call("model_save", {"handle": handle, "meshAsset": {
            "path": "Models/" + name,
            "fileName": "Models/" + name + ".tmd",
            "guid": spec["guid"],
        }})
        call("model_close", {"handle": handle})

        # model_save only writes the file and the reference; the rest of the asset is
        # ours. scaleFactor especially: PickupAmmo was the shared crate mesh at 0.45.
        for member, value in (("scaleFactor", [1, 1, 1, 1]),
                              ("offset", [0, 0, 0, 0]),
                              ("materialShaders", bindings)):
            call("set_instance_member", {"guid": spec["guid"],
                                          "member": member, "value": value})

        mats = ", ".join(m for m, _, _, _ in spec["parts"])
        print(f"Models/{name:14} {positions:5} verts "
              f"{summary.get('counts', {}).get('polygons', '?')} polygons  [{mats}]")


if __name__ == "__main__":
    main()

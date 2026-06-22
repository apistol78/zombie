import os, math, json, struct

SIZE = 256
asset_dir = r"d:/Temp/slask/data/Assets/Textures"
os.makedirs(asset_dir, exist_ok=True)
tga_path = os.path.join(asset_dir, "Wood.tga")

def clamp(v): return 0 if v < 0 else (255 if v > 255 else int(v))

# Deterministic value-noise that wraps over [0,SIZE) for tileability.
def h(ix, iy):
    n = (ix * 374761393 + iy * 668265263) & 0xffffffff
    n = (n ^ (n >> 13)) * 1274126177 & 0xffffffff
    return ((n & 0xffff) / 65535.0)

def vnoise(x, y, period):
    # integer lattice with toroidal wrap at `period`
    x0 = math.floor(x); y0 = math.floor(y)
    fx = x - x0; fy = y - y0
    def hh(a, b): return h(a % period, b % period)
    def sm(t): return t * t * (3 - 2 * t)
    sx, sy = sm(fx), sm(fy)
    a = hh(x0, y0);     b = hh(x0 + 1, y0)
    c = hh(x0, y0 + 1); d = hh(x0 + 1, y0 + 1)
    return (a * (1 - sx) + b * sx) * (1 - sy) + (c * (1 - sx) + d * sx) * sy

# Wood planks running vertically; 4 planks across, grain along Y, seams between planks.
PLANKS = 4
plank_w = SIZE / PLANKS
base = [(150, 101, 58), (138, 92, 50), (162, 112, 66), (128, 84, 46)]

pixels = bytearray()  # BGR, top-left origin
for y in range(SIZE):
    for x in range(SIZE):
        p = int(x // plank_w)
        local = (x - p * plank_w) / plank_w  # 0..1 within plank
        r, g, b = base[p % len(base)]
        # grain: stretched vertical rings, tileable via integer freqs over SIZE
        fx, fy = x / SIZE, y / SIZE
        rings = math.sin(2 * math.pi * (fx * PLANKS * 6 + 0.6 * math.sin(2 * math.pi * fy * 2)))
        rings = 0.5 + 0.5 * rings
        # fine grain noise stretched along Y (wraps over SIZE)
        grain = vnoise(x / 4.0, y / 48.0, SIZE)
        shade = 0.78 + 0.16 * rings + 0.16 * (grain - 0.5)
        # darken plank seams
        seam = min(local, 1 - local)
        if seam < 0.04:
            shade *= 0.55 + 0.45 * (seam / 0.04)
        r, g, b = r * shade, g * shade, b * shade
        pixels += bytes((clamp(b), clamp(g), clamp(r)))

header = bytes([0,0,2,0,0,0,0,0,0,0,0,0]) + struct.pack('<HH', SIZE, SIZE) + bytes([24, 0x20])
with open(tga_path, 'wb') as f:
    f.write(header + pixels)
print("wrote", tga_path, os.path.getsize(tga_path), "bytes")

# ---- Crate box geometry: 1m cube centred at origin, per-face verts, CW outward winding ----
HALF = 0.5
faces = [  # (normal, [4 corners CCW-from-outside]) -> we fix winding below
    ((0, 0, 1),  [(-HALF,-HALF, HALF),( HALF,-HALF, HALF),( HALF, HALF, HALF),(-HALF, HALF, HALF)]),
    ((0, 0,-1),  [( HALF,-HALF,-HALF),(-HALF,-HALF,-HALF),(-HALF, HALF,-HALF),( HALF, HALF,-HALF)]),
    (( 1, 0, 0), [( HALF,-HALF, HALF),( HALF,-HALF,-HALF),( HALF, HALF,-HALF),( HALF, HALF, HALF)]),
    ((-1, 0, 0), [(-HALF,-HALF,-HALF),(-HALF,-HALF, HALF),(-HALF, HALF, HALF),(-HALF, HALF,-HALF)]),
    ((0, 1, 0),  [(-HALF, HALF, HALF),( HALF, HALF, HALF),( HALF, HALF,-HALF),(-HALF, HALF,-HALF)]),
    ((0,-1, 0),  [(-HALF,-HALF,-HALF),( HALF,-HALF,-HALF),( HALF,-HALF, HALF),(-HALF,-HALF, HALF)]),
]
uv = [(0,0),(1,0),(1,1),(0,1)]

def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def sub(a,b): return (a[0]-b[0], a[1]-b[1], a[2]-b[2])
def dot(a,b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

positions, normals, texCoords, polygons = [], [], [], []
for n, corners in faces:
    idx = list(range(len(positions), len(positions) + 4))
    c = list(corners)
    # ensure clockwise as seen from outside: cross(p1-p0, p2-p0) should oppose outward normal
    if dot(cross(sub(c[1], c[0]), sub(c[2], c[0])), n) > 0:
        c = [c[0], c[3], c[2], c[1]]
        uvf = [uv[0], uv[3], uv[2], uv[1]]
    else:
        uvf = uv
    for p, t in zip(c, uvf):
        positions.append([round(p[0],3), round(p[1],3), round(p[2],3)])
        normals.append(list(n)); texCoords.append(list(t))
    polygons.append(idx)

print("GEOMETRY_JSON_START")
print(json.dumps({"positions":positions,"normals":normals,"texCoords":texCoords,"polygons":polygons}))
print("GEOMETRY_JSON_END")

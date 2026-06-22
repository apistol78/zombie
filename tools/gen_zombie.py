import math, json, urllib.request

# Separate .tmd carrying ONLY a "ZombieWalk" take, on the SAME skeleton as the
# humanoid so it retargets onto enemies by joint name. Keeping it out of
# Humanoid.tmd means the player's mesh/clips are never touched.
MESH = "{2B1E0000-0000-694D-9B2B-1B85AD2B0002}"

def call(name, args, timeout=300):
    body = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "tools/call",
                       "params": {"name": name, "arguments": args}}).encode()
    req = urllib.request.Request("http://127.0.0.1:13880/", data=body,
                                 headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.loads(r.read().decode())

# --- Skeleton: 9 joints (identical names/bind to the humanoid) ---
J = [
    ("Hips",   -1, (0.0,  0.95, 0.0)),
    ("Spine",   0, (0.0,  1.30, 0.0)),
    ("Head",    1, (0.0,  1.62, 0.0)),
    ("LArm",    1, (0.22, 1.45, 0.0)),
    ("RArm",    1, (-0.22,1.45, 0.0)),
    ("LUpLeg",  0, (0.10, 0.90, 0.0)),
    ("RUpLeg",  0, (-0.10,0.90, 0.0)),
    ("LLoLeg",  5, (0.10, 0.48, 0.0)),
    ("RLoLeg",  6, (-0.10,0.48, 0.0)),
]
NJ = len(J)

def sub(a, b): return (a[0]-b[0], a[1]-b[1], a[2]-b[2])
def cross(a,b): return (a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0])
def dot(a,b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

localTrans = []
for name, parent, g in J:
    pg = (0.0, 0.0, 0.0) if parent < 0 else J[parent][2]
    localTrans.append(sub(g, pg))

joints = []
for i, (name, parent, g) in enumerate(J):
    lt = localTrans[i]
    joints.append({"name": name, "parent": parent,
        "translation": [round(lt[0],4), round(lt[1],4), round(lt[2],4)],
        "rotation": [0.0, 0.0, 0.0, 1.0], "length": 0.2})

# --- Geometry: one box per body part (only needed so the .tmd has a skinned mesh) ---
BOX = [
    (0, (0.0,  0.95, 0.0), (0.16, 0.10, 0.10)),
    (1, (0.0,  1.25, 0.0), (0.18, 0.22, 0.10)),
    (2, (0.0,  1.70, 0.0), (0.11, 0.11, 0.11)),
    (3, (0.27, 1.20, 0.0), (0.05, 0.25, 0.05)),
    (4, (-0.27,1.20, 0.0), (0.05, 0.25, 0.05)),
    (5, (0.10, 0.70, 0.0), (0.07, 0.21, 0.08)),
    (6, (-0.10,0.70, 0.0), (0.07, 0.21, 0.08)),
    (7, (0.10, 0.25, 0.0), (0.06, 0.22, 0.07)),
    (8, (-0.10,0.25, 0.0), (0.06, 0.22, 0.07)),
]
CORNERS = [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]
FACES = [
    ([0,1,2,3],(0,0,-1)), ([4,5,6,7],(0,0,1)),
    ([0,3,7,4],(-1,0,0)), ([1,5,6,2],(1,0,0)),
    ([0,4,5,1],(0,-1,0)), ([3,2,6,7],(0,1,0)),
]

positions, normals, jointIndices, polygons = [], [], [], []
for jidx, c, h in BOX:
    cp = [[c[0]+cx*h[0], c[1]+cy*h[1], c[2]+cz*h[2]] for (cx,cy,cz) in CORNERS]
    for quad, n in FACES:
        p0, p1, p2 = cp[quad[0]], cp[quad[1]], cp[quad[2]]
        q = [quad[0],quad[3],quad[2],quad[1]] if dot(cross(sub(p1,p0),sub(p2,p0)), n) > 0 else quad
        base = len(positions)
        for k in q:
            positions.append([round(cp[k][0],4), round(cp[k][1],4), round(cp[k][2],4)])
            normals.append([float(n[0]), float(n[1]), float(n[2])])
            jointIndices.append(jidx)
        polygons.append([base, base+1, base+2, base+3])

# --- Zombie walk: stiff arms straight out front, slow shuffling legs, slight
# forward lean and a side-to-side lurch (hip roll). Slow cadence (1.5s loop). ---
def qx(deg):
    a = math.radians(deg) * 0.5
    return [round(math.sin(a),5), 0.0, 0.0, round(math.cos(a),5)]
def qz(deg):
    a = math.radians(deg) * 0.5
    return [0.0, 0.0, round(math.sin(a),5), round(math.cos(a),5)]
IDENT = [0.0,0.0,0.0,1.0]
def pose(rots):
    out = []
    for i in range(NJ):
        lt = localTrans[i]; q = rots.get(i, IDENT)
        out.append([round(lt[0],4), round(lt[1],4), round(lt[2],4), q[0], q[1], q[2], q[3]])
    return out

ARMS = {3: qx(-82), 4: qx(-82)}   # both arms outstretched forward, stiff
LEAN = {1: qx(10), 2: qx(14)}     # forward spine lean + drooped head
def zpose(extra):
    p = {}
    p.update(ARMS); p.update(LEAN); p.update(extra)
    return pose(p)

zombiewalk = {"name": "ZombieWalk", "keyframes": [
    {"time": 0.00, "pose": zpose({0: qz(5),  5: qx(14), 6: qx(-14), 7: qx(12)})},
    {"time": 0.75, "pose": zpose({0: qz(-5), 5: qx(-14), 6: qx(14), 8: qx(12)})},
    {"time": 1.50, "pose": zpose({0: qz(5),  5: qx(14), 6: qx(-14), 7: qx(12)})}]}
animations = [zombiewalk]

print("zombie anims:", len(positions), "verts", len(polygons), "quads")
res = call("create_mesh_from_geometry", {
    "path": "Models/EnemyAnims", "fileName": "Models/EnemyAnims.tmd", "guid": MESH,
    "meshType": "skinned", "material": "HumanoidMat",
    "positions": positions, "normals": normals, "polygons": polygons,
    "jointIndices": jointIndices, "joints": joints, "animations": animations,
})
print("create_mesh:", json.dumps(res)[:240])

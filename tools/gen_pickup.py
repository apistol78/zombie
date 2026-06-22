import json, urllib.request

MESH = "{B1CCEE00-0000-4000-8000-000000000001}"

def call(name, args, timeout=120):
    body = json.dumps({"jsonrpc":"2.0","id":1,"method":"tools/call",
                       "params":{"name":name,"arguments":args}}).encode()
    req = urllib.request.Request("http://127.0.0.1:13880/", data=body,
                                 headers={"Content-Type":"application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        return json.loads(r.read().decode())

# small gem-ish box, hard normals
HX, HY, HZ = 0.18, 0.28, 0.18
CORNERS = [(-1,-1,-1),(1,-1,-1),(1,1,-1),(-1,1,-1),(-1,-1,1),(1,-1,1),(1,1,1),(-1,1,1)]
FACES = [([0,1,2,3],(0,0,-1)),([4,5,6,7],(0,0,1)),([0,3,7,4],(-1,0,0)),
         ([1,5,6,2],(1,0,0)),([0,4,5,1],(0,-1,0)),([3,2,6,7],(0,1,0))]
def sub(a,b): return (a[0]-b[0],a[1]-b[1],a[2]-b[2])
def cross(a,b): return (a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0])
def dot(a,b): return a[0]*b[0]+a[1]*b[1]+a[2]*b[2]

cp = [[cx*HX, cy*HY, cz*HZ] for (cx,cy,cz) in CORNERS]
positions, normals, polygons = [], [], []
for quad, n in FACES:
    p0,p1,p2 = cp[quad[0]], cp[quad[1]], cp[quad[2]]
    q = [quad[0],quad[3],quad[2],quad[1]] if dot(cross(sub(p1,p0),sub(p2,p0)), n) > 0 else quad
    base = len(positions)
    for k in q:
        positions.append([round(v,4) for v in cp[k]]); normals.append([float(n[0]),float(n[1]),float(n[2])])
    polygons.append([base,base+1,base+2,base+3])

res = call("create_mesh_from_geometry", {
    "path":"Models/Pickup", "fileName":"Models/Pickup.tmd", "guid":MESH,
    "material":"PickupMat", "positions":positions, "normals":normals, "polygons":polygons})
print(json.dumps(res)[:240])

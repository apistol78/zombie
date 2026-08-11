#!/usr/bin/env python3
"""Merge "A - Zombie - Idle" (legs) with "A - Zombie - Walking" (torso + arms).

Legs/hips keep the idle motion verbatim; every joint in the Spine sub-tree (spine,
neck, head, shoulders, arms, hands, fingers) comes from the walk animation,
time-warped so one walk cycle spans the whole idle loop (keeps the loop seamless).
Result is written as an animation-only model, Models/Zombie/Zombie Idle Arms.tmd.
"""
import json, math, sys, urllib.request

URL = "http://127.0.0.1:13880/"
IDLE = "Models/Zombie/Zombie Idle.blend"
WALK = "Models/Zombie/Zombie Walking.blend"

# "local":  graft the walk's local Spine transform as-is (torso stays as upright as the idle hips).
# "world":  rotate the grafted Spine so the torso reproduces the walk's world-space orientation,
#           i.e. the forward lean the walk stores in the Hips is carried over too.
# "spread": same world-space result, but the make-up rotation is split evenly over Spine/Spine1/
#           Spine2 so the waist curves instead of folding at a single joint.
MODE = sys.argv[1] if len(sys.argv) > 1 else "world"
OUT = sys.argv[2] if len(sys.argv) > 2 else "Models/Zombie/Zombie Idle Arms.tmd"


def call(name, args, timeout=300):
    body = json.dumps({"jsonrpc": "2.0", "id": 1, "method": "tools/call",
                       "params": {"name": name, "arguments": args}}).encode()
    req = urllib.request.Request(URL, data=body, headers={"Content-Type": "application/json"})
    with urllib.request.urlopen(req, timeout=timeout) as r:
        res = json.loads(r.read())
    if "error" in res:
        raise RuntimeError(f"{name}: {res['error']}")
    out = res["result"]
    if out.get("isError"):
        raise RuntimeError(f"{name}: {out['content'][0]['text']}")
    return out.get("structuredContent") or json.loads(out["content"][0]["text"])


def qdot(a, b):
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3]


def slerp(a, b, s):
    d = qdot(a, b)
    if d < 0.0:
        b, d = [-v for v in b], -d
    if d > 0.9995:
        o = [a[i] + (b[i] - a[i]) * s for i in range(4)]
        l = math.sqrt(sum(v * v for v in o))
        return [v / l for v in o]
    th0 = math.acos(d)
    th = th0 * s
    st0 = math.sin(th0)
    s0, s1 = math.sin(th0 - th) / st0, math.sin(th) / st0
    return [a[i] * s0 + b[i] * s1 for i in range(4)]


def vlerp(a, b, s):
    return [a[i] + (b[i] - a[i]) * s for i in range(3)]


def qmul(a, b):
    ax, ay, az, aw = a
    bx, by, bz, bw = b
    return [aw * bx + ax * bw + ay * bz - az * by,
            aw * by - ax * bz + ay * bw + az * bx,
            aw * bz + ax * by - ay * bx + az * bw,
            aw * bw - ax * bx - ay * by - az * bz]


def qconj(a):
    return [-a[0], -a[1], -a[2], a[3]]


def qpow(q, s):
    """Fractional rotation: same axis, angle scaled by s."""
    if q[3] < 0.0:
        q = [-v for v in q]
    half = math.acos(min(1.0, q[3]))
    sn = math.sin(half)
    if sn < 1e-8:
        return [0.0, 0.0, 0.0, 1.0]
    axis = [q[i] / sn for i in range(3)]
    h = half * s
    return [axis[0] * math.sin(h), axis[1] * math.sin(h), axis[2] * math.sin(h), math.cos(h)]


def global_rot(joints, pose, i):
    """Accumulated world rotation of joint i under a pose."""
    q = [0.0, 0.0, 0.0, 1.0]
    chain = []
    while i >= 0:
        chain.append(i)
        i = joints[i]["parent"]
    for j in reversed(chain):
        q = qmul(q, pose[j]["rotation"])
    return q


def qangle(a, b):
    return math.degrees(2.0 * math.acos(min(1.0, abs(qdot(a, b)))))


def rnd(v):
    return round(v, 6)


def main():
    h_idle = call("model_open", {"file": IDLE})["handle"]
    h_walk = call("model_open", {"file": WALK})["handle"]
    idle = call("model_inspect", {"handle": h_idle})
    walk = call("model_inspect", {"handle": h_walk})
    ia, wa = idle["animations"][0], walk["animations"][0]
    print(f"idle handle {h_idle}: {idle['jointCount']} joints, {ia['keyFrameCount']} frames, {ia['duration']}s")
    print(f"walk handle {h_walk}: {walk['jointCount']} joints, {wa['keyFrameCount']} frames, {wa['duration']}s")

    # joint sets: everything below (and including) "Spine" comes from the walk.
    jn = [j["name"] for j in idle["joints"]]
    if jn != [j["name"] for j in walk["joints"]]:
        raise RuntimeError("skeletons differ between idle and walk")
    from_walk = {jn.index("Spine")}
    for i, j in enumerate(idle["joints"]):          # parents always precede children
        if j["parent"] in from_walk:
            from_walk.add(i)
    print(f"from walk ({len(from_walk)}): {', '.join(jn[i] for i in sorted(from_walk)[:12])} ...")
    print(f"from idle ({len(jn) - len(from_walk)}): {', '.join(n for i, n in enumerate(jn) if i not in from_walk)}")

    def read_poses(handle, info):
        out = []
        for k in range(info["animations"][0]["keyFrameCount"]):
            p = call("model_get_elements", {"handle": handle, "kind": "pose", "animation": 0,
                                            "keyFrame": k, "count": 4096})
            if p["total"] != info["jointCount"]:
                print(f"  ! frame {k} has {p['total']}/{info['jointCount']} joint transforms")
            out.append(p["elements"])
        return out

    idle_poses = read_poses(h_idle, idle)
    walk_poses = read_poses(h_walk, walk)
    print(f"read {len(idle_poses)} idle + {len(walk_poses)} walk poses")

    wt, wn = wa["keyFrameTimes"], wa["keyFrameCount"]
    it, inn = ia["keyFrameTimes"], ia["keyFrameCount"]
    walk_dup = max(qangle(e["rotation"], walk_poses[wn - 1][i]["rotation"])
                   for i, e in enumerate(walk_poses[0])) < 1.0
    idle_dup = max(qangle(e["rotation"], idle_poses[inn - 1][i]["rotation"])
                   for i, e in enumerate(idle_poses[0])) < 1.0
    pw = wt[wn - 1] if walk_dup else wt[wn - 1] + (wt[1] - wt[0])
    pi = it[inn - 1] if idle_dup else it[inn - 1] + (it[1] - it[0])
    print(f"walk loop: first/last frame {'match' if walk_dup else 'differ'} -> period {pw:.4f}s")
    print(f"idle loop: first/last frame {'match' if idle_dup else 'differ'} -> period {pi:.4f}s")

    def sample_walk(u):
        u = u % pw
        i = 0
        while i + 1 < wn and wt[i + 1] <= u:
            i += 1
        t0 = wt[i]
        t1 = wt[i + 1] if i + 1 < wn else pw
        j = i + 1 if i + 1 < wn else 0
        s = (u - t0) / (t1 - t0) if t1 > t0 else 0.0
        return walk_poses[i], walk_poses[j], s

    for name in ("Spine", "Spine1", "Neck", "LeftShoulder", "LeftArm", "LeftForeArm", "RightArm", "RightForeArm"):
        i = jn.index(name)
        swing = max(qangle(walk_poses[0][i]["rotation"], walk_poses[k][i]["rotation"]) for k in range(1, wn))
        vs_idle = qangle(walk_poses[0][i]["rotation"], idle_poses[0][i]["rotation"])
        print(f"  {name:<14} walk swing {swing:6.1f} deg over the cycle, walk-vs-idle at t=0 {vs_idle:6.1f} deg")

    hips, spine = jn.index("Hips"), jn.index("Spine")
    chain = [jn.index(n) for n in ("Spine", "Spine1", "Spine2")]
    print(f"mode {MODE} -> {OUT}")
    print(f"hips world rotation, idle vs walk at t=0: "
          f"{qangle(global_rot(idle['joints'], idle_poses[0], hips), global_rot(walk['joints'], walk_poses[0], hips)):.1f} deg")

    edits = []
    for k in range(inn):
        a, b, s = sample_walk((it[k] / pi) * pw)
        wp = [{"translation": vlerp(a[i]["translation"], b[i]["translation"], s),
               "rotation": slerp(a[i]["rotation"], b[i]["rotation"], s)} for i in range(len(jn))]
        pose = [wp[i] if i in from_walk else idle_poses[k][i] for i in range(len(jn))]
        if MODE in ("world", "spread"):
            # Re-root the grafted chain: the walk keeps its forward lean in the Hips, which we
            # take from the idle, so make it up inside the spine. D is that make-up rotation,
            # expressed in the idle hips' frame.
            d = qmul(qconj(global_rot(idle["joints"], pose, hips)), global_rot(walk["joints"], wp, hips))
            if MODE == "world":
                pose[spine] = dict(pose[spine], rotation=qmul(d, wp[spine]["rotation"]))
            else:
                # Split D over the three spine joints; each share is conjugated into the frame of
                # the (unmodified) walk chain above it, so the accumulated result is unchanged.
                part = qpow(d, 1.0 / len(chain))
                acc = [0.0, 0.0, 0.0, 1.0]
                for j in chain:
                    pose[j] = dict(pose[j], rotation=qmul(qmul(qmul(qconj(acc), part), acc), wp[j]["rotation"]))
                    acc = qmul(acc, wp[j]["rotation"])
        jts = [{"translation": [rnd(v) for v in e["translation"]],
                "rotation": [rnd(v) for v in e["rotation"]]} for e in pose]
        edits.append({"op": "setKeyFramePose", "animation": 0, "keyFrame": k, "pose": {"jointTransforms": jts}})
    for i in range(0, len(edits), 16):
        call("model_edit", {"handle": h_idle, "edits": edits[i:i + 16]})
    print(f"wrote {len(edits)} merged key frames")

    # strip geometry; the animation pipeline only needs the joints and the take.
    call("model_apply_operation", {"handle": h_idle, "operations": [
        {"name": "Clear", "flags": ["Vertices", "Polygons", "Positions", "Colors", "Normals", "TexCoords"]}]})
    print("saved " + call("model_save", {"handle": h_idle, "file": OUT})["file"])

    h_chk = call("model_open", {"file": OUT})["handle"]
    chk = call("model_inspect", {"handle": h_chk})
    print(f"round-trip: {chk['jointCount']} joints, takes "
          f"{[(a['name'], a['keyFrameCount'], a['duration']) for a in chk['animations']]}")
    p0 = call("model_get_elements", {"handle": h_chk, "kind": "pose", "animation": 0, "keyFrame": 0, "count": 4096})
    for name in ("LeftArm", "LeftUpLeg"):
        i = jn.index(name)
        r = p0["elements"][i]["rotation"]
        print(f"  {name:<10} vs walk {qangle(r, walk_poses[0][i]['rotation']):6.2f} deg / "
              f"vs idle {qangle(r, idle_poses[0][i]['rotation']):6.2f} deg")

    for h in (h_chk, h_idle, h_walk):
        call("model_close", {"handle": h})
    print("done")


main()

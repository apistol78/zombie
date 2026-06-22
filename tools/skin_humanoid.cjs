// Rebuild the humanoid as a SKINNED, animated mesh + skeleton/animation assets + entity wiring.
// Run after rebuilding the editor (create_mesh_from_geometry must accept meshType/joints/jointIndices/animations).
const http = require("node:http"), crypto = require("node:crypto");
function rpc(m, p, t) {
  return new Promise((res, rej) => {
    const b = JSON.stringify({ jsonrpc: "2.0", id: 1, method: m, params: p });
    const r = http.request({ host: "127.0.0.1", port: 13880, path: "/", method: "POST",
      headers: { "Content-Type": "application/json", "Content-Length": Buffer.byteLength(b) } },
      x => { let d = ""; x.on("data", c => d += c); x.on("end", () => { try { res(JSON.parse(d)); } catch (e) { rej(new Error("bad json: " + d.slice(0,160))); } }); });
    r.on("error", rej); if (t) r.setTimeout(t, () => r.destroy(new Error("timeout"))); r.write(b); r.end();
  });
}
const call = async (n, a, t) => { const r = await rpc("tools/call", { name: n, arguments: a }, t); if (r.error) return { err: r.error }; const x = r.result; return x.isError ? { toolError: x.content[0].text } : (x.structuredContent ?? JSON.parse(x.content[0].text)); };
const newGuid = () => "{" + crypto.randomUUID().toUpperCase() + "}";
const rnd = v => Number(v.toFixed(5));
const sub = (a, b) => [a[0]-b[0], a[1]-b[1], a[2]-b[2]];
const cross = (a, b) => [a[1]*b[2]-a[2]*b[1], a[2]*b[0]-a[0]*b[2], a[0]*b[1]-a[1]*b[0]];
const dot = (a, b) => a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
const quatX = a => [Math.sin(a/2), 0, 0, Math.cos(a/2)];

const MAT_SHADER = "{D14A9C31-34CC-4F08-A361-D5C894CA4CBF}";   // dark-orange surface shader
const ENTITY = "{BCF74021-58D3-B24A-8E73-BEC797740D24}";       // Entities/Humanoid/Humanoid

// ---- skeleton (global bind positions) ----
const J = [
  { name: "Root",      g: [0, 0.99, 0],     parent: -1 }, // 0
  { name: "Torso",     g: [0, 1.10, 0],     parent: 0 },  // 1
  { name: "Head",      g: [0, 1.50, 0],     parent: 1 },  // 2
  { name: "LUpperArm", g: [0.24, 1.45, 0],  parent: 1 },  // 3
  { name: "LForearm",  g: [0.265, 1.10, 0], parent: 3 },  // 4
  { name: "LHand",     g: [0.265, 0.76, 0], parent: 4 },  // 5
  { name: "RUpperArm", g: [-0.24, 1.45, 0], parent: 1 },  // 6
  { name: "RForearm",  g: [-0.265, 1.10, 0],parent: 6 },  // 7
  { name: "RHand",     g: [-0.265, 0.76, 0],parent: 7 },  // 8
  { name: "LThigh",    g: [0.11, 0.92, 0],  parent: 0 },  // 9
  { name: "LShin",     g: [0.10, 0.52, 0],  parent: 9 },  // 10
  { name: "LFoot",     g: [0.10, 0.08, 0],  parent: 10 }, // 11
  { name: "RThigh",    g: [-0.11, 0.92, 0], parent: 0 },  // 12
  { name: "RShin",     g: [-0.10, 0.52, 0], parent: 12 }, // 13
  { name: "RFoot",     g: [-0.10, 0.08, 0], parent: 13 }, // 14
];
const localT = J.map(j => j.parent < 0 ? j.g.slice() : sub(j.g, J[j.parent].g));
const joints = J.map((j, i) => ({ name: j.name, parent: j.parent, translation: localT[i].map(rnd), rotation: [0,0,0,1], length: 0.1 }));

// ---- geometry, tagging each position with its joint ----
const positions = [], normals = [], polygons = [], jointIndices = [];
let curJoint = 0;
function quad(ps, ns, outN) { const rn = cross(sub(ps[1],ps[0]), sub(ps[2],ps[0])); let ord = [0,1,2,3]; if (dot(rn,outN) > 0) ord = [0,3,2,1]; const base = positions.length; for (const k of ord) { positions.push(ps[k].map(rnd)); normals.push(ns[k].map(rnd)); jointIndices.push(curJoint); } polygons.push([base, base+1, base+2, base+3]); }
function face(a, b, c, d, n) { quad([a,b,c,d], [n,n,n,n], n); }
function box(cx, cy, cz, hx, hy, hz) { const x0=cx-hx,x1=cx+hx,y0=cy-hy,y1=cy+hy,z0=cz-hz,z1=cz+hz; const c=[[x0,y0,z0],[x1,y0,z0],[x1,y1,z0],[x0,y1,z0],[x0,y0,z1],[x1,y0,z1],[x1,y1,z1],[x0,y1,z1]]; face(c[4],c[5],c[6],c[7],[0,0,1]); face(c[1],c[0],c[3],c[2],[0,0,-1]); face(c[1],c[5],c[6],c[2],[1,0,0]); face(c[0],c[4],c[7],c[3],[-1,0,0]); face(c[3],c[2],c[6],c[7],[0,1,0]); face(c[4],c[5],c[1],c[0],[0,-1,0]); }
function sphere(cx, cy, cz, r, seg, rings) { const g=[]; for (let i=0;i<=rings;i++){const phi=Math.PI*i/rings,sp=Math.sin(phi),cp=Math.cos(phi),row=[]; for(let j=0;j<=seg;j++){const th=2*Math.PI*j/seg,nx=sp*Math.cos(th),ny=cp,nz=sp*Math.sin(th);row.push({p:[cx+r*nx,cy+r*ny,cz+r*nz],n:[nx,ny,nz]});}g.push(row);} for(let i=0;i<rings;i++)for(let j=0;j<seg;j++){const a=g[i][j],b=g[i+1][j],c2=g[i+1][j+1],d=g[i][j+1];const cn=[(a.n[0]+b.n[0]+c2.n[0]+d.n[0])/4,(a.n[1]+b.n[1]+c2.n[1]+d.n[1])/4,(a.n[2]+b.n[2]+c2.n[2]+d.n[2])/4];quad([a.p,b.p,c2.p,d.p],[a.n,b.n,c2.n,d.n],cn);} }

curJoint = 0; box(0, 0.99, 0, 0.17, 0.09, 0.11);   // pelvis -> Root
curJoint = 1; box(0, 1.30, 0, 0.21, 0.20, 0.115);  // torso -> Torso
curJoint = 2; box(0, 1.55, 0, 0.055, 0.06, 0.055); // neck -> Head
curJoint = 2; sphere(0, 1.72, 0, 0.135, 18, 12);   // head -> Head
for (const s of [-1, 1]) {
  curJoint = s > 0 ? 3 : 6;  box(s*0.265, 1.30, 0, 0.06, 0.18, 0.07);
  curJoint = s > 0 ? 4 : 7;  box(s*0.265, 0.93, 0, 0.052, 0.17, 0.06);
  curJoint = s > 0 ? 5 : 8;  box(s*0.265, 0.68, 0, 0.055, 0.08, 0.035);
  curJoint = s > 0 ? 9 : 12; box(s*0.11, 0.73, 0, 0.085, 0.21, 0.095);
  curJoint = s > 0 ? 10 : 13;box(s*0.10, 0.30, 0, 0.065, 0.22, 0.075);
  curJoint = s > 0 ? 11 : 14;box(s*0.10, 0.04, 0.07, 0.07, 0.04, 0.13);
}

// ---- walk animation (full poses: bind local translation + per-joint rotation) ----
function pose(rotMap) { return J.map((j, i) => { const t = localT[i], r = rotMap[i] || [0,0,0,1]; return [rnd(t[0]),rnd(t[1]),rnd(t[2]),rnd(r[0]),rnd(r[1]),rnd(r[2]),rnd(r[3])]; }); }
const A = pose({ 3: quatX(0.45), 6: quatX(-0.45), 9: quatX(-0.5), 12: quatX(0.5), 10: quatX(0.6) });
const B = pose({ 3: quatX(-0.45), 6: quatX(0.45), 9: quatX(0.5), 12: quatX(-0.5), 13: quatX(0.6) });
const animations = [{ name: "Walk", keyframes: [{ time: 0.0, pose: A }, { time: 0.5, pose: B }, { time: 1.0, pose: A }] }];

(async () => {
  const tool = (await rpc("tools/list")).result.tools.find(t => t.name === "create_mesh_from_geometry");
  if (!tool.inputSchema.properties.meshType) { console.log("Editor not rebuilt (no meshType arg) - aborting."); return; }
  console.log("skinned tool live. verts=" + positions.length + " joints=" + joints.length);

  const mesh = await call("create_mesh_from_geometry", {
    path: "Models/Humanoid", fileName: "Models/Humanoid.tmd",
    positions, polygons, normals, jointIndices, joints, animations,
    meshType: "skinned", material: "HumanoidMat", triangulate: true
  }, 120000);
  console.log("create_mesh:", JSON.stringify(mesh).slice(0, 360));
  if (mesh.toolError || mesh.err) return;

  console.log("rebind shader:", JSON.stringify(await call("set_mesh_material_shader", { guid: mesh.guid, material: "HumanoidMat", shader: MAT_SHADER })).slice(0,160));
  console.log("build mesh:", JSON.stringify(await call("build_asset", { guid: mesh.guid, rebuild: true, wait: true }, 180000)));

  // skeleton asset
  const skel = await call("create_instance", { path: "Entities/Humanoid/S - Humanoid", type: "traktor.animation.SkeletonAsset" });
  await call("set_instance_member", { guid: skel.guid, member: "fileName", value: "Models/Humanoid.tmd" });
  await call("set_instance_member", { guid: skel.guid, member: "scale", value: [1,1,1,1] });
  await call("set_instance_member", { guid: skel.guid, member: "offset", value: [0,0,0,0] });
  await call("set_instance_member", { guid: skel.guid, member: "radius", value: 0.08 });
  console.log("skeleton:", skel.guid, "build:", JSON.stringify(await call("build_asset", { guid: skel.guid, rebuild: true, wait: true }, 120000)).slice(0,120));

  // animation asset
  const anim = await call("create_instance", { path: "Entities/Humanoid/A - Walk", type: "traktor.animation.AnimationAsset" });
  await call("set_instance_member", { guid: anim.guid, member: "fileName", value: "Models/Humanoid.tmd" });
  await call("set_instance_member", { guid: anim.guid, member: "removeLocomotion", value: false });
  console.log("animation:", anim.guid, "build:", JSON.stringify(await call("build_asset", { guid: anim.guid, rebuild: true, wait: true }, 120000)).slice(0,120));

  // rewire entity: skeleton (+ simple controller) + animated mesh
  const rew = await call("set_instance_member", { guid: ENTITY, member: "components", value: [
    { "$type": "traktor.animation.SkeletonComponentData", set: {
        skeleton: skel.guid,
        poseController: { "$type": "traktor.animation.SimpleAnimationControllerData", set: { animation: anim.guid } }
    } },
    { "$type": "traktor.animation.AnimatedMeshComponentData", set: { mesh: mesh.guid } }
  ] });
  console.log("rewire:", JSON.stringify(rew).slice(0, 200));

  // verify
  const e = await call("inspect_instance", { guid: ENTITY, maxDepth: 5 });
  const comps = e.members.find(m => m.name === "components");
  for (const el of comps.elements || []) {
    let line = "  " + el.type;
    const pc = (el.members || []).find(m => m.name === "poseController");
    if (pc) line += " poseController=" + (pc.type || pc.kind) + (pc.members ? " anim=" + JSON.stringify((pc.members.find(x=>x.name==="animation")||{}).value) : "");
    console.log(line);
  }
})();

// Regenerate the humanoid as a rounded "cartoon" figure (capsule limbs, ellipsoid body/head),
// keeping the same 15-joint skeleton, rigid weights and walk animation. Re-bind the yellow shader.
const http = require("node:http");
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
const rnd = v => Number(v.toFixed(4));
const sub = (a,b)=>[a[0]-b[0],a[1]-b[1],a[2]-b[2]], cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]], dot=(a,b)=>a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
const nrm = v => { const l = Math.hypot(v[0],v[1],v[2]) || 1; return [v[0]/l, v[1]/l, v[2]/l]; };

const MESH = "{8E459F05-82A8-694D-9B2B-1B85AD2B9459}";
const MAT = "{D14A9C31-34CC-4F08-A361-D5C894CA4CBF}";

// ---- skeleton (same as before) ----
const J = [
  { name:"Root", g:[0,0.99,0], parent:-1 },{ name:"Torso", g:[0,1.10,0], parent:0 },{ name:"Head", g:[0,1.50,0], parent:1 },
  { name:"LUpperArm", g:[0.24,1.45,0], parent:1 },{ name:"LForearm", g:[0.265,1.10,0], parent:3 },{ name:"LHand", g:[0.265,0.76,0], parent:4 },
  { name:"RUpperArm", g:[-0.24,1.45,0], parent:1 },{ name:"RForearm", g:[-0.265,1.10,0], parent:6 },{ name:"RHand", g:[-0.265,0.76,0], parent:7 },
  { name:"LThigh", g:[0.11,0.92,0], parent:0 },{ name:"LShin", g:[0.10,0.52,0], parent:9 },{ name:"LFoot", g:[0.10,0.08,0], parent:10 },
  { name:"RThigh", g:[-0.11,0.92,0], parent:0 },{ name:"RShin", g:[-0.10,0.52,0], parent:12 },{ name:"RFoot", g:[-0.10,0.08,0], parent:13 },
];
const localT = J.map(j => j.parent < 0 ? j.g.slice() : sub(j.g, J[j.parent].g));
const joints = J.map((j,i)=>({ name:j.name, parent:j.parent, translation: localT[i].map(rnd), rotation:[0,0,0,1], length:0.1 }));

// ---- walk animation (same) ----
const quatX = a => [Math.sin(a/2),0,0,Math.cos(a/2)];
const pose = m => J.map((j,i)=>{const t=localT[i],r=m[i]||[0,0,0,1];return [rnd(t[0]),rnd(t[1]),rnd(t[2]),rnd(r[0]),rnd(r[1]),rnd(r[2]),rnd(r[3])];});
const A = pose({3:quatX(0.45),6:quatX(-0.45),9:quatX(-0.5),12:quatX(0.5),10:quatX(0.6)});
const B = pose({3:quatX(-0.45),6:quatX(0.45),9:quatX(0.5),12:quatX(-0.5),13:quatX(0.6)});
const animations = [{ name:"Walk", keyframes:[{time:0.0,pose:A},{time:0.5,pose:B},{time:1.0,pose:A}] }];

// ---- rounded geometry ----
const positions=[], normals=[], polygons=[], jointIndices=[];
let curJoint = 0;
function quad(ps, ns, outN){ const rn=cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])); let ord=[0,1,2,3]; if(dot(rn,outN)>0) ord=[0,3,2,1]; const base=positions.length; for(const k of ord){ positions.push(ps[k].map(rnd)); normals.push(ns[k].map(rnd)); jointIndices.push(curJoint); } polygons.push([base,base+1,base+2,base+3]); }
function emitGrid(g){ const rings=g.length-1, seg=g[0].length-1; for(let i=0;i<rings;i++)for(let j=0;j<seg;j++){ const a=g[i][j],b=g[i+1][j],c=g[i+1][j+1],d=g[i][j+1]; const cn=nrm([a.n[0]+b.n[0]+c.n[0]+d.n[0],a.n[1]+b.n[1]+c.n[1]+d.n[1],a.n[2]+b.n[2]+c.n[2]+d.n[2]]); quad([a.p,b.p,c.p,d.p],[a.n,b.n,c.n,d.n],cn); } }
function ellipsoid(cx,cy,cz,rx,ry,rz,seg,rings){ const g=[]; for(let i=0;i<=rings;i++){ const phi=Math.PI*i/rings,sp=Math.sin(phi),cp=Math.cos(phi),row=[]; for(let j=0;j<=seg;j++){ const th=2*Math.PI*j/seg,ct=Math.cos(th),st=Math.sin(th); row.push({ p:[cx+rx*sp*ct, cy+ry*cp, cz+rz*sp*st], n:nrm([sp*ct/rx, cp/ry, sp*st/rz]) }); } g.push(row);} emitGrid(g); }
function capsuleY(cx,cy,cz,r,cylHalf,seg,rings){ const g=[]; for(let i=0;i<=rings;i++){ const phi=Math.PI*i/rings,sp=Math.sin(phi),cp=Math.cos(phi),yo=(phi<=Math.PI/2)?cylHalf:-cylHalf,row=[]; for(let j=0;j<=seg;j++){ const th=2*Math.PI*j/seg,ct=Math.cos(th),st=Math.sin(th); row.push({ p:[cx+r*sp*ct, cy+yo+r*cp, cz+r*sp*st], n:[sp*ct,cp,sp*st] }); } g.push(row);} emitGrid(g); }

curJoint=0; ellipsoid(0, 0.99, 0, 0.18, 0.13, 0.14, 10, 8);   // pelvis -> Root
curJoint=1; ellipsoid(0, 1.28, 0, 0.21, 0.23, 0.15, 12, 8);   // torso -> Torso
curJoint=2; ellipsoid(0, 1.70, 0, 0.18, 0.19, 0.18, 14, 10);  // head -> Head
for (const s of [-1,1]) {
  curJoint = s>0?3:6;   capsuleY(s*0.26, 1.28, 0, 0.06, 0.11, 10, 10);            // upper arm
  curJoint = s>0?4:7;   capsuleY(s*0.265, 0.93, 0, 0.052, 0.11, 10, 10);          // forearm
  curJoint = s>0?5:8;   ellipsoid(s*0.265, 0.69, 0, 0.07, 0.07, 0.06, 10, 8);     // hand
  curJoint = s>0?9:12;  capsuleY(s*0.11, 0.73, 0, 0.09, 0.12, 10, 10);            // thigh
  curJoint = s>0?10:13; capsuleY(s*0.10, 0.31, 0, 0.07, 0.12, 10, 10);            // shin
  curJoint = s>0?11:14; ellipsoid(s*0.10, 0.05, 0.07, 0.07, 0.05, 0.13, 10, 8);   // foot
}

(async () => {
  console.log("cartoon humanoid: " + positions.length + " verts, " + polygons.length + " quads");
  const mesh = await call("create_mesh_from_geometry", {
    path:"Models/Humanoid", fileName:"Models/Humanoid.tmd",
    positions, polygons, normals, jointIndices, joints, animations,
    meshType:"skinned", material:"HumanoidMat", triangulate:true
  }, 180000);
  console.log("create_mesh:", JSON.stringify(mesh).slice(0,300));
  if (mesh.toolError || mesh.err) return;
  console.log("rebind shader:", JSON.stringify(await call("set_mesh_material_shader", { guid: mesh.guid, material:"HumanoidMat", shader: MAT })).slice(0,120));
  console.log("build mesh:", JSON.stringify(await call("build_asset", { guid: mesh.guid, rebuild:true, wait:true }, 240000)));
})();

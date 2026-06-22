// Add an Idle animation, build an Idle<->Walk state graph, and switch the humanoid to an
// AnimationGraph pose controller driven by a "moving" condition (script sets it).
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
const call = async (n, a, t) => { const r = await rpc("tools/call", { name: n, arguments: a }, t); if (r.error) return { err: JSON.stringify(r.error) }; const x = r.result; return x.isError ? { toolError: x.content[0].text } : (x.structuredContent ?? JSON.parse(x.content[0].text)); };
const rnd = v => Number(v.toFixed(4));
const sub=(a,b)=>[a[0]-b[0],a[1]-b[1],a[2]-b[2]], cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]], dot=(a,b)=>a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
const nrm = v => { const l = Math.hypot(v[0],v[1],v[2]) || 1; return [v[0]/l, v[1]/l, v[2]/l]; };
const MESH="{8E459F05-82A8-694D-9B2B-1B85AD2B9459}", MAT="{D14A9C31-34CC-4F08-A361-D5C894CA4CBF}";
const AWALK="{7D4EE619-0FF7-6243-A37C-03F86B3192F8}", SKEL="{1E9C2788-505E-6C43-BB3D-527228721118}";
const ENTITY="{BCF74021-58D3-B24A-8E73-BEC797740D24}", SCENE="{FF0A61F5-30AD-4647-A89F-37700B557CD4}";

// ---- skeleton + animations ----
const J=[{g:[0,0.99,0],p:-1},{g:[0,1.10,0],p:0},{g:[0,1.50,0],p:1},{g:[0.24,1.45,0],p:1},{g:[0.265,1.10,0],p:3},{g:[0.265,0.76,0],p:4},{g:[-0.24,1.45,0],p:1},{g:[-0.265,1.10,0],p:6},{g:[-0.265,0.76,0],p:7},{g:[0.11,0.92,0],p:0},{g:[0.10,0.52,0],p:9},{g:[0.10,0.08,0],p:10},{g:[-0.11,0.92,0],p:0},{g:[-0.10,0.52,0],p:12},{g:[-0.10,0.08,0],p:13}];
const names=["Root","Torso","Head","LUpperArm","LForearm","LHand","RUpperArm","RForearm","RHand","LThigh","LShin","LFoot","RThigh","RShin","RFoot"];
const localT=J.map(j=>j.p<0?j.g.slice():sub(j.g,J[j.p].g));
const joints=J.map((j,i)=>({name:names[i],parent:j.p,translation:localT[i].map(rnd),rotation:[0,0,0,1],length:0.1}));
const quatX=a=>[Math.sin(a/2),0,0,Math.cos(a/2)];
const pose=m=>J.map((j,i)=>{const t=localT[i],r=m[i]||[0,0,0,1];return [rnd(t[0]),rnd(t[1]),rnd(t[2]),rnd(r[0]),rnd(r[1]),rnd(r[2]),rnd(r[3])];});
const WA=pose({3:quatX(0.45),6:quatX(-0.45),9:quatX(-0.5),12:quatX(0.5),10:quatX(0.6)});
const WB=pose({3:quatX(-0.45),6:quatX(0.45),9:quatX(0.5),12:quatX(-0.5),13:quatX(0.6)});
const I0=pose({}), I1=pose({3:quatX(0.05),6:quatX(0.05),2:quatX(0.03)});
const animations=[
  {name:"Walk", keyframes:[{time:0.0,pose:WA},{time:0.5,pose:WB},{time:1.0,pose:WA}]},
  {name:"Idle", keyframes:[{time:0.0,pose:I0},{time:1.0,pose:I1},{time:2.0,pose:I0}]}
];

// ---- cartoon geometry (capsule limbs + ellipsoid body) ----
const positions=[],normals=[],polygons=[],jointIndices=[]; let curJoint=0;
function quad(ps,ns,outN){const rn=cross(sub(ps[1],ps[0]),sub(ps[2],ps[0]));let ord=[0,1,2,3];if(dot(rn,outN)>0)ord=[0,3,2,1];const base=positions.length;for(const k of ord){positions.push(ps[k].map(rnd));normals.push(ns[k].map(rnd));jointIndices.push(curJoint);}polygons.push([base,base+1,base+2,base+3]);}
function grid(g){const rings=g.length-1,seg=g[0].length-1;for(let i=0;i<rings;i++)for(let j=0;j<seg;j++){const a=g[i][j],b=g[i+1][j],c=g[i+1][j+1],d=g[i][j+1];const cn=nrm([a.n[0]+b.n[0]+c.n[0]+d.n[0],a.n[1]+b.n[1]+c.n[1]+d.n[1],a.n[2]+b.n[2]+c.n[2]+d.n[2]]);quad([a.p,b.p,c.p,d.p],[a.n,b.n,c.n,d.n],cn);}}
function ell(cx,cy,cz,rx,ry,rz,seg,rings){const g=[];for(let i=0;i<=rings;i++){const ph=Math.PI*i/rings,sp=Math.sin(ph),cp=Math.cos(ph),row=[];for(let j=0;j<=seg;j++){const th=2*Math.PI*j/seg,ct=Math.cos(th),st=Math.sin(th);row.push({p:[cx+rx*sp*ct,cy+ry*cp,cz+rz*sp*st],n:nrm([sp*ct/rx,cp/ry,sp*st/rz])});}g.push(row);}grid(g);}
function cap(cx,cy,cz,r,ch,seg,rings){const g=[];for(let i=0;i<=rings;i++){const ph=Math.PI*i/rings,sp=Math.sin(ph),cp=Math.cos(ph),yo=(ph<=Math.PI/2)?ch:-ch,row=[];for(let j=0;j<=seg;j++){const th=2*Math.PI*j/seg,ct=Math.cos(th),st=Math.sin(th);row.push({p:[cx+r*sp*ct,cy+yo+r*cp,cz+r*sp*st],n:[sp*ct,cp,sp*st]});}g.push(row);}grid(g);}
curJoint=0; ell(0,0.99,0,0.18,0.13,0.14,10,8); curJoint=1; ell(0,1.28,0,0.21,0.23,0.15,12,8); curJoint=2; ell(0,1.70,0,0.18,0.19,0.18,14,10);
for(const s of [-1,1]){curJoint=s>0?3:6;cap(s*0.26,1.28,0,0.06,0.11,10,10);curJoint=s>0?4:7;cap(s*0.265,0.93,0,0.052,0.11,10,10);curJoint=s>0?5:8;ell(s*0.265,0.69,0,0.07,0.07,0.06,10,8);curJoint=s>0?9:12;cap(s*0.11,0.73,0,0.09,0.12,10,10);curJoint=s>0?10:13;cap(s*0.10,0.31,0,0.07,0.12,10,10);curJoint=s>0?11:14;ell(s*0.10,0.05,0.07,0.07,0.05,0.13,10,8);}

(async()=>{
  // 1) mesh with Walk + Idle
  const mesh=await call("create_mesh_from_geometry",{path:"Models/Humanoid",fileName:"Models/Humanoid.tmd",positions,polygons,normals,jointIndices,joints,animations,meshType:"skinned",material:"HumanoidMat",triangulate:true},180000);
  console.log("1) mesh:",JSON.stringify(mesh).slice(0,200));
  if(mesh.toolError||mesh.err)return;
  await call("set_mesh_material_shader",{guid:MESH,material:"HumanoidMat",shader:MAT});
  console.log("   build mesh:",JSON.stringify(await call("build_asset",{guid:MESH,rebuild:true,wait:true},240000)).slice(0,90));

  // 2) A - Walk take = Walk
  await call("set_instance_member",{guid:AWALK,member:"take",value:"Walk"});
  console.log("2) A-Walk take=Walk; build:",JSON.stringify(await call("build_asset",{guid:AWALK,rebuild:true,wait:true},120000)).slice(0,90));

  // 3) A - Idle asset
  const aidle=await call("create_instance",{path:"Entities/Humanoid/A - Idle",type:"traktor.animation.AnimationAsset"});
  await call("set_instance_member",{guid:aidle.guid,member:"fileName",value:"Models/Humanoid.tmd"});
  await call("set_instance_member",{guid:aidle.guid,member:"take",value:"Idle"});
  await call("set_instance_member",{guid:aidle.guid,member:"removeLocomotion",value:false});
  console.log("3) A-Idle:",aidle.guid,"build:",JSON.stringify(await call("build_asset",{guid:aidle.guid,rebuild:true,wait:true},120000)).slice(0,90));

  // 4) StateGraph (Idle<->Walk on "moving")
  const sg=await call("create_instance",{path:"Entities/Humanoid/SG - Humanoid",type:"traktor.animation.StateGraph"});
  const xml=`<?xml version="1.0" encoding="utf-8"?>
<object type="traktor.animation.StateGraph">
	<states>
		<item type="traktor.animation.StateNodeAnimation"><name>Idle</name><position><first>120</first><second>120</second></position><animation>${aidle.guid}</animation></item>
		<item type="traktor.animation.StateNodeAnimation"><name>Walk</name><position><first>440</first><second>120</second></position><animation>${AWALK}</animation></item>
	</states>
	<transitions>
		<item type="traktor.animation.StateTransition"><from ref="/object/states/item"/><to ref="/object/states/item[1]"/><duration>0.2</duration><moment>Immediately</moment><condition>moving</condition></item>
		<item type="traktor.animation.StateTransition"><from ref="/object/states/item[1]"/><to ref="/object/states/item"/><duration>0.2</duration><moment>Immediately</moment><condition>!moving</condition></item>
	</transitions>
	<rootState ref="/object/states/item"/>
	<previewSkeleton>${SKEL}</previewSkeleton>
	<previewMesh>${MESH}</previewMesh>
	<previewPosition>0, -2, 7, 1</previewPosition>
	<previewAngles>0, 0, 0, 0</previewAngles>
</object>`;
  const imp=await call("import_instance_from_xml",{guid:sg.guid,xml});
  console.log("4) StateGraph:",sg.guid,imp.toolError||imp.err?JSON.stringify(imp):"imported","build:",JSON.stringify(await call("build_asset",{guid:sg.guid,rebuild:true,wait:true},120000)).slice(0,90));
  if(imp.toolError||imp.err)return;

  // 5) switch humanoid pose controller to the state graph
  const r=await call("set_instance_member",{guid:ENTITY,member:"components[0].poseController",
    value:{ "$type":"traktor.animation.AnimationGraphPoseControllerData", set:{ stateGraph: sg.guid } }});
  console.log("5) poseController:",r.toolError||r.err?JSON.stringify(r):"ok");
  if(r.toolError||r.err)return;
  console.log("   build scene:",JSON.stringify(await call("build_asset",{guid:SCENE,rebuild:true,wait:true},300000)).slice(0,110));
  console.log("SG_GUID="+sg.guid);
})();

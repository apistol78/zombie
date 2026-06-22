// Build a labyrinth: brick wall mesh (visual) + concave triangle-mesh collider, placed in the scene.
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
const call = async (n, a, t) => { const r = await rpc("tools/call", { name: n, arguments: a }, t); if (r.error) return { err: JSON.stringify(r.error) }; const x = r.result; return x.isError ? { toolError: x.content[0].text } : (x.structuredContent ?? JSON.parse(x.content[0].text)); };
const rnd = v => Number(v.toFixed(4));
const sub=(a,b)=>[a[0]-b[0],a[1]-b[1],a[2]-b[2]], cross=(a,b)=>[a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]], dot=(a,b)=>a[0]*b[0]+a[1]*b[1]+a[2]*b[2];
const BRICKS="{B6BBC8B0-8D9B-7A48-A4BE-BC1EDAB0147A}", BRICKS_N="{587BF917-4605-9446-8670-AD34116AB6D4}";
const DEFAULT_COL="{F9805131-50C2-504C-9421-13C99E44616C}", SCENE="{FF0A61F5-30AD-4647-A89F-37700B557CD4}";

// ---- maze generation (recursive backtracker / DFS) ----
const N=9, C=4, off=N*C/2, H=2.0, Tw=0.4;
const vWall=[], hWall=[], visited=[];
for(let x=0;x<=N;x++){vWall[x]=[];for(let y=0;y<N;y++)vWall[x][y]=true;}
for(let x=0;x<N;x++){hWall[x]=[];for(let y=0;y<=N;y++)hWall[x][y]=true;}
for(let x=0;x<N;x++){visited[x]=[];for(let y=0;y<N;y++)visited[x][y]=false;}
const stack=[[0,0]]; visited[0][0]=true;
while(stack.length){
  const [cx,cy]=stack[stack.length-1];
  const nb=[];
  if(cx>0 && !visited[cx-1][cy]) nb.push(["W",cx-1,cy]);
  if(cx<N-1 && !visited[cx+1][cy]) nb.push(["E",cx+1,cy]);
  if(cy>0 && !visited[cx][cy-1]) nb.push(["S",cx,cy-1]);
  if(cy<N-1 && !visited[cx][cy+1]) nb.push(["N",cx,cy+1]);
  if(nb.length===0){ stack.pop(); continue; }
  const [dir,nx,ny]=nb[Math.floor(Math.random()*nb.length)];
  if(dir==="W") vWall[cx][cy]=false; else if(dir==="E") vWall[cx+1][cy]=false;
  else if(dir==="S") hWall[cx][cy]=false; else hWall[cx][cy+1]=false;
  visited[nx][ny]=true; stack.push([nx,ny]);
}

// ---- geometry: a brick box per present wall (with world-projected UVs) ----
const positions=[], normals=[], texCoords=[], polygons=[];
const TILE=1.5;
function uvFor(p,n){ if(Math.abs(n[0])>0.5) return [p[2]/TILE,p[1]/TILE]; if(Math.abs(n[1])>0.5) return [p[0]/TILE,p[2]/TILE]; return [p[0]/TILE,p[1]/TILE]; }
function face(ps,n){ const rn=cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])); let ord=[0,1,2,3]; if(dot(rn,n)>0) ord=[0,3,2,1]; const base=positions.length; for(const k of ord){ positions.push(ps[k].map(rnd)); normals.push(n); texCoords.push(uvFor(ps[k],n).map(rnd)); } polygons.push([base,base+1,base+2,base+3]); }
function box(cx,cy,cz,hx,hy,hz){ const x0=cx-hx,x1=cx+hx,y0=cy-hy,y1=cy+hy,z0=cz-hz,z1=cz+hz; const P=[[x0,y0,z0],[x1,y0,z0],[x1,y1,z0],[x0,y1,z0],[x0,y0,z1],[x1,y0,z1],[x1,y1,z1],[x0,y1,z1]];
  face([P[4],P[5],P[6],P[7]],[0,0,1]); face([P[1],P[0],P[3],P[2]],[0,0,-1]); face([P[1],P[5],P[6],P[2]],[1,0,0]); face([P[0],P[4],P[7],P[3]],[-1,0,0]); face([P[3],P[2],P[6],P[7]],[0,1,0]); face([P[4],P[5],P[1],P[0]],[0,-1,0]); }
for(let x=0;x<=N;x++)for(let y=0;y<N;y++) if(vWall[x][y]) box(x*C-off, H/2, (y+0.5)*C-off, Tw/2, H/2, C/2+Tw/2);
for(let x=0;x<N;x++)for(let y=0;y<=N;y++) if(hWall[x][y]) box((x+0.5)*C-off, H/2, y*C-off, C/2+Tw/2, H/2, Tw/2);

(async()=>{
  console.log("maze: "+N+"x"+N+" cells, "+(positions.length/4)+" quads, "+positions.length+" verts");
  // 1) visual mesh (brick)
  const mesh=await call("create_mesh_from_geometry",{path:"Models/Maze",fileName:"Models/Maze.tmd",positions,polygons,normals,texCoords,material:"MazeMat",maps:{diffuse:BRICKS,normal:BRICKS_N},triangulate:true},180000);
  console.log("1) visual mesh:",JSON.stringify(mesh).slice(0,200));
  if(mesh.toolError||mesh.err)return;
  console.log("   build:",JSON.stringify(await call("build_asset",{guid:mesh.guid,rebuild:true,wait:true},240000)).slice(0,90));

  // 2) physics collision mesh (concave triangle mesh)
  const phys=await call("create_instance",{path:"Models/Maze - Collision",type:"traktor.physics.MeshAsset"});
  await call("set_instance_member",{guid:phys.guid,member:"fileName",value:"Models/Maze.tmd"});
  await call("set_instance_member",{guid:phys.guid,member:"calculateConvexHull",value:false});
  await call("set_instance_member",{guid:phys.guid,member:"margin",value:0.0});
  console.log("2) physics mesh:",phys.guid,"build:",JSON.stringify(await call("build_asset",{guid:phys.guid,rebuild:true,wait:true},240000)).slice(0,90));

  // 3) maze entity (visual + static mesh collider), appended to the Entity layer
  const newGuid="{"+crypto.randomUUID().toUpperCase()+"}";
  const r=await call("set_instance_member",{guid:SCENE,member:"layers[1].components[0].entityData",append:true,value:[
    { "$type":"traktor.world.EntityData", set:{
      name:"Maze", id:newGuid, "transform.translation":[0,0,0,1],
      components:[
        { "$type":"traktor.mesh.MeshComponentData", set:{ mesh: mesh.guid } },
        { "$type":"traktor.physics.RigidBodyComponentData", set:{
          bodyDesc:{ "$type":"traktor.physics.StaticBodyDesc", set:{
            shape:{ "$type":"traktor.physics.MeshShapeDesc", set:{ mesh: phys.guid, collisionGroup:[DEFAULT_COL], collisionMask:[DEFAULT_COL] } }
          }}
        }}
      ]
    }}
  ]});
  console.log("3) maze entity appended:",r.toolError||r.err?JSON.stringify(r):"ok");
  if(r.toolError||r.err)return;
  console.log("   build scene:",JSON.stringify(await call("build_asset",{guid:SCENE,rebuild:true,wait:true},300000)).slice(0,110));
})();

// Two-level maze: ground maze + upper maze on a slab, joined by a ramp through a shaft.
// Writes the combined geometry to Models/Maze.tmd (same guid the scene already references).
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
const MESH="{66B277D9-1F75-1443-817D-BFE6D673FFFE}";

const N=8, C=4, off=N*C/2, H=4.0, Tw=0.4, SLAB=0.4, YUP=H+SLAB; // YUP=4.4 upper floor top (tall floors give the follow-camera headroom)
// Stair shaft (hole in the slab); ramp rises west->east inside it. Longer run for the taller climb.
const hx0=-6, hx1=6, hz0=-2, hz1=2;

// Seeded PRNG (mulberry32) so the layout is reproducible across regenerations.
let _seed = 0x1a2b3c4d;
function rng(){ _seed |= 0; _seed = (_seed + 0x6D2B79F5) | 0; let t = Math.imul(_seed ^ (_seed >>> 15), 1 | _seed); t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296; }

function genMaze(){
  const vWall=[], hWall=[], visited=[];
  for(let x=0;x<=N;x++){vWall[x]=[];for(let y=0;y<N;y++)vWall[x][y]=true;}
  for(let x=0;x<N;x++){hWall[x]=[];for(let y=0;y<=N;y++)hWall[x][y]=true;}
  for(let x=0;x<N;x++){visited[x]=[];for(let y=0;y<N;y++)visited[x][y]=false;}
  const stack=[[0,0]]; visited[0][0]=true;
  while(stack.length){
    const [cx,cy]=stack[stack.length-1]; const nb=[];
    if(cx>0 && !visited[cx-1][cy]) nb.push(["W",cx-1,cy]);
    if(cx<N-1 && !visited[cx+1][cy]) nb.push(["E",cx+1,cy]);
    if(cy>0 && !visited[cx][cy-1]) nb.push(["S",cx,cy-1]);
    if(cy<N-1 && !visited[cx][cy+1]) nb.push(["N",cx,cy+1]);
    if(nb.length===0){ stack.pop(); continue; }
    const [dir,nx,ny]=nb[Math.floor(rng()*nb.length)];
    if(dir==="W") vWall[cx][cy]=false; else if(dir==="E") vWall[cx+1][cy]=false;
    else if(dir==="S") hWall[cx][cy]=false; else hWall[cx][cy+1]=false;
    visited[nx][ny]=true; stack.push([nx,ny]);
  }
  return {vWall,hWall};
}

const positions=[], normals=[], texCoords=[], polygons=[];
const TILE=1.5;
function uvFor(p,n){ if(Math.abs(n[0])>0.5) return [p[2]/TILE,p[1]/TILE]; if(Math.abs(n[1])>0.5) return [p[0]/TILE,p[2]/TILE]; return [p[0]/TILE,p[1]/TILE]; }
function face(ps,n){ const rn=cross(sub(ps[1],ps[0]),sub(ps[2],ps[0])); let ord=[0,1,2,3]; if(dot(rn,n)>0) ord=[0,3,2,1]; const base=positions.length; for(const k of ord){ positions.push(ps[k].map(rnd)); normals.push(n.map(rnd)); texCoords.push(uvFor(ps[k],n).map(rnd)); } polygons.push([base,base+1,base+2,base+3]); }
function box(cx,cy,cz,hx,hy,hz){ const x0=cx-hx,x1=cx+hx,y0=cy-hy,y1=cy+hy,z0=cz-hz,z1=cz+hz; const P=[[x0,y0,z0],[x1,y0,z0],[x1,y1,z0],[x0,y1,z0],[x0,y0,z1],[x1,y0,z1],[x1,y1,z1],[x0,y1,z1]];
  face([P[4],P[5],P[6],P[7]],[0,0,1]); face([P[1],P[0],P[3],P[2]],[0,0,-1]); face([P[1],P[5],P[6],P[2]],[1,0,0]); face([P[0],P[4],P[7],P[3]],[-1,0,0]); face([P[3],P[2],P[6],P[7]],[0,1,0]); face([P[4],P[5],P[1],P[0]],[0,-1,0]); }
// axis-aligned box from min/max corners
function aabox(x0,x1,y0,y1,z0,z1){ box((x0+x1)/2,(y0+y1)/2,(z0+z1)/2,(x1-x0)/2,(y1-y0)/2,(z1-z0)/2); }

// Does a wall box footprint overlap the shaft hole? (skip those so the shaft stays open)
function overlapsHole(cx,cz,hx,hz){ return (cx-hx < hx1) && (cx+hx > hx0) && (cz-hz < hz1) && (cz+hz > hz0); }

function walls(maze, baseY){
  for(let x=0;x<=N;x++)for(let y=0;y<N;y++) if(maze.vWall[x][y]){ const cx=x*C-off, cz=(y+0.5)*C-off, hx=Tw/2, hz=C/2+Tw/2; if(overlapsHole(cx,cz,hx,hz)) continue; box(cx, baseY+H/2, cz, hx, H/2, hz); }
  for(let x=0;x<N;x++)for(let y=0;y<=N;y++) if(maze.hWall[x][y]){ const cx=(x+0.5)*C-off, cz=y*C-off, hx=C/2+Tw/2, hz=Tw/2; if(overlapsHole(cx,cz,hx,hz)) continue; box(cx, baseY+H/2, cz, hx, H/2, hz); }
}

// --- Level 0 walls (ground maze) ---
walls(genMaze(), 0);

// --- Upper floor slab over the footprint, minus the shaft hole (4 boxes) ---
aabox(-off, off, H, YUP, -off, hz0);        // south strip
aabox(-off, off, H, YUP, hz1, off);         // north strip
aabox(-off, hx0, H, YUP, hz0, hz1);         // west of shaft
aabox(hx1, off, H, YUP, hz0, hz1);          // east of shaft

// --- Level 1 walls (upper maze) on the slab ---
walls(genMaze(), YUP);

// --- Ramp: inclined slab from (hx0, y=0) up to (hx1, y=YUP), width = shaft z ---
(function ramp(){
  const T=0.3; // thickness (downward)
  const yA=0.0, yB=YUP; // y at hx0 and hx1
  // top corners (CCW-ish): at x=hx0 y=yA, at x=hx1 y=yB
  const A=[hx0,yA,hz0], B=[hx1,yB,hz0], Cc=[hx1,yB,hz1], D=[hx0,yA,hz1];
  const E=[hx0,yA-T,hz0], F=[hx1,yB-T,hz0], G=[hx1,yB-T,hz1], Hh=[hx0,yA-T,hz1];
  const nUp=[-(yB-yA),(hx1-hx0),0]; // outward normal of the inclined top (points up & -x)
  face([A,B,Cc,D], nUp);                 // top (walkable)
  face([E,F,G,Hh], [(yB-yA),-(hx1-hx0),0]); // bottom
  face([A,B,F,E], [0,0,-1]);             // -z side
  face([D,Cc,G,Hh], [0,0,1]);            // +z side
  face([A,D,Hh,E], [-1,0,0]);            // start cap (low end)
  face([B,Cc,G,F], [1,0,0]);             // end cap (high end)
})();

(async()=>{
  console.log("two-level maze: "+(positions.length/4)+" quads, "+positions.length+" verts");
  const mesh=await call("create_mesh_from_geometry",{path:"Models/Maze",fileName:"Models/Maze.tmd",guid:MESH,positions,polygons,normals,texCoords,material:"MazeMat",triangulate:true},300000);
  console.log("create_mesh:",JSON.stringify(mesh).slice(0,220));
})();

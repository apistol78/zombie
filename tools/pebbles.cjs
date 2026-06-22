// Generate grey pebblestone textures (color + normal) and apply them to the ground.
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

// ---- procedural grey pebblestone (tileable Voronoi cells) ----
const W=256,H=256,G=7,cw=W/G,ch=H/G;
const seeds=[];
for(let gx=0;gx<G;gx++){seeds[gx]=[];for(let gy=0;gy<G;gy++){seeds[gx][gy]={x:(gx+0.15+0.7*Math.random())*cw,y:(gy+0.15+0.7*Math.random())*ch,grey:118+Math.random()*52};}}
const r=cw*0.62;
const height=new Float32Array(W*H), grey=new Uint8Array(W*H);
for(let py=0;py<H;py++)for(let px=0;px<W;px++){
  const gx=Math.floor(px/cw),gy=Math.floor(py/ch); let d1=1e9,d2=1e9,g1=128;
  for(let ox=-1;ox<=1;ox++)for(let oy=-1;oy<=1;oy++){
    let sx=gx+ox,sy=gy+oy,wx=0,wy=0;
    if(sx<0){sx+=G;wx=-W;}else if(sx>=G){sx-=G;wx=W;}
    if(sy<0){sy+=G;wy=-H;}else if(sy>=G){sy-=G;wy=H;}
    const s=seeds[sx][sy],dx=px-(s.x+wx),dy=py-(s.y+wy),d=Math.sqrt(dx*dx+dy*dy);
    if(d<d1){d2=d1;d1=d;g1=s.grey;}else if(d<d2){d2=d;}
  }
  const ph = d1<r ? Math.sqrt(Math.max(0,1-(d1/r)*(d1/r))) : 0;
  height[py*W+px]=ph;
  let v = g1*(0.5+0.5*ph) + (Math.random()*2-1)*5;
  if(ph<0.05) v*=0.68;                 // darker mortar in the gaps
  grey[py*W+px]=Math.max(0,Math.min(255,v|0));
}
const hAt=(x,y)=>{x=((x%W)+W)%W;y=((y%H)+H)%H;return height[y*W+x];};
function tga(rgbFn){
  const hdr=Buffer.alloc(18); hdr[2]=2; hdr.writeUInt16LE(W,12); hdr.writeUInt16LE(H,14); hdr[16]=24; hdr[17]=0x20;
  const pix=Buffer.alloc(W*H*3); let o=0;
  for(let y=0;y<H;y++)for(let x=0;x<W;x++){const c=rgbFn(x,y); pix[o++]=c[2]; pix[o++]=c[1]; pix[o++]=c[0];}
  return Buffer.concat([hdr,pix]).toString("base64");
}
const colorB64 = tga((x,y)=>{const v=grey[y*W+x]; return [v,v,v];});
const STR=3.2;
const normalB64 = tga((x,y)=>{
  let nx=(hAt(x-1,y)-hAt(x+1,y))*STR, ny=(hAt(x,y-1)-hAt(x,y+1))*STR, nz=1;
  const l=Math.sqrt(nx*nx+ny*ny+nz*nz)||1; nx/=l;ny/=l;nz/=l;
  return [Math.round((nx*0.5+0.5)*255),Math.round((ny*0.5+0.5)*255),Math.round((nz*0.5+0.5)*255)];
});

(async()=>{
  console.log("generated pebble tgas: color "+colorB64.length+"b64, normal "+normalB64.length+"b64");
  // import color + normal
  const col=await call("import_texture_from_data",{path:"Textures/Pebbles",fileName:"Textures/Pebbles.tga",data:colorB64},120000);
  console.log("color tex:",col.toolError||col.err?JSON.stringify(col):col.guid);
  if(col.toolError||col.err)return;
  const nor=await call("import_texture_from_data",{path:"Textures/Pebbles_N",fileName:"Textures/Pebbles_N.tga",data:normalB64},120000);
  console.log("normal tex:",nor.toolError||nor.err?JSON.stringify(nor):nor.guid);
  if(nor.toolError||nor.err)return;
  // flag the normal map
  await call("set_instance_member",{guid:nor.guid,member:"normalMap",value:true});
  await call("set_instance_member",{guid:nor.guid,member:"assumeLinearGamma",value:true});
  // build textures
  console.log("build color:",JSON.stringify(await call("build_asset",{guid:col.guid,rebuild:true,wait:true},120000)).slice(0,80));
  console.log("build normal:",JSON.stringify(await call("build_asset",{guid:nor.guid,rebuild:true,wait:true},120000)).slice(0,80));

  // re-skin the ground with the pebble textures (same flat 50x50 quad)
  const R=18;
  const positions=[[-25,0.5,-25],[25,0.5,-25],[25,0.5,25],[-25,0.5,25]];
  const normals=[[0,1,0],[0,1,0],[0,1,0],[0,1,0]];
  const texCoords=[[0,0],[R,0],[R,R],[0,R]];
  const polygons=[[0,1,2,3]];
  const mesh=await call("create_mesh_from_geometry",{path:"Models/Ground",fileName:"Models/Ground.tmd",positions,polygons,normals,texCoords,material:"GroundMat",maps:{diffuse:col.guid,normal:nor.guid},triangulate:true},120000);
  console.log("ground mesh:",mesh.toolError||mesh.err?JSON.stringify(mesh):mesh.guid);
  if(mesh.toolError||mesh.err)return;
  console.log("build ground:",JSON.stringify(await call("build_asset",{guid:mesh.guid,rebuild:true,wait:true},180000)).slice(0,90));
})();

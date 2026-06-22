// Re-create the torus WITH material maps (brick albedo + normal), then build.
// Run after rebuilding the editor (create_mesh_from_geometry now accepts "maps").
const http = require("node:http");
function rpc(m, p, t) {
  return new Promise((res, rej) => {
    const b = JSON.stringify({ jsonrpc: "2.0", id: 1, method: m, params: p });
    const r = http.request({ host: "127.0.0.1", port: 13880, path: "/", method: "POST",
      headers: { "Content-Type": "application/json", "Content-Length": Buffer.byteLength(b) } },
      x => { let d = ""; x.on("data", c => d += c); x.on("end", () => { try { res(JSON.parse(d)); } catch (e) { rej(new Error("bad json")); } }); });
    r.on("error", rej); if (t) r.setTimeout(t, () => r.destroy(new Error("timeout"))); r.write(b); r.end();
  });
}
const call = async (n, a, t) => { const r = await rpc("tools/call", { name: n, arguments: a }, t); if (r.error) return { err: r.error }; const x = r.result; return x.isError ? { toolError: x.content[0].text } : (x.structuredContent ?? JSON.parse(x.content[0].text)); };
const rnd = v => Number(v.toFixed(5));

const BRICKS = "{B6BBC8B0-8D9B-7A48-A4BE-BC1EDAB0147A}";   // color / albedo
const BRICKS_N = "{587BF917-4605-9446-8670-AD34116AB6D4}"; // normal map

(async () => {
  // Guard: confirm the rebuilt tool exposes the "maps" argument.
  const tool = (await rpc("tools/list")).result.tools.find(t => t.name === "create_mesh_from_geometry");
  const hasMaps = tool && tool.inputSchema && tool.inputSchema.properties && tool.inputSchema.properties.maps;
  console.log("maps argument present:", !!hasMaps);
  if (!hasMaps) { console.log("Editor not rebuilt yet (no 'maps' arg) - aborting."); return; }

  // Torus geometry: reversed (clockwise) winding + UVs, seam duplicated.
  const U = 48, V = 24, R = 1.0, r = 0.35, repU = 9, repV = 3;
  const positions = [], normals = [], texCoords = [];
  for (let i = 0; i <= U; i++) { const u = 2 * Math.PI * i / U, cu = Math.cos(u), su = Math.sin(u);
    for (let j = 0; j <= V; j++) { const v = 2 * Math.PI * j / V, cv = Math.cos(v), sv = Math.sin(v);
      positions.push([rnd((R + r * cv) * cu), rnd(r * sv), rnd((R + r * cv) * su)]);
      normals.push([rnd(cv * cu), rnd(sv), rnd(cv * su)]);
      texCoords.push([rnd(i / U * repU), rnd(j / V * repV)]);
    } }
  const w = V + 1, idx = (i, j) => i * w + j, polygons = [];
  for (let i = 0; i < U; i++) for (let j = 0; j < V; j++) polygons.push([idx(i, j), idx(i + 1, j), idx(i + 1, j + 1), idx(i, j + 1)]);

  const mesh = await call("create_mesh_from_geometry", {
    path: "Models/Torus", fileName: "Models/Torus.tmd",
    positions, polygons, normals, texCoords,
    material: "TorusMat",
    maps: { diffuse: BRICKS, normal: BRICKS_N },
    triangulate: true
  }, 120000);
  console.log("create_mesh:", JSON.stringify(mesh).slice(0, 360));
  if (mesh.toolError || mesh.err) return;

  const built = await call("build_asset", { guid: mesh.guid, rebuild: true, wait: true }, 180000);
  console.log("build:", JSON.stringify(built));
})();

// Add a static box ground collider to the "Many Characters" scene (Entity layer),
// using set_instance_member append mode (no reconstruction of existing entities).
// Run after rebuilding the editor (set_instance_member must accept "append").
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
const SCENE = "{FF0A61F5-30AD-4647-A89F-37700B557CD4}";
const DEFAULT_COL = "{F9805131-50C2-504C-9421-13C99E44616C}";

const groundSpec = {
  "$type": "traktor.world.EntityData",
  set: {
    name: "Ground",
    id: newGuid(),
    "transform.translation": [0, -0.5, 0, 1],
    components: [
      { "$type": "traktor.physics.RigidBodyComponentData", set: {
        bodyDesc: { "$type": "traktor.physics.StaticBodyDesc", set: {
          shape: { "$type": "traktor.physics.BoxShapeDesc", set: {
            extent: [25, 0.5, 25, 0],
            margin: 0.04,
            collisionGroup: [DEFAULT_COL],
            collisionMask: [DEFAULT_COL]
          } }
        } }
      } }
    ]
  }
};

(async () => {
  const tool = (await rpc("tools/list")).result.tools.find(t => t.name === "set_instance_member");
  if (!tool.inputSchema.properties.append) { console.log("Editor not rebuilt (no append arg) - aborting."); return; }

  const r = await call("set_instance_member", { guid: SCENE, member: "layers[1].components[0].entityData", value: [groundSpec], append: true });
  console.log("append ground:", r.toolError || r.err ? JSON.stringify(r) : "ok committed=" + r.committed);
  if (r.toolError || r.err) return;

  // verify
  const s = await call("inspect_instance", { guid: SCENE, maxDepth: 8 });
  const layer = s.members.find(m => m.name === "layers").elements[1];
  const ed = layer.members.find(m => m.name === "components").elements[0].members.find(m => m.name === "entityData");
  const names = (ed.elements || []).map(e => (e.members.find(m => m.name === "name") || {}).value);
  console.log("entityData (" + (ed.elements || []).length + "): " + names.join(", "));
  const g = (ed.elements || []).find(e => (e.members.find(m => m.name === "name") || {}).value === "Ground");
  if (g) {
    const comp = g.members.find(m => m.name === "components").elements[0];
    const shape = comp.members.find(m => m.name === "bodyDesc").members.find(m => m.name === "shape");
    const ext = shape.members.find(m => m.name === "extent");
    const cg = shape.members.find(m => m.name === "collisionGroup");
    console.log("  Ground:", comp.type, "shape", shape.type, "extent", JSON.stringify(ext.value),
      "collisionGroup", JSON.stringify((cg.elements || []).map(x => x.value)));
  }

  console.log("build scene:", JSON.stringify(await call("build_asset", { guid: SCENE, rebuild: false, wait: true }, 300000)));
})();

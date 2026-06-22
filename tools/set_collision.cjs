// Set the humanoid CharacterComponent's collision/trace groups to "Default".
// Run after rebuilding the editor (set_instance_member must accept guid-string arrays).
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
const ENTITY = "{BCF74021-58D3-B24A-8E73-BEC797740D24}";
const DEFAULT_COL = "{F9805131-50C2-504C-9421-13C99E44616C}";

(async () => {
  // find the CharacterComponentData index
  let e = await call("inspect_instance", { guid: ENTITY, maxDepth: 2 });
  const comps = e.members.find(m => m.name === "components");
  const idx = (comps.elements || []).findIndex(el => (el.type || "").indexOf("CharacterComponentData") >= 0);
  if (idx < 0) { console.log("no CharacterComponentData found"); return; }
  console.log("CharacterComponentData at components[" + idx + "]");

  for (const member of ["collisionGroup", "collisionMask", "traceInclude"]) {
    const r = await call("set_instance_member", { guid: ENTITY, member: "components[" + idx + "]." + member, value: [DEFAULT_COL] });
    console.log(member + ": " + (r.toolError || r.err ? JSON.stringify(r) : "ok"));
    if (r.toolError) { console.log("  -> editor likely not rebuilt yet (guid-array support missing). Aborting."); return; }
  }

  // verify
  e = await call("inspect_instance", { guid: ENTITY, maxDepth: 4 });
  const ch = e.members.find(m => m.name === "components").elements[idx];
  for (const m of ch.members || []) {
    if (["collisionGroup", "collisionMask", "traceInclude", "traceIgnore"].includes(m.name)) {
      const vals = (m.elements || []).map(x => x.value).filter(Boolean);
      console.log("  " + m.name + " = [" + vals.join(", ") + "]");
    }
  }
})();

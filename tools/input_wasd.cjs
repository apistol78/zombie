// Map STATE_MOVE_X (A/D) and STATE_MOVE_Z (W/S) onto WASD in Input/Input.
// Run after rebuilding the editor (set_instance_member must support map-mode + {"$grow":N}).
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
const INPUT = "{E7DE1157-DA53-B14F-9A14-70707DACAFB9}";
const kb = key => ({ "$type": "traktor.input.KeyboardInputSourceData", set: { "controlTypes": { "$grow": 1 }, "controlTypes[0]": key } });
const readv = id => ({ "$type": "traktor.input.InReadValue", set: { "valueId": id } });
const combine = (idPos, idNeg) => ({ "$type": "traktor.input.InputStateData", set: {
  "source": { "$type": "traktor.input.InCombine", set: {
    "operator": "CoAdd",
    "entries": { "$grow": 2 },
    "entries[0].source": readv(idPos), "entries[0].mul": 1, "entries[0].add": 0,
    "entries[1].source": readv(idNeg), "entries[1].mul": -1, "entries[1].add": 0
  } }
} });

(async () => {
  const tool = (await rpc("tools/list")).result.tools.find(t => t.name === "set_instance_member");
  if (!/\$grow/.test(tool.description)) { console.log("Editor not rebuilt (no map/grow support) - aborting."); return; }

  // 4 keyboard sources
  const r1 = await call("set_instance_member", { guid: INPUT, member: "sourceData.sourceData", value: {
    "ID_KEY_W": kb("KeyW"), "ID_KEY_A": kb("KeyA"), "ID_KEY_S": kb("KeyS"), "ID_KEY_D": kb("KeyD")
  } });
  console.log("sourceData:", r1.toolError || r1.err ? JSON.stringify(r1) : "ok");
  if (r1.toolError || r1.err) return;

  // X = A(+1) - D(-1) ; Z = W(+1) - S(-1)
  const r2 = await call("set_instance_member", { guid: INPUT, member: "stateData.stateData", value: {
    "STATE_MOVE_X": combine("ID_KEY_A", "ID_KEY_D"),
    "STATE_MOVE_Z": combine("ID_KEY_W", "ID_KEY_S")
  } });
  console.log("stateData:", r2.toolError || r2.err ? JSON.stringify(r2) : "ok");
  if (r2.toolError || r2.err) return;

  // verify
  const mem = (o, n) => ((o && o.members) || []).find(m => m.name === n);
  const e = await call("inspect_instance", { guid: INPUT, maxDepth: 9 });
  const sd = mem(mem(e, "sourceData"), "sourceData");
  console.log("--- sources ---");
  for (const el of sd.elements || []) {
    const id = (mem(el, "first") || {}).value;
    const ct = mem(mem(el, "second"), "controlTypes");
    const key = ((ct && ct.elements || [])[0] || {}).value;
    console.log("  " + id + " -> " + key);
  }
  const st = mem(mem(e, "stateData"), "stateData");
  console.log("--- states ---");
  for (const el of st.elements || []) {
    const name = (mem(el, "first") || {}).value;
    const src = mem(mem(el, "second"), "source");
    const entries = mem(src, "entries");
    const parts = (entries && entries.elements || []).map(en => {
      const v = mem(mem(en, "source"), "valueId");
      const mul = mem(en, "mul");
      return (v ? v.value : "?") + "*" + (mul ? mul.value : "?");
    });
    console.log("  " + name + " = " + (src ? src.type : "?") + "(" + parts.join(", ") + ")");
  }

  console.log("build:", JSON.stringify(await call("build_asset", { guid: INPUT, rebuild: true, wait: true }, 120000)));
})();

// Re-import Input/Input adding a Space->STATE_JUMP mapping, keeping the editor graph consistent (shared refs).
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

// inputNodes: A(0) D(1) W(2) S(3) InCombine_X(4=A-D) InCombine_Z(5=W-S) Space(6)
// positions: 0-5 node layout, 6 = State X, 7 = State Z, 8 = Space layout, 9 = State JUMP
// stateData: MOVE_X->pos[6], MOVE_Z->pos[7], JUMP->pos[9]  (all shared refs)
const xml = `<?xml version="1.0" encoding="utf-8"?>
<object type="traktor.input.InputMappingAsset" version="2">
	<inputNodes>
		<item type="traktor.input.InReadValue"><valueId>ID_KEY_A</valueId></item>
		<item type="traktor.input.InReadValue"><valueId>ID_KEY_D</valueId></item>
		<item type="traktor.input.InReadValue"><valueId>ID_KEY_W</valueId></item>
		<item type="traktor.input.InReadValue"><valueId>ID_KEY_S</valueId></item>
		<item type="traktor.input.InCombine" version="1">
			<entries>
				<item><source ref="/object/inputNodes/item"/><mul>1</mul><add>0</add></item>
				<item><source ref="/object/inputNodes/item[1]"/><mul>-1</mul><add>0</add></item>
			</entries>
			<operator>CoAdd</operator>
		</item>
		<item type="traktor.input.InCombine" version="1">
			<entries>
				<item><source ref="/object/inputNodes/item[2]"/><mul>1</mul><add>0</add></item>
				<item><source ref="/object/inputNodes/item[3]"/><mul>-1</mul><add>0</add></item>
			</entries>
			<operator>CoAdd</operator>
		</item>
		<item type="traktor.input.InReadValue"><valueId>ID_KEY_SPACE</valueId></item>
	</inputNodes>
	<positions>
		<item><object ref="/object/inputNodes/item"/><position><x>120</x><y>120</y></position></item>
		<item><object ref="/object/inputNodes/item[1]"/><position><x>120</x><y>260</y></position></item>
		<item><object ref="/object/inputNodes/item[2]"/><position><x>120</x><y>440</y></position></item>
		<item><object ref="/object/inputNodes/item[3]"/><position><x>120</x><y>580</y></position></item>
		<item><object ref="/object/inputNodes/item[4]"/><position><x>460</x><y>190</y></position></item>
		<item><object ref="/object/inputNodes/item[5]"/><position><x>460</x><y>480</y></position></item>
		<item><object type="traktor.input.InputStateData"><source ref="/object/inputNodes/item[4]"/></object><position><x>820</x><y>190</y></position></item>
		<item><object type="traktor.input.InputStateData"><source ref="/object/inputNodes/item[5]"/></object><position><x>820</x><y>480</y></position></item>
		<item><object ref="/object/inputNodes/item[6]"/><position><x>120</x><y>720</y></position></item>
		<item><object type="traktor.input.InputStateData"><source ref="/object/inputNodes/item[6]"/></object><position><x>820</x><y>720</y></position></item>
	</positions>
	<sourceData type="traktor.input.InputMappingSourceData">
		<sourceData>
			<item><first>ID_KEY_A</first><second type="traktor.input.KeyboardInputSourceData" version="1"><controlTypes><item>KeyA</item></controlTypes></second></item>
			<item><first>ID_KEY_D</first><second type="traktor.input.KeyboardInputSourceData" version="1"><controlTypes><item>KeyD</item></controlTypes></second></item>
			<item><first>ID_KEY_S</first><second type="traktor.input.KeyboardInputSourceData" version="1"><controlTypes><item>KeyS</item></controlTypes></second></item>
			<item><first>ID_KEY_W</first><second type="traktor.input.KeyboardInputSourceData" version="1"><controlTypes><item>KeyW</item></controlTypes></second></item>
			<item><first>ID_KEY_SPACE</first><second type="traktor.input.KeyboardInputSourceData" version="1"><controlTypes><item>KeySpace</item></controlTypes></second></item>
		</sourceData>
	</sourceData>
	<stateData type="traktor.input.InputMappingStateData">
		<stateData>
			<item><first>STATE_MOVE_X</first><second ref="/object/positions/item[6]/object"/></item>
			<item><first>STATE_MOVE_Z</first><second ref="/object/positions/item[7]/object"/></item>
			<item><first>STATE_JUMP</first><second ref="/object/positions/item[9]/object"/></item>
		</stateData>
	</stateData>
	<dependencies/>
</object>`;

(async () => {
  const r = await call("import_instance_from_xml", { guid: INPUT, xml });
  console.log("import:", r.toolError || r.err ? JSON.stringify(r) : ("ok type=" + r.type));
  if (r.toolError || r.err) return;
  const mem = (o, n) => ((o && o.members) || []).find(m => m.name === n);
  const e = await call("inspect_instance", { guid: INPUT, maxDepth: 6 });
  const st = mem(mem(e, "stateData"), "stateData");
  console.log("states:", (st.elements || []).map(el => (mem(el, "first") || {}).value).join(", "));
  const sd = mem(mem(e, "sourceData"), "sourceData");
  console.log("sources:", (sd.elements || []).map(el => (mem(el, "first") || {}).value).join(", "));
  console.log("build:", JSON.stringify(await call("build_asset", { guid: INPUT, rebuild: true, wait: true }, 120000)));
})();

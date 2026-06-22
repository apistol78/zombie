// Record this session's procedures as published SkillAsset instances under a "Skills" group.
const http = require("node:http"), fs = require("node:fs");
function rpc(m, p, t) {
  return new Promise((res, rej) => {
    const b = JSON.stringify({ jsonrpc: "2.0", id: 1, method: m, params: p });
    const r = http.request({ host: "127.0.0.1", port: 13880, path: "/", method: "POST",
      headers: { "Content-Type": "application/json", "Content-Length": Buffer.byteLength(b) } },
      x => { let d = ""; x.on("data", c => d += c); x.on("end", () => { try { res(JSON.parse(d)); } catch (e) { rej(new Error("bad json: " + d.slice(0,200))); } }); });
    r.on("error", rej); if (t) r.setTimeout(t, () => r.destroy(new Error("timeout"))); r.write(b); r.end();
  });
}
const call = async (n, a, t) => { const r = await rpc("tools/call", { name: n, arguments: a }, t); if (r.error) return { err: JSON.stringify(r.error) }; const x = r.result; return x.isError ? { toolError: x.content[0].text } : (x.structuredContent ?? JSON.parse(x.content[0].text)); };
const dir = "D:/Temp/slask/skills/";
const body = f => fs.readFileSync(dir + f, "utf8");

const skills = [
  { path: "Skills/Create Animated Character", name: "create-animated-character",
    file: "animated_character.md",
    description: "Build a skinned, animated character entity (skeleton, weights, clips, state graph, CharacterComponent).",
    whenToUse: "Asked to create a playable/animated character or creature with a skeleton and animations.",
    parameters: [ {name:"name",description:"Entity/asset name",defaultValue:"Character"}, {name:"color",description:"Body colour",defaultValue:"yellow"}, {name:"height",description:"Approx height in metres",defaultValue:"1.85"} ] },
  { path: "Skills/Create Mesh From Geometry", name: "create-mesh-from-geometry",
    file: "mesh_from_geometry.md",
    description: "Author a render mesh from procedurally generated vertices via create_mesh_from_geometry.",
    whenToUse: "Need a custom mesh from generated geometry (winding, UVs, textured material, static or skinned).",
    parameters: [ {name:"name",description:"Mesh asset name",defaultValue:"Mesh"}, {name:"material",description:"Material name",defaultValue:"Material"} ] },
  { path: "Skills/Create Procedural Texture", name: "create-procedural-texture",
    file: "procedural_texture.md",
    description: "Generate a tileable texture (color/normal) as TGA, import it, and bind it to a mesh.",
    whenToUse: "Asked for a generated/procedural texture or to re-skin a surface (pebbles, brick, noise).",
    parameters: [ {name:"name",description:"Texture name",defaultValue:"Texture"}, {name:"size",description:"Texture size in pixels",defaultValue:"256"} ] },
  { path: "Skills/Add Static Mesh Collider", name: "add-static-mesh-collider",
    file: "static_mesh_collider.md",
    description: "Give static/level geometry a concave triangle-mesh collider a character controller respects.",
    whenToUse: "Need collision for level/static geometry (maze, terrain, props) from an existing model.",
    parameters: [ {name:"name",description:"Entity name",defaultValue:"Level"}, {name:"model",description:"Source model path (.tmd)",defaultValue:"Models/Level.tmd"} ] },
  { path: "Skills/Third Person Controller", name: "third-person-controller",
    file: "third_person_controller.md",
    description: "Wire WASD/Space input, drive a CharacterComponent from a LUA script, and follow with a rubberband camera.",
    whenToUse: "Asked to make a character playable / add third-person movement, jump, or a follow camera.",
    parameters: [ {name:"character",description:"Controlled entity name",defaultValue:"Humanoid"}, {name:"speed",description:"Move speed (m/s)",defaultValue:"3"}, {name:"turnRate",description:"Turn rate",defaultValue:"4.5"} ] },
];

(async()=>{
  // 0) verify the rebuilt server exposes the new tools + prompts capability
  const tl = await rpc("tools/list", {});
  const names = (tl.result?.tools || []).map(t => t.name);
  const need = ["create_skill","list_skills","get_skill","publish_skill"];
  const missing = need.filter(n => !names.includes(n));
  if (missing.length) { console.log("ABORT - tools missing (rebuild not live?):", missing.join(", ")); return; }
  const init = await rpc("initialize", {});
  console.log("prompts capability:", JSON.stringify(init.result?.capabilities?.prompts ?? "ABSENT"));

  // 1) create + publish each skill
  for (const s of skills) {
    const r = await call("create_skill", { path: s.path, name: s.name, description: s.description, whenToUse: s.whenToUse, body: body(s.file), parameters: s.parameters, published: true });
    console.log((r.toolError||r.err) ? ("FAIL "+s.name+": "+JSON.stringify(r)) : ("created+published  "+s.name+"  "+r.guid));
  }

  // 2) verify via list_skills + prompts/list
  const ls = await call("list_skills", {});
  console.log("\nlist_skills -> "+ls.count+" skills:");
  for (const k of ls.skills||[]) console.log("  ["+(k.published?"published":"draft")+"] "+k.name+"  ("+k.path+")  params="+(k.parameters||[]).length);

  const pl = await rpc("prompts/list", {});
  const prompts = pl.result?.prompts || [];
  console.log("\nprompts/list -> "+prompts.length+" prompts (slash commands):");
  for (const p of prompts) console.log("  /"+p.name+"  args=["+(p.arguments||[]).map(a=>a.name).join(", ")+"]");

  // 3) sanity: expand one prompt with arguments
  const pg = await rpc("prompts/get", { name: "create-animated-character", arguments: { name: "Robot", color: "steel blue", height: "2.2" } });
  const txt = pg.result?.messages?.[0]?.content?.text || "";
  console.log("\nprompts/get create-animated-character (substituted): "+txt.length+" chars, title -> "+txt.split("\n")[0]);
  console.log("  substitution check:", txt.includes("Robot") && txt.includes("steel blue") && txt.includes("2.2") ? "OK ({{name}}/{{color}}/{{height}} replaced)" : "tokens NOT replaced");
})();

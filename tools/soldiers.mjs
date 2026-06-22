import http from 'node:http';
import fs from 'node:fs';

const PORT = 13880;

function rpc(method, params) {
  const body = JSON.stringify({ jsonrpc: '2.0', id: Date.now() % 100000, method, params });
  return new Promise((resolve, reject) => {
    const req = http.request(
      { host: '127.0.0.1', port: PORT, path: '/', method: 'POST',
        headers: { 'Content-Type': 'application/json', 'Content-Length': Buffer.byteLength(body) } },
      (res) => {
        let data = '';
        res.setEncoding('utf8');
        res.on('data', (c) => (data += c));
        res.on('end', () => {
          try { resolve(JSON.parse(data)); } catch (e) { reject(new Error('Bad JSON: ' + data.slice(0, 200))); }
        });
      });
    req.on('error', reject);
    req.write(body);
    req.end();
  });
}

// inspect_instance returns structuredContent directly.
async function inspect(ref, maxDepth) {
  const args = /^\{.*\}$/.test(ref) ? { guid: ref } : { path: ref };
  if (maxDepth != null) args.maxDepth = maxDepth;
  const r = await rpc('tools/call', { name: 'inspect_instance', arguments: args });
  if (r.error) throw new Error('RPC error: ' + JSON.stringify(r.error));
  const res = r.result;
  if (res.isError) throw new Error('Tool error: ' + (res.content?.[0]?.text || '?'));
  return res.structuredContent;
}

const memberByName = (node, name) => (node.members || []).find((m) => m.name === name);

function getChildren() {
  return inspect('Scenes/Soldiers').then((inst) => {
    const group = memberByName(inst, 'components').elements[0];
    return memberByName(group, 'entityData').elements;
  });
}

// Turn one inspected ExternalEntityData element into a lossless $type spec.
function toSpec(el) {
  const id = memberByName(el, 'id').value;
  const name = memberByName(el, 'name').value;
  const st = memberByName(el, 'state');
  const tr = memberByName(el, 'transform');
  const translation = memberByName(tr, 'translation').value;
  const rotation = memberByName(tr, 'rotation').value;
  const entityData = memberByName(el, 'entityData').value;
  return {
    $type: 'traktor.world.ExternalEntityData',
    set: {
      id,
      name,
      'state.visible': memberByName(st, 'visible').value,
      'state.dynamic': memberByName(st, 'dynamic').value,
      'state.locked': memberByName(st, 'locked').value,
      'transform.translation': translation,
      'transform.rotation': rotation,
      entityData,
    },
  };
}

async function callTool(name, args) {
  const r = await rpc('tools/call', { name, arguments: args });
  if (r.error) throw new Error('RPC error: ' + JSON.stringify(r.error));
  if (r.result.isError) throw new Error('Tool error: ' + (r.result.content?.[0]?.text || '?'));
  return r.result.structuredContent ?? JSON.parse(r.result.content[0].text);
}

const cmd = process.argv[2] || 'analyze';

if (cmd === 'inspect') {
  const ref = process.argv[3];
  const depth = process.argv[4] ? parseInt(process.argv[4]) : 4;
  const args = { maxDepth: depth };
  if (/^\{.*\}$/.test(ref)) args.guid = ref; else args.path = ref;
  const r = await rpc('tools/call', { name: 'inspect_instance', arguments: args });
  if (r.error) throw new Error(JSON.stringify(r.error));
  console.log(JSON.stringify(r.result.structuredContent ?? JSON.parse(r.result.content[0].text), null, 2));
  process.exit(0);
}

if (cmd === 'list') {
  const args = {};
  if (process.argv[3]) args.typeName = process.argv[3];
  if (process.argv[4]) args.groupPath = process.argv[4];
  const sc = await callTool('list_instances', args);
  console.log(JSON.stringify(sc, null, 2));
  process.exit(0);
}

if (cmd === 'nodetypes') {
  const args = {};
  if (process.argv[3]) args.filter = process.argv[3];
  const sc = await callTool('list_shader_node_types', args);
  console.log(JSON.stringify(sc, null, 2));
  process.exit(0);
}

if (cmd === 'describe') {
  const sc = await callTool('describe_node', { type: process.argv[3] });
  console.log(JSON.stringify(sc, null, 2));
  process.exit(0);
}

// Generic: convert an inspected object node into a {$type, set:{...}} spec (lossless for
// primitive/enum leaves, compound (flattened with dotted paths), nested objects and
// object-element arrays — which is exactly what entity/component data is made of).
function specFromInspected(node) {
  const set = {};
  const walk = (m, prefix) => {
    const path = prefix + m.name;
    if (m.kind === 'primitive' || m.kind === 'enum') set[path] = m.value;
    else if (m.kind === 'compound') (m.members || []).forEach((s) => walk(s, path + '.'));
    else if (m.kind === 'object') set[path] = (m.value === null) ? null : specFromInspected(m);
    else if (m.kind === 'array') set[path] = (m.elements || []).map(specFromInspected);
  };
  (node.members || []).forEach((m) => walk(m, ''));
  return { $type: node.type, set };
}

// Worm animation script (shared by all 6 segments; per-segment phase from the entity name).
const WORM_LUA = [
  '-- CubeRotate: one worm segment. Orbits the world origin (XZ), bobs vertically',
  '-- (sine), and tumbles. Six instances form a worm via a per-segment time delay',
  '-- parsed from the entity name\'s trailing number (e.g. "WormCube3" -> segment 3;',
  '-- a name with no number is the head, segment 0). The whole animation is',
  '-- deterministic, so every segment does the SAME motion, just delayed.',
  '-- Inherits the native ScriptComponent so "self" exposes .owner (the Entity).',
  '',
  'CubeRotate = CubeRotate or class("CubeRotate", traktor.world.ScriptComponent)',
  '',
  'function CubeRotate:new()',
  '\tself.bobAmplitude = 5.0     -- units (vertical)',
  '\tself.bobFrequency = 2.0     -- radians/sec (~3.1s period)',
  '\tself.orbitRadius  = 5.0     -- units, around world origin',
  '\tself.orbitSpeed   = 1.0     -- radians/sec (~6.3s per revolution)',
  '\tself.spin = traktor.Vector4(1.3, 0.9, 1.7, 0)  -- tumble rates (rad/sec) per axis',
  '\tself.segmentDelay = 0.3     -- seconds of trail per segment',
  '\tself.phase = nil            -- resolved lazily from the entity name',
  'end',
  '',
  'function CubeRotate:update(context, totalTime, deltaTime)',
  '\tlocal entity = self.owner',
  '\tif entity == nil then return end',
  '\tif self.phase == nil then',
  '\t\tlocal index = tonumber(string.match(entity.name, "%d+")) or 0',
  '\t\tself.phase = index * self.segmentDelay',
  '\tend',
  '\tlocal t = totalTime - self.phase',
  '\t-- Orbit world origin (XZ) + vertical sine bob.',
  '\tlocal a = t * self.orbitSpeed',
  '\tlocal px = self.orbitRadius * math.cos(a)',
  '\tlocal pz = self.orbitRadius * math.sin(a)',
  '\tlocal py = self.bobAmplitude * math.sin(t * self.bobFrequency)',
  '\tlocal position = traktor.Vector4(px, py, pz, 0)',
  '\t-- Tumble.',
  '\tlocal qx = traktor.Quaternion(traktor.Vector4(1, 0, 0, 0), self.spin.x * t)',
  '\tlocal qy = traktor.Quaternion(traktor.Vector4(0, 1, 0, 0), self.spin.y * t)',
  '\tlocal qz = traktor.Quaternion(traktor.Vector4(0, 0, 1, 0), self.spin.z * t)',
  '\tlocal rotation = qx:concat(qy):concat(qz)',
  '\tentity.transform = traktor.Transform(position, rotation)',
  'end',
  '',
].join('\n');

// Worm script v2 — per-segment phase read from the script component's "properties".
const WORM_LUA_PROPS = [
  '-- CubeRotate: one worm segment. Orbits the world origin (XZ), bobs vertically',
  '-- (sine), and tumbles. Six instances form a worm via a per-segment phase offset',
  '-- (seconds) read from the script component\'s "properties" ("phase" float). The',
  '-- whole animation is deterministic, so every segment does the SAME motion, just',
  '-- delayed. Inherits the native ScriptComponent so "self" exposes .owner and',
  '-- .properties.',
  '',
  'CubeRotate = CubeRotate or class("CubeRotate", traktor.world.ScriptComponent)',
  '',
  'function CubeRotate:new()',
  '\tself.bobAmplitude = 5.0     -- units (vertical)',
  '\tself.bobFrequency = 2.0     -- radians/sec (~3.1s period)',
  '\tself.orbitRadius  = 5.0     -- units, around world origin',
  '\tself.orbitSpeed   = 1.0     -- radians/sec (~6.3s per revolution)',
  '\tself.spin = traktor.Vector4(3.9, 2.7, 5.1, 0)  -- tumble rates (rad/sec) per axis',
  '\tself.phase = nil            -- resolved lazily from "properties"',
  'end',
  '',
  'function CubeRotate:update(context, totalTime, deltaTime)',
  '\tlocal entity = self.owner',
  '\tif entity == nil then return end',
  '\tif self.phase == nil then',
  '\t\tself.phase = (self.properties ~= nil and self.properties:getProperty("phase")) or 0',
  '\tend',
  '\tlocal t = totalTime - self.phase',
  '\t-- Orbit world origin (XZ) + vertical sine bob.',
  '\tlocal a = t * self.orbitSpeed',
  '\tlocal px = self.orbitRadius * math.cos(a)',
  '\tlocal pz = self.orbitRadius * math.sin(a)',
  '\tlocal py = self.bobAmplitude * math.sin(t * self.bobFrequency)',
  '\tlocal position = traktor.Vector4(px, py, pz, 0)',
  '\t-- Tumble.',
  '\tlocal qx = traktor.Quaternion(traktor.Vector4(1, 0, 0, 0), self.spin.x * t)',
  '\tlocal qy = traktor.Quaternion(traktor.Vector4(0, 1, 0, 0), self.spin.y * t)',
  '\tlocal qz = traktor.Quaternion(traktor.Vector4(0, 0, 1, 0), self.spin.z * t)',
  '\tlocal rotation = qx:concat(qy):concat(qz)',
  '\tentity.transform = traktor.Transform(position, rotation)',
  'end',
  '',
].join('\n');

if (cmd === 'update-script') {
  const SCRIPT = '{B9E5E20E-4325-9245-9C97-7D6176FE6BC5}';
  await callTool('set_instance_member', { guid: SCRIPT, member: 'text', value: WORM_LUA_PROPS });
  const t = memberByName(await inspect(SCRIPT, 1), 'text').value;
  console.log('script updated | spin line:', (t.match(/self\.spin = traktor\.Vector4\([^)]*\)/) || [])[0]);
  process.exit(0);
}

if (cmd === 'worm-props') {
  const MESH = '{6ECB10E8-4B5A-9D44-853D-3334F2791782}';
  const SCRIPT = '{B9E5E20E-4325-9245-9C97-7D6176FE6BC5}';
  const DELAY = 0.3; // seconds of trail per segment

  // 1) Switch the script to read phase from "properties".
  await callTool('set_instance_member', { guid: SCRIPT, member: 'text', value: WORM_LUA_PROPS });
  console.log('1) script now reads phase from properties');

  // 2) Write a "phase" float property into each segment's ScriptComponentData.
  //    A PropertyGroup map entry is built in one components set: create an empty
  //    pair, then set its key (first) and value object (second).
  const scriptSpec = (phase) => ({ $type: 'traktor.world.ScriptComponentData', set: {
    class: SCRIPT,
    editorSupport: true,
    properties: { $type: 'traktor.PropertyGroup', set: { value: [{ $type: 'traktor.PropertyFloat' }] } },
    'properties.value[0].first': 'phase',
    'properties.value[0].second': { $type: 'traktor.PropertyFloat', set: { value: phase } },
  } });
  const segments = [{ path: 'Entities/Cube/Cube', i: 0 }];
  for (let i = 1; i <= 5; i++) segments.push({ path: 'Entities/Cube/WormCube' + i, i });

  for (const s of segments) {
    await callTool('set_instance_member', { path: s.path, member: 'components', value: [
      { $type: 'traktor.mesh.MeshComponentData', set: { mesh: MESH } },
      scriptSpec(s.i * DELAY),
    ]});
  }
  console.log('2) phase property written to', segments.length, 'segments (delay ' + DELAY + 's)');

  // 3) Verify each segment stores the expected phase.
  console.log('--- verify ---');
  for (const s of segments) {
    const e = await inspect(s.path, 6);
    const sc = memberByName(e, 'components').elements.find((c) => c.type === 'traktor.world.ScriptComponentData');
    const entry = memberByName(sc, 'properties').members ? memberByName(memberByName(sc, 'properties'), 'value').elements[0] : null;
    const key = entry && memberByName(entry, 'first').value;
    const val = entry && memberByName(memberByName(entry, 'second'), 'value').value;
    console.log(`  ${e.name}: "${key}" = ${val} (expect ${s.i * DELAY})`);
  }
  process.exit(0);
}

if (cmd === 'worm') {
  const MESH = '{6ECB10E8-4B5A-9D44-853D-3334F2791782}';   // Entities/Cube/M - Cube (green)
  const SCRIPT = '{B9E5E20E-4325-9245-9C97-7D6176FE6BC5}';  // CubeRotate
  const SCENE = '{FF0A61F5-30AD-4647-A89F-37700B557CD4}';   // Scenes/Many Characters
  const newGuid = () => '{' + crypto.randomUUID().toUpperCase() + '}';

  // 1) Upgrade the shared script to the worm animation.
  await callTool('set_instance_member', { guid: SCRIPT, member: 'text', value: WORM_LUA });
  console.log('1) script upgraded to worm animation');

  // 2) Ensure 5 trailing segment entities (the existing "Cube" stays as the head = segment 0).
  async function ensureEntity(path, name) {
    let guid;
    try { guid = (await inspect(path, 1)).guid; }
    catch (_) { guid = (await callTool('create_instance', { path, type: 'traktor.world.EntityData' })).guid; }
    await callTool('set_instance_member', { guid, member: 'name', value: name });
    await callTool('set_instance_member', { guid, member: 'id', value: newGuid() });
    await callTool('set_instance_member', { guid, member: 'components', value: [
      { $type: 'traktor.mesh.MeshComponentData', set: { mesh: MESH } },
      { $type: 'traktor.world.ScriptComponentData', set: { class: SCRIPT, editorSupport: true } },
    ]});
    return guid;
  }
  const segments = [];
  for (let i = 1; i <= 5; i++) {
    const name = 'WormCube' + i;
    const guid = await ensureEntity('Entities/Cube/' + name, name);
    segments.push({ name, guid });
  }
  console.log('2) segment entities:', segments.map((s) => s.name + ' ' + s.guid).join(', '));

  // 3) Place segments in the scene's Entity layer, preserving existing entities.
  const scene = await inspect(SCENE, 12);
  const layers = memberByName(scene, 'layers').elements;
  const idx = layers.findIndex((l) => (memberByName(l, 'name') || {}).value === 'Entity');
  const group = memberByName(layers[idx], 'components').elements[0];
  const kids = memberByName(group, 'entityData').elements;
  const keep = kids.filter((k) => !/^WormCube\d+$/.test(memberByName(k, 'name')?.value || '')).map(specFromInspected);
  const refs = segments.map((s) => ({ $type: 'traktor.world.ExternalEntityData', set: {
    id: newGuid(), name: s.name, entityData: s.guid,
    'state.visible': true, 'state.dynamic': false, 'state.locked': false,
    'transform.translation': [0, 0, 0, 0], 'transform.rotation': [0, 0, 0, 1],
  } }));
  await callTool('set_instance_member', { guid: SCENE, member: `layers[${idx}].components[0].entityData`, value: [...keep, ...refs] });

  // 4) Verify.
  const after = await inspect(SCENE, 12);
  const ak = memberByName(memberByName(memberByName(after, 'layers').elements[idx], 'components').elements[0], 'entityData').elements;
  const names = ak.map((k) => memberByName(k, 'name').value);
  const cubes = names.filter((n) => n === 'Cube' || /^WormCube\d+$/.test(n));
  console.log('--- verify ---');
  console.log('layer kids:', names.join(', '));
  console.log('worm cubes (head + segments):', cubes.length, '->', cubes.join(', '));
  for (const s of segments) {
    const e = await inspect(s.guid, 3);
    const comps = memberByName(e, 'components').elements.map((c) => c.type);
    console.log('  ' + memberByName(e, 'name').value + ': ' + comps.join(', '));
  }
  process.exit(0);
}

if (cmd === 'fix-rotate') {
  const SCRIPT = '{B9E5E20E-4325-9245-9C97-7D6176FE6BC5}'; // Entities/Cube/CubeRotate
  const lua = [
    '-- CubeRotate: tumble the owning entity at random per-axis speeds, orbit the',
    '-- world origin in the XZ plane, and bob vertically (sine). Inherits the native',
    '-- ScriptComponent so "self" exposes .owner (the Entity).',
    '',
    'CubeRotate = CubeRotate or class("CubeRotate", traktor.world.ScriptComponent)',
    '',
    'function CubeRotate:new()',
    '\tlocal function spin()',
    '\t\tlocal s = 0.5 + math.random() * 1.5',
    '\t\tif math.random() < 0.5 then s = -s end',
    '\t\treturn s',
    '\tend',
    '\tself.sx = spin()',
    '\tself.sy = spin()',
    '\tself.sz = spin()',
    '\tself.bobAmplitude = 5.0   -- units (vertical)',
    '\tself.bobFrequency = 2.0   -- radians/sec (~3.1s period)',
    '\tself.orbitRadius = 5.0    -- units, around world origin',
    '\tself.orbitSpeed = 1.0     -- radians/sec (~6.3s per revolution)',
    'end',
    '',
    'function CubeRotate:update(context, totalTime, deltaTime)',
    '\tlocal entity = self.owner',
    '\tif entity == nil then return end',
    '\t-- Orbit the world origin in the XZ plane + vertical sine bob.',
    '\tlocal a = totalTime * self.orbitSpeed',
    '\tlocal px = self.orbitRadius * math.cos(a)',
    '\tlocal pz = self.orbitRadius * math.sin(a)',
    '\tlocal py = self.bobAmplitude * math.sin(totalTime * self.bobFrequency)',
    '\tlocal position = traktor.Vector4(px, py, pz, 0)',
    '\t-- Random tumble.',
    '\tlocal qx = traktor.Quaternion(traktor.Vector4(1, 0, 0, 0), self.sx * totalTime)',
    '\tlocal qy = traktor.Quaternion(traktor.Vector4(0, 1, 0, 0), self.sy * totalTime)',
    '\tlocal qz = traktor.Quaternion(traktor.Vector4(0, 0, 1, 0), self.sz * totalTime)',
    '\tlocal rotation = qx:concat(qy):concat(qz)',
    '\tentity.transform = traktor.Transform(position, rotation)',
    'end',
    '',
  ].join('\n');

  await callTool('set_instance_member', { guid: SCRIPT, member: 'text', value: lua });
  const s = await inspect(SCRIPT, 1);
  const txt = memberByName(s, 'text').value;
  console.log('updated', s.name, '| lines:', txt.split('\n').length);
  console.log('inherits ScriptComponent:', /class\("CubeRotate",\s*traktor\.world\.ScriptComponent\)/.test(txt));
  console.log('new() no args           :', /function CubeRotate:new\(\)/.test(txt));
  console.log('owner access            :', /self\.owner/.test(txt));
  console.log('update 3-arg            :', /:update\(context, totalTime, deltaTime\)/.test(txt));
  process.exit(0);
}

if (cmd === 'cube-rotate') {
  const ENT = '{AD4B4CA6-1125-5B43-AD55-700985D82A1B}'; // Entities/Cube/Cube
  const scriptPath = 'Entities/Cube/CubeRotate';          // instance name MUST equal the lua class name
  const lua = [
    'CubeRotate = CubeRotate or class("CubeRotate")',
    '',
    'function CubeRotate:new(component)',
    '\tself.entity = component.owner',
    '\tself.origin = self.entity.transform.translation',
    '',
    '\t-- Random angular velocity (radians/sec) per axis, biased away from zero.',
    '\tlocal function spin()',
    '\t\tlocal s = 0.5 + math.random() * 1.5',
    '\t\tif math.random() < 0.5 then s = -s end',
    '\t\treturn s',
    '\tend',
    '\tself.sx = spin()',
    '\tself.sy = spin()',
    '\tself.sz = spin()',
    'end',
    '',
    'function CubeRotate:update(context, totalTime, deltaTime)',
    '\tlocal qx = traktor.Quaternion(traktor.Vector4(1, 0, 0, 0), self.sx * totalTime)',
    '\tlocal qy = traktor.Quaternion(traktor.Vector4(0, 1, 0, 0), self.sy * totalTime)',
    '\tlocal qz = traktor.Quaternion(traktor.Vector4(0, 0, 1, 0), self.sz * totalTime)',
    '\tlocal rotation = qx:concat(qy):concat(qz)',
    '\tself.entity.transform = traktor.Transform(self.origin, rotation)',
    'end',
    '',
  ].join('\n');

  // 1) Create the lua Script instance (named CubeRotate so it binds as the IRuntimeClass).
  const script = await callTool('create_instance', { path: scriptPath, type: 'traktor.script.Script' });
  console.log('1) Script created:', scriptPath, script.guid);
  await callTool('set_instance_member', { guid: script.guid, member: 'text', value: lua });

  // 2) Add a ScriptComponentData to the cube, preserving the existing MeshComponentData.
  const ent = await inspect(ENT, 3);
  const comps = memberByName(ent, 'components').elements;
  console.log('2) existing components:', comps.map((c) => c.type).join(', '));
  const specs = comps.map(specFromInspected);
  specs.push({ $type: 'traktor.world.ScriptComponentData', set: { class: script.guid, editorSupport: true } });
  await callTool('set_instance_member', { guid: ENT, member: 'components', value: specs });

  // 3) Verify.
  console.log('--- verify ---');
  const s = await inspect(script.guid, 1);
  const txt = memberByName(s, 'text').value;
  console.log('script name:', s.name, '| text lines:', txt.split('\n').length, '| has class+update:', /class\("CubeRotate"\)/.test(txt) && /:update\(/.test(txt));
  const e = await inspect(ENT, 3);
  const ec = memberByName(e, 'components').elements;
  const sc = ec.find((c) => c.type === 'traktor.world.ScriptComponentData');
  console.log('entity components:', ec.map((c) => c.type).join(', '));
  console.log('ScriptComponentData.class ->', sc && memberByName(sc, 'class').value, '(expect', script.guid + ')');
  console.log('ScriptComponentData.editorSupport ->', sc && memberByName(sc, 'editorSupport').value);
  process.exit(0);
}

if (cmd === 'face') {
  // Cube sits at the scene origin and the Soldiers group is referenced at the scene
  // origin too, so in each soldier's local space the cube is at (0,0,0).
  const target = [0, 0, 0];
  const offDeg = parseFloat(process.argv[3] || '0'); // yaw offset if the model's forward isn't +Z
  const off = offDeg * Math.PI / 180;

  const els = await getChildren();
  const yawOf = (p) => Math.atan2(target[0] - p[0], target[2] - p[2]) + off; // forward=+Z, lookAt convention
  const specs = els.map((el) => {
    const p = memberByName(memberByName(el, 'transform'), 'translation').value;
    const th = yawOf(p);
    const spec = toSpec(el);                          // preserves id/name/state/translation/ref
    spec.set['transform.rotation'] = [0, Math.sin(th / 2), 0, Math.cos(th / 2)];
    return spec;
  });
  await callTool('set_instance_member', { path: 'Scenes/Soldiers', member: 'components[0].entityData', value: specs });

  // Verify: derive each committed forward and measure angle to the direction-to-cube.
  const after = await getChildren();
  let maxFaceErr = 0;
  for (const el of after) {
    const p = memberByName(memberByName(el, 'transform'), 'translation').value;
    const q = memberByName(memberByName(el, 'transform'), 'rotation').value;
    const th = 2 * Math.atan2(q[1], q[3]);
    const fwd = [Math.sin(th), Math.cos(th)];
    const d = Math.hypot(target[0] - p[0], target[2] - p[2]);
    const des = [(target[0] - p[0]) / d, (target[2] - p[2]) / d];
    const dot = Math.max(-1, Math.min(1, fwd[0] * des[0] + fwd[1] * des[1]));
    maxFaceErr = Math.max(maxFaceErr, Math.acos(dot) * 180 / Math.PI);
  }
  console.log('soldiers oriented:', after.length, '| yaw offset:', offDeg + '°');
  console.log('max angle between forward and direction-to-cube:', maxFaceErr.toFixed(3) + '°', '(expect ~' + Math.abs(offDeg) + '°)');
  const sample = (i) => {
    const p = memberByName(memberByName(after[i], 'transform'), 'translation').value;
    const q = memberByName(memberByName(after[i], 'transform'), 'rotation').value;
    return `pos(${p[0].toFixed(1)},${p[2].toFixed(1)}) yaw=${(2 * Math.atan2(q[1], q[3]) * 180 / Math.PI).toFixed(1)}°`;
  };
  console.log('samples:', [0, 10, 20, 30].map(sample).join(' | '));
  process.exit(0);
}

if (cmd === 'cube-material') {
  const MESH = '{6ECB10E8-4B5A-9D44-853D-3334F2791782}'; // Entities/Cube/M - Cube
  const SLOT = process.argv[3] || 'MAT_Cube';
  const shaderPath = 'Entities/Cube/MAT_Cube';

  // Reuse the known-valid static green surface fragment captured in the backup.
  const src = JSON.parse(fs.readFileSync('D:/Temp/slask/visor-shader-backup.json', 'utf8'));
  const nodes = src.nodes.map((n) => {
    const { guid, info, fragmentName, deprecated, ...keep } = n; // drop node guid -> regenerate fresh
    if (keep.type === 'Color') keep.color = [0, 1, 0, 1];        // force green albedo
    return keep;
  });
  const graph = { nodes, edges: src.edges };

  const created = await callTool('create_shader_graph', { path: shaderPath, graph });
  console.log('shader created:', shaderPath, created.guid, '| valid:', created.validation?.valid, '| committed:', created.committed);
  if (!created.committed) { console.log('NOT committed:', JSON.stringify(created.validation?.errorNodes), created.message); process.exit(2); }

  // Bind the green shader to the MAT_Cube material slot of the cube mesh asset.
  const bound = await callTool('set_mesh_material_shader', { guid: MESH, material: SLOT, shader: created.guid });
  console.log('bound material:', SLOT, '->', created.guid);

  // Verify.
  console.log('--- verify ---');
  const mesh = await callTool('get_mesh_asset', { guid: MESH });
  console.log('mesh materialShaders:', JSON.stringify(mesh.materialShaders));
  const g = await callTool('get_shader_graph', { guid: created.guid });
  const cn = g.nodes.find((n) => n.type === 'Color');
  const ae = g.edges.find((e) => e.to.pin === 'Albedo');
  console.log('shader graphType:', g.graphType, '| Color:', JSON.stringify(cn.color), '| info:', cn.info, '| Albedo fed by:', ae && ae.from.node);
  process.exit(0);
}

if (cmd === 'cube-place') {
  const entGuid = process.argv[3] || '{AD4B4CA6-1125-5B43-AD55-700985D82A1B}';
  const SCENE = '{FF0A61F5-30AD-4647-A89F-37700B557CD4}';
  const newGuid = () => '{' + crypto.randomUUID().toUpperCase() + '}';

  const scene = await inspect(SCENE, 10);
  const layers = memberByName(scene, 'layers').elements;
  let idx = layers.findIndex((l) => (memberByName(l, 'name') || {}).value === 'Entity');
  if (idx < 0) idx = layers.length - 1;
  const group = memberByName(layers[idx], 'components').elements[0];
  const kids = memberByName(group, 'entityData').elements;
  console.log(`layer[${idx}] "${memberByName(layers[idx], 'name').value}" kids:`, kids.map((k) => memberByName(k, 'name')?.value).join(', '));

  // Rebuild existing kids faithfully (drop any prior "Cube" so re-runs are idempotent), append Cube.
  const specs = kids.filter((k) => memberByName(k, 'name')?.value !== 'Cube').map(specFromInspected);
  const cubeRef = { $type: 'traktor.world.ExternalEntityData', set: {
    id: newGuid(),
    name: 'Cube',
    entityData: entGuid,
    'state.visible': true, 'state.dynamic': false, 'state.locked': false,
    'transform.translation': [0, 0, 0, 0],
    'transform.rotation': [0, 0, 0, 1],
  } };
  await callTool('set_instance_member', { guid: SCENE, member: `layers[${idx}].components[0].entityData`, value: [...specs, cubeRef] });

  // Verify: existing entities preserved (id/name/ref/camera fov) + Cube added.
  const after = await inspect(SCENE, 10);
  const ak = memberByName(memberByName(memberByName(after, 'layers').elements[idx], 'components').elements[0], 'entityData').elements;
  const byName = (arr, n) => arr.find((k) => memberByName(k, 'name')?.value === n);
  const cam = byName(ak, 'Camera0'), sol = byName(ak, 'Soldiers'), cube = byName(ak, 'Cube');
  const camFov = cam && memberByName(memberByName(cam, 'components').elements[0], 'fov').value;
  console.log('--- verify scene placement ---');
  console.log('kids after        :', ak.length, '->', ak.map((k) => memberByName(k, 'name').value).join(', '));
  console.log('Camera0 preserved :', !!cam, '| id', cam && memberByName(cam, 'id').value, '| fov', camFov);
  console.log('Soldiers preserved:', !!sol, '| ref', sol && memberByName(sol, 'entityData').value);
  console.log('Cube added        :', !!cube, '| ref', cube && memberByName(cube, 'entityData').value, '(expect', entGuid + ')');
  process.exit(0);
}

if (cmd === 'cube-setup') {
  const blendPath = process.argv[3] || 'Cube.blend';     // relative to Pipeline.AssetPath (data/Assets)
  const SCENE = '{FF0A61F5-30AD-4647-A89F-37700B557CD4}'; // Scenes/Many Characters
  const newGuid = () => '{' + crypto.randomUUID().toUpperCase() + '}';
  const set = (guid, member, value) => callTool('set_instance_member', { guid, member, value });

  // 1) Mesh asset that imports the blend file.
  const meshPath = 'Entities/Cube/M - Cube';
  const mesh = await callTool('create_instance', { path: meshPath, type: 'traktor.mesh.MeshAsset' });
  console.log('1) MeshAsset created:', meshPath, mesh.guid);
  await set(mesh.guid, 'fileName', blendPath);
  await set(mesh.guid, 'meshType', 'MtStatic');

  // 2) Entity that renders the mesh via a static MeshComponentData.
  const entPath = 'Entities/Cube/Cube';
  const ent = await callTool('create_instance', { path: entPath, type: 'traktor.world.EntityData' });
  console.log('2) Entity created  :', entPath, ent.guid);
  await set(ent.guid, 'name', 'Cube');
  await set(ent.guid, 'id', newGuid());
  await set(ent.guid, 'components', [
    { $type: 'traktor.mesh.MeshComponentData', set: { mesh: mesh.guid } },
  ]);

  // 3) Reference the entity from the scene's "Entity" layer (external reference, like the soldiers).
  const scene = await inspect(SCENE, 3);
  const layers = memberByName(scene, 'layers').elements;
  let layerIdx = layers.findIndex((l) => (memberByName(l, 'name') || {}).value === 'Entity');
  if (layerIdx < 0) layerIdx = layers.length - 1;
  const layerName = memberByName(layers[layerIdx], 'name').value;
  const existing = memberByName(memberByName(layers[layerIdx], 'components').elements[0], 'entityData').elements;
  console.log(`3) Placing into layer[${layerIdx}] "${layerName}" (had ${existing.length} entities)`);

  // Preserve any existing entities in that layer, append the Cube reference.
  const keep = existing.map((el) => {
    const tr = memberByName(el, 'transform');
    return { $type: el.type, set: {
      id: memberByName(el, 'id').value,
      name: memberByName(el, 'name').value,
      entityData: memberByName(el, 'entityData').value,
      'transform.translation': memberByName(tr, 'translation').value,
      'transform.rotation': memberByName(tr, 'rotation').value,
    } };
  });
  const cubeRef = { $type: 'traktor.world.ExternalEntityData', set: {
    id: newGuid(),
    name: 'Cube',
    entityData: ent.guid,
    'state.visible': true, 'state.dynamic': false, 'state.locked': false,
    'transform.translation': [0, 0, 0, 0],
    'transform.rotation': [0, 0, 0, 1],
  } };
  await set(SCENE, `layers[${layerIdx}].components[0].entityData`, [...keep, cubeRef]);

  // 4) Verify everything round-tripped.
  console.log('--- verify ---');
  const m = await inspect(mesh.guid, 2);
  console.log('MeshAsset fileName :', memberByName(m, 'fileName').value, '| meshType:', memberByName(m, 'meshType').value);
  const e = await inspect(ent.guid, 3);
  const comp = memberByName(e, 'components').elements[0];
  console.log('Entity name        :', memberByName(e, 'name').value);
  console.log('Entity component   :', comp.type, '| mesh ->', memberByName(comp, 'mesh').value, '(expect', mesh.guid + ')');
  const scene2 = await inspect(SCENE, 4);
  const grp = memberByName(memberByName(scene2, 'layers').elements[layerIdx], 'components').elements[0];
  const kids = memberByName(grp, 'entityData').elements;
  const cube = kids.find((k) => memberByName(k, 'name')?.value === 'Cube');
  console.log('Scene layer kids   :', kids.length, '| Cube present:', !!cube,
    cube ? '| references -> ' + memberByName(cube, 'entityData').value : '');
  process.exit(0);
}

if (cmd === 'visor-pulse') {
  const SHADER = '{93EA9BD6-F7B4-E444-9772-1A9EFD838BBE}';
  const speed = parseFloat(process.argv[3] || '3'); // rad/s; period = 2*pi/speed (~2.1s at 3)
  const force = process.argv.includes('--force');

  const g = await callTool('get_shader_graph', { guid: SHADER });
  fs.writeFileSync('D:/Temp/slask/visor-shader-backup.json', JSON.stringify(g, null, 2));
  console.log('backed up current shader IR ->', 'visor-shader-backup.json', '(' + g.nodes.length + ' nodes)');

  // Sanity: the albedo is currently driven by Parameter n3 fed by Color n4.
  const albedoEdge = g.edges.find((e) => e.to.node === 'n2' && e.to.pin === 'Albedo');
  console.log('current Albedo source:', albedoEdge ? albedoEdge.from.node + '.' + albedoEdge.from.pin : '(none)');

  // Drop the old constant-color albedo source and any edges touching it.
  const drop = new Set(['n3', 'n4']);
  const nodes = g.nodes.filter((n) => !drop.has(n.id));
  const edges = g.edges.filter((e) => !drop.has(e.from.node) && !drop.has(e.to.node));

  // Time-driven pulse: t = sin(World_Time * speed) * 0.5 + 0.5 ; Albedo = lerp(green, red, t).
  nodes.push(
    { id: 'pU', type: 'Uniform', position: [-960, 700], properties: { parameterName: 'World_Time' } },
    { id: 'pSpeed', type: 'Scalar', position: [-960, 800], properties: { value: speed } },
    { id: 'pHalf', type: 'Scalar', position: [-760, 880], properties: { value: 0.5 } },
    { id: 'pMul', type: 'Mul', position: [-760, 710] },
    { id: 'pSin', type: 'Sin', position: [-580, 710] },
    { id: 'pRemap', type: 'MulAdd', position: [-400, 760] },          // Input1*Input2 + Input3
    { id: 'pGreen', type: 'Color', position: [-400, 590], color: [0, 1, 0, 1], properties: { linear: true } },
    { id: 'pRed', type: 'Color', position: [-400, 650], color: [1, 0, 0, 1], properties: { linear: true } },
    { id: 'pLerp', type: 'Lerp', position: [-160, 640] },
  );
  const E = (fn, fp, tn, tp) => ({ from: { node: fn, pin: fp }, to: { node: tn, pin: tp } });
  edges.push(
    E('pU', 'Output', 'pMul', 'Input1'),
    E('pSpeed', 'Output', 'pMul', 'Input2'),
    E('pMul', 'Output', 'pSin', 'Theta'),
    E('pSin', 'Output', 'pRemap', 'Input1'),
    E('pHalf', 'Output', 'pRemap', 'Input2'),
    E('pHalf', 'Output', 'pRemap', 'Input3'),
    E('pGreen', 'Output', 'pLerp', 'Input1'),                          // Blend=0 -> green
    E('pRed', 'Output', 'pLerp', 'Input2'),                            // Blend=1 -> red
    E('pRemap', 'Output', 'pLerp', 'Blend'),
    E('pLerp', 'Output', 'n2', 'Albedo'),
  );

  const args = { guid: SHADER, graph: { nodes, edges } };
  if (force) args.force = true;
  const sc = await callTool('update_shader_graph', args);
  console.log('validation.valid :', sc.validation?.valid, '| graphType:', sc.validation?.graphType, '| integrity:', sc.validation?.integrity);
  console.log('errorNodes       :', JSON.stringify(sc.validation?.errorNodes));
  console.log('warnings         :', JSON.stringify(sc.warnings));
  console.log('committed        :', sc.committed);
  if (!sc.committed) { console.log('NOT committed:', sc.message); process.exit(2); }

  // Verify the committed graph.
  const after = await callTool('get_shader_graph', { guid: SHADER });
  const ae = after.edges.find((e) => e.to.node === 'n2' && e.to.pin === 'Albedo');
  const u = after.nodes.find((n) => n.type === 'Uniform');
  const lerp = after.nodes.find((n) => n.type === 'Lerp');
  console.log('--- after ---');
  console.log('Albedo now fed by :', ae ? ae.from.node + '.' + ae.from.pin : '(none)');
  console.log('Uniform present   :', !!u, '| parameterName:', u?.properties?.parameterName);
  console.log('Lerp present      :', !!lerp, '| Lerp inputs fed:',
    after.edges.filter((e) => e.to.node === lerp?.id).map((e) => e.to.pin + '<-' + e.from.node).join(', '));
  process.exit(0);
}

if (cmd === 'visor-green') {
  const SHADER = '{93EA9BD6-F7B4-E444-9772-1A9EFD838BBE}';
  const ALBEDO_COLOR_NODE_ID = '{6D6E3E5B-EFA2-7F4E-8040-F35BAA88199E}'; // n4 -> Albedo
  const green = (process.argv[3] || '0,1,0,1').split(',').map(Number);

  // Locate the albedo Color node by id (robust to array order).
  const inst = await inspect(SHADER, 3);
  const nodes = memberByName(inst, 'nodes').elements;
  const idx = nodes.findIndex((n) => (memberByName(n, 'id') || {}).value === ALBEDO_COLOR_NODE_ID);
  if (idx < 0) throw new Error('albedo color node not found');
  const before = memberByName(nodes[idx], 'color').value;
  console.log('albedo Color node at nodes[' + idx + '], type', nodes[idx].type, '| before:', JSON.stringify(before));

  const r = await rpc('tools/call', {
    name: 'set_instance_member',
    arguments: { guid: SHADER, member: `nodes[${idx}].color`, value: green },
  });
  if (r.error) throw new Error('RPC error: ' + JSON.stringify(r.error));
  if (r.result.isError) throw new Error('Tool error: ' + (r.result.content?.[0]?.text || '?'));
  const sc = r.result.structuredContent;
  console.log('committed:', sc.committed, '| member:', sc.member);
  console.log('echoed new value:', JSON.stringify(sc.current?.value));

  // Verify via the shader-graph view that the albedo color is now green.
  const g = await callTool('get_shader_graph', { guid: SHADER });
  const cn = g.nodes.find((n) => n.guid === ALBEDO_COLOR_NODE_ID);
  const albedoEdge = g.edges.find((e) => e.to.pin === 'Albedo');
  console.log('shader Color node now:', JSON.stringify(cn.color), '| info:', cn.info);
  console.log('graph type/valid path intact, Albedo fed by node:', albedoEdge?.from.node);
  process.exit(0);
}

if (cmd === 'shader') {
  const ref = process.argv[3];
  const args = {};
  if (process.argv.includes('--resolve')) args.resolve = true;
  if (/^\{.*\}$/.test(ref)) args.guid = ref; else args.path = ref;
  const sc = await callTool('get_shader_graph', args);
  console.log(JSON.stringify(sc, null, 2));
  process.exit(0);
}

if (cmd === 'mesh') {
  const ref = process.argv[3];
  const args = {};
  if (/^\{.*\}$/.test(ref)) args.guid = ref; else args.path = ref;
  const sc = await callTool('get_mesh_asset', args);
  console.log(JSON.stringify(sc, null, 2));
  process.exit(0);
}

if (cmd === 'analyze') {
  const inst = await inspect('Scenes/Soldiers'); // default depth 8 — deep enough
  const components = memberByName(inst, 'components');
  const group = components.elements[0];           // GroupComponentData
  const entityData = memberByName(group, 'entityData');
  const els = entityData.elements;

  console.log('instance type :', inst.type);
  console.log('group type    :', group.type);
  console.log('child count   :', els.length);

  const types = {};
  let withComponents = 0;
  const xs = [], zs = [];
  for (const el of els) {
    types[el.type] = (types[el.type] || 0) + 1;
    const comps = memberByName(el, 'components');
    const compCount = comps && comps.elements ? comps.elements.length : 0;
    if (compCount > 0) withComponents++;
    const tr = memberByName(el, 'transform');
    const tl = memberByName(tr, 'translation').value;
    xs.push(tl[0]); zs.push(tl[2]);
  }
  console.log('type breakdown:', JSON.stringify(types));
  console.log('children with per-soldier component overrides:', withComponents);
  console.log('X range:', Math.min(...xs).toFixed(2), '->', Math.max(...xs).toFixed(2));
  console.log('Z range:', Math.min(...zs).toFixed(2), '->', Math.max(...zs).toFixed(2));

  // Full backup of the raw array (all members, all depth) for safe restore.
  fs.writeFileSync('D:/Temp/slask/soldiers-backup.json', JSON.stringify(entityData, null, 2));
  console.log('backup written: D:/Temp/slask/soldiers-backup.json (' + els.length + ' elements)');

  // Show first & last element member names so we confirm we capture everything.
  console.log('element[0] members:', (els[0].members || []).map((m) => m.name).join(', '));
}

if (cmd === 'square') {
  const h = parseFloat(process.argv[3] || '10');   // half-side; square spans -h..h
  const els = await getChildren();
  const n = els.length;
  const S = 2 * h, P = 4 * S;                        // side length, perimeter
  console.log('arranging', n, 'soldiers on a square perimeter, half-side', h, '(spans', -h, 'to', h + '), Y=0');

  const perimeterPoint = (d) => {                    // walk clockwise from corner (-h,-h)
    if (d < S) return [-h + d, -h];
    if (d < 2 * S) return [h, -h + (d - S)];
    if (d < 3 * S) return [h - (d - 2 * S), h];
    return [-h, h - (d - 3 * S)];
  };

  const specs = els.map((el, k) => {
    const [x, z] = perimeterPoint((k / n) * P);
    const spec = toSpec(el);                         // preserves id/name/state/rotation/ref
    spec.set['transform.translation'] = [x, 0, z, 0];
    return spec;
  });

  const r = await rpc('tools/call', {
    name: 'set_instance_member',
    arguments: { path: 'Scenes/Soldiers', member: 'components[0].entityData', value: specs },
  });
  if (r.error) throw new Error('RPC error: ' + JSON.stringify(r.error));
  if (r.result.isError) throw new Error('Tool error: ' + (r.result.content?.[0]?.text || '?'));
  console.log('committed:', r.result.structuredContent.committed);

  // Verify: every soldier on the boundary => max(|x|,|z|) == h.
  const after = await getChildren();
  let maxEdgeErr = 0, corners = 0;
  for (const el of after) {
    const p = memberByName(memberByName(el, 'transform'), 'translation').value;
    maxEdgeErr = Math.max(maxEdgeErr, Math.abs(Math.max(Math.abs(p[0]), Math.abs(p[2])) - h));
    if (Math.abs(Math.abs(p[0]) - h) < 1e-4 && Math.abs(Math.abs(p[2]) - h) < 1e-4) corners++;
  }
  console.log('count after     :', after.length);
  console.log('max edge error  :', maxEdgeErr.toExponential(2), '(should be ~0)');
  console.log('soldiers on corners:', corners);
  const show = (i) => JSON.stringify(memberByName(memberByName(after[i], 'transform'), 'translation').value.map((v) => +v.toFixed(2)));
  console.log('soldier[0]/[10]/[20]/[30]:', show(0), show(10), show(20), show(30));
}

if (cmd === 'circle') {
  const radius = parseFloat(process.argv[3] || '10');
  const els = await getChildren();
  const n = els.length;
  console.log('arranging', n, 'soldiers on a circle, radius', radius, 'centered at origin (XZ plane, Y=0)');

  const specs = els.map((el, k) => {
    const angle = (2 * Math.PI * k) / n;
    const x = radius * Math.cos(angle);
    const z = radius * Math.sin(angle);
    const spec = toSpec(el);                       // preserves id/name/state/rotation/ref
    spec.set['transform.translation'] = [x, 0, z, 0];
    return spec;
  });

  const r = await rpc('tools/call', {
    name: 'set_instance_member',
    arguments: { path: 'Scenes/Soldiers', member: 'components[0].entityData', value: specs },
  });
  if (r.error) throw new Error('RPC error: ' + JSON.stringify(r.error));
  if (r.result.isError) throw new Error('Tool error: ' + (r.result.content?.[0]?.text || '?'));
  console.log('committed:', r.result.structuredContent.committed);

  // Verify: every soldier should be at distance == radius from origin.
  const after = await getChildren();
  let maxErr = 0;
  for (const el of after) {
    const p = memberByName(memberByName(el, 'transform'), 'translation').value;
    const d = Math.hypot(p[0], p[2]);
    maxErr = Math.max(maxErr, Math.abs(d - radius));
  }
  console.log('count after:', after.length);
  console.log('max radius error:', maxErr.toExponential(2), '(should be ~0)');
  const p0 = memberByName(memberByName(after[0], 'transform'), 'translation').value;
  const pq = memberByName(memberByName(after[Math.floor(n / 4)], 'transform'), 'translation').value;
  console.log('soldier[0]   pos:', JSON.stringify(p0.map((v) => +v.toFixed(3))));
  console.log('soldier[n/4] pos:', JSON.stringify(pq.map((v) => +v.toFixed(3))));
}

if (cmd === 'verify') {
  const backup = JSON.parse(fs.readFileSync('D:/Temp/slask/soldiers-backup.json', 'utf8')).elements;
  const after = await getChildren();
  console.log('count after:', after.length, '| backup count:', backup.length);
  const stride = Math.floor(backup.length / after.length);
  const fld = (el, n) => memberByName(el, n).value;
  const pos = (el) => memberByName(memberByName(el, 'transform'), 'translation').value;
  let mism = 0;
  const xs = [], zs = [];
  for (let k = 0; k < after.length; k++) {
    const src = backup[k * stride], dst = after[k];
    const same = fld(src, 'id') === fld(dst, 'id')
      && fld(src, 'name') === fld(dst, 'name')
      && fld(src, 'entityData') === fld(dst, 'entityData')
      && JSON.stringify(pos(src)) === JSON.stringify(pos(dst));
    if (!same) { mism++; if (mism <= 3) console.log('  MISMATCH at kept', k, '(orig idx', k * stride + ')'); }
    const p = pos(dst); xs.push(p[0]); zs.push(p[2]);
  }
  console.log('field mismatches (id/name/ref/pos):', mism, '/', after.length);
  console.log('retained X range:', Math.min(...xs).toFixed(2), '->', Math.max(...xs).toFixed(2));
  console.log('retained Z range:', Math.min(...zs).toFixed(2), '->', Math.max(...zs).toFixed(2));
  console.log('unique external entity refs:', new Set(after.map((e) => fld(e, 'entityData'))).size);
}

if (cmd === 'reduce') {
  const TARGET = 40;
  const els = await getChildren();
  const total = els.length;
  console.log('current child count:', total);
  if (total <= TARGET) {
    console.log('already <= target; nothing to do.');
    process.exit(0);
  }
  const stride = Math.floor(total / TARGET); // 400/40 = 10
  const keepIdx = [];
  for (let k = 0; k < TARGET; k++) keepIdx.push(k * stride);
  const specs = keepIdx.map((i) => toSpec(els[i]));
  console.log('keeping indices  :', keepIdx[0] + '..' + keepIdx[keepIdx.length - 1] + ' (stride ' + stride + ', ' + specs.length + ' kept)');

  const r = await rpc('tools/call', {
    name: 'set_instance_member',
    arguments: { path: 'Scenes/Soldiers', member: 'components[0].entityData', value: specs },
  });
  if (r.error) throw new Error('RPC error: ' + JSON.stringify(r.error));
  if (r.result.isError) throw new Error('Tool error: ' + (r.result.content?.[0]?.text || '?'));
  const sc = r.result.structuredContent;
  console.log('committed        :', sc.committed, '| member:', sc.member);

  // Verify by re-reading.
  const after = await getChildren();
  console.log('child count after:', after.length);
  const okType = after.every((e) => e.type === 'traktor.world.ExternalEntityData');
  console.log('all ExternalEntityData:', okType);
  const first = memberByName(memberByName(after[0], 'transform'), 'translation').value;
  const last = memberByName(memberByName(after[after.length - 1], 'transform'), 'translation').value;
  const expFirst = memberByName(memberByName(els[keepIdx[0]], 'transform'), 'translation').value;
  const expLast = memberByName(memberByName(els[keepIdx[keepIdx.length - 1]], 'transform'), 'translation').value;
  console.log('first kept pos   :', JSON.stringify(first), 'expected', JSON.stringify(expFirst));
  console.log('last  kept pos   :', JSON.stringify(last), 'expected', JSON.stringify(expLast));
}

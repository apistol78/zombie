const fs = require("node:fs");
const f = "D:/private/traktor/code/MCP/Server/Editor/CreateMeshFromGeometryTool.cpp";
let s = fs.readFileSync(f, "utf8");
const nl = s.includes("\r\n") ? "\r\n" : "\n";
const L = (...lines) => lines.join(nl);
const Q = "\\\""; // backslash-quote, i.e. an escaped quote inside a C++ string literal

function rep(name, oldS, newS) {
  const i = s.indexOf(oldS);
  if (i < 0) { console.log("MISS  " + name); process.exitCode = 2; return; }
  if (s.indexOf(oldS, i + 1) >= 0) { console.log("DUPE  " + name); process.exitCode = 2; return; }
  s = s.slice(0, i) + newS + s.slice(i + oldS.length);
  console.log("ok    " + name);
}

// 1a winding word in description
rep("desc-winding",
  "each an array of indices into positions (counter-clockwise winding for front faces; triangles, quads or n-gons).",
  "each an array of indices into positions (clockwise winding for front faces; triangles, quads or n-gons).");

// 1b maps note in description
rep("desc-maps",
  "Build it (build_asset) before use.",
  `Optional ${Q}maps${Q} binds material textures (keys ${Q}diffuse${Q}/${Q}normal${Q}/${Q}roughness${Q}/${Q}specular${Q}/${Q}metalness${Q}/${Q}emissive${Q}, each a texture instance guid; requires ${Q}texCoords${Q}) so the pipeline auto-generates a textured PBR material shader. Build it (build_asset) before use.`);

// 1c polygons schema (CCW)->(CW)
rep("schema-ccw",
  "each is an array of indices into positions (CCW).",
  "each is an array of indices into positions (CW).");

// 2 obj lambda
rep("obj-lambda",
  L(`\tauto boolean = [](const wchar_t* d) { Ref< Json > p = Json::createObject(); p->setString(L"type", L"boolean"); p->setString(L"description", d); return p; };`),
  L(`\tauto boolean = [](const wchar_t* d) { Ref< Json > p = Json::createObject(); p->setString(L"type", L"boolean"); p->setString(L"description", d); return p; };`,
    `\tauto obj = [](const wchar_t* d) { Ref< Json > p = Json::createObject(); p->setString(L"type", L"object"); p->setString(L"description", d); return p; };`));

// 3 maps property
rep("schema-maps",
  L(`\tproperties->set(L"guid", str(L"Optional explicit guid for the new instance."));`),
  L(`\tproperties->set(L"maps", obj(L"Optional material texture maps; keys ${Q}diffuse${Q}/${Q}normal${Q}/${Q}roughness${Q}/${Q}specular${Q}/${Q}metalness${Q}/${Q}emissive${Q}, each a texture instance guid. Requires ${Q}texCoords${Q}; the pipeline auto-generates a PBR material shader sampling them."));`,
    `\tproperties->set(L"guid", str(L"Optional explicit guid for the new instance."));`));

// 4 material + maps block in invoke
rep("invoke-material",
  L(`\tRef< model::Model > mdl = new model::Model();`,
    `\tconst uint32_t materialIndex = mdl->addMaterial(model::Material(materialName));`,
    ``,
    `\t// Positions / normals / texcoords are kept index-parallel so a polygon's`,
    `\t// position index also selects its normal and texcoord.`,
    `\tAlignedVector< uint32_t > positionIndex(positionCount);`,
    `\tAlignedVector< uint32_t > normalIndex(haveNormals ? positionCount : 0);`,
    `\tAlignedVector< uint32_t > texCoordIndex(haveTexCoords ? positionCount : 0);`,
    `\tuint32_t texCoordChannel = 0;`,
    `\tif (haveTexCoords)`,
    `\t\ttexCoordChannel = mdl->addUniqueTexCoordChannel(L"UVMap");`),
  L(`\tRef< model::Model > mdl = new model::Model();`,
    ``,
    `\tuint32_t texCoordChannel = 0;`,
    `\tif (haveTexCoords)`,
    `\t\ttexCoordChannel = mdl->addUniqueTexCoordChannel(L"UVMap");`,
    ``,
    `\t// Build the material, optionally binding texture maps so the mesh pipeline`,
    `\t// auto-generates a textured PBR material shader from them.`,
    `\tmodel::Material material(materialName);`,
    `\tconst Json* maps = arguments->getMember(L"maps");`,
    `\tif (maps && maps->isObject())`,
    `\t{`,
    `\t\tif (!haveTexCoords)`,
    `\t\t{`,
    `\t\t\toutError = L"${Q}maps${Q} requires ${Q}texCoords${Q} so the material can be sampled.";`,
    `\t\t\treturn nullptr;`,
    `\t\t}`,
    `\t\tauto bindMap = [&](const wchar_t* key) -> model::Material::Map {`,
    `\t\t\tconst Json* m = maps->getMember(key);`,
    `\t\t\tif (!m)`,
    `\t\t\t\treturn model::Material::Map();`,
    `\t\t\tconst Guid g(m->getString());`,
    `\t\t\tif (!g.isValid())`,
    `\t\t\t\treturn model::Material::Map();`,
    `\t\t\treturn model::Material::Map(key, texCoordChannel, false, g);`,
    `\t\t};`,
    `\t\tconst model::Material::Map dm = bindMap(L"diffuse"); if (dm.texture.isNotNull()) material.setDiffuseMap(dm);`,
    `\t\tconst model::Material::Map nm = bindMap(L"normal"); if (nm.texture.isNotNull()) material.setNormalMap(nm);`,
    `\t\tconst model::Material::Map rm = bindMap(L"roughness"); if (rm.texture.isNotNull()) material.setRoughnessMap(rm);`,
    `\t\tconst model::Material::Map sm = bindMap(L"specular"); if (sm.texture.isNotNull()) material.setSpecularMap(sm);`,
    `\t\tconst model::Material::Map mm = bindMap(L"metalness"); if (mm.texture.isNotNull()) material.setMetalnessMap(mm);`,
    `\t\tconst model::Material::Map em = bindMap(L"emissive"); if (em.texture.isNotNull()) material.setEmissiveMap(em);`,
    `\t}`,
    `\tconst uint32_t materialIndex = mdl->addMaterial(material);`,
    ``,
    `\t// Positions / normals / texcoords are kept index-parallel so a polygon's`,
    `\t// position index also selects its normal and texcoord.`,
    `\tAlignedVector< uint32_t > positionIndex(positionCount);`,
    `\tAlignedVector< uint32_t > normalIndex(haveNormals ? positionCount : 0);`,
    `\tAlignedVector< uint32_t > texCoordIndex(haveTexCoords ? positionCount : 0);`));

if (process.exitCode === 2) console.log("ABORTED - no write");
else { fs.writeFileSync(f, s, "utf8"); console.log("WROTE " + f); }

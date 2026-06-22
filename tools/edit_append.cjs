const fs = require("node:fs");
function edit(file, reps) {
  let s = fs.readFileSync(file, "utf8");
  const nl = s.includes("\r\n") ? "\r\n" : "\n";
  for (const [name, oldS_, newS_] of reps) {
    const oldS = oldS_.split("\n").join(nl), newS = newS_.split("\n").join(nl);
    const i = s.indexOf(oldS);
    if (i < 0) { console.log("MISS  " + name); process.exitCode = 2; continue; }
    if (s.indexOf(oldS, i + 1) >= 0) { console.log("DUPE  " + name); process.exitCode = 2; continue; }
    s = s.slice(0, i) + newS + s.slice(i + oldS.length);
    console.log("ok    " + name);
  }
  if (process.exitCode !== 2) { fs.writeFileSync(file, s, "utf8"); console.log("WROTE " + file); }
}

// --- InstanceReflectionSupport.h ---
edit("D:/private/traktor/code/MCP/Server/Editor/InstanceReflectionSupport.h", [
  ["hdr-sig",
    `bool setMemberThroughPath(db::Database* database, ISerializable* object, const AlignedVector< PathStep >& steps, size_t start, const Json* spec, std::wstring& outError);`,
    `bool setMemberThroughPath(db::Database* database, ISerializable* object, const AlignedVector< PathStep >& steps, size_t start, const Json* spec, std::wstring& outError, bool append = false);`],
]);

// --- InstanceReflectionSupport.cpp ---
edit("D:/private/traktor/code/MCP/Server/Editor/InstanceReflectionSupport.cpp", [
  ["assign-sig",
    `bool assignValue(db::Database* database, ReflectionMember* target, const Json* spec, std::wstring& outError)`,
    `bool assignValue(db::Database* database, ReflectionMember* target, const Json* spec, std::wstring& outError, bool append)`],
  ["assign-clear",
    `\t\twhile (arr->getMemberCount() > 0)\n\t\t\tarr->removeMember(arr->getMember(0));`,
    `\t\tif (!append)\n\t\t{\n\t\t\twhile (arr->getMemberCount() > 0)\n\t\t\t\tarr->removeMember(arr->getMember(0));\n\t\t}`],
  ["path-sig",
    `bool setMemberThroughPath(db::Database* database, ISerializable* object, const AlignedVector< PathStep >& steps, size_t start, const Json* spec, std::wstring& outError)\n{`,
    `bool setMemberThroughPath(db::Database* database, ISerializable* object, const AlignedVector< PathStep >& steps, size_t start, const Json* spec, std::wstring& outError, bool append)\n{`],
  ["path-recurse",
    `\t\tif (!setMemberThroughPath(database, nested, steps, next, spec, outError))`,
    `\t\tif (!setMemberThroughPath(database, nested, steps, next, spec, outError, append))`],
  ["path-assign",
    `\tif (!assignValue(database, member, spec, outError))`,
    `\tif (!assignValue(database, member, spec, outError, append))`],
]);

// --- InstanceSetMemberTool.cpp ---
edit("D:/private/traktor/code/MCP/Server/Editor/InstanceSetMemberTool.cpp", [
  ["tool-desc",
    `For (object-element) array members: a JSON array of those object specs (replaces the array).`,
    `For array members: a JSON array of element specs (object specs, or guid strings for resource::Id sets); replaces the array, or pass \\"append\\":true to add the element(s) to the existing array (existing elements preserved).`],
  ["tool-schema",
    `\tproperties->set(L"value", valueProperty);`,
    `\tRef< Json > appendProperty = Json::createObject();\n\tappendProperty->setString(L"type", L"boolean");\n\tappendProperty->setString(L"description", L"For array members, add the given element(s) to the existing array instead of replacing it (default false).");\n\n\tproperties->set(L"value", valueProperty);\n\tproperties->set(L"append", appendProperty);`],
  ["tool-invoke",
    `\tif (!setMemberThroughPath(database, object, steps, 0, value, outError))\n\t\treturn nullptr;`,
    `\tconst bool append = arguments && arguments->getMember(L"append") && arguments->getMember(L"append")->getBoolean();\n\tif (!setMemberThroughPath(database, object, steps, 0, value, outError, append))\n\t\treturn nullptr;`],
]);

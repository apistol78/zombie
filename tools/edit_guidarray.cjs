const fs = require("node:fs");
const f = "D:/private/traktor/code/MCP/Server/Editor/InstanceReflectionSupport.cpp";
let s = fs.readFileSync(f, "utf8");
const nl = s.includes("\r\n") ? "\r\n" : "\n";
const L = (...lines) => lines.join(nl);
function rep(name, oldS, newS) {
  const i = s.indexOf(oldS);
  if (i < 0) { console.log("MISS  " + name); process.exitCode = 2; return; }
  if (s.indexOf(oldS, i + 1) >= 0) { console.log("DUPE  " + name); process.exitCode = 2; return; }
  s = s.slice(0, i) + newS + s.slice(i + oldS.length);
  console.log("ok    " + name);
}

rep("guid-array-element",
  L(`\t\t\telse`,
    `\t\t\t{`,
    `\t\t\t\toutError = L"Only object-element arrays are supported for set; each element must be an object with \\"$type\\" or \\"$clone\\".";`,
    `\t\t\t\treturn false;`,
    `\t\t\t}`),
  L(`\t\t\telse if (element && element->isString() && Guid(element->getString()).isValid())`,
    `\t\t\t{`,
    `\t\t\t\t// Guid-string element, e.g. a resource::Id set such as physics collision groups.`,
    `\t\t\t\tarr->addMember(new RfmPrimitiveGuid(L"item", Guid(element->getString()), nullptr));`,
    `\t\t\t}`,
    `\t\t\telse`,
    `\t\t\t{`,
    `\t\t\t\toutError = L"Array elements must be object specs (\\"$type\\"/\\"$clone\\") or guid strings.";`,
    `\t\t\t\treturn false;`,
    `\t\t\t}`));

if (process.exitCode === 2) console.log("ABORTED - no write");
else { fs.writeFileSync(f, s, "utf8"); console.log("WROTE " + f); }

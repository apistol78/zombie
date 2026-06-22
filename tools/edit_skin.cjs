const fs = require("node:fs");
const f = "D:/private/traktor/code/MCP/Server/Editor/CreateMeshFromGeometryTool.cpp";
let s = fs.readFileSync(f, "utf8");
const nl = s.includes("\r\n") ? "\r\n" : "\n";
const L = (...lines) => lines.join(nl);
const Q = "\\\"";
function rep(name, oldS, newS) {
  const i = s.indexOf(oldS);
  if (i < 0) { console.log("MISS  " + name); process.exitCode = 2; return; }
  if (s.indexOf(oldS, i + 1) >= 0) { console.log("DUPE  " + name); process.exitCode = 2; return; }
  s = s.slice(0, i) + newS + s.slice(i + oldS.length);
  console.log("ok    " + name);
}

// A. includes
rep("includes",
  L(`#include "Model/Operations/Triangulate.h"`, `#include "MCP/Server/Json.h"`),
  L(`#include "Model/Operations/Triangulate.h"`,
    `#include "Model/Animation.h"`,
    `#include "Model/Joint.h"`,
    `#include "Model/Pose.h"`,
    `#include "Core/Math/Quaternion.h"`,
    `#include "Core/Math/Transform.h"`,
    `#include "MCP/Server/Json.h"`));

// B. joints + jointIndices parsing, inserted before the polygon loop
rep("joints-block",
  L(`\tfor (uint32_t i = 0; i < polygons->size(); ++i)`, `\t{`, `\t\tconst Json* poly = polygons->at(i);`),
  L(`\t// Skeleton joints (optional): each { name, parent, translation[3], rotation[4], length }.`,
    `\tconst Json* jointsJson = arguments->getMember(L"joints");`,
    `\tif (jointsJson && jointsJson->isArray())`,
    `\t{`,
    `\t\tfor (uint32_t i = 0; i < jointsJson->size(); ++i)`,
    `\t\t{`,
    `\t\t\tconst Json* jn = jointsJson->at(i);`,
    `\t\t\tif (!jn || !jn->isObject())`,
    `\t\t\t{`,
    `\t\t\t\toutError = L"joints[" + std::to_wstring(i) + L"] must be an object.";`,
    `\t\t\t\treturn nullptr;`,
    `\t\t\t}`,
    `\t\t\tconst std::wstring jname = jn->getMember(L"name") ? jn->getMember(L"name")->getString() : (L"joint" + std::to_wstring(i));`,
    `\t\t\tconst int32_t parent = jn->getMember(L"parent") ? (int32_t)jn->getMember(L"parent")->getNumber() : -1;`,
    `\t\t\tfloat tr[3] = { 0.0f, 0.0f, 0.0f };`,
    `\t\t\tif (jn->getMember(L"translation"))`,
    `\t\t\t\treadVec(jn->getMember(L"translation"), 3, tr);`,
    `\t\t\tfloat rot[4] = { 0.0f, 0.0f, 0.0f, 1.0f };`,
    `\t\t\tif (jn->getMember(L"rotation"))`,
    `\t\t\t\treadVec(jn->getMember(L"rotation"), 4, rot);`,
    `\t\t\tconst float length = jn->getMember(L"length") ? (float)jn->getMember(L"length")->getReal() : 0.1f;`,
    `\t\t\tconst uint32_t parentIdx = (parent >= 0) ? (uint32_t)parent : model::c_InvalidIndex;`,
    `\t\t\tmdl->addJoint(model::Joint(parentIdx, jname, Transform(Vector4(tr[0], tr[1], tr[2], 0.0f), Quaternion(rot[0], rot[1], rot[2], rot[3])), length));`,
    `\t\t}`,
    `\t}`,
    ``,
    `\t// Per-position rigid skin weights (optional): joint index per position, weight 1.0.`,
    `\tconst Json* jointIndices = arguments->getMember(L"jointIndices");`,
    `\tconst bool haveJointWeights = jointIndices && jointIndices->isArray() && jointIndices->size() == positionCount;`,
    `\tAlignedVector< int32_t > jointIndexPerPos;`,
    `\tif (haveJointWeights)`,
    `\t{`,
    `\t\tjointIndexPerPos.resize(positionCount);`,
    `\t\tfor (uint32_t i = 0; i < positionCount; ++i)`,
    `\t\t\tjointIndexPerPos[i] = (int32_t)jointIndices->at(i)->getNumber();`,
    `\t}`,
    ``,
    `\tfor (uint32_t i = 0; i < polygons->size(); ++i)`,
    `\t{`,
    `\t\tconst Json* poly = polygons->at(i);`));

// C. joint influence in vertex loop
rep("vertex-weight",
  L(`\t\t\tif (haveTexCoords)`, `\t\t\t\tvertex.setTexCoord(texCoordChannel, texCoordIndex[pi]);`, `\t\t\tpolygon.addVertex(mdl->addVertex(vertex));`),
  L(`\t\t\tif (haveTexCoords)`, `\t\t\t\tvertex.setTexCoord(texCoordChannel, texCoordIndex[pi]);`,
    `\t\t\tif (haveJointWeights && jointIndexPerPos[pi] >= 0)`,
    `\t\t\t\tvertex.setJointInfluence((uint32_t)jointIndexPerPos[pi], 1.0f);`,
    `\t\t\tpolygon.addVertex(mdl->addVertex(vertex));`));

// D. animations block after triangulate
rep("animations-block",
  L(`\tif (triangulate)`, `\t\tmdl->apply(model::Triangulate());`, ``, `\t// Write the model to a file under the asset path.`),
  L(`\tif (triangulate)`, `\t\tmdl->apply(model::Triangulate());`,
    ``,
    `\t// Animations (optional): each { name, keyframes:[{ time, pose:[[tx,ty,tz,qx,qy,qz,qw] per joint] }] }.`,
    `\tconst Json* animations = arguments->getMember(L"animations");`,
    `\tif (animations && animations->isArray())`,
    `\t{`,
    `\t\tconst uint32_t jointCount = mdl->getJointCount();`,
    `\t\tfor (uint32_t a = 0; a < animations->size(); ++a)`,
    `\t\t{`,
    `\t\t\tconst Json* an = animations->at(a);`,
    `\t\t\tif (!an || !an->isObject())`,
    `\t\t\t\tcontinue;`,
    `\t\t\tRef< model::Animation > anim = new model::Animation();`,
    `\t\t\tanim->setName(an->getMember(L"name") ? an->getMember(L"name")->getString() : (L"anim" + std::to_wstring(a)));`,
    `\t\t\tconst Json* keyframes = an->getMember(L"keyframes");`,
    `\t\t\tif (keyframes && keyframes->isArray())`,
    `\t\t\t{`,
    `\t\t\t\tfor (uint32_t k = 0; k < keyframes->size(); ++k)`,
    `\t\t\t\t{`,
    `\t\t\t\t\tconst Json* kf = keyframes->at(k);`,
    `\t\t\t\t\tif (!kf || !kf->isObject())`,
    `\t\t\t\t\t\tcontinue;`,
    `\t\t\t\t\tconst float time = kf->getMember(L"time") ? (float)kf->getMember(L"time")->getReal() : 0.0f;`,
    `\t\t\t\t\tconst Json* poseJson = kf->getMember(L"pose");`,
    `\t\t\t\t\tRef< model::Pose > pose = new model::Pose();`,
    `\t\t\t\t\tif (poseJson && poseJson->isArray())`,
    `\t\t\t\t\t{`,
    `\t\t\t\t\t\tconst uint32_t n = (poseJson->size() < jointCount) ? poseJson->size() : jointCount;`,
    `\t\t\t\t\t\tfor (uint32_t j = 0; j < n; ++j)`,
    `\t\t\t\t\t\t{`,
    `\t\t\t\t\t\t\tfloat p[7] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f };`,
    `\t\t\t\t\t\t\treadVec(poseJson->at(j), 7, p);`,
    `\t\t\t\t\t\t\tpose->setJointTransform(j, Transform(Vector4(p[0], p[1], p[2], 0.0f), Quaternion(p[3], p[4], p[5], p[6])));`,
    `\t\t\t\t\t\t}`,
    `\t\t\t\t\t}`,
    `\t\t\t\t\tanim->insertKeyFrame(time, pose);`,
    `\t\t\t\t}`,
    `\t\t\t}`,
    `\t\t\tmdl->addAnimation(anim);`,
    `\t\t}`,
    `\t}`,
    ``,
    `\t// Write the model to a file under the asset path.`));

// E. meshType
rep("meshtype",
  L(`\tRef< mesh::MeshAsset > meshAsset = new mesh::MeshAsset();`, `\tmeshAsset->setFileName(Path(fileName));`, `\tmeshAsset->setMeshType(mesh::MeshAsset::MeshType::Static);`),
  L(`\tRef< mesh::MeshAsset > meshAsset = new mesh::MeshAsset();`, `\tmeshAsset->setFileName(Path(fileName));`,
    `\tconst std::wstring meshTypeStr = arguments->getMember(L"meshType") ? arguments->getMember(L"meshType")->getString() : L"static";`,
    `\tmeshAsset->setMeshType(meshTypeStr == L"skinned" ? mesh::MeshAsset::MeshType::Skinned : mesh::MeshAsset::MeshType::Static);`));

// F. result fields
rep("result",
  L(`\tresult->setBoolean(L"triangulated", triangulate);`, `\tresult->setBoolean(L"committed", true);`),
  L(`\tresult->setBoolean(L"triangulated", triangulate);`,
    `\tresult->set(L"joints", Json::createNumber((int64_t)mdl->getJointCount()));`,
    `\tresult->set(L"animations", Json::createNumber((int64_t)mdl->getAnimationCount()));`,
    `\tresult->setString(L"meshType", meshTypeStr);`,
    `\tresult->setBoolean(L"committed", true);`));

// G. schema props
rep("schema",
  L(`\tproperties->set(L"triangulate", boolean(L"Triangulate the model (default true)."));`),
  L(`\tproperties->set(L"triangulate", boolean(L"Triangulate the model (default true)."));`,
    `\tproperties->set(L"meshType", str(L"${Q}static${Q} (default) or ${Q}skinned${Q}. Skinned requires ${Q}joints${Q} and ${Q}jointIndices${Q}."));`,
    `\tproperties->set(L"joints", arr(L"Optional skeleton joints (order = joint index); each { name, parent (joint index or -1 for root), translation [x,y,z], rotation [x,y,z,w], length }."));`,
    `\tproperties->set(L"jointIndices", arr(L"Optional per-position joint index (parallel to positions) for rigid skinning (weight 1.0; -1 = none)."));`,
    `\tproperties->set(L"animations", arr(L"Optional animations; each { name, keyframes:[{ time, pose:[[tx,ty,tz,qx,qy,qz,qw], ...one full local transform per joint] }] }."));`));

// H. description note
rep("desc",
  "Build it (build_asset) before use.",
  `Pass ${Q}meshType${Q}:${Q}skinned${Q} with ${Q}joints${Q} (skeleton), ${Q}jointIndices${Q} (per-position rigid skin weights) and ${Q}animations${Q} (keyframed full poses) to author a skinned, animated mesh whose .tmd can drive a SkeletonAsset + AnimationAsset. Build it (build_asset) before use.`);

if (process.exitCode === 2) console.log("ABORTED - no write");
else { fs.writeFileSync(f, s, "utf8"); console.log("WROTE " + f); }

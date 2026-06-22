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

// --- InstanceReflectionSupport.cpp: grow-mode + map-mode in assignValue array branch ---
edit("D:/private/traktor/code/MCP/Server/Editor/InstanceReflectionSupport.cpp", [
  ["grow-map",
`	if (auto arr = dynamic_type_cast< RfmArray* >(target))
	{
		if (!spec || !spec->isArray())
		{
			outError = L"Array member '" + std::wstring(target->getName() ? target->getName() : L"") + L"' requires a JSON array value.";
			return false;
		}`,
`	if (auto arr = dynamic_type_cast< RfmArray* >(target))
	{
		// Grow mode: { "$grow": N } inserts N default-typed elements (for struct/enum/primitive vectors; set them afterwards by index path).
		if (spec && spec->isObject() && spec->getMember(L"$grow"))
		{
			const int32_t n = (int32_t)spec->getMember(L"$grow")->getNumber();
			if (!append)
				while (arr->getMemberCount() > 0)
					arr->removeMember(arr->getMember(0));
			for (int32_t i = 0; i < n; ++i)
				arr->insertDefault();
			return true;
		}
		// Map mode: a JSON object { key: valueSpec, ... } builds first/second pair elements (SmallMap members).
		if (spec && spec->isObject())
		{
			if (!append)
				while (arr->getMemberCount() > 0)
					arr->removeMember(arr->getMember(0));
			for (uint32_t i = 0; i < spec->getMemberCount(); ++i)
			{
				const std::wstring key = spec->getMemberName(i);
				const Json* valueSpec = spec->getMemberValue(i);
				RfmCompound* pair = new RfmCompound(L"item", nullptr);
				pair->addMember(new RfmPrimitiveWideString(L"first", key, nullptr));
				if (valueSpec && valueSpec->isObject() && (valueSpec->getMember(L"$type") || valueSpec->getMember(L"$clone")))
				{
					Ref< ISerializable > object = buildObjectFromSpec(database, valueSpec, outError);
					if (!object)
						return false;
					pair->addMember(new RfmObject(L"second", object, nullptr));
				}
				else if (valueSpec && valueSpec->isString() && Guid(valueSpec->getString()).isValid())
					pair->addMember(new RfmPrimitiveGuid(L"second", Guid(valueSpec->getString()), nullptr));
				else if (!valueSpec || valueSpec->isNull())
					pair->addMember(new RfmObject(L"second", (ISerializable*)nullptr, nullptr));
				else
				{
					outError = L"Map value for key '" + key + L"' must be an object spec ($type/$clone), guid string, or null.";
					return false;
				}
				arr->addMember(pair);
			}
			return true;
		}
		if (!spec || !spec->isArray())
		{
			outError = L"Array member '" + std::wstring(target->getName() ? target->getName() : L"") + L"' requires a JSON array value.";
			return false;
		}`],
]);

// --- InstanceSetMemberTool.cpp: description note ---
edit("D:/private/traktor/code/MCP/Server/Editor/InstanceSetMemberTool.cpp", [
  ["desc-mapgrow",
    `or pass \\"append\\":true to add the element(s) to the existing array (existing elements preserved).`,
    `or pass \\"append\\":true to add the element(s) to the existing array (existing elements preserved). For map (SmallMap) members pass a JSON object { key: valueSpec, ... } to set entries (value is an object spec, guid string, or null). To grow a typed vector (struct/enum/primitive elements) pass { \\"$grow\\": N }, then set each element by index path (e.g. \\"entries[0].mul\\").`],
]);

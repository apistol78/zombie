/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryReferences.h"

#include "Core/Reflection/Reflection.h"
#include "Core/Reflection/RfmObject.h"
#include "Core/Reflection/RfmPrimitive.h"
#include "Core/Reflection/RfpMemberType.h"
#include "Core/Serialization/AttributeType.h"
#include "DialogData.h"
#include "DialogNode.h"
#include "QuestStep.h"

namespace zombie
{
	namespace
	{

void collect(const ISerializable* object, SmallSet< Guid >& outReferences, SmallSet< const ISerializable* >& visited)
{
	if (object == nullptr || !visited.insert(object))
		return;

	Ref< Reflection > reflection = Reflection::create(object);
	if (!reflection)
		return;

	// Resource references are guids typed by the resource they refer to.
	RefArray< ReflectionMember > guidMembers;
	reflection->findMembers(RfpMemberType(type_of< RfmPrimitiveGuid >()), guidMembers);
	for (const ReflectionMember* member : guidMembers)
	{
		const Attribute* attributes = member->getAttributes();
		const AttributeType* attributeType = attributes ? attributes->find< AttributeType >() : nullptr;
		if (attributeType == nullptr || isPartReference(attributeType->getMemberType()))
			continue;

		const Guid& id = static_cast< const RfmPrimitiveGuid* >(member)->get();
		if (id.isValid() && id.isNotNull())
			outReferences.insert(id);
	}

	RefArray< ReflectionMember > objectMembers;
	reflection->findMembers(RfpMemberType(type_of< RfmObject >()), objectMembers);
	for (const ReflectionMember* member : objectMembers)
		collect(static_cast< const RfmObject* >(member)->get(), outReferences, visited);
}

	}

bool isPartReference(const TypeInfo& type)
{
	return is_type_of< QuestStep >(type) || is_type_of< DialogNode >(type);
}

void collectReferences(const ISerializable* object, SmallSet< Guid >& outReferences)
{
	SmallSet< const ISerializable* > visited;
	collect(object, outReferences, visited);
}

void visitObjects(const ISerializable* object, const std::function< void(const ISerializable*) >& visitor)
{
	SmallSet< const ISerializable* > visited;
	std::function< void(const ISerializable*) > visit = [&](const ISerializable* object) {
		if (object == nullptr || !visited.insert(object))
			return;

		visitor(object);

		Ref< Reflection > reflection = Reflection::create(object);
		if (!reflection)
			return;

		RefArray< ReflectionMember > objectMembers;
		reflection->findMembers(RfpMemberType(type_of< RfmObject >()), objectMembers);
		for (const ReflectionMember* member : objectMembers)
			visit(static_cast< const RfmObject* >(member)->get());
	};
	visit(object);
}

}

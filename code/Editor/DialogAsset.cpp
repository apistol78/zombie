/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/DialogAsset.h"

#include "Core/Serialization/AttributeMultiLine.h"
#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRef.h"
#include "Core/Serialization/MemberRefArray.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.DialogAsset", 2, DialogAsset, ISerializable)

DialogNodeAsset* DialogAsset::findNode(const Guid& id) const
{
	if (id.isNull())
		return nullptr;

	for (auto node : nodes)
	{
		if (node->id == id)
			return node;
	}
	return nullptr;
}

DialogNodeAsset* DialogAsset::findNodeByName(const std::wstring& name) const
{
	for (auto node : nodes)
	{
		if (node->name == name)
			return node;
	}
	return nullptr;
}

void DialogAsset::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", name);
	s >> MemberRefArray< DialogStartAsset >(L"start", start);
	s >> MemberRefArray< DialogNodeAsset >(L"nodes", nodes);

	if (s.getVersion< DialogAsset >() >= 1)
		s >> Member< Vector2 >(L"startPosition", startPosition, AttributePrivate());

	// Links written by name, before lines had guids, are turned into links by guid.
	if (s.getDirection() == ISerializer::Direction::Read)
	{
		auto resolve = [&](const std::wstring& legacy) -> Guid {
			const DialogNodeAsset* node = !legacy.empty() ? findNodeByName(legacy) : nullptr;
			return node ? node->id : Guid();
		};

		for (auto startAsset : start)
		{
			if (!startAsset->legacyNode.empty())
			{
				startAsset->node = resolve(startAsset->legacyNode);
				startAsset->legacyNode.clear();
			}
		}

		for (auto node : nodes)
		{
			for (auto response : node->responses)
			{
				if (!response->legacyTarget.empty())
				{
					response->target = resolve(response->legacyTarget);
					response->legacyTarget.clear();
				}
			}
		}
	}
}

}

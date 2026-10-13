/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ItemData.h"

#include "Core/Serialization/AttributeMultiLine.h"
#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Resource/Member.h"
#include "World/EntityData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ItemData", 0, ItemData, ISerializable)

const IItemTrait* ItemData::getTrait(const TypeInfo& traitType) const
{
	for (auto trait : m_traits)
	{
		if (is_type_of(traitType, type_of(trait)))
			return trait;
	}
	return nullptr;
}

void ItemData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
	s >> Member< std::wstring >(L"pluralName", m_pluralName);
	s >> Member< std::wstring >(L"info", m_info, AttributeMultiLine());
	s >> Member< std::wstring >(L"icon", m_icon);
	s >> Member< int32_t >(L"maxStack", m_maxStack, AttributeRange(1));
	s >> Member< bool >(L"quest", m_quest);
	s >> Member< bool >(L"keep", m_keep);
	s >> resource::Member< world::EntityData >(L"dropEntity", m_dropEntity);
	s >> MemberRefArray< IItemTrait >(L"traits", m_traits);
}

}

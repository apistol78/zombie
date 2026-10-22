/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "StoryData.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberAlignedVector.h"
#include "Core/Serialization/MemberComposite.h"
#include "DialogData.h"
#include "ItemData.h"
#include "QuestData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.StoryData", 2, StoryData, ISerializable)

void StoryData::serialize(ISerializer& s)
{
	s >> MemberAlignedVector< resource::Id< QuestData >, resource::Member< QuestData > >(L"quests", m_quests);
	if (s.getVersion< StoryData >() < 1)
	{
		resource::Id< DialogData > defaultDialog;
		s >> resource::Member< DialogData >(L"defaultDialog", defaultDialog);
	}
	s >> MemberAlignedVector< ItemStack, MemberComposite< ItemStack > >(L"startKit", m_startKit);
	if (s.getVersion< StoryData >() >= 2)
		s >> MemberAlignedVector< resource::Id< ItemData >, resource::Member< ItemData > >(L"items", m_items);
}

void StoryData::ItemStack::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"item", item);
	s >> Member< int32_t >(L"count", count, AttributeRange(1));
}

}

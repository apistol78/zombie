/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Conditions/HasItemCondition.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Inventory.h"
#include "ItemData.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.HasItemCondition", 0, HasItemCondition, ICondition)

HasItemCondition::HasItemCondition(const resource::Id< ItemData >& item, int32_t count, bool negate)
:	m_item(item)
,	m_count(count)
,	m_negate(negate)
{
}

bool HasItemCondition::evaluate(const Story* story) const
{
	const bool has = story->getInventory()->count(m_item) >= m_count;
	return has != m_negate;
}

void HasItemCondition::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"item", m_item);
	s >> Member< int32_t >(L"count", m_count, AttributeRange(1));
	s >> Member< bool >(L"negate", m_negate);
}

}

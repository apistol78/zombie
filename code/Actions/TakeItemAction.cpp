/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Actions/TakeItemAction.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "ItemData.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.TakeItemAction", 0, TakeItemAction, IAction)

TakeItemAction::TakeItemAction(const resource::Id< ItemData >& item, int32_t count)
:	m_item(item)
,	m_count(count)
{
}

void TakeItemAction::execute(Story* story) const
{
	story->takeItem(m_item, m_count);
}

void TakeItemAction::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"item", m_item);
	s >> Member< int32_t >(L"count", m_count, AttributeRange(1));
}

}

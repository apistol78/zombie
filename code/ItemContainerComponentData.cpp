/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ItemContainerComponentData.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberAlignedVector.h"
#include "Core/Serialization/MemberComposite.h"
#include "ItemContainerComponent.h"
#include "ItemData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ItemContainerComponentData", 0, ItemContainerComponentData, world::IEntityComponentData)

void ItemContainerComponentData::serialize(ISerializer& s)
{
	s >> MemberAlignedVector< Entry, MemberComposite< Entry > >(L"entries", m_entries);
}

Ref< ItemContainerComponent > ItemContainerComponentData::createComponent() const
{
	Ref< ItemContainerComponent > component = new ItemContainerComponent();
	for (const auto& entry : m_entries)
		component->add(entry.item, entry.count);
	return component;
}

int32_t ItemContainerComponentData::getOrdinal() const
{
	return 0;
}

void ItemContainerComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void ItemContainerComponentData::Entry::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"item", item);
	s >> Member< int32_t >(L"count", count, AttributeRange(1));
}

}

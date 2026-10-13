/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ItemPickupComponentData.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "ItemData.h"
#include "ItemPickupComponent.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ItemPickupComponentData", 0, ItemPickupComponentData, world::IEntityComponentData)

Ref< ItemPickupComponent > ItemPickupComponentData::createComponent() const
{
	return new ItemPickupComponent(m_item, m_count);
}

int32_t ItemPickupComponentData::getOrdinal() const
{
	return 0;
}

void ItemPickupComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void ItemPickupComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"item", m_item);
	s >> Member< int32_t >(L"count", m_count, AttributeRange(1));
}

}

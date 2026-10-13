/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ItemPickupComponent.h"

#include "World/Entity.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.ItemPickupComponent", ItemPickupComponent, world::IEntityComponent)

ItemPickupComponent::ItemPickupComponent(const Guid& item, int32_t count)
:	m_item(item)
,	m_count(count)
{
}

void ItemPickupComponent::destroy()
{
	m_owner = nullptr;
}

void ItemPickupComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
}

void ItemPickupComponent::setTransform(const Transform& transform)
{
}

Aabb3 ItemPickupComponent::getBoundingBox() const
{
	return Aabb3();
}

void ItemPickupComponent::update(const world::UpdateParams& update)
{
}

}

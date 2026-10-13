/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PlaceComponent.h"

#include "World/Entity.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.PlaceComponent", PlaceComponent, world::IEntityComponent)

PlaceComponent::PlaceComponent(const Guid& place)
:	m_place(place)
{
}

void PlaceComponent::destroy()
{
	m_owner = nullptr;
}

void PlaceComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
}

void PlaceComponent::setTransform(const Transform& transform)
{
}

Aabb3 PlaceComponent::getBoundingBox() const
{
	return Aabb3();
}

void PlaceComponent::update(const world::UpdateParams& update)
{
}

}

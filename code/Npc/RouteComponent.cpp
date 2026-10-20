/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/RouteComponent.h"

#include "World/Entity.h"
#include "World/Entity/GroupComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.RouteComponent", RouteComponent, world::IEntityComponent)

RouteComponent::RouteComponent(bool loop, float pause)
:	m_loop(loop)
,	m_pause(pause)
{
}

void RouteComponent::destroy()
{
	m_owner = nullptr;
}

void RouteComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
	m_pointsRead = false;
}

void RouteComponent::setTransform(const Transform& transform)
{
}

Aabb3 RouteComponent::getBoundingBox() const
{
	return Aabb3();
}

void RouteComponent::update(const world::UpdateParams& update)
{
}

const AlignedVector< Vector4 >& RouteComponent::getPoints()
{
	if (m_pointsRead || !m_owner)
		return m_points;

	m_points.resize(0);
	if (auto group = m_owner->getComponent< world::GroupComponent >())
	{
		for (auto entity : group->getEntities())
			m_points.push_back(entity->getTransform().translation().xyz1());
	}

	m_pointsRead = true;
	return m_points;
}

}

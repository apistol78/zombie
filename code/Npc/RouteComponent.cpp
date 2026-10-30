/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/RouteComponent.h"

#include "Core/Math/Random.h"
#include "World/Entity.h"
#include "World/Entity/GroupComponent.h"

#include <algorithm>

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.RouteComponent", RouteComponent, world::IEntityComponent)

RouteComponent::RouteComponent(RouteMode mode, float pause)
:	m_mode(mode)
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

	for (uint32_t i = 0; i < m_points.size(); ++i)
	{
		const Vector4& p = m_points[i];
		m_minX = (i == 0) ? p.x() : std::min< float >(m_minX, p.x());
		m_maxX = (i == 0) ? p.x() : std::max< float >(m_maxX, p.x());
		m_minY = (i == 0) ? p.y() : std::min< float >(m_minY, p.y());
		m_maxY = (i == 0) ? p.y() : std::max< float >(m_maxY, p.y());
		m_minZ = (i == 0) ? p.z() : std::min< float >(m_minZ, p.z());
		m_maxZ = (i == 0) ? p.z() : std::max< float >(m_maxZ, p.z());
	}

	m_pointsRead = true;
	return m_points;
}

bool RouteComponent::inside(float x, float z) const
{
	const uint32_t n = (uint32_t)m_points.size();
	if (n < 3)
		return false;

	// Even-odd; crossings of a ray along +X by the outline's edges.
	bool in = false;
	for (uint32_t i = 0, j = n - 1; i < n; j = i++)
	{
		const float xi = m_points[i].x(), zi = m_points[i].z();
		const float xj = m_points[j].x(), zj = m_points[j].z();
		if ((zi > z) != (zj > z) && x < xi + (z - zi) * (xj - xi) / (zj - zi))
			in = !in;
	}
	return in;
}

bool RouteComponent::pickInside(Random& random, float& outX, float& outZ) const
{
	if (m_points.size() < 3)
		return false;

	// Drawn over the bounds until one falls within; few miss unless the outline is very thin.
	for (int32_t i = 0; i < 32; ++i)
	{
		const float x = m_minX + random.nextFloat() * (m_maxX - m_minX);
		const float z = m_minZ + random.nextFloat() * (m_maxZ - m_minZ);
		if (inside(x, z))
		{
			outX = x;
			outZ = z;
			return true;
		}
	}
	return false;
}

}

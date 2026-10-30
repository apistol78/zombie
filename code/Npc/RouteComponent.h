/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Math/Vector4.h"
#include "World/IEntityComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class Random;

}

namespace zombie
{

using namespace traktor;

/*! How a route is walked. */
enum class RouteMode
{
	Path,		//!< From the first point to the last, then back again.
	Loop,		//!< From the last point round to the first.
	Enclosure	//!< The points outline an area, walked about from one random spot in it to the next.
};

/*! Route component.
 * \ingroup Zombie
 *
 * A chain of waypoints, the children of the owner's group in the order they are listed; or,
 * as an enclosure, the outline of an area.
 */
class T_DLLCLASS RouteComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit RouteComponent(RouteMode mode, float pause);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	RouteMode getMode() const { return m_mode; }

	bool getLoop() const { return m_mode == RouteMode::Loop; }

	float getPause() const { return m_pause; }

	/*! Waypoint positions; read once, as waypoints never move. */
	const AlignedVector< Vector4 >& getPoints();

	/*! Whether (x, z) lies within the outline of the points. */
	bool inside(float x, float z) const;

	/*! Height span of the points. */
	float getMinY() const { return m_minY; }

	float getMaxY() const { return m_maxY; }

	/*! A random spot within the outline, in the ground plane; false if none was found. */
	bool pickInside(Random& random, float& outX, float& outZ) const;

private:
	world::Entity* m_owner = nullptr;
	RouteMode m_mode;
	float m_pause;
	bool m_pointsRead = false;
	AlignedVector< Vector4 > m_points;
	float m_minX = 0.0f;
	float m_maxX = 0.0f;
	float m_minY = 0.0f;
	float m_maxY = 0.0f;
	float m_minZ = 0.0f;
	float m_maxZ = 0.0f;
};

}

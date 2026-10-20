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

namespace zombie
{

using namespace traktor;

/*! Route component.
 * \ingroup Zombie
 *
 * A chain of waypoints, the children of the owner's group in the order they are listed.
 */
class T_DLLCLASS RouteComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit RouteComponent(bool loop, float pause);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	bool getLoop() const { return m_loop; }

	float getPause() const { return m_pause; }

	/*! Waypoint positions; read once, as waypoints never move. */
	const AlignedVector< Vector4 >& getPoints();

private:
	world::Entity* m_owner = nullptr;
	bool m_loop;
	float m_pause;
	bool m_pointsRead = false;
	AlignedVector< Vector4 > m_points;
};

}

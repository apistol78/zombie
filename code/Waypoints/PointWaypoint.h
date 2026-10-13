/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Math/Vector4.h"
#include "IWaypoint.h"

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

/*! Waypoint at a point in the world.
 * \ingroup Zombie
 */
class T_DLLCLASS PointWaypoint : public IWaypoint
{
	T_RTTI_CLASS;

public:
	const Vector4& getPosition() const { return m_position; }

	void setPosition(const Vector4& position) { m_position = position; }

	virtual void serialize(ISerializer& s) override final;

private:
	Vector4 m_position = Vector4::origo();
};

}

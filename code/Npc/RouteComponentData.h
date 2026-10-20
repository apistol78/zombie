/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Ref.h"
#include "World/IEntityComponentData.h"

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

class RouteComponent;

/*! Route component data.
 * \ingroup Zombie
 *
 * Makes the owner entity a route for characters to walk; its waypoints are the children of its group.
 */
class T_DLLCLASS RouteComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< RouteComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	/*! Go round from the last point to the first, rather than back again. */
	bool getLoop() const { return m_loop; }

	/*! Seconds stood at each point. */
	float getPause() const { return m_pause; }

private:
	bool m_loop = false;
	float m_pause = 0.0f;
};

}

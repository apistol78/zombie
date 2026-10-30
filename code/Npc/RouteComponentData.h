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
#include "Npc/RouteComponent.h"
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

/*! Route component data.
 * \ingroup Zombie
 *
 * Makes the owner entity a route for characters to walk; its waypoints are the children of its group.
 * As an enclosure the waypoints outline an area instead, in order around it.
 */
class T_DLLCLASS RouteComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< RouteComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	RouteMode getMode() const { return m_mode; }

	/*! Seconds stood at each point. */
	float getPause() const { return m_pause; }

private:
	RouteMode m_mode = RouteMode::Path;
	float m_pause = 0.0f;
};

}

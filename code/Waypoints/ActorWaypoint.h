/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "IWaypoint.h"
#include "Resource/Id.h"

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

class ActorData;

/*! Waypoint at an actor, wherever the actor has walked to.
 * \ingroup Zombie
 */
class T_DLLCLASS ActorWaypoint : public IWaypoint
{
	T_RTTI_CLASS;

public:
	const resource::Id< ActorData >& getActor() const { return m_actor; }

	void setActor(const resource::Id< ActorData >& actor) { m_actor = actor; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ActorData > m_actor;
};

}

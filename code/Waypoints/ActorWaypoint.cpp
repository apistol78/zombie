/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Waypoints/ActorWaypoint.h"

#include "ActorData.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ActorWaypoint", 0, ActorWaypoint, IWaypoint)

void ActorWaypoint::serialize(ISerializer& s)
{
	s >> resource::Member< ActorData >(L"actor", m_actor);
}

}

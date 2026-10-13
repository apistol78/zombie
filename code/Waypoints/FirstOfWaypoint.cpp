/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Waypoints/FirstOfWaypoint.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberRefArray.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.FirstOfWaypoint", 0, FirstOfWaypoint, IWaypoint)

void FirstOfWaypoint::serialize(ISerializer& s)
{
	s >> MemberRefArray< IWaypoint >(L"candidates", m_candidates);
}

}

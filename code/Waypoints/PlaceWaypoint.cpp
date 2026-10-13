/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Waypoints/PlaceWaypoint.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "PlaceData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.PlaceWaypoint", 0, PlaceWaypoint, IWaypoint)

void PlaceWaypoint::serialize(ISerializer& s)
{
	s >> resource::Member< PlaceData >(L"place", m_place);
}

}

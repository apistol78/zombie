/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Waypoints/PointWaypoint.h"

#include "Core/Serialization/AttributePoint.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.PointWaypoint", 0, PointWaypoint, IWaypoint)

void PointWaypoint::serialize(ISerializer& s)
{
	s >> Member< Vector4 >(L"position", m_position, AttributePoint());
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Waypoints/ItemWaypoint.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "ItemData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ItemWaypoint", 1, ItemWaypoint, IWaypoint)

void ItemWaypoint::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"item", m_item);
	if (s.getVersion< ItemWaypoint >() >= 1)
		s >> Member< bool >(L"untilFound", m_untilFound);
}

}

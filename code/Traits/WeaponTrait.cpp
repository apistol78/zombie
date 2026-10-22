/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Traits/WeaponTrait.h"

#include "Core/Serialization/ISerializer.h"
#include "ItemData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.WeaponTrait", 0, WeaponTrait, IItemTrait)

void WeaponTrait::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"ammo", m_ammo);
}

}

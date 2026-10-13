/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Traits/EquipTrait.h"

#include "Core/Serialization/ISerializer.h"
#include "Keys/EquipSlotData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.EquipTrait", 1, EquipTrait, IItemTrait)

EquipTrait::EquipTrait(const resource::Id< EquipSlotData >& slot)
:	m_slot(slot)
{
}

void EquipTrait::serialize(ISerializer& s)
{
	s >> resource::Member< EquipSlotData >(L"slot", m_slot);
}

}

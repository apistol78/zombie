/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/LockComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Interaction/LockComponent.h"
#include "ItemData.h"
#include "Keys/EquipSlotData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.LockComponentData", 0, LockComponentData, world::IEntityComponentData)

Ref< LockComponent > LockComponentData::createComponent() const
{
	return new LockComponent(this);
}

int32_t LockComponentData::getOrdinal() const
{
	return 0;
}

void LockComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void LockComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< ItemData >(L"item", m_item);
	s >> resource::Member< EquipSlotData >(L"slot", m_slot);
	s >> Member< std::wstring >(L"unequippedRefusal", m_unequippedRefusal);
}

}

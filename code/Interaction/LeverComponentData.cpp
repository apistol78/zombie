/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/LeverComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberRef.h"
#include "Interaction/LeverComponent.h"
#include "World/Entity/PropertyEntityReference.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.LeverComponentData", 0, LeverComponentData, world::IEntityComponentData)

Ref< LeverComponent > LeverComponentData::createComponent() const
{
	return new LeverComponent(m_target ? m_target->getEntityId() : Guid());
}

int32_t LeverComponentData::getOrdinal() const
{
	return 0;
}

void LeverComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void LeverComponentData::serialize(ISerializer& s)
{
	s >> MemberRef< const world::PropertyEntityReference >(L"target", m_target);
}

}

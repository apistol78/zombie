/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/RouteComponentData.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/AttributeUnit.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Npc/RouteComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.RouteComponentData", 0, RouteComponentData, world::IEntityComponentData)

Ref< RouteComponent > RouteComponentData::createComponent() const
{
	return new RouteComponent(m_loop, m_pause);
}

int32_t RouteComponentData::getOrdinal() const
{
	return 0;
}

void RouteComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void RouteComponentData::serialize(ISerializer& s)
{
	s >> Member< bool >(L"loop", m_loop);
	s >> Member< float >(L"pause", m_pause, AttributeRange(0.0f) | AttributeUnit(UnitType::Seconds));
}

}

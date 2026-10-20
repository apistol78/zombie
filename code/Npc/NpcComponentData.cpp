/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/NpcComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberEnum.h"
#include "Core/Serialization/MemberRef.h"
#include "Npc/EnemyComponent.h"
#include "Npc/HumanComponent.h"
#include "World/Entity/PropertyEntityReference.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.NpcComponentData", 0, NpcComponentData, world::IEntityComponentData)

Ref< NpcComponent > NpcComponentData::createComponent() const
{
	const Guid route = m_route ? m_route->getEntityId() : Guid();
	if (m_brain == Brain::Human)
		return new HumanComponent(route);
	else
		return new EnemyComponent(route);
}

int32_t NpcComponentData::getOrdinal() const
{
	return 1000;
}

void NpcComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void NpcComponentData::serialize(ISerializer& s)
{
	const MemberEnum< Brain >::Key c_Brain_Keys[] = {
		{ L"Enemy", Brain::Enemy },
		{ L"Human", Brain::Human },
		{ 0 }
	};

	s >> MemberEnum< Brain >(L"brain", m_brain, c_Brain_Keys);
	s >> MemberRef< const world::PropertyEntityReference >(L"route", m_route);
}

}

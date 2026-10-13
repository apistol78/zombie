/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ActorComponentData.h"

#include "ActorComponent.h"
#include "ActorData.h"
#include "Core/Serialization/ISerializer.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ActorComponentData", 0, ActorComponentData, world::IEntityComponentData)

Ref< ActorComponent > ActorComponentData::createComponent(resource::IResourceManager* resourceManager) const
{
	resource::Proxy< ActorData > actorData;
	if (resourceManager && m_actor)
		resourceManager->bind(m_actor, actorData);
	return new ActorComponent(m_actor, actorData);
}

int32_t ActorComponentData::getOrdinal() const
{
	return 0;
}

void ActorComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void ActorComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< ActorData >(L"actor", m_actor);
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ActorComponent.h"
#include "ActorData.h"

#include "World/Entity.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.ActorComponent", ActorComponent, world::IEntityComponent)

ActorComponent::ActorComponent(const Guid& actor, const resource::Proxy< ActorData >& actorData)
:	m_actor(actor)
,	m_actorData(actorData)
{
}

void ActorComponent::destroy()
{
	m_owner = nullptr;
}

void ActorComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
}

void ActorComponent::setTransform(const Transform& transform)
{
}

Aabb3 ActorComponent::getBoundingBox() const
{
	return Aabb3();
}

void ActorComponent::update(const world::UpdateParams& update)
{
}

const ActorData* ActorComponent::getActorData() const
{
	return m_actorData.getResource();
}

std::wstring ActorComponent::getName() const
{
	const ActorData* actorData = getActorData();
	return actorData ? actorData->getName() : L"";
}

const PropertyGroup* ActorComponent::getProperties() const
{
	const ActorData* actorData = getActorData();
	return actorData ? actorData->getProperties() : nullptr;
}

}

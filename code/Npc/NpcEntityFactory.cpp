/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/NpcEntityFactory.h"

#include "Core/Misc/ObjectStore.h"
#include "Npc/NpcComponent.h"
#include "Npc/NpcComponentData.h"
#include "Npc/NpcWorldComponent.h"
#include "Npc/NpcWorldComponentData.h"
#include "Npc/RouteComponent.h"
#include "Npc/RouteComponentData.h"
#include "Physics/PhysicsManager.h"
#include "Resource/IResourceManager.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.NpcEntityFactory", 0, NpcEntityFactory, world::AbstractEntityFactory)

bool NpcEntityFactory::initialize(const ObjectStore& objectStore)
{
	m_resourceManager = objectStore.get< resource::IResourceManager >();
	m_physicsManager = objectStore.get< physics::PhysicsManager >();
	return true;
}

const TypeInfoSet NpcEntityFactory::getEntityComponentTypes() const
{
	return makeTypeInfoSet< NpcComponentData, RouteComponentData >();
}

Ref< world::IEntityComponent > NpcEntityFactory::createEntityComponent(const world::IEntityBuilder* builder, const world::IEntityComponentData& entityComponentData) const
{
	if (auto npcComponentData = dynamic_type_cast< const NpcComponentData* >(&entityComponentData))
		return npcComponentData->createComponent();
	else if (auto routeComponentData = dynamic_type_cast< const RouteComponentData* >(&entityComponentData))
		return routeComponentData->createComponent();
	else
		return nullptr;
}

const TypeInfoSet NpcEntityFactory::getWorldComponentTypes() const
{
	return makeTypeInfoSet< NpcWorldComponentData >();
}

Ref< world::IWorldComponent > NpcEntityFactory::createWorldComponent(const world::IEntityBuilder* builder, const world::IWorldComponentData& worldComponentData) const
{
	if (auto npcWorldComponentData = dynamic_type_cast< const NpcWorldComponentData* >(&worldComponentData))
		return npcWorldComponentData->createComponent(m_resourceManager, m_physicsManager);
	else
		return nullptr;
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "StoryEntityFactory.h"

#include "ActorComponent.h"
#include "ActorComponentData.h"
#include "Core/Misc/ObjectStore.h"
#include "ItemContainerComponent.h"
#include "ItemContainerComponentData.h"
#include "ItemPickupComponent.h"
#include "ItemPickupComponentData.h"
#include "PlaceComponent.h"
#include "PlaceComponentData.h"
#include "Resource/IResourceManager.h"
#include "StoryWorldComponent.h"
#include "StoryWorldComponentData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.StoryEntityFactory", 0, StoryEntityFactory, world::AbstractEntityFactory)

bool StoryEntityFactory::initialize(const ObjectStore& objectStore)
{
	m_resourceManager = objectStore.get< resource::IResourceManager >();
	return true;
}

const TypeInfoSet StoryEntityFactory::getEntityComponentTypes() const
{
	return makeTypeInfoSet< ActorComponentData, ItemContainerComponentData, ItemPickupComponentData, PlaceComponentData >();
}

Ref< world::IEntityComponent > StoryEntityFactory::createEntityComponent(const world::IEntityBuilder* builder, const world::IEntityComponentData& entityComponentData) const
{
	if (auto itemPickupComponentData = dynamic_type_cast< const ItemPickupComponentData* >(&entityComponentData))
		return itemPickupComponentData->createComponent();
	else if (auto itemContainerComponentData = dynamic_type_cast< const ItemContainerComponentData* >(&entityComponentData))
		return itemContainerComponentData->createComponent();
	else if (auto actorComponentData = dynamic_type_cast< const ActorComponentData* >(&entityComponentData))
		return actorComponentData->createComponent(m_resourceManager);
	else if (auto placeComponentData = dynamic_type_cast< const PlaceComponentData* >(&entityComponentData))
		return placeComponentData->createComponent();
	else
		return nullptr;
}

const TypeInfoSet StoryEntityFactory::getWorldComponentTypes() const
{
	return makeTypeInfoSet< StoryWorldComponentData >();
}

Ref< world::IWorldComponent > StoryEntityFactory::createWorldComponent(const world::IEntityBuilder* builder, const world::IWorldComponentData& worldComponentData) const
{
	if (auto storyWorldComponentData = dynamic_type_cast< const StoryWorldComponentData* >(&worldComponentData))
		return storyWorldComponentData->createComponent();
	else
		return nullptr;
}

}

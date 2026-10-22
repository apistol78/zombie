/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/InteractionEntityFactory.h"

#include "Interaction/InteractableComponent.h"
#include "Interaction/InteractableComponentData.h"
#include "Interaction/InteractionWorldComponent.h"
#include "Interaction/InteractionWorldComponentData.h"
#include "Interaction/LeverComponent.h"
#include "Interaction/LeverComponentData.h"
#include "Interaction/LockComponent.h"
#include "Interaction/LockComponentData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.InteractionEntityFactory", 0, InteractionEntityFactory, world::AbstractEntityFactory)

const TypeInfoSet InteractionEntityFactory::getEntityComponentTypes() const
{
	return makeTypeInfoSet< InteractableComponentData, LeverComponentData, LockComponentData >();
}

Ref< world::IEntityComponent > InteractionEntityFactory::createEntityComponent(const world::IEntityBuilder* builder, const world::IEntityComponentData& entityComponentData) const
{
	if (auto interactableComponentData = dynamic_type_cast< const InteractableComponentData* >(&entityComponentData))
		return interactableComponentData->createComponent();
	else if (auto leverComponentData = dynamic_type_cast< const LeverComponentData* >(&entityComponentData))
		return leverComponentData->createComponent();
	else if (auto lockComponentData = dynamic_type_cast< const LockComponentData* >(&entityComponentData))
		return lockComponentData->createComponent();
	else
		return nullptr;
}

const TypeInfoSet InteractionEntityFactory::getWorldComponentTypes() const
{
	return makeTypeInfoSet< InteractionWorldComponentData >();
}

Ref< world::IWorldComponent > InteractionEntityFactory::createWorldComponent(const world::IEntityBuilder* builder, const world::IWorldComponentData& worldComponentData) const
{
	if (auto interactionWorldComponentData = dynamic_type_cast< const InteractionWorldComponentData* >(&worldComponentData))
		return interactionWorldComponentData->createComponent();
	else
		return nullptr;
}

}

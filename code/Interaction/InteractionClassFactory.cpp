/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/InteractionClassFactory.h"

#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/Boxes/BoxedGuid.h"
#include "Core/Class/Boxes/BoxedVector4.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Interaction/InteractableComponent.h"
#include "Interaction/InteractionWorldComponent.h"
#include "Interaction/LeverComponent.h"
#include "Interaction/LockComponent.h"
#include "Inventory.h"
#include "World/Entity.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.InteractionClassFactory", 0, InteractionClassFactory, IRuntimeClassFactory)

void InteractionClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	auto classInteractableComponent = new AutoRuntimeClass< InteractableComponent >();
	classInteractableComponent->addProperty("owner", &InteractableComponent::getOwner);
	classInteractableComponent->addProperty("enabled", &InteractableComponent::setEnabled, &InteractableComponent::isEnabled);
	classInteractableComponent->addProperty("prompt", &InteractableComponent::setPrompt, &InteractableComponent::getPrompt);
	classInteractableComponent->addProperty("notice", &InteractableComponent::getNotice);
	classInteractableComponent->addProperty("priority", &InteractableComponent::getPriority);
	classInteractableComponent->addMethod("getRefusal", &InteractableComponent::getRefusal);
	registrar->registerClass(classInteractableComponent);

	auto classLockComponent = new AutoRuntimeClass< LockComponent >();
	classLockComponent->addProperty("item", &LockComponent::getItem);
	registrar->registerClass(classLockComponent);

	auto classLeverComponent = new AutoRuntimeClass< LeverComponent >();
	classLeverComponent->addProperty("target", &LeverComponent::getTarget);
	registrar->registerClass(classLeverComponent);

	auto classInteractionWorldComponent = new AutoRuntimeClass< InteractionWorldComponent >();
	classInteractionWorldComponent->addMethod("findUsable", &InteractionWorldComponent::findUsable);
	classInteractionWorldComponent->addMethod("findRefused", &InteractionWorldComponent::findRefused);
	classInteractionWorldComponent->addMethod("findSpendable", &InteractionWorldComponent::findSpendable);
	classInteractionWorldComponent->addMethod("findWanted", &InteractionWorldComponent::findWanted);
	classInteractionWorldComponent->addMethod("countPickups", &InteractionWorldComponent::countPickups);
	registrar->registerClass(classInteractionWorldComponent);
}

}

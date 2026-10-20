/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/NpcClassFactory.h"

#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/Boxes/BoxedVector4.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Core/Class/IRuntimeDelegate.h"
#include "Npc/EnemyComponent.h"
#include "Npc/HumanComponent.h"
#include "Npc/NpcWorldComponent.h"
#include "Runtime/Engine/DebugLayer.h"
#include "World/Entity.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.NpcClassFactory", 0, NpcClassFactory, IRuntimeClassFactory)

void NpcClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	auto classNpcComponent = new AutoRuntimeClass< NpcComponent >();
	classNpcComponent->addProperty("class", &NpcComponent::getClass);
	classNpcComponent->addProperty("faction", &NpcComponent::getFaction);
	classNpcComponent->addProperty("name", &NpcComponent::getName);
	classNpcComponent->addProperty("mood", &NpcComponent::getMood);
	classNpcComponent->addProperty("health", &NpcComponent::getHealth);
	classNpcComponent->addProperty("healthFraction", &NpcComponent::getHealthFraction);
	classNpcComponent->addProperty("hostility", &NpcComponent::getHostility);
	classNpcComponent->addProperty("state", &NpcComponent::getState);
	classNpcComponent->addMethod("hatesPlayer", &NpcComponent::hatesPlayer);
	classNpcComponent->addMethod("takeDamage", &NpcComponent::takeDamage);
	classNpcComponent->addMethod("hit", &NpcComponent::hit);
	classNpcComponent->addMethod("flinch", &NpcComponent::flinch);
	classNpcComponent->addMethod("kill", &NpcComponent::kill);
	classNpcComponent->addMethod("restore", &NpcComponent::restore);
	registrar->registerClass(classNpcComponent);

	auto classEnemyComponent = new AutoRuntimeClass< EnemyComponent >();
	classEnemyComponent->addMethod("alertTo", &EnemyComponent::alertTo);
	classEnemyComponent->addMethod("hunt", &EnemyComponent::hunt);
	registrar->registerClass(classEnemyComponent);

	auto classHumanComponent = new AutoRuntimeClass< HumanComponent >();
	classHumanComponent->addMethod("canTalk", &HumanComponent::canTalk);
	classHumanComponent->addMethod("talk", &HumanComponent::talk);
	registrar->registerClass(classHumanComponent);

	auto classNpcWorldComponent = new AutoRuntimeClass< NpcWorldComponent >();
	classNpcWorldComponent->addMethod("setFrozen", &NpcWorldComponent::setFrozen);
	classNpcWorldComponent->addMethod("setCutscene", &NpcWorldComponent::setCutscene);
	classNpcWorldComponent->addMethod("clearCutscene", &NpcWorldComponent::clearCutscene);
	classNpcWorldComponent->addMethod("setEye", &NpcWorldComponent::setEye);
	classNpcWorldComponent->addMethod("setAim", &NpcWorldComponent::setAim);
	classNpcWorldComponent->addMethod("setGunshot", &NpcWorldComponent::setGunshot);
	classNpcWorldComponent->addMethod("setDebug", &NpcWorldComponent::setDebug);
	classNpcWorldComponent->addMethod("setDamageHandler", &NpcWorldComponent::setDamageHandler);
	classNpcWorldComponent->addMethod("consumePlayerDamage", &NpcWorldComponent::consumePlayerDamage);
	registrar->registerClass(classNpcWorldComponent);
}

}

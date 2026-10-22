/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/FixtureClassFactory.h"

#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/Boxes/BoxedGuid.h"
#include "Core/Class/Boxes/BoxedRefArray.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Core/Settings/PropertyGroup.h"
#include "Fixtures/FixtureComponent.h"
#include "Fixtures/FixtureWorldComponent.h"
#include "Fixtures/LandmarkComponent.h"
#include "Fixtures/StageExitComponent.h"
#include "World/Entity.h"

namespace zombie
{
	namespace
	{

Any StageExitComponent_getArrival(StageExitComponent* self)
{
	const Guid& arrival = self->getArrival();
	return (arrival.isValid() && arrival.isNotNull()) ? CastAny< Guid >::set(arrival) : Any();
}

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.FixtureClassFactory", 0, FixtureClassFactory, IRuntimeClassFactory)

void FixtureClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	auto classFixtureComponent = new AutoRuntimeClass< FixtureComponent >();
	classFixtureComponent->addProperty("owner", &FixtureComponent::getOwner);
	classFixtureComponent->addProperty("done", &FixtureComponent::setDone, &FixtureComponent::isDone);
	registrar->registerClass(classFixtureComponent);

	auto classFixtureWorldComponent = new AutoRuntimeClass< FixtureWorldComponent >();
	classFixtureWorldComponent->addMethod("getFixtures", &FixtureWorldComponent::getFixtures);
	classFixtureWorldComponent->addMethod("saveState", &FixtureWorldComponent::saveState);
	classFixtureWorldComponent->addMethod("restoreState", &FixtureWorldComponent::restoreState);
	registrar->registerClass(classFixtureWorldComponent);

	auto classLandmarkComponent = new AutoRuntimeClass< LandmarkComponent >();
	classLandmarkComponent->addProperty("glyph", &LandmarkComponent::getGlyph);
	registrar->registerClass(classLandmarkComponent);

	auto classStageExitComponent = new AutoRuntimeClass< StageExitComponent >();
	classStageExitComponent->addProperty("target", &StageExitComponent::getTarget);
	classStageExitComponent->addProperty("arrival", &StageExitComponent_getArrival);
	registrar->registerClass(classStageExitComponent);
}

}

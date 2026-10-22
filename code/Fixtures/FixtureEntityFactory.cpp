/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/FixtureEntityFactory.h"

#include "Fixtures/FixtureComponent.h"
#include "Fixtures/FixtureComponentData.h"
#include "Fixtures/FixtureWorldComponent.h"
#include "Fixtures/FixtureWorldComponentData.h"
#include "Fixtures/LandmarkComponent.h"
#include "Fixtures/LandmarkComponentData.h"
#include "Fixtures/StageExitComponent.h"
#include "Fixtures/StageExitComponentData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.FixtureEntityFactory", 0, FixtureEntityFactory, world::AbstractEntityFactory)

const TypeInfoSet FixtureEntityFactory::getEntityComponentTypes() const
{
	return makeTypeInfoSet< FixtureComponentData, LandmarkComponentData, StageExitComponentData >();
}

Ref< world::IEntityComponent > FixtureEntityFactory::createEntityComponent(const world::IEntityBuilder* builder, const world::IEntityComponentData& entityComponentData) const
{
	if (auto fixtureComponentData = dynamic_type_cast< const FixtureComponentData* >(&entityComponentData))
		return fixtureComponentData->createComponent();
	else if (auto landmarkComponentData = dynamic_type_cast< const LandmarkComponentData* >(&entityComponentData))
		return landmarkComponentData->createComponent();
	else if (auto stageExitComponentData = dynamic_type_cast< const StageExitComponentData* >(&entityComponentData))
		return stageExitComponentData->createComponent();
	else
		return nullptr;
}

const TypeInfoSet FixtureEntityFactory::getWorldComponentTypes() const
{
	return makeTypeInfoSet< FixtureWorldComponentData >();
}

Ref< world::IWorldComponent > FixtureEntityFactory::createWorldComponent(const world::IEntityBuilder* builder, const world::IWorldComponentData& worldComponentData) const
{
	if (auto fixtureWorldComponentData = dynamic_type_cast< const FixtureWorldComponentData* >(&worldComponentData))
		return fixtureWorldComponentData->createComponent();
	else
		return nullptr;
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Minimap/MinimapEntityFactory.h"

#include "Core/Misc/ObjectStore.h"
#include "Minimap/MinimapWorldComponent.h"
#include "Minimap/MinimapWorldComponentData.h"
#include "Resource/IResourceManager.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.MinimapEntityFactory", 0, MinimapEntityFactory, world::AbstractEntityFactory)

bool MinimapEntityFactory::initialize(const ObjectStore& objectStore)
{
	m_resourceManager = objectStore.get< resource::IResourceManager >();
	return m_resourceManager != nullptr;
}

const TypeInfoSet MinimapEntityFactory::getWorldComponentTypes() const
{
	return makeTypeInfoSet< MinimapWorldComponentData >();
}

Ref< world::IWorldComponent > MinimapEntityFactory::createWorldComponent(const world::IEntityBuilder* builder, const world::IWorldComponentData& worldComponentData) const
{
	if (auto minimapWorldComponentData = dynamic_type_cast< const MinimapWorldComponentData* >(&worldComponentData))
		return minimapWorldComponentData->createComponent(m_resourceManager);
	else
		return nullptr;
}

}

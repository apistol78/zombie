/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Minimap/MinimapClassFactory.h"

#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/Boxes/BoxedAabb2.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Minimap/MinimapWorldComponent.h"
#include "Render/ITexture.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.MinimapClassFactory", 0, MinimapClassFactory, IRuntimeClassFactory)

void MinimapClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	auto classMinimapWorldComponent = new AutoRuntimeClass< MinimapWorldComponent >();
	classMinimapWorldComponent->addProperty("texture", &MinimapWorldComponent::getTexture);
	classMinimapWorldComponent->addProperty("bounds", &MinimapWorldComponent::getBounds);
	registrar->registerClass(classMinimapWorldComponent);
}

}

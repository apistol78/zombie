/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "SettingsResourceFactory.h"

#include "Core/Settings/PropertyGroup.h"
#include "Database/Instance.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.SettingsResourceFactory", 0, SettingsResourceFactory, resource::IResourceFactory)

bool SettingsResourceFactory::initialize(const ObjectStore& objectStore)
{
	return true;
}

const TypeInfoSet SettingsResourceFactory::getResourceTypes() const
{
	return makeTypeInfoSet< PropertyGroup >();
}

const TypeInfoSet SettingsResourceFactory::getProductTypes(const TypeInfo& resourceType) const
{
	return makeTypeInfoSet< PropertyGroup >();
}

bool SettingsResourceFactory::isCacheable(const TypeInfo& productType) const
{
	return true;
}

Ref< Object > SettingsResourceFactory::create(resource::IResourceManager* resourceManager, const db::Database* database, const db::Instance* instance, const TypeInfo& productType, const Object* current) const
{
	return instance->getObject< PropertyGroup >();
}

void SettingsResourceFactory::destroy(Object* resource) const
{
}

}

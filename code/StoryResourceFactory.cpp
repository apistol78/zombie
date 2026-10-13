/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "StoryResourceFactory.h"

#include "ActorData.h"
#include "Database/Instance.h"
#include "DialogData.h"
#include "ItemData.h"
#include "PlaceData.h"
#include "QuestData.h"
#include "StoryData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.StoryResourceFactory", 0, StoryResourceFactory, resource::IResourceFactory)

bool StoryResourceFactory::initialize(const ObjectStore& objectStore)
{
	return true;
}

const TypeInfoSet StoryResourceFactory::getResourceTypes() const
{
	return makeTypeInfoSet< ActorData, DialogData, ItemData, PlaceData, QuestData, StoryData >();
}

const TypeInfoSet StoryResourceFactory::getProductTypes(const TypeInfo& resourceType) const
{
	return makeTypeInfoSet(resourceType);
}

bool StoryResourceFactory::isCacheable(const TypeInfo& productType) const
{
	// Story data is immutable and shared by everyone.
	return true;
}

Ref< Object > StoryResourceFactory::create(resource::IResourceManager* resourceManager, const db::Database* database, const db::Instance* instance, const TypeInfo& productType, const Object* current) const
{
	return instance->getObject();
}

void StoryResourceFactory::destroy(Object* resource) const
{
}

}

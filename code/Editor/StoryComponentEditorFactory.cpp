/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryComponentEditorFactory.h"

#include "ActorComponentData.h"
#include "Editor/StoryComponentEditor.h"
#include "ItemContainerComponentData.h"
#include "ItemPickupComponentData.h"
#include "PlaceComponentData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.StoryComponentEditorFactory", StoryComponentEditorFactory, scene::IComponentEditorFactory)

const TypeInfoSet StoryComponentEditorFactory::getComponentDataTypes() const
{
	return makeTypeInfoSet< ActorComponentData, PlaceComponentData, ItemPickupComponentData, ItemContainerComponentData >();
}

bool StoryComponentEditorFactory::alwaysRebuild(const world::IEntityComponentData* componentData) const
{
	return false;
}

Ref< scene::IComponentEditor > StoryComponentEditorFactory::createComponentEditor(scene::SceneEditorContext* context, scene::EntityAdapter* entityAdapter, world::IEntityComponentData* componentData) const
{
	return new StoryComponentEditor(context, entityAdapter, componentData);
}

}

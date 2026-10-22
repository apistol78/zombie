/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StorySceneEditorPlugin.h"

#include "Editor/StoryComponentEditor.h"
#include "Editor/StoryComponentEditorFactory.h"
#include "Editor/StorySceneEditorUIExtension.h"
#include "Fixtures/FixtureEntityFactory.h"
#include "Interaction/InteractionEntityFactory.h"
#include "StoryEntityFactory.h"
#include "StoryResourceFactory.h"
#include "Ui/Command.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.StorySceneEditorPlugin", 0, StorySceneEditorPlugin, scene::ISceneEditorPlugin)

void StorySceneEditorPlugin::getCommands(std::list< ui::Command >& outCommands) const
{
	outCommands.push_back(ui::Command(L"Story.Scene.Quest"));
}

void StorySceneEditorPlugin::getGuideDrawIds(std::set< std::wstring >& outIds) const
{
	outIds.insert(StoryComponentEditor::GuideId);
}

void StorySceneEditorPlugin::createUIExtensions(scene::SceneEditorContext* context, RefArray< scene::ISceneEditorUIExtension >& outUIExtensions) const
{
	outUIExtensions.push_back(new StorySceneEditorUIExtension(context));
}

void StorySceneEditorPlugin::createResourceFactories(scene::SceneEditorContext* context, RefArray< const resource::IResourceFactory >& outResourceFactories) const
{
	outResourceFactories.push_back(new StoryResourceFactory());
}

void StorySceneEditorPlugin::createEntityFactories(scene::SceneEditorContext* context, RefArray< world::IEntityFactory >& outEntityFactories) const
{
	outEntityFactories.push_back(new StoryEntityFactory());
	outEntityFactories.push_back(new InteractionEntityFactory());
	outEntityFactories.push_back(new FixtureEntityFactory());
}

void StorySceneEditorPlugin::createEntityRenderers(
	scene::SceneEditorContext* context,
	render::IRenderView* renderView,
	render::PrimitiveRenderer* primitiveRenderer,
	const TypeInfo& worldRendererType,
	RefArray< world::IEntityRenderer >& outEntityRenderers) const
{
}

void StorySceneEditorPlugin::createComponentPanelEditorFactories(scene::SceneEditorContext* context, RefArray< const scene::IComponentPanelEditorFactory >& outComponentPanelEditorFactories) const
{
}

void StorySceneEditorPlugin::createEntityEditorFactories(scene::SceneEditorContext* context, RefArray< const scene::IEntityEditorFactory >& outEntityEditorFactories) const
{
}

void StorySceneEditorPlugin::createComponentEditorFactories(scene::SceneEditorContext* context, RefArray< const scene::IComponentEditorFactory >& outComponentEditorFactories) const
{
	outComponentEditorFactories.push_back(new StoryComponentEditorFactory());
}

Ref< world::EntityData > StorySceneEditorPlugin::createEntityData(scene::SceneEditorContext* context, db::Instance* instance) const
{
	return nullptr;
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Guid.h"
#include "Core/Ref.h"
#include "Scene/Editor/ISceneEditorUIExtension.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::scene
{

class SceneEditorContext;

}

namespace traktor::ui
{

class ToolBarDropDown;

}

namespace zombie
{

using namespace traktor;

/*! A quest to highlight, picked in the scene editor's toolbar.
 * \ingroup Zombie
 */
class T_DLLCLASS StorySceneEditorUIExtension : public scene::ISceneEditorUIExtension
{
	T_RTTI_CLASS;

public:
	explicit StorySceneEditorUIExtension(scene::SceneEditorContext* context);

	virtual bool create(ui::Widget* parent, ui::ToolBar* toolBar) override final;

	virtual bool handleCommand(const ui::Command& command) override final;

	virtual void handleDatabaseEvent(db::Database* database, const Guid& eventId) override final;

private:
	scene::SceneEditorContext* m_context;
	Ref< ui::ToolBarDropDown > m_dropDownQuest;
	AlignedVector< Guid > m_quests;

	void updateQuests();
};

}

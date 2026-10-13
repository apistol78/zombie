/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StorySceneEditorUIExtension.h"

#include "Core/Misc/String.h"
#include "Database/Database.h"
#include "Database/Group.h"
#include "Database/Instance.h"
#include "Database/Traverse.h"
#include "Editor/StoryGuides.h"
#include "I18N/Text.h"
#include "QuestData.h"
#include "Scene/Editor/SceneEditorContext.h"
#include "Ui/Command.h"
#include "Ui/ToolBar/ToolBar.h"
#include "Ui/ToolBar/ToolBarDropDown.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.StorySceneEditorUIExtension", StorySceneEditorUIExtension, scene::ISceneEditorUIExtension)

StorySceneEditorUIExtension::StorySceneEditorUIExtension(scene::SceneEditorContext* context)
:	m_context(context)
{
}

bool StorySceneEditorUIExtension::create(ui::Widget* parent, ui::ToolBar* toolBar)
{
	m_dropDownQuest = new ui::ToolBarDropDown(ui::Command(L"Story.Scene.Quest"), 220_ut, i18n::Text(L"STORY_SCENE_QUEST"));
	toolBar->addItem(m_dropDownQuest);
	updateQuests();
	return true;
}

bool StorySceneEditorUIExtension::handleCommand(const ui::Command& command)
{
	if (command != L"Story.Scene.Quest")
		return false;

	const int32_t selected = m_dropDownQuest->getSelected();
	const Guid quest = (selected > 0 && selected <= (int32_t)m_quests.size()) ? m_quests[selected - 1] : Guid();
	StoryGuides::get(m_context)->setQuest(m_context->getSourceDatabase(), quest);
	m_context->enqueueRedraw(nullptr);
	return true;
}

void StorySceneEditorUIExtension::handleDatabaseEvent(db::Database* database, const Guid& eventId)
{
	if (database != m_context->getSourceDatabase())
		return;

	// Quests may have been added, renamed or edited.
	updateQuests();
	StoryGuides* guides = StoryGuides::get(m_context);
	guides->setQuest(database, guides->getQuest());
	m_context->enqueueRedraw(nullptr);
}

void StorySceneEditorUIExtension::updateQuests()
{
	db::Database* database = m_context->getSourceDatabase();
	const Guid current = StoryGuides::get(m_context)->getQuest();

	m_dropDownQuest->removeAll();
	m_dropDownQuest->add(i18n::Text(L"STORY_SCENE_NO_QUEST"));
	m_quests.clear();

	RefArray< db::Instance > quests;
	if (database)
		db::recursiveFindChildInstances(database->getRootGroup(), db::FindInstanceByType(type_of< QuestData >()), quests);
	quests.sort([](const db::Instance* lh, const db::Instance* rh) {
		return compareIgnoreCase(lh->getName(), rh->getName()) < 0;
	});

	int32_t selected = 0;
	for (auto quest : quests)
	{
		m_dropDownQuest->add(quest->getName());
		m_quests.push_back(quest->getGuid());
		if (quest->getGuid() == current)
			selected = (int32_t)m_quests.size();
	}
	m_dropDownQuest->select(selected);
}

}

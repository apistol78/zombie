/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/QuestEditorPage.h"

#include "Actions/AdvanceQuestAction.h"
#include "Actions/FinishQuestAction.h"
#include "Actions/StartQuestAction.h"
#include "ActorData.h"
#include "Conditions/QuestCondition.h"
#include "Core/Misc/SafeDestroy.h"
#include "Core/Misc/String.h"
#include "Core/Serialization/DeepClone.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Database/Database.h"
#include "Database/Group.h"
#include "Database/Instance.h"
#include "Database/Traverse.h"
#include "Editor/DialogAsset.h"
#include "Editor/IDocument.h"
#include "Editor/IEditor.h"
#include "Editor/IEditorPageSite.h"
#include "Editor/PropertiesView.h"
#include "Editor/QuestClipboardData.h"
#include "Editor/StoryReferences.h"
#include "I18N/Format.h"
#include "I18N/Text.h"
#include "ItemData.h"
#include "PlaceData.h"
#include "QuestData.h"
#include "Steps/FindItemStep.h"
#include "Steps/ReachStep.h"
#include "Steps/TalkStep.h"
#include "Steps/TriggerStep.h"
#include "StoryData.h"
#include "Ui/Application.h"
#include "Ui/Clipboard.h"
#include "Ui/Container.h"
#include "Ui/Events/ContentChangeEvent.h"
#include "Ui/Events/MouseButtonDownEvent.h"
#include "Ui/Events/MouseDoubleClickEvent.h"
#include "Ui/Events/SelectionChangeEvent.h"
#include "Ui/Graph/DefaultNodeShape.h"
#include "Ui/Graph/GraphControl.h"
#include "Ui/Graph/Node.h"
#include "Ui/Graph/NodeActivateEvent.h"
#include "Ui/Graph/NodeMovedEvent.h"
#include "Ui/ListBox/ListBox.h"
#include "Ui/Menu.h"
#include "Ui/MenuItem.h"
#include "Ui/TableLayout.h"
#include "Ui/ToolBar/ToolBar.h"
#include "Ui/ToolBar/ToolBarButton.h"
#include "Ui/ToolBar/ToolBarButtonClickEvent.h"
#include "Ui/ToolBar/ToolBarSeparator.h"

#include <algorithm>

namespace zombie
{
	namespace
	{

const int32_t c_stepSpacing = 110;
const int32_t c_maxExcerpt = 48;

std::wstring excerpt(const std::wstring& text, int32_t length)
{
	if ((int32_t)text.length() <= length)
		return text;
	return text.substr(0, length - 3) + L"...";
}

std::wstring kindOf(const QuestStep* step)
{
	if (is_a< FindItemStep >(step))
		return i18n::Text(L"STORY_QUEST_KIND_FIND_ITEM");
	else if (is_a< TalkStep >(step))
		return i18n::Text(L"STORY_QUEST_KIND_TALK");
	else if (is_a< ReachStep >(step))
		return i18n::Text(L"STORY_QUEST_KIND_REACH");
	else if (is_a< TriggerStep >(step))
		return i18n::Text(L"STORY_QUEST_KIND_TRIGGER");

	std::wstring name = type_name(step);
	const size_t dot = name.find_last_of(L'.');
	return (dot != name.npos) ? name.substr(dot + 1) : name;
}

/*! What a step is about: the item, actor or place, or nothing. */
Guid targetOf(const QuestStep* step)
{
	if (auto findItemStep = dynamic_type_cast< const FindItemStep* >(step))
		return findItemStep->getItem();
	else if (auto talkStep = dynamic_type_cast< const TalkStep* >(step))
		return talkStep->getActor();
	else if (auto reachStep = dynamic_type_cast< const ReachStep* >(step))
		return reachStep->getPlace();
	return Guid();
}

std::wstring stateOf(QuestCondition::State state)
{
	switch (state)
	{
	case QuestCondition::State::NotStarted:
		return i18n::Text(L"STORY_QUEST_STATE_NOT_STARTED");
	case QuestCondition::State::Started:
		return i18n::Text(L"STORY_QUEST_STATE_STARTED");
	case QuestCondition::State::Active:
		return i18n::Text(L"STORY_QUEST_STATE_ACTIVE");
	case QuestCondition::State::Done:
		return i18n::Text(L"STORY_QUEST_STATE_DONE");
	case QuestCondition::State::NotDone:
		return i18n::Text(L"STORY_QUEST_STATE_NOT_DONE");
	default:
		return L"";
	}
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.QuestEditorPage", QuestEditorPage, editor::IEditorPage)

QuestEditorPage::QuestEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document)
:	m_editor(editor)
,	m_site(site)
,	m_document(document)
{
}

bool QuestEditorPage::create(ui::Container* parent)
{
	m_quest = m_document->getObject< QuestData >(0);
	if (!m_quest)
		return false;

	m_questId = m_document->getInstance(0)->getGuid();

	Ref< ui::Container > container = new ui::Container();
	container->create(parent, ui::WsNone, new ui::TableLayout(L"100%", L"*,100%", 0_ut, 0_ut));

	m_toolBar = new ui::ToolBar();
	m_toolBar->create(container);
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_QUEST_ADD_FIND_ITEM"), ui::Command(L"Quest.Editor.AddFindItem")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_QUEST_ADD_TALK"), ui::Command(L"Quest.Editor.AddTalk")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_QUEST_ADD_REACH"), ui::Command(L"Quest.Editor.AddReach")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_QUEST_ADD_TRIGGER"), ui::Command(L"Quest.Editor.AddTrigger")));
	m_toolBar->addItem(new ui::ToolBarSeparator());
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_QUEST_MOVE_UP"), ui::Command(L"Quest.Editor.MoveUp")));
	m_toolBar->addItem(new ui::ToolBarButton(i18n::Text(L"STORY_QUEST_MOVE_DOWN"), ui::Command(L"Quest.Editor.MoveDown")));
	m_toolBar->addEventHandler< ui::ToolBarButtonClickEvent >(this, &QuestEditorPage::eventToolBarClick);

	m_graph = new ui::GraphControl();
	m_graph->create(container, ui::WsDoubleBuffer | ui::WsAccelerated);
	m_graph->setText(L"QUEST");
	m_graph->addEventHandler< ui::MouseButtonDownEvent >(this, &QuestEditorPage::eventButtonDown);
	m_graph->addEventHandler< ui::SelectionChangeEvent >(this, &QuestEditorPage::eventSelect);
	m_graph->addEventHandler< ui::NodeMovedEvent >(this, &QuestEditorPage::eventNodeMoved);
	m_graph->addEventHandler< ui::NodeActivateEvent >(this, &QuestEditorPage::eventNodeActivate);

	m_menuPopup = new ui::Menu();
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Quest.Editor.AddFindItem"), i18n::Text(L"STORY_QUEST_ADD_FIND_ITEM")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Quest.Editor.AddTalk"), i18n::Text(L"STORY_QUEST_ADD_TALK")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Quest.Editor.AddReach"), i18n::Text(L"STORY_QUEST_ADD_REACH")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Quest.Editor.AddTrigger"), i18n::Text(L"STORY_QUEST_ADD_TRIGGER")));
	m_menuPopup->add(new ui::MenuItem(L"-"));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Quest.Editor.MoveUp"), i18n::Text(L"STORY_QUEST_MOVE_UP")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Quest.Editor.MoveDown"), i18n::Text(L"STORY_QUEST_MOVE_DOWN")));
	m_menuPopup->add(new ui::MenuItem(L"-"));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Cut"), i18n::Text(L"STORY_DIALOG_CUT")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Copy"), i18n::Text(L"STORY_DIALOG_COPY")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Paste"), i18n::Text(L"STORY_DIALOG_PASTE")));
	m_menuPopup->add(new ui::MenuItem(ui::Command(L"Editor.Delete"), i18n::Text(L"STORY_DIALOG_DELETE")));

	m_propertiesView = m_site->createPropertiesView(parent, {});
	m_propertiesView->addEventHandler< ui::ContentChangeEvent >(this, &QuestEditorPage::eventPropertiesChanged);
	m_site->createAdditionalPanel(m_propertiesView, 400_ut, false);

	// The quest panel: what is wrong with the quest, and what refers to it.
	m_containerQuest = new ui::Container();
	m_containerQuest->create(parent, ui::WsNone, new ui::TableLayout(L"100%", L"100%", 0_ut, 0_ut));
	m_containerQuest->setText(i18n::Text(L"STORY_QUEST_PANEL"));

	m_listQuest = new ui::ListBox();
	m_listQuest->create(m_containerQuest, ui::ListBox::WsSingle);
	m_listQuest->addEventHandler< ui::MouseDoubleClickEvent >(this, &QuestEditorPage::eventQuestPanelDoubleClick);

	m_site->createAdditionalPanel(m_containerQuest, 200_ut, true);

	findReferences();
	rebuildGraph(RefArray< QuestStep >(), true);

	parent->update();
	m_graph->center();

	select(m_quest);
	return true;
}

void QuestEditorPage::destroy()
{
	if (m_propertiesView)
		m_site->destroyAdditionalPanel(m_propertiesView);
	if (m_containerQuest)
		m_site->destroyAdditionalPanel(m_containerQuest);

	safeDestroy(m_propertiesView);
	safeDestroy(m_containerQuest);
	safeDestroy(m_graph);

	m_site = nullptr;
}

bool QuestEditorPage::dropInstance(db::Instance* instance, const ui::Point& position)
{
	return false;
}

bool QuestEditorPage::handleCommand(const ui::Command& command)
{
	if (m_propertiesView->handleCommand(command))
		return true;

	if (command == L"Editor.SelectAll")
		m_graph->selectAllNodes();
	else if (command == L"Editor.Unselect")
		m_graph->deselectAllNodes();
	else if (command == L"Editor.Delete")
		deleteSelection();
	else if (command == L"Editor.Cut" || command == L"Editor.Copy")
		copySelection(command == L"Editor.Cut");
	else if (command == L"Editor.Paste")
		paste();
	else if (command == L"Editor.Undo" || command == L"Editor.Redo")
	{
		if (!(command == L"Editor.Undo" ? m_document->undo() : m_document->redo()))
			return false;

		m_quest = m_document->getObject< QuestData >(0);
		rebuildGraph(RefArray< QuestStep >(), true);
		select(m_quest);
	}
	else if (command == L"Quest.Editor.AddFindItem")
		addStep(type_of< FindItemStep >());
	else if (command == L"Quest.Editor.AddTalk")
		addStep(type_of< TalkStep >());
	else if (command == L"Quest.Editor.AddReach")
		addStep(type_of< ReachStep >());
	else if (command == L"Quest.Editor.AddTrigger")
		addStep(type_of< TriggerStep >());
	else if (command == L"Quest.Editor.MoveUp")
		moveSelection(-1);
	else if (command == L"Quest.Editor.MoveDown")
		moveSelection(1);
	else
		return false;

	m_graph->update();
	return true;
}

void QuestEditorPage::handleDatabaseEvent(db::Database* database, const Guid& eventId)
{
	// Dialogs and quests elsewhere may have come to refer to this quest, or stopped.
	if (database != m_editor->getSourceDatabase() || eventId == m_questId)
		return;

	findReferences();
	rebuildGraph(getSelectedSteps(), false);
}

void QuestEditorPage::rebuildGraph(const RefArray< QuestStep >& select, bool selectQuest)
{
	m_graph->removeAllDependencyHints();
	m_graph->removeAllEdges();
	m_graph->removeAllNodes();

	RefArray< ui::Node > flow;

	// The quest itself at the top.
	ui::Node* questNode = m_graph->createNode(
		!m_quest->getTitle().empty() ? m_quest->getTitle() : std::wstring(i18n::Text(L"STORY_QUEST_UNTITLED")),
		excerpt(m_quest->getInfo(), c_maxExcerpt),
		ui::UnitPoint(0_ut, 0_ut),
		new ui::DefaultNodeShape(ui::DefaultNodeShape::StExternal)
	);
	questNode->setData(L"QUEST", m_quest);
	questNode->setSelected(selectQuest);
	questNode->setState(m_quest->getSteps().empty() ? 1 : 0);
	flow.push_back(questNode);

	// A node per step, in the order they are taken.
	const auto& steps = m_quest->getSteps();
	for (int32_t i = 0; i < (int32_t)steps.size(); ++i)
	{
		const QuestStep* step = steps[i];
		if (!step)
			continue;

		const std::wstring title = !step->getName().empty() ?
			i18n::Format(L"STORY_QUEST_STEP_TITLE_ID", i + 1, kindOf(step), step->getName()).str() :
			i18n::Format(L"STORY_QUEST_STEP_TITLE", i + 1, kindOf(step)).str();

		ui::Node* node = m_graph->createNode(
			title,
			!step->getText().empty() ? excerpt(step->getText(), c_maxExcerpt) : std::wstring(i18n::Text(L"STORY_QUEST_NO_OBJECTIVE")),
			ui::UnitPoint(0_ut, 0_ut),
			new ui::DefaultNodeShape(ui::DefaultNodeShape::StDefault)
		);
		node->setData(L"STEP", const_cast< QuestStep* >(step));
		const bool canFail = step->getFailWhen() != nullptr || !m_quest->getNeededActors(i).empty();
		node->setComment(describeTarget(step) + (canFail ? std::wstring(i18n::Text(L"STORY_QUEST_CAN_FAIL")) : std::wstring()));
		node->setState(checkStep(i).empty() ? 0 : 1);
		node->setSelected(std::find(select.begin(), select.end(), step) != select.end());
		flow.push_back(node);
	}

	// The reward at the end.
	std::wstring reward;
	for (int32_t i = 0; i < (int32_t)m_quest->getReward().size(); ++i)
	{
		std::wstring name = type_name(m_quest->getReward()[i]);
		const size_t dot = name.find_last_of(L'.');
		if (dot != name.npos)
			name = name.substr(dot + 1);
		reward += (!reward.empty() ? L", " : L"") + name;
	}

	ui::Node* rewardNode = m_graph->createNode(
		i18n::Text(L"STORY_QUEST_REWARD"),
		!reward.empty() ? excerpt(reward, c_maxExcerpt) : std::wstring(i18n::Text(L"STORY_QUEST_NO_REWARD")),
		ui::UnitPoint(0_ut, 0_ut),
		new ui::DefaultNodeShape(ui::DefaultNodeShape::StExternal)
	);
	rewardNode->setData(L"REWARD", m_quest);
	flow.push_back(rewardNode);

	// One column, each node centred on it, in order from the top.
	for (int32_t i = 0; i < (int32_t)flow.size(); ++i)
	{
		const ui::UnitRect rc = flow[i]->calculateRect();
		flow[i]->setPosition(ui::UnitPoint(ui::Unit(-rc.getWidth().get() / 2), ui::Unit(i * c_stepSpacing)));
		if (i > 0)
			m_graph->addDependencyHint(flow[i - 1], flow[i]);
	}

	m_graph->update();
	updateQuestPanel();
}

void QuestEditorPage::findReferences()
{
	m_references.clear();

	Ref< db::Database > database = m_editor->getSourceDatabase();
	if (!database)
		return;

	RefArray< db::Instance > instances;
	for (const TypeInfo* type : { &type_of< DialogAsset >(), &type_of< QuestData >(), &type_of< StoryData >(), &type_of< ItemData >(), &type_of< ActorData >() })
		db::recursiveFindChildInstances(database->getRootGroup(), db::FindInstanceByType(*type), instances);

	for (auto instance : instances)
	{
		if (instance->getGuid() == m_questId)
			continue;

		Ref< ISerializable > object = instance->getObject();
		if (!object)
			continue;

		// Objects which start, advance, finish or check this quest, with where they are.
		auto find = [&](const ISerializable* root, const std::wstring& where) {
			visitObjects(root, [&](const ISerializable* child) {
				Guid quest;
				if (auto startQuestAction = dynamic_type_cast< const StartQuestAction* >(child))
					quest = startQuestAction->getQuest();
				else if (auto advanceQuestAction = dynamic_type_cast< const AdvanceQuestAction* >(child))
					quest = advanceQuestAction->getQuest();
				else if (auto finishQuestAction = dynamic_type_cast< const FinishQuestAction* >(child))
					quest = finishQuestAction->getQuest();
				else if (auto questCondition = dynamic_type_cast< const QuestCondition* >(child))
					quest = questCondition->getQuest();
				if (quest == m_questId)
					m_references.push_back({ instance, where, child });
			});
		};

		if (auto dialogAsset = dynamic_type_cast< const DialogAsset* >(object))
		{
			for (int32_t i = 0; i < (int32_t)dialogAsset->start.size(); ++i)
				find(dialogAsset->start[i], i18n::Format(L"STORY_QUEST_WHERE_START", i + 1));
			for (auto node : dialogAsset->nodes)
			{
				const std::wstring line = !node->name.empty() ? node->name : std::wstring(i18n::Text(L"STORY_DIALOG_UNNAMED"));
				for (auto action : node->onEnter)
					find(action, i18n::Format(L"STORY_QUEST_WHERE_LINE", line));
				for (auto response : node->responses)
					find(response, i18n::Format(L"STORY_QUEST_WHERE_RESPONSE", line, excerpt(response->text, c_maxExcerpt)));
			}
		}
		else
			find(object, L"");
	}
}

void QuestEditorPage::updateQuestPanel()
{
	m_listQuest->removeAll();

	// What is wrong with the quest first.
	if (m_quest->getSteps().empty())
		m_listQuest->add(i18n::Text(L"STORY_QUEST_NO_STEPS"));
	for (int32_t i = 0; i < (int32_t)m_quest->getSteps().size(); ++i)
	{
		const std::wstring problem = checkStep(i);
		if (!problem.empty())
			m_listQuest->add(i18n::Format(L"STORY_QUEST_PROBLEM", i + 1, problem));
	}

	// Then what refers to it, checked against the steps as they are now.
	auto stepLabel = [&](const Guid& step) -> std::wstring {
		const int32_t index = m_quest->findStep(step);
		if (index < 0)
			return step.format();
		const std::wstring& name = m_quest->getSteps()[index]->getName();
		return toString(index + 1) + (!name.empty() ? L" " + name : L"");
	};
	auto stepExists = [&](const Guid& step) {
		return m_quest->findStep(step) >= 0;
	};

	for (const auto& reference : m_references)
	{
		std::wstring what;
		if (is_a< StartQuestAction >(reference.object))
			what = i18n::Text(L"STORY_QUEST_REF_STARTS");
		else if (auto advanceQuestAction = dynamic_type_cast< const AdvanceQuestAction* >(reference.object))
		{
			const Guid& step = advanceQuestAction->getStep();
			if (step.isNull())
				what = i18n::Text(L"STORY_QUEST_REF_ADVANCES");
			else
				what = i18n::Format(stepExists(step) ? L"STORY_QUEST_REF_ADVANCES_STEP" : L"STORY_QUEST_REF_ADVANCES_UNKNOWN", stepLabel(step)).str();
		}
		else if (is_a< FinishQuestAction >(reference.object))
			what = i18n::Text(L"STORY_QUEST_REF_FINISHES");
		else if (auto questCondition = dynamic_type_cast< const QuestCondition* >(reference.object))
		{
			if (questCondition->getState() == QuestCondition::State::AtStep)
				what = i18n::Format(stepExists(questCondition->getStep()) ? L"STORY_QUEST_REF_CHECKS_STEP" : L"STORY_QUEST_REF_CHECKS_UNKNOWN", stepLabel(questCondition->getStep())).str();
			else
				what = i18n::Format(L"STORY_QUEST_REF_CHECKS", stateOf(questCondition->getState())).str();
		}

		const std::wstring where = !reference.where.empty() ? reference.instance->getName() + L", " + reference.where : reference.instance->getName();
		m_listQuest->add(where + L": " + what, reference.instance);
	}

	if (m_references.empty())
		m_listQuest->add(i18n::Text(L"STORY_QUEST_NO_REFERENCES"));

	m_listQuest->update();
}

std::wstring QuestEditorPage::checkStep(int32_t index) const
{
	const QuestStep* step = m_quest->getSteps()[index];
	if (!step)
		return L"";

	Ref< db::Database > database = m_editor->getSourceDatabase();
	std::wstring problems;
	auto problem = [&](const std::wstring& text) {
		problems += (!problems.empty() ? L"; " : L"") + text;
	};

	if (step->getId().isNull())
		problem(i18n::Text(L"STORY_QUEST_NO_ID"));
	else
	{
		for (int32_t i = 0; i < (int32_t)m_quest->getSteps().size(); ++i)
		{
			const QuestStep* other = m_quest->getSteps()[i];
			if (i != index && other && other->getId() == step->getId())
			{
				problem(i18n::Text(L"STORY_QUEST_DUPLICATE_ID"));
				break;
			}
		}
	}

	const Guid target = targetOf(step);
	if (is_a< FindItemStep >(step) && target.isNull())
		problem(i18n::Text(L"STORY_QUEST_NO_ITEM"));
	else if (is_a< TalkStep >(step) && target.isNull())
		problem(i18n::Text(L"STORY_QUEST_NO_ACTOR"));
	else if (is_a< ReachStep >(step) && target.isNull())
		problem(i18n::Text(L"STORY_QUEST_NO_PLACE"));
	else if (target.isNotNull() && (!database || !database->getInstance(target)))
		problem(i18n::Text(L"STORY_QUEST_TARGET_MISSING"));

	// A talk step at a line needs the line in the actor's dialog.
	auto talkStep = dynamic_type_cast< const TalkStep* >(step);
	if (talkStep && talkStep->getLine().isNotNull() && target.isNotNull() && database)
	{
		Ref< ActorData > actorData = database->getObjectReadOnly< ActorData >(target);
		if (actorData)
		{
			const Guid& dialog = actorData->getDialog();
			Ref< DialogAsset > dialogAsset = dialog.isNotNull() ? database->getObjectReadOnly< DialogAsset >(dialog) : nullptr;
			if (dialog.isNull())
				problem(i18n::Text(L"STORY_QUEST_ACTOR_NO_DIALOG"));
			else if (!dialogAsset)
				problem(i18n::Text(L"STORY_QUEST_DIALOG_MISSING"));
			else if (!dialogAsset->findNode(talkStep->getLine()))
				problem(i18n::Text(L"STORY_QUEST_DIALOG_NO_LINE"));
		}
	}

	return problems;
}

std::wstring QuestEditorPage::describeTarget(const QuestStep* step) const
{
	if (auto talkStep = dynamic_type_cast< const TalkStep* >(step))
	{
		const std::wstring actor = nameOf(talkStep->getActor());
		if (talkStep->getLine().isNull())
			return actor;

		std::wstring line = talkStep->getLine().format();
		Ref< db::Database > database = m_editor->getSourceDatabase();
		Ref< ActorData > actorData = database ? database->getObjectReadOnly< ActorData >(talkStep->getActor()) : nullptr;
		Ref< DialogAsset > dialogAsset = actorData ? database->getObjectReadOnly< DialogAsset >(actorData->getDialog()) : nullptr;
		if (const DialogNodeAsset* nodeAsset = dialogAsset ? dialogAsset->findNode(talkStep->getLine()) : nullptr)
			line = !nodeAsset->name.empty() ? nodeAsset->name : excerpt(nodeAsset->text, c_maxExcerpt);
		return i18n::Format(L"STORY_QUEST_TALK_AT", actor, line).str();
	}
	else if (auto reachStep = dynamic_type_cast< const ReachStep* >(step))
		return i18n::Format(L"STORY_QUEST_REACH_WITHIN", nameOf(reachStep->getPlace()), str(L"%g", reachStep->getRadius())).str();
	else if (auto findItemStep = dynamic_type_cast< const FindItemStep* >(step))
		return nameOf(findItemStep->getItem());
	else if (is_a< TriggerStep >(step))
		return i18n::Text(L"STORY_QUEST_TRIGGERED");
	return L"";
}

std::wstring QuestEditorPage::nameOf(const Guid& id) const
{
	if (id.isNull())
		return i18n::Text(L"STORY_QUEST_NONE");

	Ref< db::Database > database = m_editor->getSourceDatabase();
	Ref< db::Instance > instance = database ? database->getInstance(id) : nullptr;
	if (!instance)
		return i18n::Text(L"STORY_QUEST_MISSING");

	// The name the player knows it by, if it has one.
	std::wstring name;
	Ref< ISerializable > object = database->getObjectReadOnly(id);
	if (auto itemData = dynamic_type_cast< const ItemData* >(object))
		name = itemData->getName();
	else if (auto actorData = dynamic_type_cast< const ActorData* >(object))
		name = actorData->getName();
	else if (auto placeData = dynamic_type_cast< const PlaceData* >(object))
		name = placeData->getName();

	return !name.empty() ? name : instance->getName();
}

RefArray< QuestStep > QuestEditorPage::getSelectedSteps() const
{
	const RefArray< ui::Node > nodes = m_graph->getSelectedNodes();
	const auto& steps = m_quest->getSteps();

	RefArray< QuestStep > selected;
	for (int32_t i = 0; i < (int32_t)steps.size(); ++i)
	{
		for (int32_t j = 0; j < (int32_t)nodes.size(); ++j)
		{
			if (nodes[j]->getData< QuestStep >(L"STEP") == steps[i])
			{
				selected.push_back(const_cast< QuestStep* >(steps[i]));
				break;
			}
		}
	}
	return selected;
}

void QuestEditorPage::addStep(const TypeInfo& stepType)
{
	Ref< QuestStep > step = dynamic_type_cast< QuestStep* >(stepType.createInstance());
	if (!step)
		return;

	auto& steps = m_quest->getSteps();

	m_document->push();

	// After the last selected step, else last.
	const RefArray< QuestStep > selected = getSelectedSteps();
	auto at = !selected.empty() ? std::find(steps.begin(), steps.end(), selected.back()) + 1 : steps.end();
	steps.insert(at, step);

	RefArray< QuestStep > added;
	added.push_back(step);
	rebuildGraph(added, false);
	select(step);
}

void QuestEditorPage::moveSelection(int32_t direction)
{
	const RefArray< QuestStep > selected = getSelectedSteps();
	if (selected.empty())
		return;

	auto& steps = m_quest->getSteps();
	const int32_t count = (int32_t)steps.size();

	// Nothing moves if any selected step is already at the end it moves towards.
	const int32_t first = (int32_t)(std::find(steps.begin(), steps.end(), selected.front()) - steps.begin());
	const int32_t last = (int32_t)(std::find(steps.begin(), steps.end(), selected.back()) - steps.begin());
	if ((direction < 0 && first <= 0) || (direction > 0 && last >= count - 1))
		return;

	m_document->push();

	// Each selected step swaps with its neighbour, taken from the end it moves towards.
	auto isSelected = [&](const QuestStep* step) {
		return std::find(selected.begin(), selected.end(), step) != selected.end();
	};
	if (direction < 0)
	{
		for (int32_t i = 1; i < count; ++i)
		{
			if (isSelected(steps[i]) && !isSelected(steps[i - 1]))
			{
				Ref< QuestStep > step = steps[i];
				steps[i] = steps[i - 1];
				steps[i - 1] = step.ptr();
			}
		}
	}
	else
	{
		for (int32_t i = count - 2; i >= 0; --i)
		{
			if (isSelected(steps[i]) && !isSelected(steps[i + 1]))
			{
				Ref< QuestStep > step = steps[i];
				steps[i] = steps[i + 1];
				steps[i + 1] = step.ptr();
			}
		}
	}

	rebuildGraph(selected, false);
}

void QuestEditorPage::copySelection(bool cut)
{
	const RefArray< QuestStep > selected = getSelectedSteps();
	if (selected.empty())
		return;

	Ref< QuestClipboardData > data = new QuestClipboardData();
	for (int32_t i = 0; i < (int32_t)selected.size(); ++i)
		data->steps.push_back(DeepClone(selected[i]).create< QuestStep >());
	ui::Application::getInstance()->getClipboard()->setObject(data);

	if (cut)
		deleteSelection();
}

void QuestEditorPage::paste()
{
	Ref< QuestClipboardData > data = dynamic_type_cast< QuestClipboardData* >(ui::Application::getInstance()->getClipboard()->getObject());
	if (!data || data->steps.empty())
		return;

	m_document->push();

	auto& steps = m_quest->getSteps();
	const RefArray< QuestStep > selected = getSelectedSteps();
	int32_t at = !selected.empty() ? (int32_t)(std::find(steps.begin(), steps.end(), selected.back()) - steps.begin()) + 1 : (int32_t)steps.size();

	RefArray< QuestStep > pasted;
	for (int32_t i = 0; i < (int32_t)data->steps.size(); ++i)
	{
		Ref< QuestStep > step = DeepClone(data->steps[i]).create< QuestStep >();
		if (!step)
			continue;

		// A pasted step is a new step; what refers to the copied one keeps referring to it.
		step->setId(Guid::create());

		steps.insert(steps.begin() + at++, step);
		pasted.push_back(step);
	}

	rebuildGraph(pasted, false);
	select(m_quest);
}

void QuestEditorPage::deleteSelection()
{
	const RefArray< QuestStep > selected = getSelectedSteps();
	if (selected.empty())
		return;

	m_document->push();

	for (int32_t i = 0; i < (int32_t)selected.size(); ++i)
		m_quest->getSteps().remove(selected[i]);

	rebuildGraph(RefArray< QuestStep >(), true);
	select(m_quest);
}

void QuestEditorPage::select(ISerializable* object)
{
	m_propertiesView->setPropertyObject(object);
}

void QuestEditorPage::eventToolBarClick(ui::ToolBarButtonClickEvent* event)
{
	handleCommand(event->getCommand());
}

void QuestEditorPage::eventButtonDown(ui::MouseButtonDownEvent* event)
{
	if (event->getButton() != ui::MbtRight)
		return;

	const ui::MenuItem* selected = m_menuPopup->showModal(m_graph, event->getPosition());
	if (selected)
		handleCommand(selected->getCommand());
}

void QuestEditorPage::eventSelect(ui::SelectionChangeEvent* event)
{
	const RefArray< ui::Node > nodes = m_graph->getSelectedNodes();
	QuestStep* step = (nodes.size() == 1) ? nodes[0]->getData< QuestStep >(L"STEP") : nullptr;
	if (step)
		select(step);
	else
		select(m_quest);
}

void QuestEditorPage::eventNodeMoved(ui::NodeMovedEvent* event)
{
	// Steps are taken from the top; a step dragged past another changes places with it. The
	// nodes are kept, only moved back onto the column, as more nodes may be moving still.
	AlignedVector< ui::Node* > stepNodes;
	ui::Node* questNode = nullptr;
	ui::Node* rewardNode = nullptr;
	const RefArray< ui::Node > nodes = m_graph->getNodes();
	for (int32_t i = 0; i < (int32_t)nodes.size(); ++i)
	{
		ui::Node* node = nodes[i];
		if (node->getData< QuestStep >(L"STEP"))
			stepNodes.push_back(node);
		else if (node->getData< QuestData >(L"QUEST"))
			questNode = node;
		else
			rewardNode = node;
	}

	std::stable_sort(stepNodes.begin(), stepNodes.end(), [](const ui::Node* lh, const ui::Node* rh) {
		return lh->getPosition().y.get() < rh->getPosition().y.get();
	});

	auto& steps = m_quest->getSteps();
	bool reordered = false;
	for (int32_t i = 0; i < (int32_t)stepNodes.size() && i < (int32_t)steps.size(); ++i)
		reordered |= (stepNodes[i]->getData< QuestStep >(L"STEP") != steps[i]);

	if (reordered)
	{
		m_document->push();
		for (int32_t i = 0; i < (int32_t)stepNodes.size() && i < (int32_t)steps.size(); ++i)
			steps[i] = stepNodes[i]->getData< QuestStep >(L"STEP");
	}

	AlignedVector< ui::Node* > flow;
	if (questNode)
		flow.push_back(questNode);
	for (auto node : stepNodes)
		flow.push_back(node);
	if (rewardNode)
		flow.push_back(rewardNode);

	m_graph->removeAllDependencyHints();
	for (int32_t i = 0; i < (int32_t)flow.size(); ++i)
	{
		ui::Node* node = flow[i];
		if (const QuestStep* step = node->getData< QuestStep >(L"STEP"))
		{
			node->setTitle(!step->getName().empty() ?
				i18n::Format(L"STORY_QUEST_STEP_TITLE_ID", i, kindOf(step), step->getName()).str() :
				i18n::Format(L"STORY_QUEST_STEP_TITLE", i, kindOf(step)).str());
		}

		const ui::UnitRect rc = node->calculateRect();
		node->setPosition(ui::UnitPoint(ui::Unit(-rc.getWidth().get() / 2), ui::Unit(i * c_stepSpacing)));
		if (i > 0)
			m_graph->addDependencyHint(flow[i - 1], node);
	}

	if (reordered)
		updateQuestPanel();

	m_graph->update();
}

void QuestEditorPage::eventNodeActivate(ui::NodeActivateEvent* event)
{
	const QuestStep* step = event->getNode()->getData< QuestStep >(L"STEP");
	const Guid target = step ? targetOf(step) : Guid();
	if (target.isNull())
		return;

	Ref< db::Instance > instance = m_editor->getSourceDatabase()->getInstance(target);
	if (instance)
		m_editor->openEditor(instance);
}

void QuestEditorPage::eventPropertiesChanged(ui::ContentChangeEvent* event)
{
	// Titles, objectives and what steps are about may have changed.
	const RefArray< ui::Node > nodes = m_graph->getSelectedNodes();
	const bool questSelected = (nodes.size() == 1 && nodes[0]->getData< QuestData >(L"QUEST") != nullptr);
	rebuildGraph(getSelectedSteps(), questSelected);
}

void QuestEditorPage::eventQuestPanelDoubleClick(ui::MouseDoubleClickEvent* event)
{
	Ref< db::Instance > instance = m_listQuest->getSelectedData< db::Instance >();
	if (instance)
		m_editor->openEditor(instance);
}

}

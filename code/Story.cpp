/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Story.h"

#include "ActorData.h"
#include "Conversation.h"
#include "Core/Class/Any.h"
#include "Core/Class/Boxes/BoxedGuid.h"
#include "Core/Class/IRuntimeDelegate.h"
#include "Core/Log/Log.h"
#include "Core/Settings/PropertyGroup.h"
#include "DialogData.h"
#include "IAction.h"
#include "Inventory.h"
#include "InventorySlot.h"
#include "ItemData.h"
#include "PlaceData.h"
#include "QuestData.h"
#include "QuestLog.h"
#include "Resource/IResourceManager.h"
#include "Resource/ResourceHandle.h"
#include "Steps/FindItemStep.h"
#include "StoryData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.Story", Story, Object)

Ref< Story > Story::create(resource::IResourceManager* resourceManager, const Guid& storyData, int32_t capacity)
{
	Ref< Story > story = new Story();
	story->m_resourceManager = resourceManager;

	resource::Proxy< StoryData > data;
	if (!resourceManager || !resourceManager->bind(resource::Id< StoryData >(storyData), data))
	{
		log::error << L"Story: unable to bind story " << storyData.format() << L"." << Endl;
		return nullptr;
	}

	story->m_data = data.getResource();
	story->m_inventory = new Inventory(resourceManager, capacity);
	story->m_questLog = new QuestLog(story);

	for (const auto& stack : story->m_data->getStartKit())
		story->m_inventory->add(stack.item, stack.count);

	return story;
}

Story::~Story()
{
	// The quest log refers back to the story.
	m_questLog = nullptr;
}

AlignedVector< Guid > Story::getQuests() const
{
	AlignedVector< Guid > quests;
	for (const auto& quest : m_data->getQuests())
		quests.push_back(quest);
	return quests;
}

const QuestData* Story::getQuest(const Guid& quest) const
{
	return dynamic_type_cast< const QuestData* >(bind(type_of< QuestData >(), quest));
}

const ActorData* Story::getActor(const Guid& actor) const
{
	return dynamic_type_cast< const ActorData* >(bind(type_of< ActorData >(), actor));
}

const PlaceData* Story::getPlace(const Guid& place) const
{
	return dynamic_type_cast< const PlaceData* >(bind(type_of< PlaceData >(), place));
}

const DialogData* Story::getDialog(const Guid& dialog) const
{
	return dynamic_type_cast< const DialogData* >(bind(type_of< DialogData >(), dialog));
}

Ref< Conversation > Story::talk(const Guid& actor)
{
	const ActorData* actorData = actor.isNotNull() ? getActor(actor) : nullptr;
	if (!actorData || !actorData->getDialog())
		return nullptr;

	const DialogData* dialogData = getDialog(actorData->getDialog());
	if (!dialogData)
		return nullptr;

	const std::wstring& speaker = !dialogData->getName().empty() ? dialogData->getName() : actorData->getName();
	Ref< Conversation > conversation = new Conversation(this, actor, dialogData, speaker);
	conversation->start();
	return conversation;
}

bool Story::isHeldBack(const Guid& item) const
{
	bool wanted = false;
	for (const auto& quest : m_data->getQuests())
	{
		const QuestData* questData = getQuest(quest);
		if (!questData)
			continue;

		const QuestLog* questLog = m_questLog;
		const bool done = questLog->isDone(quest);
		const int32_t current = questLog->getStepIndex(quest);

		for (int32_t i = 0; i < (int32_t)questData->getSteps().size(); ++i)
		{
			const FindItemStep* findItemStep = dynamic_type_cast< const FindItemStep* >(questData->getSteps()[i]);
			if (findItemStep == nullptr || findItemStep->getItem() != item)
				continue;

			// A quest at or past the step has asked for the item; it is not held back from anyone.
			if (done || current >= i)
				return false;

			wanted = true;
		}
	}
	return wanted;
}

void Story::update()
{
	for (int32_t i = 0; i < m_inventory->getCapacity(); ++i)
	{
		const InventorySlot* slot = m_inventory->getSlot(i);
		if (slot != nullptr && slot->getItemData() != nullptr && slot->getItemData()->isQuest())
			m_questLog->found(slot->getItem());
	}
	m_questLog->update();
}

int32_t Story::giveItem(const Guid& item, int32_t count)
{
	const int32_t added = m_inventory->add(item, count);
	notify(L"itemGiven", CastAny< Guid >::set(item), Any::fromInt32(added));
	return added;
}

int32_t Story::takeItem(const Guid& item, int32_t count)
{
	const int32_t removed = m_inventory->remove(item, count);
	notify(L"itemTaken", CastAny< Guid >::set(item), Any::fromInt32(removed));
	return removed;
}

void Story::raiseEvent(const Guid& event, bool endConversation)
{
	if (m_conversation != nullptr)
		m_conversation->noteEvent(event, endConversation);
	notify(L"event", CastAny< Guid >::set(event), Any());
}

void Story::runActions(const RefArray< IAction >& actions)
{
	for (auto action : actions)
	{
		if (action != nullptr)
			action->execute(this);
	}
}

bool Story::evaluateScriptCondition(const Guid& condition) const
{
	if (!m_conditionHandler)
		return false;

	const Any argv[] = { CastAny< Guid >::set(condition) };
	return m_conditionHandler->call(1, argv).getBoolean();
}

bool Story::getDistance(const Guid& place, float& outDistance) const
{
	if (!m_distanceHandler)
		return false;

	const Any argv[] = { CastAny< Guid >::set(place) };
	const Any distance = m_distanceHandler->call(1, argv);
	if (!distance.isNumeric())
		return false;

	outDistance = distance.getFloat();
	return true;
}

void Story::setEventHandler(IRuntimeDelegate* eventHandler)
{
	m_eventHandler = eventHandler;
}

void Story::setConditionHandler(IRuntimeDelegate* conditionHandler)
{
	m_conditionHandler = conditionHandler;
}

void Story::setDistanceHandler(IRuntimeDelegate* distanceHandler)
{
	m_distanceHandler = distanceHandler;
}

void Story::notify(const wchar_t* what, const Any& a, const Any& b) const
{
	if (!m_eventHandler)
		return;

	const Any argv[] = { Any::fromString(what), a, b };
	m_eventHandler->call(3, argv);
}

Ref< PropertyGroup > Story::saveState() const
{
	Ref< PropertyGroup > state = new PropertyGroup();
	state->setProperty(L"inventory", m_inventory->saveState());
	m_questLog->saveState(state);
	return state;
}

void Story::restoreState(const PropertyGroup* state)
{
	if (state == nullptr)
		return;

	m_inventory->restoreState(dynamic_type_cast< const PropertyGroup* >(state->getProperty(L"inventory")));
	m_questLog->restoreState(state);
}

const Object* Story::bind(const TypeInfo& productType, const Guid& id) const
{
	if (id.isNull() || !id.isValid())
		return nullptr;

	const auto it = m_bound.find(id);
	if (it != m_bound.end())
		return it->second ? it->second->get() : nullptr;

	Ref< resource::ResourceHandle > handle = m_resourceManager->bind(productType, id);
	m_bound[id] = handle;
	return handle ? handle->get() : nullptr;
}

}

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
#include "Core/Containers/SmallMap.h"
#include "Core/Guid.h"
#include "Core/Object.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class Any;
class IRuntimeDelegate;
class PropertyGroup;

}

namespace traktor::resource
{

class IResourceManager;
class ResourceHandle;

}

namespace zombie
{

using namespace traktor;

class ActorData;
class Conversation;
class DialogData;
class IAction;
class Inventory;
class PlaceData;
class QuestData;
class QuestLog;
class StoryData;

/*! Story; one per playthrough. Owns the inventory and quest log, and tells and asks the game through handlers.
 * \ingroup Zombie
 */
class T_DLLCLASS Story : public Object
{
	T_RTTI_CLASS;

public:
	/*! Create story for a new playthrough; inventory holds the story's start kit. */
	static Ref< Story > create(resource::IResourceManager* resourceManager, const Guid& storyData, int32_t capacity);

	virtual ~Story();

	resource::IResourceManager* getResourceManager() const { return m_resourceManager; }

	const StoryData* getData() const { return m_data; }

	Inventory* getInventory() const { return m_inventory; }

	QuestLog* getQuestLog() const { return m_questLog; }

	/*! Every quest of the story. */
	AlignedVector< Guid > getQuests() const;

	const QuestData* getQuest(const Guid& quest) const;

	const ActorData* getActor(const Guid& actor) const;

	const PlaceData* getPlace(const Guid& place) const;

	const DialogData* getDialog(const Guid& dialog) const;

	/*! Start a conversation with actor; null if the actor has no dialog. Ended at once if no start holds. */
	Ref< Conversation > talk(const Guid& actor);

	/*! Item a find step waits for but no quest has reached yet; held back from pickups and containers. */
	bool isHeldBack(const Guid& item) const;

	/*! Note quest items in the inventory as found, and complete steps already met. */
	void update();

	/*! Add to inventory; announced as "itemGiven". */
	int32_t giveItem(const Guid& item, int32_t count);

	/*! Remove from inventory; announced as "itemTaken". */
	int32_t takeItem(const Guid& item, int32_t count);

	/*! Raise game event, by the guid of its EventData; announced as "event", and recorded by the conversation being run. */
	void raiseEvent(const Guid& event, bool endConversation);

	void runActions(const RefArray< IAction >& actions);

	/*! Ask the game, by the guid of the ScriptConditionData; false if it has no condition handler. */
	bool evaluateScriptCondition(const Guid& condition) const;

	/*! Distance from player to place, false if the place is not in the player's world. */
	bool getDistance(const Guid& place, float& outDistance) const;

	void setEventHandler(IRuntimeDelegate* eventHandler);

	void setConditionHandler(IRuntimeDelegate* conditionHandler);

	void setDistanceHandler(IRuntimeDelegate* distanceHandler);

	/*! Call event handler. */
	void notify(const wchar_t* what, const Any& a, const Any& b) const;

	/*! Inventory, quests and flags as plain values. */
	Ref< PropertyGroup > saveState() const;

	void restoreState(const PropertyGroup* state);

private:
	friend class Conversation;

	Ref< resource::IResourceManager > m_resourceManager;
	Ref< const StoryData > m_data;
	Ref< Inventory > m_inventory;
	Ref< QuestLog > m_questLog;
	mutable SmallMap< Guid, Ref< resource::ResourceHandle > > m_bound;
	Ref< IRuntimeDelegate > m_eventHandler;
	Ref< IRuntimeDelegate > m_conditionHandler;
	Ref< IRuntimeDelegate > m_distanceHandler;
	Conversation* m_conversation = nullptr;

	Story() = default;

	const Object* bind(const TypeInfo& productType, const Guid& id) const;
};

}

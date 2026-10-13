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
#include "Core/Containers/SmallSet.h"
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

class PropertyGroup;

}

namespace zombie
{

using namespace traktor;

class QuestEntry;
class QuestData;
class QuestStep;
class Story;

/*! The player's quests, and the flags, found items and deaths they depend on.
 * \ingroup Zombie
 */
class T_DLLCLASS QuestLog : public Object
{
	T_RTTI_CLASS;

public:
	explicit QuestLog(Story* story);

	/*! Give quest; false if unknown or given before. */
	bool start(const Guid& quest);

	/*! Complete quest's current step; only if it is step, unless null. */
	bool advance(const Guid& quest, const Guid& step);

	/*! Finish quest outright. */
	bool finish(const Guid& quest);

	bool isStarted(const Guid& quest) const;

	/*! Given, and neither finished nor failed. */
	bool isActive(const Guid& quest) const;

	bool isDone(const Guid& quest) const;

	/*! Index of quest's current step, -1 if not under way. */
	int32_t getStepIndex(const Guid& quest) const;

	/*! Quest's current step, null if not under way. */
	const QuestStep* getCurrentStep(const Guid& quest) const;

	/*! Guid of quest's current step, null if not under way. */
	Guid getCurrentStepId(const Guid& quest) const;

	/*! Quest being followed, null guid if none. */
	const Guid& getActive() const { return m_active; }

	/*! Follow quest under way, or none with a null guid. */
	bool setActive(const Guid& quest);

	/*! Current step of the quest being followed, null if none. */
	const QuestStep* getActiveStep() const;

	/*! Every quest given; under way first, then finished, newest first. */
	RefArray< QuestEntry > getEntries() const;

	/*! Conversation with actor is at line, by its guid; completes talk steps waiting for it. */
	void talkedTo(const Guid& actor, const Guid& line);

	/*! Item has entered the inventory; remembered for good. */
	void found(const Guid& item);

	bool hasFound(const Guid& item) const;

	/*! Actor has died, as the game tells; remembered for good, and quests failing on it fail. */
	void died(const Guid& actor);

	bool hasDied(const Guid& actor) const;

	/*! Flag, by the guid of its FlagData. */
	bool getFlag(const Guid& flag) const;

	void setFlag(const Guid& flag, bool value);

	/*! Once-response, by its guid, has been chosen talking to actor. */
	bool isSaid(const Guid& actor, const Guid& response) const;

	void setSaid(const Guid& actor, const Guid& response);

	/*! Complete steps already met, such as items found or places reached. */
	void update();

	/*! Incremented on every change. */
	uint32_t getSerial() const { return m_serial; }

	void saveState(PropertyGroup* state) const;

	void restoreState(const PropertyGroup* state);

private:
	/*! A once-response given to an actor. Not a std::pair, whose comparison would compare the guids' addresses. */
	struct Said
	{
		Guid actor;
		Guid response;

		bool operator < (const Said& rh) const { return actor < rh.actor || (actor == rh.actor && response < rh.response); }

		bool operator > (const Said& rh) const { return rh < *this; }

		bool operator == (const Said& rh) const { return actor == rh.actor && response == rh.response; }
	};

	struct Record
	{
		Guid quest;
		int32_t step;
		bool done;
		int32_t order;
		bool failed = false;
	};

	Story* m_story;
	AlignedVector< Record > m_records;
	Guid m_active;
	SmallSet< Guid > m_flags;
	SmallSet< Said > m_said;
	SmallSet< Guid > m_found;
	SmallSet< Guid > m_dead;
	int32_t m_order = 0;
	uint32_t m_serial = 0;

	const Record* findRecord(const Guid& quest) const;

	Record* findRecord(const Guid& quest);

	AlignedVector< Guid > getUnderWay() const;

	bool isMet(const QuestStep* step) const;

	void settle(const Guid& quest);

	void completeStep(const Guid& quest);

	void fail(const Guid& quest);

	void followNewest(const Guid& ended);

	void changed();
};

}

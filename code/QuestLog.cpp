/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "QuestLog.h"

#include "Core/Class/Any.h"
#include "Core/Class/Boxes/BoxedGuid.h"
#include "Core/Log/Log.h"
#include "Core/Math/MathUtils.h"
#include "Core/Misc/String.h"
#include "Core/Settings/PropertyBoolean.h"
#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyInteger.h"
#include "Core/Settings/PropertyString.h"
#include "QuestData.h"
#include "QuestEntry.h"
#include "Steps/FindItemStep.h"
#include "Steps/ReachStep.h"
#include "Steps/TalkStep.h"
#include "Story.h"

#include <algorithm>

namespace zombie
{
	namespace
	{

// Completing a step may complete the next at once, such as an item already found;
// bounded so a malformed quest cannot loop forever.
const int32_t c_maxSettle = 32;

Guid stepIdOf(const QuestData* questData, int32_t step)
{
	if (!questData || step < 0 || step >= (int32_t)questData->getSteps().size() || !questData->getSteps()[step])
		return Guid();
	return questData->getSteps()[step]->getId();
}

std::wstring indexed(const wchar_t* prefix, int32_t index, const wchar_t* member)
{
	return prefix + toString(index) + (member != nullptr ? (std::wstring(L".") + member) : std::wstring());
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.QuestLog", QuestLog, Object)

QuestLog::QuestLog(Story* story)
:	m_story(story)
{
}

bool QuestLog::start(const Guid& quest)
{
	const QuestData* questData = m_story->getQuest(quest);
	if (!questData)
	{
		log::warning << L"Story: no quest " << quest.format() << L"." << Endl;
		return false;
	}
	if (findRecord(quest) != nullptr)
		return false;

	m_records.push_back({ quest, 0, false, ++m_order });
	if (m_active.isNull())
		m_active = quest;

	changed();
	m_story->notify(L"questStarted", CastAny< Guid >::set(quest), Any::fromObject(const_cast< QuestStep* >(getCurrentStep(quest))));
	settle(quest);
	return true;
}

bool QuestLog::advance(const Guid& quest, const Guid& step)
{
	if (!isActive(quest))
		return false;
	if (step.isNotNull() && getCurrentStepId(quest) != step)
		return false;

	completeStep(quest);
	return true;
}

bool QuestLog::finish(const Guid& quest)
{
	Record* record = findRecord(quest);
	if (record == nullptr || record->done || record->failed)
		return false;

	const QuestData* questData = m_story->getQuest(quest);
	if (!questData)
		return false;

	record->step = std::max((int32_t)questData->getSteps().size() - 1, 0);
	completeStep(quest);
	return true;
}

bool QuestLog::isStarted(const Guid& quest) const
{
	return findRecord(quest) != nullptr;
}

bool QuestLog::isActive(const Guid& quest) const
{
	const Record* record = findRecord(quest);
	return record != nullptr && !record->done && !record->failed;
}

bool QuestLog::isDone(const Guid& quest) const
{
	const Record* record = findRecord(quest);
	return record != nullptr && record->done;
}

int32_t QuestLog::getStepIndex(const Guid& quest) const
{
	const Record* record = findRecord(quest);
	return (record != nullptr && !record->done && !record->failed) ? record->step : -1;
}

const QuestStep* QuestLog::getCurrentStep(const Guid& quest) const
{
	const int32_t step = getStepIndex(quest);
	if (step < 0)
		return nullptr;

	const QuestData* questData = m_story->getQuest(quest);
	if (!questData || step >= (int32_t)questData->getSteps().size())
		return nullptr;

	return questData->getSteps()[step];
}

Guid QuestLog::getCurrentStepId(const Guid& quest) const
{
	const QuestStep* step = getCurrentStep(quest);
	return step ? step->getId() : Guid();
}

bool QuestLog::setActive(const Guid& quest)
{
	if (quest.isNotNull() && !isActive(quest))
		return false;

	if (quest != m_active)
	{
		m_active = quest.isNotNull() ? quest : Guid();
		changed();
	}
	return true;
}

const QuestStep* QuestLog::getActiveStep() const
{
	return m_active.isNotNull() ? getCurrentStep(m_active) : nullptr;
}

RefArray< QuestEntry > QuestLog::getEntries() const
{
	AlignedVector< Record > records = m_records;
	std::sort(records.begin(), records.end(), [](const Record& a, const Record& b) {
		const bool aEnded = a.done || a.failed;
		const bool bEnded = b.done || b.failed;
		if (aEnded != bEnded)
			return !aEnded;
		return a.order > b.order;
	});

	RefArray< QuestEntry > entries;
	for (const auto& record : records)
	{
		const QuestData* questData = m_story->getQuest(record.quest);
		if (questData)
			entries.push_back(new QuestEntry(record.quest, questData, record.step, record.done, record.failed, record.quest == m_active));
	}
	return entries;
}

void QuestLog::talkedTo(const Guid& actor, const Guid& line)
{
	for (const auto& quest : getUnderWay())
	{
		const TalkStep* talkStep = dynamic_type_cast< const TalkStep* >(getCurrentStep(quest));
		if (talkStep == nullptr || actor != talkStep->getActor())
			continue;
		if (talkStep->getLine().isNotNull() && talkStep->getLine() != line)
			continue;
		completeStep(quest);
	}
}

void QuestLog::found(const Guid& item)
{
	if (item.isNotNull() && m_found.insert(item))
		changed();
}

bool QuestLog::hasFound(const Guid& item) const
{
	return m_found.find(item) != m_found.end();
}

void QuestLog::died(const Guid& actor)
{
	if (actor.isNull() || !m_dead.insert(actor))
		return;

	changed();

	// Quests waiting on the dead may fail now.
	for (const auto& quest : getUnderWay())
		settle(quest);
}

bool QuestLog::hasDied(const Guid& actor) const
{
	return m_dead.find(actor) != m_dead.end();
}

bool QuestLog::getFlag(const Guid& flag) const
{
	return m_flags.find(flag) != m_flags.end();
}

void QuestLog::setFlag(const Guid& flag, bool value)
{
	if (flag.isNull())
		return;
	if (value ? m_flags.insert(flag) : m_flags.erase(flag))
		changed();
}

bool QuestLog::isSaid(const Guid& actor, const Guid& response) const
{
	return m_said.find({ actor, response }) != m_said.end();
}

void QuestLog::setSaid(const Guid& actor, const Guid& response)
{
	if (response.isNotNull() && m_said.insert({ actor, response }))
		changed();
}

void QuestLog::update()
{
	for (const auto& quest : getUnderWay())
		settle(quest);
}

void QuestLog::saveState(PropertyGroup* state) const
{
	state->setProperty< PropertyInteger >(L"quests", (int32_t)m_records.size());
	for (int32_t i = 0; i < (int32_t)m_records.size(); ++i)
	{
		const Record& record = m_records[i];
		state->setProperty< PropertyString >(indexed(L"quest", i, L"id"), record.quest.format());
		state->setProperty< PropertyInteger >(indexed(L"quest", i, L"step"), record.step);
		state->setProperty< PropertyString >(indexed(L"quest", i, L"stepId"), stepIdOf(m_story->getQuest(record.quest), record.step).format());
		state->setProperty< PropertyBoolean >(indexed(L"quest", i, L"done"), record.done);
		state->setProperty< PropertyInteger >(indexed(L"quest", i, L"order"), record.order);
		state->setProperty< PropertyBoolean >(indexed(L"quest", i, L"failed"), record.failed);
	}
	state->setProperty< PropertyString >(L"active", m_active.isNotNull() ? m_active.format() : L"");
	state->setProperty< PropertyInteger >(L"order", m_order);

	int32_t i = 0;
	state->setProperty< PropertyInteger >(L"flags", (int32_t)m_flags.size());
	for (const auto& flag : m_flags)
		state->setProperty< PropertyString >(indexed(L"flag", i++, nullptr), flag.format());

	i = 0;
	state->setProperty< PropertyInteger >(L"said", (int32_t)m_said.size());
	for (const auto& said : m_said)
	{
		state->setProperty< PropertyString >(indexed(L"said", i, L"actor"), said.actor.format());
		state->setProperty< PropertyString >(indexed(L"said", i++, L"response"), said.response.format());
	}

	i = 0;
	state->setProperty< PropertyInteger >(L"dead", (int32_t)m_dead.size());
	for (const auto& actor : m_dead)
		state->setProperty< PropertyString >(indexed(L"dead", i++, nullptr), actor.format());

	i = 0;
	state->setProperty< PropertyInteger >(L"found", (int32_t)m_found.size());
	for (const auto& item : m_found)
		state->setProperty< PropertyString >(indexed(L"found", i++, nullptr), item.format());
}

void QuestLog::restoreState(const PropertyGroup* state)
{
	m_records.clear();
	m_flags.clear();
	m_said.clear();
	m_found.clear();
	m_dead.clear();
	m_active = Guid();
	m_order = 0;

	if (state != nullptr)
	{
		const int32_t quests = state->getProperty< int32_t >(L"quests", 0);
		for (int32_t i = 0; i < quests; ++i)
		{
			const Guid quest(state->getProperty< std::wstring >(indexed(L"quest", i, L"id"), L""));

			// A quest since removed from the story is dropped, and a step past its end clamped, so an old save still loads.
			const QuestData* questData = m_story->getQuest(quest);
			if (!questData || questData->getSteps().empty())
				continue;

			Record record;
			record.quest = quest;
			// The step by its guid, so steps moved about since still load; by its index otherwise.
			const int32_t stepById = questData->findStep(Guid(state->getProperty< std::wstring >(indexed(L"quest", i, L"stepId"), L"")));
			record.step = (stepById >= 0) ? stepById : clamp(state->getProperty< int32_t >(indexed(L"quest", i, L"step"), 0), 0, (int32_t)questData->getSteps().size() - 1);
			record.done = state->getProperty< bool >(indexed(L"quest", i, L"done"), false);
			record.order = state->getProperty< int32_t >(indexed(L"quest", i, L"order"), 0);
			record.failed = state->getProperty< bool >(indexed(L"quest", i, L"failed"), false);
			m_records.push_back(record);
		}

		const Guid active(state->getProperty< std::wstring >(L"active", L""));
		if (active.isValid() && isActive(active))
			m_active = active;

		m_order = state->getProperty< int32_t >(L"order", 0);

		const int32_t flags = state->getProperty< int32_t >(L"flags", 0);
		for (int32_t i = 0; i < flags; ++i)
		{
			const Guid flag(state->getProperty< std::wstring >(indexed(L"flag", i, nullptr), L""));
			if (flag.isValid() && flag.isNotNull())
				m_flags.insert(flag);
		}

		const int32_t said = state->getProperty< int32_t >(L"said", 0);
		for (int32_t i = 0; i < said; ++i)
		{
			const Guid actor(state->getProperty< std::wstring >(indexed(L"said", i, L"actor"), L""));
			const Guid response(state->getProperty< std::wstring >(indexed(L"said", i, L"response"), L""));
			if (response.isValid() && response.isNotNull())
				m_said.insert({ actor.isValid() ? actor : Guid(), response });
		}

		const int32_t dead = state->getProperty< int32_t >(L"dead", 0);
		for (int32_t i = 0; i < dead; ++i)
		{
			const Guid actor(state->getProperty< std::wstring >(indexed(L"dead", i, nullptr), L""));
			if (actor.isValid() && actor.isNotNull())
				m_dead.insert(actor);
		}

		const int32_t found = state->getProperty< int32_t >(L"found", 0);
		for (int32_t i = 0; i < found; ++i)
		{
			const Guid item(state->getProperty< std::wstring >(indexed(L"found", i, nullptr), L""));
			if (item.isValid() && item.isNotNull())
				m_found.insert(item);
		}
	}

	changed();
}

const QuestLog::Record* QuestLog::findRecord(const Guid& quest) const
{
	for (const auto& record : m_records)
	{
		if (record.quest == quest)
			return &record;
	}
	return nullptr;
}

QuestLog::Record* QuestLog::findRecord(const Guid& quest)
{
	for (auto& record : m_records)
	{
		if (record.quest == quest)
			return &record;
	}
	return nullptr;
}

AlignedVector< Guid > QuestLog::getUnderWay() const
{
	// A list of its own; completing a step may give another quest.
	AlignedVector< Guid > quests;
	for (const auto& record : m_records)
	{
		if (!record.done && !record.failed)
			quests.push_back(record.quest);
	}
	return quests;
}

bool QuestLog::isMet(const QuestStep* step) const
{
	if (auto findItemStep = dynamic_type_cast< const FindItemStep* >(step))
		return hasFound(findItemStep->getItem());
	else if (auto reachStep = dynamic_type_cast< const ReachStep* >(step))
	{
		float distance;
		return m_story->getDistance(reachStep->getPlace(), distance) && distance <= reachStep->getRadius();
	}
	else
		return false;
}

bool QuestLog::needsDead(const Guid& quest) const
{
	const QuestData* questData = m_story->getQuest(quest);
	const int32_t step = getStepIndex(quest);
	if (!questData || step < 0)
		return false;

	for (const auto& actor : questData->getNeededActors(step))
	{
		if (hasDied(actor))
			return true;
	}
	return false;
}

void QuestLog::settle(const Guid& quest)
{
	for (int32_t i = 0; i < c_maxSettle; ++i)
	{
		const QuestStep* step = getCurrentStep(quest);
		if (step == nullptr)
			return;

		// Failing comes first; a step met and failed at once is failed.
		if (needsDead(quest) || (step->getFailWhen() != nullptr && step->getFailWhen()->evaluate(m_story)))
		{
			fail(quest);
			return;
		}

		if (!isMet(step))
			return;
		completeStep(quest);
	}
}

void QuestLog::completeStep(const Guid& quest)
{
	const QuestData* questData = m_story->getQuest(quest);
	const Record* record = findRecord(quest);
	if (!questData || record == nullptr || record->done || record->failed)
		return;

	const int32_t step = record->step;
	if (step < (int32_t)questData->getSteps().size())
		m_story->runActions(questData->getSteps()[step]->getOnComplete());

	// Actions may have changed the records, even finished this quest.
	Record* current = findRecord(quest);
	if (current == nullptr || current->done || current->failed || current->step != step)
		return;

	if (step >= (int32_t)questData->getSteps().size() - 1)
	{
		current->done = true;
		changed();

		m_story->runActions(questData->getReward());
		m_story->notify(L"questCompleted", CastAny< Guid >::set(quest), Any());

		followNewest(quest);
		return;
	}

	current->step = step + 1;
	changed();

	m_story->notify(L"questUpdated", CastAny< Guid >::set(quest), Any::fromObject(const_cast< QuestStep* >(getCurrentStep(quest))));
	settle(quest);
}

void QuestLog::fail(const Guid& quest)
{
	Record* record = findRecord(quest);
	if (record == nullptr || record->done || record->failed)
		return;

	record->failed = true;
	changed();

	const QuestData* questData = m_story->getQuest(quest);
	const QuestStep* step = (questData && record->step < (int32_t)questData->getSteps().size()) ? questData->getSteps()[record->step] : nullptr;
	m_story->notify(L"questFailed", CastAny< Guid >::set(quest), Any::fromObject(const_cast< QuestStep* >(step)));

	followNewest(quest);
}

void QuestLog::followNewest(const Guid& ended)
{
	// The quest followed has ended; follow the newest quest still under way instead.
	if (m_active != ended)
		return;

	m_active = Guid();
	int32_t newest = -1;
	for (const auto& record : m_records)
	{
		if (!record.done && !record.failed && record.order > newest)
		{
			m_active = record.quest;
			newest = record.order;
		}
	}
}

void QuestLog::changed()
{
	++m_serial;
}

}

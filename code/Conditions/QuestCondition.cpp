/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Conditions/QuestCondition.h"

#include "Core/Serialization/AttributeType.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberEnum.h"
#include "QuestData.h"
#include "QuestLog.h"
#include "QuestStep.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.QuestCondition", 1, QuestCondition, ICondition)

QuestCondition::QuestCondition(const resource::Id< QuestData >& quest, State state, const Guid& step)
:	m_quest(quest)
,	m_state(state)
,	m_step(step)
{
}

bool QuestCondition::evaluate(const Story* story) const
{
	const QuestLog* questLog = story->getQuestLog();
	switch (m_state)
	{
	case State::NotStarted:
		return !questLog->isStarted(m_quest);
	case State::Started:
		return questLog->isStarted(m_quest);
	case State::Active:
		return questLog->isActive(m_quest);
	case State::Done:
		return questLog->isDone(m_quest);
	case State::NotDone:
		return !questLog->isDone(m_quest);
	case State::AtStep:
		return questLog->isActive(m_quest) && questLog->getCurrentStepId(m_quest) == m_step;
	}
	return false;
}

void QuestCondition::serialize(ISerializer& s)
{
	const MemberEnum< State >::Key c_State_Keys[] =
	{
		{ L"NotStarted", State::NotStarted },
		{ L"Started", State::Started },
		{ L"Active", State::Active },
		{ L"Done", State::Done },
		{ L"NotDone", State::NotDone },
		{ L"AtStep", State::AtStep },
		{ 0 }
	};

	s >> resource::Member< QuestData >(L"quest", m_quest);
	s >> MemberEnum< State >(L"state", m_state, c_State_Keys);
	s >> Member< Guid >(L"step", m_step, AttributeType(type_of< QuestStep >()));
}

}

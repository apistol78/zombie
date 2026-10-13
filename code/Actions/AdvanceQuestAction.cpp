/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Actions/AdvanceQuestAction.h"

#include "Core/Serialization/AttributeType.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "QuestData.h"
#include "QuestLog.h"
#include "QuestStep.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.AdvanceQuestAction", 1, AdvanceQuestAction, IAction)

AdvanceQuestAction::AdvanceQuestAction(const resource::Id< QuestData >& quest, const Guid& step)
:	m_quest(quest)
,	m_step(step)
{
}

void AdvanceQuestAction::execute(Story* story) const
{
	story->getQuestLog()->advance(m_quest, m_step);
}

void AdvanceQuestAction::serialize(ISerializer& s)
{
	s >> resource::Member< QuestData >(L"quest", m_quest);
	s >> Member< Guid >(L"step", m_step, AttributeType(type_of< QuestStep >()));
}

}

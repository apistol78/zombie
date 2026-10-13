/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Actions/StartQuestAction.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "QuestData.h"
#include "QuestLog.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.StartQuestAction", 0, StartQuestAction, IAction)

StartQuestAction::StartQuestAction(const resource::Id< QuestData >& quest)
:	m_quest(quest)
{
}

void StartQuestAction::execute(Story* story) const
{
	story->getQuestLog()->start(m_quest);
}

void StartQuestAction::serialize(ISerializer& s)
{
	s >> resource::Member< QuestData >(L"quest", m_quest);
}

}

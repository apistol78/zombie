/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Conditions/FlagCondition.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Keys/FlagData.h"
#include "QuestLog.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.FlagCondition", 1, FlagCondition, ICondition)

FlagCondition::FlagCondition(const resource::Id< FlagData >& flag, bool negate)
:	m_flag(flag)
,	m_negate(negate)
{
}

bool FlagCondition::evaluate(const Story* story) const
{
	return story->getQuestLog()->getFlag(m_flag) != m_negate;
}

void FlagCondition::serialize(ISerializer& s)
{
	s >> resource::Member< FlagData >(L"flag", m_flag);
	s >> Member< bool >(L"negate", m_negate);
}

}

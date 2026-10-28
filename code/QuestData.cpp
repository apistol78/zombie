/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "QuestData.h"

#include "Core/Misc/String.h"
#include "Core/Serialization/AttributeMultiLine.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Steps/TalkStep.h"

#include <algorithm>

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.QuestData", 0, QuestData, ISerializable)

int32_t QuestData::findStep(const Guid& id) const
{
	if (id.isNull())
		return -1;

	for (int32_t i = 0; i < (int32_t)m_steps.size(); ++i)
	{
		if (m_steps[i] && m_steps[i]->getId() == id)
			return i;
	}
	return -1;
}

AlignedVector< Guid > QuestData::getNeededActors(int32_t step) const
{
	AlignedVector< Guid > actors;
	for (int32_t i = std::max(step, 0); i < (int32_t)m_steps.size(); ++i)
	{
		if (auto talkStep = dynamic_type_cast< const TalkStep* >(m_steps[i]))
			actors.push_back(talkStep->getActor());
	}
	return actors;
}

void QuestData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"title", m_title);
	s >> Member< std::wstring >(L"info", m_info, AttributeMultiLine());
	s >> MemberRefArray< QuestStep >(L"steps", m_steps);
	s >> MemberRefArray< IAction >(L"reward", m_reward);
}

}

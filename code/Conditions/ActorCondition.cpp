/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Conditions/ActorCondition.h"

#include "ActorData.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberEnum.h"
#include "QuestLog.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ActorCondition", 0, ActorCondition, ICondition)

ActorCondition::ActorCondition(const resource::Id< ActorData >& actor, State state)
:	m_actor(actor)
,	m_state(state)
{
}

bool ActorCondition::evaluate(const Story* story) const
{
	const bool dead = story->getQuestLog()->hasDied(m_actor);
	return dead == (m_state == State::Dead);
}

void ActorCondition::serialize(ISerializer& s)
{
	const MemberEnum< State >::Key c_State_Keys[] =
	{
		{ L"Alive", State::Alive },
		{ L"Dead", State::Dead },
		{ 0 }
	};

	s >> resource::Member< ActorData >(L"actor", m_actor);
	s >> MemberEnum< State >(L"state", m_state, c_State_Keys);
}

}

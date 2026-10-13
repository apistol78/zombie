/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Conditions/AllCondition.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/MemberRefArray.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.AllCondition", 0, AllCondition, ICondition)

bool AllCondition::evaluate(const Story* story) const
{
	for (auto condition : m_conditions)
	{
		if (condition != nullptr && !condition->evaluate(story))
			return false;
	}
	return true;
}

void AllCondition::serialize(ISerializer& s)
{
	s >> MemberRefArray< ICondition >(L"conditions", m_conditions);
}

}

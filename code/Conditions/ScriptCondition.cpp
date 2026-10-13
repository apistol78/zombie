/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Conditions/ScriptCondition.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Keys/ScriptConditionData.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ScriptCondition", 1, ScriptCondition, ICondition)

bool ScriptCondition::evaluate(const Story* story) const
{
	return story->evaluateScriptCondition(m_condition);
}

void ScriptCondition::serialize(ISerializer& s)
{
	s >> resource::Member< ScriptConditionData >(L"condition", m_condition);
}

}

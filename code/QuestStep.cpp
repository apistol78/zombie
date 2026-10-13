/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "QuestStep.h"

#include "Core/Serialization/AttributeMultiLine.h"
#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRef.h"
#include "Core/Serialization/MemberRefArray.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.QuestStep", QuestStep, ISerializable)

QuestStep::QuestStep()
:	m_id(Guid::create())
{
}

void QuestStep::serialize(ISerializer& s)
{
	s >> Member< Guid >(L"id", m_id, AttributePrivate());
	s >> Member< std::wstring >(L"name", m_name);
	s >> Member< std::wstring >(L"text", m_text, AttributeMultiLine());
	s >> MemberRef< IWaypoint >(L"waypoint", m_waypoint);

	// Steps of version 2 can fail their quest; the version is the step type's own.
	if (s.getVersion(type_of(this)) >= 2)
		s >> MemberRef< ICondition >(L"failWhen", m_failWhen);
	s >> MemberRefArray< IAction >(L"onComplete", m_onComplete);
}

}

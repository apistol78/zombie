/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Actions/EventAction.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Keys/EventData.h"
#include "Resource/Member.h"
#include "Story.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.EventAction", 1, EventAction, IAction)

EventAction::EventAction(const resource::Id< EventData >& event, bool endConversation)
:	m_event(event)
,	m_endConversation(endConversation)
{
}

void EventAction::execute(Story* story) const
{
	story->raiseEvent(m_event, m_endConversation);
}

void EventAction::serialize(ISerializer& s)
{
	s >> resource::Member< EventData >(L"event", m_event);
	s >> Member< bool >(L"endConversation", m_endConversation);
}

}

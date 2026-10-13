/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Steps/TalkStep.h"

#include "ActorData.h"
#include "Core/Serialization/AttributeType.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "DialogNode.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.TalkStep", 2, TalkStep, QuestStep)

void TalkStep::serialize(ISerializer& s)
{
	QuestStep::serialize(s);
	s >> resource::Member< ActorData >(L"actor", m_actor);
	s >> Member< Guid >(L"line", m_line, AttributeType(type_of< DialogNode >()));
}

}

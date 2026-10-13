/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "DialogNode.h"

#include "Core/Serialization/AttributeMultiLine.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRefArray.h"
#include "DialogResponse.h"

#include <string>

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogNode", 1, DialogNode, ISerializable)

void DialogNode::serialize(ISerializer& s)
{
	s >> Member< Guid >(L"id", m_id);
	s >> Member< std::wstring >(L"name", m_name);
	s >> Member< std::wstring >(L"text", m_text, AttributeMultiLine());
	s >> MemberRefArray< IAction >(L"onEnter", m_onEnter);
	s >> MemberRefArray< DialogResponse >(L"responses", m_responses);
}

}

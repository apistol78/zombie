/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "DialogResponse.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRef.h"
#include "Core/Serialization/MemberRefArray.h"

#include <string>

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogResponse", 1, DialogResponse, ISerializable)

void DialogResponse::serialize(ISerializer& s)
{
	s >> Member< Guid >(L"id", m_id);
	s >> Member< std::wstring >(L"text", m_text);
	s >> Member< int32_t >(L"target", m_target);
	s >> MemberRef< ICondition >(L"condition", m_condition);
	s >> MemberRefArray< IAction >(L"actions", m_actions);
	s >> Member< bool >(L"once", m_once);
}

}

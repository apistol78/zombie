/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ActorData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRef.h"
#include "Core/Settings/PropertyGroup.h"
#include "DialogData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ActorData", 1, ActorData, ISerializable)

void ActorData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
	s >> resource::Member< DialogData >(L"dialog", m_dialog);

	if (s.getVersion< ActorData >() >= 1)
		s >> MemberRef< const PropertyGroup >(L"properties", m_properties);
}

}

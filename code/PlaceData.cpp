/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PlaceData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.PlaceData", 0, PlaceData, ISerializable)

void PlaceData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
}

}

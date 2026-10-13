/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/DialogNodeAsset.h"

#include "Core/Guid.h"
#include "Core/Math/Vector2.h"
#include "Core/Serialization/AttributeMultiLine.h"
#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializable.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRefArray.h"
#include "IAction.h"

#include <string>

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogNodeAsset", 1, DialogNodeAsset, ISerializable)

DialogNodeAsset::DialogNodeAsset()
:	id(Guid::create())
{
}

void DialogNodeAsset::serialize(ISerializer& s)
{
	if (s.getVersion< DialogNodeAsset >() >= 1)
	{
		s >> Member< Guid >(L"id", id, AttributePrivate());
		s >> Member< std::wstring >(L"name", name);
	}
	else
	{
		// Lines were identified by name only; given a guid as they are read.
		s >> Member< std::wstring >(L"id", name);
		id = Guid::create();
	}

	s >> Member< std::wstring >(L"text", text, AttributeMultiLine());
	s >> MemberRefArray< IAction >(L"onEnter", onEnter);
	s >> MemberRefArray< DialogResponseAsset >(L"responses", responses);
	s >> Member< Vector2 >(L"position", position, AttributePrivate());
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/DialogResponseAsset.h"

#include "Core/Guid.h"
#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializable.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRef.h"
#include "Core/Serialization/MemberRefArray.h"
#include "IAction.h"
#include "ICondition.h"

#include <string>

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogResponseAsset", 2, DialogResponseAsset, ISerializable)

DialogResponseAsset::DialogResponseAsset()
:	id(Guid::create())
{
}

void DialogResponseAsset::serialize(ISerializer& s)
{
	if (s.getVersion< DialogResponseAsset >() >= 2)
		s >> Member< Guid >(L"id", id, AttributePrivate());

	s >> Member< std::wstring >(L"text", text);

	if (s.getVersion< DialogResponseAsset >() >= 1)
		s >> Member< Guid >(L"target", target, AttributePrivate());
	else
		s >> Member< std::wstring >(L"target", legacyTarget);

	s >> MemberRef< ICondition >(L"condition", condition);
	s >> MemberRefArray< IAction >(L"actions", actions);
	s >> Member< bool >(L"once", once);
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/DialogStartAsset.h"

#include "Core/Guid.h"
#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializable.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberRef.h"
#include "ICondition.h"

#include <string>

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogStartAsset", 1, DialogStartAsset, ISerializable)

void DialogStartAsset::serialize(ISerializer& s)
{
	if (s.getVersion< DialogStartAsset >() >= 1)
		s >> Member< Guid >(L"node", node, AttributePrivate());
	else
		s >> Member< std::wstring >(L"node", legacyNode);

	s >> MemberRef< ICondition >(L"condition", condition);
}

}

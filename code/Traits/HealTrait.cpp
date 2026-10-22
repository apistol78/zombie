/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Traits/HealTrait.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.HealTrait", 1, HealTrait, IItemTrait)

HealTrait::HealTrait(float amount)
:	m_amount(amount)
{
}

void HealTrait::serialize(ISerializer& s)
{
	s >> Member< float >(L"amount", m_amount, AttributeRange(0.0f));
	if (s.getVersion< HealTrait >() >= 1)
		s >> Member< std::wstring >(L"verb", m_verb);
}

}

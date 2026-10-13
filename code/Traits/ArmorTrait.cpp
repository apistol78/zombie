/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Traits/ArmorTrait.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ArmorTrait", 0, ArmorTrait, IItemTrait)

ArmorTrait::ArmorTrait(float fraction)
:	m_fraction(fraction)
{
}

void ArmorTrait::serialize(ISerializer& s)
{
	s >> Member< float >(L"fraction", m_fraction, AttributeRange(0.0f, 1.0f));
}

}

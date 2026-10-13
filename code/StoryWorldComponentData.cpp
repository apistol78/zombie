/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "StoryWorldComponentData.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/AttributeUnit.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "StoryWorldComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.StoryWorldComponentData", 0, StoryWorldComponentData, world::IWorldComponentData)

Ref< StoryWorldComponent > StoryWorldComponentData::createComponent() const
{
	return new StoryWorldComponent(m_refreshInterval);
}

void StoryWorldComponentData::serialize(ISerializer& s)
{
	s >> Member< float >(L"refreshInterval", m_refreshInterval, AttributeRange(0.0f) | AttributeUnit(UnitType::Seconds));
}

}

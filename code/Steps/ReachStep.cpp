/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Steps/ReachStep.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "PlaceData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ReachStep", 2, ReachStep, QuestStep)

void ReachStep::serialize(ISerializer& s)
{
	QuestStep::serialize(s);
	s >> resource::Member< PlaceData >(L"place", m_place);
	s >> Member< float >(L"radius", m_radius, AttributeRange(0.0f));
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/LandmarkComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Fixtures/LandmarkComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.LandmarkComponentData", 0, LandmarkComponentData, world::IEntityComponentData)

Ref< LandmarkComponent > LandmarkComponentData::createComponent() const
{
	return new LandmarkComponent(m_glyph);
}

int32_t LandmarkComponentData::getOrdinal() const
{
	return 0;
}

void LandmarkComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void LandmarkComponentData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"glyph", m_glyph);
}

}

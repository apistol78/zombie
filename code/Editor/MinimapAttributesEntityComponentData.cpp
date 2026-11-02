/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapAttributesEntityComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.MinimapAttributesEntityComponentData", 0, MinimapAttributesEntityComponentData, world::IEntityComponentData)

int32_t MinimapAttributesEntityComponentData::getOrdinal() const
{
	return 0;
}

void MinimapAttributesEntityComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void MinimapAttributesEntityComponentData::serialize(ISerializer& s)
{
	s >> Member< bool >(L"includeInMinimap", m_includeInMinimap);
}

}

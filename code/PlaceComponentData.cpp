/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "PlaceComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "PlaceComponent.h"
#include "PlaceData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.PlaceComponentData", 0, PlaceComponentData, world::IEntityComponentData)

Ref< PlaceComponent > PlaceComponentData::createComponent() const
{
	return new PlaceComponent(m_place);
}

int32_t PlaceComponentData::getOrdinal() const
{
	return 0;
}

void PlaceComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void PlaceComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< PlaceData >(L"place", m_place);
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/StageExitComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Fixtures/StageExitComponent.h"
#include "PlaceData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.StageExitComponentData", 0, StageExitComponentData, world::IEntityComponentData)

Ref< StageExitComponent > StageExitComponentData::createComponent() const
{
	return new StageExitComponent(m_target, m_arrival);
}

int32_t StageExitComponentData::getOrdinal() const
{
	return 0;
}

void StageExitComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void StageExitComponentData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"target", m_target);
	s >> resource::Member< PlaceData >(L"arrival", m_arrival);
}

}

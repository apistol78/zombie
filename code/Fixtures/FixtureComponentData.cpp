/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/FixtureComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Fixtures/FixtureComponent.h"
#include "Keys/FixtureKindData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.FixtureComponentData", 0, FixtureComponentData, world::IEntityComponentData)

Ref< FixtureComponent > FixtureComponentData::createComponent() const
{
	return new FixtureComponent(m_kind);
}

int32_t FixtureComponentData::getOrdinal() const
{
	return 0;
}

void FixtureComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void FixtureComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< FixtureKindData >(L"kind", m_kind);
}

}

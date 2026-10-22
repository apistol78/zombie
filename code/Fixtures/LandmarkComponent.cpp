/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/LandmarkComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.LandmarkComponent", LandmarkComponent, world::IEntityComponent)

LandmarkComponent::LandmarkComponent(const std::wstring& glyph)
:	m_glyph(glyph)
{
}

void LandmarkComponent::destroy()
{
}

void LandmarkComponent::setOwner(world::Entity* owner)
{
}

void LandmarkComponent::setTransform(const Transform& transform)
{
}

Aabb3 LandmarkComponent::getBoundingBox() const
{
	return Aabb3();
}

void LandmarkComponent::update(const world::UpdateParams& update)
{
}

}

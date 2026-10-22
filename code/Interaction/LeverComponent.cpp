/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/LeverComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.LeverComponent", LeverComponent, world::IEntityComponent)

LeverComponent::LeverComponent(const Guid& target)
:	m_target(target)
{
}

void LeverComponent::destroy()
{
}

void LeverComponent::setOwner(world::Entity* owner)
{
}

void LeverComponent::setTransform(const Transform& transform)
{
}

Aabb3 LeverComponent::getBoundingBox() const
{
	return Aabb3();
}

void LeverComponent::update(const world::UpdateParams& update)
{
}

}

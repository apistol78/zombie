/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/StageExitComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.StageExitComponent", StageExitComponent, world::IEntityComponent)

StageExitComponent::StageExitComponent(const std::wstring& target, const Guid& arrival)
:	m_target(target)
,	m_arrival(arrival)
{
}

void StageExitComponent::destroy()
{
}

void StageExitComponent::setOwner(world::Entity* owner)
{
}

void StageExitComponent::setTransform(const Transform& transform)
{
}

Aabb3 StageExitComponent::getBoundingBox() const
{
	return Aabb3();
}

void StageExitComponent::update(const world::UpdateParams& update)
{
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/LockComponent.h"

#include "Interaction/LockComponentData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.LockComponent", LockComponent, world::IEntityComponent)

LockComponent::LockComponent(const LockComponentData* data)
:	m_data(data)
{
}

void LockComponent::destroy()
{
}

void LockComponent::setOwner(world::Entity* owner)
{
}

void LockComponent::setTransform(const Transform& transform)
{
}

Aabb3 LockComponent::getBoundingBox() const
{
	return Aabb3();
}

void LockComponent::update(const world::UpdateParams& update)
{
}

const Guid& LockComponent::getItem() const
{
	return m_data->getItem();
}

const Guid& LockComponent::getSlot() const
{
	return m_data->getSlot();
}

const std::wstring& LockComponent::getUnequippedRefusal() const
{
	return m_data->getUnequippedRefusal();
}

}

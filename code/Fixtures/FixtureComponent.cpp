/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/FixtureComponent.h"

#include "Fixtures/FixtureWorldComponent.h"
#include "World/Entity.h"
#include "World/World.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.FixtureComponent", FixtureComponent, world::IEntityComponent)

FixtureComponent::FixtureComponent(const Guid& kind)
:	m_kind(kind)
{
}

void FixtureComponent::destroy()
{
	setWorld(nullptr);
	m_owner = nullptr;
}

void FixtureComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
}

void FixtureComponent::setWorld(world::World* world)
{
	if (m_fixtures)
		m_fixtures->remove(this);

	m_fixtures = world ? world->getComponent< FixtureWorldComponent >() : nullptr;

	if (m_fixtures)
		m_fixtures->add(this);
}

void FixtureComponent::setTransform(const Transform& transform)
{
}

Aabb3 FixtureComponent::getBoundingBox() const
{
	return Aabb3();
}

void FixtureComponent::update(const world::UpdateParams& update)
{
}

void FixtureComponent::setDone(bool done)
{
	if (m_fixtures && m_owner)
		m_fixtures->setDone(m_owner->getId(), done);
}

bool FixtureComponent::isDone() const
{
	return m_fixtures && m_owner && m_fixtures->isDone(m_owner->getId());
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Fixtures/FixtureWorldComponent.h"

#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyStringSet.h"
#include "Fixtures/FixtureComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.FixtureWorldComponent", FixtureWorldComponent, world::IWorldComponent)

void FixtureWorldComponent::destroy()
{
	m_fixtures.clear();
}

void FixtureWorldComponent::update(world::World* world, const world::UpdateParams& update)
{
}

void FixtureWorldComponent::add(FixtureComponent* fixture)
{
	m_fixtures.push_back(fixture);
}

void FixtureWorldComponent::remove(FixtureComponent* fixture)
{
	m_fixtures.remove(fixture);
}

RefArray< FixtureComponent > FixtureWorldComponent::getFixtures(const Guid& kind) const
{
	RefArray< FixtureComponent > fixtures;
	for (auto fixture : m_fixtures)
	{
		if (fixture->getKind() == kind)
			fixtures.push_back(fixture);
	}
	return fixtures;
}

void FixtureWorldComponent::setDone(const Guid& id, bool done)
{
	if (done)
		m_done.insert(id);
	else
		m_done.erase(id);
}

bool FixtureWorldComponent::isDone(const Guid& id) const
{
	return m_done.find(id) != m_done.end();
}

Ref< PropertyGroup > FixtureWorldComponent::saveState() const
{
	SmallSet< std::wstring > done;
	for (const auto& id : m_done)
		done.insert(id.format());

	Ref< PropertyGroup > state = new PropertyGroup();
	state->setProperty< PropertyStringSet >(L"done", done);
	return state;
}

void FixtureWorldComponent::restoreState(const PropertyGroup* state)
{
	m_done.clear();
	if (!state)
		return;

	for (const auto& id : state->getProperty< SmallSet< std::wstring > >(L"done", SmallSet< std::wstring >()))
		m_done.insert(Guid(id));
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/SmallSet.h"
#include "Core/Guid.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "World/IWorldComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class PropertyGroup;

}

namespace zombie
{

using namespace traktor;

class FixtureComponent;

/*! Fixture world component; every fixture in the world, and which are done with by entity id.
 * \ingroup Zombie
 *
 * What is done outlives the entities, so a fixture streamed out and back in is as it was.
 */
class T_DLLCLASS FixtureWorldComponent : public world::IWorldComponent
{
	T_RTTI_CLASS;

public:
	virtual void destroy() override final;

	virtual void update(world::World* world, const world::UpdateParams& update) override final;

	void add(FixtureComponent* fixture);

	void remove(FixtureComponent* fixture);

	/*! Every fixture in the world of kind. */
	RefArray< FixtureComponent > getFixtures(const Guid& kind) const;

	void setDone(const Guid& id, bool done);

	bool isDone(const Guid& id) const;

	/*! Which are done with. */
	Ref< PropertyGroup > saveState() const;

	void restoreState(const PropertyGroup* state);

private:
	RefArray< FixtureComponent > m_fixtures;
	SmallSet< Guid > m_done;
};

}

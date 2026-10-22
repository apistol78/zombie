/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Guid.h"
#include "World/IEntityComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace zombie
{

using namespace traktor;

class FixtureWorldComponent;

/*! Fixture component; a fixture whose state the world keeps by the owner entity's id.
 * \ingroup Zombie
 *
 * Joins the world's FixtureWorldComponent while the owner is in a world.
 */
class T_DLLCLASS FixtureComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit FixtureComponent(const Guid& kind);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setWorld(world::World* world) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	world::Entity* getOwner() const { return m_owner; }

	/*! Kind of fixture, null if none. */
	const Guid& getKind() const { return m_kind; }

	/*! Done with, such as a gate opened; kept across streaming, stages and saves. */
	void setDone(bool done);

	bool isDone() const;

private:
	world::Entity* m_owner = nullptr;
	FixtureWorldComponent* m_fixtures = nullptr;
	Guid m_kind;
};

}

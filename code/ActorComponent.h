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
#include "Resource/Proxy.h"
#include "World/IEntityComponent.h"

#include <string>

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

class ActorData;

/*! Actor component.
 * \ingroup Zombie
 *
 * Which actor the owner entity is.
 */
class T_DLLCLASS ActorComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit ActorComponent(const Guid& actor, const resource::Proxy< ActorData >& actorData);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	world::Entity* getOwner() const { return m_owner; }

	const Guid& getActor() const { return m_actor; }

	/*! The actor, null if it could not be bound. */
	const ActorData* getActorData() const;

	/*! The actor's name; the only place an actor's name is defined. Empty if it has none. */
	std::wstring getName() const;

	/*! The actor's properties for the game, null if none. */
	const PropertyGroup* getProperties() const;

private:
	world::Entity* m_owner = nullptr;
	Guid m_actor;
	resource::Proxy< ActorData > m_actorData;
};

}

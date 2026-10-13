/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Ref.h"
#include "Resource/Id.h"
#include "World/IEntityComponentData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::resource
{

class IResourceManager;

}

namespace zombie
{

using namespace traktor;

class ActorComponent;
class ActorData;

/*! Actor component data.
 * \ingroup Zombie
 *
 * Makes the owner entity an actor; quests and dialogs refer to the actor, and through it to the entity.
 */
class T_DLLCLASS ActorComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	/*! Create the component, with the actor bound through resourceManager; the actor is left unbound without one. */
	Ref< ActorComponent > createComponent(resource::IResourceManager* resourceManager) const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	const resource::Id< ActorData >& getActor() const { return m_actor; }

	void setActor(const resource::Id< ActorData >& actor) { m_actor = actor; }

private:
	resource::Id< ActorData > m_actor;
};

}

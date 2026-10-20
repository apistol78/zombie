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
#include "World/IEntityComponentData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::world
{

class PropertyEntityReference;

}

namespace zombie
{

using namespace traktor;

class NpcComponent;

/*! NPC component data.
 * \ingroup Zombie
 *
 * Makes the owner entity a character of the world; who it is comes from its actor.
 */
class T_DLLCLASS NpcComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	enum class Brain
	{
		Enemy,
		Human
	};

	Ref< NpcComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	Brain getBrain() const { return m_brain; }

	/*! Entity of the route to walk, null if none. */
	const world::PropertyEntityReference* getRoute() const { return m_route; }

private:
	Brain m_brain = Brain::Enemy;
	Ref< const world::PropertyEntityReference > m_route;
};

}

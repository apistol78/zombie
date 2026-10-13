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

/*! Place component.
 * \ingroup Zombie
 *
 * Which place the owner entity is.
 */
class T_DLLCLASS PlaceComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit PlaceComponent(const Guid& place);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	world::Entity* getOwner() const { return m_owner; }

	const Guid& getPlace() const { return m_place; }

private:
	world::Entity* m_owner = nullptr;
	Guid m_place;
};

}

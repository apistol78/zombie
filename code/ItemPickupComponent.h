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

/*! Item pickup component; the item, and how many of it, the owner entity is.
 * \ingroup Zombie
 */
class T_DLLCLASS ItemPickupComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit ItemPickupComponent(const Guid& item, int32_t count);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	world::Entity* getOwner() const { return m_owner; }

	const Guid& getItem() const { return m_item; }

	int32_t getCount() const { return m_count; }

	void setCount(int32_t count) { m_count = count; }

private:
	world::Entity* m_owner = nullptr;
	Guid m_item;
	int32_t m_count;
};

}

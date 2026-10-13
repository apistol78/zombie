/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Guid.h"
#include "Core/Ref.h"
#include "World/IEntityComponent.h"

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

class Inventory;

/*! Item container component; items and counts held by the owner entity, without slots or stack limits.
 * \ingroup Zombie
 */
class T_DLLCLASS ItemContainerComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	world::Entity* getOwner() const { return m_owner; }

	/*! Incremented on every change. */
	uint32_t getSerial() const { return m_serial; }

	int32_t getEntryCount() const { return (int32_t)m_entries.size(); }

	/*! Item of entry, null guid if out of range. */
	Guid getItem(int32_t index) const;

	/*! Count of entry, zero if out of range. */
	int32_t getCount(int32_t index) const;

	/*! Total count of item in container. */
	int32_t count(const Guid& item) const;

	/*! Add count of item. */
	void add(const Guid& item, int32_t count);

	/*! Remove up to count from entry; return number removed. An emptied entry is removed. */
	int32_t remove(int32_t index, int32_t count);

	/*! Move as much of entry as fits into inventory; return number moved. */
	int32_t moveTo(int32_t index, Inventory* inventory);

	/*! Remove every entry. */
	void clear();

	/*! Every entry as plain values. */
	Ref< PropertyGroup > saveState() const;

	/*! Replace every entry with a saved state. */
	void restoreState(const PropertyGroup* state);

private:
	struct Entry
	{
		Guid item;
		int32_t count;
	};

	world::Entity* m_owner = nullptr;
	AlignedVector< Entry > m_entries;
	uint32_t m_serial = 0;
};

}

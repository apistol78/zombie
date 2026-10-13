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

namespace zombie
{

using namespace traktor;

class ItemData;
class ItemPickupComponent;

/*! Item pickup component data; the item, and how many of it, the owner entity is.
 * \ingroup Zombie
 */
class T_DLLCLASS ItemPickupComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< ItemPickupComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	const resource::Id< ItemData >& getItem() const { return m_item; }

	void setItem(const resource::Id< ItemData >& item) { m_item = item; }

	int32_t getCount() const { return m_count; }

	void setCount(int32_t count) { m_count = count; }

private:
	resource::Id< ItemData > m_item;
	int32_t m_count = 1;
};

}

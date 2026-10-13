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
#include "Core/Object.h"
#include "Core/RefArray.h"
#include "Resource/Proxy.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::world
{

class Entity;

}

namespace zombie
{

using namespace traktor;

class ItemData;

/*! One stack of one item in an inventory.
 * \ingroup Zombie
 *
 * Read only; every change goes through the inventory.
 */
class T_DLLCLASS InventorySlot : public Object
{
	T_RTTI_CLASS;

public:
	const Guid& getItem() const { return m_item; }

	const ItemData* getItemData() const { return m_itemData.getResource(); }

	int32_t getCount() const { return m_count; }

	/*! Entities of a kept item, one per item, in the order they went in. */
	const RefArray< world::Entity >& getKept() const { return m_kept; }

private:
	friend class Inventory;

	Guid m_item;
	resource::Proxy< ItemData > m_itemData;
	int32_t m_count = 0;
	RefArray< world::Entity > m_kept;
};

}

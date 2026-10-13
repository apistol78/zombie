/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/SmallMap.h"
#include "Core/Guid.h"
#include "Core/Object.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "Resource/Proxy.h"

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

class IRuntimeDelegate;
class PropertyGroup;

}

namespace traktor::resource
{

class IResourceManager;

}

namespace traktor::world
{

class Entity;

}

namespace zombie
{

using namespace traktor;

class InventorySlot;
class ItemData;

/*! Inventory; fixed slots of item stacks, and the items equipped.
 * \ingroup Zombie
 */
class T_DLLCLASS Inventory : public Object
{
	T_RTTI_CLASS;

public:
	explicit Inventory(resource::IResourceManager* resourceManager, int32_t capacity);

	/*! Get item data, null if the item is unknown. */
	const ItemData* getItem(const Guid& item) const;

	int32_t getCapacity() const { return (int32_t)m_slots.size(); }

	/*! Stack in slot, null if the slot is empty. */
	const InventorySlot* getSlot(int32_t index) const;

	/*! Number of slots which hold something. */
	int32_t getUsed() const;

	/*! Incremented on every change. */
	uint32_t getSerial() const { return m_serial; }

	/*! Number of item carried, across all stacks. */
	int32_t count(const Guid& item) const;

	/*! Number of item which would fit. */
	int32_t room(const Guid& item) const;

	/*! Add up to count of item; return number added. A kept item's entity makes it exactly one. */
	int32_t add(const Guid& item, int32_t count, world::Entity* kept = nullptr);

	/*! Remove up to count of item, from the last stack first; return number removed. */
	int32_t remove(const Guid& item, int32_t count, RefArray< world::Entity >* outKept = nullptr);

	/*! Remove up to count of item; return the entities of the kept items removed. */
	RefArray< world::Entity > removeKept(const Guid& item, int32_t count);

	/*! Remove an entire stack; return the stack as it was. */
	Ref< InventorySlot > removeSlot(int32_t index);

	/*! First entity of a kept item. */
	world::Entity* first(const Guid& item) const;

	/*! Check if entity is carried as a kept item. */
	bool holds(const world::Entity* entity) const;

	/*! All entities of kept items. */
	RefArray< world::Entity > getKept() const;

	/*! Give item's stacks back their entities, front to back; not announced. */
	void attachKept(const Guid& item, const RefArray< world::Entity >& entities);

	/*! Release every entity of kept items, such as when leaving the world they belong to; not announced. */
	void detachKept();

	/*! Check if stack in slot count kept items it has no entities for. */
	bool keptMissing(int32_t index) const;

	/*! Item equipped in slot, by the guid of its EquipSlotData; null guid if none. */
	Guid getEquipped(const Guid& slot) const;

	bool isEquipped(const Guid& item) const;

	/*! Equip item where its EquipTrait says; refused if not carried or not equippable. */
	bool equip(const Guid& item);

	bool unequip(const Guid& slot);

	/*! Every stack and equipped mark as plain values; no entities. */
	Ref< PropertyGroup > saveState() const;

	/*! Replace entire inventory with a saved state; announced once. */
	void restoreState(const PropertyGroup* state);

	/*! Add listener, called with the inventory after every change. */
	void listen(IRuntimeDelegate* listener);

	/*! Remove every listener, such as those of a stage the player has left. */
	void removeAllListeners();

private:
	Ref< resource::IResourceManager > m_resourceManager;
	mutable SmallMap< Guid, resource::Proxy< ItemData > > m_items;
	RefArray< InventorySlot > m_slots;
	SmallMap< Guid, Guid > m_equipped;
	RefArray< IRuntimeDelegate > m_listeners;
	uint32_t m_serial = 0;
	bool m_notifying = false;
	bool m_pending = false;

	bool bindItem(const Guid& item, resource::Proxy< ItemData >& outItemData) const;

	void unequipGone(const Guid& item);

	void changed();
};

}

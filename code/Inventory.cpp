/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Inventory.h"

#include "Core/Class/Any.h"
#include "Core/Class/IRuntimeDelegate.h"
#include "Core/Misc/String.h"
#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyInteger.h"
#include "Core/Settings/PropertyString.h"
#include "InventorySlot.h"
#include "ItemData.h"
#include "Resource/IResourceManager.h"
#include "Traits/EquipTrait.h"
#include "World/Entity.h"

#include <algorithm>

namespace zombie
{
	namespace
	{

const int32_t c_maxNotifyPasses = 8;

const std::wstring c_equippedPrefix = L"equipped.";

std::wstring slotPrefix(int32_t index)
{
	return L"slot" + toString(index) + L".";
}

const EquipTrait* equipOf(const ItemData* itemData)
{
	return itemData != nullptr ? itemData->getTrait< EquipTrait >() : nullptr;
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.Inventory", Inventory, Object)

Inventory::Inventory(resource::IResourceManager* resourceManager, int32_t capacity)
:	m_resourceManager(resourceManager)
{
	m_slots.resize(std::max(capacity, 0));
}

const ItemData* Inventory::getItem(const Guid& item) const
{
	resource::Proxy< ItemData > itemData;
	return bindItem(item, itemData) ? itemData.getResource() : nullptr;
}

const InventorySlot* Inventory::getSlot(int32_t index) const
{
	return (index >= 0 && index < getCapacity()) ? m_slots[index] : nullptr;
}

int32_t Inventory::getUsed() const
{
	return (int32_t)std::count_if(m_slots.begin(), m_slots.end(), [](const InventorySlot* slot) { return slot != nullptr; });
}

int32_t Inventory::count(const Guid& item) const
{
	int32_t n = 0;
	for (auto slot : m_slots)
	{
		if (slot != nullptr && slot->m_item == item)
			n += slot->m_count;
	}
	return n;
}

int32_t Inventory::room(const Guid& item) const
{
	const ItemData* itemData = getItem(item);
	if (!itemData)
		return 0;

	const int32_t maxStack = std::max(itemData->getMaxStack(), 1);
	int32_t n = 0;
	for (auto slot : m_slots)
	{
		if (slot == nullptr)
			n += maxStack;
		else if (slot->m_item == item)
			n += std::max(maxStack - slot->m_count, 0);
	}
	return n;
}

int32_t Inventory::add(const Guid& item, int32_t count, world::Entity* kept)
{
	resource::Proxy< ItemData > itemData;
	if (count <= 0 || !bindItem(item, itemData))
		return 0;

	if (kept != nullptr)
	{
		// A kept item is its entity; the same one twice would be one item counted as two.
		if (holds(kept))
			return 0;
		count = 1;
	}

	const int32_t maxStack = std::max(itemData->getMaxStack(), 1);
	int32_t left = count;

	auto put = [&](InventorySlot* slot, int32_t n) {
		slot->m_count += n;
		left -= n;
		if (kept != nullptr)
			slot->m_kept.push_back(kept);
	};

	// Top up existing stacks first...
	for (auto slot : m_slots)
	{
		if (left > 0 && slot != nullptr && slot->m_item == item && slot->m_count < maxStack)
			put(slot, std::min(left, maxStack - slot->m_count));
	}

	// ...then open new stacks, front to back.
	for (int32_t i = 0; i < getCapacity() && left > 0; ++i)
	{
		if (getSlot(i) != nullptr)
			continue;

		Ref< InventorySlot > slot = new InventorySlot();
		slot->m_item = item;
		slot->m_itemData = itemData;
		m_slots[i] = slot;
		put(slot, std::min(left, maxStack));
	}

	const int32_t added = count - left;
	if (added > 0)
		changed();

	return added;
}

int32_t Inventory::remove(const Guid& item, int32_t count, RefArray< world::Entity >* outKept)
{
	int32_t left = std::max(count, 0);
	for (int32_t i = getCapacity() - 1; i >= 0 && left > 0; --i)
	{
		InventorySlot* slot = m_slots[i];
		if (slot == nullptr || slot->m_item != item)
			continue;

		const int32_t n = std::min(left, slot->m_count);
		slot->m_count -= n;
		left -= n;

		for (int32_t j = 0; j < n && !slot->m_kept.empty(); ++j)
		{
			if (outKept)
				outKept->push_back(slot->m_kept.back());
			slot->m_kept.pop_back();
		}

		if (slot->m_count <= 0)
			m_slots[i] = nullptr;
	}

	const int32_t removed = std::max(count, 0) - left;
	if (removed > 0)
	{
		unequipGone(item);
		changed();
	}
	return removed;
}

RefArray< world::Entity > Inventory::removeKept(const Guid& item, int32_t count)
{
	RefArray< world::Entity > kept;
	remove(item, count, &kept);
	return kept;
}

Ref< InventorySlot > Inventory::removeSlot(int32_t index)
{
	if (getSlot(index) == nullptr)
		return nullptr;

	Ref< InventorySlot > slot = m_slots[index];
	m_slots[index] = nullptr;

	unequipGone(slot->m_item);
	changed();
	return slot;
}

world::Entity* Inventory::first(const Guid& item) const
{
	for (auto slot : m_slots)
	{
		if (slot != nullptr && slot->m_item == item && !slot->m_kept.empty())
			return slot->m_kept.front();
	}
	return nullptr;
}

bool Inventory::holds(const world::Entity* entity) const
{
	if (entity == nullptr)
		return false;

	for (auto slot : m_slots)
	{
		if (slot != nullptr && std::find(slot->m_kept.begin(), slot->m_kept.end(), entity) != slot->m_kept.end())
			return true;
	}
	return false;
}

RefArray< world::Entity > Inventory::getKept() const
{
	RefArray< world::Entity > kept;
	for (auto slot : m_slots)
	{
		if (slot != nullptr)
		{
			for (auto entity : slot->m_kept)
				kept.push_back(entity);
		}
	}
	return kept;
}

void Inventory::attachKept(const Guid& item, const RefArray< world::Entity >& entities)
{
	auto it = entities.begin();
	for (auto slot : m_slots)
	{
		if (slot == nullptr || slot->m_item != item)
			continue;

		while ((int32_t)slot->m_kept.size() < slot->m_count && it != entities.end())
			slot->m_kept.push_back(*it++);
	}
}

void Inventory::detachKept()
{
	for (auto slot : m_slots)
	{
		if (slot != nullptr)
			slot->m_kept.clear();
	}
}

bool Inventory::keptMissing(int32_t index) const
{
	const InventorySlot* slot = getSlot(index);
	if (slot == nullptr)
		return false;

	const ItemData* itemData = slot->getItemData();
	if (itemData == nullptr || !itemData->isKeep())
		return false;

	return (int32_t)slot->m_kept.size() < slot->m_count;
}

Guid Inventory::getEquipped(const Guid& slot) const
{
	const auto it = m_equipped.find(slot);
	return it != m_equipped.end() ? it->second : Guid();
}

bool Inventory::isEquipped(const Guid& item) const
{
	const EquipTrait* equipTrait = equipOf(getItem(item));
	return equipTrait != nullptr && getEquipped(equipTrait->getSlot()) == item;
}

bool Inventory::equip(const Guid& item)
{
	const EquipTrait* equipTrait = equipOf(getItem(item));
	if (equipTrait == nullptr || equipTrait->getSlot().isNull() || count(item) <= 0)
		return false;

	Guid& equipped = m_equipped[equipTrait->getSlot()];
	if (equipped != item)
	{
		equipped = item;
		changed();
	}
	return true;
}

bool Inventory::unequip(const Guid& slot)
{
	const auto it = m_equipped.find(slot);
	if (it == m_equipped.end())
		return false;

	m_equipped.erase(it);
	changed();
	return true;
}

Ref< PropertyGroup > Inventory::saveState() const
{
	Ref< PropertyGroup > state = new PropertyGroup();
	for (int32_t i = 0; i < getCapacity(); ++i)
	{
		const InventorySlot* slot = m_slots[i];
		if (slot == nullptr)
			continue;

		const std::wstring prefix = slotPrefix(i);
		state->setProperty< PropertyString >(prefix + L"item", slot->m_item.format());
		state->setProperty< PropertyInteger >(prefix + L"count", slot->m_count);
	}
	for (const auto& it : m_equipped)
		state->setProperty< PropertyString >(c_equippedPrefix + it.first.format(), it.second.format());
	return state;
}

void Inventory::restoreState(const PropertyGroup* state)
{
	for (int32_t i = 0; i < getCapacity(); ++i)
		m_slots[i] = nullptr;
	m_equipped.clear();

	if (state != nullptr)
	{
		for (int32_t i = 0; i < getCapacity(); ++i)
		{
			const std::wstring prefix = slotPrefix(i);
			const Guid item(state->getProperty< std::wstring >(prefix + L"item", L""));
			const int32_t count = state->getProperty< int32_t >(prefix + L"count", 0);

			resource::Proxy< ItemData > itemData;
			if (count <= 0 || !bindItem(item, itemData))
				continue;

			Ref< InventorySlot > slot = new InventorySlot();
			slot->m_item = item;
			slot->m_itemData = itemData;
			slot->m_count = count;
			m_slots[i] = slot;
		}

		for (const auto& it : state->getValues())
		{
			if (!startsWith(it.first, c_equippedPrefix))
				continue;

			const Guid slot(it.first.substr(c_equippedPrefix.length()));
			const Guid item(PropertyString::get(it.second));
			if (slot.isValid() && slot.isNotNull() && count(item) > 0)
				m_equipped[slot] = item;
		}
	}

	changed();
}

void Inventory::listen(IRuntimeDelegate* listener)
{
	if (listener != nullptr)
		m_listeners.push_back(listener);
}

void Inventory::removeAllListeners()
{
	m_listeners.clear();
}

bool Inventory::bindItem(const Guid& item, resource::Proxy< ItemData >& outItemData) const
{
	if (item.isNull() || !item.isValid())
		return false;

	const auto it = m_items.find(item);
	if (it != m_items.end())
	{
		outItemData = it->second;
		return bool(outItemData);
	}

	// Unknown items are remembered as well, so they are not looked up again.
	const bool bound = m_resourceManager != nullptr && m_resourceManager->bind(resource::Id< ItemData >(item), outItemData);
	m_items[item] = outItemData;
	return bound;
}

void Inventory::unequipGone(const Guid& item)
{
	if (count(item) > 0)
		return;

	for (auto it = m_equipped.begin(); it != m_equipped.end();)
	{
		if (it->second == item)
			it = m_equipped.erase(it);
		else
			++it;
	}
}

void Inventory::changed()
{
	++m_serial;

	if (m_notifying)
	{
		m_pending = true;
		return;
	}

	m_notifying = true;
	for (int32_t pass = 0; pass < c_maxNotifyPasses; ++pass)
	{
		m_pending = false;

		const Any argv[] = { Any::fromObject(this) };
		const RefArray< IRuntimeDelegate > listeners = m_listeners;
		for (auto listener : listeners)
			listener->call(1, argv);

		if (!m_pending)
			break;
	}
	m_notifying = false;
}

}

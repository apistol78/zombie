/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ItemContainerComponent.h"

#include "Core/Misc/String.h"
#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyInteger.h"
#include "Core/Settings/PropertyString.h"
#include "Inventory.h"
#include "World/Entity.h"

#include <algorithm>

namespace zombie
{
	namespace
	{

std::wstring entryPrefix(int32_t index)
{
	return L"entry" + toString(index) + L".";
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.ItemContainerComponent", ItemContainerComponent, world::IEntityComponent)

void ItemContainerComponent::destroy()
{
	m_owner = nullptr;
}

void ItemContainerComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
}

void ItemContainerComponent::setTransform(const Transform& transform)
{
}

Aabb3 ItemContainerComponent::getBoundingBox() const
{
	return Aabb3();
}

void ItemContainerComponent::update(const world::UpdateParams& update)
{
}

Guid ItemContainerComponent::getItem(int32_t index) const
{
	return (index >= 0 && index < getEntryCount()) ? m_entries[index].item : Guid();
}

int32_t ItemContainerComponent::getCount(int32_t index) const
{
	return (index >= 0 && index < getEntryCount()) ? m_entries[index].count : 0;
}

int32_t ItemContainerComponent::count(const Guid& item) const
{
	int32_t n = 0;
	for (const auto& entry : m_entries)
	{
		if (entry.item == item)
			n += entry.count;
	}
	return n;
}

void ItemContainerComponent::add(const Guid& item, int32_t count)
{
	if (item.isNull() || count <= 0)
		return;

	auto it = std::find_if(m_entries.begin(), m_entries.end(), [&](const Entry& entry) { return entry.item == item; });
	if (it != m_entries.end())
		it->count += count;
	else
		m_entries.push_back({ item, count });

	++m_serial;
}

int32_t ItemContainerComponent::remove(int32_t index, int32_t count)
{
	if (index < 0 || index >= getEntryCount() || count <= 0)
		return 0;

	Entry& entry = m_entries[index];
	const int32_t removed = std::min(count, entry.count);
	entry.count -= removed;
	if (entry.count <= 0)
		m_entries.erase(m_entries.begin() + index);

	++m_serial;
	return removed;
}

int32_t ItemContainerComponent::moveTo(int32_t index, Inventory* inventory)
{
	if (inventory == nullptr || index < 0 || index >= getEntryCount())
		return 0;

	const int32_t added = inventory->add(m_entries[index].item, m_entries[index].count);
	return remove(index, added);
}

void ItemContainerComponent::clear()
{
	m_entries.clear();
	++m_serial;
}

Ref< PropertyGroup > ItemContainerComponent::saveState() const
{
	Ref< PropertyGroup > state = new PropertyGroup();
	state->setProperty< PropertyInteger >(L"entries", getEntryCount());
	for (int32_t i = 0; i < getEntryCount(); ++i)
	{
		const std::wstring prefix = entryPrefix(i);
		state->setProperty< PropertyString >(prefix + L"item", m_entries[i].item.format());
		state->setProperty< PropertyInteger >(prefix + L"count", m_entries[i].count);
	}
	return state;
}

void ItemContainerComponent::restoreState(const PropertyGroup* state)
{
	m_entries.clear();
	if (state != nullptr)
	{
		const int32_t entries = state->getProperty< int32_t >(L"entries", 0);
		for (int32_t i = 0; i < entries; ++i)
		{
			const std::wstring prefix = entryPrefix(i);
			const Guid item(state->getProperty< std::wstring >(prefix + L"item", L""));
			const int32_t count = state->getProperty< int32_t >(prefix + L"count", 0);
			if (item.isValid() && !item.isNull() && count > 0)
				m_entries.push_back({ item, count });
		}
	}
	++m_serial;
}

}

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
#include "Core/Serialization/ISerializable.h"
#include "Resource/Id.h"

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
class QuestData;

/*! Story; every quest, every item and the start kit of a playthrough.
 * \ingroup Zombie
 */
class T_DLLCLASS StoryData : public ISerializable
{
	T_RTTI_CLASS;

public:
	struct ItemStack
	{
		resource::Id< ItemData > item;
		int32_t count = 1;

		void serialize(ISerializer& s);
	};

	const AlignedVector< resource::Id< QuestData > >& getQuests() const { return m_quests; }

	AlignedVector< resource::Id< QuestData > >& getQuests() { return m_quests; }

	const AlignedVector< ItemStack >& getStartKit() const { return m_startKit; }

	AlignedVector< ItemStack >& getStartKit() { return m_startKit; }

	/*! Every item there is. */
	const AlignedVector< resource::Id< ItemData > >& getItems() const { return m_items; }

	AlignedVector< resource::Id< ItemData > >& getItems() { return m_items; }

	virtual void serialize(ISerializer& s) override final;

private:
	AlignedVector< resource::Id< QuestData > > m_quests;
	AlignedVector< ItemStack > m_startKit;
	AlignedVector< resource::Id< ItemData > > m_items;
};

}

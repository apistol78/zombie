/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "QuestStep.h"
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

/*! Find an item; done once the item has entered the inventory, however it got there.
 * \ingroup Zombie
 *
 * Until a quest reaches the step the item is held back (Story::isHeldBack).
 */
class T_DLLCLASS FindItemStep : public QuestStep
{
	T_RTTI_CLASS;

public:
	const resource::Id< ItemData >& getItem() const { return m_item; }

	void setItem(const resource::Id< ItemData >& item) { m_item = item; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ItemData > m_item;
};

}

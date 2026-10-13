/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "IItemTrait.h"
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

class EquipSlotData;

/*! Equip trait; the slot the item goes in when equipped, such as the hands.
 * \ingroup Zombie
 */
class T_DLLCLASS EquipTrait : public IItemTrait
{
	T_RTTI_CLASS;

public:
	EquipTrait() = default;

	explicit EquipTrait(const resource::Id< EquipSlotData >& slot);

	const resource::Id< EquipSlotData >& getSlot() const { return m_slot; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< EquipSlotData > m_slot;
};

}

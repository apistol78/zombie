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

#include <string>

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
class ItemData;
class LockComponent;

/*! Lock component data; the item spent using the owner entity's interactable.
 * \ingroup Zombie
 */
class T_DLLCLASS LockComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< LockComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	const resource::Id< ItemData >& getItem() const { return m_item; }

	/*! Slot the item must be equipped in; null if carrying it is enough. */
	const resource::Id< EquipSlotData >& getSlot() const { return m_slot; }

	/*! Said in reach when the item is carried but not equipped. */
	const std::wstring& getUnequippedRefusal() const { return m_unequippedRefusal; }

private:
	resource::Id< ItemData > m_item;
	resource::Id< EquipSlotData > m_slot;
	std::wstring m_unequippedRefusal;
};

}

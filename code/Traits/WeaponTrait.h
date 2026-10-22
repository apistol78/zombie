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

class ItemData;

/*! Weapon trait; what the item loads from the inventory when it is a weapon.
 * \ingroup Zombie
 */
class T_DLLCLASS WeaponTrait : public IItemTrait
{
	T_RTTI_CLASS;

public:
	/*! Item loaded as ammunition; null if it carries nothing spare. */
	const resource::Id< ItemData >& getAmmo() const { return m_ammo; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ItemData > m_ammo;
};

}

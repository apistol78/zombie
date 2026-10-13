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

/*! Armor trait; share of damage taken while the item is worn.
 * \ingroup Zombie
 */
class T_DLLCLASS ArmorTrait : public IItemTrait
{
	T_RTTI_CLASS;

public:
	explicit ArmorTrait(float fraction = 0.0f);

	float getFraction() const { return m_fraction; }

	virtual void serialize(ISerializer& s) override final;

private:
	float m_fraction = 0.0f;
};

}

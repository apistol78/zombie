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

/*! Heal trait; health restored when the item is used.
 * \ingroup Zombie
 */
class T_DLLCLASS HealTrait : public IItemTrait
{
	T_RTTI_CLASS;

public:
	explicit HealTrait(float amount = 0.0f);

	float getAmount() const { return m_amount; }

	virtual void serialize(ISerializer& s) override final;

private:
	float m_amount = 0.0f;
};

}

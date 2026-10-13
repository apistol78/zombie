/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ICondition.h"
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

/*! Inventory holds at least count of item; negated, fewer than count.
 * \ingroup Zombie
 */
class T_DLLCLASS HasItemCondition : public ICondition
{
	T_RTTI_CLASS;

public:
	HasItemCondition() = default;

	explicit HasItemCondition(const resource::Id< ItemData >& item, int32_t count, bool negate);

	const resource::Id< ItemData >& getItem() const { return m_item; }

	int32_t getCount() const { return m_count; }

	bool getNegate() const { return m_negate; }

	virtual bool evaluate(const Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ItemData > m_item;
	int32_t m_count = 1;
	bool m_negate = false;
};

}

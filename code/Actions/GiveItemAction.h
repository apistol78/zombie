/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "IAction.h"
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

/*! Put count of item in the inventory; announced as "itemGiven".
 * \ingroup Zombie
 */
class T_DLLCLASS GiveItemAction : public IAction
{
	T_RTTI_CLASS;

public:
	GiveItemAction() = default;

	explicit GiveItemAction(const resource::Id< ItemData >& item, int32_t count);

	const resource::Id< ItemData >& getItem() const { return m_item; }

	int32_t getCount() const { return m_count; }

	virtual void execute(Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ItemData > m_item;
	int32_t m_count = 1;
};

}

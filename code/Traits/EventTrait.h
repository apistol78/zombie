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

class EventData;

/*! Event trait; event raised when one of the item enters an inventory.
 * \ingroup Zombie
 */
class T_DLLCLASS EventTrait : public IItemTrait
{
	T_RTTI_CLASS;

public:
	EventTrait() = default;

	explicit EventTrait(const resource::Id< EventData >& event);

	const resource::Id< EventData >& getEvent() const { return m_event; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< EventData > m_event;
};

}

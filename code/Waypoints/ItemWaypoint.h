/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "IWaypoint.h"
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

/*! Waypoint at wherever an item is held; a pickup, a container or a carrier.
 * \ingroup Zombie
 */
class T_DLLCLASS ItemWaypoint : public IWaypoint
{
	T_RTTI_CLASS;

public:
	const resource::Id< ItemData >& getItem() const { return m_item; }

	/*! Points only until the item is found; off to keep pointing at the nearest one left. */
	bool isUntilFound() const { return m_untilFound; }

	void setItem(const resource::Id< ItemData >& item) { m_item = item; }

	void setUntilFound(bool untilFound) { m_untilFound = untilFound; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ItemData > m_item;
	bool m_untilFound = true;
};

}

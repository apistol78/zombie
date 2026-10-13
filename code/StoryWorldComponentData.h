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
#include "World/IWorldComponentData.h"

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

class StoryWorldComponent;

/*! Story world component data.
 * \ingroup Zombie
 */
class T_DLLCLASS StoryWorldComponentData : public world::IWorldComponentData
{
	T_RTTI_CLASS;

public:
	Ref< StoryWorldComponent > createComponent() const;

	/*! How often the world is looked through again for actors, places, pickups and containers (s). */
	float getRefreshInterval() const { return m_refreshInterval; }

	virtual void serialize(ISerializer& s) override final;

private:
	float m_refreshInterval = 1.0f;
};

}

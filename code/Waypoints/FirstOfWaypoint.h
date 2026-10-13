/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/RefArray.h"
#include "IWaypoint.h"

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

/*! First candidate present in the world the player is in.
 * \ingroup Zombie
 */
class T_DLLCLASS FirstOfWaypoint : public IWaypoint
{
	T_RTTI_CLASS;

public:
	const RefArray< IWaypoint >& getCandidates() const { return m_candidates; }

	RefArray< IWaypoint >& getCandidates() { return m_candidates; }

	virtual void serialize(ISerializer& s) override final;

private:
	RefArray< IWaypoint > m_candidates;
};

}

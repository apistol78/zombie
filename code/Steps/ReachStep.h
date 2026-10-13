/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "QuestStep.h"
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

class PlaceData;

/*! Reach a place; done when the player is within radius of it.
 * \ingroup Zombie
 */
class T_DLLCLASS ReachStep : public QuestStep
{
	T_RTTI_CLASS;

public:
	const resource::Id< PlaceData >& getPlace() const { return m_place; }

	void setPlace(const resource::Id< PlaceData >& place) { m_place = place; }

	float getRadius() const { return m_radius; }

	void setRadius(float radius) { m_radius = radius; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< PlaceData > m_place;
	float m_radius = 4.0f;
};

}

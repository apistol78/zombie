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
#include "Resource/Id.h"
#include "World/IEntityComponentData.h"

#include <string>

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
class StageExitComponent;

/*! Stage exit component data; where leaving through the owner entity leads.
 * \ingroup Zombie
 */
class T_DLLCLASS StageExitComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< StageExitComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	/*! Transition of the loading stage to take. */
	const std::wstring& getTarget() const { return m_target; }

	/*! Place in the destination to arrive at; null to arrive where its scene puts the player. */
	const resource::Id< PlaceData >& getArrival() const { return m_arrival; }

private:
	std::wstring m_target;
	resource::Id< PlaceData > m_arrival;
};

}

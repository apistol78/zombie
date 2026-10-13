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

class PlaceComponent;
class PlaceData;

/*! Place component data.
 * \ingroup Zombie
 *
 * Makes the owner entity a place quests can point at.
 */
class T_DLLCLASS PlaceComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< PlaceComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	const resource::Id< PlaceData >& getPlace() const { return m_place; }

	void setPlace(const resource::Id< PlaceData >& place) { m_place = place; }

private:
	resource::Id< PlaceData > m_place;
};

}

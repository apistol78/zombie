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
#include "World/IEntityComponentData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::world
{

class PropertyEntityReference;

}

namespace zombie
{

using namespace traktor;

class LeverComponent;

/*! Lever component data; the entity pulling the owner entity opens.
 * \ingroup Zombie
 */
class T_DLLCLASS LeverComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< LeverComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	/*! Entity opened, null if none. */
	const world::PropertyEntityReference* getTarget() const { return m_target; }

private:
	Ref< const world::PropertyEntityReference > m_target;
};

}

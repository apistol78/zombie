/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Guid.h"
#include "World/IEntityComponent.h"

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

/*! Stage exit component; where leaving through the owner entity leads.
 * \ingroup Zombie
 */
class T_DLLCLASS StageExitComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit StageExitComponent(const std::wstring& target, const Guid& arrival);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	/*! Transition of the loading stage to take. */
	const std::wstring& getTarget() const { return m_target; }

	/*! Place in the destination to arrive at; null to arrive where its scene puts the player. */
	const Guid& getArrival() const { return m_arrival; }

private:
	std::wstring m_target;
	Guid m_arrival;
};

}

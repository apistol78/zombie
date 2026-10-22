/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

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

/*! Landmark component; something the map marks.
 * \ingroup Zombie
 */
class T_DLLCLASS LandmarkComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit LandmarkComponent(const std::wstring& glyph);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	/*! Drawing the map gives it, in the game's art. */
	const std::wstring& getGlyph() const { return m_glyph; }

private:
	std::wstring m_glyph;
};

}

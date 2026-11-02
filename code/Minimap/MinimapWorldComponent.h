/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Math/Aabb2.h"
#include "Resource/Proxy.h"
#include "World/IWorldComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::render
{

class ITexture;

}

namespace zombie
{

using namespace traktor;

/*! Minimap world component; the world as seen from above, for scripts to bind to UI.
 * \ingroup Zombie
 *
 * U runs along +X and V along -Z of the bounds, so north is up.
 */
class T_DLLCLASS MinimapWorldComponent : public world::IWorldComponent
{
	T_RTTI_CLASS;

public:
	explicit MinimapWorldComponent(const resource::Proxy< render::ITexture >& texture, const Aabb2& bounds);

	virtual void destroy() override final;

	virtual void update(world::World* world, const world::UpdateParams& update) override final;

	render::ITexture* getTexture() const;

	/*! World XZ region covered by the texture. */
	const Aabb2& getBounds() const { return m_bounds; }

private:
	resource::Proxy< render::ITexture > m_texture;
	Aabb2 m_bounds;
};

}

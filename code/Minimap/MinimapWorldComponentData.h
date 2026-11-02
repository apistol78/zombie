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
#include "Core/Math/Aabb2.h"
#include "Resource/Id.h"
#include "World/IWorldComponentData.h"

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

namespace traktor::resource
{

class IResourceManager;

}

namespace zombie
{

using namespace traktor;

class MinimapWorldComponent;

/*! Minimap world component data.
 * \ingroup Zombie
 */
class T_DLLCLASS MinimapWorldComponentData : public world::IWorldComponentData
{
	T_RTTI_CLASS;

public:
	Ref< MinimapWorldComponent > createComponent(resource::IResourceManager* resourceManager) const;

	/*! Minimap texture of the world. */
	const resource::Id< render::ITexture >& getTexture() const { return m_texture; }

	/*! World XZ region covered by the texture; copied from the minimap texture when built. */
	const Aabb2& getBounds() const { return m_bounds; }

	virtual void serialize(ISerializer& s) override final;

private:
	friend class MinimapWorldComponentPipeline;

	resource::Id< render::ITexture > m_texture;
	Aabb2 m_bounds = Aabb2(Vector2::zero(), Vector2::zero());
};

}

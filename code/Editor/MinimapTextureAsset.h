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
#include "Core/Math/Aabb2.h"
#include "Core/Math/Color4f.h"
#include "Core/Serialization/ISerializable.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace zombie
{

using namespace traktor;

/*! Minimap texture asset; a scene rasterized from above.
 * \ingroup Zombie
 *
 * U runs along +X and V along -Z, so the texture is seen with +Z up.
 * Terrain is tinted from a low to a high color over its range of heights, with
 * elevation lines, everything below the ocean elevation as ocean outlined by a coastline. Models are drawn with
 * their own colors, or blended towards a flat color which keeps only their relief.
 */
class T_DLLCLASS MinimapTextureAsset : public ISerializable
{
	T_RTTI_CLASS;

public:
	virtual void serialize(ISerializer& s) override final;

private:
	friend class MinimapRasterizer;
	friend class MinimapTextureAssetEditorPage;
	friend class MinimapTexturePipeline;
	friend class MinimapWorldComponentPipeline;

	Guid m_source;
	Aabb2 m_bounds = Aabb2(Vector2(-512.0f, -512.0f), Vector2(512.0f, 512.0f));	//!< World XZ region covered by the texture.
	uint32_t m_size = 1024;
	Color4f m_terrainLowColor = Color4f(0.66f, 0.72f, 0.52f, 1.0f);	//!< At the ocean, or the lowest terrain if there is no ocean.
	Color4f m_terrainHighColor = Color4f(0.88f, 0.82f, 0.68f, 1.0f);	//!< At the highest terrain within bounds.
	Color4f m_contourColor = Color4f(0.55f, 0.42f, 0.30f, 1.0f);
	float m_contourInterval = 2.0f;
	Color4f m_oceanColor = Color4f(0.60f, 0.75f, 0.85f, 1.0f);
	Color4f m_coastColor = Color4f(0.20f, 0.36f, 0.52f, 1.0f);
	Color4f m_modelColor = Color4f(0.84f, 0.80f, 0.68f, 1.0f);
	float m_modelColorBlend = 0.0f;	//!< How much of the models' own colors is replaced by modelColor.
};

}

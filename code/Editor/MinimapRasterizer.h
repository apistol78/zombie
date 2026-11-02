/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <atomic>
#include "Core/Object.h"
#include "Core/Ref.h"
#include "Core/Containers/SmallMap.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::drawing
{

class Image;

}

namespace traktor::editor
{

class IPipelineCommon;
class IPipelineSettings;

}

namespace traktor::scene
{

class SceneOperatorChain;

}

namespace traktor::world
{

class IEntityReplicator;

}

namespace zombie
{

using namespace traktor;

class MinimapTextureAsset;

/*! Minimap rasterizer; draws the scene of a minimap texture asset from above.
 * \ingroup Zombie
 *
 * Shared by the minimap texture pipeline and the editor's preview, so both draw the
 * scene as built: after its geometric operators, static entities replicated into models,
 * terrain sampled from its heightfield.
 */
class T_DLLCLASS MinimapRasterizer : public Object
{
	T_RTTI_CLASS;

public:
	/*! What a rasterization drew. */
	struct Statistics
	{
		int32_t models = 0;
		int32_t terrains = 0;
	};

	bool create(const editor::IPipelineSettings* settings);

	void destroy();

	/*! Rasterize the asset's scene; null if it failed or was cancelled.
	 *
	 * \param cancelled Polled while rasterizing; may be null.
	 */
	Ref< drawing::Image > rasterize(editor::IPipelineCommon* pipelineCommon, const MinimapTextureAsset* asset, const std::atomic< bool >* cancelled, Statistics* outStatistics) const;

private:
	std::wstring m_assetPath;
	SmallMap< const TypeInfo*, Ref< const world::IEntityReplicator > > m_entityReplicators;
	Ref< scene::SceneOperatorChain > m_sceneOperatorChain;
};

}

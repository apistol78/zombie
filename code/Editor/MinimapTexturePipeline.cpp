/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapTexturePipeline.h"

#include "Core/Log/Log.h"
#include "Core/Misc/SafeDestroy.h"
#include "Drawing/Image.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/MinimapRasterizer.h"
#include "Editor/MinimapTextureAsset.h"
#include "Render/Editor/Texture/TextureOutput.h"
#include "Terrain/Editor/TerrainAsset.h"
#include "Terrain/TerrainComponentData.h"
#include "World/Editor/ResolveExternal.h"
#include "World/Editor/Traverser.h"
#include "World/EntityData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.MinimapTexturePipeline", 8, MinimapTexturePipeline, editor::DefaultPipeline)

bool MinimapTexturePipeline::create(const editor::IPipelineSettings* settings, db::Database* database)
{
	if (!editor::DefaultPipeline::create(settings, database))
		return false;

	m_rasterizer = new MinimapRasterizer();
	return m_rasterizer->create(settings);
}

void MinimapTexturePipeline::destroy()
{
	safeDestroy(m_rasterizer);
	editor::DefaultPipeline::destroy();
}

TypeInfoSet MinimapTexturePipeline::getAssetTypes() const
{
	return makeTypeInfoSet< MinimapTextureAsset >();
}

bool MinimapTexturePipeline::shouldCache() const
{
	return true;
}

bool MinimapTexturePipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	const MinimapTextureAsset* asset = mandatory_non_null_type_cast< const MinimapTextureAsset* >(sourceAsset);
	pipelineDepends->addDependency(asset->m_source, editor::PdfUse);
	pipelineDepends->addDependency< render::TextureOutput >();

	Ref< const ISerializable > sourceData = pipelineDepends->getObjectReadOnly(asset->m_source);
	if (!sourceData)
		return true;

	AlignedVector< Guid > externalEntities;
	sourceData = world::resolveExternal(
		[&](const Guid& objectId) -> Ref< const ISerializable > {
			return pipelineDepends->getObjectReadOnly(objectId);
		},
		sourceData,
		Guid::null,
		&externalEntities
	);

	for (const auto& externalEntity : externalEntities)
		pipelineDepends->addDependency(externalEntity, editor::PdfUse);

	// Terrain is sampled from its heightfield.
	world::Traverser::visit(sourceData, [&](const world::EntityData* entityData) -> world::Traverser::Result {
		if (auto terrainComponentData = entityData->getComponent< terrain::TerrainComponentData >())
		{
			pipelineDepends->addDependency(terrainComponentData->getTerrain(), editor::PdfUse);
			if (auto terrainAsset = pipelineDepends->getObjectReadOnly< terrain::TerrainAsset >(terrainComponentData->getTerrain()))
				pipelineDepends->addDependency(terrainAsset->getHeightfield(), editor::PdfUse);
		}
		return world::Traverser::Result::Continue;
	});

	return true;
}
bool MinimapTexturePipeline::buildOutput(
	editor::IPipelineBuilder* pipelineBuilder,
	const editor::PipelineDependencySet* dependencySet,
	const editor::PipelineDependency* dependency,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid,
	const Object* buildParams,
	uint32_t reason
) const
{
	const MinimapTextureAsset* asset = mandatory_non_null_type_cast< const MinimapTextureAsset* >(sourceAsset);

	MinimapRasterizer::Statistics statistics;
	Ref< drawing::Image > image = m_rasterizer->rasterize(pipelineBuilder, asset, nullptr, &statistics);
	if (!image)
	{
		log::error << L"Minimap texture pipeline failed; unable to rasterize scene." << Endl;
		return false;
	}

	log::info << L"Minimap rasterized from " << statistics.models << L" model(s) and " << statistics.terrains << L" terrain(s)." << Endl;

	Ref< render::TextureOutput > output = new render::TextureOutput();
	output->m_hasAlpha = true;
	output->m_enableCompression = true;

	return pipelineBuilder->buildAdHocOutput(
		output,
		outputPath,
		outputGuid,
		image
	);
}

}

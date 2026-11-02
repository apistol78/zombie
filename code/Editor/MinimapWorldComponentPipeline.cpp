/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapWorldComponentPipeline.h"

#include "Core/Log/Log.h"
#include "Core/Serialization/DeepClone.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/MinimapTextureAsset.h"
#include "Minimap/MinimapWorldComponentData.h"
#include "Render/ITexture.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.MinimapWorldComponentPipeline", 1, MinimapWorldComponentPipeline, world::EntityPipeline)

TypeInfoSet MinimapWorldComponentPipeline::getAssetTypes() const
{
	return makeTypeInfoSet< MinimapWorldComponentData >();
}

bool MinimapWorldComponentPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	const MinimapWorldComponentData* minimapWorldComponentData = mandatory_non_null_type_cast< const MinimapWorldComponentData* >(sourceAsset);
	pipelineDepends->addDependency(minimapWorldComponentData->getTexture(), editor::PdfBuild | editor::PdfResource);
	return true;
}

Ref< ISerializable > MinimapWorldComponentPipeline::buildProduct(
	editor::IPipelineBuilder* pipelineBuilder,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const Object* buildParams
) const
{
	const MinimapWorldComponentData* minimapWorldComponentData = mandatory_non_null_type_cast< const MinimapWorldComponentData* >(sourceAsset);

	// Bounds are authored in the minimap texture only.
	Ref< const MinimapTextureAsset > textureAsset = pipelineBuilder->getObjectReadOnly< MinimapTextureAsset >(minimapWorldComponentData->getTexture());
	if (!textureAsset)
	{
		log::error << L"Minimap world component pipeline failed; texture must be a minimap texture." << Endl;
		return nullptr;
	}

	Ref< MinimapWorldComponentData > outputData = DeepClone(minimapWorldComponentData).create< MinimapWorldComponentData >();
	outputData->m_bounds = textureAsset->m_bounds;
	return outputData;
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/ItemPipeline.h"

#include "Core/Containers/SmallSet.h"
#include "Database/Instance.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/StoryReferences.h"
#include "Editor/StoryValidation.h"
#include "ItemData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.ItemPipeline", 1, ItemPipeline, editor::DefaultPipeline)

TypeInfoSet ItemPipeline::getAssetTypes() const
{
	return makeTypeInfoSet< ItemData >();
}

bool ItemPipeline::buildDependencies(
	editor::IPipelineDepends* pipelineDepends,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const std::wstring& outputPath,
	const Guid& outputGuid
) const
{
	SmallSet< Guid > references;
	collectReferences(sourceAsset, references);
	for (const auto& reference : references)
		pipelineDepends->addDependency(reference, editor::PdfBuild | editor::PdfResource);
	return true;
}

bool ItemPipeline::buildOutput(
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
	if (!validateStoryAsset(pipelineBuilder, sourceAsset, sourceInstance ? sourceInstance->getName() : outputPath))
		return false;

	return editor::DefaultPipeline::buildOutput(
		pipelineBuilder,
		dependencySet,
		dependency,
		sourceInstance,
		sourceAsset,
		outputPath,
		outputGuid,
		buildParams,
		reason
	);
}

}

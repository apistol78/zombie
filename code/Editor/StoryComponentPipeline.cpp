/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryComponentPipeline.h"

#include "ActorComponentData.h"
#include "Core/Serialization/DeepClone.h"
#include "Core/Serialization/DeepHash.h"
#include "Database/Instance.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/StoryReferences.h"
#include "Editor/StoryValidation.h"
#include "ItemContainerComponentData.h"
#include "ItemPickupComponentData.h"
#include "PlaceComponentData.h"
#include "StoryWorldComponentData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.StoryComponentPipeline", 1, StoryComponentPipeline, editor::IPipeline)

bool StoryComponentPipeline::create(const editor::IPipelineSettings* settings, db::Database* database)
{
	return true;
}

void StoryComponentPipeline::destroy()
{
}

TypeInfoSet StoryComponentPipeline::getAssetTypes() const
{
	return makeTypeInfoSet< ActorComponentData, ItemContainerComponentData, ItemPickupComponentData, PlaceComponentData, StoryWorldComponentData >();
}

bool StoryComponentPipeline::shouldCache() const
{
	return false;
}

uint32_t StoryComponentPipeline::hashAsset(const ISerializable* sourceAsset) const
{
	return DeepHash(sourceAsset).get();
}

bool StoryComponentPipeline::buildDependencies(
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

bool StoryComponentPipeline::buildOutput(
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
	return false;
}

Ref< ISerializable > StoryComponentPipeline::buildProduct(
	editor::IPipelineBuilder* pipelineBuilder,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const Object* buildParams
) const
{
	// A component in a scene is named by its scene, if there is one.
	const std::wstring name = sourceInstance ? sourceInstance->getName() + L" (" + type_name(sourceAsset) + L")" : std::wstring(type_name(sourceAsset));
	if (!validateStoryAsset(pipelineBuilder, sourceAsset, name))
		return nullptr;

	return DeepClone(sourceAsset).create();
}

}

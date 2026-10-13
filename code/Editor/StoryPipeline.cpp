/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryPipeline.h"

#include "ActorData.h"
#include "Core/Log/Log.h"
#include "Database/Instance.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/StoryReferences.h"
#include "Editor/StoryValidation.h"
#include "Keys/EquipSlotData.h"
#include "Keys/EventData.h"
#include "Keys/FlagData.h"
#include "Keys/ScriptConditionData.h"
#include "PlaceData.h"
#include "QuestData.h"
#include "StoryData.h"

namespace zombie
{
T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.StoryPipeline", 1, StoryPipeline, editor::DefaultPipeline)

TypeInfoSet StoryPipeline::getAssetTypes() const
{
	TypeInfoSet types = makeTypeInfoSet< ActorData, PlaceData, QuestData, StoryData >();
	const TypeInfoSet keys = makeTypeInfoSet< FlagData, EventData, EquipSlotData, ScriptConditionData >();
	types.insert(keys.begin(), keys.end());
	return types;
}

bool StoryPipeline::buildDependencies(
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

bool StoryPipeline::buildOutput(
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
	const std::wstring name = sourceInstance ? sourceInstance->getName() : outputPath;
	if (!validateStoryAsset(pipelineBuilder, sourceAsset, name))
		return false;

	// The story's quests must find items that can be had.
	if (auto storyData = dynamic_type_cast< const StoryData* >(sourceAsset))
	{
		if (!validateObtainable(pipelineBuilder, storyData, name))
			return false;
	}

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

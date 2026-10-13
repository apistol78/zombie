/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/DialogPipeline.h"

#include "Core/Containers/SmallMap.h"
#include "Core/Containers/SmallSet.h"
#include "Core/Log/Log.h"
#include "Core/Misc/String.h"
#include "Database/Instance.h"
#include "DialogData.h"
#include "DialogNode.h"
#include "DialogResponse.h"
#include "Editor/DialogAsset.h"
#include "Editor/IPipelineBuilder.h"
#include "Editor/IPipelineDepends.h"
#include "Editor/StoryReferences.h"
#include "Editor/StoryValidation.h"

#include <functional>

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogPipeline", 1, DialogPipeline, editor::DefaultPipeline)

TypeInfoSet DialogPipeline::getAssetTypes() const
{
	return makeTypeInfoSet< DialogAsset >();
}

bool DialogPipeline::buildDependencies(
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

bool DialogPipeline::buildOutput(
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

	Ref< DialogData > dialogData = compile(checked_type_cast< const DialogAsset*, false >(sourceAsset), name);
	if (!dialogData)
		return false;

	return editor::DefaultPipeline::buildOutput(
		pipelineBuilder,
		dependencySet,
		dependency,
		sourceInstance,
		dialogData,
		outputPath,
		outputGuid,
		buildParams,
		reason
	);
}

Ref< DialogData > DialogPipeline::compile(const DialogAsset* dialogAsset, const std::wstring& name)
{
	bool ok = true;

	auto lineName = [](const DialogNodeAsset* nodeAsset) {
		return !nodeAsset->name.empty() ? L"\"" + nodeAsset->name + L"\"" : std::wstring(L"(unnamed)");
	};

	// Lines by guid, the links between them, and the responses by guid, what once-responses are remembered by.
	// Names are only what the editor shows; the same name twice is allowed, if confusing.
	SmallMap< Guid, int32_t > nodeIndices;
	SmallSet< Guid > responseIds;
	SmallSet< std::wstring > names;
	for (int32_t i = 0; i < (int32_t)dialogAsset->nodes.size(); ++i)
	{
		const DialogNodeAsset* nodeAsset = dialogAsset->nodes[i];
		if (!nodeAsset)
		{
			log::error << L"Dialog \"" << name << L"\"; line " << i + 1 << L" is empty." << Endl;
			return nullptr;
		}
		if (nodeAsset->id.isNull() || !nodeAsset->id.isValid() || !nodeIndices.insert(nodeAsset->id, i))
		{
			log::error << L"Dialog \"" << name << L"\"; line " << lineName(nodeAsset) << L" has no guid of its own." << Endl;
			ok = false;
		}
		if (nodeAsset->text.empty())
		{
			log::error << L"Dialog \"" << name << L"\"; line " << lineName(nodeAsset) << L" says nothing." << Endl;
			ok = false;
		}
		if (!nodeAsset->name.empty() && !names.insert(nodeAsset->name))
			log::warning << L"Dialog \"" << name << L"\"; more than one line named \"" << nodeAsset->name << L"\"." << Endl;

		for (int32_t j = 0; j < (int32_t)nodeAsset->responses.size(); ++j)
		{
			const DialogResponseAsset* responseAsset = nodeAsset->responses[j];
			if (!responseAsset)
			{
				log::error << L"Dialog \"" << name << L"\"; line " << lineName(nodeAsset) << L" response " << j + 1 << L" is empty." << Endl;
				return nullptr;
			}
			if (responseAsset->id.isNull() || !responseAsset->id.isValid() || !responseIds.insert(responseAsset->id))
			{
				log::error << L"Dialog \"" << name << L"\"; line " << lineName(nodeAsset) << L" response " << j + 1 << L" has no guid of its own." << Endl;
				ok = false;
			}
		}
	}

	auto indexOf = [&](const Guid& id, const std::wstring& where) -> int32_t {
		const auto it = nodeIndices.find(id);
		if (it != nodeIndices.end())
			return it->second;
		log::error << L"Dialog \"" << name << L"\"; " << where << L" leads to a line which does not exist." << Endl;
		ok = false;
		return -1;
	};

	Ref< DialogData > dialogData = new DialogData();
	dialogData->setName(dialogAsset->name);

	for (int32_t i = 0; i < (int32_t)dialogAsset->start.size(); ++i)
	{
		const DialogStartAsset* startAsset = dialogAsset->start[i];
		if (!startAsset || startAsset->node.isNull())
		{
			log::error << L"Dialog \"" << name << L"\"; start " << i + 1 << L" leads nowhere." << Endl;
			ok = false;
			continue;
		}
		dialogData->getStart().push_back({ indexOf(startAsset->node, L"start " + toString(i + 1)), startAsset->condition });
	}

	if (dialogData->getStart().empty() && ok)
	{
		if (dialogAsset->nodes.empty())
		{
			log::error << L"Dialog \"" << name << L"\"; no lines." << Endl;
			return nullptr;
		}
		dialogData->getStart().push_back({ 0, nullptr });
	}

	for (const auto nodeAsset : dialogAsset->nodes)
	{
		Ref< DialogNode > node = new DialogNode();
		node->setId(nodeAsset->id);
		node->setName(nodeAsset->name);
		node->setText(nodeAsset->text);
		node->getOnEnter() = nodeAsset->onEnter;

		for (const auto responseAsset : nodeAsset->responses)
		{
			Ref< DialogResponse > response = new DialogResponse();
			response->setId(responseAsset->id);
			response->setText(responseAsset->text);
			response->setTarget(responseAsset->target.isNotNull() ? indexOf(responseAsset->target, L"line " + lineName(nodeAsset) + L" response \"" + responseAsset->text + L"\"") : -1);
			response->setCondition(responseAsset->condition);
			response->getActions() = responseAsset->actions;
			response->setOnce(responseAsset->once);
			node->getResponses().push_back(response);
		}

		dialogData->getNodes().push_back(node);
	}

	if (!ok)
		return nullptr;

	// Lines nothing leads to are kept, but likely a mistake.
	AlignedVector< bool > reached(dialogData->getNodes().size(), false);
	std::function< void(int32_t) > reach = [&](int32_t node) {
		if (node < 0 || reached[node])
			return;
		reached[node] = true;
		for (auto response : dialogData->getNodes()[node]->getResponses())
			reach(response->getTarget());
	};
	for (const auto& start : dialogData->getStart())
		reach(start.node);
	for (int32_t i = 0; i < (int32_t)reached.size(); ++i)
	{
		if (!reached[i])
			log::warning << L"Dialog \"" << name << L"\"; nothing leads to line " << lineName(dialogAsset->nodes[i]) << L"." << Endl;
	}

	return dialogData;
}

}

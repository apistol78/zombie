/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryReferenceResolver.h"

#include "Core/Misc/String.h"
#include "Database/Database.h"
#include "Database/Group.h"
#include "Database/Instance.h"
#include "Database/Traverse.h"
#include "DialogNode.h"
#include "Editor/BrowsePartDialog.h"
#include "Editor/DialogAsset.h"
#include "Editor/IEditor.h"
#include "I18N/Text.h"
#include "QuestData.h"
#include "QuestStep.h"

#include <algorithm>

namespace zombie
{
	namespace
	{

const int32_t c_maxExcerpt = 40;

std::wstring excerpt(const std::wstring& text)
{
	return ((int32_t)text.length() <= c_maxExcerpt) ? text : text.substr(0, c_maxExcerpt - 3) + L"...";
}

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.StoryReferenceResolver", 0, StoryReferenceResolver, editor::IReferenceResolver)

bool StoryReferenceResolver::initialize(editor::IEditor* editor)
{
	Ref< db::Database > database = editor->getSourceDatabase();
	if (!database)
		return false;

	auto add = [&](const Guid& id, const std::wstring& name, const TypeInfo& type, db::Instance* holder) {
		if (id.isNull() || !id.isValid())
			return;
		m_index.insert(id, (int32_t)m_parts.size());
		m_parts.push_back({ id, name, &type, holder });
	};

	// Every quest's steps, as "quest > number. name".
	RefArray< db::Instance > quests;
	db::recursiveFindChildInstances(database->getRootGroup(), db::FindInstanceByType(type_of< QuestData >()), quests);
	for (auto instance : quests)
	{
		Ref< QuestData > questData = instance->getObject< QuestData >();
		if (!questData)
			continue;

		const auto& steps = questData->getSteps();
		for (int32_t i = 0; i < (int32_t)steps.size(); ++i)
		{
			if (!steps[i])
				continue;
			const std::wstring step = toString(i + 1) + L". " + (!steps[i]->getName().empty() ? steps[i]->getName() : excerpt(steps[i]->getText()));
			add(steps[i]->getId(), instance->getName() + L" > " + step, type_of< QuestStep >(), instance);
		}
	}

	// Every dialog's lines, as "dialog > name"; a line without a name by what it says.
	RefArray< db::Instance > dialogs;
	db::recursiveFindChildInstances(database->getRootGroup(), db::FindInstanceByType(type_of< DialogAsset >()), dialogs);
	for (auto instance : dialogs)
	{
		Ref< DialogAsset > dialogAsset = instance->getObject< DialogAsset >();
		if (!dialogAsset)
			continue;

		for (auto node : dialogAsset->nodes)
		{
			if (!node)
				continue;
			const std::wstring line = !node->name.empty() ? node->name : L"\"" + excerpt(node->text) + L"\"";
			add(node->id, instance->getName() + L" > " + line, type_of< DialogNode >(), instance);
		}
	}
	return true;
}

TypeInfoSet StoryReferenceResolver::getReferenceTypes() const
{
	return makeTypeInfoSet< QuestStep, DialogNode >();
}

bool StoryReferenceResolver::resolve(const Guid& id, std::wstring& outName) const
{
	const auto it = m_index.find(id);
	if (it == m_index.end())
		return false;

	outName = m_parts[it->second].name;
	return true;
}

bool StoryReferenceResolver::browse(ui::Widget* parent, const TypeInfo& referenceType, const Guid& current, Guid& outId) const
{
	AlignedVector< const Part* > parts;
	for (const auto& part : m_parts)
	{
		if (is_type_of(referenceType, *part.type))
			parts.push_back(&part);
	}
	std::stable_sort(parts.begin(), parts.end(), [](const Part* lh, const Part* rh) {
		return compareIgnoreCase(lh->name, rh->name) < 0;
	});

	AlignedVector< std::wstring > names;
	int32_t selected = -1;
	for (int32_t i = 0; i < (int32_t)parts.size(); ++i)
	{
		names.push_back(parts[i]->name);
		if (parts[i]->id == current)
			selected = i;
	}

	const std::wstring title = i18n::Text(is_type_of< QuestStep >(referenceType) ? L"STORY_BROWSE_STEP" : L"STORY_BROWSE_LINE");

	BrowsePartDialog dialog;
	if (!dialog.create(parent, title, names, selected))
		return false;

	const bool picked = (dialog.showModal() == ui::DialogResult::Ok && dialog.getSelected() >= 0);
	if (picked)
		outId = parts[dialog.getSelected()]->id;

	dialog.destroy();
	return picked;
}

Ref< db::Instance > StoryReferenceResolver::getOwnerInstance(const Guid& id) const
{
	const auto it = m_index.find(id);
	return (it != m_index.end()) ? m_parts[it->second].holder : nullptr;
}

}

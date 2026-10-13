/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/QuestEditorPageFactory.h"

#include "Core/Serialization/DeepClone.h"
#include "Editor/QuestEditorPage.h"
#include "QuestData.h"
#include "Ui/Command.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.QuestEditorPageFactory", 0, QuestEditorPageFactory, editor::IEditorPageFactory)

const TypeInfoSet QuestEditorPageFactory::getEditableTypes() const
{
	return makeTypeInfoSet< QuestData >();
}

bool QuestEditorPageFactory::needOutputResources(const TypeInfo& typeInfo, std::set< Guid >& outDependencies) const
{
	return false;
}

Ref< editor::IEditorPage > QuestEditorPageFactory::createEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document) const
{
	return new QuestEditorPage(editor, site, document);
}

void QuestEditorPageFactory::getCommands(std::list< ui::Command >& outCommands) const
{
	outCommands.push_back(ui::Command(L"Quest.Editor.AddFindItem"));
	outCommands.push_back(ui::Command(L"Quest.Editor.AddTalk"));
	outCommands.push_back(ui::Command(L"Quest.Editor.AddReach"));
	outCommands.push_back(ui::Command(L"Quest.Editor.AddTrigger"));
	outCommands.push_back(ui::Command(L"Quest.Editor.MoveUp"));
	outCommands.push_back(ui::Command(L"Quest.Editor.MoveDown"));
}

Ref< ISerializable > QuestEditorPageFactory::cloneAsset(const ISerializable* asset) const
{
	return DeepClone(asset).create();
}

}

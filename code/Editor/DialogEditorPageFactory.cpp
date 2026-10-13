/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/DialogEditorPageFactory.h"

#include "Core/Serialization/DeepClone.h"
#include "Editor/DialogAsset.h"
#include "Editor/DialogEditorPage.h"
#include "Ui/Command.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogEditorPageFactory", 0, DialogEditorPageFactory, editor::IEditorPageFactory)

const TypeInfoSet DialogEditorPageFactory::getEditableTypes() const
{
	return makeTypeInfoSet< DialogAsset >();
}

bool DialogEditorPageFactory::needOutputResources(const TypeInfo& typeInfo, std::set< Guid >& outDependencies) const
{
	return false;
}

Ref< editor::IEditorPage > DialogEditorPageFactory::createEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document) const
{
	return new DialogEditorPage(editor, site, document);
}

void DialogEditorPageFactory::getCommands(std::list< ui::Command >& outCommands) const
{
	outCommands.push_back(ui::Command(L"Dialog.Editor.CreateLine"));
	outCommands.push_back(ui::Command(L"Dialog.Editor.AddResponse"));
	outCommands.push_back(ui::Command(L"Dialog.Editor.AddStart"));
	outCommands.push_back(ui::Command(L"Dialog.Editor.Arrange"));
}

Ref< ISerializable > DialogEditorPageFactory::cloneAsset(const ISerializable* asset) const
{
	return DeepClone(asset).create();
}

}

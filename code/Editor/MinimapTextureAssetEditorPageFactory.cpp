/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapTextureAssetEditorPageFactory.h"

#include "Core/Serialization/DeepClone.h"
#include "Editor/MinimapTextureAsset.h"
#include "Editor/MinimapTextureAssetEditorPage.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.MinimapTextureAssetEditorPageFactory", 0, MinimapTextureAssetEditorPageFactory, editor::IEditorPageFactory)

const TypeInfoSet MinimapTextureAssetEditorPageFactory::getEditableTypes() const
{
	return makeTypeInfoSet< MinimapTextureAsset >();
}

bool MinimapTextureAssetEditorPageFactory::needOutputResources(const TypeInfo& typeInfo, std::set< Guid >& outDependencies) const
{
	return false;
}

Ref< editor::IEditorPage > MinimapTextureAssetEditorPageFactory::createEditorPage(editor::IEditor* editor, editor::IEditorPageSite* site, editor::IDocument* document) const
{
	return new MinimapTextureAssetEditorPage(editor, site, document);
}

void MinimapTextureAssetEditorPageFactory::getCommands(std::list< ui::Command >& outCommands) const
{
}

Ref< ISerializable > MinimapTextureAssetEditorPageFactory::cloneAsset(const ISerializable* asset) const
{
	return DeepClone(asset).create();
}

}

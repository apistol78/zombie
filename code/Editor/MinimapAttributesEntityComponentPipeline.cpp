/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapAttributesEntityComponentPipeline.h"

#include "Editor/MinimapAttributesEntityComponentData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.MinimapAttributesEntityComponentPipeline", 0, MinimapAttributesEntityComponentPipeline, world::EntityPipeline)

TypeInfoSet MinimapAttributesEntityComponentPipeline::getAssetTypes() const
{
	return makeTypeInfoSet< MinimapAttributesEntityComponentData >();
}

Ref< ISerializable > MinimapAttributesEntityComponentPipeline::buildProduct(
	editor::IPipelineBuilder* pipelineBuilder,
	const db::Instance* sourceInstance,
	const ISerializable* sourceAsset,
	const Object* buildParams
) const
{
	return nullptr;
}

}

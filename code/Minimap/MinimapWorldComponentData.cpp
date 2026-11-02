/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Minimap/MinimapWorldComponentData.h"

#include "Core/Serialization/AttributePrivate.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Minimap/MinimapWorldComponent.h"
#include "Render/ITexture.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.MinimapWorldComponentData", 0, MinimapWorldComponentData, world::IWorldComponentData)

Ref< MinimapWorldComponent > MinimapWorldComponentData::createComponent(resource::IResourceManager* resourceManager) const
{
	resource::Proxy< render::ITexture > texture;
	if (m_texture && !resourceManager->bind(m_texture, texture))
		return nullptr;
	return new MinimapWorldComponent(texture, m_bounds);
}

void MinimapWorldComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< render::ITexture >(L"texture", m_texture);
	s >> Member< Vector2 >(L"boundsMin", m_bounds.mn, AttributePrivate());
	s >> Member< Vector2 >(L"boundsMax", m_bounds.mx, AttributePrivate());
}

}

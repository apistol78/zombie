/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Minimap/MinimapWorldComponent.h"

#include "Render/ITexture.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.MinimapWorldComponent", MinimapWorldComponent, world::IWorldComponent)

MinimapWorldComponent::MinimapWorldComponent(const resource::Proxy< render::ITexture >& texture, const Aabb2& bounds)
:	m_texture(texture)
,	m_bounds(bounds)
{
}

void MinimapWorldComponent::destroy()
{
	m_texture.clear();
}

void MinimapWorldComponent::update(world::World* world, const world::UpdateParams& update)
{
}

render::ITexture* MinimapWorldComponent::getTexture() const
{
	return m_texture.getResource();
}

}

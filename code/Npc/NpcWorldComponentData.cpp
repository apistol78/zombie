/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/NpcWorldComponentData.h"

#include "Core/Serialization/ISerializer.h"
#include "Core/Settings/PropertyGroup.h"
#include "Npc/NpcWorldComponent.h"
#include "Resource/IResourceManager.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.NpcWorldComponentData", 0, NpcWorldComponentData, world::IWorldComponentData)

Ref< NpcWorldComponent > NpcWorldComponentData::createComponent(resource::IResourceManager* resourceManager, physics::PhysicsManager* physicsManager) const
{
	resource::Proxy< PropertyGroup > settings;
	if (resourceManager && m_settings)
		resourceManager->bind(m_settings, settings);
	return new NpcWorldComponent(settings, physicsManager);
}

void NpcWorldComponentData::serialize(ISerializer& s)
{
	s >> resource::Member< PropertyGroup >(L"settings", m_settings);
}

}

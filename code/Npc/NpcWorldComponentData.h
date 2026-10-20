/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Ref.h"
#include "Resource/Id.h"
#include "World/IWorldComponentData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class PropertyGroup;

}

namespace traktor::physics
{

class PhysicsManager;

}

namespace traktor::resource
{

class IResourceManager;

}

namespace zombie
{

using namespace traktor;

class NpcWorldComponent;

/*! NPC world component data.
 * \ingroup Zombie
 */
class T_DLLCLASS NpcWorldComponentData : public world::IWorldComponentData
{
	T_RTTI_CLASS;

public:
	Ref< NpcWorldComponent > createComponent(resource::IResourceManager* resourceManager, physics::PhysicsManager* physicsManager) const;

	virtual void serialize(ISerializer& s) override final;

	/*! Game play settings the characters are tuned by. */
	const resource::Id< PropertyGroup >& getSettings() const { return m_settings; }

private:
	resource::Id< PropertyGroup > m_settings;
};

}

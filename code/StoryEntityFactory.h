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
#include "World/AbstractEntityFactory.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::resource
{

class IResourceManager;

}

namespace zombie
{

using namespace traktor;

/*! Story entity factory.
 * \ingroup Zombie
 */
class T_DLLCLASS StoryEntityFactory : public world::AbstractEntityFactory
{
	T_RTTI_CLASS;

public:
	virtual bool initialize(const ObjectStore& objectStore) override final;

	virtual const TypeInfoSet getEntityComponentTypes() const override final;

	virtual Ref< world::IEntityComponent > createEntityComponent(const world::IEntityBuilder* builder, const world::IEntityComponentData& entityComponentData) const override final;

	virtual const TypeInfoSet getWorldComponentTypes() const override final;

	virtual Ref< world::IWorldComponent > createWorldComponent(const world::IEntityBuilder* builder, const world::IWorldComponentData& worldComponentData) const override final;

private:
	Ref< resource::IResourceManager > m_resourceManager;
};

}

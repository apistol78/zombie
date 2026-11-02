/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "World/IEntityComponentData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace zombie
{

using namespace traktor;

/*! Editor-only minimap attributes.
 * \ingroup Zombie
 *
 * How the owner entity is drawn in minimap textures; entities without
 * attributes are included. Stripped when built.
 */
class T_DLLCLASS MinimapAttributesEntityComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	bool getIncludeInMinimap() const { return m_includeInMinimap; }

private:
	bool m_includeInMinimap = true;
};

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Ref.h"
#include "Resource/Id.h"
#include "World/IEntityComponentData.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace zombie
{

using namespace traktor;

class ItemContainerComponent;
class ItemData;

/*! Item container component data; what the owner entity holds.
 * \ingroup Zombie
 */
class T_DLLCLASS ItemContainerComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	struct Entry
	{
		resource::Id< ItemData > item;
		int32_t count = 1;

		void serialize(ISerializer& s);
	};

	Ref< ItemContainerComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	const AlignedVector< Entry >& getEntries() const { return m_entries; }

	AlignedVector< Entry >& getEntries() { return m_entries; }

private:
	AlignedVector< Entry > m_entries;
};

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Guid.h"
#include "Core/Ref.h"
#include "World/IEntityComponent.h"

#include <string>

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

class LockComponentData;

/*! Lock component; the item spent using the owner entity's interactable.
 * \ingroup Zombie
 */
class T_DLLCLASS LockComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit LockComponent(const LockComponentData* data);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	const Guid& getItem() const;

	/*! Slot the item must be equipped in; null if carrying it is enough. */
	const Guid& getSlot() const;

	const std::wstring& getUnequippedRefusal() const;

private:
	Ref< const LockComponentData > m_data;
};

}

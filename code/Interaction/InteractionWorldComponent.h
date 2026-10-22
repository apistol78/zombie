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
#include "Core/Math/Vector4.h"
#include "Core/RefArray.h"
#include "World/IWorldComponent.h"

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

class InteractableComponent;
class Inventory;

/*! Interaction world component; every interactable in the world, and which one the player would use.
 * \ingroup Zombie
 *
 * Of several candidates the highest priority is picked, then the nearest.
 */
class T_DLLCLASS InteractionWorldComponent : public world::IWorldComponent
{
	T_RTTI_CLASS;

public:
	virtual void destroy() override final;

	virtual void update(world::World* world, const world::UpdateParams& update) override final;

	void add(InteractableComponent* interactable);

	void remove(InteractableComponent* interactable);

	/*! What the use key would use at position, null if nothing in reach can be used. */
	InteractableComponent* findUsable(const Vector4& position, const Inventory* inventory) const;

	/*! What in reach cannot be used and says why, null if none. */
	InteractableComponent* findRefused(const Vector4& position, const Inventory* inventory) const;

	/*! Anywhere, one whose lock's item the inventory carries; null if none. */
	InteractableComponent* findSpendable(const Vector4& position, const Inventory* inventory) const;

	/*! Anywhere, a pickup of an item some lock wants; null if none. */
	InteractableComponent* findWanted(const Vector4& position, const Inventory* inventory) const;

	/*! Number of pickups of item in the world, carried or not. */
	int32_t countPickups(const Guid& item) const;

private:
	enum class Find
	{
		Usable,
		Refused,
		Spendable,
		Wanted
	};

	RefArray< InteractableComponent > m_interactables;

	InteractableComponent* find(Find what, const Vector4& position, const Inventory* inventory) const;
};

}

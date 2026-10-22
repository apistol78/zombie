/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/InteractionWorldComponent.h"

#include "Core/Containers/SmallSet.h"
#include "Interaction/InteractableComponent.h"
#include "Interaction/InteractableComponentData.h"
#include "Interaction/LockComponent.h"
#include "Inventory.h"
#include "ItemPickupComponent.h"
#include "World/Entity.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.InteractionWorldComponent", InteractionWorldComponent, world::IWorldComponent)

void InteractionWorldComponent::destroy()
{
	m_interactables.clear();
}

void InteractionWorldComponent::update(world::World* world, const world::UpdateParams& update)
{
}

void InteractionWorldComponent::add(InteractableComponent* interactable)
{
	m_interactables.push_back(interactable);
}

void InteractionWorldComponent::remove(InteractableComponent* interactable)
{
	m_interactables.remove(interactable);
}

InteractableComponent* InteractionWorldComponent::findUsable(const Vector4& position, const Inventory* inventory) const
{
	return find(Find::Usable, position, inventory);
}

InteractableComponent* InteractionWorldComponent::findRefused(const Vector4& position, const Inventory* inventory) const
{
	return find(Find::Refused, position, inventory);
}

InteractableComponent* InteractionWorldComponent::findSpendable(const Vector4& position, const Inventory* inventory) const
{
	return find(Find::Spendable, position, inventory);
}

InteractableComponent* InteractionWorldComponent::findWanted(const Vector4& position, const Inventory* inventory) const
{
	return find(Find::Wanted, position, inventory);
}

int32_t InteractionWorldComponent::countPickups(const Guid& item) const
{
	int32_t count = 0;
	for (auto interactable : m_interactables)
	{
		world::Entity* owner = interactable->getOwner();
		const ItemPickupComponent* pickup = owner ? owner->getComponent< ItemPickupComponent >() : nullptr;
		if (pickup && pickup->getItem() == item)
			++count;
	}
	return count;
}

InteractableComponent* InteractionWorldComponent::find(Find what, const Vector4& position, const Inventory* inventory) const
{
	if (!inventory)
		return nullptr;

	// Items some enabled lock in the world wants.
	SmallSet< Guid > wanted;
	if (what == Find::Wanted)
	{
		for (auto interactable : m_interactables)
		{
			const LockComponent* lock = interactable->getLock();
			if (lock && interactable->isEnabled())
				wanted.insert(lock->getItem());
		}
	}

	InteractableComponent* best = nullptr;
	int32_t bestPriority = 0;
	Scalar bestDistance(0.0f);

	for (auto interactable : m_interactables)
	{
		if (!interactable->isAvailable(inventory))
			continue;

		const Vector4 offset = (interactable->getOwner()->getTransform().translation() - position).xyz0();
		Scalar distance;

		if (what == Find::Usable || what == Find::Refused)
		{
			distance = offset.length();
			if (distance > Scalar(interactable->getData()->getRange()))
				continue;
			if (interactable->isUsable(inventory) != (what == Find::Usable))
				continue;
			if (what == Find::Refused && interactable->getRefusal(inventory).empty())
				continue;
		}
		else
		{
			// Anywhere is measured flat; height only adds the slope the player stands on.
			distance = (offset * Vector4(1.0f, 0.0f, 1.0f, 0.0f)).length();
			if (what == Find::Spendable)
			{
				const LockComponent* lock = interactable->getLock();
				if (!lock || inventory->count(lock->getItem()) <= 0)
					continue;
			}
			else
			{
				const ItemPickupComponent* pickup = interactable->getOwner()->getComponent< ItemPickupComponent >();
				if (!pickup || wanted.find(pickup->getItem()) == wanted.end())
					continue;
			}
		}

		const int32_t priority = interactable->getPriority();
		if (!best || priority > bestPriority || (priority == bestPriority && distance < bestDistance))
		{
			best = interactable;
			bestPriority = priority;
			bestDistance = distance;
		}
	}

	return best;
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/InteractableComponent.h"

#include "Interaction/InteractableComponentData.h"
#include "Interaction/InteractionWorldComponent.h"
#include "Interaction/LockComponent.h"
#include "Inventory.h"
#include "World/Entity.h"
#include "World/World.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.InteractableComponent", InteractableComponent, world::IEntityComponent)

InteractableComponent::InteractableComponent(const InteractableComponentData* data)
:	m_data(data)
,	m_prompt(data->getPrompt())
{
}

void InteractableComponent::destroy()
{
	setWorld(nullptr);
	m_owner = nullptr;
}

void InteractableComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
}

void InteractableComponent::setWorld(world::World* world)
{
	if (m_interaction)
		m_interaction->remove(this);

	m_interaction = world ? world->getComponent< InteractionWorldComponent >() : nullptr;

	if (m_interaction)
		m_interaction->add(this);
}

void InteractableComponent::setTransform(const Transform& transform)
{
}

Aabb3 InteractableComponent::getBoundingBox() const
{
	return Aabb3();
}

void InteractableComponent::update(const world::UpdateParams& update)
{
}

const LockComponent* InteractableComponent::getLock() const
{
	return m_owner ? m_owner->getComponent< LockComponent >() : nullptr;
}

const std::wstring& InteractableComponent::getNotice() const
{
	return m_data->getNotice();
}

int32_t InteractableComponent::getPriority() const
{
	return m_data->getPriority();
}

bool InteractableComponent::isAvailable(const Inventory* inventory) const
{
	return m_enabled && m_owner != nullptr && m_owner->isVisible() && !inventory->holds(m_owner);
}

bool InteractableComponent::isUsable(const Inventory* inventory) const
{
	if (m_prompt.empty())
		return false;

	const LockComponent* lock = getLock();
	if (!lock)
		return true;

	if (inventory->count(lock->getItem()) <= 0)
		return false;

	return lock->getSlot().isNull() || inventory->getEquipped(lock->getSlot()) == lock->getItem();
}

const std::wstring& InteractableComponent::getRefusal(const Inventory* inventory) const
{
	const LockComponent* lock = getLock();
	if (lock && lock->getSlot().isNotNull() && inventory->count(lock->getItem()) > 0 && inventory->getEquipped(lock->getSlot()) != lock->getItem())
		return lock->getUnequippedRefusal();
	else
		return m_data->getRefusal();
}

}

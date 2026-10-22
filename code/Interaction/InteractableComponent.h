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

class InteractableComponentData;
class InteractionWorldComponent;
class Inventory;
class LockComponent;

/*! Interactable component; something the use key does at the owner entity.
 * \ingroup Zombie
 *
 * Joins the world's InteractionWorldComponent while the owner is in a world.
 */
class T_DLLCLASS InteractableComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit InteractableComponent(const InteractableComponentData* data);

	virtual void destroy() override final;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setWorld(world::World* world) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual void update(const world::UpdateParams& update) override final;

	world::Entity* getOwner() const { return m_owner; }

	const InteractableComponentData* getData() const { return m_data; }

	/*! Lock of the owner entity, null if using it is free. */
	const LockComponent* getLock() const;

	/*! Not offered while disabled; disabled once done with. */
	void setEnabled(bool enabled) { m_enabled = enabled; }

	bool isEnabled() const { return m_enabled; }

	/*! What using it is called; the data's until changed. */
	void setPrompt(const std::wstring& prompt) { m_prompt = prompt; }

	const std::wstring& getPrompt() const { return m_prompt; }

	const std::wstring& getNotice() const;

	int32_t getPriority() const;

	/*! Enabled, shown, and not carried in the inventory. */
	bool isAvailable(const Inventory* inventory) const;

	/*! It has a prompt, and the inventory meets its lock. */
	bool isUsable(const Inventory* inventory) const;

	/*! What is said when it cannot be used. */
	const std::wstring& getRefusal(const Inventory* inventory) const;

private:
	Ref< const InteractableComponentData > m_data;
	world::Entity* m_owner = nullptr;
	InteractionWorldComponent* m_interaction = nullptr;
	std::wstring m_prompt;
	bool m_enabled = true;
};

}

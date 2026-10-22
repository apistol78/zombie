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
#include "World/IEntityComponentData.h"

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

class InteractableComponent;

/*! Interactable component data; what the use key does at the owner entity.
 * \ingroup Zombie
 */
class T_DLLCLASS InteractableComponentData : public world::IEntityComponentData
{
	T_RTTI_CLASS;

public:
	Ref< InteractableComponent > createComponent() const;

	virtual int32_t getOrdinal() const override final;

	virtual void setTransform(const world::EntityData* owner, const Transform& transform) override final;

	virtual void serialize(ISerializer& s) override final;

	/*! What using it is called; empty if it is never used. */
	const std::wstring& getPrompt() const { return m_prompt; }

	/*! Said in reach when it cannot be used; empty to say nothing. */
	const std::wstring& getRefusal() const { return m_refusal; }

	/*! Said once it has been used. */
	const std::wstring& getNotice() const { return m_notice; }

	/*! How close the player must come (m). */
	float getRange() const { return m_range; }

	/*! Higher is offered first, however near the others are. */
	int32_t getPriority() const { return m_priority; }

private:
	std::wstring m_prompt;
	std::wstring m_refusal;
	std::wstring m_notice;
	float m_range = 2.0f;
	int32_t m_priority = 0;
};

}

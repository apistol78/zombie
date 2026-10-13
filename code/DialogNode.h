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
#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "DialogResponse.h"
#include "IAction.h"

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

/*! A line of a dialog, and the responses to it.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogNode : public ISerializable
{
	T_RTTI_CLASS;

public:
	/*! The line's guid, as in the dialog asset. */
	const Guid& getId() const { return m_id; }

	void setId(const Guid& id) { m_id = id; }

	/*! The line's name in the dialog asset, for logs; may be empty. */
	const std::wstring& getName() const { return m_name; }

	void setName(const std::wstring& name) { m_name = name; }

	/*! What the actor says. */
	const std::wstring& getText() const { return m_text; }

	void setText(const std::wstring& text) { m_text = text; }

	/*! Actions run when the conversation arrives at the node. */
	const RefArray< IAction >& getOnEnter() const { return m_onEnter; }

	RefArray< IAction >& getOnEnter() { return m_onEnter; }

	const RefArray< DialogResponse >& getResponses() const { return m_responses; }

	RefArray< DialogResponse >& getResponses() { return m_responses; }

	virtual void serialize(ISerializer& s) override final;

private:
	Guid m_id;
	std::wstring m_name;
	std::wstring m_text;
	RefArray< IAction > m_onEnter;
	RefArray< DialogResponse > m_responses;
};

}

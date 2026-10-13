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
#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "IAction.h"
#include "ICondition.h"

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

/*! Response the player can give at a dialog node.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogResponse : public ISerializable
{
	T_RTTI_CLASS;

public:
	/*! The response's guid, as in the dialog asset; what a once-response is remembered by. */
	const Guid& getId() const { return m_id; }

	void setId(const Guid& id) { m_id = id; }

	const std::wstring& getText() const { return m_text; }

	void setText(const std::wstring& text) { m_text = text; }

	/*! Node the response leads to, index into the dialog's nodes; -1 ends the conversation. */
	int32_t getTarget() const { return m_target; }

	void setTarget(int32_t target) { m_target = target; }

	/*! Response is offered only while condition holds; null always. */
	const ICondition* getCondition() const { return m_condition; }

	void setCondition(ICondition* condition) { m_condition = condition; }

	const RefArray< IAction >& getActions() const { return m_actions; }

	RefArray< IAction >& getActions() { return m_actions; }

	/*! Response is offered only until chosen once. */
	bool isOnce() const { return m_once; }

	void setOnce(bool once) { m_once = once; }

	virtual void serialize(ISerializer& s) override final;

private:
	Guid m_id;
	std::wstring m_text;
	int32_t m_target = -1;
	Ref< ICondition > m_condition;
	RefArray< IAction > m_actions;
	bool m_once = false;
};

}

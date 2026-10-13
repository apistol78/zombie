/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "IAction.h"
#include "Resource/Id.h"

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

class EventData;

/*! Raise an event for the game; may end the conversation it is raised in.
 * \ingroup Zombie
 */
class T_DLLCLASS EventAction : public IAction
{
	T_RTTI_CLASS;

public:
	EventAction() = default;

	explicit EventAction(const resource::Id< EventData >& event, bool endConversation);

	const resource::Id< EventData >& getEvent() const { return m_event; }

	bool getEndConversation() const { return m_endConversation; }

	virtual void execute(Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< EventData > m_event;
	bool m_endConversation = false;
};

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/SmallSet.h"
#include "Core/Guid.h"
#include "Core/Object.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"

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

class DialogData;
class DialogNode;
class DialogResponse;
class IAction;
class Story;

/*! One talk with one actor: the line said, the responses open and what choosing one does.
 * \ingroup Zombie
 */
class T_DLLCLASS Conversation : public Object
{
	T_RTTI_CLASS;

public:
	explicit Conversation(Story* story, const Guid& actor, const DialogData* dialog, const std::wstring& name);

	/*! Begin at the first start entry whose condition holds; false if none does, which ends it. */
	bool start();

	/*! Actor talked to. */
	const Guid& getActor() const { return m_actor; }

	/*! Speaker's name. */
	const std::wstring& getName() const { return m_name; }

	/*! What the actor is saying now. */
	std::wstring getLine() const;

	/*! Guid of the current line, null if none. */
	Guid getNodeId() const;

	/*! Responses open now, in order; those whose condition fails, and once responses already given, left out. */
	RefArray< DialogResponse > getResponses() const;

	/*! Give response; runs its actions and moves on to its target, or ends. */
	bool choose(const DialogResponse* response);

	bool hasEnded() const { return m_ended; }

	/*! Check if an event (EventAction), by the guid of its EventData, was raised in this conversation. */
	bool hasEvent(const Guid& event) const;

private:
	friend class Story;

	Ref< Story > m_story;
	Guid m_actor;
	Ref< const DialogData > m_dialog;
	std::wstring m_name;
	int32_t m_node = -1;
	bool m_ended = false;
	SmallSet< Guid > m_events;

	const DialogNode* getNode() const;

	void enter(int32_t node);

	void runActions(const RefArray< IAction >& actions);

	void noteEvent(const Guid& event, bool endConversation);
};

}

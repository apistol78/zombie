/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Conversation.h"

#include "Core/Log/Log.h"
#include "DialogData.h"
#include "DialogNode.h"
#include "DialogResponse.h"
#include "QuestLog.h"
#include "Story.h"

#include <algorithm>

namespace zombie
{
T_IMPLEMENT_RTTI_CLASS(L"zombie.Conversation", Conversation, Object)

Conversation::Conversation(Story* story, const Guid& actor, const DialogData* dialog, const std::wstring& name)
:	m_story(story)
,	m_actor(actor)
,	m_dialog(dialog)
,	m_name(name)
{
}

bool Conversation::start()
{
	if (m_dialog)
	{
		for (const auto& start : m_dialog->getStart())
		{
			if (start.condition == nullptr || start.condition->evaluate(m_story))
			{
				enter(start.node);
				return !m_ended;
			}
		}
	}
	m_ended = true;
	return false;
}

std::wstring Conversation::getLine() const
{
	const DialogNode* node = getNode();
	return node != nullptr ? node->getText() : L"";
}

Guid Conversation::getNodeId() const
{
	const DialogNode* node = getNode();
	return node != nullptr ? node->getId() : Guid();
}

RefArray< DialogResponse > Conversation::getResponses() const
{
	RefArray< DialogResponse > responses;

	const DialogNode* node = getNode();
	if (node == nullptr || m_ended)
		return responses;

	for (auto response : node->getResponses())
	{
		if (response->getCondition() != nullptr && !response->getCondition()->evaluate(m_story))
			continue;
		if (response->isOnce() && m_story->getQuestLog()->isSaid(m_actor, response->getId()))
			continue;
		responses.push_back(response);
	}
	return responses;
}

bool Conversation::choose(const DialogResponse* response)
{
	const DialogNode* node = getNode();
	if (node == nullptr || m_ended || response == nullptr)
		return false;

	const auto& responses = node->getResponses();
	if (std::find(responses.begin(), responses.end(), response) == responses.end())
		return false;

	// Keep the response, and the node it is in, while its actions run.
	Ref< const DialogResponse > keep = response;

	if (response->isOnce())
		m_story->getQuestLog()->setSaid(m_actor, response->getId());

	runActions(response->getActions());

	if (m_ended)
		return true;

	if (response->getTarget() < 0)
	{
		m_ended = true;
		return true;
	}

	enter(response->getTarget());
	return true;
}

bool Conversation::hasEvent(const Guid& event) const
{
	return m_events.find(event) != m_events.end();
}

const DialogNode* Conversation::getNode() const
{
	if (!m_dialog || m_node < 0 || m_node >= (int32_t)m_dialog->getNodes().size())
		return nullptr;
	return m_dialog->getNodes()[m_node];
}

void Conversation::enter(int32_t node)
{
	m_node = node;

	const DialogNode* dialogNode = getNode();
	if (dialogNode == nullptr)
	{
		log::warning << L"Story: conversation with \"" << m_name << L"\" led to no node (" << node << L")." << Endl;
		m_ended = true;
		return;
	}

	runActions(dialogNode->getOnEnter());

	if (m_actor.isNotNull())
		m_story->getQuestLog()->talkedTo(m_actor, dialogNode->getId());
}

void Conversation::runActions(const RefArray< IAction >& actions)
{
	// Events raised by the actions are recorded into this conversation.
	Conversation* previous = m_story->m_conversation;
	m_story->m_conversation = this;
	m_story->runActions(actions);
	m_story->m_conversation = previous;
}

void Conversation::noteEvent(const Guid& event, bool endConversation)
{
	m_events.insert(event);
	if (endConversation)
		m_ended = true;
}

}

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
#include "QuestStep.h"
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

class ActorData;

/*! Talk to an actor; done when a conversation with the actor reaches a node.
 * \ingroup Zombie
 */
class T_DLLCLASS TalkStep : public QuestStep
{
	T_RTTI_CLASS;

public:
	const resource::Id< ActorData >& getActor() const { return m_actor; }

	void setActor(const resource::Id< ActorData >& actor) { m_actor = actor; }

	/*! Node the conversation must reach; empty for any. */
	/*! Line of the actor's dialog the conversation is to reach, by its guid; null for any talk. */
	const Guid& getLine() const { return m_line; }

	void setLine(const Guid& line) { m_line = line; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ActorData > m_actor;
	Guid m_line;
};

}

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
#include "ICondition.h"
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

class QuestData;

/*! State of a quest.
 * \ingroup Zombie
 */
class T_DLLCLASS QuestCondition : public ICondition
{
	T_RTTI_CLASS;

public:
	enum class State
	{
		NotStarted,
		Started,	//!< Given, under way or done.
		Active,		//!< Under way.
		Done,
		NotDone,
		AtStep		//!< Under way, at step.
	};

	QuestCondition() = default;

	explicit QuestCondition(const resource::Id< QuestData >& quest, State state, const Guid& step = Guid());

	const resource::Id< QuestData >& getQuest() const { return m_quest; }

	State getState() const { return m_state; }

	/*! Step the quest is to be at (AtStep); a step of the quest, by its guid. */
	const Guid& getStep() const { return m_step; }

	virtual bool evaluate(const Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< QuestData > m_quest;
	State m_state = State::Active;
	Guid m_step;
};

}

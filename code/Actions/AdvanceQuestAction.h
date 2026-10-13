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

class QuestData;

/*! Complete a quest's current step; only if it is the given step, unless empty.
 * \ingroup Zombie
 */
class T_DLLCLASS AdvanceQuestAction : public IAction
{
	T_RTTI_CLASS;

public:
	AdvanceQuestAction() = default;

	explicit AdvanceQuestAction(const resource::Id< QuestData >& quest, const Guid& step);

	const resource::Id< QuestData >& getQuest() const { return m_quest; }

	/*! Step to complete, by its guid; null completes whichever step the quest is at. */
	const Guid& getStep() const { return m_step; }

	virtual void execute(Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< QuestData > m_quest;
	Guid m_step;
};

}

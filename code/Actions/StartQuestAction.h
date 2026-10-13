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

class QuestData;

/*! Give the player a quest; a quest is only ever given once.
 * \ingroup Zombie
 */
class T_DLLCLASS StartQuestAction : public IAction
{
	T_RTTI_CLASS;

public:
	StartQuestAction() = default;

	explicit StartQuestAction(const resource::Id< QuestData >& quest);

	const resource::Id< QuestData >& getQuest() const { return m_quest; }

	virtual void execute(Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< QuestData > m_quest;
};

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Guid.h"
#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "IAction.h"
#include "QuestStep.h"

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

/*! Quest; linear steps done in order, then the reward.
 * \ingroup Zombie
 */
class T_DLLCLASS QuestData : public ISerializable
{
	T_RTTI_CLASS;

public:
	const std::wstring& getTitle() const { return m_title; }

	void setTitle(const std::wstring& title) { m_title = title; }

	const std::wstring& getInfo() const { return m_info; }

	void setInfo(const std::wstring& info) { m_info = info; }

	const RefArray< QuestStep >& getSteps() const { return m_steps; }

	RefArray< QuestStep >& getSteps() { return m_steps; }

	/*! Index of the step with guid id; -1 if none. */
	int32_t findStep(const Guid& id) const;

	/*! Actors the steps from step on need alive, such as one to talk to; the quest fails if any dies. */
	AlignedVector< Guid > getNeededActors(int32_t step) const;

	const RefArray< IAction >& getReward() const { return m_reward; }

	RefArray< IAction >& getReward() { return m_reward; }

	virtual void serialize(ISerializer& s) override final;

private:
	std::wstring m_title;
	std::wstring m_info;
	RefArray< QuestStep > m_steps;
	RefArray< IAction > m_reward;
};

}

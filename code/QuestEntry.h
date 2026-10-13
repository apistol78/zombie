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
#include "Core/Object.h"
#include "Core/Ref.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif


namespace traktor
{

class PropertyGroup;

}

namespace zombie
{

using namespace traktor;

class QuestData;
class Story;

/*! A quest the player has been given, as the journal lists it.
 * \ingroup Zombie
 */
class T_DLLCLASS QuestEntry : public Object
{
	T_RTTI_CLASS;

public:
	explicit QuestEntry(const Guid& quest, const QuestData* data, int32_t step, bool done, bool failed, bool active);

	const Guid& getQuest() const { return m_quest; }

	const QuestData* getData() const { return m_data; }

	/*! Index of current step; of the last step when done. */
	int32_t getStep() const { return m_step; }

	bool isDone() const { return m_done; }

	/*! Failed; step is the step it failed at. */
	bool isFailed() const { return m_failed; }

	bool isActive() const { return m_active; }

private:
	Guid m_quest;
	Ref< const QuestData > m_data;
	int32_t m_step;
	bool m_done;
	bool m_failed;
	bool m_active;
};

}

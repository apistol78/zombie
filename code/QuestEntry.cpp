/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "QuestEntry.h"

#include "QuestData.h"

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.QuestEntry", QuestEntry, Object)

QuestEntry::QuestEntry(const Guid& quest, const QuestData* data, int32_t step, bool done, bool failed, bool active)
:	m_quest(quest)
,	m_data(data)
,	m_step(step)
,	m_done(done)
,	m_failed(failed)
,	m_active(active)
{
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

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

class ScriptConditionData;

/*! A condition the game answers, asked with the condition's guid.
 * \ingroup Zombie
 */
class T_DLLCLASS ScriptCondition : public ICondition
{
	T_RTTI_CLASS;

public:
	const resource::Id< ScriptConditionData >& getCondition() const { return m_condition; }

	virtual bool evaluate(const Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ScriptConditionData > m_condition;
};

}

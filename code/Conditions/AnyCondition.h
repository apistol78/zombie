/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/RefArray.h"
#include "ICondition.h"

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

/*! Any child holds.
 * \ingroup Zombie
 */
class T_DLLCLASS AnyCondition : public ICondition
{
	T_RTTI_CLASS;

public:
	const RefArray< ICondition >& getConditions() const { return m_conditions; }

	RefArray< ICondition >& getConditions() { return m_conditions; }

	virtual bool evaluate(const Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	RefArray< ICondition > m_conditions;
};

}

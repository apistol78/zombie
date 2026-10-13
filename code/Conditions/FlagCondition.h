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

class FlagData;

/*! Story flag is set; negated, not set.
 * \ingroup Zombie
 */
class T_DLLCLASS FlagCondition : public ICondition
{
	T_RTTI_CLASS;

public:
	FlagCondition() = default;

	explicit FlagCondition(const resource::Id< FlagData >& flag, bool negate);

	const resource::Id< FlagData >& getFlag() const { return m_flag; }

	bool getNegate() const { return m_negate; }

	virtual bool evaluate(const Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< FlagData > m_flag;
	bool m_negate = false;
};

}

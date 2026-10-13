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

class FlagData;

/*! Set or clear a story flag.
 * \ingroup Zombie
 */
class T_DLLCLASS SetFlagAction : public IAction
{
	T_RTTI_CLASS;

public:
	SetFlagAction() = default;

	explicit SetFlagAction(const resource::Id< FlagData >& flag, bool value);

	const resource::Id< FlagData >& getFlag() const { return m_flag; }

	bool getValue() const { return m_value; }

	virtual void execute(Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< FlagData > m_flag;
	bool m_value = true;
};

}

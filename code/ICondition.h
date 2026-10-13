/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Serialization/ISerializable.h"

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

class Story;

/*! Condition, such as on a dialog response.
 * \ingroup Zombie
 */
class T_DLLCLASS ICondition : public ISerializable
{
	T_RTTI_CLASS;

public:
	/*! Check if condition holds in story. */
	virtual bool evaluate(const Story* story) const = 0;
};

}

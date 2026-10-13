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

/*! Action, such as run when a dialog response is chosen or a quest step completed.
 * \ingroup Zombie
 */
class T_DLLCLASS IAction : public ISerializable
{
	T_RTTI_CLASS;

public:
	/*! Perform action on story. */
	virtual void execute(Story* story) const = 0;
};

}

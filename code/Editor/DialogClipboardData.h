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
#include "Core/Serialization/ISerializable.h"
#include "Editor/DialogNodeAsset.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace zombie
{

using namespace traktor;

/*! Lines copied from a dialog, on the clipboard.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogClipboardData : public ISerializable
{
	T_RTTI_CLASS;

public:
	RefArray< DialogNodeAsset > nodes;

	virtual void serialize(ISerializer& s) override final;
};

}

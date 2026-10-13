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
#include "Core/Ref.h"
#include "Core/Serialization/ISerializable.h"
#include "ICondition.h"

#include <string>

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

/*! Where a conversation may start.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogStartAsset : public ISerializable
{
	T_RTTI_CLASS;

public:
	Guid node;					//!< Line the conversation starts at; set by the dialog editor.
	Ref< ICondition > condition;	//!< Null always holds.

	std::wstring legacyNode;	//!< Line by name, read from a dialog written before lines had guids.

	virtual void serialize(ISerializer& s) override final;
};

}

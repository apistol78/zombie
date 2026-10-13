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
#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "IAction.h"
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

/*! Response in a dialog asset.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogResponseAsset : public ISerializable
{
	T_RTTI_CLASS;

public:
	Guid id;					//!< What a once-response is remembered by; made with the response.
	std::wstring text;
	Guid target;				//!< Line the response leads to; null ends the conversation. Set by the dialog editor.
	Ref< ICondition > condition;
	RefArray< IAction > actions;
	bool once = false;

	std::wstring legacyTarget;	//!< Target by name, read from a dialog written before lines had guids.

	DialogResponseAsset();

	virtual void serialize(ISerializer& s) override final;
};

}

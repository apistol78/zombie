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
#include "Core/Math/Vector2.h"
#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "Editor/DialogResponseAsset.h"
#include "IAction.h"

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

/*! Line in a dialog asset.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogNodeAsset : public ISerializable
{
	T_RTTI_CLASS;

public:
	Guid id;					//!< What responses, starts and talk steps lead to; made with the line.
	std::wstring name;			//!< What the editor shows the line by; defined only here, may be empty.
	std::wstring text;			//!< What the actor says.
	RefArray< IAction > onEnter;
	RefArray< DialogResponseAsset > responses;
	Vector2 position = Vector2::zero();	//!< Position in the dialog editor.

	DialogNodeAsset();

	virtual void serialize(ISerializer& s) override final;
};

}

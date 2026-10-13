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
#include "Editor/DialogNodeAsset.h"
#include "Editor/DialogStartAsset.h"

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

/*! Dialog asset; lines linked by guid, compiled into a DialogData.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogAsset : public ISerializable
{
	T_RTTI_CLASS;

public:
	std::wstring name;			//!< Speaker's name; empty for the actor's.
	RefArray< DialogStartAsset > start;	//!< First whose condition holds; empty starts at the first line.
	RefArray< DialogNodeAsset > nodes;
	Vector2 startPosition = Vector2::zero();	//!< Position of the start in the dialog editor.

	/*! Line by guid, null if none. */
	DialogNodeAsset* findNode(const Guid& id) const;

	/*! Line by name, null if none. */
	DialogNodeAsset* findNodeByName(const std::wstring& name) const;

	virtual void serialize(ISerializer& s) override final;
};

}

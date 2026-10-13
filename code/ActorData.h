/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Ref.h"
#include "Core/Serialization/ISerializable.h"
#include "Resource/Id.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class PropertyGroup;

}

namespace zombie
{

using namespace traktor;

class DialogData;

/*! Actor; a character's identity: its name, dialog and properties for the game.
 * \ingroup Zombie
 */
class T_DLLCLASS ActorData : public ISerializable
{
	T_RTTI_CLASS;

public:
	const std::wstring& getName() const { return m_name; }

	void setName(const std::wstring& name) { m_name = name; }

	/*! Dialog; null if the actor cannot be talked to. */
	const resource::Id< DialogData >& getDialog() const { return m_dialog; }

	void setDialog(const resource::Id< DialogData >& dialog) { m_dialog = dialog; }

	/*! Properties for the game, null if none. */
	const PropertyGroup* getProperties() const { return m_properties; }

	void setProperties(const PropertyGroup* properties) { m_properties = properties; }

	virtual void serialize(ISerializer& s) override final;

private:
	std::wstring m_name;
	resource::Id< DialogData > m_dialog;
	Ref< const PropertyGroup > m_properties;
};

}

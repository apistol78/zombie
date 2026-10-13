/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Guid.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "DialogNode.h"
#include "IAction.h"
#include "ICondition.h"

#include <string>

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

/*! Dialog, compiled from a dialog asset; lines are referred to by index.
 * \ingroup Zombie
 */
class T_DLLCLASS DialogData : public ISerializable
{
	T_RTTI_CLASS;

public:
	struct Start
	{
		int32_t node = -1;
		Ref< ICondition > condition;

		void serialize(ISerializer& s);
	};

	/*! Name shown for the speaker; empty for the actor's name. */
	const std::wstring& getName() const { return m_name; }

	void setName(const std::wstring& name) { m_name = name; }

	/*! Where a conversation starts: the first entry whose condition holds. */
	const AlignedVector< Start >& getStart() const { return m_start; }

	AlignedVector< Start >& getStart() { return m_start; }

	const RefArray< DialogNode >& getNodes() const { return m_nodes; }

	RefArray< DialogNode >& getNodes() { return m_nodes; }

	/*! Index of the line with guid id, -1 if none. */
	int32_t findNode(const Guid& id) const;

	virtual void serialize(ISerializer& s) override final;

private:
	std::wstring m_name;
	AlignedVector< Start > m_start;
	RefArray< DialogNode > m_nodes;
};

}

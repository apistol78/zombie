/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/SmallMap.h"
#include "Core/Guid.h"
#include "Core/Object.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_EDITOR_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::db
{

class Database;

}

namespace traktor::scene
{

class SceneEditorContext;

}

namespace zombie
{

using namespace traktor;

class IWaypoint;

/*! The quest picked in a scene editor, and the actors, places and items its steps are about.
 * \ingroup Zombie
 */
class T_DLLCLASS StoryGuides : public Object
{
	T_RTTI_CLASS;

public:
	struct Target
	{
		std::wstring label;		//!< Step, such as "2. find".
		float radius = 0.0f;	//!< Reach radius around a place; 0 if none.
	};

	/*! Guides of a scene editor, made on first use. */
	static StoryGuides* get(const scene::SceneEditorContext* context);

	const Guid& getQuest() const { return m_quest; }

	void setQuest(db::Database* database, const Guid& quest);

	/*! What the picked quest makes of id, null if nothing. */
	const Target* find(const Guid& id) const;

private:
	Guid m_quest;
	SmallMap< Guid, Target > m_targets;

	void add(const Guid& id, const std::wstring& label, float radius);

	void addWaypoint(const IWaypoint* waypoint, const std::wstring& label);
};

}

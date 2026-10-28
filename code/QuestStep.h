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
#include "IWaypoint.h"

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

/*! Quest step; what completes it depends on its kind.
 * \ingroup Zombie
 */
class T_DLLCLASS QuestStep : public ISerializable
{
	T_RTTI_CLASS;

public:
	QuestStep();

	/*! What conditions, actions and saved games refer to the step by; made with the step. */
	const Guid& getId() const { return m_id; }

	void setId(const Guid& id) { m_id = id; }

	/*! Name the editor shows the step by; defined only here, and not needed to be unique. */
	const std::wstring& getName() const { return m_name; }

	void setName(const std::wstring& name) { m_name = name; }

	/*! Objective, as the journal and HUD show it. */
	const std::wstring& getText() const { return m_text; }

	void setText(const std::wstring& text) { m_text = text; }

	/*! Where the step points the player; null to derive from the step. */
	const IWaypoint* getWaypoint() const { return m_waypoint; }

	void setWaypoint(IWaypoint* waypoint) { m_waypoint = waypoint; }

	/*! The quest fails while at this step and this holds; null never. Deaths of actors still to talk to fail it regardless. */
	const ICondition* getFailWhen() const { return m_failWhen; }

	void setFailWhen(ICondition* failWhen) { m_failWhen = failWhen; }

	/*! Actions run when the step is completed. */
	const RefArray< IAction >& getOnComplete() const { return m_onComplete; }

	RefArray< IAction >& getOnComplete() { return m_onComplete; }

	virtual void serialize(ISerializer& s) override;

private:
	Guid m_id;
	std::wstring m_name;
	std::wstring m_text;
	Ref< IWaypoint > m_waypoint;
	Ref< ICondition > m_failWhen;
	RefArray< IAction > m_onComplete;
};

}

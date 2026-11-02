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
#include "Resource/Id.h"

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

class DialogData;

/*! Actor; a character's identity: its name, dialog and stats for the game.
 * \ingroup Zombie
 */
class T_DLLCLASS ActorData : public ISerializable
{
	T_RTTI_CLASS;

public:
	enum class Class
	{
		Wanderer,
		Worker,
		Merchant
	};

	enum class Faction
	{
		Villager,
		Bandit,
		Undead
	};

	enum class Mood
	{
		Neutral,
		Happy,
		Angry,
		Scared
	};

	const std::wstring& getName() const { return m_name; }

	void setName(const std::wstring& name) { m_name = name; }

	/*! Dialog; null if the actor cannot be talked to. */
	const resource::Id< DialogData >& getDialog() const { return m_dialog; }

	void setDialog(const resource::Id< DialogData >& dialog) { m_dialog = dialog; }

	Class getClass() const { return m_class; }

	int32_t getAge() const { return m_age; }

	/*! Courage, 0 flees at once and 1 never. */
	float getCourage() const { return m_courage; }

	Faction getFaction() const { return m_faction; }

	float getHealth() const { return m_health; }

	Mood getMood() const { return m_mood; }

	virtual void serialize(ISerializer& s) override final;

private:
	std::wstring m_name;
	resource::Id< DialogData > m_dialog;
	Class m_class = Class::Wanderer;
	int32_t m_age = 35;
	float m_courage = 0.5f;
	Faction m_faction = Faction::Villager;
	float m_health = 250.0f;
	Mood m_mood = Mood::Neutral;
};

}

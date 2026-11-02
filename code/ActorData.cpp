/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "ActorData.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberEnum.h"
#include "Core/Serialization/MemberRef.h"
#include "Core/Settings/PropertyGroup.h"
#include "DialogData.h"
#include "Resource/Member.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.ActorData", 2, ActorData, ISerializable)

void ActorData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
	s >> resource::Member< DialogData >(L"dialog", m_dialog);

	if (s.getVersion< ActorData >() == 1)
	{
		Ref< const PropertyGroup > properties;
		s >> MemberRef< const PropertyGroup >(L"properties", properties);
	}

	if (s.getVersion< ActorData >() >= 2)
	{
		const MemberEnum< Class >::Key c_Class_Keys[] =
		{
			{ L"Wanderer", Class::Wanderer },
			{ L"Worker", Class::Worker },
			{ L"Merchant", Class::Merchant },
			{ 0 }
		};

		const MemberEnum< Faction >::Key c_Faction_Keys[] =
		{
			{ L"Villager", Faction::Villager },
			{ L"Bandit", Faction::Bandit },
			{ L"Undead", Faction::Undead },
			{ 0 }
		};

		const MemberEnum< Mood >::Key c_Mood_Keys[] =
		{
			{ L"Neutral", Mood::Neutral },
			{ L"Happy", Mood::Happy },
			{ L"Angry", Mood::Angry },
			{ L"Scared", Mood::Scared },
			{ 0 }
		};

		s >> MemberEnum< Class >(L"class", m_class, c_Class_Keys);
		s >> Member< int32_t >(L"age", m_age, AttributeRange(0));
		s >> Member< float >(L"courage", m_courage, AttributeRange(0.0f, 1.0f));
		s >> MemberEnum< Faction >(L"faction", m_faction, c_Faction_Keys);
		s >> Member< float >(L"health", m_health, AttributeRange(0.0f));
		s >> MemberEnum< Mood >(L"mood", m_mood, c_Mood_Keys);
	}
}

}

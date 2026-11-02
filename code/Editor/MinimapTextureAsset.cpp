/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/MinimapTextureAsset.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/AttributeType.h"
#include "Core/Serialization/AttributeUnit.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberAabb.h"
#include "Scene/Editor/SceneAsset.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.MinimapTextureAsset", 2, MinimapTextureAsset, ISerializable)

void MinimapTextureAsset::serialize(ISerializer& s)
{
	s >> Member< Guid >(L"source", m_source, AttributeType(type_of< scene::SceneAsset >()));
	s >> MemberAabb2(L"bounds", m_bounds);
	s >> Member< uint32_t >(L"size", m_size, AttributeRange(16));

	if (s.getVersion< MinimapTextureAsset >() >= 2)
	{
		s >> Member< Color4f >(L"terrainLowColor", m_terrainLowColor);
		s >> Member< Color4f >(L"terrainHighColor", m_terrainHighColor);
	}
	else
	{
		s >> Member< Color4f >(L"terrainColor", m_terrainLowColor);
		m_terrainHighColor = m_terrainLowColor;
	}

	s >> Member< Color4f >(L"contourColor", m_contourColor);
	s >> Member< float >(L"contourInterval", m_contourInterval, AttributeRange(0.01f) | AttributeUnit(UnitType::Metres));
	s >> Member< Color4f >(L"oceanColor", m_oceanColor);
	s >> Member< Color4f >(L"coastColor", m_coastColor);

	if (s.getVersion< MinimapTextureAsset >() >= 1)
	{
		s >> Member< Color4f >(L"modelColor", m_modelColor);
		s >> Member< float >(L"modelColorBlend", m_modelColorBlend, AttributeRange(0.0f, 1.0f) | AttributeUnit(UnitType::Percent));
	}
}

}

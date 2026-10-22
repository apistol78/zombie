/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Interaction/InteractableComponentData.h"

#include "Core/Serialization/AttributeRange.h"
#include "Core/Serialization/AttributeUnit.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Interaction/InteractableComponent.h"

namespace zombie
{

T_IMPLEMENT_RTTI_EDIT_CLASS(L"zombie.InteractableComponentData", 0, InteractableComponentData, world::IEntityComponentData)

Ref< InteractableComponent > InteractableComponentData::createComponent() const
{
	return new InteractableComponent(this);
}

int32_t InteractableComponentData::getOrdinal() const
{
	return 0;
}

void InteractableComponentData::setTransform(const world::EntityData* owner, const Transform& transform)
{
}

void InteractableComponentData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"prompt", m_prompt);
	s >> Member< std::wstring >(L"refusal", m_refusal);
	s >> Member< std::wstring >(L"notice", m_notice);
	s >> Member< float >(L"range", m_range, AttributeRange(0.0f) | AttributeUnit(UnitType::Metres));
	s >> Member< int32_t >(L"priority", m_priority);
}

}

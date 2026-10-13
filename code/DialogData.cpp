/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "DialogData.h"

#include "Core/Serialization/AttributeMultiLine.h"
#include "Core/Serialization/ISerializer.h"
#include "Core/Serialization/Member.h"
#include "Core/Serialization/MemberAlignedVector.h"
#include "Core/Serialization/MemberComposite.h"
#include "Core/Serialization/MemberRef.h"
#include "Core/Serialization/MemberRefArray.h"
#include "DialogNode.h"

namespace zombie
{

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.DialogData", 1, DialogData, ISerializable)

void DialogData::serialize(ISerializer& s)
{
	s >> Member< std::wstring >(L"name", m_name);
	s >> MemberAlignedVector< Start, MemberComposite< Start > >(L"start", m_start);
	s >> MemberRefArray< DialogNode >(L"nodes", m_nodes);
}

int32_t DialogData::findNode(const Guid& id) const
{
	for (int32_t i = 0; i < (int32_t)m_nodes.size(); ++i)
	{
		if (m_nodes[i]->getId() == id)
			return i;
	}
	return -1;
}

void DialogData::Start::serialize(ISerializer& s)
{
	s >> Member< int32_t >(L"node", node);
	s >> MemberRef< ICondition >(L"condition", condition);
}

}

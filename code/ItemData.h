/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/RefArray.h"
#include "Core/Serialization/ISerializable.h"
#include "IItemTrait.h"
#include "Resource/Id.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::world
{

class EntityData;

}

namespace zombie
{

using namespace traktor;

/*! Item; what an item is, wherever one lies. Named by the guid of its instance.
 * \ingroup Zombie
 */
class T_DLLCLASS ItemData : public ISerializable
{
	T_RTTI_CLASS;

public:
	const std::wstring& getName() const { return m_name; }

	/*! Name of a stack of more than one; the name if empty. */
	const std::wstring& getPluralName() const { return m_pluralName.empty() ? m_name : m_pluralName; }

	const std::wstring& getInfo() const { return m_info; }

	/*! Icon identifier in the game's art. */
	const std::wstring& getIcon() const { return m_icon; }

	/*! How many share one inventory slot. */
	int32_t getMaxStack() const { return m_maxStack; }

	/*! The item is its entity; the entity rides along in the inventory rather than being destroyed. */
	bool isKeep() const { return m_keep; }

	/*! Entity a dropped one is built from; null if the item cannot be put down. */
	const resource::Id< world::EntityData >& getDropEntity() const { return m_dropEntity; }

	const RefArray< IItemTrait >& getTraits() const { return m_traits; }

	/*! First trait of given type, or null. */
	const IItemTrait* getTrait(const TypeInfo& traitType) const;

	template < typename TraitType >
	const TraitType* getTrait() const
	{
		return static_cast< const TraitType* >(getTrait(type_of< TraitType >()));
	}

	void setName(const std::wstring& name) { m_name = name; }

	void setMaxStack(int32_t maxStack) { m_maxStack = maxStack; }

	void setKeep(bool keep) { m_keep = keep; }

	void addTrait(IItemTrait* trait) { m_traits.push_back(trait); }

	virtual void serialize(ISerializer& s) override final;

private:
	std::wstring m_name;
	std::wstring m_pluralName;
	std::wstring m_info;
	std::wstring m_icon;
	int32_t m_maxStack = 1;
	bool m_keep = false;
	resource::Id< world::EntityData > m_dropEntity;
	RefArray< IItemTrait > m_traits;
};

}

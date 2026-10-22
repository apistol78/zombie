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
#include "World/IWorldComponentData.h"

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

class InteractionWorldComponent;

/*! Interaction world component data.
 * \ingroup Zombie
 */
class T_DLLCLASS InteractionWorldComponentData : public world::IWorldComponentData
{
	T_RTTI_CLASS;

public:
	Ref< InteractionWorldComponent > createComponent() const;

	virtual void serialize(ISerializer& s) override final;
};

}

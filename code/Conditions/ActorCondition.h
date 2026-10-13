/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "ICondition.h"
#include "Resource/Id.h"

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

class ActorData;

/*! An actor is alive or dead; alive until told it has died.
 * \ingroup Zombie
 */
class T_DLLCLASS ActorCondition : public ICondition
{
	T_RTTI_CLASS;

public:
	enum class State
	{
		Alive,
		Dead
	};

	ActorCondition() = default;

	explicit ActorCondition(const resource::Id< ActorData >& actor, State state);

	const resource::Id< ActorData >& getActor() const { return m_actor; }

	State getState() const { return m_state; }

	virtual bool evaluate(const Story* story) const override final;

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< ActorData > m_actor;
	State m_state = State::Alive;
};

}

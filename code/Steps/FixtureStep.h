/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "QuestStep.h"
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

class FixtureKindData;

/*! Be done with every fixture of a kind, such as lighting every ward; done in a world that has
 *  fixtures of the kind once all of them are.
 * \ingroup Zombie
 */
class T_DLLCLASS FixtureStep : public QuestStep
{
	T_RTTI_CLASS;

public:
	const resource::Id< FixtureKindData >& getKind() const { return m_kind; }

	void setKind(const resource::Id< FixtureKindData >& kind) { m_kind = kind; }

	virtual void serialize(ISerializer& s) override final;

private:
	resource::Id< FixtureKindData > m_kind;
};

}

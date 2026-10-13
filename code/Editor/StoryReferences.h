/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/SmallSet.h"
#include "Core/Guid.h"
#include "Core/Rtti/TypeInfo.h"

#include <functional>

namespace traktor
{

class ISerializable;

}

namespace zombie
{

using namespace traktor;

/*! Check if a reference of type is to a part of an instance (a quest step, a dialog line) rather than an instance.
 * \ingroup Zombie
 */
bool isPartReference(const TypeInfo& type);

/*! Collect every resource reference (resource::Id) in object and its child objects.
 * \ingroup Zombie
 *
 * References to parts of instances are left out; they are no dependencies.
 */
void collectReferences(const ISerializable* object, SmallSet< Guid >& outReferences);

/*! Call visitor with object and every child object in it, each once.
 * \ingroup Zombie
 */
void visitObjects(const ISerializable* object, const std::function< void(const ISerializable*) >& visitor);

}

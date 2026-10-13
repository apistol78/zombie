/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include <string>

namespace traktor
{

class ISerializable;

}

namespace traktor::editor
{

class IPipelineCommon;

}

namespace zombie
{

using namespace traktor;

class StoryData;

/*! Check every reference in a story asset, and what each kind of object needs; false if anything is wrong.
 * \ingroup Zombie
 */
bool validateStoryAsset(editor::IPipelineCommon* pipelineCommon, const ISerializable* asset, const std::wstring& name);

/*! Check that a pickup, container, action or the start kit gives every item the story's quests find.
 * \ingroup Zombie
 */
bool validateObtainable(editor::IPipelineCommon* pipelineCommon, const StoryData* storyData, const std::wstring& name);

}

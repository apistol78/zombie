/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Core/Rtti/TypeInfo.h"

#if defined(T_STATIC)
#	include "Editor/DialogAsset.h"
#	include "Editor/DialogClipboardData.h"
#	include "Editor/DialogEditorPageFactory.h"
#	include "Editor/DialogNodeAsset.h"
#	include "Editor/DialogPipeline.h"
#	include "Editor/DialogResponseAsset.h"
#	include "Editor/DialogStartAsset.h"
#	include "Editor/ItemPipeline.h"
#	include "Editor/QuestClipboardData.h"
#	include "Editor/QuestEditorPageFactory.h"
#	include "Editor/StoryComponentPipeline.h"
#	include "Editor/StoryPipeline.h"
#	include "Editor/StoryReferenceResolver.h"
#	include "Editor/StorySceneEditorPlugin.h"

namespace zombie
{

extern "C" void __module__Zombie_Editor()
{
	T_FORCE_LINK_REF(DialogAsset);
	T_FORCE_LINK_REF(DialogClipboardData);
	T_FORCE_LINK_REF(DialogEditorPageFactory);
	T_FORCE_LINK_REF(DialogNodeAsset);
	T_FORCE_LINK_REF(DialogPipeline);
	T_FORCE_LINK_REF(DialogResponseAsset);
	T_FORCE_LINK_REF(DialogStartAsset);
	T_FORCE_LINK_REF(ItemPipeline);
	T_FORCE_LINK_REF(QuestClipboardData);
	T_FORCE_LINK_REF(QuestEditorPageFactory);
	T_FORCE_LINK_REF(StoryComponentPipeline);
	T_FORCE_LINK_REF(StoryPipeline);
	T_FORCE_LINK_REF(StoryReferenceResolver);
	T_FORCE_LINK_REF(StorySceneEditorPlugin);
}

}

#endif

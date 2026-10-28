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
#	include "Actions/AdvanceQuestAction.h"
#	include "Actions/EventAction.h"
#	include "Actions/FinishQuestAction.h"
#	include "Actions/GiveItemAction.h"
#	include "Actions/SetFlagAction.h"
#	include "Actions/StartQuestAction.h"
#	include "Actions/TakeItemAction.h"
#	include "ActorComponentData.h"
#	include "ActorData.h"
#	include "Conditions/ActorCondition.h"
#	include "Conditions/AllCondition.h"
#	include "Conditions/AnyCondition.h"
#	include "Conditions/FlagCondition.h"
#	include "Conditions/HasItemCondition.h"
#	include "Conditions/QuestCondition.h"
#	include "Conditions/ScriptCondition.h"
#	include "DialogData.h"
#	include "DialogNode.h"
#	include "DialogResponse.h"
#	include "Fixtures/FixtureClassFactory.h"
#	include "Fixtures/FixtureComponentData.h"
#	include "Fixtures/FixtureEntityFactory.h"
#	include "Fixtures/FixtureWorldComponentData.h"
#	include "Fixtures/LandmarkComponentData.h"
#	include "Fixtures/StageExitComponentData.h"
#	include "Interaction/InteractableComponentData.h"
#	include "Interaction/InteractionClassFactory.h"
#	include "Interaction/InteractionEntityFactory.h"
#	include "Interaction/InteractionWorldComponentData.h"
#	include "Interaction/LeverComponentData.h"
#	include "Interaction/LockComponentData.h"
#	include "ItemContainerComponentData.h"
#	include "ItemData.h"
#	include "ItemPickupComponentData.h"
#	include "Npc/NpcClassFactory.h"
#	include "Npc/NpcComponentData.h"
#	include "Npc/NpcEntityFactory.h"
#	include "Npc/NpcWorldComponentData.h"
#	include "Npc/RouteComponentData.h"
#	include "Keys/EquipSlotData.h"
#	include "Keys/EventData.h"
#	include "Keys/FixtureKindData.h"
#	include "Keys/FlagData.h"
#	include "Keys/ScriptConditionData.h"
#	include "PlaceComponentData.h"
#	include "PlaceData.h"
#	include "QuestData.h"
#	include "SettingsResourceFactory.h"
#	include "Steps/FindItemStep.h"
#	include "Steps/FixtureStep.h"
#	include "Steps/ReachStep.h"
#	include "Steps/TalkStep.h"
#	include "Steps/TriggerStep.h"
#	include "StoryClassFactory.h"
#	include "StoryData.h"
#	include "StoryEntityFactory.h"
#	include "StoryResourceFactory.h"
#	include "StoryWorldComponentData.h"
#	include "Traits/ArmorTrait.h"
#	include "Traits/EquipTrait.h"
#	include "Traits/EventTrait.h"
#	include "Traits/HealTrait.h"
#	include "Traits/WeaponTrait.h"
#	include "Waypoints/ActorWaypoint.h"
#	include "Waypoints/FirstOfWaypoint.h"
#	include "Waypoints/ItemWaypoint.h"
#	include "Waypoints/PlaceWaypoint.h"
#	include "Waypoints/PointWaypoint.h"

namespace zombie
{

extern "C" void __module__Zombie()
{
	T_FORCE_LINK_REF(ActorComponentData);
	T_FORCE_LINK_REF(ActorCondition);
	T_FORCE_LINK_REF(ActorData);
	T_FORCE_LINK_REF(ActorWaypoint);
	T_FORCE_LINK_REF(AdvanceQuestAction);
	T_FORCE_LINK_REF(AllCondition);
	T_FORCE_LINK_REF(AnyCondition);
	T_FORCE_LINK_REF(ArmorTrait);
	T_FORCE_LINK_REF(DialogData);
	T_FORCE_LINK_REF(DialogNode);
	T_FORCE_LINK_REF(DialogResponse);
	T_FORCE_LINK_REF(EquipSlotData);
	T_FORCE_LINK_REF(EquipTrait);
	T_FORCE_LINK_REF(EventAction);
	T_FORCE_LINK_REF(EventData);
	T_FORCE_LINK_REF(EventTrait);
	T_FORCE_LINK_REF(FindItemStep);
	T_FORCE_LINK_REF(FinishQuestAction);
	T_FORCE_LINK_REF(FirstOfWaypoint);
	T_FORCE_LINK_REF(FixtureClassFactory);
	T_FORCE_LINK_REF(FixtureComponentData);
	T_FORCE_LINK_REF(FixtureEntityFactory);
	T_FORCE_LINK_REF(FixtureKindData);
	T_FORCE_LINK_REF(FixtureStep);
	T_FORCE_LINK_REF(FixtureWorldComponentData);
	T_FORCE_LINK_REF(FlagCondition);
	T_FORCE_LINK_REF(FlagData);
	T_FORCE_LINK_REF(GiveItemAction);
	T_FORCE_LINK_REF(HasItemCondition);
	T_FORCE_LINK_REF(HealTrait);
	T_FORCE_LINK_REF(InteractableComponentData);
	T_FORCE_LINK_REF(InteractionClassFactory);
	T_FORCE_LINK_REF(InteractionEntityFactory);
	T_FORCE_LINK_REF(InteractionWorldComponentData);
	T_FORCE_LINK_REF(ItemContainerComponentData);
	T_FORCE_LINK_REF(ItemData);
	T_FORCE_LINK_REF(ItemPickupComponentData);
	T_FORCE_LINK_REF(ItemWaypoint);
	T_FORCE_LINK_REF(LandmarkComponentData);
	T_FORCE_LINK_REF(LeverComponentData);
	T_FORCE_LINK_REF(LockComponentData);
	T_FORCE_LINK_REF(NpcClassFactory);
	T_FORCE_LINK_REF(NpcComponentData);
	T_FORCE_LINK_REF(NpcEntityFactory);
	T_FORCE_LINK_REF(NpcWorldComponentData);
	T_FORCE_LINK_REF(PlaceComponentData);
	T_FORCE_LINK_REF(PlaceData);
	T_FORCE_LINK_REF(PlaceWaypoint);
	T_FORCE_LINK_REF(PointWaypoint);
	T_FORCE_LINK_REF(QuestCondition);
	T_FORCE_LINK_REF(QuestData);
	T_FORCE_LINK_REF(ReachStep);
	T_FORCE_LINK_REF(RouteComponentData);
	T_FORCE_LINK_REF(ScriptCondition);
	T_FORCE_LINK_REF(ScriptConditionData);
	T_FORCE_LINK_REF(SetFlagAction);
	T_FORCE_LINK_REF(SettingsResourceFactory);
	T_FORCE_LINK_REF(StageExitComponentData);
	T_FORCE_LINK_REF(StartQuestAction);
	T_FORCE_LINK_REF(StoryClassFactory);
	T_FORCE_LINK_REF(StoryData);
	T_FORCE_LINK_REF(StoryEntityFactory);
	T_FORCE_LINK_REF(StoryResourceFactory);
	T_FORCE_LINK_REF(StoryWorldComponentData);
	T_FORCE_LINK_REF(TakeItemAction);
	T_FORCE_LINK_REF(TalkStep);
	T_FORCE_LINK_REF(TriggerStep);
	T_FORCE_LINK_REF(WeaponTrait);
}

}

#endif

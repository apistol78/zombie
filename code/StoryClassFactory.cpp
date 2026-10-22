/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "StoryClassFactory.h"

#include "ActorComponent.h"
#include "ActorData.h"
#include "Conversation.h"
#include "Core/Class/AutoRuntimeClass.h"
#include "Core/Class/Boxes/BoxedGuid.h"
#include "Core/Class/Boxes/BoxedRefArray.h"
#include "Core/Class/Boxes/BoxedTypeInfo.h"
#include "Core/Class/Boxes/BoxedVector4.h"
#include "Core/Class/IRuntimeClassRegistrar.h"
#include "Core/Class/IRuntimeDelegate.h"
#include "Core/Settings/PropertyGroup.h"
#include "DialogData.h"
#include "DialogResponse.h"
#include "Inventory.h"
#include "InventorySlot.h"
#include "ItemContainerComponent.h"
#include "ItemData.h"
#include "ItemPickupComponent.h"
#include "PlaceComponent.h"
#include "PlaceData.h"
#include "QuestData.h"
#include "QuestEntry.h"
#include "QuestLog.h"
#include "Resource/IResourceManager.h"
#include "Steps/FindItemStep.h"
#include "Steps/ReachStep.h"
#include "Steps/TalkStep.h"
#include "Steps/TriggerStep.h"
#include "Story.h"
#include "StoryData.h"
#include "StoryResourceFactory.h"
#include "StoryWorldComponent.h"
#include "Traits/ArmorTrait.h"
#include "Traits/EquipTrait.h"
#include "Traits/EventTrait.h"
#include "Traits/HealTrait.h"
#include "Traits/WeaponTrait.h"
#include "Waypoints/ActorWaypoint.h"
#include "Waypoints/FirstOfWaypoint.h"
#include "Waypoints/ItemWaypoint.h"
#include "Waypoints/PlaceWaypoint.h"
#include "Waypoints/PointWaypoint.h"
#include "World/Entity.h"

namespace zombie
{
	namespace
	{

/*! Null guids are nil in script. */
Any guidOrNil(const Guid& guid)
{
	return (guid.isValid() && guid.isNotNull()) ? CastAny< Guid >::set(guid) : Any();
}

/*! Script names an item either by Guid or by the guid as text. */
Guid itemOf(const Any& item)
{
	if (CastAny< Guid >::accept(item))
		return CastAny< Guid >::get(item);
	else if (item.isString())
		return Guid(item.getWideString());
	else
		return Guid();
}

/*! Script names a quest, an actor, a place, a step, a line, a flag, an event or a slot the same way. */
Guid idOf(const Any& id)
{
	return itemOf(id);
}

Any ItemData_getDropEntity(ItemData* self)
{
	return guidOrNil(self->getDropEntity());
}

const IItemTrait* ItemData_getTrait(ItemData* self, const TypeInfo& traitType)
{
	return self->getTrait(traitType);
}

const ItemData* Inventory_getItem(Inventory* self, const Any& item)
{
	return self->getItem(itemOf(item));
}

int32_t Inventory_count(Inventory* self, const Any& item)
{
	return self->count(itemOf(item));
}

int32_t Inventory_room(Inventory* self, const Any& item)
{
	return self->room(itemOf(item));
}

int32_t Inventory_add_2(Inventory* self, const Any& item, int32_t count)
{
	return self->add(itemOf(item), count);
}

int32_t Inventory_add_3(Inventory* self, const Any& item, int32_t count, world::Entity* kept)
{
	return self->add(itemOf(item), count, kept);
}

int32_t Inventory_remove(Inventory* self, const Any& item, int32_t count)
{
	return self->remove(itemOf(item), count);
}

RefArray< world::Entity > Inventory_removeKept(Inventory* self, const Any& item, int32_t count)
{
	return self->removeKept(itemOf(item), count);
}

world::Entity* Inventory_first(Inventory* self, const Any& item)
{
	return self->first(itemOf(item));
}

void Inventory_attachKept(Inventory* self, const Any& item, const RefArray< world::Entity >& entities)
{
	self->attachKept(itemOf(item), entities);
}

Any Inventory_getEquipped(Inventory* self, const Any& slot)
{
	return guidOrNil(self->getEquipped(idOf(slot)));
}

bool Inventory_unequip(Inventory* self, const Any& slot)
{
	return self->unequip(idOf(slot));
}

Any EquipTrait_getSlot(EquipTrait* self) { return guidOrNil(self->getSlot()); }

Any EventTrait_getEvent(EventTrait* self) { return guidOrNil(self->getEvent()); }

Any WeaponTrait_getAmmo(WeaponTrait* self) { return guidOrNil(self->getAmmo()); }

int32_t StoryData_getItemCount(StoryData* self) { return (int32_t)self->getItems().size(); }

Any StoryData_getItem(StoryData* self, int32_t index)
{
	const auto& items = self->getItems();
	return (index >= 0 && index < (int32_t)items.size()) ? guidOrNil(items[index]) : Any();
}

Any QuestStep_getId(QuestStep* self) { return guidOrNil(self->getId()); }

Any TalkStep_getLine(TalkStep* self) { return guidOrNil(self->getLine()); }

int32_t QuestData_findStep(QuestData* self, const Any& step) { return self->findStep(idOf(step)); }

bool Inventory_isEquipped(Inventory* self, const Any& item)
{
	return self->isEquipped(itemOf(item));
}

bool Inventory_equip(Inventory* self, const Any& item)
{
	return self->equip(itemOf(item));
}

Any ItemContainerComponent_getItem(ItemContainerComponent* self, int32_t index)
{
	return guidOrNil(self->getItem(index));
}

int32_t ItemContainerComponent_count(ItemContainerComponent* self, const Any& item)
{
	return self->count(itemOf(item));
}

void ItemContainerComponent_add(ItemContainerComponent* self, const Any& item, int32_t count)
{
	self->add(itemOf(item), count);
}

Any ActorData_getDialog(ActorData* self)
{
	return guidOrNil(self->getDialog());
}

std::wstring QuestStep_getKind(QuestStep* self)
{
	if (is_a< FindItemStep >(self))
		return L"find";
	else if (is_a< TalkStep >(self))
		return L"talk";
	else if (is_a< ReachStep >(self))
		return L"reach";
	else
		return L"trigger";
}

Any FindItemStep_getItem(FindItemStep* self) { return guidOrNil(self->getItem()); }

Any TalkStep_getActor(TalkStep* self) { return guidOrNil(self->getActor()); }

Any ReachStep_getPlace(ReachStep* self) { return guidOrNil(self->getPlace()); }

Any ActorWaypoint_getActor(ActorWaypoint* self) { return guidOrNil(self->getActor()); }

Any PlaceWaypoint_getPlace(PlaceWaypoint* self) { return guidOrNil(self->getPlace()); }

Any ItemWaypoint_getItem(ItemWaypoint* self) { return guidOrNil(self->getItem()); }

bool QuestLog_start(QuestLog* self, const Any& quest) { return self->start(idOf(quest)); }

bool QuestLog_advance_1(QuestLog* self, const Any& quest) { return self->advance(idOf(quest), Guid()); }

bool QuestLog_advance_2(QuestLog* self, const Any& quest, const Any& step) { return self->advance(idOf(quest), idOf(step)); }

bool QuestLog_finish(QuestLog* self, const Any& quest) { return self->finish(idOf(quest)); }

bool QuestLog_isStarted(QuestLog* self, const Any& quest) { return self->isStarted(idOf(quest)); }

bool QuestLog_isActive(QuestLog* self, const Any& quest) { return self->isActive(idOf(quest)); }

bool QuestLog_isDone(QuestLog* self, const Any& quest) { return self->isDone(idOf(quest)); }

int32_t QuestLog_getStepIndex(QuestLog* self, const Any& quest) { return self->getStepIndex(idOf(quest)); }

const QuestStep* QuestLog_getCurrentStep(QuestLog* self, const Any& quest) { return self->getCurrentStep(idOf(quest)); }

Any QuestLog_getCurrentStepId(QuestLog* self, const Any& quest) { return guidOrNil(self->getCurrentStepId(idOf(quest))); }

bool QuestLog_getFlag(QuestLog* self, const Any& flag) { return self->getFlag(idOf(flag)); }

void QuestLog_setFlag(QuestLog* self, const Any& flag, bool value) { self->setFlag(idOf(flag), value); }

Any Conversation_getNodeId(Conversation* self) { return guidOrNil(self->getNodeId()); }

bool Conversation_hasEvent(Conversation* self, const Any& event) { return self->hasEvent(idOf(event)); }

Any QuestLog_getActive(QuestLog* self) { return guidOrNil(self->getActive()); }

bool QuestLog_setActive(QuestLog* self, const Any& quest) { return self->setActive(idOf(quest)); }

void QuestLog_talkedTo(QuestLog* self, const Any& actor, const Any& line) { self->talkedTo(idOf(actor), idOf(line)); }

void QuestLog_found(QuestLog* self, const Any& item) { self->found(itemOf(item)); }

void QuestLog_died(QuestLog* self, const Any& actor) { self->died(idOf(actor)); }

bool QuestLog_hasFound(QuestLog* self, const Any& item) { return self->hasFound(itemOf(item)); }

Ref< Story > Story_create(resource::IResourceManager* resourceManager, const Any& story, int32_t capacity) { return Story::create(resourceManager, idOf(story), capacity); }

int32_t Story_getQuestCount(Story* self) { return (int32_t)self->getQuests().size(); }

Any Story_getQuestId(Story* self, int32_t index)
{
	const auto quests = self->getQuests();
	return (index >= 0 && index < (int32_t)quests.size()) ? guidOrNil(quests[index]) : Any();
}

const QuestData* Story_getQuest(Story* self, const Any& quest) { return self->getQuest(idOf(quest)); }

const ActorData* Story_getActor(Story* self, const Any& actor) { return self->getActor(idOf(actor)); }

const PlaceData* Story_getPlace(Story* self, const Any& place) { return self->getPlace(idOf(place)); }

Ref< Conversation > Story_talk(Story* self, const Any& actor) { return self->talk(idOf(actor)); }

bool Story_isHeldBack(Story* self, const Any& item) { return self->isHeldBack(itemOf(item)); }

int32_t Story_giveItem(Story* self, const Any& item, int32_t count) { return self->giveItem(itemOf(item), count); }

int32_t Story_takeItem(Story* self, const Any& item, int32_t count) { return self->takeItem(itemOf(item), count); }

Any Conversation_getActor(Conversation* self) { return guidOrNil(self->getActor()); }

Any StoryWorldComponent_getWaypoint(StoryWorldComponent* self)
{
	Vector4 position;
	return self->getWaypoint(position) ? CastAny< Vector4 >::set(position) : Any();
}

world::Entity* StoryWorldComponent_findActor(StoryWorldComponent* self, const Any& actor) { return self->findActor(idOf(actor)); }

world::Entity* StoryWorldComponent_findPlace(StoryWorldComponent* self, const Any& place) { return self->findPlace(idOf(place)); }

world::Entity* StoryWorldComponent_findHolder(StoryWorldComponent* self, const Any& item) { return self->findHolder(itemOf(item)); }

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.StoryClassFactory", 0, StoryClassFactory, IRuntimeClassFactory)

void StoryClassFactory::createClasses(IRuntimeClassRegistrar* registrar) const
{
	auto classIItemTrait = new AutoRuntimeClass< IItemTrait >();
	registrar->registerClass(classIItemTrait);

	auto classEquipTrait = new AutoRuntimeClass< EquipTrait >();
	classEquipTrait->addProperty("slot", &EquipTrait_getSlot);
	registrar->registerClass(classEquipTrait);

	auto classHealTrait = new AutoRuntimeClass< HealTrait >();
	classHealTrait->addProperty("amount", &HealTrait::getAmount);
	classHealTrait->addProperty("verb", &HealTrait::getVerb);
	registrar->registerClass(classHealTrait);

	auto classArmorTrait = new AutoRuntimeClass< ArmorTrait >();
	classArmorTrait->addProperty("fraction", &ArmorTrait::getFraction);
	registrar->registerClass(classArmorTrait);

	auto classWeaponTrait = new AutoRuntimeClass< WeaponTrait >();
	classWeaponTrait->addProperty("ammo", &WeaponTrait_getAmmo);
	registrar->registerClass(classWeaponTrait);

	auto classEventTrait = new AutoRuntimeClass< EventTrait >();
	classEventTrait->addProperty("event", &EventTrait_getEvent);
	registrar->registerClass(classEventTrait);

	auto classItemData = new AutoRuntimeClass< ItemData >();
	classItemData->addProperty("name", &ItemData::getName);
	classItemData->addProperty("pluralName", &ItemData::getPluralName);
	classItemData->addProperty("info", &ItemData::getInfo);
	classItemData->addProperty("icon", &ItemData::getIcon);
	classItemData->addProperty("maxStack", &ItemData::getMaxStack);
	classItemData->addProperty("quest", &ItemData::isQuest);
	classItemData->addProperty("keep", &ItemData::isKeep);
	classItemData->addProperty("dropEntity", &ItemData_getDropEntity);
	classItemData->addMethod("getTrait", &ItemData_getTrait);
	registrar->registerClass(classItemData);

	auto classStoryData = new AutoRuntimeClass< StoryData >();
	classStoryData->addProperty("itemCount", &StoryData_getItemCount);
	classStoryData->addMethod("getItem", &StoryData_getItem);
	registrar->registerClass(classStoryData);

	auto classStoryResourceFactory = new AutoRuntimeClass< StoryResourceFactory >();
	classStoryResourceFactory->addConstructor();
	registrar->registerClass(classStoryResourceFactory);

	auto classActorData = new AutoRuntimeClass< ActorData >();
	classActorData->addProperty("name", &ActorData::getName);
	classActorData->addProperty("dialog", &ActorData_getDialog);
	registrar->registerClass(classActorData);

	auto classPlaceData = new AutoRuntimeClass< PlaceData >();
	classPlaceData->addProperty("name", &PlaceData::getName);
	registrar->registerClass(classPlaceData);

	auto classIWaypoint = new AutoRuntimeClass< IWaypoint >();
	registrar->registerClass(classIWaypoint);

	auto classActorWaypoint = new AutoRuntimeClass< ActorWaypoint >();
	classActorWaypoint->addProperty("actor", &ActorWaypoint_getActor);
	registrar->registerClass(classActorWaypoint);

	auto classPlaceWaypoint = new AutoRuntimeClass< PlaceWaypoint >();
	classPlaceWaypoint->addProperty("place", &PlaceWaypoint_getPlace);
	registrar->registerClass(classPlaceWaypoint);

	auto classItemWaypoint = new AutoRuntimeClass< ItemWaypoint >();
	classItemWaypoint->addProperty("item", &ItemWaypoint_getItem);
	registrar->registerClass(classItemWaypoint);

	auto classPointWaypoint = new AutoRuntimeClass< PointWaypoint >();
	classPointWaypoint->addProperty("position", &PointWaypoint::getPosition);
	registrar->registerClass(classPointWaypoint);

	auto classFirstOfWaypoint = new AutoRuntimeClass< FirstOfWaypoint >();
	classFirstOfWaypoint->addProperty< const RefArray< IWaypoint >& >("candidates", &FirstOfWaypoint::getCandidates);
	registrar->registerClass(classFirstOfWaypoint);

	auto classQuestStep = new AutoRuntimeClass< QuestStep >();
	classQuestStep->addProperty("id", &QuestStep_getId);
	classQuestStep->addProperty("name", &QuestStep::getName);
	classQuestStep->addProperty("text", &QuestStep::getText);
	classQuestStep->addProperty("waypoint", &QuestStep::getWaypoint);
	classQuestStep->addProperty("kind", &QuestStep_getKind);
	registrar->registerClass(classQuestStep);

	auto classFindItemStep = new AutoRuntimeClass< FindItemStep >();
	classFindItemStep->addProperty("item", &FindItemStep_getItem);
	registrar->registerClass(classFindItemStep);

	auto classTalkStep = new AutoRuntimeClass< TalkStep >();
	classTalkStep->addProperty("actor", &TalkStep_getActor);
	classTalkStep->addProperty("line", &TalkStep_getLine);
	registrar->registerClass(classTalkStep);

	auto classReachStep = new AutoRuntimeClass< ReachStep >();
	classReachStep->addProperty("place", &ReachStep_getPlace);
	classReachStep->addProperty("radius", &ReachStep::getRadius);
	registrar->registerClass(classReachStep);

	auto classTriggerStep = new AutoRuntimeClass< TriggerStep >();
	registrar->registerClass(classTriggerStep);

	auto classQuestData = new AutoRuntimeClass< QuestData >();
	classQuestData->addProperty("title", &QuestData::getTitle);
	classQuestData->addProperty("info", &QuestData::getInfo);
	classQuestData->addProperty< const RefArray< QuestStep >& >("steps", &QuestData::getSteps);
	classQuestData->addMethod("findStep", &QuestData_findStep);
	registrar->registerClass(classQuestData);

	auto classDialogResponse = new AutoRuntimeClass< DialogResponse >();
	classDialogResponse->addProperty("text", &DialogResponse::getText);
	classDialogResponse->addProperty("once", &DialogResponse::isOnce);
	registrar->registerClass(classDialogResponse);

	auto classDialogData = new AutoRuntimeClass< DialogData >();
	classDialogData->addProperty("name", &DialogData::getName);
	registrar->registerClass(classDialogData);

	auto classQuestEntry = new AutoRuntimeClass< QuestEntry >();
	classQuestEntry->addProperty("quest", &QuestEntry::getQuest);
	classQuestEntry->addProperty("data", &QuestEntry::getData);
	classQuestEntry->addProperty("step", &QuestEntry::getStep);
	classQuestEntry->addProperty("failed", &QuestEntry::isFailed);
	classQuestEntry->addProperty("done", &QuestEntry::isDone);
	classQuestEntry->addProperty("active", &QuestEntry::isActive);
	registrar->registerClass(classQuestEntry);

	auto classQuestLog = new AutoRuntimeClass< QuestLog >();
	classQuestLog->addProperty("active", &QuestLog_getActive);
	classQuestLog->addProperty("activeStep", &QuestLog::getActiveStep);
	classQuestLog->addProperty("entries", &QuestLog::getEntries);
	classQuestLog->addProperty("serial", &QuestLog::getSerial);
	classQuestLog->addMethod("start", &QuestLog_start);
	classQuestLog->addMethod("advance", &QuestLog_advance_1);
	classQuestLog->addMethod("advance", &QuestLog_advance_2);
	classQuestLog->addMethod("finish", &QuestLog_finish);
	classQuestLog->addMethod("isStarted", &QuestLog_isStarted);
	classQuestLog->addMethod("isActive", &QuestLog_isActive);
	classQuestLog->addMethod("isDone", &QuestLog_isDone);
	classQuestLog->addMethod("getStepIndex", &QuestLog_getStepIndex);
	classQuestLog->addMethod("getCurrentStep", &QuestLog_getCurrentStep);
	classQuestLog->addMethod("getCurrentStepId", &QuestLog_getCurrentStepId);
	classQuestLog->addMethod("setActive", &QuestLog_setActive);
	classQuestLog->addMethod("talkedTo", &QuestLog_talkedTo);
	classQuestLog->addMethod("found", &QuestLog_found);
	classQuestLog->addMethod("hasFound", &QuestLog_hasFound);
	classQuestLog->addMethod("died", &QuestLog_died);
	classQuestLog->addMethod("getFlag", &QuestLog_getFlag);
	classQuestLog->addMethod("setFlag", &QuestLog_setFlag);
	classQuestLog->addMethod("update", &QuestLog::update);
	registrar->registerClass(classQuestLog);

	auto classConversation = new AutoRuntimeClass< Conversation >();
	classConversation->addProperty("actor", &Conversation_getActor);
	classConversation->addProperty("name", &Conversation::getName);
	classConversation->addProperty("line", &Conversation::getLine);
	classConversation->addProperty("nodeId", &Conversation_getNodeId);
	classConversation->addProperty("responses", &Conversation::getResponses);
	classConversation->addProperty("ended", &Conversation::hasEnded);
	classConversation->addMethod("choose", &Conversation::choose);
	classConversation->addMethod("hasEvent", &Conversation_hasEvent);
	registrar->registerClass(classConversation);

	auto classStory = new AutoRuntimeClass< Story >();
	classStory->addStaticMethod("create", &Story_create);
	classStory->addProperty("inventory", &Story::getInventory);
	classStory->addProperty("questLog", &Story::getQuestLog);
	classStory->addProperty("questCount", &Story_getQuestCount);
	classStory->addMethod("getQuestId", &Story_getQuestId);
	classStory->addMethod("getQuest", &Story_getQuest);
	classStory->addMethod("getActor", &Story_getActor);
	classStory->addMethod("getPlace", &Story_getPlace);
	classStory->addMethod("talk", &Story_talk);
	classStory->addMethod("isHeldBack", &Story_isHeldBack);
	classStory->addMethod("update", &Story::update);
	classStory->addMethod("giveItem", &Story_giveItem);
	classStory->addMethod("takeItem", &Story_takeItem);
	classStory->addMethod("setEventHandler", &Story::setEventHandler);
	classStory->addMethod("setConditionHandler", &Story::setConditionHandler);
	classStory->addMethod("setDistanceHandler", &Story::setDistanceHandler);
	classStory->addMethod("saveState", &Story::saveState);
	classStory->addMethod("restoreState", &Story::restoreState);
	registrar->registerClass(classStory);

	auto classActorComponent = new AutoRuntimeClass< ActorComponent >();
	classActorComponent->addProperty("owner", &ActorComponent::getOwner);
	classActorComponent->addProperty("actor", &ActorComponent::getActor);
	classActorComponent->addProperty("name", &ActorComponent::getName);
	classActorComponent->addProperty("properties", &ActorComponent::getProperties);
	registrar->registerClass(classActorComponent);

	auto classStoryWorldComponent = new AutoRuntimeClass< StoryWorldComponent >();
	classStoryWorldComponent->addProperty("story", &StoryWorldComponent::setStory, &StoryWorldComponent::getStory);
	classStoryWorldComponent->addProperty("player", &StoryWorldComponent::setPlayer, &StoryWorldComponent::getPlayer);
	classStoryWorldComponent->addProperty("waypoint", &StoryWorldComponent_getWaypoint);
	classStoryWorldComponent->addProperty("talkTarget", &StoryWorldComponent::getTalkTarget);
	classStoryWorldComponent->addMethod("refresh", &StoryWorldComponent::refresh);
	classStoryWorldComponent->addMethod("findActor", &StoryWorldComponent_findActor);
	classStoryWorldComponent->addMethod("findPlace", &StoryWorldComponent_findPlace);
	classStoryWorldComponent->addMethod("findHolder", &StoryWorldComponent_findHolder);
	registrar->registerClass(classStoryWorldComponent);

	auto classPlaceComponent = new AutoRuntimeClass< PlaceComponent >();
	classPlaceComponent->addProperty("owner", &PlaceComponent::getOwner);
	classPlaceComponent->addProperty("place", &PlaceComponent::getPlace);
	registrar->registerClass(classPlaceComponent);

	auto classInventorySlot = new AutoRuntimeClass< InventorySlot >();
	classInventorySlot->addProperty("item", &InventorySlot::getItem);
	classInventorySlot->addProperty("itemData", &InventorySlot::getItemData);
	classInventorySlot->addProperty("count", &InventorySlot::getCount);
	classInventorySlot->addProperty("kept", &InventorySlot::getKept);
	registrar->registerClass(classInventorySlot);

	auto classInventory = new AutoRuntimeClass< Inventory >();
	classInventory->addConstructor< resource::IResourceManager*, int32_t >();
	classInventory->addProperty("capacity", &Inventory::getCapacity);
	classInventory->addProperty("used", &Inventory::getUsed);
	classInventory->addProperty("serial", &Inventory::getSerial);
	classInventory->addProperty("kept", &Inventory::getKept);
	classInventory->addMethod("getItem", &Inventory_getItem);
	classInventory->addMethod("getSlot", &Inventory::getSlot);
	classInventory->addMethod("count", &Inventory_count);
	classInventory->addMethod("room", &Inventory_room);
	classInventory->addMethod("add", &Inventory_add_2);
	classInventory->addMethod("add", &Inventory_add_3);
	classInventory->addMethod("remove", &Inventory_remove);
	classInventory->addMethod("removeKept", &Inventory_removeKept);
	classInventory->addMethod("removeSlot", &Inventory::removeSlot);
	classInventory->addMethod("first", &Inventory_first);
	classInventory->addMethod("holds", &Inventory::holds);
	classInventory->addMethod("attachKept", &Inventory_attachKept);
	classInventory->addMethod("detachKept", &Inventory::detachKept);
	classInventory->addMethod("keptMissing", &Inventory::keptMissing);
	classInventory->addMethod("getEquipped", &Inventory_getEquipped);
	classInventory->addMethod("isEquipped", &Inventory_isEquipped);
	classInventory->addMethod("equip", &Inventory_equip);
	classInventory->addMethod("unequip", &Inventory_unequip);
	classInventory->addMethod("saveState", &Inventory::saveState);
	classInventory->addMethod("restoreState", &Inventory::restoreState);
	classInventory->addMethod("listen", &Inventory::listen);
	classInventory->addMethod("removeAllListeners", &Inventory::removeAllListeners);
	registrar->registerClass(classInventory);

	auto classItemPickupComponent = new AutoRuntimeClass< ItemPickupComponent >();
	classItemPickupComponent->addProperty("owner", &ItemPickupComponent::getOwner);
	classItemPickupComponent->addProperty("item", &ItemPickupComponent::getItem);
	classItemPickupComponent->addProperty("count", &ItemPickupComponent::setCount, &ItemPickupComponent::getCount);
	registrar->registerClass(classItemPickupComponent);

	auto classItemContainerComponent = new AutoRuntimeClass< ItemContainerComponent >();
	classItemContainerComponent->addProperty("owner", &ItemContainerComponent::getOwner);
	classItemContainerComponent->addProperty("serial", &ItemContainerComponent::getSerial);
	classItemContainerComponent->addProperty("entryCount", &ItemContainerComponent::getEntryCount);
	classItemContainerComponent->addMethod("getItem", &ItemContainerComponent_getItem);
	classItemContainerComponent->addMethod("getCount", &ItemContainerComponent::getCount);
	classItemContainerComponent->addMethod("count", &ItemContainerComponent_count);
	classItemContainerComponent->addMethod("add", &ItemContainerComponent_add);
	classItemContainerComponent->addMethod("remove", &ItemContainerComponent::remove);
	classItemContainerComponent->addMethod("moveTo", &ItemContainerComponent::moveTo);
	classItemContainerComponent->addMethod("clear", &ItemContainerComponent::clear);
	classItemContainerComponent->addMethod("saveState", &ItemContainerComponent::saveState);
	classItemContainerComponent->addMethod("restoreState", &ItemContainerComponent::restoreState);
	registrar->registerClass(classItemContainerComponent);
}

}

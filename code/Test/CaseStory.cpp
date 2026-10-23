/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Test/CaseStory.h"

#include "Actions/EventAction.h"
#include "Actions/GiveItemAction.h"
#include "Actions/StartQuestAction.h"
#include "Actions/TakeItemAction.h"
#include "ActorData.h"
#include "Conditions/ActorCondition.h"
#include "Conditions/AllCondition.h"
#include "Conditions/HasItemCondition.h"
#include "Conditions/QuestCondition.h"
#include "Conversation.h"
#include "Core/Class/Any.h"
#include "Core/Class/IRuntimeDelegate.h"
#include "Core/Containers/SmallMap.h"
#include "Core/Settings/PropertyGroup.h"
#include "DialogData.h"
#include "DialogNode.h"
#include "DialogResponse.h"
#include "Inventory.h"
#include "ItemData.h"
#include "Keys/EventData.h"
#include "PlaceData.h"
#include "QuestData.h"
#include "QuestEntry.h"
#include "QuestLog.h"
#include "Resource/ExplicitResourceHandle.h"
#include "Resource/IResourceManager.h"
#include "Steps/FindItemStep.h"
#include "Steps/ReachStep.h"
#include "Steps/TalkStep.h"
#include "Story.h"
#include "StoryData.h"

namespace zombie::test
{
	namespace
	{

class FakeResourceManager : public resource::IResourceManager
{
public:
	void add(const Guid& id, Object* object) { m_objects[id] = object; }

	virtual void destroy() override final {}

	virtual void addFactory(const resource::IResourceFactory* factory) override final {}

	virtual void removeFactory(const resource::IResourceFactory* factory) override final {}

	virtual void removeAllFactories() override final {}

	virtual bool load(const resource::ResourceBundle* bundle) override final { return false; }

	virtual Ref< resource::ResourceHandle > bind(const TypeInfo& productType, const Guid& guid) override final
	{
		const auto it = m_objects.find(guid);
		if (it == m_objects.end() || !is_type_of(productType, type_of(it->second)))
			return nullptr;
		return new resource::ExplicitResourceHandle(it->second);
	}

	virtual bool reload(const Guid& guid, bool flushedOnly) override final { return false; }

	virtual void reload(const TypeInfo& productType, bool flushedOnly) override final {}

	virtual void unload(const TypeInfo& productType) override final {}

	virtual void unloadUnusedResident() override final {}

	virtual void getStatistics(resource::ResourceManagerStatistics& outStatistics) const override final {}

private:
	SmallMap< Guid, Ref< Object > > m_objects;
};

/*! Distance handler; place is at a fixed distance. */
class Distance : public IRuntimeDelegate
{
public:
	float distance = 100.0f;

	virtual Any call(int32_t argc, const Any* argv) override final
	{
		return Any::fromFloat(distance);
	}
};

/*! Event handler; counts what it hears. */
class Events : public IRuntimeDelegate
{
public:
	int32_t started = 0;
	int32_t updated = 0;
	int32_t completed = 0;
	int32_t failed = 0;
	int32_t given = 0;

	virtual Any call(int32_t argc, const Any* argv) override final
	{
		const std::wstring what = argv[0].getWideString();
		if (what == L"questStarted")
			++started;
		else if (what == L"questUpdated")
			++updated;
		else if (what == L"questCompleted")
			++completed;
		else if (what == L"questFailed")
			++failed;
		else if (what == L"itemGiven")
			++given;
		return Any();
	}
};

const Guid c_story(L"{5A0F0000-0000-4000-8000-000000000001}");
const Guid c_ring(L"{5A0F0000-0000-4000-8000-000000000010}");
const Guid c_coin(L"{5A0F0000-0000-4000-8000-000000000011}");
const Guid c_klara(L"{5A0F0000-0000-4000-8000-000000000020}");
const Guid c_huntsman(L"{5A0F0000-0000-4000-8000-000000000021}");
const Guid c_altar(L"{5A0F0000-0000-4000-8000-000000000030}");
const Guid c_ringQuest(L"{5A0F0000-0000-4000-8000-000000000040}");
const Guid c_prayQuest(L"{5A0F0000-0000-4000-8000-000000000041}");
const Guid c_askQuest(L"{5A0F0000-0000-4000-8000-000000000042}");
const Guid c_tollQuest(L"{5A0F0000-0000-4000-8000-000000000043}");
const Guid c_klaraDialog(L"{5A0F0000-0000-4000-8000-000000000050}");
const Guid c_findStep(L"{5A0F0000-0000-4000-8000-000000000060}");
const Guid c_returnStep(L"{5A0F0000-0000-4000-8000-000000000061}");
const Guid c_helloLine(L"{5A0F0000-0000-4000-8000-000000000070}");
const Guid c_ringLine(L"{5A0F0000-0000-4000-8000-000000000071}");
const Guid c_thanksLine(L"{5A0F0000-0000-4000-8000-000000000072}");
const Guid c_trade(L"{5A0F0000-0000-4000-8000-000000000080}");
const Guid c_met(L"{5A0F0000-0000-4000-8000-000000000081}");

Ref< DialogResponse > response(const std::wstring& text, int32_t target, ICondition* condition = nullptr, IAction* action = nullptr, bool once = false)
{
	Ref< DialogResponse > r = new DialogResponse();
	r->setId(Guid::create());
	r->setText(text);
	r->setTarget(target);
	r->setCondition(condition);
	if (action)
		r->getActions().push_back(action);
	r->setOnce(once);
	return r;
}

Ref< DialogNode > node(const Guid& id, const std::wstring& text)
{
	Ref< DialogNode > n = new DialogNode();
	n->setId(id);
	n->setText(text);
	return n;
}

Ref< FakeResourceManager > createStory()
{
	Ref< FakeResourceManager > rm = new FakeResourceManager();

	Ref< ItemData > ring = new ItemData();
	ring->setName(L"RING");
	rm->add(c_ring, ring);

	Ref< ItemData > coin = new ItemData();
	coin->setName(L"COIN");
	coin->setMaxStack(99);
	rm->add(c_coin, coin);

	Ref< ActorData > klara = new ActorData();
	klara->setName(L"KLARA");
	klara->setDialog(resource::Id< DialogData >(c_klaraDialog));
	rm->add(c_klara, klara);

	Ref< ActorData > huntsman = new ActorData();
	huntsman->setName(L"HUNTSMAN");
	rm->add(c_huntsman, huntsman);

	Ref< PlaceData > altar = new PlaceData();
	altar->setName(L"ALTAR");
	rm->add(c_altar, altar);

	// Ring: find the ring, then talk to Klara at "thanks"; reward 15 coins.
	Ref< QuestData > ringQuest = new QuestData();
	ringQuest->setTitle(L"KLARA'S RING");
	{
		Ref< FindItemStep > find = new FindItemStep();
		find->setId(c_findStep);
		find->setName(L"find");
		find->setItem(resource::Id< ItemData >(c_ring));
		find->setHoldBack(true);
		ringQuest->getSteps().push_back(find);

		Ref< TalkStep > talk = new TalkStep();
		talk->setId(c_returnStep);
		talk->setName(L"return");
		talk->setActor(resource::Id< ActorData >(c_klara));
		talk->setLine(c_thanksLine);
		ringQuest->getSteps().push_back(talk);

		ringQuest->getReward().push_back(new GiveItemAction(resource::Id< ItemData >(c_coin), 15));
	}
	rm->add(c_ringQuest, ringQuest);

	// Pray: reach the altar.
	Ref< QuestData > prayQuest = new QuestData();
	prayQuest->setTitle(L"PRAY");
	{
		Ref< ReachStep > reach = new ReachStep();
		reach->setPlace(resource::Id< PlaceData >(c_altar));
		reach->setRadius(3.0f);
		prayQuest->getSteps().push_back(reach);
	}
	rm->add(c_prayQuest, prayQuest);

	// Ask: talk to Klara, failing if she dies first; then reach the altar.
	Ref< QuestData > askQuest = new QuestData();
	askQuest->setTitle(L"ASK");
	{
		Ref< TalkStep > talk = new TalkStep();
		talk->setActor(resource::Id< ActorData >(c_klara));
		talk->setFailWhen(new ActorCondition(resource::Id< ActorData >(c_klara), ActorCondition::State::Dead));
		askQuest->getSteps().push_back(talk);

		Ref< ReachStep > reach = new ReachStep();
		reach->setPlace(resource::Id< PlaceData >(c_altar));
		askQuest->getSteps().push_back(reach);
	}
	rm->add(c_askQuest, askQuest);

	// Toll: find a coin, holding none back; then reach the altar.
	Ref< QuestData > tollQuest = new QuestData();
	tollQuest->setTitle(L"TOLL");
	{
		Ref< FindItemStep > find = new FindItemStep();
		find->setItem(resource::Id< ItemData >(c_coin));
		tollQuest->getSteps().push_back(find);

		Ref< ReachStep > reach = new ReachStep();
		reach->setPlace(resource::Id< PlaceData >(c_altar));
		tollQuest->getSteps().push_back(reach);
	}
	rm->add(c_tollQuest, tollQuest);

	// Klara: 0 hello, 1 ring, 2 thanks.
	Ref< DialogData > klaraDialog = new DialogData();
	{
		Ref< DialogNode > hello = node(c_helloLine, L"A LIVING FACE.");
		hello->getResponses().push_back(response(L"YOU LOOK TROUBLED.", 1, new QuestCondition(resource::Id< QuestData >(c_ringQuest), QuestCondition::State::NotStarted)));

		Ref< AllCondition > canReturn = new AllCondition();
		canReturn->getConditions().push_back(new QuestCondition(resource::Id< QuestData >(c_ringQuest), QuestCondition::State::AtStep, c_returnStep));
		canReturn->getConditions().push_back(new HasItemCondition(resource::Id< ItemData >(c_ring), 1, false));
		hello->getResponses().push_back(response(L"I FOUND YOUR RING.", 2, canReturn, new TakeItemAction(resource::Id< ItemData >(c_ring), 1)));

		hello->getResponses().push_back(response(L"A COIN FOR YOU.", -1, new HasItemCondition(resource::Id< ItemData >(c_coin), 1, false), new TakeItemAction(resource::Id< ItemData >(c_coin), 1), true));
		hello->getResponses().push_back(response(L"WHAT ARE YOU CARRYING?", -1, nullptr, new EventAction(resource::Id< EventData >(c_trade), true)));
		klaraDialog->getNodes().push_back(hello);

		Ref< DialogNode > ring = node(c_ringLine, L"MY HENRIK'S RING.");
		ring->getResponses().push_back(response(L"I WILL FIND IT.", 0, nullptr, new StartQuestAction(resource::Id< QuestData >(c_ringQuest))));
		klaraDialog->getNodes().push_back(ring);

		klaraDialog->getNodes().push_back(node(c_thanksLine, L"YOU FOUND IT."));

		klaraDialog->getStart().push_back({ 0, nullptr });
	}
	rm->add(c_klaraDialog, klaraDialog);

	Ref< StoryData > storyData = new StoryData();
	storyData->getQuests().push_back(resource::Id< QuestData >(c_ringQuest));
	storyData->getQuests().push_back(resource::Id< QuestData >(c_prayQuest));
	storyData->getQuests().push_back(resource::Id< QuestData >(c_askQuest));
	storyData->getQuests().push_back(resource::Id< QuestData >(c_tollQuest));
	storyData->getStartKit().push_back({ resource::Id< ItemData >(c_coin), 2 });
	rm->add(c_story, storyData);

	return rm;
}

const DialogResponse* findResponse(const Conversation* conversation, const std::wstring& text)
{
	for (auto response : conversation->getResponses())
	{
		if (response->getText() == text)
			return response;
	}
	return nullptr;
}

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.test.CaseStory", 0, CaseStory, traktor::test::Case)

void CaseStory::run()
{
	Ref< FakeResourceManager > rm = createStory();

	// Quest given in a conversation, the item found, returned at the right line, rewarded.
	{
		Ref< Story > story = Story::create(rm, c_story, 8);
		CASE_ASSERT(story != nullptr);
		if (!story)
			return;

		Ref< Events > events = new Events();
		story->setEventHandler(events);

		Inventory* inventory = story->getInventory();
		QuestLog* questLog = story->getQuestLog();
		CASE_ASSERT_EQUAL(inventory->count(c_coin), 2);
		CASE_ASSERT(story->isHeldBack(c_ring));
		CASE_ASSERT(!story->isWanted(c_ring));

		Ref< Conversation > c = story->talk(c_klara);
		CASE_ASSERT(!c->hasEnded());
		CASE_ASSERT(c->getName() == L"KLARA");
		CASE_ASSERT(c->getLine() == L"A LIVING FACE.");
		CASE_ASSERT(findResponse(c, L"I FOUND YOUR RING.") == nullptr);
		CASE_ASSERT(c->choose(findResponse(c, L"YOU LOOK TROUBLED.")));
		CASE_ASSERT(c->getNodeId() == c_ringLine);
		CASE_ASSERT(c->choose(findResponse(c, L"I WILL FIND IT.")));
		CASE_ASSERT(questLog->isActive(c_ringQuest));
		CASE_ASSERT(questLog->getActive() == c_ringQuest);
		CASE_ASSERT(questLog->getCurrentStepId(c_ringQuest) == c_findStep);
		CASE_ASSERT_EQUAL(events->started, 1);
		CASE_ASSERT(!story->isHeldBack(c_ring));
		CASE_ASSERT(story->isWanted(c_ring));
		CASE_ASSERT(findResponse(c, L"YOU LOOK TROUBLED.") == nullptr);

		inventory->add(c_ring, 1);
		story->update();
		CASE_ASSERT(questLog->getCurrentStepId(c_ringQuest) == c_returnStep);
		CASE_ASSERT_EQUAL(events->updated, 1);

		Ref< Conversation > c2 = story->talk(c_klara);
		CASE_ASSERT(questLog->isActive(c_ringQuest));
		CASE_ASSERT(c2->choose(findResponse(c2, L"I FOUND YOUR RING.")));
		CASE_ASSERT(questLog->isDone(c_ringQuest));
		CASE_ASSERT(!story->isWanted(c_ring));
		CASE_ASSERT_EQUAL(inventory->count(c_ring), 0);
		CASE_ASSERT_EQUAL(inventory->count(c_coin), 17);
		CASE_ASSERT_EQUAL(events->completed, 1);
		CASE_ASSERT_EQUAL(events->given, 1);
		CASE_ASSERT(questLog->getActive().isNull());
		CASE_ASSERT(c2->getResponses().empty());

		// Once responses are offered until given; events end the conversation.
		Ref< Conversation > c3 = story->talk(c_klara);
		CASE_ASSERT(c3->choose(findResponse(c3, L"A COIN FOR YOU.")));
		CASE_ASSERT(c3->hasEnded());
		Ref< Conversation > c4 = story->talk(c_klara);
		CASE_ASSERT(findResponse(c4, L"A COIN FOR YOU.") == nullptr);
		CASE_ASSERT(c4->choose(findResponse(c4, L"WHAT ARE YOU CARRYING?")));
		CASE_ASSERT(c4->hasEnded());
		CASE_ASSERT(c4->hasEvent(c_trade));

		// No dialog, no conversation.
		CASE_ASSERT(story->talk(c_huntsman) == nullptr);
		CASE_ASSERT(story->talk(Guid()) == nullptr);
	}

	// An item found before the quest is given completes the step at once.
	{
		Ref< Story > story = Story::create(rm, c_story, 8);
		story->getInventory()->add(c_ring, 1);
		story->update();
		CASE_ASSERT(story->getQuestLog()->start(c_ringQuest));
		CASE_ASSERT(story->getQuestLog()->getCurrentStepId(c_ringQuest) == c_returnStep);
		CASE_ASSERT(!story->getQuestLog()->start(c_ringQuest));
	}

	// A find step holding nothing back: its item is never held back, and one carried from the start completes it.
	{
		Ref< Story > story = Story::create(rm, c_story, 8);
		CASE_ASSERT(!story->isHeldBack(c_coin));
		CASE_ASSERT(!story->isWanted(c_coin));
		story->update();
		CASE_ASSERT(story->getQuestLog()->start(c_tollQuest));
		CASE_ASSERT_EQUAL(story->getQuestLog()->getStepIndex(c_tollQuest), 1);
		CASE_ASSERT(story->isWanted(c_coin));
		CASE_ASSERT(!story->isHeldBack(c_coin));
	}

	// Reach steps are completed by the distance handler.
	{
		Ref< Story > story = Story::create(rm, c_story, 8);
		Ref< Distance > distance = new Distance();
		story->setDistanceHandler(distance);
		story->getQuestLog()->start(c_prayQuest);
		story->update();
		CASE_ASSERT(story->getQuestLog()->isActive(c_prayQuest));
		distance->distance = 2.0f;
		story->update();
		CASE_ASSERT(story->getQuestLog()->isDone(c_prayQuest));
	}

	// Saved and restored: inventory, quests, flags and found items.
	{
		Ref< Story > story = Story::create(rm, c_story, 8);
		QuestLog* questLog = story->getQuestLog();
		questLog->start(c_ringQuest);
		questLog->start(c_prayQuest);
		questLog->setActive(c_prayQuest);
		questLog->setFlag(c_met, true);
		story->getInventory()->add(c_ring, 1);
		story->update();

		Ref< PropertyGroup > state = story->saveState();

		Ref< Story > restored = Story::create(rm, c_story, 8);
		restored->restoreState(state);
		QuestLog* restoredLog = restored->getQuestLog();
		CASE_ASSERT(restoredLog->getCurrentStepId(c_ringQuest) == c_returnStep);
		CASE_ASSERT(restoredLog->isActive(c_prayQuest));
		CASE_ASSERT(restoredLog->getActive() == c_prayQuest);
		CASE_ASSERT(restoredLog->getFlag(c_met));
		CASE_ASSERT(restoredLog->hasFound(c_ring));
		CASE_ASSERT_EQUAL(restored->getInventory()->count(c_ring), 1);
		CASE_ASSERT_EQUAL((int32_t)restoredLog->getEntries().size(), 2);
		CASE_ASSERT(restoredLog->getEntries()[0]->getQuest() == c_prayQuest);
	}

	// A quest fails when the one to talk to dies first, and stays failed; not after.
	{
		Ref< Story > story = Story::create(rm, c_story, 8);
		Ref< Events > events = new Events();
		story->setEventHandler(events);
		QuestLog* questLog = story->getQuestLog();

		const ActorCondition alive(resource::Id< ActorData >(c_klara), ActorCondition::State::Alive);
		CASE_ASSERT(alive.evaluate(story));

		questLog->start(c_askQuest);
		CASE_ASSERT(questLog->isActive(c_askQuest));
		questLog->died(c_klara);
		CASE_ASSERT(!alive.evaluate(story));
		CASE_ASSERT(questLog->getEntries()[0]->isFailed());
		CASE_ASSERT(!questLog->isActive(c_askQuest));
		CASE_ASSERT(!questLog->isDone(c_askQuest));
		CASE_ASSERT(questLog->getActive().isNull());
		CASE_ASSERT_EQUAL(events->failed, 1);
		CASE_ASSERT(!questLog->start(c_askQuest));
		CASE_ASSERT(!questLog->finish(c_askQuest));

		Ref< PropertyGroup > state = story->saveState();
		Ref< Story > restored = Story::create(rm, c_story, 8);
		restored->restoreState(state);
		CASE_ASSERT(restored->getQuestLog()->hasDied(c_klara));
		CASE_ASSERT(restored->getQuestLog()->getEntries()[0]->isFailed());

		// Spoken to first: dying later fails nothing.
		Ref< Story > other = Story::create(rm, c_story, 8);
		QuestLog* otherLog = other->getQuestLog();
		otherLog->start(c_askQuest);
		otherLog->talkedTo(c_klara, Guid());
		CASE_ASSERT_EQUAL(otherLog->getStepIndex(c_askQuest), 1);
		otherLog->died(c_klara);
		CASE_ASSERT(otherLog->isActive(c_askQuest));
	}
}

}

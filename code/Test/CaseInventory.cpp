/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Test/CaseInventory.h"

#include "Core/Class/Any.h"
#include "Core/Class/IRuntimeDelegate.h"
#include "Core/Containers/SmallMap.h"
#include "Core/Settings/PropertyGroup.h"
#include "Inventory.h"
#include "InventorySlot.h"
#include "ItemContainerComponent.h"
#include "ItemData.h"
#include "Keys/EquipSlotData.h"
#include "Resource/ExplicitResourceHandle.h"
#include "Resource/IResourceManager.h"
#include "Traits/EquipTrait.h"
#include "World/Entity.h"

namespace zombie::test
{
	namespace
	{

/*! Resource manager which only knows a fixed set of items. */
class FakeResourceManager : public resource::IResourceManager
{
public:
	void add(const Guid& id, ItemData* item) { m_items[id] = item; }

	virtual void destroy() override final {}

	virtual void addFactory(const resource::IResourceFactory* factory) override final {}

	virtual void removeFactory(const resource::IResourceFactory* factory) override final {}

	virtual void removeAllFactories() override final {}

	virtual bool load(const resource::ResourceBundle* bundle) override final { return false; }

	virtual Ref< resource::ResourceHandle > bind(const TypeInfo& productType, const Guid& guid) override final
	{
		const auto it = m_items.find(guid);
		return it != m_items.end() ? new resource::ExplicitResourceHandle(it->second) : nullptr;
	}

	virtual bool reload(const Guid& guid, bool flushedOnly) override final { return false; }

	virtual void reload(const TypeInfo& productType, bool flushedOnly) override final {}

	virtual void unload(const TypeInfo& productType) override final {}

	virtual void unloadUnusedResident() override final {}

	virtual void getStatistics(resource::ResourceManagerStatistics& outStatistics) const override final {}

private:
	SmallMap< Guid, Ref< ItemData > > m_items;
};

/*! Counts calls; optionally changes the inventory again from within the first call. */
class CountingDelegate : public IRuntimeDelegate
{
public:
	int32_t calls = 0;
	Guid addOnFirst;

	virtual Any call(int32_t argc, const Any* argv) override final
	{
		if (calls++ == 0 && addOnFirst.isNotNull())
		{
			Inventory* inventory = checked_type_cast< Inventory* >(argv[0].getObject());
			inventory->add(addOnFirst, 1);
		}
		return Any();
	}
};

const Guid c_rounds(L"{D1A10000-0000-4000-8000-000000000203}");
const Guid c_rifle(L"{D1A10000-0000-4000-8000-000000000200}");
const Guid c_relic(L"{D1A10000-0000-4000-8000-000000000209}");
const Guid c_unknown(L"{D1A10000-0000-4000-8000-0000000002FF}");
const Guid c_weaponSlot(L"{D1A10000-0000-4000-8000-000000000600}");
const Guid c_handSlot(L"{D1A10000-0000-4000-8000-000000000601}");

Ref< FakeResourceManager > createItems()
{
	Ref< FakeResourceManager > resourceManager = new FakeResourceManager();

	Ref< ItemData > rounds = new ItemData();
	rounds->setName(L"ROUND");
	rounds->setMaxStack(24);
	resourceManager->add(c_rounds, rounds);

	Ref< ItemData > rifle = new ItemData();
	rifle->setName(L"RIFLE");
	rifle->addTrait(new EquipTrait(resource::Id< EquipSlotData >(c_weaponSlot)));
	resourceManager->add(c_rifle, rifle);

	Ref< ItemData > relic = new ItemData();
	relic->setName(L"RELIC");
	relic->setMaxStack(3);
	relic->setKeep(true);
	relic->addTrait(new EquipTrait(resource::Id< EquipSlotData >(c_handSlot)));
	resourceManager->add(c_relic, relic);

	return resourceManager;
}

	}

T_IMPLEMENT_RTTI_FACTORY_CLASS(L"zombie.test.CaseInventory", 0, CaseInventory, traktor::test::Case)

void CaseInventory::run()
{
	Ref< FakeResourceManager > resourceManager = createItems();

	// Stacks top up before opening new slots, and unknown items are refused.
	{
		Ref< Inventory > inventory = new Inventory(resourceManager, 3);
		CASE_ASSERT_EQUAL(inventory->getCapacity(), 3);
		CASE_ASSERT_EQUAL(inventory->add(c_unknown, 1), 0);
		CASE_ASSERT_EQUAL(inventory->room(c_unknown), 0);
		CASE_ASSERT_EQUAL(inventory->add(c_rounds, 30), 30);
		CASE_ASSERT_EQUAL(inventory->getUsed(), 2);
		CASE_ASSERT_EQUAL(inventory->getSlot(0)->getCount(), 24);
		CASE_ASSERT_EQUAL(inventory->getSlot(1)->getCount(), 6);
		CASE_ASSERT_EQUAL(inventory->room(c_rounds), 18 + 24);
		CASE_ASSERT_EQUAL(inventory->add(c_rounds, 100), 42);
		CASE_ASSERT_EQUAL(inventory->count(c_rounds), 72);
		CASE_ASSERT_EQUAL(inventory->room(c_rounds), 0);
		CASE_ASSERT_EQUAL(inventory->add(c_rifle, 1), 0);
	}

	// Removal takes from the last stack first, and an emptied slot stays empty.
	{
		Ref< Inventory > inventory = new Inventory(resourceManager, 3);
		inventory->add(c_rounds, 30);
		inventory->add(c_rifle, 1);
		CASE_ASSERT(inventory->getSlot(2)->getItem() == c_rifle);
		CASE_ASSERT_EQUAL(inventory->remove(c_rounds, 10), 10);
		CASE_ASSERT(inventory->getSlot(1) == nullptr);
		CASE_ASSERT_EQUAL(inventory->getSlot(0)->getCount(), 20);
		CASE_ASSERT(inventory->getSlot(2)->getItem() == c_rifle);
		CASE_ASSERT_EQUAL(inventory->remove(c_rounds, 100), 20);
		CASE_ASSERT_EQUAL(inventory->getUsed(), 1);
	}

	// Equipped is a mark which goes when the last of the item leaves.
	{
		Ref< Inventory > inventory = new Inventory(resourceManager, 3);
		CASE_ASSERT(!inventory->equip(c_rifle));
		inventory->add(c_rifle, 1);
		inventory->add(c_rounds, 1);
		CASE_ASSERT(!inventory->equip(c_rounds));
		CASE_ASSERT(inventory->equip(c_rifle));
		CASE_ASSERT(inventory->isEquipped(c_rifle));
		CASE_ASSERT(inventory->getEquipped(c_weaponSlot) == c_rifle);
		Ref< InventorySlot > slot = inventory->removeSlot(0);
		CASE_ASSERT(slot != nullptr);
		CASE_ASSERT_EQUAL(slot->getCount(), 1);
		CASE_ASSERT(!inventory->isEquipped(c_rifle));
		CASE_ASSERT(inventory->getEquipped(c_weaponSlot).isNull());
	}

	// Kept items carry their entities.
	{
		Ref< Inventory > inventory = new Inventory(resourceManager, 3);
		Ref< world::Entity > a = new world::Entity();
		Ref< world::Entity > b = new world::Entity();
		CASE_ASSERT_EQUAL(inventory->add(c_relic, 5, a), 1);
		CASE_ASSERT_EQUAL(inventory->add(c_relic, 1, a), 0);
		CASE_ASSERT_EQUAL(inventory->add(c_relic, 1, b), 1);
		CASE_ASSERT_EQUAL(inventory->count(c_relic), 2);
		CASE_ASSERT(inventory->holds(a));
		CASE_ASSERT(inventory->first(c_relic) == a);
		CASE_ASSERT(!inventory->keptMissing(0));

		RefArray< world::Entity > out = inventory->removeKept(c_relic, 1);
		CASE_ASSERT_EQUAL((int32_t)out.size(), 1);
		CASE_ASSERT(out[0] == b);
		CASE_ASSERT(!inventory->holds(b));

		inventory->detachKept();
		CASE_ASSERT(inventory->keptMissing(0));
		inventory->attachKept(c_relic, RefArray< world::Entity >({ a, b }));
		CASE_ASSERT(!inventory->keptMissing(0));
		CASE_ASSERT_EQUAL((int32_t)inventory->getKept().size(), 1);

		a->destroy();
		b->destroy();
	}

	// Saved state restores slots and equipped marks, but not entities.
	{
		Ref< Inventory > inventory = new Inventory(resourceManager, 4);
		inventory->add(c_rounds, 30);
		inventory->add(c_rifle, 1);
		inventory->equip(c_rifle);
		inventory->remove(c_rounds, 24);

		Ref< PropertyGroup > state = inventory->saveState();
		CASE_ASSERT(state != nullptr);

		Ref< Inventory > restored = new Inventory(resourceManager, 4);
		restored->restoreState(state);
		CASE_ASSERT_EQUAL(restored->getSlot(0)->getCount(), 6);
		CASE_ASSERT(restored->getSlot(1) == nullptr);
		CASE_ASSERT(restored->getSlot(2)->getItem() == c_rifle);
		CASE_ASSERT(restored->isEquipped(c_rifle));
	}

	// Listeners hear every change once, and a change from a listener is not recursed into.
	{
		Ref< Inventory > inventory = new Inventory(resourceManager, 3);
		Ref< CountingDelegate > listener = new CountingDelegate();
		listener->addOnFirst = c_rounds;
		inventory->listen(listener);

		const uint32_t serial = inventory->getSerial();
		inventory->add(c_rifle, 1);
		CASE_ASSERT_EQUAL(listener->calls, 2);
		CASE_ASSERT_EQUAL(inventory->count(c_rounds), 1);
		CASE_ASSERT_EQUAL(inventory->getSerial(), serial + 2);

		inventory->remove(c_unknown, 1);
		CASE_ASSERT_EQUAL(listener->calls, 2);
	}

	// Containers merge entries, and move what fits into an inventory->
	{
		Ref< Inventory > inventory = new Inventory(resourceManager, 1);
		Ref< ItemContainerComponent > container = new ItemContainerComponent();
		container->add(c_rounds, 20);
		container->add(c_rifle, 1);
		container->add(c_rounds, 10);
		CASE_ASSERT_EQUAL(container->getEntryCount(), 2);
		CASE_ASSERT_EQUAL(container->count(c_rounds), 30);

		CASE_ASSERT_EQUAL(container->moveTo(0, inventory), 24);
		CASE_ASSERT_EQUAL(container->getCount(0), 6);
		CASE_ASSERT_EQUAL(container->moveTo(1, inventory), 0);
		CASE_ASSERT_EQUAL(container->getEntryCount(), 2);

		Ref< PropertyGroup > state = container->saveState();
		container->clear();
		container->restoreState(state);
		CASE_ASSERT_EQUAL(container->getEntryCount(), 2);
		CASE_ASSERT(container->getItem(1) == c_rifle);
		CASE_ASSERT_EQUAL(container->remove(0, 100), 6);
		CASE_ASSERT_EQUAL(container->getEntryCount(), 1);
	}
}

}

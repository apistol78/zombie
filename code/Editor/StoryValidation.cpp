/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryValidation.h"

#include "Actions/AdvanceQuestAction.h"
#include "Actions/GiveItemAction.h"
#include "ActorData.h"
#include "Conditions/AllCondition.h"
#include "Conditions/AnyCondition.h"
#include "Conditions/QuestCondition.h"
#include "Core/Log/Log.h"
#include "Core/Misc/String.h"
#include "Core/Reflection/Reflection.h"
#include "Core/Reflection/RfmObject.h"
#include "Core/Reflection/RfmPrimitive.h"
#include "Core/Reflection/RfpMemberType.h"
#include "Core/Serialization/AttributeType.h"
#include "Database/Database.h"
#include "Database/Group.h"
#include "Database/Instance.h"
#include "Database/Traverse.h"
#include "DialogData.h"
#include "Editor/DialogAsset.h"
#include "Editor/IPipelineCommon.h"
#include "Editor/StoryReferences.h"
#include "ItemContainerComponentData.h"
#include "ItemData.h"
#include "ItemPickupComponentData.h"
#include "QuestData.h"
#include "Scene/Editor/SceneAsset.h"
#include "Steps/FindItemStep.h"
#include "Steps/TalkStep.h"
#include "StoryData.h"
#include "Waypoints/FirstOfWaypoint.h"
#include "World/Entity/ExternalEntityData.h"

namespace zombie
{
	namespace
	{

std::wstring shortName(const TypeInfo& type)
{
	const std::wstring name = type.getName();
	const size_t dot = name.find_last_of(L'.');
	return (dot != name.npos) ? name.substr(dot + 1) : name;
}

/*! References which may be null, as null means something. */
bool isOptional(const ISerializable* object, const std::wstring& member)
{
	const struct { const TypeInfo& type; const wchar_t* member; } c_optional[] =
	{
		{ type_of< ActorData >(), L"dialog" },					// Not talked to.
		{ type_of< QuestCondition >(), L"step" },				// Only AtStep needs one; checked there.
		{ type_of< AdvanceQuestAction >(), L"step" },			// Whichever step the quest is at.
		{ type_of< TalkStep >(), L"line" },						// Any talk.
		{ type_of< ItemData >(), L"dropEntity" }				// Not dropped.
	};
	for (const auto& optional : c_optional)
	{
		if (is_type_of(optional.type, type_of(object)) && member == optional.member)
			return true;
	}
	return false;
}

/*! Story types are checked by type too; a dialog is referred to as its DialogData but exists as a DialogAsset. */
bool isOfType(const TypeInfo& referenceType, const ISerializable* target)
{
	if (!startsWith(referenceType.getName(), L"zombie."))
		return true;
	if (is_type_of(referenceType, type_of(target)))
		return true;
	return is_type_of< DialogData >(referenceType) && is_a< DialogAsset >(target);
}

class Validator
{
public:
	explicit Validator(editor::IPipelineCommon* pipelineCommon, const std::wstring& name)
	:	m_pipelineCommon(pipelineCommon)
	,	m_name(name)
	{
	}

	bool ok() const { return m_ok; }

	void error(const std::wstring& where, const std::wstring& what)
	{
		log::error << L"Story \"" << m_name << L"\"" << (where.empty() ? L"" : L", " + where) << L"; " << what << Endl;
		m_ok = false;
	}

	void warning(const std::wstring& where, const std::wstring& what)
	{
		log::warning << L"Story \"" << m_name << L"\"" << (where.empty() ? L"" : L", " + where) << L"; " << what << Endl;
	}

	template < typename T >
	Ref< const T > read(const Guid& id) const
	{
		if (id.isNull() || !id.isValid())
			return nullptr;
		return dynamic_type_cast< const T* >(m_pipelineCommon->getObjectReadOnly(id));
	}

	std::wstring nameOf(const Guid& id) const
	{
		db::Database* database = m_pipelineCommon->getSourceDatabase();
		Ref< db::Instance > instance = database ? database->getInstance(id) : nullptr;
		return instance ? L"\"" + instance->getName() + L"\"" : id.format();
	}

	template < typename T >
	void checkEntries(const RefArray< T >& entries, const std::wstring& where, const wchar_t* what)
	{
		for (int32_t i = 0; i < (int32_t)entries.size(); ++i)
		{
			if (entries[i] == nullptr)
				error(where, std::wstring(what) + L" " + toString(i + 1) + L" is empty.");
		}
	}

	/*! Step must be a step of quest. */
	void checkStep(const Guid& quest, const Guid& step, const std::wstring& where, const std::wstring& what)
	{
		Ref< const QuestData > questData = read< QuestData >(quest);
		if (questData && questData->findStep(step) < 0)
			error(where, what + L" refers to a step quest " + nameOf(quest) + L" does not have.");
	}

	/*! Line must be a line of actor's dialog. */
	void checkLine(const Guid& actor, const Guid& line, const std::wstring& where)
	{
		Ref< const ActorData > actorData = read< ActorData >(actor);
		if (!actorData)
			return;

		const Guid& dialog = actorData->getDialog();
		if (dialog.isNull())
		{
			error(where, L"talk step waits for a line, but actor " + nameOf(actor) + L" has no dialog of its own.");
			return;
		}

		Ref< const DialogAsset > dialogAsset = read< DialogAsset >(dialog);
		if (!dialogAsset)
			error(where, L"talk step waits for a line of dialog " + nameOf(dialog) + L", which is no dialog.");
		else if (!dialogAsset->findNode(line))
			error(where, L"talk step waits for a line dialog " + nameOf(dialog) + L" does not have.");
	}

	/*! Object, and every object in it, all reported at where. */
	void checkObject(const ISerializable* object, const std::wstring& where)
	{
		if (!object)
			return;

		Ref< Reflection > reflection = Reflection::create(object);
		if (!reflection)
			return;

		// References: none dangling, of the right type, and null only where it means something.
		RefArray< ReflectionMember > guidMembers;
		reflection->findMembers(RfpMemberType(type_of< RfmPrimitiveGuid >()), guidMembers);
		for (const ReflectionMember* member : guidMembers)
		{
			const Attribute* attributes = member->getAttributes();
			const AttributeType* attributeType = attributes ? attributes->find< AttributeType >() : nullptr;
			if (!attributeType)
				continue;

			const TypeInfo& referenceType = attributeType->getMemberType();
			const Guid& id = static_cast< const RfmPrimitiveGuid* >(member)->get();
			const std::wstring what = shortName(type_of(object)) + L" \"" + member->getName() + L"\"";

			if (!id.isValid() || id.isNull())
			{
				if (!isOptional(object, member->getName()))
					error(where, what + L" refers to nothing.");
				continue;
			}

			// Parts of instances are checked against what holds them, below.
			if (isPartReference(referenceType))
				continue;

			Ref< const ISerializable > target = m_pipelineCommon->getObjectReadOnly(id);
			if (!target)
				error(where, what + L" refers to " + id.format() + L", which does not exist.");
			else if (!isOfType(referenceType, target))
				error(where, what + L" refers to " + nameOf(id) + L", which is a " + shortName(type_of(target)) + L", not a " + shortName(referenceType) + L".");
		}

		// What each kind of object needs besides.
		if (auto questCondition = dynamic_type_cast< const QuestCondition* >(object))
		{
			if (questCondition->getState() == QuestCondition::State::AtStep)
			{
				if (questCondition->getStep().isNull())
					error(where, L"QuestCondition checks that the quest is at a step, but names none.");
				else
					checkStep(questCondition->getQuest(), questCondition->getStep(), where, L"QuestCondition \"step\"");
			}
			else if (questCondition->getStep().isNotNull())
				warning(where, L"QuestCondition names a step, which only state AtStep uses.");
		}
		else if (auto advanceQuestAction = dynamic_type_cast< const AdvanceQuestAction* >(object))
		{
			if (advanceQuestAction->getStep().isNotNull())
				checkStep(advanceQuestAction->getQuest(), advanceQuestAction->getStep(), where, L"AdvanceQuestAction \"step\"");
		}
		else if (auto talkStep = dynamic_type_cast< const TalkStep* >(object))
		{
			if (talkStep->getLine().isNotNull())
				checkLine(talkStep->getActor(), talkStep->getLine(), where);
		}
		else if (auto allCondition = dynamic_type_cast< const AllCondition* >(object))
			checkEntries(allCondition->getConditions(), where, L"AllCondition condition");
		else if (auto anyCondition = dynamic_type_cast< const AnyCondition* >(object))
		{
			checkEntries(anyCondition->getConditions(), where, L"AnyCondition condition");
			if (anyCondition->getConditions().empty())
				warning(where, L"AnyCondition has no conditions; it never holds.");
		}
		else if (auto firstOfWaypoint = dynamic_type_cast< const FirstOfWaypoint* >(object))
			checkEntries(firstOfWaypoint->getCandidates(), where, L"FirstOfWaypoint candidate");

		if (auto questStep = dynamic_type_cast< const QuestStep* >(object))
			checkEntries(questStep->getOnComplete(), where, L"action");
		else if (auto nodeAsset = dynamic_type_cast< const DialogNodeAsset* >(object))
			checkEntries(nodeAsset->onEnter, where, L"action");
		else if (auto responseAsset = dynamic_type_cast< const DialogResponseAsset* >(object))
			checkEntries(responseAsset->actions, where, L"action");
		else if (auto itemData = dynamic_type_cast< const ItemData* >(object))
			checkEntries(itemData->getTraits(), where, L"trait");

		// Every object in it.
		RefArray< ReflectionMember > objectMembers;
		reflection->findMembers(RfpMemberType(type_of< RfmObject >()), objectMembers);
		for (const ReflectionMember* member : objectMembers)
			checkObject(static_cast< const RfmObject* >(member)->get(), where);
	}

	void checkQuest(const QuestData* questData)
	{
		const auto& steps = questData->getSteps();
		if (steps.empty())
			error(L"", L"the quest has no steps.");

		for (int32_t i = 0; i < (int32_t)steps.size(); ++i)
		{
			const QuestStep* step = steps[i];
			const std::wstring where = L"step " + toString(i + 1) + (step && !step->getName().empty() ? L" \"" + step->getName() + L"\"" : L"");
			if (!step)
			{
				error(where, L"the step is empty.");
				continue;
			}

			if (step->getId().isNull() || !step->getId().isValid())
				error(where, L"the step has no guid.");
			for (int32_t j = 0; j < i; ++j)
			{
				if (steps[j] && steps[j]->getId() == step->getId())
					error(where, L"the step has the same guid as step " + toString(j + 1) + L".");
			}

			checkObject(step, where);
		}

		checkEntries(questData->getReward(), L"reward", L"action");
		for (auto action : questData->getReward())
			checkObject(action, L"reward");
	}

	void checkDialog(const DialogAsset* dialogAsset)
	{
		for (int32_t i = 0; i < (int32_t)dialogAsset->start.size(); ++i)
			checkObject(dialogAsset->start[i], L"start " + toString(i + 1));

		for (auto nodeAsset : dialogAsset->nodes)
		{
			if (!nodeAsset)
				continue;

			const std::wstring line = L"line \"" + (!nodeAsset->name.empty() ? nodeAsset->name : nodeAsset->id.format()) + L"\"";
			checkEntries(nodeAsset->onEnter, line, L"action");
			for (auto action : nodeAsset->onEnter)
				checkObject(action, line);

			for (int32_t i = 0; i < (int32_t)nodeAsset->responses.size(); ++i)
				checkObject(nodeAsset->responses[i], line + L", response " + toString(i + 1));
		}
	}

private:
	editor::IPipelineCommon* m_pipelineCommon;
	std::wstring m_name;
	bool m_ok = true;
};

	}

bool validateStoryAsset(editor::IPipelineCommon* pipelineCommon, const ISerializable* asset, const std::wstring& name)
{
	Validator validator(pipelineCommon, name);

	if (auto questData = dynamic_type_cast< const QuestData* >(asset))
		validator.checkQuest(questData);
	else if (auto dialogAsset = dynamic_type_cast< const DialogAsset* >(asset))
		validator.checkDialog(dialogAsset);
	else
		validator.checkObject(asset, L"");

	return validator.ok();
}

bool validateObtainable(editor::IPipelineCommon* pipelineCommon, const StoryData* storyData, const std::wstring& name)
{
	db::Database* database = pipelineCommon->getSourceDatabase();
	if (!database)
		return true;

	SmallSet< Guid > obtainable;
	SmallSet< Guid > visited;

	// What an object holds or gives, prefabs placed in it followed.
	std::function< void(const ISerializable*) > collect = [&](const ISerializable* root) {
		visitObjects(root, [&](const ISerializable* object) {
			if (auto pickup = dynamic_type_cast< const ItemPickupComponentData* >(object))
				obtainable.insert(pickup->getItem());
			else if (auto container = dynamic_type_cast< const ItemContainerComponentData* >(object))
			{
				for (const auto& entry : container->getEntries())
					obtainable.insert(entry.item);
			}
			else if (auto giveItemAction = dynamic_type_cast< const GiveItemAction* >(object))
				obtainable.insert(giveItemAction->getItem());
			else if (auto externalEntityData = dynamic_type_cast< const world::ExternalEntityData* >(object))
			{
				const Guid& prefab = externalEntityData->getEntityData();
				if (prefab.isNotNull() && visited.insert(prefab))
				{
					if (Ref< const ISerializable > prefabData = pipelineCommon->getObjectReadOnly(prefab))
						collect(prefabData);
				}
			}
		});
	};

	// Every scene, and everything in the story that gives items.
	RefArray< db::Instance > instances;
	for (const TypeInfo* type : { &type_of< scene::SceneAsset >(), &type_of< DialogAsset >(), &type_of< QuestData >() })
		db::recursiveFindChildInstances(database->getRootGroup(), db::FindInstanceByType(*type), instances);
	for (auto instance : instances)
	{
		if (visited.insert(instance->getGuid()))
		{
			if (Ref< const ISerializable > object = pipelineCommon->getObjectReadOnly(instance->getGuid()))
				collect(object);
		}
	}

	for (const auto& stack : storyData->getStartKit())
		obtainable.insert(stack.item);

	// Every item a quest finds.
	Validator validator(pipelineCommon, name);
	for (const auto& quest : storyData->getQuests())
	{
		Ref< const QuestData > questData = validator.read< QuestData >(quest);
		if (!questData)
			continue;

		const auto& steps = questData->getSteps();
		for (int32_t i = 0; i < (int32_t)steps.size(); ++i)
		{
			auto findItemStep = dynamic_type_cast< const FindItemStep* >(steps[i]);
			if (!findItemStep || findItemStep->getItem().isNull() || obtainable.find(findItemStep->getItem()) != obtainable.end())
				continue;

			validator.error(
				L"quest " + validator.nameOf(quest) + L", step " + toString(i + 1),
				L"the step finds item " + validator.nameOf(findItemStep->getItem()) + L", which no scene holds and nothing gives."
			);
		}
	}
	return validator.ok();
}

}

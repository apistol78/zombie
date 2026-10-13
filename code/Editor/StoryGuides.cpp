/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Editor/StoryGuides.h"

#include "Core/Misc/String.h"
#include "Database/Database.h"
#include "QuestData.h"
#include "Steps/FindItemStep.h"
#include "Steps/ReachStep.h"
#include "Steps/TalkStep.h"
#include "Waypoints/ActorWaypoint.h"
#include "Waypoints/FirstOfWaypoint.h"
#include "Waypoints/ItemWaypoint.h"
#include "Waypoints/PlaceWaypoint.h"

#include <algorithm>

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.StoryGuides", StoryGuides, Object)

StoryGuides* StoryGuides::get(const scene::SceneEditorContext* context)
{
	static SmallMap< const scene::SceneEditorContext*, Ref< StoryGuides > > s_guides;
	Ref< StoryGuides >& guides = s_guides[context];
	if (!guides)
		guides = new StoryGuides();
	return guides;
}

void StoryGuides::setQuest(db::Database* database, const Guid& quest)
{
	m_quest = quest;
	m_targets.clear();

	Ref< QuestData > questData = (database && quest.isNotNull()) ? database->getObjectReadOnly< QuestData >(quest) : nullptr;
	if (!questData)
		return;

	const auto& steps = questData->getSteps();
	for (int32_t i = 0; i < (int32_t)steps.size(); ++i)
	{
		const QuestStep* step = steps[i];
		if (!step)
			continue;

		const std::wstring typeName = type_name(step);
		const std::wstring label = toString(i + 1) + L". " + (!step->getName().empty() ? step->getName() : typeName.substr(typeName.find_last_of(L'.') + 1));

		if (auto findItemStep = dynamic_type_cast< const FindItemStep* >(step))
			add(findItemStep->getItem(), label, 0.0f);
		else if (auto talkStep = dynamic_type_cast< const TalkStep* >(step))
			add(talkStep->getActor(), label, 0.0f);
		else if (auto reachStep = dynamic_type_cast< const ReachStep* >(step))
			add(reachStep->getPlace(), label, reachStep->getRadius());

		addWaypoint(step->getWaypoint(), label);
	}
}

const StoryGuides::Target* StoryGuides::find(const Guid& id) const
{
	const auto it = m_targets.find(id);
	return (it != m_targets.end()) ? &it->second : nullptr;
}

void StoryGuides::add(const Guid& id, const std::wstring& label, float radius)
{
	if (id.isNull())
		return;

	Target& target = m_targets[id];
	target.label = !target.label.empty() ? target.label + L", " + label : label;
	target.radius = std::max(target.radius, radius);
}

void StoryGuides::addWaypoint(const IWaypoint* waypoint, const std::wstring& label)
{
	if (auto actorWaypoint = dynamic_type_cast< const ActorWaypoint* >(waypoint))
		add(actorWaypoint->getActor(), label, 0.0f);
	else if (auto placeWaypoint = dynamic_type_cast< const PlaceWaypoint* >(waypoint))
		add(placeWaypoint->getPlace(), label, 0.0f);
	else if (auto itemWaypoint = dynamic_type_cast< const ItemWaypoint* >(waypoint))
		add(itemWaypoint->getItem(), label, 0.0f);
	else if (auto firstOfWaypoint = dynamic_type_cast< const FirstOfWaypoint* >(waypoint))
	{
		for (auto candidate : firstOfWaypoint->getCandidates())
			addWaypoint(candidate, label);
	}
}

}

/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "StoryWorldComponent.h"

#include "ActorComponent.h"
#include "Core/Math/Transform.h"
#include "ItemContainerComponent.h"
#include "ItemPickupComponent.h"
#include "PlaceComponent.h"
#include "QuestData.h"
#include "QuestLog.h"
#include "Steps/FindItemStep.h"
#include "Steps/ReachStep.h"
#include "Steps/TalkStep.h"
#include "Story.h"
#include "Waypoints/ActorWaypoint.h"
#include "Waypoints/FirstOfWaypoint.h"
#include "Waypoints/ItemWaypoint.h"
#include "Waypoints/PlaceWaypoint.h"
#include "Waypoints/PointWaypoint.h"
#include "World/Entity.h"
#include "World/World.h"

#include <limits>

namespace zombie
{

T_IMPLEMENT_RTTI_CLASS(L"zombie.StoryWorldComponent", StoryWorldComponent, world::IWorldComponent)

StoryWorldComponent::StoryWorldComponent(float refreshInterval)
:	m_refreshInterval(refreshInterval)
{
}

void StoryWorldComponent::destroy()
{
	m_world = nullptr;
	m_story = nullptr;
	m_player = nullptr;
	m_actors.clear();
	m_places.clear();
	m_pickups.clear();
	m_containers.clear();
	m_talkTarget = nullptr;
}

void StoryWorldComponent::update(world::World* world, const world::UpdateParams& update)
{
	m_untilRefresh -= (float)update.deltaTime;
	if (m_untilRefresh <= 0.0f || world != m_world)
	{
		index(world);
		m_untilRefresh = m_refreshInterval;
	}

	if (!m_story)
		return;

	completeReachSteps();
	updateHeldBack();
	updateWaypoint();
}

void StoryWorldComponent::setStory(Story* story)
{
	if (story == m_story)
		return;

	m_story = story;
	m_shownSerial = ~0U;

	// Every item some quest holds back; only their pickups are shown and hidden.
	m_holdBack.clear();
	if (m_story)
	{
		for (const auto& quest : m_story->getQuests())
		{
			const QuestData* questData = m_story->getQuest(quest);
			if (!questData)
				continue;
			for (auto step : questData->getSteps())
			{
				auto findItemStep = dynamic_type_cast< const FindItemStep* >(step);
				if (findItemStep != nullptr && findItemStep->isHoldBack())
					m_holdBack.insert(findItemStep->getItem());
			}
		}
	}
}

void StoryWorldComponent::setPlayer(world::Entity* player)
{
	m_player = player;
}

void StoryWorldComponent::refresh()
{
	m_untilRefresh = 0.0f;
}

bool StoryWorldComponent::getWaypoint(Vector4& outPosition) const
{
	if (!m_haveWaypoint)
		return false;
	outPosition = m_waypoint;
	return true;
}

world::Entity* StoryWorldComponent::findActor(const Guid& actor) const
{
	const auto it = m_actors.find(actor);
	return (it != m_actors.end() && inWorld(it->second)) ? (world::Entity*)it->second : nullptr;
}

world::Entity* StoryWorldComponent::findPlace(const Guid& place) const
{
	const auto it = m_places.find(place);
	return (it != m_places.end() && inWorld(it->second)) ? (world::Entity*)it->second : nullptr;
}

world::Entity* StoryWorldComponent::findHolder(const Guid& item) const
{
	const Vector4 from = m_player ? m_player->getTransform().translation() : Vector4::origo();

	world::Entity* nearest = nullptr;
	Scalar nearestDistance(std::numeric_limits< float >::max());

	auto consider = [&](world::Entity* entity) {
		const Scalar distance = (entity->getTransform().translation() - from).xyz0().length2();
		if (distance < nearestDistance)
		{
			nearest = entity;
			nearestDistance = distance;
		}
	};

	for (auto pickup : m_pickups)
	{
		world::Entity* owner = pickup->getOwner();
		if (inWorld(owner) && owner->isVisible() && pickup->getItem() == item && pickup->getCount() > 0)
			consider(owner);
	}

	for (auto container : m_containers)
	{
		world::Entity* owner = container->getOwner();
		if (inWorld(owner) && container->count(item) > 0)
			consider(owner);
	}

	return nearest;
}

void StoryWorldComponent::index(world::World* world)
{
	m_world = world;
	m_actors.clear();
	m_places.clear();
	m_pickups.clear();
	m_containers.clear();

	if (!world)
		return;

	for (auto entity : world->getEntities())
	{
		if (auto actor = entity->getComponent< ActorComponent >())
			m_actors[actor->getActor()] = entity;
		if (auto place = entity->getComponent< PlaceComponent >())
			m_places[place->getPlace()] = entity;
		if (auto pickup = entity->getComponent< ItemPickupComponent >())
			m_pickups.push_back(pickup);
		if (auto container = entity->getComponent< ItemContainerComponent >())
			m_containers.push_back(container);
	}

	// Pickups found anew are shown or hidden as the story says.
	m_shownSerial = ~0U;
}

void StoryWorldComponent::completeReachSteps()
{
	if (!m_player)
		return;

	QuestLog* questLog = m_story->getQuestLog();
	const Vector4 at = m_player->getTransform().translation();

	for (const auto& quest : m_story->getQuests())
	{
		const ReachStep* reachStep = dynamic_type_cast< const ReachStep* >(questLog->getCurrentStep(quest));
		if (reachStep == nullptr)
			continue;

		const world::Entity* place = findPlace(reachStep->getPlace());
		if (place == nullptr)
			continue;

		const Scalar distance = (place->getTransform().translation() - at).xyz0().length();
		if (distance <= Scalar(reachStep->getRadius()))
			questLog->advance(quest, questLog->getCurrentStepId(quest));
	}
}

void StoryWorldComponent::updateHeldBack()
{
	const uint32_t serial = m_story->getQuestLog()->getSerial();
	if (serial == m_shownSerial)
		return;
	m_shownSerial = serial;

	for (auto pickup : m_pickups)
	{
		world::Entity* owner = pickup->getOwner();
		if (!inWorld(owner) || m_holdBack.find(pickup->getItem()) == m_holdBack.end())
			continue;

		const bool show = !m_story->isHeldBack(pickup->getItem());
		if (owner->isVisible() != show)
			owner->setState(show ? world::EntityState::All : world::EntityState::None, world::EntityState::Visible, true);
	}
}

void StoryWorldComponent::updateWaypoint()
{
	m_haveWaypoint = false;
	m_talkTarget = nullptr;

	const QuestLog* questLog = m_story->getQuestLog();
	const QuestStep* step = questLog->getActiveStep();
	if (step == nullptr)
		return;

	if (auto talkStep = dynamic_type_cast< const TalkStep* >(step))
		m_talkTarget = findActor(talkStep->getActor());

	bool point = false;
	Vector4 position;
	world::Entity* target = nullptr;

	if (step->getWaypoint() != nullptr)
		target = resolve(step->getWaypoint(), point, position);
	else if (auto talkStep = dynamic_type_cast< const TalkStep* >(step))
		target = findActor(talkStep->getActor());
	else if (auto reachStep = dynamic_type_cast< const ReachStep* >(step))
		target = findPlace(reachStep->getPlace());
	else if (auto findItemStep = dynamic_type_cast< const FindItemStep* >(step))
	{
		if (!questLog->hasFound(findItemStep->getItem()))
			target = findHolder(findItemStep->getItem());
	}

	if (target != nullptr)
	{
		m_waypoint = target->getTransform().translation().xyz1();
		m_haveWaypoint = true;
	}
	else if (point)
	{
		m_waypoint = position.xyz1();
		m_haveWaypoint = true;
	}
}

world::Entity* StoryWorldComponent::resolve(const IWaypoint* waypoint, bool& outPoint, Vector4& outPosition) const
{
	outPoint = false;
	if (auto firstOf = dynamic_type_cast< const FirstOfWaypoint* >(waypoint))
	{
		for (auto candidate : firstOf->getCandidates())
		{
			world::Entity* target = resolve(candidate, outPoint, outPosition);
			if (target != nullptr || outPoint)
				return target;
		}
		return nullptr;
	}
	else if (auto actorWaypoint = dynamic_type_cast< const ActorWaypoint* >(waypoint))
		return findActor(actorWaypoint->getActor());
	else if (auto placeWaypoint = dynamic_type_cast< const PlaceWaypoint* >(waypoint))
		return findPlace(placeWaypoint->getPlace());
	else if (auto itemWaypoint = dynamic_type_cast< const ItemWaypoint* >(waypoint))
		return !m_story->getQuestLog()->hasFound(itemWaypoint->getItem()) ? findHolder(itemWaypoint->getItem()) : nullptr;
	else if (auto pointWaypoint = dynamic_type_cast< const PointWaypoint* >(waypoint))
	{
		outPoint = true;
		outPosition = pointWaypoint->getPosition();
		return nullptr;
	}
	return nullptr;
}

bool StoryWorldComponent::inWorld(const world::Entity* entity) const
{
	return entity != nullptr && m_world != nullptr && entity->getWorld() == m_world;
}

}

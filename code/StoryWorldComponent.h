/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/SmallMap.h"
#include "Core/Containers/SmallSet.h"
#include "Core/Guid.h"
#include "Core/Math/Vector4.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "World/IWorldComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor::world
{

class Entity;

}

namespace zombie
{

using namespace traktor;

class IWaypoint;
class ItemContainerComponent;
class ItemPickupComponent;
class Story;

/*! Story world component; keeps the world in step with the story and resolves waypoints.
 * \ingroup Zombie
 */
class T_DLLCLASS StoryWorldComponent : public world::IWorldComponent
{
	T_RTTI_CLASS;

public:
	explicit StoryWorldComponent(float refreshInterval);

	virtual void destroy() override final;

	virtual void update(world::World* world, const world::UpdateParams& update) override final;

	void setStory(Story* story);

	Story* getStory() const { return m_story; }

	void setPlayer(world::Entity* player);

	world::Entity* getPlayer() const { return m_player; }

	/*! Look through the world again on the next update. */
	void refresh();

	/*! Position the followed quest's current step points at, false if none in this world. */
	bool getWaypoint(Vector4& outPosition) const;

	/*! Entity of the actor the followed quest's current step wants talked to, null if none in this world. */
	world::Entity* getTalkTarget() const { return m_talkTarget; }

	/*! Entity being actor, null if not in this world. */
	world::Entity* findActor(const Guid& actor) const;

	/*! Entity being place, null if not in this world. */
	world::Entity* findPlace(const Guid& place) const;

	/*! Nearest entity holding item, a pickup or a container, null if none in this world. */
	world::Entity* findHolder(const Guid& item) const;

	/*! Nearest fixture of kind not yet done with, null if none in this world. */
	world::Entity* findFixture(const Guid& kind) const;

private:
	world::World* m_world = nullptr;
	Ref< Story > m_story;
	Ref< world::Entity > m_player;
	float m_refreshInterval;
	float m_untilRefresh = 0.0f;
	SmallMap< Guid, Ref< world::Entity > > m_actors;
	SmallMap< Guid, Ref< world::Entity > > m_places;
	RefArray< ItemPickupComponent > m_pickups;
	RefArray< ItemContainerComponent > m_containers;
	SmallSet< Guid > m_holdBack;
	SmallSet< Guid > m_heldBack;
	uint32_t m_heldBackSerial = ~0U;
	SmallSet< const ItemPickupComponent* > m_hidden;
	bool m_haveWaypoint = false;
	Vector4 m_waypoint = Vector4::origo();
	Ref< world::Entity > m_talkTarget;

	void index(world::World* world);

	void completeReachSteps();

	void completeFixtureSteps();

	void updateHeldBack();

	void updateWaypoint();

	world::Entity* resolve(const IWaypoint* waypoint, bool& outPoint, Vector4& outPosition) const;

	bool inWorld(const world::Entity* entity) const;
};

}

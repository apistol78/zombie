/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/NpcWorldComponent.h"

#include "ActorComponent.h"
#include "Animation/SkeletonComponent.h"
#include "Core/Class/Boxes/BoxedVector4.h"
#include "Core/Class/IRuntimeDelegate.h"
#include "Core/Math/Color4f.h"
#include "Core/Settings/PropertyGroup.h"
#include "Core/Thread/Acquire.h"
#include "Npc/NpcComponent.h"
#include "Npc/RouteComponent.h"
#include "Render/Types.h"
#include "Runtime/Engine/DebugLayer.h"
#include "World/Entity.h"
#include "World/World.h"

namespace zombie
{
	namespace
	{

const Vector4 c_routeLift(0.0f, 0.08f, 0.0f, 0.0f);
const float c_routeWidth = 1.5f;
const float c_routeNodeSize = 4.0f;
const Color4f c_routeColor(0.65f, 0.30f, 0.95f, 1.0f);

// The "Head" joint is at the base of the skull; without one, a head's height above the feet.
const Vector4 c_headRise(0.0f, 0.1f, 0.0f, 0.0f);
const Vector4 c_headHeight(0.0f, 1.6f, 0.0f, 0.0f);
const render::Handle c_handleHead(L"Head");

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.NpcWorldComponent", NpcWorldComponent, world::IWorldComponent)

NpcWorldComponent::NpcWorldComponent(const resource::Proxy< PropertyGroup >& settings, physics::PhysicsManager* physicsManager)
:	m_settings(settings)
,	m_physicsManager(physicsManager)
{
}

void NpcWorldComponent::destroy()
{
	m_world = nullptr;
	m_debug = nullptr;
	m_damageHandler = nullptr;
	m_blows.clear();
	m_player = nullptr;
	m_npcs.clear();
	m_crowd.clear();
	m_crowdIndex.clear();
	m_playerMember = Member();
	m_routes.clear();
}

void NpcWorldComponent::update(world::World* world, const world::UpdateParams& update)
{
	m_world = world;
	if (!m_player)
		m_player = world->getEntity(L"Humanoid");

	// Before the characters update, and outside of it, as the handler is script.
	dealBlows();

	m_crowd.resize(0);
	m_crowdIndex.reset();
	for (auto npc : m_npcs)
	{
		world::Entity* entity = npc->getOwner();
		if (!entity)
			continue;

		Member& member = m_crowd.push_back();
		snapshot(entity, member);
		member.placed = entity->getComponent< ActorComponent >() != nullptr;
		m_crowdIndex[entity] = (uint32_t)(m_crowd.size() - 1);
	}

	m_playerMember = Member();
	if (m_player)
		snapshot(m_player, m_playerMember);

	if (m_debug)
	{
		drawRoutes();
		if (!m_frozen)
			drawCharacters();
	}
}

void NpcWorldComponent::add(NpcComponent* npc)
{
	m_npcs.push_back(npc);
}

void NpcWorldComponent::remove(NpcComponent* npc)
{
	m_npcs.remove(npc);
}

const PropertyGroup* NpcWorldComponent::getSettings() const
{
	return m_settings.getResource();
}

void NpcWorldComponent::setCutscene(float x, float z, float radius2)
{
	m_cutscene = true;
	m_cutsceneX = x;
	m_cutsceneZ = z;
	m_cutsceneRadius2 = radius2;
}

void NpcWorldComponent::clearCutscene()
{
	m_cutscene = false;
}

bool NpcWorldComponent::outsideCutscene(const Vector4& position) const
{
	if (!m_cutscene)
		return false;
	const float dx = position.x() - m_cutsceneX;
	const float dz = position.z() - m_cutsceneZ;
	return dx * dx + dz * dz > m_cutsceneRadius2;
}

void NpcWorldComponent::setEye(const Vector4& eye)
{
	m_eye = eye.xyz1();
	m_haveEye = true;
}

bool NpcWorldComponent::getEye(Vector4& outEye) const
{
	outEye = m_eye;
	return m_haveEye;
}

void NpcWorldComponent::setGunshot(float x, float z, int32_t seq)
{
	m_haveGunshot = true;
	m_gunshotX = x;
	m_gunshotZ = z;
	m_gunshotSeq = seq;
}

bool NpcWorldComponent::getGunshot(float& outX, float& outZ, int32_t& outSeq) const
{
	outX = m_gunshotX;
	outZ = m_gunshotZ;
	outSeq = m_gunshotSeq;
	return m_haveGunshot;
}

void NpcWorldComponent::setDebug(runtime::DebugLayer* debug)
{
	m_debug = debug;
}

void NpcWorldComponent::setDamageHandler(IRuntimeDelegate* damageHandler)
{
	m_damageHandler = damageHandler;
}

void NpcWorldComponent::damage(world::Entity* target, float amount, const Vector4& at, world::Entity* source)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
	m_blows.push_back({ target, amount, at, source });
}

void NpcWorldComponent::hurtPlayer(float amount)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
	m_playerDamage += amount;
}

float NpcWorldComponent::consumePlayerDamage()
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
	const float amount = m_playerDamage;
	m_playerDamage = 0.0f;
	return amount;
}

const NpcWorldComponent::Member* NpcWorldComponent::findMember(const world::Entity* entity) const
{
	const auto it = m_crowdIndex.find(entity);
	if (it != m_crowdIndex.end())
		return &m_crowd[it->second];
	if (entity != nullptr && entity == m_playerMember.entity)
		return &m_playerMember;
	return nullptr;
}

RouteComponent* NpcWorldComponent::findRoute(const Guid& id)
{
	T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);

	const auto it = m_routes.find(id);
	if (it != m_routes.end())
		return it->second;

	world::Entity* entity = m_world ? m_world->getEntity(id) : nullptr;
	RouteComponent* route = entity ? entity->getComponent< RouteComponent >() : nullptr;
	if (route)
	{
		// Read here, so the characters sharing it only ever read the points.
		route->getPoints();
		m_routes[id] = route;
	}
	return route;
}

RefArray< world::Entity > NpcWorldComponent::getPeople() const
{
	RefArray< world::Entity > people;
	for (auto npc : m_npcs)
	{
		world::Entity* entity = npc->getOwner();
		if (entity && entity->getComponent< ActorComponent >() != nullptr)
			people.push_back(entity);
	}
	return people;
}

RefArray< world::Entity > NpcWorldComponent::getHorde() const
{
	RefArray< world::Entity > horde;
	for (auto npc : m_npcs)
	{
		world::Entity* entity = npc->getOwner();
		if (entity && entity->getComponent< ActorComponent >() == nullptr)
			horde.push_back(entity);
	}
	return horde;
}

void NpcWorldComponent::dealBlows()
{
	AlignedVector< Blow > blows;
	{
		T_ANONYMOUS_VAR(Acquire< Semaphore >)(m_lock);
		blows.swap(m_blows);
	}

	if (!m_damageHandler)
		return;

	for (const auto& blow : blows)
	{
		const Any argv[] = { Any::fromObject(blow.target), Any::fromFloat(blow.amount), CastAny< Vector4 >::set(blow.at), Any::fromObject(blow.source) };
		m_damageHandler->call(sizeof_array(argv), argv);
	}
}

void NpcWorldComponent::snapshot(world::Entity* entity, Member& outMember) const
{
	const Transform transform = entity->getTransform();

	outMember.entity = entity;
	outMember.npc = entity->getComponent< NpcComponent >();
	outMember.position = transform.translation().xyz1();
	outMember.head = (transform.translation() + c_headHeight).xyz1();

	if (auto skeleton = entity->getComponent< animation::SkeletonComponent >())
	{
		Transform head;
		if (skeleton->getPoseTransform(c_handleHead, head) && head.translation().xyz0().length() > 0.0f)
			outMember.head = transform * head.translation().xyz1() + c_headRise;
	}

	// Settled now, as the others ask who it is while they update.
	if (outMember.npc)
		outMember.npc->getFaction();
}

void NpcWorldComponent::drawRoutes() const
{
	for (const auto& it : m_routes)
	{
		const AlignedVector< Vector4 >& points = it.second->getPoints();
		if (points.empty())
			continue;
		m_debug->point(points[0] + c_routeLift, c_routeNodeSize, c_routeColor);
		for (uint32_t i = 1; i < points.size(); ++i)
		{
			m_debug->line(points[i - 1] + c_routeLift, points[i] + c_routeLift, c_routeWidth, c_routeColor);
			m_debug->point(points[i] + c_routeLift, c_routeNodeSize, c_routeColor);
		}
		if (it.second->getLoop() && points.size() > 2)
			m_debug->line(points.back() + c_routeLift, points[0] + c_routeLift, c_routeWidth, c_routeColor);
	}
}

void NpcWorldComponent::drawCharacters() const
{
	for (const auto& member : m_crowd)
	{
		if (member.npc && !member.npc->isDead())
			member.npc->drawDebug(m_debug, member.position);
	}
}

}

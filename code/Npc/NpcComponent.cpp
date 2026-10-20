/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/NpcComponent.h"

#include "ActorComponent.h"
#include "Ai/MoveQuery.h"
#include "Ai/MoveQueryResult.h"
#include "Ai/NavMesh.h"
#include "Ai/NavMeshComponent.h"
#include "Animation/Animation/AnimationGraphPoseController.h"
#include "Animation/IK/LookAtComponent.h"
#include "Animation/SkeletonComponent.h"
#include "Core/Log/Log.h"
#include "Core/Math/Const.h"
#include "Core/Math/MathUtils.h"
#include "Core/Settings/PropertyBoolean.h"
#include "Core/Settings/PropertyFloat.h"
#include "Core/Settings/PropertyGroup.h"
#include "Core/Settings/PropertyInteger.h"
#include "Core/Settings/PropertyString.h"
#include "Npc/NpcWorldComponent.h"
#include "Npc/RouteComponent.h"
#include "Physics/PhysicsManager.h"
#include "Physics/World/Character/AiCharacterComponent.h"
#include "Physics/World/Character/CharacterComponent.h"
#include "Render/Types.h"
#include "Runtime/Engine/DebugLayer.h"
#include "World/Entity.h"
#include "World/World.h"

#include <algorithm>
#include <cmath>
#include <cwchar>
#include <limits>

namespace zombie
{
	namespace
	{

const uint32_t c_collisionDefault = 0x00000001;

// How far ahead a step is tested against the navigation mesh (m).
const float c_navProbe = 0.6f;

// How far round a character the crowd is felt at all (m).
const float c_crowdRange = 4.0f;

// Further from the player than this a head is left alone (m).
const float c_lookRange = 25.0f;

const Vector4 c_sightEye(0.0f, 1.6f, 0.0f, 0.0f);
const Vector4 c_sightChest(0.0f, 1.2f, 0.0f, 0.0f);
const Vector4 c_blowHeight(0.0f, 1.2f, 0.0f, 0.0f);

const Vector4 c_debugDotHeight(0.0f, 2.1f, 0.0f, 0.0f);
const float c_debugDotSize = 5.0f;
const Vector4 c_pathLift(0.0f, 0.15f, 0.0f, 0.0f);
const float c_pathWidth = 2.0f;
const float c_pathNodeSize = 3.0f;

const render::Handle c_handlePoses[] =
{
	render::Handle(L"idle"),
	render::Handle(L"walk"),
	render::Handle(L"running"),
	render::Handle(L"attack"),
	render::Handle(L"hit"),
	render::Handle(L"turnLeft90"),
	render::Handle(L"turnLeft180"),
	render::Handle(L"turnRight90"),
	render::Handle(L"turnRight180")
};

const render::Handle c_handleDead(L"dead");
const render::Handle c_handleWork(L"work");

/*! Numeric value of a property; numbers written as text are parsed. */
bool numberOf(const IPropertyValue* value, float& outNumber)
{
	if (auto propertyFloat = dynamic_type_cast< const PropertyFloat* >(value))
	{
		outNumber = PropertyFloat::get(propertyFloat);
		return true;
	}
	else if (auto propertyInteger = dynamic_type_cast< const PropertyInteger* >(value))
	{
		outNumber = (float)PropertyInteger::get(propertyInteger);
		return true;
	}
	else if (auto propertyString = dynamic_type_cast< const PropertyString* >(value))
	{
		const std::wstring text = PropertyString::get(propertyString);
		wchar_t* end = nullptr;
		const double number = std::wcstod(text.c_str(), &end);
		if (end == text.c_str())
			return false;
		outNumber = (float)number;
		return true;
	}
	return false;
}

/*! How far into old age or childhood: 0 for an adult, rising to 1 at 0 and 90. */
float ageFactor(float age, float youngAge, float oldAge)
{
	if (age < youngAge)
		return (youngAge - age) / youngAge;
	else if (age > oldAge)
		return std::min(1.0f, (age - oldAge) / (90.0f - oldAge));
	return 0.0f;
}

float wrapAngle(float angle)
{
	while (angle > PI)
		angle -= TWO_PI;
	while (angle < -PI)
		angle += TWO_PI;
	return angle;
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.NpcComponent", NpcComponent, world::IEntityComponent)

NpcComponent::NpcComponent(const Guid& route)
:	m_random((uint32_t)((uintptr_t)this >> 4))
,	m_routeId(route)
{
}

void NpcComponent::destroy()
{
	m_owner = nullptr;
	m_world = nullptr;
	m_npcWorld = nullptr;
	m_character = nullptr;
	m_skeleton = nullptr;
	m_pose = nullptr;
	m_lookAt = nullptr;
	m_navMesh = nullptr;
	m_player = nullptr;
	m_playerCharacter = nullptr;
	m_resolved = false;
	m_pathResult = nullptr;
	m_pathQuery = nullptr;
	m_grudges.clear();
	m_provoked = nullptr;
	m_route = nullptr;
}

void NpcComponent::setOwner(world::Entity* owner)
{
	m_owner = owner;
	m_resolved = false;
}

void NpcComponent::setWorld(world::World* world)
{
	m_world = world;
	m_npcWorld = nullptr;
	m_resolved = false;
	readStats();
}

void NpcComponent::setTransform(const Transform& transform)
{
}

Aabb3 NpcComponent::getBoundingBox() const
{
	return Aabb3();
}

const std::wstring& NpcComponent::getClass()
{
	readStats();
	return m_class;
}

const std::wstring& NpcComponent::getFaction()
{
	readStats();
	return m_faction;
}

const std::wstring& NpcComponent::getName()
{
	readStats();
	return m_name;
}

const std::wstring& NpcComponent::getMood()
{
	readStats();
	return m_mood;
}

float NpcComponent::getHealth()
{
	readStats();
	return m_health;
}

float NpcComponent::getHealthFraction()
{
	readStats();
	return m_maxHealth > 0.0f ? m_health / m_maxHealth : 0.0f;
}

float NpcComponent::getHostility()
{
	readStats();
	return m_hostility;
}

bool NpcComponent::hatesPlayer()
{
	readStats();
	return m_hatesPlayerFaction || m_hostility >= m_hostileAt;
}

bool NpcComponent::takeDamage(float amount, world::Entity* source)
{
	if (m_dead)
		return false;
	readStats();
	m_health -= amount;
	m_hurtAgo = 0.0f;
	if (source)
		hurtBy(source);
	return m_health <= 0.0f;
}

bool NpcComponent::hit(float dirX, float dirZ)
{
	if (m_dead)
		return false;

	const float len = std::sqrt(dirX * dirX + dirZ * dirZ);
	if (len < 0.001f)
		return false;

	readStats();
	m_hitX = dirX / len;
	m_hitZ = dirZ / len;
	m_hitTimer = m_hitStun;
	m_pushTimer = m_hitPushTime;
	m_pushLeft = m_hitKnockback;
	return true;
}

void NpcComponent::flinch()
{
	if (m_dead || m_hitTimer > 0.0f)
		return;
	readStats();
	m_hitTimer = (m_flinchTime >= 0.0f) ? m_flinchTime : m_hitStun;
	m_pushTimer = m_hitPushTime;
	m_pushLeft = 0.0f;
}

void NpcComponent::kill()
{
	// On the stage's thread, never during the concurrent update, as it takes the controller's body out.
	if (m_dead)
		return;
	m_dead = true;
	enterRagDoll();
}

void NpcComponent::restore(float health, float hostility, const std::wstring& mood)
{
	readStats();
	m_health = std::min(m_maxHealth, health);
	m_hostility = hostility;
	if (!mood.empty() && mood != m_baseMood)
		setMood(mood);
}

void NpcComponent::drawDebug(runtime::DebugLayer* debug, const Vector4& position) const
{
	if (m_debugColor.getAlpha() <= 0.0f)
		return;

	debug->point(position + c_debugDotHeight, c_debugDotSize, m_debugColor);

	if (!m_pathQuery)
		return;

	const uint32_t count = m_pathQuery->getSteerPathCount();
	Vector4 from = position + c_pathLift;
	for (uint32_t i = m_pathQuery->getSteerIndex(); i < count; ++i)
	{
		const Vector4 to = m_pathQuery->getSteerPathPosition(i) + c_pathLift;
		debug->line(from, to, c_pathWidth, m_debugColor);
		debug->point(to, c_pathNodeSize, m_debugColor);
		from = to;
	}
}

bool NpcComponent::readStats()
{
	if (m_statsRead)
		return true;

	if (!m_npcWorld && m_world)
		m_npcWorld = m_world->getComponent< NpcWorldComponent >();
	if (!m_npcWorld || !m_owner)
		return false;

	const PropertyGroup* settings = m_npcWorld->getSettings();
	if (!settings)
		return false;

	const PropertyGroup* npc = settingGroup(settings, L"npc");
	const PropertyGroup* enemy = settingGroup(settings, L"enemy");
	const PropertyGroup* melee = settingGroup(settings, L"melee");

	m_statsRead = true;

	auto clazz = dynamic_type_cast< const PropertyString* >(actorProperty(L"class"));
	m_class = clazz ? PropertyString::get(clazz) : m_defaultClass;
	m_faction = statString(npc, L"faction");

	const ActorComponent* actor = m_owner->getComponent< ActorComponent >();
	m_name = actor ? actor->getName() : L"";

	bool given;
	m_maxHealth = statNumber(npc, L"health", given);
	m_health = m_maxHealth;

	const float aged = ageFactor(statNumber(npc, L"age", given), settingNumber(npc, L"youngAge", 14.0f), settingNumber(npc, L"oldAge", 60.0f));
	float courage = statNumber(npc, L"courage", given);
	if (!given)
		courage -= settingNumber(npc, L"ageCourage", 0.0f) * aged;
	m_courage = clamp(courage, 0.0f, 1.0f);

	m_baseMood = statString(npc, L"mood");
	m_mood = m_baseMood;
	m_moodTimer = 0.0f;
	m_hostility = statNumber(npc, L"hostility", given);

	m_reach = statNumber(npc, L"reach", given);
	m_strikeDamage = statNumber(npc, L"strikeDamage", given);
	m_strikeInterval = statNumber(npc, L"strikeInterval", given);
	const float pace = 1.0f - settingNumber(npc, L"agePace", 0.0f) * aged;
	m_walkSpeed = statNumber(npc, L"walkSpeed", given) * pace;
	m_runSpeed = statNumber(npc, L"runSpeed", given) * pace;

	if (const PropertyGroup* hated = settingGroup(settingGroup(npc, L"factions"), m_faction))
	{
		for (const auto& it : hated->getValues())
		{
			auto yes = dynamic_type_cast< const PropertyBoolean* >(it.second);
			if (!yes || !PropertyBoolean::get(yes))
				continue;
			if (it.first == L"player")
				m_hatesPlayerFaction = true;
			else
				m_hated.insert(it.first);
		}
	}

	if (const PropertyGroup* threatWeight = settingGroup(npc, L"threatWeight"))
	{
		for (const auto& it : threatWeight->getValues())
		{
			float weight;
			if (numberOf(it.second, weight))
				m_threatWeight[it.first] = weight;
		}
	}

	m_turnRate = settingNumber(enemy, L"turnRate", 0.0f);
	m_faceTurnRate = settingNumber(enemy, L"faceTurnRate", 0.0f);
	m_avoidRadius = settingNumber(enemy, L"avoidRadius", 0.0f);
	m_separation = settingNumber(enemy, L"separation", 0.0f);
	m_playerSpace = settingNumber(enemy, L"playerSpace", 0.0f);
	m_personalSpace = m_playerSpace * 0.5f;
	m_overlapSpace = settingNumber(enemy, L"overlapSpace", 0.0f);
	m_pushOutGain = settingNumber(enemy, L"pushOutGain", 0.0f);
	m_pushOutMax = settingNumber(enemy, L"pushOutMax", 0.0f);
	m_turnCreep = settingNumber(enemy, L"turnCreep", 0.0f);
	m_closeQuarters = settingNumber(enemy, L"closeQuarters", 0.0f);

	m_hitKnockback = settingNumber(melee, L"knockback", 0.0f);
	m_hitPushTime = settingNumber(melee, L"pushTime", 0.0f);
	m_hitStun = settingNumber(melee, L"stun", 0.0f);

	m_gaitStart = settingNumber(npc, L"gaitStart", 0.0f);
	m_gaitStop = settingNumber(npc, L"gaitStop", 0.0f);
	m_gaitSmooth = settingNumber(npc, L"gaitSmooth", 0.0f);
	m_gaitSmoothStop = settingNumber(npc, L"gaitSmoothStop", 0.0f);
	m_arrive = settingNumber(npc, L"arrive", 1.0f);
	m_hostileAt = settingNumber(npc, L"hostileAt", 0.5f);
	m_hostilityOnHurt = settingNumber(npc, L"hostilityOnHurt", 0.0f);
	m_moodHold = settingNumber(npc, L"moodHold", 0.0f);
	m_regenDelay = settingNumber(npc, L"regenDelay", 0.0f);
	m_regenRate = settingNumber(npc, L"regenRate", 0.0f);
	m_threatPer = settingNumber(npc, L"threatPer", 0.0f);

	readSettings(settings);
	return true;
}

void NpcComponent::hurtBy(world::Entity* source)
{
	if (source == m_player || (m_npcWorld && source == m_npcWorld->getPlayer()))
		m_hostility = std::min(1.0f, m_hostility + m_hostilityOnHurt);
	else if (std::find(m_grudges.begin(), m_grudges.end(), source) == m_grudges.end())
		m_grudges.push_back(source);
	m_provoked = source;
	setMood(m_courage >= 0.5f ? L"angry" : L"scared");
}

void NpcComponent::setMood(const std::wstring& mood)
{
	m_mood = mood;
	m_moodTimer = m_moodHold;
}

void NpcComponent::tickMood(float deltaTime)
{
	if (m_moodTimer > 0.0f)
	{
		m_moodTimer -= deltaTime;
		if (m_moodTimer <= 0.0f)
			m_mood = m_baseMood;
	}
}

void NpcComponent::tickRegen(float deltaTime)
{
	if (m_dead || m_faction == L"undead" || m_health >= m_maxHealth)
		return;
	m_hurtAgo += deltaTime;
	if (m_hurtAgo >= m_regenDelay)
		m_health = std::min(m_maxHealth, m_health + m_regenRate * deltaTime);
}

void NpcComponent::stagger(float deltaTime)
{
	m_hitTimer -= deltaTime;

	// The shove's own metres are not walking; measure the gait from rest once back on its feet.
	m_haveGaitPos = false;
	m_gaitSpeed = 0.0f;
	m_gaitMoving = false;

	const bool pulsing = m_pushTimer > 0.0f;
	if (m_pose)
	{
		if (m_reelPose != Pose::None)
			poseState(m_reelPose);
		else
			poseState(pulsing ? Pose::Hit : Pose::None);
		if (m_usesWork)
			poseWork(false);
	}
	if (!pulsing)
		return;
	if (m_pushLeft <= 0.0f)
	{
		m_pushTimer -= deltaTime;
		return;
	}

	// Eased out, and taken from what remains so the distance is exact whatever the frame rate.
	float speed = 2.0f * m_pushLeft / m_pushTimer;
	float step = speed * deltaTime;
	if (step > m_pushLeft)
	{
		step = m_pushLeft;
		speed = deltaTime > 0.0f ? step / deltaTime : 0.0f;
	}
	m_pushLeft -= step;
	m_pushTimer -= deltaTime;

	setMaxVelocity(speed);
	float vx, vz;
	onMesh(where(), m_hitX * speed, m_hitZ * speed, vx, vz);
	m_character->move(Vector4(vx, 0.0f, vz, 0.0f), false);
}

void NpcComponent::enterRagDoll()
{
	if (!m_pose)
	{
		if (auto skeleton = m_owner->getComponent< animation::SkeletonComponent >())
			m_pose = dynamic_type_cast< animation::AnimationGraphPoseController* >(skeleton->getPoseController());
	}
	if (m_pose)
	{
		m_pose->setParameterValue(c_handleDead, true);
		poseState(Pose::None);
	}

	// Hand the body over to the rag doll; the controller stops driving the transform.
	physics::AiCharacterComponent* character = m_character;
	if (!character)
		character = m_owner->getComponent< physics::AiCharacterComponent >();
	if (character)
		character->setEnable(false);

	if (m_lookAt)
	{
		m_lookAt->setEnable(false);
		m_lookOn = 0;
	}
}

void NpcComponent::lookAtEntity(world::Entity* target, float pdist)
{
	if (!m_lookAt || pdist > c_lookRange)
		return;

	if (!target)
	{
		if (m_lookOn != 0)
		{
			m_lookAt->setEnable(false);
			m_lookOn = 0;
		}
		return;
	}

	// The player in the eye, anyone else in the face.
	Vector4 eye;
	if (target == m_player && m_npcWorld->getEye(eye))
		m_lookAt->setTarget(eye);
	else if (const auto member = m_npcWorld->findMember(target))
		m_lookAt->setTarget(member->head);

	if (m_lookOn != 1)
	{
		m_lookAt->setEnable(true);
		m_lookOn = 1;
	}
}

bool NpcComponent::resolve()
{
	if (m_resolved)
		return true;
	if (!m_owner || !m_world || !m_npcWorld)
		return false;

	if (!m_character)
	{
		m_character = m_owner->getComponent< physics::AiCharacterComponent >();
		if (m_character)
			setMaxVelocity(m_topSpeed);
	}
	if (!m_pose)
	{
		m_skeleton = m_owner->getComponent< animation::SkeletonComponent >();
		if (m_skeleton)
			m_pose = dynamic_type_cast< animation::AnimationGraphPoseController* >(m_skeleton->getPoseController());
	}
	if (!m_lookAt)
		m_lookAt = m_owner->getComponent< animation::LookAtComponent >();
	if (!m_navMesh)
		m_navMesh = m_world->getComponent< ai::NavMeshComponent >();
	if (!m_player)
		m_player = m_world->getEntity(L"Humanoid");
	if (!m_playerCharacter && m_player)
		m_playerCharacter = m_player->getComponent< physics::CharacterComponent >();

	if (!m_character || !m_navMesh || !m_navMesh->get() || !m_player)
		return false;

	if (!m_haveHeading)
	{
		m_heading = m_character->getHeadAngle();
		m_haveHeading = true;
	}

	m_resolved = m_pose != nullptr && m_playerCharacter != nullptr;
	return true;
}

void NpcComponent::setMaxVelocity(float maxVelocity)
{
	if (m_maxVelocity != maxVelocity)
	{
		m_character->setMaxVelocity(maxVelocity);
		m_maxVelocity = maxVelocity;
	}
}

Vector4 NpcComponent::where()
{
	// Its own, exact; its controller has just moved it.
	return m_owner->getTransform().translation().xyz1();
}

Vector4 NpcComponent::positionOf(world::Entity* entity)
{
	if (const auto member = m_npcWorld->findMember(entity))
		return member->position;
	return entity->getTransform().translation().xyz1();
}

void NpcComponent::collectPath()
{
	// Cleared either way; a query which found no route reports ready too.
	if (m_pathResult && m_pathResult->ready())
	{
		if (ai::MoveQuery* query = m_pathResult->get())
			m_pathQuery = query;
		m_pathResult = nullptr;
	}
}

bool NpcComponent::canPath() const
{
	return m_pathResult == nullptr;
}

void NpcComponent::requestPath(const Vector4& from, const Vector4& to)
{
	if (!m_pathResult)
		m_pathResult = m_navMesh->get()->createMoveQuery(from.xyz1(), to.xyz1());
}

Vector4 NpcComponent::ground(float x, float z, float fromY) const
{
	const Vector4 from(x, fromY, z, 1.0f);
	Vector4 point;
	if (m_navMesh->get()->findClosestPointXZ(from, 4.0f, point))
		return point;
	return from;
}

bool NpcComponent::randomPoint(const Vector4& center, float radius, Vector4& outPoint) const
{
	return m_navMesh->get()->findRandomPoint(center.xyz1(), radius, outPoint);
}

void NpcComponent::steer(const Vector4& position, float& outX, float& outZ)
{
	outX = outZ = 0.0f;
	if (!m_pathQuery)
		return;

	Vector4 moveTo;
	if (!m_pathQuery->update(position, moveTo, 0.5f))
		return;

	const float dx = moveTo.x() - position.x();
	const float dz = moveTo.z() - position.z();
	const float len = std::sqrt(dx * dx + dz * dz);
	if (len > 0.01f)
	{
		outX = dx / len;
		outZ = dz / len;
	}
}

void NpcComponent::dropPath()
{
	m_pathQuery = nullptr;
}

void NpcComponent::onMesh(const Vector4& position, float vx, float vz, float& outVx, float& outVz) const
{
	outVx = vx;
	outVz = vz;

	const float v = std::sqrt(vx * vx + vz * vz);
	if (!m_pathQuery || v < 0.001f)
		return;

	const float px = position.x();
	const float pz = position.z();
	const float k = c_navProbe / v;
	Vector4 p;
	if (!m_pathQuery->constrain(position, Vector4(px + vx * k, position.y(), pz + vz * k, 1.0f), p))
		return;

	const float s = v / c_navProbe;
	outVx = (p.x() - px) * s;
	outVz = (p.z() - pz) * s;
}

bool NpcComponent::drive(const Vector4& position, const Vector4& playerPosition, float targetX, float targetZ, float speed, const Vector4* face, float faceRange, world::Entity* engaged, float deltaTime, bool& outMoving)
{
	outMoving = false;

	const float px = position.x();
	const float py = position.y();
	const float pz = position.z();
	const float qx = playerPosition.x();
	const float qz = playerPosition.z();

	float toX = 0.0f, toY = 0.0f, toZ = 0.0f;
	if (face)
	{
		toX = face->x() - px;
		toY = face->y() - py;
		toZ = face->z() - pz;
	}

	// Up close to a character it is fighting: straight at it, rather than along a stale path.
	bool duel = false;
	if (engaged && face)
	{
		const float al = std::sqrt(toX * toX + toZ * toZ);
		if (al > 0.001f && al < faceRange)
		{
			targetX = toX / al;
			targetZ = toZ / al;
			duel = true;
		}
	}

	// Push away from nearby characters, measured in three axes so a ledge above is no part of the crowd.
	float sepX = 0.0f, sepZ = 0.0f;
	for (const auto& member : m_npcWorld->getCrowd())
	{
		const float ax = px - member.position.x();
		if (ax >= c_crowdRange || ax <= -c_crowdRange)
			continue;
		const float az = pz - member.position.z();
		const float ay = py - member.position.y();
		if (ax * ax + az * az + ay * ay > c_crowdRange * c_crowdRange || member.entity == m_owner)
			continue;
		const float radius = (member.entity == engaged) ? m_playerSpace : m_avoidRadius;
		const float d = std::sqrt(ax * ax + az * az);
		if (d > 0.001f && d < radius)
		{
			const float w = (radius - d) / radius;
			sepX += (ax / d) * w;
			sepZ += (az / d) * w;
		}
	}

	// Keep out of the player, but only just.
	{
		const float ax = px - qx;
		const float az = pz - qz;
		const float d = std::sqrt(ax * ax + az * az);
		if (d > 0.001f && d < m_playerSpace)
		{
			const float w = (m_playerSpace - d) / m_playerSpace;
			sepX += (ax / d) * w;
			sepZ += (az / d) * w;
		}
	}

	targetX += sepX * m_separation;
	targetZ += sepZ * m_separation;
	float tl = std::sqrt(targetX * targetX + targetZ * targetZ);
	if (tl > 0.001f)
	{
		targetX /= tl;
		targetZ /= tl;
	}
	else
	{
		targetX = 0.0f;
		targetZ = 0.0f;
	}

	// Inside the player's personal space the inward part of the steering is removed outright.
	float depth = 0.0f;
	float outX = 0.0f, outZ = 0.0f;
	{
		const float ox = px - qx;
		const float oz = pz - qz;
		const float od = std::sqrt(ox * ox + oz * oz);
		if (od <= 0.001f)
		{
			depth = m_overlapSpace;
			outX = std::sin(m_heading);
			outZ = std::cos(m_heading);
		}
		else if (od < m_personalSpace)
		{
			outX = ox / od;
			outZ = oz / od;
			if (od < m_overlapSpace)
				depth = m_overlapSpace - od;

			const float inward = -(targetX * outX + targetZ * outZ);
			if (inward > 0.0f)
			{
				targetX += outX * inward;
				targetZ += outZ * inward;
				const float dl = std::sqrt(targetX * targetX + targetZ * targetZ);
				if (dl > 0.001f)
				{
					targetX /= dl;
					targetZ /= dl;
				}
				else
					tl = 0.0f;
			}
		}
	}

	// Face the steering, or what it is after once that is within reach.
	float faceX = targetX, faceZ = targetZ;
	bool facing = false;
	if (face && toX * toX + toY * toY + toZ * toZ < faceRange * faceRange)
	{
		const float al = std::sqrt(toX * toX + toZ * toZ);
		if (al > 0.001f)
		{
			faceX = toX / al;
			faceZ = toZ / al;
			facing = true;
		}
	}
	const float fl = std::sqrt(faceX * faceX + faceZ * faceZ);

	// Turn at a limited rate, the shortest way round.
	if (fl > 0.001f)
	{
		const float desired = std::atan2(faceX, faceZ);
		float delta = wrapAngle(desired - m_heading);
		const float maxStep = (facing ? m_faceTurnRate : m_turnRate) * deltaTime;
		delta = clamp(delta, -maxStep, maxStep);
		m_heading += delta;
	}

	const float fx = std::sin(m_heading);
	const float fz = std::cos(m_heading);
	m_character->setHeadAngle(m_heading);

	// Freshly spawned: keep turning but take no step, and leave the graph in its stand-up state.
	if (m_spawnDelay > 0.0f)
	{
		m_spawnDelay -= deltaTime;
		return false;
	}

	// Nothing carried over in a duel, or the two orbit each other.
	if (duel)
	{
		const Vector4 velocity = m_character->getVelocity();
		m_character->setVelocity(Vector4(0.0f, velocity.y(), 0.0f, 0.0f));
	}

	if (depth > 0.0f)
	{
		// Sharing a space with the player: straight out of it before anything else.
		float back = std::min(depth * m_pushOutGain, m_pushOutMax);
		if (back > speed)
			setMaxVelocity(back);
		float vx, vz;
		onMesh(position, outX * back, outZ * back, vx, vz);
		m_character->move(Vector4(vx, 0.0f, vz, 0.0f), false);
		outMoving = true;
	}
	else if (tl > 0.001f)
	{
		// A turning character keeps some ground going by, unless what it faces is a target.
		float align = fx * targetX + fz * targetZ;
		if (!facing && align < m_turnCreep)
			align = m_turnCreep;

		if (align > 0.05f)
		{
			const float step = speed * align;
			float vx, vz;
			onMesh(position, fx * step, fz * step, vx, vz);
			const Vector4 walked(vx, 0.0f, vz, 0.0f);
			m_character->move(walked, false);
			m_coastStep = walked;
			m_haveCoastStep = true;
			outMoving = vx * vx + vz * vz > (step * 0.2f) * (step * 0.2f);
		}
	}
	return true;
}

void NpcComponent::poseState(Pose pose)
{
	if (!m_pose)
		return;

	if (m_poseWritten)
	{
		if (m_poseNow == pose)
			return;
		if (m_poseNow != Pose::None)
			m_pose->setParameterValue(c_handlePoses[(int32_t)m_poseNow], false);
		if (pose != Pose::None)
			m_pose->setParameterValue(c_handlePoses[(int32_t)pose], true);
	}
	else
	{
		for (int32_t i = 0; i < (int32_t)sizeof_array(c_handlePoses); ++i)
			m_pose->setParameterValue(c_handlePoses[i], i == (int32_t)pose);
		m_poseWritten = true;
	}
	m_poseNow = pose;
}

void NpcComponent::poseWork(bool work)
{
	const int32_t on = work ? 1 : 0;
	if (m_pose && m_workNow != on)
	{
		m_pose->setParameterValue(c_handleWork, work);
		m_workNow = on;
	}
}

void NpcComponent::applyRootMotion(float deltaTime)
{
	if (!m_skeleton)
		m_skeleton = m_owner->getComponent< animation::SkeletonComponent >();
	if (!m_skeleton || !m_character)
		return;

	const Transform delta = m_skeleton->consumeRootMotion();
	const float turn = delta.rotation().toEulerAngles().x();
	const float tx = delta.translation().x();
	const float tz = delta.translation().z();
	if (std::abs(turn) < 0.00001f && tx * tx + tz * tz < 0.0000001f)
		return;

	// Travel is in the character's frame before this delta's turn; it replaces the horizontal velocity.
	const float h = m_heading;
	if (deltaTime > 0.0f)
	{
		const float c = std::cos(h);
		const float s = std::sin(h);
		const float vx = (tx * c + tz * s) / deltaTime;
		const float vz = (-tx * s + tz * c) / deltaTime;
		const Vector4 velocity = m_character->getVelocity();
		m_character->setVelocity(Vector4(0.0f, velocity.y(), 0.0f, 0.0f));
		const float current = (m_maxVelocity >= 0.0f) ? m_maxVelocity : m_character->getMaxVelocity();
		setMaxVelocity(std::max(current, std::sqrt(vx * vx + vz * vz)));
		m_character->move(Vector4(vx, 0.0f, vz, 0.0f), false);
	}
	m_heading = h + turn;
	m_character->setHeadAngle(m_heading);
}

bool NpcComponent::gait(const Vector4& position, float deltaTime, bool driving)
{
	const bool hadLast = m_haveGaitPos;
	const Vector4 last = m_gaitPos;
	m_gaitPos = position;
	m_haveGaitPos = true;
	if (!hadLast || deltaTime <= 0.0f)
		return m_gaitMoving;

	const float dx = position.x() - last.x();
	const float dz = position.z() - last.z();
	const float v = std::sqrt(dx * dx + dz * dz) / deltaTime;
	const float k = std::min(1.0f, deltaTime * (driving ? m_gaitSmooth : m_gaitSmoothStop));
	m_gaitSpeed += (v - m_gaitSpeed) * k;
	m_gaitMoving = m_gaitSpeed > (m_gaitMoving ? m_gaitStop : m_gaitStart);
	return m_gaitMoving;
}

bool NpcComponent::sees(const Vector4& from, const Vector4& to) const
{
	physics::PhysicsManager* physicsManager = m_npcWorld->getPhysicsManager();
	if (!physicsManager)
		return true;

	const Vector4 e = from.xyz1() + c_sightEye;
	const Vector4 t = to.xyz1() + c_sightChest;
	const Vector4 r = t - e;
	const float rl = r.length();
	if (rl <= 0.01f)
		return true;

	physics::QueryResult result;
	if (!physicsManager->queryRay(e, r / Scalar(rl), rl, physics::QueryFilter(c_collisionDefault), true, result))
		return true;
	return result.distance >= rl - 0.8f;
}

bool NpcComponent::alive(world::Entity* entity) const
{
	if (!entity)
		return false;
	const NpcComponent* npc = entity->getComponent< NpcComponent >();
	return npc != nullptr && !npc->isDead();
}

bool NpcComponent::standing(world::Entity* entity) const
{
	if (!entity)
		return false;
	if (entity == m_player)
		return true;
	return alive(entity);
}

float NpcComponent::threatOf(world::Entity* other) const
{
	if (other && other != m_player)
	{
		if (NpcComponent* npc = other->getComponent< NpcComponent >())
		{
			const auto it = m_threatWeight.find(npc->m_faction);
			if (it != m_threatWeight.end())
				return it->second;
		}
	}
	return m_threatPer;
}

world::Entity* NpcComponent::nearestHated(const Vector4& position, float range, bool placedOnly, float countRange, float& outDistance, float& outThreat)
{
	world::Entity* best = nullptr;
	float bestD = range;
	float threat = 0.0f;

	const float px = position.x();
	const float py = position.y();
	const float pz = position.z();
	const float near2 = countRange * countRange;

	for (const auto& member : m_npcWorld->getCrowd())
	{
		if (placedOnly && !member.placed)
			continue;

		const float dx = member.position.x() - px;
		const float dy = member.position.y() - py;
		const float dz = member.position.z() - pz;
		const float d2 = dx * dx + dy * dy + dz * dz;
		if (d2 > range * range || member.entity == m_owner || !member.npc || member.npc->isDead())
			continue;

		const bool grudge = std::find(m_grudges.begin(), m_grudges.end(), member.entity) != m_grudges.end();
		if (!grudge && m_hated.find(member.npc->m_faction) == m_hated.end())
			continue;

		if (d2 < near2)
			threat += threatOf(member.entity);

		const float d = std::sqrt(d2);
		if (d < bestD)
		{
			best = member.entity;
			bestD = d;
		}
	}

	outDistance = bestD;
	outThreat = threat;
	return best;
}

bool NpcComponent::route()
{
	if (m_route)
		return true;
	if (m_routeId.isNull() || m_routeMissing)
		return false;

	RouteComponent* route = m_npcWorld->findRoute(m_routeId);
	if (!route || route->getPoints().empty())
	{
		log::warning << L"Npc: no route " << m_routeId.format() << Endl;
		m_routeMissing = true;
		return false;
	}

	m_route = route;
	m_routeDir = 1;
	m_routeWait = 0.0f;

	// Joined at the waypoint nearest where the character stands.
	const AlignedVector< Vector4 >& points = route->getPoints();
	const Vector4 position = m_owner->getTransform().translation().xyz1();
	float bestD = std::numeric_limits< float >::max();
	m_routeIndex = 0;
	for (int32_t i = 0; i < (int32_t)points.size(); ++i)
	{
		const float d = (points[i] - position).length();
		if (d < bestD)
		{
			bestD = d;
			m_routeIndex = i;
		}
	}
	return true;
}

Vector4 NpcComponent::routeStep(const Vector4& position, float deltaTime, bool& outWaiting)
{
	const AlignedVector< Vector4 >& points = m_route->getPoints();
	if (m_routeWait > 0.0f)
	{
		m_routeWait -= deltaTime;
		outWaiting = true;
		return points[m_routeIndex];
	}

	const Vector4& p = points[m_routeIndex];
	const float dx = p.x() - position.x();
	const float dz = p.z() - position.z();
	if (dx * dx + dz * dz > m_arrive * m_arrive)
	{
		outWaiting = false;
		return p;
	}

	const int32_t n = (int32_t)points.size();
	if (n > 1)
	{
		if (m_route->getLoop())
			m_routeIndex = (m_routeIndex + 1) % n;
		else
		{
			int32_t next = m_routeIndex + m_routeDir;
			if (next < 0 || next >= n)
			{
				m_routeDir = -m_routeDir;
				next = m_routeIndex + m_routeDir;
			}
			m_routeIndex = next;
		}
	}

	// Drawn per character, so two sharing a route fall out of step.
	m_routeWait = m_route->getPause() * (0.6f + m_random.nextFloat() * 0.8f);
	dropPath();
	m_repathTimer = 0.0f;
	outWaiting = m_routeWait > 0.0f;
	return points[m_routeIndex];
}

void NpcComponent::strike(world::Entity* target, float deltaTime)
{
	if (target == m_player)
	{
		m_npcWorld->hurtPlayer(m_strikeDamage / m_strikeInterval * deltaTime);
		return;
	}
	m_strikeTimer -= deltaTime;
	if (m_strikeTimer > 0.0f)
		return;
	m_strikeTimer = m_strikeInterval;
	blow(target);
}

void NpcComponent::blow(world::Entity* target)
{
	const Vector4 at = positionOf(target) + c_blowHeight;
	m_npcWorld->damage(target, m_strikeDamage, at, m_owner);
}

const IPropertyValue* NpcComponent::settingValue(const PropertyGroup* group, const std::wstring& name)
{
	return group ? group->getProperty(name) : nullptr;
}

const PropertyGroup* NpcComponent::settingGroup(const PropertyGroup* group, const std::wstring& name)
{
	return dynamic_type_cast< const PropertyGroup* >(settingValue(group, name));
}

float NpcComponent::settingNumber(const PropertyGroup* group, const std::wstring& name, float defaultValue)
{
	float number;
	return numberOf(settingValue(group, name), number) ? number : defaultValue;
}

std::wstring NpcComponent::settingString(const PropertyGroup* group, const std::wstring& name, const std::wstring& defaultValue)
{
	auto value = dynamic_type_cast< const PropertyString* >(settingValue(group, name));
	return value ? PropertyString::get(value) : defaultValue;
}

const IPropertyValue* NpcComponent::actorProperty(const std::wstring& name) const
{
	const ActorComponent* actor = m_owner->getComponent< ActorComponent >();
	const PropertyGroup* properties = actor ? actor->getProperties() : nullptr;
	const IPropertyValue* value = settingValue(properties, name);
	if (auto text = dynamic_type_cast< const PropertyString* >(value))
	{
		if (PropertyString::get(text).empty())
			return nullptr;
	}
	return value;
}

float NpcComponent::statNumber(const PropertyGroup* npc, const std::wstring& name, bool& outGiven) const
{
	const float defaultValue = settingNumber(settingGroup(npc, L"defaults"), name, 0.0f);
	if (const IPropertyValue* value = actorProperty(name))
	{
		outGiven = true;
		float number;
		return numberOf(value, number) ? number : defaultValue;
	}
	outGiven = false;
	const PropertyGroup* tune = settingGroup(settingGroup(npc, L"classes"), m_class);
	return settingNumber(tune, name, defaultValue);
}

std::wstring NpcComponent::statString(const PropertyGroup* npc, const std::wstring& name) const
{
	if (auto value = dynamic_type_cast< const PropertyString* >(actorProperty(name)))
		return PropertyString::get(value);
	const std::wstring defaultValue = settingString(settingGroup(npc, L"defaults"), name, L"");
	const PropertyGroup* tune = settingGroup(settingGroup(npc, L"classes"), m_class);
	return settingString(tune, name, defaultValue);
}

}

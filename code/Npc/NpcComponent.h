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
#include "Core/Math/Color4f.h"
#include "Core/Math/Random.h"
#include "Core/Math/Vector4.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "World/IEntityComponent.h"

#include <string>

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class IPropertyValue;
class PropertyGroup;

}

namespace traktor::ai
{

class MoveQuery;
class MoveQueryResult;
class NavMesh;
class NavMeshComponent;

}

namespace traktor::animation
{

class AnimationGraphPoseController;
class LookAtComponent;
class SkeletonComponent;

}

namespace traktor::physics
{

class AiCharacterComponent;
class CharacterComponent;

}

namespace traktor::runtime
{

class DebugLayer;

}

namespace zombie
{

using namespace traktor;

class NpcWorldComponent;
class RouteComponent;

/*! NPC component; what every character of the world is made of, whatever its brain.
 * \ingroup Zombie
 *
 * Updated concurrently; other characters are only read, as the world component snapshots them.
 */
class T_DLLCLASS NpcComponent : public world::IEntityComponent
{
	T_RTTI_CLASS;

public:
	explicit NpcComponent(const Guid& route);

	virtual void destroy() override;

	virtual void setOwner(world::Entity* owner) override final;

	virtual void setWorld(world::World* world) override final;

	virtual void setTransform(const Transform& transform) override final;

	virtual Aabb3 getBoundingBox() const override final;

	virtual bool allowConcurrentUpdate() const override final { return true; }

	/*! What the brain is doing, for the stage to show. */
	virtual std::wstring getState() const = 0;

	const std::wstring& getClass();

	const std::wstring& getFaction();

	/*! Actor's name; empty for a character without one. */
	const std::wstring& getName();

	const std::wstring& getMood();

	float getHealth();

	float getHealthFraction();

	float getHostility();

	bool isDead() const { return m_dead; }

	/*! Attacks the player on sight, by faction or by hostility. */
	bool hatesPlayer();

	/*! Take damage from source, null for the world; true if this killed it. */
	bool takeDamage(float amount, world::Entity* source);

	/*! Be shoved along (dirX, dirZ); false for a corpse. */
	bool hit(float dirX, float dirZ);

	/*! A hit which hurts without shoving. */
	void flinch();

	/*! Killed by the stage; becomes a rag doll. */
	void kill();

	/*! How it felt when the player last left this world. */
	void restore(float health, float hostility, const std::wstring& mood);

	/*! Draw what the brain is doing, and the route it follows, at position. */
	void drawDebug(runtime::DebugLayer* debug, const Vector4& position) const;

protected:
	enum class Pose
	{
		None = -1,
		Idle,
		Walk,
		Running,
		Attack,
		Hit,
		TurnLeft90,
		TurnLeft180,
		TurnRight90,
		TurnRight180
	};

	world::Entity* m_owner = nullptr;
	world::World* m_world = nullptr;
	NpcWorldComponent* m_npcWorld = nullptr;
	physics::AiCharacterComponent* m_character = nullptr;
	animation::SkeletonComponent* m_skeleton = nullptr;
	animation::AnimationGraphPoseController* m_pose = nullptr;
	animation::LookAtComponent* m_lookAt = nullptr;
	ai::NavMeshComponent* m_navMesh = nullptr;
	Ref< world::Entity > m_player;
	physics::CharacterComponent* m_playerCharacter = nullptr;
	bool m_resolved = false;

	// Who it is, read once from its actor and the settings.
	bool m_statsRead = false;
	std::wstring m_defaultClass = L"wanderer";
	std::wstring m_class;
	std::wstring m_faction;
	std::wstring m_name;
	float m_maxHealth = 1.0f;
	float m_health = 1.0f;
	float m_courage = 0.5f;
	std::wstring m_baseMood;
	std::wstring m_mood;
	float m_moodTimer = 0.0f;
	float m_hostility = 0.0f;
	float m_reach = 1.0f;
	float m_strikeDamage = 0.0f;
	float m_strikeInterval = 1.0f;
	float m_walkSpeed = 1.0f;
	float m_runSpeed = 1.0f;
	SmallSet< std::wstring > m_hated;
	bool m_hatesPlayerFaction = false;
	SmallMap< std::wstring, float > m_threatWeight;

	// Tuning, from the settings.
	float m_turnRate = 0.0f;
	float m_faceTurnRate = 0.0f;
	float m_avoidRadius = 0.0f;
	float m_separation = 0.0f;
	float m_playerSpace = 0.0f;
	float m_personalSpace = 0.0f;
	float m_overlapSpace = 0.0f;
	float m_pushOutGain = 0.0f;
	float m_pushOutMax = 0.0f;
	float m_turnCreep = 0.0f;
	float m_closeQuarters = 0.0f;
	float m_hitKnockback = 0.0f;
	float m_hitPushTime = 0.0f;
	float m_hitStun = 0.0f;
	float m_gaitStart = 0.0f;
	float m_gaitStop = 0.0f;
	float m_gaitSmooth = 0.0f;
	float m_gaitSmoothStop = 0.0f;
	float m_arrive = 1.0f;
	float m_hostileAt = 0.5f;
	float m_hostilityOnHurt = 0.0f;
	float m_moodHold = 0.0f;
	float m_regenDelay = 0.0f;
	float m_regenRate = 0.0f;
	float m_threatPer = 0.0f;

	// Its own, as characters update concurrently.
	Random m_random;
	Color4f m_debugColor = Color4f(0.0f, 0.0f, 0.0f, 0.0f);

	// Getting about.
	float m_topSpeed = 3.0f;
	float m_maxVelocity = -1.0f;
	bool m_haveHeading = false;
	float m_heading = 0.0f;
	float m_repathTimer = 0.0f;
	Ref< ai::MoveQueryResult > m_pathResult;
	Ref< ai::MoveQuery > m_pathQuery;
	float m_spawnDelay = 0.0f;
	bool m_haveCoastStep = false;
	Vector4 m_coastStep = Vector4::zero();

	// Being hurt.
	bool m_dead = false;
	float m_hitTimer = 0.0f;
	float m_pushTimer = 0.0f;
	float m_pushLeft = 0.0f;
	float m_hitX = 0.0f;
	float m_hitZ = 0.0f;
	float m_flinchTime = -1.0f;
	Pose m_reelPose = Pose::None;
	bool m_usesWork = false;
	float m_hurtAgo = 0.0f;
	float m_strikeTimer = 0.0f;
	RefArray< world::Entity > m_grudges;
	Ref< world::Entity > m_provoked;

	// Showing it.
	Pose m_poseNow = Pose::None;
	bool m_poseWritten = false;
	int32_t m_workNow = -1;
	int32_t m_lookOn = -1;
	bool m_haveGaitPos = false;
	Vector4 m_gaitPos = Vector4::zero();
	float m_gaitSpeed = 0.0f;
	bool m_gaitMoving = false;

	// Walking a route.
	Guid m_routeId;
	bool m_routeMissing = false;
	Ref< RouteComponent > m_route;
	int32_t m_routeIndex = 0;
	int32_t m_routeDir = 1;
	float m_routeWait = 0.0f;

	/*! Read who it is, once; false until the world's settings are there. */
	bool readStats();

	/*! The brain's own settings. */
	virtual void readSettings(const PropertyGroup* settings) = 0;

	/*! What being hurt by source does to how it feels. */
	virtual void hurtBy(world::Entity* source);

	void setMood(const std::wstring& mood);

	void tickMood(float deltaTime);

	void tickRegen(float deltaTime);

	void stagger(float deltaTime);

	void enterRagDoll();

	/*! Turn the head to target, null to let it go; pdist is the distance to the player. */
	void lookAtEntity(world::Entity* target, float pdist);

	/*! Resolve what walking needs; false until it is all there. */
	bool resolve();

	void setMaxVelocity(float maxVelocity);

	/*! Where this character stands this step. */
	Vector4 where();

	/*! Where entity, a character or the player, stands this step. */
	Vector4 positionOf(world::Entity* entity);

	void collectPath();

	bool canPath() const;

	void requestPath(const Vector4& from, const Vector4& to);

	/*! Ground beneath (x, z), searched from height fromY. */
	Vector4 ground(float x, float z, float fromY) const;

	bool randomPoint(const Vector4& center, float radius, Vector4& outPoint) const;

	/*! Unit direction towards the next corner of the route followed, zero if none. */
	void steer(const Vector4& position, float& outX, float& outZ);

	void dropPath();

	/*! Velocity which keeps a step on the navigation mesh. */
	void onMesh(const Vector4& position, float vx, float vz, float& outVx, float& outVz) const;

	/*! Walk one frame; false while held by the spawn delay, outMoving if it moved. */
	bool drive(const Vector4& position, const Vector4& playerPosition, float targetX, float targetZ, float speed, const Vector4* face, float faceRange, world::Entity* engaged, float deltaTime, bool& outMoving);

	void poseState(Pose pose);

	void poseWork(bool work);

	void applyRootMotion(float deltaTime);

	/*! Whether to show walking, from how fast it really covers ground. */
	bool gait(const Vector4& position, float deltaTime, bool driving);

	/*! Clear line from an eye at from to a chest at to. */
	bool sees(const Vector4& from, const Vector4& to) const;

	/*! A character still standing. */
	bool alive(world::Entity* entity) const;

	/*! Still worth going for, or running from. */
	bool standing(world::Entity* entity) const;

	float threatOf(world::Entity* other) const;

	/*! Nearest living character it hates within range, and the threat of those within countRange. */
	world::Entity* nearestHated(const Vector4& position, float range, bool placedOnly, float countRange, float& outDistance, float& outThreat);

	/*! Route given, resolved and joined at its nearest waypoint; false if none. */
	bool route();

	/*! Waypoint walked to, and whether it is waiting at one. */
	Vector4 routeStep(const Vector4& position, float deltaTime, bool& outWaiting);

	void strike(world::Entity* target, float deltaTime);

	void blow(world::Entity* target);

	static const IPropertyValue* settingValue(const PropertyGroup* group, const std::wstring& name);

	static const PropertyGroup* settingGroup(const PropertyGroup* group, const std::wstring& name);

	static float settingNumber(const PropertyGroup* group, const std::wstring& name, float defaultValue);

	static std::wstring settingString(const PropertyGroup* group, const std::wstring& name, const std::wstring& defaultValue);

private:
	const IPropertyValue* actorProperty(const std::wstring& name) const;

	float statNumber(const PropertyGroup* npc, const std::wstring& name, bool& outGiven) const;

	std::wstring statString(const PropertyGroup* npc, const std::wstring& name) const;
};

}

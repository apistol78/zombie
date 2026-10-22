/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Core/Containers/AlignedVector.h"
#include "Core/Containers/SmallMap.h"
#include "Core/Guid.h"
#include "Core/Thread/Semaphore.h"
#include "Core/Math/Vector4.h"
#include "Core/Ref.h"
#include "Core/RefArray.h"
#include "Resource/Proxy.h"
#include "World/IWorldComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace traktor
{

class IRuntimeDelegate;
class PropertyGroup;

}

namespace traktor::physics
{

class PhysicsManager;

}

namespace traktor::runtime
{

class DebugLayer;

}

namespace traktor::world
{

class Entity;

}

namespace zombie
{

using namespace traktor;

class NpcComponent;
class RouteComponent;

/*! NPC world component; what the characters of a world share, and their link to the stage.
 * \ingroup Zombie
 *
 * Characters update concurrently; what they share is read only during that, and what they hand back is queued.
 */
class T_DLLCLASS NpcWorldComponent : public world::IWorldComponent
{
	T_RTTI_CLASS;

public:
	/*! A character, or the player, as it stood when this step began. */
	struct Member
	{
		world::Entity* entity = nullptr;
		NpcComponent* npc = nullptr;
		Vector4 position = Vector4::zero();
		Vector4 head = Vector4::zero();
		bool placed = false;	//!< With an actor; one of the world's own people rather than the horde.
	};

	explicit NpcWorldComponent(const resource::Proxy< PropertyGroup >& settings, physics::PhysicsManager* physicsManager);

	virtual void destroy() override final;

	virtual void update(world::World* world, const world::UpdateParams& update) override final;

	void add(NpcComponent* npc);

	void remove(NpcComponent* npc);

	/*! Game play settings, null until bound. */
	const PropertyGroup* getSettings() const;

	physics::PhysicsManager* getPhysicsManager() const { return m_physicsManager; }

	/*! Stage is over or paused; characters stand still. */
	void setFrozen(bool frozen) { m_frozen = frozen; }

	bool getFrozen() const { return m_frozen; }

	/*! A cutscene is watching a disc of squared radius at (x, z). */
	void setCutscene(float x, float z, float radius2);

	void clearCutscene();

	bool inCutscene() const { return m_cutscene; }

	/*! True while a cutscene runs and position is outside the disc it watches. */
	bool outsideCutscene(const Vector4& position) const;

	/*! The player's eye, as the stage places the camera. */
	void setEye(const Vector4& eye);

	bool getEye(Vector4& outEye) const;

	/*! Where the player is aiming. */
	void setAim(const Vector4& aim) { m_aim = aim; }

	const Vector4& getAim() const { return m_aim; }

	/*! Last shot fired; seq counts up with every shot. */
	void setGunshot(float x, float z, int32_t seq);

	bool getGunshot(float& outX, float& outZ, int32_t& outSeq) const;

	/*! Characters and the routes they walk are drawn into debug, if any. */
	void setDebug(runtime::DebugLayer* debug);

	/*! Called as (target, amount, at, source) for every blow at a character, on the next update. */
	void setDamageHandler(IRuntimeDelegate* damageHandler);

	/*! A blow at a character; handed to the damage handler on the next update. */
	void damage(world::Entity* target, float amount, const Vector4& at, world::Entity* source);

	void hurtPlayer(float amount);

	/*! Damage dealt the player since last asked. */
	float consumePlayerDamage();

	world::Entity* getPlayer() const { return m_player; }

	/*! Every character of the world, as this step began. */
	const AlignedVector< Member >& getCrowd() const { return m_crowd; }

	/*! Character, or the player, of entity; null if neither. */
	const Member* findMember(const world::Entity* entity) const;

	/*! Route of entity id, null if it has none. */
	RouteComponent* findRoute(const Guid& id);

	/*! Characters with an actor; the world's own people. */
	RefArray< world::Entity > getPeople() const;

	/*! Characters without an actor; the horde. */
	RefArray< world::Entity > getHorde() const;

private:
	struct Blow
	{
		Ref< world::Entity > target;
		float amount;
		Vector4 at;
		Ref< world::Entity > source;
	};

	resource::Proxy< PropertyGroup > m_settings;
	physics::PhysicsManager* m_physicsManager;
	world::World* m_world = nullptr;
	Semaphore m_lock;
	bool m_frozen = false;
	bool m_cutscene = false;
	float m_cutsceneX = 0.0f;
	float m_cutsceneZ = 0.0f;
	float m_cutsceneRadius2 = 0.0f;
	bool m_haveEye = false;
	Vector4 m_eye = Vector4::origo();
	Vector4 m_aim = Vector4(0.0f, 0.0f, 1.0f, 0.0f);
	bool m_haveGunshot = false;
	float m_gunshotX = 0.0f;
	float m_gunshotZ = 0.0f;
	int32_t m_gunshotSeq = 0;
	Ref< runtime::DebugLayer > m_debug;
	Ref< IRuntimeDelegate > m_damageHandler;
	AlignedVector< Blow > m_blows;
	float m_playerDamage = 0.0f;
	Ref< world::Entity > m_player;
	RefArray< NpcComponent > m_npcs;
	AlignedVector< Member > m_crowd;
	SmallMap< const world::Entity*, uint32_t > m_crowdIndex;
	Member m_playerMember;
	SmallMap< Guid, Ref< RouteComponent > > m_routes;

	void dealBlows();


	void snapshot(world::Entity* entity, Member& outMember) const;

	void drawRoutes() const;

	void drawCharacters() const;
};

}

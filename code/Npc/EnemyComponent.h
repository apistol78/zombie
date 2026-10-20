/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#pragma once

#include "Npc/NpcComponent.h"

// import/export mechanism.
#undef T_DLLCLASS
#if defined(T_NATIVE_SHARED_EXPORT)
#	define T_DLLCLASS T_DLLEXPORT
#else
#	define T_DLLCLASS T_DLLIMPORT
#endif

namespace zombie
{

/*! Enemy component; the horde's brain, which patrols, investigates shots and chases.
 * \ingroup Zombie
 */
class T_DLLCLASS EnemyComponent : public NpcComponent
{
	T_RTTI_CLASS;

public:
	explicit EnemyComponent(const Guid& route);

	virtual void update(const world::UpdateParams& update) override final;

	virtual std::wstring getState() const override final;

	/*! Arrive with intent; chase the player from memory though not seen. */
	void alertTo();

	/*! Chase the player and never give up. */
	void hunt();

protected:
	virtual void readSettings(const PropertyGroup* settings) override final;

	virtual void hurtBy(world::Entity* source) override final;

private:
	enum class State
	{
		Patrol,
		Investigate,
		Attack
	};

	State m_state = State::Patrol;
	Ref< world::Entity > m_target;
	bool m_closing = false;
	float m_lostTimer = 0.0f;
	bool m_spotted = false;
	bool m_relentless = false;
	bool m_havePatrolTarget = false;
	Vector4 m_patrolTarget = Vector4::origo();
	bool m_haveHeardShot = false;
	int32_t m_heardShot = 0;
	bool m_haveInvestigateTarget = false;
	float m_investigateX = 0.0f;
	float m_investigateZ = 0.0f;
	float m_investigateTimer = 0.0f;
	bool m_haveVoiceTimer = false;
	float m_voiceTimer = 0.0f;

	float m_patrolSpeed = 0.0f;
	float m_attackSpeed = 0.0f;
	float m_investigateSpeed = 0.0f;
	float m_sightRange = 0.0f;
	float m_attackMemory = 0.0f;
	float m_spawnHunt = 0.0f;
	float m_frontDot = 0.0f;
	float m_frontReleaseDot = 0.0f;
	float m_frontCommit = 0.0f;
	float m_frontRange = 0.0f;
	float m_chaseExactRange = 0.0f;
	float m_frontRadius = 0.0f;
	float m_frontStep = 0.0f;
	float m_frontArc = 0.0f;
	float m_hearRange = 0.0f;
	float m_hearSpread = 0.0f;
	float m_hearSpreadPerM = 0.0f;
	float m_investigateArrive = 0.0f;
	float m_investigateTime = 0.0f;
	float m_patrolLeash = 0.0f;
	float m_patrolPull = 0.0f;
	float m_patrolStep = 0.0f;
	float m_patrolBias = 0.0f;
	float m_voiceRange = 0.0f;
	float m_voiceCloseRange = 0.0f;
	float m_voiceIntervalMin = 0.0f;
	float m_voiceIntervalMax = 0.0f;
	float m_voiceIntervalClose = 0.0f;
	float m_hurtRange = 1.0f;
	float m_enemyDps = 0.0f;

	/*! Chase the player from memory for this long. */
	void alertFor(float memory);

	/*! Walk to (x, z) and have a look. */
	void investigate(float x, float z);

	/*! Where a chase aims; the player, or a point on the way round to their front. */
	void chaseAim(const Vector4& enemyPosition, const Vector4& playerPosition, float pdist, float& outX, float& outZ);

	/*! Next patrol destination, drawn towards the player past the leash. */
	bool wanderPoint(const Vector4& enemyPosition, const Vector4& playerPosition, Vector4& outPoint) const;

	void voice(float deltaTime, float pdist);

	const Color4f& debugColor() const;
};

}

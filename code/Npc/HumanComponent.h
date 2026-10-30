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

/*! Human component; a living character's brain, which goes about its routine, fights or flees.
 * \ingroup Zombie
 */
class T_DLLCLASS HumanComponent : public NpcComponent
{
	T_RTTI_CLASS;

public:
	explicit HumanComponent(const Guid& route);

	virtual void update(const world::UpdateParams& update) override final;

	virtual std::wstring getState() const override final;

	/*! May the player talk to it. */
	bool canTalk();

	/*! Talked to; stop and face the player for a while. */
	void talk();

protected:
	virtual void readSettings(const PropertyGroup* settings) override final;

private:
	enum class State
	{
		Routine,
		Fight,
		Flee
	};

	/*! A one-shot clip being played out; a turn or a swing. */
	struct Clip
	{
		bool active = false;
		Pose pose = Pose::None;
		float t = 0.0f;
		float duration = 0.0f;
		float release = 0.0f;
		Ref< world::Entity > target;
		bool landed = false;
	};

	State m_state = State::Routine;
	Ref< world::Entity > m_target;
	Clip m_swing;
	Clip m_turn;
	float m_seenTimer = 0.0f;
	bool m_haveFleeFrom = false;
	Vector4 m_fleeFrom = Vector4::origo();
	float m_calmTimer = 0.0f;
	float m_attend = 0.0f;
	bool m_noticing = false;
	float m_noticeTimer = 0.0f;
	bool m_noticeTired = false;
	bool m_haveHeardShot = false;
	int32_t m_heardShot = 0;
	float m_wanderWait = 0.0f;
	bool m_haveWanderTarget = false;
	Vector4 m_wanderTarget = Vector4::origo();
	bool m_havePost = false;
	Vector4 m_post = Vector4::origo();
	Vector4 m_fightFrom = Vector4::origo();
	float m_aheadX = 0.0f;
	float m_aheadZ = 1.0f;
	int32_t m_lodPhase = 0;
	int32_t m_lodCount = 0;
	float m_lodOwed = 0.0f;
	int32_t m_lodSteps = 0;

	float m_sightRange = 0.0f;
	float m_threatRange = 0.0f;
	float m_engageRange = 0.0f;
	float m_forgetTime = 0.0f;
	float m_chaseLeash = 0.0f;
	float m_calmTime = 0.0f;
	float m_fleeHealth = 0.0f;
	float m_fleeLeg = 0.0f;
	float m_hearRange = 0.0f;
	float m_noticeRange = 0.0f;
	float m_noticeSlack = 0.0f;
	float m_noticeTime = 0.0f;
	float m_talkRange = 0.0f;
	float m_attendTime = 0.0f;
	float m_strayRadius = 0.0f;
	float m_turnMin = 0.0f;
	float m_turn180At = 0.0f;
	float m_turnRelease = 0.0f;
	float m_turnDurations[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float m_attackDuration = 0.0f;
	float m_attackHitAt = 0.0f;
	float m_attackRelease = 0.0f;
	float m_strikeArc = 0.0f;
	float m_lodNear = 0.0f;
	float m_lodFar = 0.0f;

	/*! Too far from where the fight started to chase anything at. */
	bool strayed(const Vector4& at) const;

	/*! Fight or flee from foe. */
	void face(world::Entity* foe, float distance, float threat, bool provoked);

	/*! One perception sample; who is about, and what to do about it. */
	void perceive(const Vector4& position, const Vector4& playerPosition, float pdist);

	/*! The player is roughly facing this character. */
	bool lookedAt(const Vector4& position, float pdist);

	/*! Where the routine walks to; false to stand. */
	bool routine(const Vector4& position, float pdist, float deltaTime, Vector4& outGoal);

	/*! A leg away from the threat, the one whose reachable end lies furthest from it. */
	Vector4 fleeGoal(const Vector4& position);

	/*! Step round with a turn clip; true while one runs. */
	bool turnOnTheSpot(const Vector4& position, bool haveGoal, const Vector4* face, float faceRange, float deltaTime);

	void turnTowards(const Vector4& face, const Vector4& position, float deltaTime);

	/*! One frame of a one-shot clip; true once it has ended. */
	bool playClip(Clip& clip, const Vector4& position, float deltaTime);

	/*! Swing at what it fights; true while a swing runs. */
	bool swing(const Vector4& position, world::Entity* target, const Vector4* targetPosition, float deltaTime);

	/*! A frame of the world held still; clips are played out. */
	void hold(float deltaTime);

	/*! Far off and at its routine it thinks less often; true if this update was coasted through. */
	bool coast(float pdist, float deltaTime);
};

}

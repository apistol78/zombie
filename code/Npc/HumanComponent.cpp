/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/HumanComponent.h"

#include "Ai/MoveQuery.h"
#include "Core/Math/Const.h"
#include "Core/Math/MathUtils.h"
#include "Core/Settings/PropertyGroup.h"
#include "Npc/NpcWorldComponent.h"
#include "Physics/World/Character/AiCharacterComponent.h"
#include "World/Entity.h"

#include <atomic>
#include <cmath>
#include <limits>

namespace zombie
{
	namespace
	{

const Color4f c_debugRoutine(0.85f, 0.85f, 0.85f, 1.0f);
const Color4f c_debugAttend(0.20f, 0.80f, 0.90f, 1.0f);
const Color4f c_debugFight(1.0f, 0.12f, 0.06f, 1.0f);
const Color4f c_debugFlee(1.0f, 0.90f, 0.10f, 1.0f);

// The timid run from a shot alone; braver ones only take fright at it.
const float c_shotFleeCourage = 0.35f;

// Cosine of the half-angle the player has to be looking within to notice a character.
const float c_noticeFov = 0.75f;

// Headings tried for a flight leg, from straight away.
const float c_fleeFan[] = { 0.0f, 0.785f, -0.785f, 1.571f, -1.571f, 2.356f, -2.356f };

// Handed out as characters are made, so the ones thinking less often spread over the updates.
std::atomic< int32_t > s_lodPhase = 0;

float wrapAngle(float angle)
{
	while (angle > PI)
		angle -= TWO_PI;
	while (angle < -PI)
		angle += TWO_PI;
	return angle;
}

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.HumanComponent", HumanComponent, NpcComponent)

HumanComponent::HumanComponent(const Guid& route)
:	NpcComponent(route)
{
	m_defaultClass = L"wanderer";
	m_usesWork = true;

	// No hit reaction in the graph; a hard blow only stops it for a moment, standing in Idle.
	m_flinchTime = 0.5f;
	m_reelPose = Pose::Idle;

	m_lodPhase = ++s_lodPhase;
}

void HumanComponent::update(const world::UpdateParams& update)
{
	if (!m_owner || !readStats())
		return;

	float deltaTime = (float)update.deltaTime;

	if (m_npcWorld->getFrozen())
	{
		hold(deltaTime);
		return;
	}
	if (m_dead)
		return;
	if (m_npcWorld->inCutscene())
	{
		hold(deltaTime);
		return;
	}
	if (!resolve())
		return;

	const Vector4 position = where();
	const float px = position.x();
	const float py = position.y();
	const float pz = position.z();

	// Where it was placed, and facing which way; what a merchant or worker goes back to.
	if (!m_havePost)
	{
		m_post = position;
		m_aheadX = std::sin(m_heading);
		m_aheadZ = std::cos(m_heading);
		m_havePost = true;
	}

	const Vector4 playerPosition = positionOf(m_player);
	const float pdist = (playerPosition - position).xyz0().length();

	// Far off with nothing going on; walk on, and make up for it when it next thinks.
	if (coast(pdist, deltaTime))
		return;
	const float stepTime = deltaTime;
	deltaTime += m_lodOwed;
	const int32_t steps = m_lodSteps + 1;
	m_lodOwed = 0.0f;
	m_lodSteps = 0;
	m_haveCoastStep = false;

	tickMood(deltaTime);

	// Root motion over this update alone; the clips' travel since the last think is handed over now.
	applyRootMotion(stepTime);
	tickRegen(deltaTime);
	tickHostility(deltaTime);

	if (m_target && !standing(m_target))
		m_target = nullptr;

	// A quarter of updates, or the chance any of the coasted ones would have.
	const float chance = (steps == 1) ? 0.25f : 1.0f - std::pow(0.75f, (float)steps);
	if (m_random.nextFloat() < chance)
		perceive(position, playerPosition, pdist);

	// A fight ends when the target is down, forgotten, chased too far from home or a player forgiven.
	if (m_state == State::Fight)
	{
		m_seenTimer -= deltaTime;
		bool strayed = false;
		if (m_target)
		{
			const Vector4 targetPosition = positionOf(m_target);
			const float hx = targetPosition.x() - m_post.x();
			const float hz = targetPosition.z() - m_post.z();
			strayed = hx * hx + hz * hz > m_chaseLeash * m_chaseLeash;
		}
		if (!m_target || m_seenTimer <= 0.0f || strayed || (m_target == m_player && !hatesPlayer()))
		{
			m_state = State::Routine;
			m_target = nullptr;
			m_repathTimer = 0.0f;
			dropPath();
		}
	}
	else if (m_state == State::Flee)
	{
		// A flight ends once it has gone a while without seeing anything to run from.
		m_calmTimer -= deltaTime;
		if (m_calmTimer <= 0.0f)
		{
			m_state = State::Routine;
			m_haveFleeFrom = false;
			m_repathTimer = 0.0f;
			dropPath();
		}
	}

	if (m_state == State::Flee)
		lookAtEntity(nullptr, pdist);
	else if (m_state == State::Fight && m_target)
		lookAtEntity(m_target, pdist);
	else
		lookAtEntity(m_player, pdist);

	m_debugColor =
		(m_state == State::Fight) ? c_debugFight :
		(m_state == State::Flee) ? c_debugFlee :
		(m_attend > 0.0f) ? c_debugAttend :
		c_debugRoutine;

	// Shoved or flinching; the reel replaces everything below, a turn or a swing included.
	if (m_hitTimer > 0.0f)
	{
		m_turn.active = false;
		m_swing.active = false;
		stagger(deltaTime);
		return;
	}

	collectPath();
	m_repathTimer -= deltaTime;

	float speed = m_walkSpeed;
	bool haveGoal = false;
	Vector4 goal = Vector4::origo();
	const Vector4* face = nullptr;
	float faceRange = m_closeQuarters;
	world::Entity* target = m_target;
	const Vector4 targetPosition = target ? positionOf(target) : Vector4::origo();
	Vector4 postFace = Vector4::origo();

	if (m_state == State::Fight)
	{
		speed = m_runSpeed;
		if (target)
		{
			haveGoal = true;
			goal = targetPosition;
			face = &targetPosition;
			if (m_repathTimer <= 0.0f && canPath())
			{
				m_repathTimer = 0.5f;
				requestPath(position, ground(targetPosition.x(), targetPosition.z(), py));
			}
		}
	}
	else if (m_state == State::Flee)
	{
		speed = m_runSpeed;
		if ((m_repathTimer <= 0.0f || !m_pathQuery) && canPath())
		{
			m_repathTimer = 2.0f;
			requestPath(position, fleeGoal(position));
		}
		haveGoal = true;
	}
	else
	{
		haveGoal = routine(position, pdist, deltaTime, goal);
		if (haveGoal && m_repathTimer <= 0.0f && canPath())
		{
			m_repathTimer = 1.5f;
			requestPath(position, ground(goal.x(), goal.z(), py));
		}
		if (m_attend > 0.0f || m_noticing)
		{
			face = &playerPosition;
			faceRange = m_talkRange * 2.0f;
		}
		else if (!haveGoal && m_class != L"wanderer")
		{
			// At its post, facing the way it was placed.
			postFace = Vector4(px + m_aheadX, py, pz + m_aheadZ, 1.0f);
			face = &postFace;
			faceRange = 2.0f;
		}
	}

	if (swing(position, target, target ? &targetPosition : nullptr, deltaTime))
		return;

	if (turnOnTheSpot(position, haveGoal, face, faceRange, deltaTime))
		return;

	float targetX = 0.0f, targetZ = 0.0f;
	if (haveGoal)
		steer(position, targetX, targetZ);

	setMaxVelocity(speed);

	bool moving;
	drive(position, playerPosition, targetX, targetZ, speed, face, faceRange, target != m_player ? target : nullptr, deltaTime, moving);

	if (m_pose)
	{
		const bool walking = gait(position, deltaTime, moving);
		poseState(walking ? (speed > m_walkSpeed ? Pose::Running : Pose::Walk) : Pose::Idle);
		poseWork(m_class == L"worker" && m_state == State::Routine && !walking && m_attend <= 0.0f && !m_noticing);
	}
}

std::wstring HumanComponent::getState() const
{
	switch (m_state)
	{
	case State::Fight:
		return L"fight";
	case State::Flee:
		return L"flee";
	default:
		return L"routine";
	}
}

bool HumanComponent::canTalk()
{
	return !m_dead && m_state == State::Routine && !hatesPlayer();
}

void HumanComponent::talk()
{
	readStats();
	m_attend = m_attendTime;
	dropPath();

	// The body stops dead with nothing driving it, whatever the gait last measured.
	m_gaitSpeed = 0.0f;
	m_gaitMoving = false;
	if (!m_turn.active && !m_swing.active && m_pose)
	{
		poseState(Pose::Idle);
		poseWork(false);
	}
}

void HumanComponent::readSettings(const PropertyGroup* settings)
{
	const PropertyGroup* npc = settingGroup(settings, L"npc");
	const PropertyGroup* turns = settingGroup(npc, L"turns");
	const PropertyGroup* attack = settingGroup(npc, L"attack");

	m_sightRange = settingNumber(npc, L"sightRange", 0.0f);
	m_threatRange = settingNumber(npc, L"threatRange", 0.0f);
	m_engageRange = settingNumber(npc, L"engageRange", 0.0f);
	m_forgetTime = settingNumber(npc, L"forgetTime", 0.0f);
	m_chaseLeash = settingNumber(npc, L"chaseLeash", 0.0f);
	m_calmTime = settingNumber(npc, L"calmTime", 0.0f);
	m_fleeHealth = settingNumber(npc, L"fleeHealth", 0.0f);
	m_fleeLeg = settingNumber(npc, L"fleeLeg", 0.0f);
	m_hearRange = settingNumber(npc, L"hearRange", 0.0f);
	m_noticeRange = settingNumber(npc, L"noticeRange", 0.0f);
	m_noticeSlack = settingNumber(npc, L"noticeSlack", 0.0f);
	m_noticeTime = settingNumber(npc, L"noticeTime", 0.0f);
	m_talkRange = settingNumber(npc, L"talkRange", 0.0f);
	m_attendTime = settingNumber(npc, L"attendTime", 0.0f);
	m_strayRadius = settingNumber(npc, L"strayRadius", 0.0f);
	m_turnMin = settingNumber(npc, L"turnMin", 0.0f);
	m_turn180At = settingNumber(npc, L"turn180At", 0.0f);
	m_turnRelease = settingNumber(npc, L"turnRelease", 0.0f);
	m_turnDurations[0] = settingNumber(turns, L"turnLeft90", 0.0f);
	m_turnDurations[1] = settingNumber(turns, L"turnLeft180", 0.0f);
	m_turnDurations[2] = settingNumber(turns, L"turnRight90", 0.0f);
	m_turnDurations[3] = settingNumber(turns, L"turnRight180", 0.0f);
	m_attackDuration = settingNumber(attack, L"duration", 0.0f);
	m_attackHitAt = settingNumber(attack, L"hitAt", 0.0f);
	m_attackRelease = settingNumber(attack, L"release", 0.0f);
	m_strikeArc = settingNumber(npc, L"strikeArc", 0.0f);
	m_lodNear = settingNumber(npc, L"lodNear", 0.0f);
	m_lodFar = settingNumber(npc, L"lodFar", 0.0f);
}

void HumanComponent::face(world::Entity* foe, float distance, float threat, bool provoked)
{
	// The one faced counts whatever its distance.
	threat = std::max(threat, threatOf(foe));
	const bool hurt = getHealthFraction() < (1.0f - m_courage) * m_fleeHealth;
	if (hurt || threat > m_courage)
	{
		if (m_state != State::Flee)
		{
			setMood(L"scared");
			m_repathTimer = 0.0f;
			dropPath();
		}
		m_state = State::Flee;
		m_target = nullptr;
		m_fleeFrom = positionOf(foe);
		m_haveFleeFrom = true;
		m_calmTimer = m_calmTime;
	}
	else
	{
		// A fight is only started in engage range, unless provoked or already fighting it.
		const bool fighting = m_state == State::Fight && m_target == foe;
		if (!fighting && !provoked && distance > m_engageRange)
			return;
		if (!fighting)
		{
			setMood(L"angry");
			m_repathTimer = 0.0f;
		}
		m_state = State::Fight;
		m_target = foe;
		m_seenTimer = m_forgetTime;
	}
	m_attend = 0.0f;
}

void HumanComponent::perceive(const Vector4& position, const Vector4& playerPosition, float pdist)
{
	// Whoever just hurt it is seen, whatever the rays would say.
	Ref< world::Entity > provoked = m_provoked;
	m_provoked = nullptr;

	float foeDistance, threat;
	world::Entity* foe = nearestHated(position, m_sightRange, false, m_threatRange, foeDistance, threat);
	if (foe && !sees(position, positionOf(foe)))
	{
		foe = nullptr;
		foeDistance = std::numeric_limits< float >::max();
	}
	if (hatesPlayer() && pdist < m_sightRange && pdist < foeDistance && sees(position, playerPosition))
	{
		foe = m_player;
		foeDistance = pdist;
		threat += threatOf(m_player);
	}
	if (provoked && standing(provoked))
	{
		foe = provoked;
		foeDistance = (positionOf(provoked) - position).xyz0().length();
	}

	if (foe)
		face(foe, foeDistance, threat, foe == provoked);

	// Nothing new to go for, but the one it is after is still in view.
	if (m_state == State::Fight && m_target && m_target != foe && sees(position, positionOf(m_target)))
		m_seenTimer = m_forgetTime;

	// A shot nearby is frightening, if it is not busy with something worse.
	float shotX, shotZ;
	int32_t shotSeq;
	if (m_npcWorld->getGunshot(shotX, shotZ, shotSeq) && (!m_haveHeardShot || shotSeq != m_heardShot))
	{
		m_heardShot = shotSeq;
		m_haveHeardShot = true;

		const float hx = shotX - position.x();
		const float hz = shotZ - position.z();
		if (m_state == State::Routine && hx * hx + hz * hz < m_hearRange * m_hearRange)
		{
			if (m_mood != L"angry")
				setMood(L"scared");
			if (m_courage < c_shotFleeCourage)
			{
				m_state = State::Flee;
				m_fleeFrom = Vector4(shotX, position.y(), shotZ, 1.0f);
				m_haveFleeFrom = true;
				m_calmTimer = m_calmTime;
				m_attend = 0.0f;
				m_repathTimer = 0.0f;
				dropPath();
			}
		}
	}
}

bool HumanComponent::lookedAt(const Vector4& position, float pdist)
{
	if (pdist < 0.0001f)
		return true;
	const Vector4 playerPosition = positionOf(m_player);
	const Vector4& aim = m_npcWorld->getAim();
	return (position.x() - playerPosition.x()) * aim.x() + (position.z() - playerPosition.z()) * aim.z() > pdist * c_noticeFov;
}

bool HumanComponent::routine(const Vector4& position, float pdist, float deltaTime, Vector4& outGoal)
{
	// Noticed at noticeRange and let go a little further out; capped so it gets back to its day.
	const float near = m_noticeRange + (m_noticing ? m_noticeSlack : 0.0f);
	const bool inRange = pdist < near && lookedAt(position, pdist);
	if (!inRange)
	{
		m_noticeTimer = 0.0f;
		m_noticeTired = false;
	}
	m_noticing = inRange && !m_noticeTired && !hatesPlayer();
	if (m_noticing)
	{
		m_noticeTimer += deltaTime;
		if (m_noticeTimer >= m_noticeTime)
		{
			m_noticeTired = true;
			m_noticing = false;
		}
	}
	if (m_attend > 0.0f || m_noticing)
	{
		m_attend = std::max(0.0f, m_attend - deltaTime);
		dropPath();
		return false;
	}

	if (m_class == L"wanderer")
	{
		if (route())
		{
			bool waiting;
			const Vector4 point = routeStep(position, deltaTime, waiting);
			if (waiting)
			{
				dropPath();
				return false;
			}
			outGoal = point;
			return true;
		}

		// No route; amble about the spot it was placed on, a short leg at a time.
		if (m_wanderWait > 0.0f)
		{
			m_wanderWait -= deltaTime;
			return false;
		}
		bool goal = m_haveWanderTarget;
		if (goal)
		{
			const float dx = m_wanderTarget.x() - position.x();
			const float dz = m_wanderTarget.z() - position.z();
			if (dx * dx + dz * dz < m_arrive * m_arrive)
			{
				goal = false;
				m_wanderWait = 2.0f + m_random.nextFloat() * 4.0f;
				dropPath();
			}
		}
		if (!goal && m_wanderWait <= 0.0f)
		{
			goal = randomPoint(m_post, m_strayRadius, m_wanderTarget);
			m_repathTimer = 0.0f;
		}
		m_haveWanderTarget = goal;
		outGoal = m_wanderTarget;
		return goal;
	}

	// Merchant, worker; back to the post, then stand at it.
	const float dx = m_post.x() - position.x();
	const float dz = m_post.z() - position.z();
	if (dx * dx + dz * dz > m_arrive * m_arrive)
	{
		outGoal = m_post;
		return true;
	}
	dropPath();
	return false;
}

Vector4 HumanComponent::fleeGoal(const Vector4& position)
{
	const float px = position.x();
	const float py = position.y();
	const float pz = position.z();
	const float fx = m_haveFleeFrom ? (float)m_fleeFrom.x() : px;
	const float fz = m_haveFleeFrom ? (float)m_fleeFrom.z() : pz;

	float ax = px - fx;
	float az = pz - fz;
	const float al = std::sqrt(ax * ax + az * az);
	if (al < 0.001f)
	{
		ax = std::sin(m_heading);
		az = std::cos(m_heading);
	}
	else
	{
		ax /= al;
		az /= al;
	}

	// Chosen rather than aimed; each heading walked out along the mesh, the furthest from the threat wins.
	if (m_pathQuery)
	{
		const float away = std::atan2(ax, az);
		bool haveBest = false;
		Vector4 best;
		float bestScore = -std::numeric_limits< float >::max();
		for (const float turn : c_fleeFan)
		{
			const float h = away + turn;
			const Vector4 want(px + std::sin(h) * m_fleeLeg, py, pz + std::cos(h) * m_fleeLeg, 1.0f);
			Vector4 reached;
			if (!m_pathQuery->constrain(position, want, reached))
				continue;
			const float mx = reached.x() - px;
			const float mz = reached.z() - pz;
			const float tx = reached.x() - fx;
			const float tz = reached.z() - fz;
			float score = std::sqrt(tx * tx + tz * tz);
			if (mx * mx + mz * mz < 4.0f)
				score -= 100.0f;
			if (score > bestScore)
			{
				best = reached;
				bestScore = score;
				haveBest = true;
			}
		}
		if (haveBest)
			return best;
	}

	const float x = px + ax * m_fleeLeg;
	const float z = pz + az * m_fleeLeg;
	Vector4 point;
	if (randomPoint(Vector4(x, py, z, 1.0f), 3.0f, point))
		return point;
	return ground(x, z, py);
}

bool HumanComponent::turnOnTheSpot(const Vector4& position, bool haveGoal, const Vector4* face, float faceRange, float deltaTime)
{
	if (m_turn.active && (m_state != State::Routine || haveGoal))
		m_turn.active = false;

	if (!m_turn.active)
	{
		if (m_state != State::Routine || haveGoal || !face || m_gaitMoving)
			return false;

		const Vector4 d = (*face - position).xyz0();
		if (d.length2() >= faceRange * faceRange)
			return false;

		const float desired = std::atan2((float)d.x(), (float)d.z());
		const float delta = wrapAngle(desired - m_heading);
		if (std::abs(delta) < m_turnMin)
			return false;

		// Heading grows to the right.
		const bool wide = std::abs(delta) >= m_turn180At;
		const int32_t clip = (delta < 0.0f ? 0 : 2) + (wide ? 1 : 0);
		const Pose poses[] = { Pose::TurnLeft90, Pose::TurnLeft180, Pose::TurnRight90, Pose::TurnRight180 };

		m_turn = Clip();
		m_turn.active = true;
		m_turn.pose = poses[clip];
		m_turn.duration = m_turnDurations[clip];
		m_turn.release = m_turnRelease;
	}

	if (playClip(m_turn, position, deltaTime))
		m_turn.active = false;
	return true;
}

void HumanComponent::turnTowards(const Vector4& face, const Vector4& position, float deltaTime)
{
	const float delta = wrapAngle(std::atan2(face.x() - position.x(), face.z() - position.z()) - m_heading);
	const float step = m_faceTurnRate * deltaTime;
	m_heading += clamp(delta, -step, step);
	m_character->setHeadAngle(m_heading);
}

bool HumanComponent::playClip(Clip& clip, const Vector4& position, float deltaTime)
{
	// The clip's root motion is not walking; the gait is held at a standstill throughout.
	clip.t += deltaTime;
	m_gaitPos = position;
	m_haveGaitPos = true;
	m_gaitSpeed = 0.0f;
	m_gaitMoving = false;
	if (m_pose)
	{
		poseState((clip.t < clip.duration - clip.release) ? clip.pose : Pose::Idle);
		poseWork(false);
	}
	return clip.t >= clip.duration;
}

bool HumanComponent::swing(const Vector4& position, world::Entity* target, const Vector4* targetPosition, float deltaTime)
{
	if (m_state == State::Fight && targetPosition && (*targetPosition - position).xyz0().length2() < m_reach * m_reach)
	{
		if (target == m_player)
			m_npcWorld->hurtPlayer(m_strikeDamage / m_strikeInterval * deltaTime);

		m_strikeTimer -= deltaTime;
		const float aim = wrapAngle(std::atan2(targetPosition->x() - position.x(), targetPosition->z() - position.z()) - m_heading);
		if (m_strikeTimer <= 0.0f && !m_swing.active && std::abs(aim) < m_strikeArc)
		{
			m_strikeTimer = m_strikeInterval;
			m_swing = Clip();
			m_swing.active = true;
			m_swing.pose = Pose::Attack;
			m_swing.duration = m_attackDuration;
			m_swing.release = m_attackRelease;
			m_swing.target = target;
		}
	}

	if (!m_swing.active)
		return false;

	const bool ended = playClip(m_swing, position, deltaTime);
	if (targetPosition && m_swing.target == target)
		turnTowards(*targetPosition, position, deltaTime);

	// The blow lands where the fist is out, on whatever was swung at.
	if (!m_swing.landed && m_swing.t >= m_attackHitAt)
	{
		m_swing.landed = true;
		if (m_swing.target != m_player && standing(m_swing.target))
			blow(m_swing.target);
	}

	if (ended)
		m_swing.active = false;
	return true;
}

void HumanComponent::hold(float deltaTime)
{
	if (m_dead || !m_haveHeading || !m_character)
		return;

	applyRootMotion(deltaTime);

	const Vector4 position = m_owner->getTransform().translation().xyz1();
	if (m_swing.active)
	{
		// No blow is struck while the world is held; the swing is only played out.
		m_swing.landed = true;
		if (playClip(m_swing, position, deltaTime))
			m_swing.active = false;
	}
	else if (m_attend > 0.0f && m_state == State::Routine && m_player)
	{
		const Vector4 face = positionOf(m_player);
		if (!turnOnTheSpot(position, false, &face, std::numeric_limits< float >::max(), deltaTime))
			turnTowards(face, position, deltaTime);
	}
	else if (m_turn.active)
	{
		if (playClip(m_turn, position, deltaTime))
			m_turn.active = false;
	}
}

bool HumanComponent::coast(float pdist, float deltaTime)
{
	int32_t period = 1;
	if (pdist > m_lodNear && m_state == State::Routine && m_attend <= 0.0f && !m_noticing && !m_turn.active && !m_swing.active && m_hitTimer <= 0.0f && !m_provoked)
		period = pdist > m_lodFar ? 4 : 2;

	m_lodCount++;
	if (period == 1 || (m_lodCount + m_lodPhase) % period == 0)
		return false;

	// Walk on with the last step; a controller left alone for an update stops dead.
	m_lodOwed += deltaTime;
	m_lodSteps++;
	if (m_haveCoastStep)
		m_character->move(m_coastStep, false);
	return true;
}

}

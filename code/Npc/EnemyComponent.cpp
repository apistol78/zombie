/*
 * TRAKTOR
 * Copyright (c) 2026 Anders Pistol.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#include "Npc/EnemyComponent.h"

#include "Core/Settings/PropertyGroup.h"
#include "Npc/NpcWorldComponent.h"
#include "Physics/World/Character/CharacterComponent.h"
#include "World/Entity.h"
#include "World/Entity/EventSetComponent.h"

#include <cmath>

namespace zombie
{
	namespace
	{

const Color4f c_debugSighted(1.0f, 0.12f, 0.06f, 1.0f);
const Color4f c_debugMemory(1.0f, 0.62f, 0.05f, 1.0f);
const Color4f c_debugUnaware(0.18f, 0.34f, 0.62f, 1.0f);
const Color4f c_debugHeard(0.30f, 0.82f, 0.36f, 1.0f);

	}

T_IMPLEMENT_RTTI_CLASS(L"zombie.EnemyComponent", EnemyComponent, NpcComponent)

EnemyComponent::EnemyComponent(const Guid& route)
:	NpcComponent(route)
{
	m_defaultClass = L"braindead";
	m_rising = true;
}

void EnemyComponent::update(const world::UpdateParams& update)
{
	if (!m_owner || !readStats())
		return;

	const float deltaTime = (float)update.deltaTime;

	if (m_npcWorld->getFrozen())
		return;

	// Killed; the corpse is driven by physics from now on.
	if (m_dead)
		return;

	// Only the zombies inside a hive being watched move during its cutscene.
	if (m_npcWorld->outsideCutscene(m_owner->getTransform().translation()))
		return;

	if (!resolve())
		return;

	const Vector4 enemyPosition = where();
	const Vector4 playerPosition = positionOf(m_player);
	const float ex = enemyPosition.x();
	const float ey = enemyPosition.y();
	const float ez = enemyPosition.z();
	const float pdist = (playerPosition - enemyPosition).xyz0().length();

	world::Entity* target = m_target;
	if (target && (!alive(target) || target->getWorld() == nullptr))
	{
		m_target = nullptr;
		target = nullptr;
	}

	// Perception is sampled, since it costs rays.
	if (m_random.nextFloat() < 0.25f)
	{
		bool spotted = false;
		if (pdist < m_sightRange)
			spotted = sees(enemyPosition, playerPosition);

		// Anyone else to eat; the nearest the horde hates, if it can be seen and is nearer than the player.
		float preyDistance, threat;
		world::Entity* prey = nearestHated(enemyPosition, m_sightRange, true, m_sightRange, preyDistance, threat);
		if (prey && (!spotted || preyDistance < pdist) && sees(enemyPosition, positionOf(prey)))
		{
			if (m_target != prey)
			{
				m_closing = false;
				m_repathTimer = 0.0f;
			}
			m_target = prey;
			spotted = true;
		}
		else if (spotted && m_target)
		{
			m_target = nullptr;
			m_closing = false;
			m_repathTimer = 0.0f;
		}
		target = m_target;
		m_spotted = spotted;

		if (spotted)
		{
			m_lostTimer = m_attackMemory;
			if (m_state != State::Attack)
			{
				m_state = State::Attack;
				m_closing = false;
			}
		}
		else if (m_state == State::Attack && !m_relentless)
		{
			m_lostTimer -= deltaTime;
			if (m_lostTimer <= 0.0f)
			{
				m_state = State::Patrol;
				m_target = nullptr;
				target = nullptr;
				m_havePatrolTarget = false;
				m_repathTimer = 0.0f;
			}
		}

		// Hearing; every shot is read off the counter, whether or not it was in a position to care.
		float shotX, shotZ;
		int32_t shotSeq;
		if (m_npcWorld->getGunshot(shotX, shotZ, shotSeq) && (!m_haveHeardShot || shotSeq != m_heardShot))
		{
			m_heardShot = shotSeq;
			m_haveHeardShot = true;

			const float hx = shotX - ex;
			const float hz = shotZ - ez;
			const float hd = std::sqrt(hx * hx + hz * hz);
			if (m_state == State::Attack)
			{
				if (hd < m_hearRange && !spotted && !m_relentless)
				{
					m_lostTimer = m_attackMemory;
					m_repathTimer = 0.0f;
				}
			}
			else if (hd < m_hearRange)
			{
				const float spread = m_hearSpread + hd * m_hearSpreadPerM;
				const float gx = shotX + (m_random.nextFloat() * 2.0f - 1.0f) * spread;
				const float gz = shotZ + (m_random.nextFloat() * 2.0f - 1.0f) * spread;
				investigate(gx, gz);
			}
		}
	}

	m_debugColor = debugColor();

	// Shoved; the reel replaces everything below, but it goes on watching through it.
	if (m_hitTimer > 0.0f)
	{
		stagger(deltaTime);
		return;
	}

	float speed = m_patrolSpeed;
	if (m_state == State::Attack)
		speed = m_attackSpeed;
	else if (m_state == State::Investigate)
		speed = m_investigateSpeed;
	setMaxVelocity(m_attackSpeed);

	collectPath();
	m_repathTimer -= deltaTime;

	if (m_state == State::Attack)
	{
		if (m_repathTimer <= 0.0f && canPath())
		{
			m_repathTimer = 0.5f;
			float aimX, aimZ;
			if (target)
			{
				const Vector4 targetPosition = positionOf(target);
				aimX = targetPosition.x();
				aimZ = targetPosition.z();
			}
			else
			{
				// Jittered so the horde does not stack its paths on one point, but not from far off.
				const float jitter = pdist > m_chaseExactRange ? 0.0f : 1.5f;
				chaseAim(enemyPosition, playerPosition, pdist, aimX, aimZ);
				aimX += (m_random.nextFloat() * 2.0f - 1.0f) * jitter;
				aimZ += (m_random.nextFloat() * 2.0f - 1.0f) * jitter;
			}
			requestPath(enemyPosition, ground(aimX, aimZ, ey));
		}
	}
	else if (m_state == State::Investigate)
	{
		m_investigateTimer -= deltaTime;
		bool done = !m_haveInvestigateTarget || m_investigateTimer <= 0.0f;
		if (!done)
		{
			const float gx = m_investigateX - ex;
			const float gz = m_investigateZ - ez;
			done = gx * gx + gz * gz < m_investigateArrive * m_investigateArrive;
		}
		if (done)
		{
			m_state = State::Patrol;
			m_haveInvestigateTarget = false;
			m_havePatrolTarget = false;
			m_repathTimer = 0.0f;
		}
		else if (m_repathTimer <= 0.0f && canPath())
		{
			m_repathTimer = 1.5f;
			requestPath(enemyPosition, ground(m_investigateX, m_investigateZ, ey));
		}
	}
	else if (route())
	{
		bool waiting;
		const Vector4 point = routeStep(enemyPosition, deltaTime, waiting);
		if (waiting)
			dropPath();
		else if (m_repathTimer <= 0.0f && canPath())
		{
			m_repathTimer = 1.5f;
			requestPath(enemyPosition, ground(point.x(), point.z(), ey));
		}
	}
	else
	{
		// Patrol; a fresh point when there is none, on arrival, or re-routed every few seconds.
		bool needNew = !m_havePatrolTarget;
		if (!needNew)
		{
			const float ax = m_patrolTarget.x() - ex;
			const float az = m_patrolTarget.z() - ez;
			needNew = ax * ax + az * az < 1.2f * 1.2f;
		}
		if ((needNew || m_repathTimer <= 0.0f) && canPath())
		{
			m_repathTimer = 1.5f;
			if (needNew)
				m_havePatrolTarget = wanderPoint(enemyPosition, playerPosition, m_patrolTarget);
			if (m_havePatrolTarget)
				requestPath(enemyPosition, m_patrolTarget);
		}
	}

	float targetX, targetZ;
	steer(enemyPosition, targetX, targetZ);

	// Up close it faces what it chases; not chasing, it faces where it goes.
	const Vector4 targetPosition = target ? positionOf(target) : Vector4::origo();
	const Vector4* face = nullptr;
	if (m_state == State::Attack)
		face = target ? &targetPosition : &playerPosition;

	bool moving;
	if (!drive(enemyPosition, playerPosition, targetX, targetZ, speed, face, m_closeQuarters, target, deltaTime, moving))
		return;

	// Anything this close to the player is eating them, whatever it was doing.
	const bool biting = pdist < m_hurtRange;
	if (biting)
		m_npcWorld->hurtPlayer(m_enemyDps * deltaTime);

	bool striking = false;
	if (target && (targetPosition - enemyPosition).xyz0().length() < m_hurtRange)
	{
		striking = true;
		strike(target, deltaTime);
	}

	if (m_pose)
	{
		const bool walking = gait(enemyPosition, deltaTime, moving);
		poseState((biting || striking) ? Pose::Attack : (walking ? Pose::Walk : Pose::Idle));
	}

	voice(deltaTime, pdist);
}

std::wstring EnemyComponent::getState() const
{
	switch (m_state)
	{
	case State::Investigate:
		return L"investigate";
	case State::Attack:
		return L"attack";
	default:
		return L"patrol";
	}
}

void EnemyComponent::alertTo()
{
	readStats();
	alertFor(m_spawnHunt);
}

void EnemyComponent::hunt()
{
	alertTo();
	m_relentless = true;
}

void EnemyComponent::readSettings(const PropertyGroup* settings)
{
	const PropertyGroup* enemy = settingGroup(settings, L"enemy");
	const PropertyGroup* player = settingGroup(settings, L"player");

	// Drawn once, so an individual keeps a consistent gait.
	const float pace = settingNumber(enemy, L"paceMin", 1.0f) + m_random.nextFloat() * settingNumber(enemy, L"paceRange", 0.0f);
	m_patrolSpeed = settingNumber(enemy, L"patrolSpeed", 0.0f) * pace;
	m_attackSpeed = settingNumber(enemy, L"attackSpeed", 0.0f) * pace;
	m_investigateSpeed = settingNumber(enemy, L"investigateSpeed", 0.0f) * pace;
	m_topSpeed = m_attackSpeed;

	// Only the undead drift off their steering; the living walk where they mean to.
	m_wanderAngle = settingNumber(enemy, L"wanderAngle", 0.0f);
	m_wanderTime = settingNumber(enemy, L"wanderTime", 0.0f);

	m_sightRange = settingNumber(enemy, L"sightRange", 0.0f);
	m_attackMemory = settingNumber(enemy, L"attackMemory", 0.0f);
	m_spawnHunt = settingNumber(enemy, L"spawnHunt", 0.0f);

	m_frontDot = settingNumber(enemy, L"frontDot", 0.0f);
	m_frontReleaseDot = settingNumber(enemy, L"frontReleaseDot", 0.0f);
	m_frontCommit = settingNumber(enemy, L"frontCommit", 0.0f);
	m_frontRange = settingNumber(enemy, L"frontRange", 0.0f);
	m_chaseExactRange = settingNumber(enemy, L"chaseExactRange", 0.0f);
	m_frontRadius = settingNumber(enemy, L"frontRadius", 0.0f);
	m_frontStep = settingNumber(enemy, L"frontStep", 0.0f);
	m_frontArc = std::acos(m_frontDot);

	m_hearRange = settingNumber(enemy, L"hearRange", 0.0f);
	m_hearSpread = settingNumber(enemy, L"hearSpread", 0.0f);
	m_hearSpreadPerM = settingNumber(enemy, L"hearSpreadPerM", 0.0f);
	m_investigateArrive = settingNumber(enemy, L"investigateArrive", 0.0f);
	m_investigateTime = settingNumber(enemy, L"investigateTime", 0.0f);

	m_patrolLeash = settingNumber(enemy, L"patrolLeash", 0.0f);
	m_patrolPull = settingNumber(enemy, L"patrolPull", 1.0f);
	m_patrolStep = settingNumber(enemy, L"patrolStep", 0.0f);
	m_patrolBias = settingNumber(enemy, L"patrolBias", 0.0f);

	m_voiceRange = settingNumber(enemy, L"voiceRange", 0.0f);
	m_voiceCloseRange = settingNumber(enemy, L"voiceCloseRange", 0.0f);
	m_voiceIntervalMin = settingNumber(enemy, L"voiceIntervalMin", 0.0f);
	m_voiceIntervalMax = settingNumber(enemy, L"voiceIntervalMax", 0.0f);
	m_voiceIntervalClose = settingNumber(enemy, L"voiceIntervalClose", 0.0f);

	m_hurtRange = settingNumber(player, L"hurtRange", 1.0f);
	m_enemyDps = settingNumber(player, L"enemyDps", 0.0f);
}

void EnemyComponent::hurtBy(world::Entity* source)
{
	// No feelings to hurt; it simply turns on whoever it was.
	if (source == m_player || source == m_npcWorld->getPlayer())
	{
		if (m_state != State::Attack || m_target)
			alertFor(m_attackMemory);
		else
			m_lostTimer = std::max(m_lostTimer, m_attackMemory);
	}
	else if (source)
	{
		m_state = State::Attack;
		m_target = source;
		m_lostTimer = m_attackMemory;
		m_havePatrolTarget = false;
		m_repathTimer = 0.0f;
	}
}

void EnemyComponent::alertFor(float memory)
{
	m_state = State::Attack;
	m_target = nullptr;
	m_lostTimer = memory;
	m_havePatrolTarget = false;
	m_closing = false;
	m_repathTimer = 0.0f;
}

void EnemyComponent::investigate(float x, float z)
{
	m_state = State::Investigate;
	m_haveInvestigateTarget = true;
	m_investigateX = x;
	m_investigateZ = z;
	m_investigateTimer = m_investigateTime;
	m_havePatrolTarget = false;
	m_repathTimer = 0.0f;
}

void EnemyComponent::chaseAim(const Vector4& enemyPosition, const Vector4& playerPosition, float pdist, float& outX, float& outZ)
{
	const float qx = playerPosition.x();
	const float qz = playerPosition.z();
	outX = qx;
	outZ = qz;
	if (!m_playerCharacter || pdist < m_frontCommit || pdist > m_frontRange)
		return;

	// Bearing around the player, in the frame of where they are looking.
	const float look = m_playerCharacter->getHeadAngle();
	const float fx = std::sin(look);
	const float fz = std::cos(look);
	const float dx = enemyPosition.x() - qx;
	const float dz = enemyPosition.z() - qz;
	const float d = std::sqrt(dx * dx + dz * dz);
	if (d < 0.001f)
		return;
	const float ahead = (dx * fx + dz * fz) / d;
	const float side = (dx * fz - dz * fx) / d;

	// Committed, and staying committed until well round the back again.
	if (m_closing)
	{
		if (ahead > m_frontReleaseDot)
			return;
		m_closing = false;
	}
	else if (ahead >= m_frontDot)
	{
		m_closing = true;
		return;
	}

	// Still coming round; step the bearing towards the front, taking the ring in.
	const float offset = std::atan2(side, ahead);
	float turned = std::abs(offset) - m_frontStep;
	if (turned < m_frontArc)
		turned = m_frontArc;
	const float aim = look + (offset < 0.0f ? -turned : turned);
	const float radius = std::min(d, m_frontRadius);
	outX = qx + std::sin(aim) * radius;
	outZ = qz + std::cos(aim) * radius;
}

bool EnemyComponent::wanderPoint(const Vector4& enemyPosition, const Vector4& playerPosition, Vector4& outPoint) const
{
	const float ex = enemyPosition.x();
	const float ez = enemyPosition.z();
	float cx = ex, cz = ez;

	const float dx = playerPosition.x() - ex;
	const float dz = playerPosition.z() - ez;
	const float d = std::sqrt(dx * dx + dz * dz);
	if (d > m_patrolLeash)
	{
		const float pull = std::min((d - m_patrolLeash) / m_patrolPull, 1.0f);
		const float bias = m_patrolBias * pull;
		cx += (dx / d) * bias;
		cz += (dz / d) * bias;
	}

	if (randomPoint(Vector4(cx, enemyPosition.y(), cz, 1.0f), m_patrolStep, outPoint))
		return true;

	// The biased draw can miss the mesh on steep ground; the feet are on it by construction.
	return randomPoint(enemyPosition, m_patrolStep, outPoint);
}

void EnemyComponent::voice(float deltaTime, float pdist)
{
	if (!m_haveVoiceTimer)
	{
		// Staggered, or every zombie of a wave would speak on the same frame.
		m_voiceTimer = m_voiceIntervalMin + m_random.nextFloat() * m_voiceIntervalMax;
		m_haveVoiceTimer = true;
	}

	m_voiceTimer -= deltaTime;
	if (m_voiceTimer > 0.0f)
		return;

	const bool close = m_state == State::Attack && pdist < m_voiceCloseRange;
	m_voiceTimer = (close ? m_voiceIntervalClose : m_voiceIntervalMin) + m_random.nextFloat() * m_voiceIntervalMax;

	if (pdist > m_voiceRange)
		return;

	if (auto eventSet = m_owner->getComponent< world::EventSetComponent >())
		eventSet->raise(L"groan");
}

const Color4f& EnemyComponent::debugColor() const
{
	if (m_state == State::Investigate)
		return c_debugHeard;
	else if (m_state != State::Attack)
		return c_debugUnaware;
	else if (m_spotted)
		return c_debugSighted;
	else
		return c_debugMemory;
}

}

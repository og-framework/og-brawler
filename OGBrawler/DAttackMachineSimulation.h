#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/DAttackMachineSimulation-rationale.md · docs/DAttackMachineSimulation-guards.md

#include <vector>
#include "glm/vec3.hpp"
#include "glm/common.hpp"
#include <glm/gtc/quaternion.hpp>
#include "DAttackRadialSequence.h"
#include "DAttackRadialSimulation.h"
#include "OGBrawler/DAttackSequenceId.h"
#include "OGBrawler/HitReaction.h"
#include "OGBrawler/DAttackDirectionClassifier.h"
#include "OGBrawler/BrawlerProjectileSimulation.h"
#include "OGBrawler/BrawlerCharacterBindings.h"
#include "OGBrawler/BrawlerInboundHit.h"
#include "OGBrawler/InputSequence/InputSequence.h"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/OGAssert.h"
#include "OGBrawlerLog.h"

static constexpr float kHadoukenCommitmentSeconds = 0.3f;

static constexpr float kHitFlinchDuration = 0.3f;


#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

enum class DAttackState
{
	Attacking,
	Idle,
	GuardFlinch,
	HitFlinch,
};

inline constexpr unsigned char kDAttackStateCount = 4u;

class DAttackRadialSequence;

namespace dAttackMachineSimulation
{

class PlayerInput
{
public:
	glm::vec3 aimDirection{};
	bool attackLeft = false;
	bool attackRight = false;
	glm::vec2 moveDirection{};
	glm::vec3 moveDirectionWorld{};
	uint32_t triggeredActionId = 0;

	static PlayerInput zero()
	{
		// ⛔G-01  docs/DAttackMachineSimulation-guards.md
		return PlayerInput(glm::vec3(0.f, 0.f, 1.f), false, false, glm::vec2(0.f), glm::vec3(0.f));
	}
};

template <typename PhysicsAdapterType>
class IntegrationUtils
{
public:
	IntegrationUtils(float deltaTime,
		uint32_t currentTick,
		const std::vector<DAttackRadialSequence>& attackSequences,
		PhysicsAdapterType& physicsAdapter,
		const brawlerProjectileSimulation::StaticData& projectileStaticData)
		: deltaTime(deltaTime)
		, m_currentTick(currentTick)
		, attackSequences(attackSequences)
		, m_physicsAdapter(physicsAdapter)
		, m_projectileStaticData(projectileStaticData)
	{}

	float getDeltaTime() const { return deltaTime; }
	uint32_t getCurrentTick() const { return m_currentTick; }
	const std::vector<DAttackRadialSequence>& getAttackSequences() const { return attackSequences; }
	PhysicsAdapterType& getPhysicsAdapter() const { return m_physicsAdapter; }
	const brawlerProjectileSimulation::StaticData& getProjectileStaticData() const { return m_projectileStaticData; }

private:
	float deltaTime;
	uint32_t m_currentTick;
	const std::vector<DAttackRadialSequence>& attackSequences;
	PhysicsAdapterType& m_physicsAdapter;
	const brawlerProjectileSimulation::StaticData& m_projectileStaticData;
};

inline uint32_t swingTickCount(float duration, float dt)
{
	OG_CHECK(duration > 0.f && dt > 0.f,
		"dAttackMachineSimulation::swingTickCount - a non-positive duration or step would not "
		"terminate. `duration` comes from DAttackRadialSequence::getDuration() (authored data, "
		"plus the appended zero-velocity point) and `dt` from the fixed sim step; a zero in "
		"either is a construction error, not wire input.");

	float t = 0.f;
	uint32_t k = 0u;
	while (t < duration)
	{
		// ⛔G-02  docs/DAttackMachineSimulation-guards.md
		t = t + dt;
		++k;
	}
	return k;
}

template <typename PhysicsAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsAdapterType>>;

class State
{
public:
	DAttackState m_currentState = DAttackState::Idle;
	float m_timeInCurrentState = 0.f;
	unsigned int m_activeAttackSequence = InvalidAttackSequenceId;
	unsigned int m_queuedAttackSequence = InvalidAttackSequenceId;
	HitReactionKind m_hitReaction = HitReactionKind::Stun;
	float m_flinchDuration = kHitFlinchDuration;
	uint32_t m_attackEndTick = 0u;
};

struct Dependencies {
	using Owned = OwnedDeps<dAttackMachineSimulation::State>;
	using External = ExternalDeps<
		const dAttackRadialSimulation::State&,
		dAttackRadialSimulation::InitialConditions&,
		brawlerProjectileSimulation::InitialConditions&>;
	using InputType = dAttackMachineSimulation::PlayerInput;
	Owned owned;
	External external;
};

namespace {

template <typename PhysicsAdapterType>
void setRadialSimulationInitialConditions(float deltaTime,
	const AllInput<PhysicsAdapterType>& input,
	dAttackRadialSimulation::InitialConditions& attackIntialConditions,
	State& state)
{
	attackIntialConditions.activeAttackSequence = state.m_activeAttackSequence;

	const glm::vec3 aimDirection = glm::normalize(glm::vec3(input.getPlayerInput().aimDirection.x, input.getPlayerInput().aimDirection.y, 0.f));
	const glm::vec3 defaultForward(1.f, 0.f, 0.f);
	const glm::vec3 defaultUp(0.f, 0.f, 1.f);
	const float aimDot = glm::dot(aimDirection, defaultForward);
	const float aimAngle = glm::acos(aimDot);
	// ⛔G-03  docs/DAttackMachineSimulation-guards.md
	const bool aimEqualsForward = glm::abs(glm::abs(aimDot) - 1.f) < 0.0001f;
	const glm::vec3 aimRotationAxis = [&aimEqualsForward, &defaultUp, &defaultForward, &aimDirection]() {
		if (aimEqualsForward)
			return defaultUp;
		else
			return glm::normalize(glm::cross(defaultForward, aimDirection));
	}();

	attackIntialConditions.initialAimAngle = aimAngle;
	attackIntialConditions.initialAimRotationAxis = aimRotationAxis;
}
}

template <typename PhysicsAdapterType>
void integrate(float deltaTime, 
	const AllInput<PhysicsAdapterType>& input,
	const dAttackRadialSimulation::State& attackState, 
	dAttackRadialSimulation::InitialConditions& attackIntialConditions,  
	State& state)
{
	state.m_timeInCurrentState += deltaTime;

	const PlayerInput& playerInput = input.getPlayerInput();

	const bool inboundHit_PLACEHOLDER = false;
	if (inboundHit_PLACEHOLDER && state.m_currentState != DAttackState::HitFlinch)
	{
		state.m_currentState = DAttackState::HitFlinch; state.m_timeInCurrentState = 0.f;
		state.m_activeAttackSequence = InvalidAttackSequenceId;
		state.m_queuedAttackSequence = InvalidAttackSequenceId;
		attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
	}

	switch (state.m_currentState)
	{
	case DAttackState::Idle:
	{
		if (playerInput.attackLeft && playerInput.attackRight)
		{
			state.m_activeAttackSequence = 4;
		}
		else if (playerInput.attackLeft)
		{
			state.m_activeAttackSequence = 0;
		}
		else if (playerInput.attackRight)
		{
			state.m_activeAttackSequence = 1;
		}
		
		if (state.m_activeAttackSequence != InvalidAttackSequenceId)
		{
			state.m_currentState = DAttackState::Attacking; state.m_timeInCurrentState = 0.f;

			setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
		}

		break;
	}
	case DAttackState::Attacking:
	{
		const bool guardBlocked_PLACEHOLDER = false;
		if (guardBlocked_PLACEHOLDER)
		{
			state.m_currentState = DAttackState::GuardFlinch; state.m_timeInCurrentState = 0.f;
			state.m_activeAttackSequence = InvalidAttackSequenceId;
			state.m_queuedAttackSequence = InvalidAttackSequenceId;
			attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
		}

		if (playerInput.attackLeft && playerInput.attackRight)
		{
			if (attackState.attackTimer < 0.1)
			{
				state.m_queuedAttackSequence = InvalidAttackSequenceId;
				state.m_activeAttackSequence = 4;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}
			else if (attackState.attackTimer > 0.3)
			{
				state.m_queuedAttackSequence = 4;
			}
		}

		if (attackState.attackTimer > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
		{
			if (playerInput.attackLeft && (attackIntialConditions.activeAttackSequence == 0 || attackIntialConditions.activeAttackSequence == 2))
				state.m_queuedAttackSequence = 2;
			if (playerInput.attackRight && (attackIntialConditions.activeAttackSequence == 1 || attackIntialConditions.activeAttackSequence == 3))
				state.m_queuedAttackSequence = 3;

		}

		if (attackState.currenSequenceId == InvalidAttackSequenceId)
		{
			if (state.m_queuedAttackSequence != InvalidAttackSequenceId)
			{
				state.m_activeAttackSequence = state.m_queuedAttackSequence;
				state.m_queuedAttackSequence = InvalidAttackSequenceId;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}
			else
			{
				state.m_activeAttackSequence = InvalidAttackSequenceId;
				state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
				attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
			}
		}

		break;
	}
	case DAttackState::GuardFlinch:
	{
		const float guardFlinchDuration = 0.3f;
		if (state.m_timeInCurrentState > guardFlinchDuration)
		{
			state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
		}
		break;
	}
	case DAttackState::HitFlinch:
	{
		if (state.m_timeInCurrentState > state.m_flinchDuration)
		{
			state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
		}
		break;
	}
	default:
		break;
	}
}



template <typename PhysicsAdapterType>
void integrate2(float deltaTime,
	const AllInput<PhysicsAdapterType>& input,
	const dAttackRadialSimulation::State& attackState,
	dAttackRadialSimulation::InitialConditions& attackIntialConditions,
	State& state)
{
	state.m_timeInCurrentState += deltaTime;

	const PlayerInput& playerInput = input.getPlayerInput();

	const bool inboundHit_PLACEHOLDER = false;
	if (inboundHit_PLACEHOLDER && state.m_currentState != DAttackState::HitFlinch)
	{
		state.m_currentState = DAttackState::HitFlinch; state.m_timeInCurrentState = 0.f;
		state.m_activeAttackSequence = InvalidAttackSequenceId;
		state.m_queuedAttackSequence = InvalidAttackSequenceId;
		attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
	}

	switch (state.m_currentState)
	{
	case DAttackState::Idle:
	{
		if (playerInput.attackLeft && playerInput.moveDirection.y > 0.5f)
		{
			state.m_activeAttackSequence = 4;
		}
		else if (playerInput.attackLeft && playerInput.moveDirection.x > 0.5f)
		{
			state.m_activeAttackSequence = 0;
		}
		else if (playerInput.attackLeft && playerInput.moveDirection.x < -0.5f)
		{
			state.m_activeAttackSequence = 1;
		}

		if (state.m_activeAttackSequence != InvalidAttackSequenceId)
		{
			state.m_currentState = DAttackState::Attacking; state.m_timeInCurrentState = 0.f;

			setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
		}

		break;
	}
	case DAttackState::Attacking:
	{
		const bool guardBlocked_PLACEHOLDER = false;
		if (guardBlocked_PLACEHOLDER)
		{
			state.m_currentState = DAttackState::GuardFlinch; state.m_timeInCurrentState = 0.f;
			state.m_activeAttackSequence = InvalidAttackSequenceId;
			state.m_queuedAttackSequence = InvalidAttackSequenceId;
			attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
		}

		if (playerInput.attackLeft && playerInput.attackRight)
		{
			if (attackState.attackTimer < 0.1)
			{
				state.m_queuedAttackSequence = InvalidAttackSequenceId;
				state.m_activeAttackSequence = 4;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}
			else if (attackState.attackTimer > 0.3)
			{
				state.m_queuedAttackSequence = 4;
			}
		}

		if (attackState.attackTimer > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
		{
			if (playerInput.attackLeft && (attackIntialConditions.activeAttackSequence == 0 || attackIntialConditions.activeAttackSequence == 2))
				state.m_queuedAttackSequence = 2;
			if (playerInput.attackRight && (attackIntialConditions.activeAttackSequence == 1 || attackIntialConditions.activeAttackSequence == 3))
				state.m_queuedAttackSequence = 3;

		}

		if (attackState.currenSequenceId == InvalidAttackSequenceId)
		{
			if (state.m_queuedAttackSequence != InvalidAttackSequenceId)
			{
				state.m_activeAttackSequence = state.m_queuedAttackSequence;
				state.m_queuedAttackSequence = InvalidAttackSequenceId;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}
			else
			{
				state.m_activeAttackSequence = InvalidAttackSequenceId;
				state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
				attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
			}
		}

		break;
	}
	case DAttackState::GuardFlinch:
	{
		const float guardFlinchDuration = 0.3f;
		if (state.m_timeInCurrentState > guardFlinchDuration)
		{
			state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
		}
		break;
	}
	case DAttackState::HitFlinch:
	{
		if (state.m_timeInCurrentState > state.m_flinchDuration)
		{
			state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
		}
		break;
	}
	default:
		break;
	}
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(error: 4062)
#endif
constexpr const char* dAttackStateName(DAttackState s)
{
	switch (s)
	{
		case DAttackState::Idle:        return "Idle";
		case DAttackState::Attacking:   return "Attacking";
		case DAttackState::GuardFlinch: return "GuardFlinch";
		case DAttackState::HitFlinch:   return "HitFlinch";
	}
	return "?";
}
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

static_assert([] {
	for (int i = 0; i < kDAttackStateCount; ++i)
		if (dAttackStateName(static_cast<DAttackState>(i))[0] == '?')
			return false;
	return dAttackStateName(static_cast<DAttackState>(kDAttackStateCount))[0] == '?';
}(), "kDAttackStateCount must equal the number of DAttackState enumerators, and dAttackStateName "
	"must name every one ('?' is only the out-of-range fallback): add a state, its name and the "
	"count together. C4062 is an error on MSVC around dAttackStateName, so an unnamed state fails "
	"there first. Replaces the comment that kept the count ADJACENT to the enum so a missed bump "
	"would be visible.");

static_assert([] {
	for (int i = 0; i <= kDAttackStateCount; ++i)
	{
		std::size_t length = 0;
		for (const char* c = dAttackStateName(static_cast<DAttackState>(i)); *c != '\0'; ++c)
			++length;
		if (length > ::ogblog::kMaxStringArgBytes)
			return false;
	}
	return true;
}(), "a dAttackStateName exceeds ogblog::kMaxStringArgBytes; the OGBLOG_G clip check no longer bounds the [Machine.*] lines that print it");

template <typename PhysicsAdapterType>
void integrate3(float deltaTime,
	const AllInput<PhysicsAdapterType>& input,
	Dependencies deps,
	const simulatableBrawler::CharacterBindings& characterBindings,
	const brawlerInboundHit::DerivedState& inboundHit)
{
	const dAttackRadialSimulation::State& attackState = deps.external.get<dAttackRadialSimulation::State>();
	dAttackRadialSimulation::InitialConditions& attackIntialConditions = deps.external.edit<dAttackRadialSimulation::InitialConditions>();
	State& state = deps.owned.edit<State>();

	state.m_timeInCurrentState += deltaTime;

	const PlayerInput& playerInput = input.getPlayerInput();

	OGBLOG_G("[Machine.integrate] state=%s activeSeq=%u queuedSeq=%u attackState.curSeq=%u attackState.timer=%.4f L=%d R=%d",
		dAttackStateName(state.m_currentState), state.m_activeAttackSequence, state.m_queuedAttackSequence,
		attackState.currenSequenceId, attackState.attackTimer,
		playerInput.attackLeft ? 1 : 0, playerInput.attackRight ? 1 : 0);

	if (inboundHit.wasHitThisTick)
	{
		OGBLOG_G("[Machine.transition] %s -> HitFlinch (inbound hit, reaction=%u dwell=%.4f)",
			dAttackStateName(state.m_currentState),
			static_cast<unsigned int>(inboundHit.reactionKind), inboundHit.flinchDuration);
		state.m_currentState = DAttackState::HitFlinch; state.m_timeInCurrentState = 0.f;
		state.m_hitReaction = inboundHit.reactionKind;
		state.m_flinchDuration = inboundHit.flinchDuration;
		state.m_activeAttackSequence = InvalidAttackSequenceId;
		state.m_queuedAttackSequence = InvalidAttackSequenceId;
		attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
	}

	// ⛔G-04  docs/DAttackMachineSimulation-guards.md
	if (inboundHit.wasProjectileBlockedThisTick && state.m_currentState != DAttackState::GuardFlinch)
	{
		OGBLOG_G("[Machine.transition] %s -> GuardFlinch (projectile blocked)", dAttackStateName(state.m_currentState));
		state.m_currentState = DAttackState::GuardFlinch; state.m_timeInCurrentState = 0.f;
		state.m_activeAttackSequence = InvalidAttackSequenceId;
		state.m_queuedAttackSequence = InvalidAttackSequenceId;
		attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
	}

	switch (state.m_currentState)
	{
	case DAttackState::Idle:
	{
		if (playerInput.triggeredActionId == inputSequence::kHadoukenActionId)
		{
			brawlerProjectileSimulation::InitialConditions& projectileIC =
				deps.external.edit<brawlerProjectileSimulation::InitialConditions>();
			const brawlerProjectileSimulation::StaticData& projSD =
				input.getIntegrationUtils().getProjectileStaticData();
			const glm::vec3 parentPosition = glm::vec3(
				input.getIntegrationUtils().getPhysicsAdapter().getBodyTransform(
					characterBindings.capsuleBodyId)[3]);

			const glm::vec3 aimXYraw(playerInput.aimDirection.x, playerInput.aimDirection.y, 0.f);
			const glm::vec3 aimXY = (glm::length(aimXYraw) < 0.0001f)
				? glm::vec3(1.f, 0.f, 0.f)
				: glm::normalize(aimXYraw);

			projectileIC.spawnRequestPending = 1;
			projectileIC.spawnPos = parentPosition
				+ aimXY * projSD.spawnForwardOffset
				+ glm::vec3(0.f, 0.f, projSD.spawnZOffset);
			projectileIC.spawnDir = aimXY;

			state.m_activeAttackSequence = kHadoukenSequenceSentinel;
			attackIntialConditions.activeAttackSequence = kHadoukenSequenceSentinel;
			state.m_currentState = DAttackState::Attacking;
			state.m_timeInCurrentState = 0.f;
			{
				const uint32_t currentTick = input.getIntegrationUtils().getCurrentTick();
				// ∴D-01  docs/DAttackMachineSimulation-rationale.md
				state.m_attackEndTick =
					currentTick + swingTickCount(kHadoukenCommitmentSeconds, deltaTime);
				OG_CHECK(state.m_attackEndTick > currentTick,
					"dAttackMachineSimulation::integrate3 - the Hadouken commitment must be at "
					"least one tick long; swingTickCount returns at least 1 for any positive "
					"duration, so this can only fire if kHadoukenCommitmentSeconds went "
					"non-positive.");
				OGBLOG_G("[Machine.transition] Idle -> Attacking (Hadouken, projectile spawn "
					"requested) endTick=%u", state.m_attackEndTick);
			}
			break;
		}

		if(playerInput.attackLeft || playerInput.attackRight)
		{
			// ⛔G-05  docs/DAttackMachineSimulation-guards.md
			state.m_activeAttackSequence = dAttackDirection::classify(
				playerInput.aimDirection,
				glm::vec3(playerInput.moveDirectionWorld),
				glm::vec2(playerInput.moveDirection));

			if (state.m_activeAttackSequence != InvalidAttackSequenceId)
			{
				const uint32_t currentTick = input.getIntegrationUtils().getCurrentTick();
				// ∴D-02  docs/DAttackMachineSimulation-rationale.md
				state.m_attackEndTick = currentTick + swingTickCount(
					input.getIntegrationUtils()
						.getAttackSequences()[state.m_activeAttackSequence].getDuration(),
					deltaTime) + 1u;
				OG_CHECK(state.m_attackEndTick > currentTick,
					"dAttackMachineSimulation::integrate3 - the attack end tick must be in the "
					"future on the tick it is written (swingTickCount returns at least 1 for any "
					"positive duration). A value at or before the current tick reaches the movement "
					"sub-sim as a stopped slide.");
				OGBLOG_G("[Machine.transition] Idle -> Attacking seq=%u endTick=%u",
					state.m_activeAttackSequence, state.m_attackEndTick);
				state.m_currentState = DAttackState::Attacking; state.m_timeInCurrentState = 0.f;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}

		}

		break;
	}
	case DAttackState::Attacking:
	{
		if (inboundHit.wasGuardBlockedThisTick)
		{
			OGBLOG_G("[Machine.transition] Attacking -> GuardFlinch (guard blocked)");
			state.m_currentState = DAttackState::GuardFlinch; state.m_timeInCurrentState = 0.f;
			state.m_activeAttackSequence = InvalidAttackSequenceId;
			state.m_queuedAttackSequence = InvalidAttackSequenceId;
			attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
		}

		if (playerInput.attackLeft && playerInput.attackRight)
		{
			if (attackState.attackTimer < 0.1)
			{
				// ⛔G-06  docs/DAttackMachineSimulation-guards.md

				OGBLOG_G("[Machine.Attacking] dualtap restart seq=4 (timer<0.1) endTick=%u (unchanged)",
					state.m_attackEndTick);
				state.m_queuedAttackSequence = InvalidAttackSequenceId;
				state.m_activeAttackSequence = 4;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}
			else if (attackState.attackTimer > 0.3)
			{
				OGBLOG_G("[Machine.Attacking] queue seq=4 (dual,timer>0.3)");
				state.m_queuedAttackSequence = 4;
			}
		}

		if (attackState.attackTimer > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
		{
			if (playerInput.attackLeft && (attackIntialConditions.activeAttackSequence == 0 || attackIntialConditions.activeAttackSequence == 2))
			{
				OGBLOG_G("[Machine.Attacking] queue seq=2 (L,timer>0.3,ic=%u)", attackIntialConditions.activeAttackSequence);
				state.m_queuedAttackSequence = 2;
			}
			if (playerInput.attackRight && (attackIntialConditions.activeAttackSequence == 1 || attackIntialConditions.activeAttackSequence == 3))
			{
				OGBLOG_G("[Machine.Attacking] queue seq=3 (R,timer>0.3,ic=%u)", attackIntialConditions.activeAttackSequence);
				state.m_queuedAttackSequence = 3;
			}
		}

		const bool inHadoukenCommitment =
			state.m_activeAttackSequence == kHadoukenSequenceSentinel
			&& state.m_timeInCurrentState < kHadoukenCommitmentSeconds;

		if (!inHadoukenCommitment && attackState.currenSequenceId == InvalidAttackSequenceId)
		{
			if (state.m_queuedAttackSequence != InvalidAttackSequenceId)
			{
				state.m_activeAttackSequence = state.m_queuedAttackSequence;
				state.m_queuedAttackSequence = InvalidAttackSequenceId;
				const uint32_t currentTick = input.getIntegrationUtils().getCurrentTick();
				// ⛔G-07  docs/DAttackMachineSimulation-guards.md
				state.m_attackEndTick = currentTick + swingTickCount(
					input.getIntegrationUtils()
						.getAttackSequences()[state.m_activeAttackSequence].getDuration(),
					deltaTime) + 1u;
				OG_CHECK(state.m_attackEndTick > currentTick,
					"dAttackMachineSimulation::integrate3 - the chained attack's end tick must be "
					"in the future on the tick it is written, for the same reason the Idle entry's "
					"must.");
				OGBLOG_G("[Machine.transition] Attacking -> Attacking (queued seq=%u) endTick=%u",
					state.m_activeAttackSequence, state.m_attackEndTick);

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}
			else
			{
				OGBLOG_G("[Machine.transition] Attacking -> Idle (radial finished, no queued)");
				state.m_activeAttackSequence = InvalidAttackSequenceId;
				state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
				attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
			}
		}

		break;
	}
	case DAttackState::GuardFlinch:
	{
		const float guardFlinchDuration = 0.3f;
		if (state.m_timeInCurrentState > guardFlinchDuration)
		{
			OGBLOG_G("[Machine.transition] GuardFlinch -> Idle");
			state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
		}
		break;
	}
	case DAttackState::HitFlinch:
	{
		if (state.m_timeInCurrentState > state.m_flinchDuration)
		{
			OGBLOG_G("[Machine.transition] HitFlinch -> Idle");
			state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
		}
		break;
	}
	default:
		break;
	}
}

}

template <>
struct SerializableFields<dAttackMachineSimulation::State>
{
	static constexpr auto get()
	{
		using S = dAttackMachineSimulation::State;
		return std::make_tuple(
			SIM_MEMBER(S, m_currentState),
			SIM_MEMBER(S, m_timeInCurrentState),
			SIM_MEMBER(S, m_activeAttackSequence),
			SIM_MEMBER(S, m_queuedAttackSequence),
			SIM_MEMBER(S, m_hitReaction),
			SIM_MEMBER(S, m_flinchDuration),
			SIM_MEMBER(S, m_attackEndTick));
	}
};

template <>
struct SerializableFields<dAttackMachineSimulation::PlayerInput>
{
	static constexpr auto get()
	{
		return std::make_tuple(
			MemberFieldDesc<&dAttackMachineSimulation::PlayerInput::aimDirection>{},
			MemberFieldDesc<&dAttackMachineSimulation::PlayerInput::attackLeft>{},
			MemberFieldDesc<&dAttackMachineSimulation::PlayerInput::attackRight>{},
			MemberFieldDesc<&dAttackMachineSimulation::PlayerInput::moveDirection>{},
			MemberFieldDesc<&dAttackMachineSimulation::PlayerInput::moveDirectionWorld>{},
			MemberFieldDesc<&dAttackMachineSimulation::PlayerInput::triggeredActionId>{});
	}
};

static_assert(SimulationState<dAttackMachineSimulation::State>);
static_assert(SimulationInput<dAttackMachineSimulation::PlayerInput>);

OGSIM_OPTIMIZE_ON




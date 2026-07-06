#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include "DAttackRadialSequence.h"
#include "DAttackRadialSimulation.h"
#include "OGBrawler/DAttackSequenceId.h"
#include "OGBrawler/BrawlerProjectileSimulation.h"
#include "OGBrawler/BrawlerMovementSimulation.h"
// [hit-resolution T2] brawlerInboundHit::DerivedState — read by integrate3 as a plain
// by-ref param (NOT an ExternalDep, see current_state.md §D7). Zero-dependency header,
// no include cycle.
#include "OGBrawler/BrawlerInboundHit.h"
#include "OGBrawler/InputSequence/InputSequence.h"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGBrawlerLog.h"

// [Task 25] Hadouken commitment duration. When integrate3 fires a Hadouken it transitions
// Idle -> Attacking with the kHadoukenSequenceSentinel active (the sentinel lives at file
// scope in DAttackRadialSimulation.h, included above). Without a commitment window the
// machine exits Attacking -> Idle one tick later (the radial early-returns and leaves
// currenSequenceId == InvalidAttackSequenceId), which lets a still-held attack button chain
// an immediate normal swing. This minimum dwell (0.3 s ≈ 18 ticks at 60 Hz) keeps the
// machine in Attacking for the projectile cast before the normal exit-to-Idle gate fires.
static constexpr float kHadoukenCommitmentSeconds = 0.3f;

// [hit-resolution T1] Minimum dwell for the target-side HitFlinch state. When an inbound hit
// signal arrives (T2 threads the real External; T1 gates on a false placeholder), the machine
// transitions Idle/Attacking -> HitFlinch and stays there until m_timeInCurrentState exceeds this
// window, then returns to Idle. Mirrors the existing GuardFlinch duration (0.3 s ≈ 18 ticks at
// 60 Hz). File-scope constant matches the kHadoukenCommitmentSeconds precedent above; the eventual
// lift into DAttackMachineSimulationRuntimeTweakables.h is R-P1 cleanup tracked separately.
static constexpr float kHitFlinchDuration = 0.3f;


#pragma optimize( "", off )

enum class DAttackState
{
	Attacking,
	Idle,
	GuardFlinch,
	HitFlinch,
};

class DAttackRadialSequence;

namespace dAttackMachineSimulation
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PlayerInput
{
public:
	// [Task 43] Plain aggregate — const dropped so MemberFieldDesc::write() can assign.
	glm::vec3 aimDirection{};
	bool attackLeft = false;
	bool attackRight = false;
	glm::vec2 moveDirection{};
	glm::vec3 moveDirectionWorld{};
	// Set by the input-layer motion matcher (buildPlayerInput) to inputSequence::kHadoukenActionId
	// on the tick a Hadouken sequence completes; 0 otherwise. Appended last so the existing
	// 5-arg aggregate-init call sites keep compiling (C++20 parenthesized aggregate init
	// defaults this to 0). Travels through the PlayerInput RPC like any other input field.
	uint32_t triggeredActionId = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsAdapterType>
class IntegrationUtils
{
public:
	IntegrationUtils(float deltaTime,
		const std::vector<DAttackRadialSequence>& attackSequences,
		PhysicsAdapterType& physicsAdapter,
		const brawlerProjectileSimulation::StaticData& projectileStaticData)
		: deltaTime(deltaTime)
		, attackSequences(attackSequences)
		, m_physicsAdapter(physicsAdapter)
		, m_projectileStaticData(projectileStaticData)
	{}

	float getDeltaTime() const { return deltaTime; }
	PhysicsAdapterType& getPhysicsAdapter() const { return m_physicsAdapter; }
	// Projectile launch parameters — needed by the Hadouken trigger block in integrate3 to
	// write the projectile InitialConditions. The parent capsule position is no longer
	// pre-resolved here (T33): integrate3 looks it up on-demand via the physics adapter from
	// the CharacterBindings handle, matching the bindings-as-integrate-param pattern radial/
	// guard/projectile already use.
	const brawlerProjectileSimulation::StaticData& getProjectileStaticData() const { return m_projectileStaticData; }

private:
	float deltaTime;
	const std::vector<DAttackRadialSequence>& attackSequences;
	PhysicsAdapterType& m_physicsAdapter;
	const brawlerProjectileSimulation::StaticData& m_projectileStaticData;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsAdapterType>>;

class State
{
public:
	DAttackState m_currentState = DAttackState::Idle;
	float m_timeInCurrentState = 0.f;
	unsigned int m_activeAttackSequence = InvalidAttackSequenceId;
	unsigned int m_queuedAttackSequence = InvalidAttackSequenceId;
};

// [Task 62] Dependencies — OwnedDeps/ExternalDeps layout.
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
	attackIntialConditions.activeRootBodyId = 0;

	const glm::vec3 aimDirection = glm::normalize(glm::vec3(input.getPlayerInput().aimDirection.x, input.getPlayerInput().aimDirection.y, 0.f));
	const glm::vec3 defaultForward(1.f, 0.f, 0.f);
	const glm::vec3 defaultUp(0.f, 0.f, 1.f);
	const float aimDot = glm::dot(aimDirection, defaultForward);
	const float aimAngle = glm::acos(aimDot);
	const bool aimEqualsForward = abs(abs(aimDot) - 1.f) < 0.0001f;
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

	// [hit-resolution T1] Inbound-hit veto. Placeholder signal (always false until T2 threads the
	// real brawlerInboundHit::DerivedState External). Read at the top of the integrate body — before
	// case dispatch and before any attack-input handling — so a live signal vetoes attack inputs on
	// the hit tick. On a live hit we cancel the active/queued sequences (mirroring the GuardFlinch
	// cancellation) and drop into HitFlinch; the switch below then lands in the HitFlinch case with
	// m_timeInCurrentState freshly reset.
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
		if (attackState.hasHitGuard)
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
			else if (attackState.attackTimer/*!sic*/ > 0.3)
			{
				state.m_queuedAttackSequence = 4;
			}
		}

		if (attackState.attackTimer/*!sic*/ > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
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
		// [hit-resolution T1] Mirrors GuardFlinch: dwell for kHitFlinchDuration, no attack-input
		// reads (gating is automatic — the switch never reaches Idle/Attacking while flinching),
		// then return to Idle.
		if (state.m_timeInCurrentState > kHitFlinchDuration)
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

	// [hit-resolution T1] Inbound-hit veto. Placeholder signal (always false until T2 threads the
	// real brawlerInboundHit::DerivedState External). Read at the top of the integrate body — before
	// case dispatch and before any attack-input handling — so a live signal vetoes attack inputs on
	// the hit tick. On a live hit we cancel the active/queued sequences (mirroring the GuardFlinch
	// cancellation) and drop into HitFlinch; the switch below then lands in the HitFlinch case with
	// m_timeInCurrentState freshly reset.
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
		if (attackState.hasHitGuard)
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
			else if (attackState.attackTimer/*!sic*/ > 0.3)
			{
				state.m_queuedAttackSequence = 4;
			}
		}

		if (attackState.attackTimer/*!sic*/ > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
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
		// [hit-resolution T1] Mirrors GuardFlinch: dwell for kHitFlinchDuration, no attack-input
		// reads (gating is automatic — the switch never reaches Idle/Attacking while flinching),
		// then return to Idle.
		if (state.m_timeInCurrentState > kHitFlinchDuration)
		{
			state.m_currentState = DAttackState::Idle; state.m_timeInCurrentState = 0.f;
		}
		break;
	}
	default:
		break;
	}
}

inline const char* dAttackStateName(DAttackState s)
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

// [Task 35] CharacterBindings now lives in BrawlerMovementSimulation.h, included above with no
// include cycle, so integrate3 takes a plain const reference (the T33 templated workaround is
// gone). The Hadouken trigger resolves the parent capsule transform on-demand from the bindings
// handle — matching the bindings-as-integrate-param pattern radial/guard/projectile already use.
template <typename PhysicsAdapterType>
void integrate3(float deltaTime,
	const AllInput<PhysicsAdapterType>& input,
	Dependencies deps,
	const brawlerMovementSimulation::CharacterBindings& characterBindings,
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

	// [hit-resolution T2] Inbound-hit veto. Real signal read from the plain by-ref parameter
	// (T1's compile-time-false placeholder is gone). inboundHit is a per-character
	// brawlerInboundHit::DerivedState slice on the composite DerivedState, populated by the
	// manager's routing pass (T3) on the prior tick. It is passed as a plain integrate3 param
	// (NOT via deps.external) because it lives on the DerivedState composite, not the serialized
	// State composite — see current_state.md §D7. Sits AHEAD of the switch — and therefore ahead
	// of the Idle case's Hadouken trigger and attack-input handling — so a live signal vetoes both
	// attack inputs and the Hadouken trigger on the hit tick. On a live hit we cancel the
	// active/queued sequences (mirroring the Attacking -> GuardFlinch cancellation) and drop into
	// HitFlinch; the switch below then lands in the HitFlinch case with m_timeInCurrentState
	// freshly reset.
	if (inboundHit.wasHitThisTick && state.m_currentState != DAttackState::HitFlinch)
	{
		OGBLOG_G("[Machine.transition] %s -> HitFlinch (inbound hit)", dAttackStateName(state.m_currentState));
		state.m_currentState = DAttackState::HitFlinch; state.m_timeInCurrentState = 0.f;
		state.m_activeAttackSequence = InvalidAttackSequenceId;
		state.m_queuedAttackSequence = InvalidAttackSequenceId;
		attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
	}

	// [hit-resolution T15] Shooter-side GuardFlinch from a blocked projectile.
	// The manager routing pass sets wasProjectileBlockedThisTick=true on THIS character
	// (the shooter) when any of its projectile slots ended the prior tick with
	// endReason=4 (blockedByGuard, per T14). Fires the same recoil the radial swing's
	// attacker-side hasHitGuard path produces (see the switch cases below). Unlike the
	// hasHitGuard path, this one intentionally fires from any origin state — including
	// Idle — because a projectile can be blocked long after the shooter's Hadouken
	// commitment window has expired and they've returned to Idle. Same cancellation
	// as the HitFlinch veto above: active/queued sequences cleared so the switch below
	// lands in the GuardFlinch case with m_timeInCurrentState freshly reset. The
	// `!= GuardFlinch` guard prevents re-transition if the character is already
	// flinching (rapid successive blocks coalesce to a single flinch window).
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
		// Hadouken trigger: the input-layer motion matcher (buildPlayerInput) sets
		// triggeredActionId to kHadoukenActionId on the tick a motion completes. Spawn a
		// projectile in the aim direction and hand the radial weapon a sentinel sequence so
		// it stays in its idle pose. This sits AHEAD of the plain attackLeft/attackRight
		// handling so a matched motion takes priority over the idle swing on the same tick.
		if (playerInput.triggeredActionId == inputSequence::kHadoukenActionId)
		{
			brawlerProjectileSimulation::InitialConditions& projectileIC =
				deps.external.edit<brawlerProjectileSimulation::InitialConditions>();
			const brawlerProjectileSimulation::StaticData& projSD =
				input.getIntegrationUtils().getProjectileStaticData();
			// [Task 33] Resolve the parent capsule position on-demand from the CharacterBindings
			// handle, instead of receiving it pre-resolved via IntegrationUtils. This matches the
			// radial/guard/projectile pattern (bindings passed to integrate, physics queried inside).
			const glm::vec3 parentPosition = glm::vec3(
				input.getIntegrationUtils().getPhysicsAdapter().getBodyTransform(
					characterBindings.capsuleBodyId)[3]);

			// XY-projected aim, with a degenerate-aim fallback to avoid a NaN from normalize.
			const glm::vec3 aimXYraw(playerInput.aimDirection.x, playerInput.aimDirection.y, 0.f);
			const glm::vec3 aimXY = (glm::length(aimXYraw) < 0.0001f)
				? glm::vec3(1.f, 0.f, 0.f)
				: glm::normalize(aimXYraw);

			// Closed-form launch parameters (Task 13): write spawnDir, NOT velocity — the
			// projectile sim derives velocity = spawnDir * projectileSpeed each tick.
			projectileIC.spawnRequestPending = 1;
			projectileIC.spawnPos = parentPosition
				+ aimXY * projSD.spawnForwardOffset
				+ glm::vec3(0.f, 0.f, projSD.spawnZOffset);
			projectileIC.spawnDir = aimXY;

			// Hand the radial weapon the sentinel so its integrate early-returns (idle pose)
			// instead of indexing attackSequences[] out of bounds.
			state.m_activeAttackSequence = kHadoukenSequenceSentinel;
			attackIntialConditions.activeAttackSequence = kHadoukenSequenceSentinel;
			state.m_currentState = DAttackState::Attacking;
			state.m_timeInCurrentState = 0.f;
			OGBLOG_G("[Machine.transition] Idle -> Attacking (Hadouken, projectile spawn requested)");
			break;
		}

		if(playerInput.attackLeft || playerInput.attackRight)
		{
			//get 3d normalized movedirection
			//const glm::vec3 moveDirection = glm::normalize(playerInput.getBody(0).getLinearVelocity()); //need to get parent velocity
			////get a version of moveDirection that is only in the x-y plane
			//const glm::vec3 moveDirectionXY = glm::normalize(glm::vec3(moveDirection.x, moveDirection.y, 0.f));
			const glm::vec3 moveDirection = glm::normalize(glm::vec3(playerInput.moveDirectionWorld));
			//get a version of moveDirection that is only in the x-y plane
			const glm::vec3 moveDirectionXY = glm::normalize(glm::vec3(moveDirection.x, moveDirection.y, 0.f));

			//get 3dnormalized aim direction
			const glm::vec3 aimDirection = glm::normalize(playerInput.aimDirection);
			//get version of aimDirection that is only in the x-y plane
			const glm::vec3 aimDirectionXY = glm::normalize(glm::vec3(playerInput.aimDirection.x, playerInput.aimDirection.y, 0.f));


			//get signed angle between aim direction and movement direction.
			//Both terms must be the XY-projected vectors — using the 3D aimDirection here
			//inflates the angle by aim's downward z (mouse-on-floor projection from a
			//character capsule offset above z=0), which trips the threshold even when the
			//XY directions are aligned and causes left/right/forward flicker.
			//Clamp the dot to [-1, 1] so FP rounding can't push it above 1.0 and turn acos into NaN.
			const float dotXY = glm::clamp(glm::dot(aimDirectionXY, moveDirectionXY), -1.f, 1.f);
			const float angle = glm::acos(dotXY);
			const float signedAngle = glm::sign(glm::cross(aimDirectionXY, moveDirectionXY).z) * angle;

			const float attackDirectionAngleIncrement = glm::pi<float>() / 6.f;

			if (glm::length(playerInput.moveDirection) < 0.00001f)
			{
				state.m_activeAttackSequence = 4;
			}
			else if (signedAngle < attackDirectionAngleIncrement && signedAngle > -attackDirectionAngleIncrement)
			{
				state.m_activeAttackSequence = 4;
			}
			else if (signedAngle > attackDirectionAngleIncrement)
			{
				state.m_activeAttackSequence = 1;
			}
			else if (signedAngle < -attackDirectionAngleIncrement)
			{
				state.m_activeAttackSequence = 0;
			}

			if (state.m_activeAttackSequence != InvalidAttackSequenceId)
			{
				OGBLOG_G("[Machine.transition] Idle -> Attacking seq=%u", state.m_activeAttackSequence);
				state.m_currentState = DAttackState::Attacking; state.m_timeInCurrentState = 0.f;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}

		}

		break;
	}
	case DAttackState::Attacking:
	{
		if (attackState.hasHitGuard)
		{
			OGBLOG_G("[Machine.transition] Attacking -> GuardFlinch (hasHitGuard)");
			state.m_currentState = DAttackState::GuardFlinch; state.m_timeInCurrentState = 0.f;
			state.m_activeAttackSequence = InvalidAttackSequenceId;
			state.m_queuedAttackSequence = InvalidAttackSequenceId;
			attackIntialConditions.activeAttackSequence = InvalidAttackSequenceId;
		}

		if (playerInput.attackLeft && playerInput.attackRight)
		{
			if (attackState.attackTimer < 0.1)
			{
				OGBLOG_G("[Machine.Attacking] dualtap restart seq=4 (timer<0.1)");
				state.m_queuedAttackSequence = InvalidAttackSequenceId;
				state.m_activeAttackSequence = 4;

				setRadialSimulationInitialConditions(deltaTime, input, attackIntialConditions, state);
			}
			else if (attackState.attackTimer/*!sic*/ > 0.3)
			{
				OGBLOG_G("[Machine.Attacking] queue seq=4 (dual,timer>0.3)");
				state.m_queuedAttackSequence = 4;
			}
		}

		if (attackState.attackTimer/*!sic*/ > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
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

		// [Task 25] Hold the Hadouken-Attacking state for a minimum commitment window before
		// the normal exit-to-Idle gate may fire. The radial sim early-returns on the sentinel
		// (leaving currenSequenceId == InvalidAttackSequenceId from tick T+1), so without this
		// guard the machine would drop back to Idle one tick after the trigger and a still-held
		// attack button would chain an immediate normal swing.
		const bool inHadoukenCommitment =
			state.m_activeAttackSequence == kHadoukenSequenceSentinel
			&& state.m_timeInCurrentState < kHadoukenCommitmentSeconds;

		if (!inHadoukenCommitment && attackState.currenSequenceId == InvalidAttackSequenceId)
		{
			if (state.m_queuedAttackSequence != InvalidAttackSequenceId)
			{
				OGBLOG_G("[Machine.transition] Attacking -> Attacking (queued seq=%u)",
					state.m_queuedAttackSequence);
				state.m_activeAttackSequence = state.m_queuedAttackSequence;
				state.m_queuedAttackSequence = InvalidAttackSequenceId;

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
		// [hit-resolution T1] Mirrors GuardFlinch: dwell for kHitFlinchDuration, no attack-input
		// reads (gating is automatic — the switch never reaches Idle/Attacking while flinching),
		// then return to Idle.
		if (state.m_timeInCurrentState > kHitFlinchDuration)
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

// [Task 39] SerializableFields specializations for dAttackMachineSimulation types.

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
			SIM_MEMBER(S, m_queuedAttackSequence));
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

#pragma optimize( "", on )




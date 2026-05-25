#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include "DAttackRadialSequence.h"
#include "DAttackRadialSimulation.h"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGBrawlerLog.h"


#pragma optimize( "", off )


enum class DAttackState
{
	Attacking,
	Idle,
	GuardFlinch,
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
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsAdapterType>
class IntegrationUtils
{
public:
	IntegrationUtils(float deltaTime,
		const std::vector<DAttackRadialSequence>& attackSequences,
		PhysicsAdapterType& physicsAdapter)
		: deltaTime(deltaTime)
		, attackSequences(attackSequences)
		, m_physicsAdapter(physicsAdapter)
	{}

	float getDeltaTime() const { return deltaTime; }
	PhysicsAdapterType& getPhysicsAdapter() const { return m_physicsAdapter; }

private:
	float deltaTime;
	const std::vector<DAttackRadialSequence>& attackSequences;
	PhysicsAdapterType& m_physicsAdapter;
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
		dAttackRadialSimulation::InitialConditions&>;
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
	}
	return "?";
}

template <typename PhysicsAdapterType>
void integrate3(float deltaTime,
	const AllInput<PhysicsAdapterType>& input,
	Dependencies deps)
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

	switch (state.m_currentState)
	{
	case DAttackState::Idle:
	{
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


			//get signed angle between aim direction and movement direction
			const float angle = glm::acos(glm::dot(aimDirection, moveDirectionXY));
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

		if (attackState.currenSequenceId == InvalidAttackSequenceId)
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
			MemberFieldDesc<&dAttackMachineSimulation::PlayerInput::moveDirectionWorld>{});
	}
};

static_assert(SimulationState<dAttackMachineSimulation::State>);
static_assert(SimulationInput<dAttackMachineSimulation::PlayerInput>);

#pragma optimize( "", on )




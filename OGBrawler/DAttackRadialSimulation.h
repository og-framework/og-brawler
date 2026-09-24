#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/DAttackRadialSimulation-rationale.md · docs/DAttackRadialSimulation-guards.md

#include "OGSimulation/OGExport.h"
#include <algorithm>
#include <vector>
#include <limits>
#include <type_traits>
#include "glm/vec3.hpp"
#include "glm/common.hpp"
#include <glm/gtc/quaternion.hpp>
#include "DAttackRadialSequence.h"
#include "OGBrawler/DAttackSequenceId.h"
#include "OGBrawler/DAttackCircle.h"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/PhysicsDeclaration.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGSimulation/SpatialQueryResult.h"
#include "OGSimulation/SpatialQueryAdapter.h"
#include "OGBrawler/CollisionCategoryConstants.h"
#include "OGBrawlerLog.h"
#include "OGSimulation/OGAssert.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF


class DAttackRadialSequence;


namespace dAttackRadialSimulation
{

class StaticData
{
public:
	StaticData(const std::vector<DAttackRadialSequence>& attackSequences,
		const DAttackCircle& attackCircle)
		: attackSequences(attackSequences)
		, attackCircle(attackCircle)
	{}

	StaticData(const StaticData&) = delete;
	StaticData(StaticData&&) = delete;
	StaticData& operator=(const StaticData&) = delete;
	StaticData& operator=(StaticData&&) = delete;

	const std::vector<DAttackRadialSequence>& getAttackSequences() const { return attackSequences; }
	const DAttackCircle& getAttackCircle() const { return attackCircle; }

private:
	StaticData() = delete;

	const std::vector<DAttackRadialSequence>& attackSequences;
	const DAttackCircle& attackCircle;
};

static_assert(!std::is_copy_constructible_v<StaticData> && !std::is_move_constructible_v<StaticData>
	&& !std::is_copy_assignable_v<StaticData> && !std::is_move_assignable_v<StaticData>,
	"dAttackRadialSimulation::StaticData holds references into sibling members of the owning "
	"simulatableBrawler::StaticData. A copy or a move rebinds them to the SOURCE object's members, "
	"which dangle once it is destroyed: keep all four special members deleted. Was the "
	"non-copyable comment on these declarations (og-netcode-v2-field-defects task 19).");


struct DAttackHit
{
	glm::vec3 position;
	BodyId hitRootBodyId;
	glm::vec3 swingTangent{ 0.f };
};


struct PhysicsSetup
{
	static inline const PhysicalObjectDescriptor body{
		BodyDescriptor{
			.simulatePhysics = true,
			.enableGravity = false
		},
		{   /*shapes*/
			ShapeDescriptor{
				SphereGeometry{30.f},
				CollisionCategories::single(collisionCategory::body)
			}
		}
	};

	static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& staticData)
	{
		const float outerRadius = staticData.getAttackCircle().getOuterRadius();
		return {
			QueryVolumeDescriptor{
				SphereGeometry{outerRadius},
				collisionCategory::bodyAndGuard,
				glm::mat4(1.f),
				collisionCategory::queryRouting
			},
			QueryVolumeDescriptor{
				SphereGeometry{outerRadius * 2.f},
				collisionCategory::bodyAndGuard,
				glm::mat4(1.f),
				collisionCategory::queryRouting
			}
		};
	}
};


using RuntimeBindings = PhysicsRuntimeBindings;


class DerivedState
{
public:
	DerivedState()
	{
		attackHits.reserve(4);
		guardHits.reserve(4);
		hitsThisTick.reserve(4);
		OG_CHECK(attackHits.empty() && guardHits.empty() && hitsThisTick.empty(),
			"dAttackRadialSimulation::DerivedState - a fresh DerivedState must hold NO hits: RESERVE, "
			"never resize (a member-init `attackHits(4)` is a resize). The detector's four-target cap "
			"reads size(), so four phantom entries make every swing register nothing. Was the "
			"RESERVE-NOT-RESIZE comment of movement-sim task 34 (og-netcode-v2-field-defects task 19).");
	}

	DerivedState(const DerivedState& other)
		: attackHits(other.attackHits)
		, guardHits(other.guardHits)
		, hitsThisTick(other.hitsThisTick)
		, guardBlockedThisTick(other.guardBlockedThisTick)
	{}

	const std::vector<DAttackHit>& getAttackHits() const { return attackHits; }
	std::vector<DAttackHit>& editAttackHits() { return attackHits; }

	const std::vector<DAttackHit>& getHitsThisTick() const { return hitsThisTick; }
	std::vector<DAttackHit>& editHitsThisTick() { return hitsThisTick; }

	bool getGuardBlockedThisTick() const { return guardBlockedThisTick; }
	bool& editGuardBlockedThisTick() { return guardBlockedThisTick; }

	const std::vector<DAttackHit>& getGuardHits() const { return guardHits; }
	std::vector<DAttackHit>& editGuardHits() { return guardHits; }

private:
	std::vector<DAttackHit> attackHits;
	std::vector<DAttackHit> guardHits;
	std::vector<DAttackHit> hitsThisTick;
	bool guardBlockedThisTick = false;
};


class PlayerInput
{
public:
	// ⛔G-02  docs/DAttackRadialSimulation-guards.md
	glm::vec3 aimDirection{};
	bool attackLeft = false;
	bool attackRight = false;

	// ⛔G-01  docs/DAttackRadialSimulation-guards.md
	static PlayerInput zero() { return PlayerInput(glm::vec3(0.f, 0.f, 1.f), false, false); }
};


template <typename PhysicsBodyAdapterType>
class IntegrationUtils
{
public:
	IntegrationUtils(float deltaTime,
		PhysicsBodyAdapterType& physicsBodyAdapter)
		: m_deltaTime(deltaTime)
		, m_physicsBodyAdapter(physicsBodyAdapter)
	{}

	float getDeltaTime() const { return m_deltaTime; }
	PhysicsBodyAdapterType& getPhysicsAdapter() const { return m_physicsBodyAdapter; }

private:
	float m_deltaTime;
	PhysicsBodyAdapterType& m_physicsBodyAdapter;
};

template <typename PhysicsBodyAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsBodyAdapterType>>;


class InitialConditions
{
public:
	float initialAimAngle = 0.f;
	glm::vec3 initialAimRotationAxis{0.f, 0.f, 0.f};
	unsigned int activeAttackSequence = InvalidAttackSequenceId;
	unsigned int activeRootBodyId = 0;
};


class State
{
public:
	float attackTimer = 0.f;
	unsigned int currenSequenceId = 0;
	PhysicsBodyState bodyState;
};


struct PhysicsDeclaration
{
	static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }
	static constexpr const char* name = "WeaponAxis";

	template <typename GameStaticDataType>
	static const StaticData& staticDataOf(const GameStaticDataType& gsd) { return gsd.m_attackSimulationStaticData; }

	static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& sd) {
		return PhysicsSetup::queryVolumes(sd);
	}
	static glm::vec3 attachmentOffset(const StaticData& sd) {
		(void)sd;
		return glm::vec3(0.f, 0.f, 30.f);
	}

	using StateType = dAttackRadialSimulation::State;
	static       PhysicsBodyState& bodyStateOf(      StateType& s) { return s.bodyState; }
	static const PhysicsBodyState& bodyStateOf(const StateType& s) { return s.bodyState; }

	RuntimeBindings bindings;
};


struct Dependencies {
	using Owned = OwnedDeps<
		dAttackRadialSimulation::InitialConditions,
		dAttackRadialSimulation::State>;
	using External = ExternalDeps<>;
	using InputType = dAttackRadialSimulation::PlayerInput;
	Owned owned;
	External external;
};


OGBRAWLER_API DAttackSegment getAttackSegment(const InitialConditions& initialConditions, const StaticData& staticData, const glm::vec3& directionInRotationPlane);


inline bool wouldGuardBlock(
	unsigned int     activeAttackSequence,
	const glm::vec3& attackerRoot,
	const glm::mat4& guardTransform,
	const glm::vec3& guardOverlapPosition)
{
	const glm::vec3 guardForward = glm::vec3(guardTransform[0]);
	glm::vec3 collidingPosition = guardOverlapPosition;
	collidingPosition.z = attackerRoot.z;
	const glm::vec3 collisionDirection = attackerRoot - collidingPosition;
	const glm::vec3 normalizedCollisionDirection = glm::normalize(collisionDirection);
	const float shieldAngle      = 0.25f;
	const float outerShieldAngle = glm::pi<float>() * 0.5;
	const float td = std::acos(glm::dot(normalizedCollisionDirection, guardForward));
	// ⛔G-03  docs/DAttackRadialSimulation-guards.md
	if (td < outerShieldAngle)
	{
		const glm::vec3 guardAxis = glm::cross(normalizedCollisionDirection, guardForward);
		if (td < shieldAngle)
		{
			if (activeAttackSequence == 4)
				return true;
		}
		else
		{
			if (guardAxis.z > 0.f && (activeAttackSequence == 0 || activeAttackSequence == 2))
				return true;
			if (guardAxis.z < 0.f && (activeAttackSequence == 1 || activeAttackSequence == 3))
				return true;
		}
	}
	return false;
}


namespace
{

template <typename PhysicsBodyAdapterType>
void setInitialConditions(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	OGBLOG_G("[Radial.setInitialConditions] seq=%u aimAngle=%.4f axis=(%.3f,%.3f,%.3f) (attackTimer reset to 0)",
		initialConditions.activeAttackSequence,
		initialConditions.initialAimAngle,
		initialConditions.initialAimRotationAxis.x,
		initialConditions.initialAimRotationAxis.y,
		initialConditions.initialAimRotationAxis.z);
	OG_CHECK(isRealAttackSequence(initialConditions.activeAttackSequence)
		&& initialConditions.activeAttackSequence < staticData.getAttackSequences().size(),
		"dAttackRadialSimulation::setInitialConditions - the InitialConditions sequence does not "
		"index the sequence table. integrate's Hadouken-sentinel return must stay AHEAD of its "
		"setInitialConditions branch, or the sentinel is used as an index. Was the Hadouken "
		"comment in integrate (og-netcode-v2-field-defects task 19).");
	state.attackTimer = 0.f;
	state.currenSequenceId = initialConditions.activeAttackSequence;

	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	const auto& activeAttackSequence = staticData.getAttackSequences()[initialConditions.activeAttackSequence];

	const glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);
	glm::vec3 rootTranslation = glm::vec3(rootTransform[3]);

	glm::mat4 initialAimRotationMatrix = glm::rotate(glm::mat4(1.f), initialConditions.initialAimAngle, initialConditions.initialAimRotationAxis);
	glm::vec4 localSequenceRotationAxis4(activeAttackSequence.getRotationAxis(), 0.f);
	glm::vec3 worldSequenceRotationAxis = glm::vec3(initialAimRotationMatrix * localSequenceRotationAxis4);
	glm::mat4 initialSequenceRotationMatrix = glm::rotate(glm::mat4(1.f), activeAttackSequence.getInitialAngle(), worldSequenceRotationAxis);
	glm::mat4 initialWorldTransform = glm::translate(glm::mat4(1.0f), rootTranslation) * initialSequenceRotationMatrix * initialAimRotationMatrix;

	physics.setBodyTransform(bindings.ownBodyId, initialWorldTransform);
	physics.setBodyAngularVelocity(bindings.ownBodyId, glm::vec3(0.f, 0.f, activeAttackSequence.getInitialVelocity()));
}

template <typename PhysicsBodyAdapterType>
void setIdlePose(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings)
{
	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	const auto& activeAttackSequence = staticData.getAttackSequences()[state.currenSequenceId];

	const glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);
	glm::vec3 rootTranslation = glm::vec3(rootTransform[3]);

	glm::mat4 upAlignmentTransform = glm::rotate(glm::mat4(1.f), glm::pi<float>()*0.5f, glm::vec3(0.0f, -1.0f, 0.0f));
	upAlignmentTransform[3] = glm::vec4(rootTranslation, 1.f);
	physics.setBodyTransform(bindings.ownBodyId, upAlignmentTransform);
}


template <typename PhysicsBodyAdapterType>
void applyTorque(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings)
{
	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();

	const auto& activeAttackSequence = staticData.getAttackSequences()[initialConditions.activeAttackSequence];
	const glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);

	float a = activeAttackSequence.getAngularAcceleration(state.attackTimer);
	glm::mat4 initialRotationMatrix = glm::rotate(glm::mat4(1.f), initialConditions.initialAimAngle, initialConditions.initialAimRotationAxis);
	glm::vec4 localSequenceRotationAxis4(activeAttackSequence.getRotationAxis(), 0.f);
	glm::vec3 worldSequenceRotationAxis = glm::vec3(rootTransform * localSequenceRotationAxis4);

	const glm::vec3& inertiaTensor = physics.getBodyInertiaTensor(bindings.ownBodyId);
	physics.addBodyTorque(bindings.ownBodyId, worldSequenceRotationAxis * a * inertiaTensor.z);
}


template <typename PhysicsBodyAdapterType>
void deactivate(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	OGBLOG_G("[Radial.deactivate] was seq=%u timer=%.4f",
		state.currenSequenceId, state.attackTimer);
	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	physics.setBodyAngularVelocity(bindings.ownBodyId, glm::vec3(0.f, 0.f, 0.f));

	setIdlePose(deltaSeconds, input, initialConditions, staticData, state, bindings);

	state.attackTimer = 0.f;
	state.currenSequenceId = InvalidAttackSequenceId;

	derivedState.editAttackHits().clear();
	derivedState.editGuardHits().clear();
}


} //anonymous namespace

template <typename PhysicsBodyAdapterType>
void integrate(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType>& input,
	const StaticData& staticData,
	Dependencies deps,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	const InitialConditions& initialConditions = deps.owned.get<InitialConditions>();
	State& state = deps.owned.edit<State>();

	// ⛔G-04  docs/DAttackRadialSimulation-guards.md
	derivedState.editHitsThisTick().clear();
	// ⛔G-05  docs/DAttackRadialSimulation-guards.md
	derivedState.editGuardBlockedThisTick() = false;

	{
		auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
		glm::mat4 parentTransform = physics.getBodyTransform(bindings.parentBodyId);
		glm::vec3 parentPosition = glm::vec3(parentTransform[3]);
		glm::mat4 childTransform = physics.getBodyTransform(bindings.ownBodyId);
		childTransform[3] = glm::vec4(parentPosition + bindings.attachmentOffset, 1.0f);
		physics.setBodyTransform(bindings.ownBodyId, childTransform);
	}

	OGBLOG_G("[Radial.integrate] ic.activeSeq=%u state.curSeq=%u state.attackTimer=%.4f",
		initialConditions.activeAttackSequence, state.currenSequenceId, state.attackTimer);

	if (initialConditions.activeAttackSequence == kHadoukenSequenceSentinel)
	{
		OGBLOG_G("[Verbose][Radial.branch] hadouken sentinel — weapon idle, projectile owns this attack");
		state.currenSequenceId = InvalidAttackSequenceId;
		return;
	}

	if (initialConditions.activeAttackSequence == InvalidAttackSequenceId && state.currenSequenceId != InvalidAttackSequenceId)
	{
		OGBLOG_G("[Verbose][Radial.branch] deactivate (ic invalid, state active)");
		deactivate(deltaSeconds, input, initialConditions, staticData, state, bindings, derivedState);
		return;
	}

	if(state.currenSequenceId != initialConditions.activeAttackSequence)
	{
		OGBLOG_G("[Verbose][Radial.branch] setInitialConditions (state.curSeq=%u -> ic.activeSeq=%u)",
			state.currenSequenceId, initialConditions.activeAttackSequence);
		setInitialConditions(deltaSeconds, input, initialConditions, staticData, state, bindings, derivedState);

		if (state.currenSequenceId == InvalidAttackSequenceId)
			OG_CHECK(false, "DAttackRadialSimulation: invalid attack sequence after setInitialConditions");
	}

	if (state.currenSequenceId == InvalidAttackSequenceId)
	{
		OGBLOG_G("[Verbose][Radial.branch] idle (state.curSeq invalid)");
		setIdlePose(deltaSeconds, input, initialConditions, staticData, state, bindings);
		return;
	}

	const auto& activeAttackSequence = staticData.getAttackSequences()[state.currenSequenceId];
	if (state.attackTimer < activeAttackSequence.getDuration())
	{
		OGBLOG_G("[Verbose][Radial.branch] applyTorque+tick (timer=%.4f dur=%.4f)",
			state.attackTimer, activeAttackSequence.getDuration());
		applyTorque(deltaSeconds, input, initialConditions, staticData, state, bindings);

		state.attackTimer = state.attackTimer + deltaSeconds;
	}
	else
	{
		OGBLOG_G("[Verbose][Radial.branch] deactivate (timer>=duration: %.4f >= %.4f)",
			state.attackTimer, activeAttackSequence.getDuration());
		deactivate(deltaSeconds, input, initialConditions, staticData, state, bindings, derivedState);
	}
}

template <typename StateReplicator>
void network(StateReplicator& replicator)
{
}

}


template <>
struct SerializableFields<dAttackRadialSimulation::InitialConditions>
{
	static constexpr auto get()
	{
		using IC = dAttackRadialSimulation::InitialConditions;
		return std::make_tuple(
			SIM_MEMBER(IC, initialAimAngle),
			SIM_MEMBER(IC, initialAimRotationAxis),
			SIM_MEMBER(IC, activeAttackSequence),
			SIM_MEMBER(IC, activeRootBodyId));
	}
};

template <>
struct SerializableFields<dAttackRadialSimulation::State>
{
	static constexpr auto get()
	{
		using S = dAttackRadialSimulation::State;
		return std::make_tuple(
			SIM_MEMBER(S, attackTimer),
			SIM_MEMBER(S, currenSequenceId),
			SIM_MEMBER(S, bodyState));
	}
};

template <>
struct SerializableFields<dAttackRadialSimulation::PlayerInput>
{
	static constexpr auto get()
	{
		return std::make_tuple(
			MemberFieldDesc<&dAttackRadialSimulation::PlayerInput::aimDirection>{},
			MemberFieldDesc<&dAttackRadialSimulation::PlayerInput::attackLeft>{},
			MemberFieldDesc<&dAttackRadialSimulation::PlayerInput::attackRight>{});
	}
};

static_assert(SimulationState<dAttackRadialSimulation::State>);
static_assert(SimulationInput<dAttackRadialSimulation::PlayerInput>);
static_assert(SimulationInitialConditions<dAttackRadialSimulation::InitialConditions>);

OGSIM_OPTIMIZE_ON



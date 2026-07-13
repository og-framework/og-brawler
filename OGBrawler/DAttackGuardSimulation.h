#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGTypes.h"
#include <algorithm>
#include <vector>
#include <limits>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include "DAttackRadialSequence.h"
#include "DAttackMachineSimulation.h"
#include "OGBrawler/DAttackCircle.h"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGBrawler/CollisionCategoryConstants.h"

#pragma optimize( "", off )


class DAttackRadialSequence;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace dAttackGuardSimulation
{

class StaticData
{
public:
	StaticData(const DAttackCircle& attackCircle)
		: attackCircle(attackCircle)
	{}

	// Holds a reference into a sibling member of the owning simulatableBrawler::StaticData
	// (attackCircle). Copying/moving would rebind that reference to the source object's
	// member, dangling once the source is destroyed. The former hand-written copy ctor
	// did exactly that silently — now compiler-enforced non-copyable.
	StaticData(const StaticData&) = delete;
	StaticData(StaticData&&) = delete;
	StaticData& operator=(const StaticData&) = delete;
	StaticData& operator=(StaticData&&) = delete;

	const DAttackCircle& getAttackCircle() const { return attackCircle; }

private:
	StaticData() = delete;

	const DAttackCircle& attackCircle;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// All physics setup descriptors for the guard simulation.
struct PhysicsSetup
{
	static inline const PhysicalObjectDescriptor body{
		BodyDescriptor{
			.simulatePhysics = true,
			.enableGravity = false
		},
		{   // shapes
			ShapeDescriptor{
				SphereGeometry{40.f},
				CollisionCategories::single(collisionCategory::guard)
			}
		}
	};
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Immutable bindings set once at init — body IDs, attachment offset, shape IDs, query volumes.
struct RuntimeBindings
{
	BodyId ownBodyId;
	BodyId parentBodyId;
	glm::vec3 attachmentOffset;
	std::vector<ShapeId> shapeIds;
	std::vector<QueryVolumeId> queryVolumeIds;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Kept as empty struct for structural consistency — guard has no mutable scratch data.
class DerivedState
{
public:
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PlayerInput
{
public:
	// [Task 43] Plain aggregate — const dropped so MemberFieldDesc::write() can assign.
	glm::vec3 aimDirection{};
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
class IntegrationUtils
{
public:
	IntegrationUtils(float deltaTime,
		PhysicsBodyAdapterType& physicsBodyAdapter,
		SpatialQueryAdapterType& queryAdapter)
		: m_deltaTime(deltaTime)
		, m_physicsBodyAdapter(physicsBodyAdapter)
		, m_queryAdapter(queryAdapter)
	{}

	float getDeltaTime() const { return m_deltaTime; }
	PhysicsBodyAdapterType& getPhysicsAdapter() const { return m_physicsBodyAdapter; }
	SpatialQueryAdapterType& getQueryAdapter() const { return m_queryAdapter; }

private:
	float m_deltaTime;
	PhysicsBodyAdapterType& m_physicsBodyAdapter;
	SpatialQueryAdapterType& m_queryAdapter;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsBodyAdapterType, SpatialQueryAdapterType>>;


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class InitialConditions
{
public:
	InitialConditions()
	{}

private:
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class State
{
public:
	// [Task 44] Plain aggregate — renamed m_attackTimer -> attackTimer.
	float attackTimer = 0.f;
	uint32 testTick = 0;
	PhysicsBodyState bodyState;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PhysicsDeclaration
{
	static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }
	static constexpr const char* name = "GuardAxis";
	static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& /*sd*/) {
		return std::vector<QueryVolumeDescriptor>{};
	}
	static glm::vec3 attachmentOffset(const StaticData& sd) {
		(void)sd;
		return glm::vec3(0.f, 0.f, 30.f);
	}

	using StateType = dAttackGuardSimulation::State;
	static       PhysicsBodyState& bodyStateOf(      StateType& s) { return s.bodyState; }
	static const PhysicsBodyState& bodyStateOf(const StateType& s) { return s.bodyState; }

	RuntimeBindings bindings;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// [Task 62] Dependencies — OwnedDeps/ExternalDeps layout.
struct Dependencies {
	using Owned = OwnedDeps<
		dAttackGuardSimulation::InitialConditions,
		dAttackGuardSimulation::State>;
	using External = ExternalDeps<
		const dAttackMachineSimulation::State&>;
	using InputType = dAttackGuardSimulation::PlayerInput;
	Owned owned;
	External external;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} //anonymous namespace
template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void integrate(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
	const StaticData& staticData,
	Dependencies deps,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	const InitialConditions& initialConditions = deps.owned.get<InitialConditions>();
	State& state = deps.owned.edit<State>();
	const dAttackMachineSimulation::State& attackMachineSimulation = deps.external.get<dAttackMachineSimulation::State>();

	++state.testTick;

	// [NP-6] Explicit attachment math — replaces updateLinearAttachmentToOwner()
	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	{
		glm::mat4 parentTransform = physics.getBodyTransform(bindings.parentBodyId);
		glm::vec3 parentPosition = glm::vec3(parentTransform[3]);
		glm::mat4 childTransform = physics.getBodyTransform(bindings.ownBodyId);
		childTransform[3] = glm::vec4(parentPosition + bindings.attachmentOffset, 1.0f);
		physics.setBodyTransform(bindings.ownBodyId, childTransform);
	}

	auto& queryAdapter = input.getIntegrationUtils().getQueryAdapter();

	if(attackMachineSimulation.m_currentState != DAttackState::Idle)
	{
		for (const auto& shapeId : bindings.shapeIds)
			queryAdapter.disableShape(shapeId);

		return;
	}
	else
	{
		for (const auto& shapeId : bindings.shapeIds)
			queryAdapter.enableShape(shapeId);
	}

	const glm::vec3 rawAim(input.getPlayerInput().aimDirection.x, input.getPlayerInput().aimDirection.y, 0.f);
	const float aimLen = glm::length(rawAim);
	const glm::vec3 defaultForward(1.f, 0.f, 0.f);
	const glm::vec3 defaultUp(0.f, 0.f, 1.f);

	// When there is no aim input, fall back to default forward to avoid NaN from normalizing a zero vector.
	const glm::vec3 aimDirection = (aimLen > 0.0001f) ? (rawAim / aimLen) : defaultForward;
	const float aimDot = glm::clamp(glm::dot(aimDirection, defaultForward), -1.f, 1.f);
	const float aimAngle = glm::acos(aimDot);
	const bool aimEqualsForward = abs(abs(aimDot) - 1.f) < 0.0001f;
	const glm::vec3 aimRotationAxis = [&aimEqualsForward, &defaultUp, &defaultForward, &aimDirection]() {
		if (aimEqualsForward)
			return defaultUp;
		else
			return glm::normalize(glm::cross(defaultForward, aimDirection));
		}();

	glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);

	glm::mat4 guardTransform = glm::rotate(glm::mat4(1.f), aimAngle, aimRotationAxis);
	guardTransform[3] = glm::vec4(glm::vec3(rootTransform[3]) /*+ glm::vec3(0.f, 0.f, 200.f)*/, 1.f);
	physics.setBodyTransform(bindings.ownBodyId, guardTransform);
}

}

// [Task 39] SerializableFields specializations for dAttackGuardSimulation types.

// [Task 47] Empty SerializableFields for InitialConditions — zero serialized fields.
template <>
struct SerializableFields<dAttackGuardSimulation::InitialConditions>
{
	static constexpr auto get() { return std::make_tuple(); }
};

template <>
struct SerializableFields<dAttackGuardSimulation::State>
{
	static constexpr auto get()
	{
		using S = dAttackGuardSimulation::State;
		return std::make_tuple(
			SIM_MEMBER(S, attackTimer),
			SIM_MEMBER(S, bodyState));
	}
};

template <>
struct SerializableFields<dAttackGuardSimulation::PlayerInput>
{
	static constexpr auto get()
	{
		return std::make_tuple(MemberFieldDesc<&dAttackGuardSimulation::PlayerInput::aimDirection>{});
	}
};

static_assert(SimulationState<dAttackGuardSimulation::State>);
static_assert(SimulationInput<dAttackGuardSimulation::PlayerInput>);
static_assert(SimulationInitialConditions<dAttackGuardSimulation::InitialConditions>);

#pragma optimize( "", on )




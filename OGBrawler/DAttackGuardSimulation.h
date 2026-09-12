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
#include "OGSimulation/PhysicsDeclaration.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGBrawler/CollisionCategoryConstants.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF


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

// The one shared definition lives in OGSimulation/PhysicsDeclaration.h. The
// PhysicsDeclaration concept requires `same_as<PhysicsRuntimeBindings&>`, so a
// field-identical per-sim copy is a DISTINCT type and does not conform; this
// alias keeps every existing `dAttackGuardSimulation::RuntimeBindings`
// spelling valid while making the type the shared one.
using RuntimeBindings = PhysicsRuntimeBindings;

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

	// THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
	// SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
	// [movement-sim task 22] The value is copied VERBATIM from what that function
	// handed this type before the fold; it is a wire value, not something to re-derive.
	// ⛔ (0,0,1) forwards, NOT PlayerInput{}: a value-initialised (0,0,0) aim would
	// reach normalize(), and the difference is also the TAG the input-resolution and
	// net-sync anti-vacuity tests discriminate on. Keep zero() != PlayerInput{}.
	static PlayerInput zero() { return PlayerInput(glm::vec3(0.f, 0.f, 1.f)); }
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
	// [movement-sim task 29] TRANSIENT, LOCAL-ONLY body state — the projectile pattern.
	//
	// Every field of the guard's pose is a CLOSED FORM over state that is already
	// replicated: position = parent capsule position + attachmentOffset, rotation =
	// f(this tick's aim input). Nothing is integrated — no torque and no angular
	// velocity is ever written — so replicating it carried 52 B that told the
	// receiver nothing it could not recompute. It is EXCLUDED from
	// SerializableFields (see the empty specialization at the bottom of this file)
	// and survives only as scratch for two readers: the physics composite's
	// captureBodyStatesAll(), which needs a PhysicsBodyState lvalue through
	// PhysicsDeclaration::bodyStateOf, and the legacy-arcs visualization in
	// DAttackTargetVisualizationTwo.h. integrate() keeps it in step with the
	// transform it writes, so the two cannot disagree WITHIN a tick.
	//
	// ⚠ SOUNDNESS CAVEAT — STALE TRANSIENT AT THE ANCHOR FRAME, stated rather than
	// discovered later. A correction rewinds the engine by pushing
	// bodyStateOf(state) back onto the body; for an off-wire field that pushes the
	// LOCAL transient, i.e. the pose this client last derived, not the authority's.
	// The next integrate re-snaps it from the (corrected) parent and aim, so the
	// staleness lasts exactly one frame — but on that anchor frame the guard shape
	// is query-enabled, so an attack resolved against the stale guard position is
	// possible in principle. This is the SAME window brawlerProjectileSimulation
	// already accepts for its own off-wire slot bodyState. Closing it generically
	// needs a live-state-aware widenForPush (task-28 note §2b), not a change here.
	PhysicsBodyState bodyState;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PhysicsDeclaration
{
	static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }
	static constexpr const char* name = "GuardAxis";

	// Maps the GAME's aggregate static data to this sub-simulation's own slice.
	// This is what makes body creation generic: the engine-side fold asks each
	// declaration for its slice instead of branching on the declaration type.
	// A member TEMPLATE deliberately — this header cannot name
	// simulatableBrawler::StaticData, because the aggregate includes this header
	// (an include cycle). GameStaticDataType is deduced at the call site, where the aggregate is
	// complete.
	template <typename GameStaticDataType>
	static const StaticData& staticDataOf(const GameStaticDataType& gsd) { return gsd.m_guardSimulationStaticData; }

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

	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	auto& queryAdapter = input.getIntegrationUtils().getQueryAdapter();

	// [NP-6] Explicit attachment math — replaces updateLinearAttachmentToOwner().
	// [movement-sim task 29, F-G3] ONE PARENT READ AND ONE WRITE. This used to be a
	// parent read, an OWN read and a write, followed further down by a SECOND own
	// read and a SECOND write that re-used the translation the first write had just
	// put there. The two compose into a single transform, so they are composed here
	// instead: five adapter round-trips per character per tick became two.
	const glm::mat4 parentTransform = physics.getBodyTransform(bindings.parentBodyId);
	const glm::vec3 rootTranslation = glm::vec3(parentTransform[3]) + bindings.attachmentOffset;

	const glm::vec3 rawAim(input.getPlayerInput().aimDirection.x, input.getPlayerInput().aimDirection.y, 0.f);
	const float aimLen = glm::length(rawAim);
	const glm::vec3 defaultForward(1.f, 0.f, 0.f);
	const glm::vec3 defaultUp(0.f, 0.f, 1.f);

	// When there is no aim input, fall back to default forward to avoid NaN from normalizing a zero vector.
	const glm::vec3 aimDirection = (aimLen > 0.0001f) ? (rawAim / aimLen) : defaultForward;
	const float aimDot = glm::clamp(glm::dot(aimDirection, defaultForward), -1.f, 1.f);
	const float aimAngle = glm::acos(aimDot);

	// [movement-sim task 29, F-G4] glm::abs, NOT unqualified abs.
	//
	// aimDot is a float, and an unqualified `abs` here binds to whatever is in scope
	// at THIS header's point of definition. MEASURED on this toolchain (MSVC 14.38,
	// DAttackGuardSimulationTest.cpp's NearPoleAimTakesTheEpsilonBandBranch, which is
	// GREEN on the pre-fix header): it was ALREADY binding the float overload, so this
	// is PORTABILITY HARDENING and not a behaviour fix. It matters because og-brawler
	// targets a Godot port and a Jolt adapter, i.e. other toolchains, where only
	// the integer `::abs` overload may be visible — and under it the whole expression
	// truncates to `|aimDot| == 1` EXACTLY, silently deleting the epsilon band below.
	// glm::abs cannot resolve to an integer overload for a float argument.
	//
	// THE BAND, and why it tests |aimDot| rather than aimDot: cross(defaultForward,
	// aim) is the ZERO VECTOR at BOTH poles, and glm::normalize of it divides by zero.
	// acos returns an UNSIGNED angle, so at the antiparallel pole every axis normal to
	// the XY plane gives the same rotation and defaultUp is a legitimate substitute.
	// Inside the band (an aim within ~0.81 degrees of ±forward) the sign of the tiny
	// residual angle is therefore NOT preserved — accepted deliberately: 0.81 degrees
	// of facing on a 40 cm sphere, against a NaN-free normalize. Pinned by that same
	// test case, which asserts the band branch is the one a near-pole aim takes.
	const bool aimEqualsForward = glm::abs(glm::abs(aimDot) - 1.f) < 0.0001f;
	const glm::vec3 aimRotationAxis = [&aimEqualsForward, &defaultUp, &defaultForward, &aimDirection]() {
		if (aimEqualsForward)
			return defaultUp;
		else
			return glm::normalize(glm::cross(defaultForward, aimDirection));
		}();

	// [F-G3] THE ONE WRITE. Composition order is preserved from the two-write form:
	// the rotation is built about the origin and the translation is dropped into
	// column 3 afterwards, so no translation is ever rotated.
	//
	// Written UNCONDITIONALLY, before the shape toggle below and with no early return,
	// so the body follows the parent in every machine state. The POSITION is identical
	// to what the old [NP-6] block wrote on both branches; the only difference is that
	// a non-Idle tick now also refreshes the ROTATION where it used to keep a stale
	// one. That is unobservable: the shapes are disabled on those ticks so no query
	// reaches the guard, and the only reader of the guard rotation
	// (DAttackTargetVisualizationTwo.h) has its two draw calls commented out. Keeping
	// the pose fresh is what makes the FIRST Idle tick after an attack correct with no
	// special case.
	glm::mat4 guardTransform = glm::rotate(glm::mat4(1.f), aimAngle, aimRotationAxis);
	guardTransform[3] = glm::vec4(rootTranslation, 1.f);
	physics.setBodyTransform(bindings.ownBodyId, guardTransform);

	// [F-G1] Keep the off-wire transient in step with the transform just written, so
	// the visualization and the post-solve capture cannot disagree within a tick.
	state.bodyState.position = rootTranslation;
	state.bodyState.rotation = glm::quat_cast(guardTransform);

	// [movement-sim task 29, F-G5] UNCONDITIONAL ON PURPOSE — idempotent and
	// resim-safe. Edge-triggering this would need a "last written" bit, and that bit
	// would have to survive resim: off-wire state resets on nothing, so a replay that
	// changed the machine state would leave the toggle desynced from it. The write is
	// a SetQueryEnabled on the physics thread and is idempotent, so it is simply done
	// every tick.
	if (attackMachineSimulation.m_currentState != DAttackState::Idle)
	{
		for (const auto& shapeId : bindings.shapeIds)
			queryAdapter.disableShape(shapeId);
	}
	else
	{
		for (const auto& shapeId : bindings.shapeIds)
			queryAdapter.enableShape(shapeId);
	}
}

}

// [Task 39] SerializableFields specializations for dAttackGuardSimulation types.

// [Task 47] Empty SerializableFields for InitialConditions — zero serialized fields.
template <>
struct SerializableFields<dAttackGuardSimulation::InitialConditions>
{
	static constexpr auto get() { return std::make_tuple(); }
};

// [movement-sim task 29] EMPTY, deliberately — the guard puts NOTHING on the wire.
// -56 B: the phantom `attackTimer` (4 B, never written by this simulation) and the
// derivable `bodyState` (52 B). See the caveat on State::bodyState above. The guard's
// InitialConditions has been empty since Task 47, so the whole sub-simulation is now a
// 0-byte slice of simulatableBrawler::State.
template <>
struct SerializableFields<dAttackGuardSimulation::State>
{
	static constexpr auto get() { return std::make_tuple(); }
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

OGSIM_OPTIMIZE_ON




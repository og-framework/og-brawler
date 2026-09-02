#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGTypes.h"  // for BodyId
#include <vector>
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/PhysicsDeclaration.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGBrawler/CollisionCategoryConstants.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

// Home of the brawler character-movement sub-sim.
//
// [movement-sim task 1, 2026-09-01] SKELETON. The sub-sim now exists structurally
// — it owns one placeholder body and does NOTHING. Its purpose at this task is to
// make every hookup point a new body-owning simulation must touch explicit, in a
// working diff, with no behaviour to reason about:
//
//   * State carries ONLY `LinearBodyState bodyState` — the 24 B slim wire shape
//     (position + linearVelocity). [movement-sim task 5, 2026-09-02] swapped it in
//     from the placeholder 52 B `PhysicsBodyState`, and that swap is the WHOLE of
//     that task's production diff: nothing in OGSimulation, the UE simulation layer
//     or the UE game layer needed a line, because the capture loop and the engine's
//     rewind push both go through the conversion bridge declared on
//     `LinearBodyState` itself (OGSimulation/PhysicsBodyState.h).
//   * InitialConditions / PlayerInput / DerivedState are empty; the first two
//     serialize to zero wire bytes (the guard-InitialConditions precedent).
//   * The body is a factory SPHERE (the factory makes nothing else today) under
//     collisionCategory::character, which is deliberately UNMAPPED in the engine
//     adapter — so no existing query can see it and no gameplay changes.
//   * integrate() is guard's [NP-6] parent-follow attachment block and nothing
//     else, so the create -> integrate -> post-solve-capture -> wire round trip
//     is observable without any movement behaviour existing yet.
//
// The real movement behaviour (surface-relative models, hover servo, sweeps,
// capsule adopt-root) lands in the grow task; the capsule-provenance migration
// that sources CharacterBindings::capsuleBodyId from this sub-sim's
// bindings.ownBodyId lands with it.
namespace brawlerMovementSimulation
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// [Task 35] The per-character handle consumed by dAttackMachineSimulation (T33).
//
// Populated ONCE at registration time from the authoritative parentBodyId — today
// the UE ACharacter::GetCapsuleComponent lookup, NOT this sub-sim's own body. That
// is still true after the skeleton: the skeleton's body is an auxiliary sphere
// attached under the capsule, not the capsule itself.
//
// FUTURE (unchanged by the skeleton): once the movement sub-sim owns the capsule,
// capsuleBodyId is sourced from this sub-sim's bindings.ownBodyId, and the
// follow-on de-duplication task replaces every other sub-sim's bare parentBodyId
// with a CharacterBindings field, eliminating the duplication.
struct CharacterBindings
{
    BodyId capsuleBodyId;  // character's main physics capsule
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Empty but REAL, not elided: the registration fold hands one of these to
// PhysicsDeclaration::queryVolumes(sd) and PhysicsDeclaration::attachmentOffset(sd),
// so the type must exist and be constructible even while it carries no data. It is
// held by value as simulatableBrawler::StaticData::m_movementStaticData, which
// PhysicsDeclaration::staticDataOf returns to the generic registration fold.
class StaticData
{
public:
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// All physics setup descriptors for the movement simulation.
//
// A 30 cm SPHERE, not a capsule: the Chaos factory createPhysicalObject() only
// supports SphereGeometry today and always attaches the new body under a
// pre-existing parent. The capsule adopt-root path is a later task; until it lands
// this placeholder body is what proves the create/capture/correct plumbing works.
struct PhysicsSetup
{
    static inline const PhysicalObjectDescriptor body{
        BodyDescriptor{
            .simulatePhysics = true,
            .enableGravity = false
        },
        {   // shapes
            ShapeDescriptor{
                SphereGeometry{30.f},
                CollisionCategories::single(collisionCategory::character)
            }
        }
    };
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The one shared definition lives in OGSimulation/PhysicsDeclaration.h. The
// PhysicsDeclaration concept requires `same_as<PhysicsRuntimeBindings&>`, so a
// field-identical per-sim copy is a DISTINCT type and does not conform; this
// alias keeps every existing `brawlerMovementSimulation::RuntimeBindings`
// spelling valid while making the type the shared one.
using RuntimeBindings = PhysicsRuntimeBindings;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Kept as an empty struct for structural consistency — the skeleton has no mutable
// scratch data (guard precedent).
class DerivedState
{
public:
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Empty at the skeleton: the movement sub-sim consumes no player input yet, and an
// empty SerializableFields tuple costs zero wire bytes in the PlayerInput composite.
// It is appended to the composite NOW, with nothing in it, precisely so the composite
// arity and every positional construction of it are settled before behaviour lands.
class PlayerInput
{
public:
    // THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
    // SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
    // [movement-sim task 22] Empty at the skeleton: no fields, zero wire bytes, so
    // the neutral value and PlayerInput{} coincide. When the grow task gives this
    // input real fields, decide the neutral pose HERE — nowhere else has to change.
    static PlayerInput zero() { return PlayerInput{}; }
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

// Empty at the skeleton — zero serialized fields (guard precedent). Present in the
// State composite anyway so the slot exists before the grow task needs it.
class InitialConditions
{
public:
    InitialConditions()
    {}

private:
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// THE ONLY STATE. `bodyState` is written by the generic post-solve capture pass
// (SimulationIntegrationExecutor::captureBodyStatesAll, via
// PhysicsDeclaration::bodyStateOf) and pushed back into Chaos's rewind timeline on
// a resim — neither of which this sub-sim writes a line of code for. Its presence
// in SerializableFields is what puts the body on the wire and in the checksum.
//
// [movement-sim task 5, 2026-09-02] `LinearBodyState`, NOT `PhysicsBodyState`:
// position + linearVelocity only, 24 B instead of 52 B. Both generic sites above
// keep compiling untouched — capture ASSIGNS a captured `PhysicsBodyState` in
// (the narrowing `operator=`), the rewind push CONVERTS one back out (the implicit
// widening `operator PhysicsBodyState()`), and neither site names a body-state
// type. See the CHOICE RULE at the top of OGSimulation/PhysicsBodyState.h.
//
// ⚠ WHAT IS DROPPED, and why it is sound HERE: rotation and angular velocity no
// longer travel, and the push fabricates identity/zero for them. The soundness
// condition on the widening operator names `BodyDescriptor::lockRotation` — a field
// that does NOT exist yet (it is the sweep/descriptor seam task's). What makes the
// drop sound in the meantime is narrower and specific to the skeleton: this body is
// an invisible auxiliary sphere under a deliberately UNMAPPED collision category,
// integrate() re-snaps its transform to parent position every tick, and NOTHING —
// no query, no gameplay path, no visualization — reads its orientation or spin.
// ⛔ When `lockRotation` lands, PhysicsSetup::body's BodyDescriptor must set it, and
// the grow task must re-check this the moment the body starts carrying real motion.
class State
{
public:
    LinearBodyState bodyState;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PhysicsDeclaration
{
    static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }
    static constexpr const char* name = "MovementBody";

    // Maps the GAME's aggregate static data to this sub-simulation's own slice.
    // This is what makes body creation generic: the engine-side fold asks each
    // declaration for its slice instead of branching on the declaration type.
    // A member TEMPLATE deliberately — this header cannot name
    // simulatableBrawler::StaticData, because the aggregate includes this header
    // (an include cycle). GameStaticDataType is deduced at the call site, where the aggregate is
    // complete.
    template <typename GameStaticDataType>
    static const StaticData& staticDataOf(const GameStaticDataType& gsd) { return gsd.m_movementStaticData; }

    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& /*sd*/) {
        return std::vector<QueryVolumeDescriptor>{};
    }
    static glm::vec3 attachmentOffset(const StaticData& sd) {
        (void)sd;
        return glm::vec3(0.f, 0.f, 0.f);
    }

    using StateType = brawlerMovementSimulation::State;
    // [movement-sim task 5] Return type follows State::bodyState. The generic
    // consumers are written against `BodyStateLike`, not against a concrete type.
    static       LinearBodyState& bodyStateOf(      StateType& s) { return s.bodyState; }
    static const LinearBodyState& bodyStateOf(const StateType& s) { return s.bodyState; }

    RuntimeBindings bindings;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Dependencies {
    using Owned = OwnedDeps<
        brawlerMovementSimulation::InitialConditions,
        brawlerMovementSimulation::State>;
    using External = ExternalDeps<>;
    using InputType = brawlerMovementSimulation::PlayerInput;
    Owned owned;
    External external;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// skeleton — parent-follow only; the real movement behaviour lands in the grow task.
template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void integrate(float deltaSeconds,
    const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
    const StaticData& staticData,
    Dependencies deps,
    const RuntimeBindings& bindings,
    DerivedState& derivedState)
{
    // Deliberately unread at the skeleton. `deps` in particular: State::bodyState is
    // written by the generic post-solve capture, NOT here, which is exactly the
    // round trip this task exists to make observable.
    (void)deltaSeconds;
    (void)staticData;
    (void)deps;
    (void)derivedState;

    // [NP-6] Explicit attachment math — read the parent transform, re-snap our own
    // body to parent position + offset. This is the WHOLE body of integrate().
    auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
    {
        glm::mat4 parentTransform = physics.getBodyTransform(bindings.parentBodyId);
        glm::vec3 parentPosition = glm::vec3(parentTransform[3]);
        glm::mat4 childTransform = physics.getBodyTransform(bindings.ownBodyId);
        childTransform[3] = glm::vec4(parentPosition + bindings.attachmentOffset, 1.0f);
        physics.setBodyTransform(bindings.ownBodyId, childTransform);
    }
}

}

// SerializableFields specializations for brawlerMovementSimulation types.

// Empty SerializableFields for InitialConditions — zero serialized fields.
template <>
struct SerializableFields<brawlerMovementSimulation::InitialConditions>
{
    static constexpr auto get() { return std::make_tuple(); }
};

template <>
struct SerializableFields<brawlerMovementSimulation::State>
{
    static constexpr auto get()
    {
        using S = brawlerMovementSimulation::State;
        return std::make_tuple(
            SIM_MEMBER(S, bodyState));
    }
};

// Empty SerializableFields for PlayerInput — zero serialized fields.
template <>
struct SerializableFields<brawlerMovementSimulation::PlayerInput>
{
    static constexpr auto get() { return std::make_tuple(); }
};

static_assert(SimulationState<brawlerMovementSimulation::State>);
static_assert(SimulationInput<brawlerMovementSimulation::PlayerInput>);
static_assert(SimulationInitialConditions<brawlerMovementSimulation::InitialConditions>);

OGSIM_OPTIMIZE_ON

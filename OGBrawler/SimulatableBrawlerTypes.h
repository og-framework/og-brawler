#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGTypes.h"
// Canonical home for the brawler composite simulation: type aggregates
// (State, DerivedState, AllState, PlayerInput, StaticData) plus the
// composite's compile-time dependency-graph validation.
//
// The integrate flow itself lives in SimulatableBrawler::integrate.

#include <vector>
#include "glm/vec3.hpp"
#include "DAttackRadialSequence.h"
#include "OGBrawler/DAttackCircle.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawler/DAttackGuardSimulation.h"
#include "OGBrawler/DAttackMachineSimulation.h"
// Phase 2 / Task 15 (2026-06-24): projectile sub-sim RE-WIRED into the brawler
// composite. The transport changes it was blocked on (og-netcode-v1-impl Stage 1/2,
// 60 Hz + split watermark-trimmed buffers) landed 2026-06-22, and the T13 closed-form
// projectile State redesign shrank the per-character wire footprint (115 B vs 196 B).
// FSimulationStateSyncBuffer::kBufferBytes was bumped to 384 (T14) to fit the re-wired
// composite (~304 B).
#include "OGBrawler/BrawlerProjectileSimulation.h"
// [hit-resolution T2] Inbound-hit signal carried on the composite DerivedState. Off-wire
// (no SerializableFields specialization), so including it here adds no bytes to the State
// composite — see current_state.md §D1.
#include "OGBrawler/BrawlerInboundHit.h"
// [Task 35, re-pointed at movement-sim task 62] THE MOVEMENT SUB-SIM HEADER. Task 35 added this
// include for `CharacterBindings`; since movement-sim task 11 it has been LOAD-BEARING for a
// different reason — the composite owns `m_movementStaticData` and the movement
// State/InitialConditions slices below. It is NOT a visibility convenience; do not drop it.
// ⚠ [movement-sim task 62] Two things task 35's wording said about this include are no longer
// true. It is not a "minimal header" (that described the skeleton; it is 1600+ lines now), and
// it is no longer where `CharacterBindings` is DEFINED — that type moved to the leaf
// `OGBrawler/BrawlerCharacterBindings.h` so the machine sub-sim could have it without an include
// cycle. Downstream consumers of SimulatableBrawlerTypes.h still see
// `simulatableBrawler::CharacterBindings` transitively, because the movement header includes
// that leaf — which is what the UNFILED, PARKED T34 migration (a bindings field on every
// sub-sim's RuntimeBindings) would want; see the FUTURE note in the leaf header for where that
// task actually lives.
// ⚠ [movement-sim task 64] The namespace read `brawlerMovementSimulation` until task 64 put it
// back to `simulatableBrawler`. Neither the include nor the transitive visibility changed.
#include "OGBrawler/BrawlerMovementSimulation.h"
#include "OGSimulation/SimulationDependencies.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace simulatableBrawler
{
// Serialization wire order. Sizes are the ones the fence PINS
// (`SimulatableBrawlerTest.cpp`, `DAttack.SimulatableBrawler.WireFootprint`); the composite
// total is 321 B. Slices with no figure are not individually pinned there — do not invent one.
//   radialIC -> radialState -> guardState (0 B) -> guardIC (0 B)
//   -> machineState -> projectileIC -> projectileState
//   -> movementIC (16 B) -> movementState (61 B)
// ⚠ [movement-sim task 17] `movementIC` read "(0 bytes)" until here. Stale since task 11 put
// the teleport seed on the wire — the SAME defect as the input-order comment further down,
// found by this task's widened sweep rather than by the routed list, which named only that one.
// (projectile slices appended last per Task 15; machine writes projectileIC, projectile
//  consumes it — see ExecutionOrder below.)
using State = SimulationStateComposite<
    dAttackRadialSimulation::InitialConditions,
    dAttackRadialSimulation::State,
    dAttackGuardSimulation::State,
    dAttackGuardSimulation::InitialConditions,
    dAttackMachineSimulation::State,
    brawlerProjectileSimulation::InitialConditions,
    brawlerProjectileSimulation::State,
    brawlerMovementSimulation::InitialConditions,
    brawlerMovementSimulation::State
>;

// OFF-WIRE per-tick scratch, accessed by TYPE: derivedState.get<T>() / .edit<T>(), exactly
// as State is.
using DerivedState = SimulationDerivedComposite<
    dAttackRadialSimulation::DerivedState,
    dAttackGuardSimulation::DerivedState,
    brawlerProjectileSimulation::DerivedState,
    // [movement-sim T1] Empty at the skeleton; the slot exists so the integrate
    // block in SimulatableBrawler.h can pass it, exactly as the other sub-sims do.
    brawlerMovementSimulation::DerivedState,
    // [hit-resolution T2] Per-tick inbound-hit signal, and the one element below that is
    // NOT a sub-simulation's own scratch. Reset + populated by brawlerHitRouting::System::
    // postIntegrate; read the FOLLOWING tick by the machine sim's integrate3 to drive the
    // HitFlinch transition. That system-owned lifecycle is why it is passed to integrate3
    // as a plain by-ref parameter and not as an ExternalDep — see D7, and follow-on F3.
    // Off-wire (see D1) — no SerializableFields entry, and the alias above now enforces it.
    brawlerInboundHit::DerivedState
>;

class AllState
{
public:
    AllState() = default;

    const State& getState() const { return m_state; }
    State& editState() { return m_state; }

    const DerivedState& getDerivedState() const { return m_derivedState; }
    DerivedState& editDerivedState() { return m_derivedState; }

private:
    State m_state;
    DerivedState m_derivedState;
};

// Serialization wire order, with each slice's SHIPPED byte count — re-derived from the fence
// (`SimulatableBrawlerTest.cpp`, `kZeroInputWireBytes == 77`) by [movement-sim task 17], which
// found the movement entry still reading `(0 bytes)`, stale since task 11 put a byte there:
//   radialInput (14) -> machineInput (38) -> guardInput (12) -> projectileInput (12)
//     -> movementInput (1) = 77 B
// ⛔ The movement byte is the `PlayerInput::flags` byte, and it is the SCARCE wire: read the
// packing rule at that type before adding a signal to it.
using PlayerInput = SimulationInputComposite<
    dAttackRadialSimulation::PlayerInput,
    dAttackMachineSimulation::PlayerInput,
    dAttackGuardSimulation::PlayerInput,
    brawlerProjectileSimulation::PlayerInput,
    brawlerMovementSimulation::PlayerInput
>;

// THE NEUTRAL INPUT.
inline PlayerInput getZeroPlayerInput() { return PlayerInput::zero(); }

class StaticData
{
public:
    // ⭐⭐ THE FIVE MOVEMENT PARAMETERS ARE PARAMETERS, AND THEY ARE DEFAULTED —
    // [movement-sim task 16, 2026-09-08]. Every other constant below is still authored here.
    //
    // ⛔ THE DEFAULTS ARE THE AUTHORED LITERALS, NOT A SECOND SET OF NUMBERS. Each one is the
    // exact value this constructor passed before task 16, so a default-constructed
    // `StaticData` — which is what every LLT rig and every test peer builds — is BYTE-FOR-BYTE
    // the object it was, and the whole test tree stays a measurement of the shipped constants
    // rather than of a parameter list. The ONE caller that passes anything is
    // `ASimulationManagerUImpl::m_staticData`, fed by `readMovementStaticDataCVars()`.
    //
    // ⚠ WHY THESE FIVE AND NOT THE OTHERS. Four are the cvars ruling #3 made a ONE-TIME read
    // (`OGBrawler.MovementModel`, `.MoveSpeed`, `.StepPeriodTicks`, `.StepSpeed`); the fifth is
    // gravity, which is not a taste knob but a value that MUST AGREE WITH THE ENGINE'S — see
    // the `checkf` in `ASimulationManagerUImpl::BeginPlay`. The hover gains are deliberately
    // NOT here: their valid region is two-dimensional and is checked, coupled, in
    // `brawlerMovementSimulation::StaticData`'s constructor. Read the `HoverFrequency` entry in
    // `DAttackMachineSimulationRuntimeTweakables.cpp`'s refused-name table before adding them.
    explicit StaticData(
        brawlerMovementSimulation::MovementModel movementModel
            = brawlerMovementSimulation::MovementModel::ContinuousAccelBrake,
        float    movementMaxWalkSpeed    = 100.f,
        uint32_t movementStepPeriodTicks = 20u,
        float    movementStepSpeed       = 100.f,
        float    movementGravity         = -980.f)
        : m_attackCircle(6.f, 90.f, 300.f, 70.f, true, 1.f)
        , m_attackSequences(
            {
                { // from idle to leftAttack
                    {
                        { 0.f, -4.f * PI / 8.f, DAttackRadialSequenceState::WindUp },
                        { 0.3f, -2.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.4f, -1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.5f, 1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.6f, 3.f * PI / 8.f, DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from idle to rightAttack
                    {
                        { 0.f, 4.f * PI / 8.f, DAttackRadialSequenceState::WindUp },
                        { 0.3f, 2.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.4f, 1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.5f, -1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.6f, -3.f * PI / 8.f, DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from leftAttack to leftAttack
                    {
                        { 0.f,   (16.f * PI / 8.f) - (12.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.1f,  (16.f * PI / 8.f) - (10.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.15f, (16.f * PI / 8.f) - (8.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.2f,  (16.f * PI / 8.f) - (6.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.25f, (16.f * PI / 8.f) - (4.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.3f,  (16.f * PI / 8.f) - (2.f  * PI / 8.f), DAttackRadialSequenceState::Damaging },
                        { 0.4f,  (16.f * PI / 8.f) + (1.f  * PI / 8.f), DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from rightAttack to rightAttack
                    {
                        { 0.f,   (-16.f * PI / 8.f) + (12.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.1f,  (-16.f * PI / 8.f) + (10.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.15f, (-16.f * PI / 8.f) + (8.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.2f,  (-16.f * PI / 8.f) + (6.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.25f, (-16.f * PI / 8.f) + (4.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.3f,  (-16.f * PI / 8.f) + (2.f  * PI / 8.f), DAttackRadialSequenceState::Damaging },
                        { 0.4f,  (-2.f * PI) - (1.f * PI / 8.f),          DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from idle to rightAttack (vertical)
                    {
                        { 0.0f,  -8.f * PI / 8.f, DAttackRadialSequenceState::WindUp },
                        { 0.3f,  -2.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.4f,   1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.42f,  1.5f * PI / 8.f, DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 1.f, 0.f)
                }
            }
        )
        , m_attackSimulationStaticData(m_attackSequences, m_attackCircle)
        , m_guardSimulationStaticData(m_attackCircle)
        // 0.875 s × 800 cm/s = 700 cm = 7 m travel distance (T26).
        // 6th arg (T29; narrowed T31) = guardMiddleSectionHalfAngle 0.25 rad (≈14.3°) →
        //   only a near-centre-line guard alignment blocks the projectile, matching the
        //   radial sim's middle-section threshold (DAttackRadialSimulation.h:450 shieldAngle).
        // 7th arg (T30) = innerCircleRadius — the brawler attack circle inner radius, so a
        //   blocked projectile's marker lands on that circle (edge facing the shooter).
        // 8th arg (T30) = indicatorPersistTicks 20 ≈ 0.333 s at 60 Hz (radial guard-hit feel).
        , m_projectileStaticData(800.f, 0.875f, 40.f, 60.f, 50.f, 0.25f,
                                 m_attackCircle.getInnerRadius(), 20)
        // [movement-sim task 11] THE MOVEMENT SUB-SIM'S AUTHORED CONSTANTS, and the ONLY
        // place any of them is spelled. In particular this is the ONE walk-speed literal in
        // the tree (R-P1): 100 cm/s, now the DEFAULT of `movementMaxWalkSpeed` above.
        // ⭐ [movement-sim task 16, 2026-09-08] FIVE OF THEM ARE NOW FORWARDED PARAMETERS —
        // model, walk speed, step period, step speed, gravity. The sim-side walk-speed global
        // and its named-pipe `MoveSpeed` case are DELETED; `OGBrawler.MoveSpeed` survives as a
        // console variable that is read ONCE, at this object's construction (ruling #3), and
        // the named-pipe spelling is now refused loudly rather than ignored.
        //
        // Argument order is architecture §3.2's, grouped by concern:
        //   model
        //   ContinuousAccelBrake  maxWalkSpeed 100, acceleration 2048, braking 2000
        //   Cadence               stepPeriodTicks 20 (= 1/3 s at 60 Hz), stepSpeed 100
        //   attachment            maxSlopeAngleDeg 45 (cosMaxSlope is derived in the ctor)
        //   gravity law           gravity -980 cm/s^2 AUTHORED (the body's own enableGravity
        //                         is FALSE — ruling #16 a), terminalFallSpeed 2000 cm/s
        //   hover geometry        rideHeight 10, snapDistance 40
        //     ⭐ RULING #13, closed 2026-09-06 ON MEASUREMENT (task 9 q3: a 40 cm drop at the
        //     exact edge of probe range — clearance hit 50.000000 == rideHeight + snapDistance —
        //     was still detected and caught in 6 ticks). These SUPERSEDE the revision-6 text's
        //     20 / 150, and BOTH STAND under ruling #28.
        //   hover servo           hoverFrequency 46 rad/s, hoverDampingRatio 0.62,
        //                         hoverMaxAccel 30000 cm/s^2,
        //                         hoverPullDownAccel 0 cm/s^2
        //     ⭐⭐ RULING #29, 2026-09-07 (movement-sim task 57). The servo is ONE-SIDED: at or
        //     below ride height it is task 56's full spring-damper; ABOVE ride height it is a
        //     bounded spring pull and nothing else, capped at hoverPullDownAccel. At the shipped
        //     0 that means gravity and nothing else -- SUPPORTED MEANS HELD UP, NEVER PULLED
        //     DOWN, which is what the user asked for after walking off a ledge under task 56.
        //     ⚠ 0 IS PROVISIONAL, LIKE omega AND zeta: it is a FEEL knob, and the user decides in
        //     PIE whether a step-down at gravity speed (40 cm in ~17 ticks) is right or whether
        //     it wants the 980 that makes it ~12. Read hoverPullDownAccel's comment in
        //     BrawlerMovementSimulation.h before moving it.
        //     ⚠ terminalFallSpeed 2000 IS ALSO STILL OPEN under #29: with the launch gone, a fall
        //     now needs ~1.75 s to reach it instead of ~0.5 s, and the user is considering
        //     3000-4000 (games commonly cap at 30-40 m/s). Not this task's to change.
        //     ⭐⭐ RULING #28, 2026-09-07 (movement-sim task 56). ONE vertical law: gravity on
        //     every tick, a spring-damper servo added while `Supported`. `k = omega^2` and
        //     `c = 2*zeta*omega` are DERIVED in the movement StaticData's constructor.
        //     ⛔ `maxSnapSpeed 400` — ruling #13's dead-beat VELOCITY clamp — IS THE ARGUMENT
        //     THAT WAS REMOVED HERE. Its measured value was 400 (task 54 read the clamp at
        //     399.94 cm/s); the rev-6 text's 150 was superseded by #13 and is not a second
        //     number. There is no dead-beat assignment left for a velocity clamp to bound.
        //     ⚠ omega AND zeta ARE PROVISIONAL FEEL DEFAULTS, NOT MEASURED ONES. The LAW is
        //     ruled; the numbers are the user's to set in PIE, and step 0's
        //     `[Warning][Movement.hover]` line prints them with the discrete critical zeta
        //     beside them so a tuning session can see which side of the ringing threshold it
        //     is on. zeta 0.62 is the DISCRETE critical damping ratio at omega 46
        //     (`1 - omega*dt/2` = 0.6167), chosen by the user 2026-09-07 after the shipped
        //     zeta 1 was MEASURED to ring; read `hoverDampingRatio`'s comment in
        //     BrawlerMovementSimulation.h before changing either value.
        //   knockback             launchDecel 4000, knockbackSpeed 2000 — travel distance is
        //                         the closed form v²/(2a) = 2000²/8000 = 500 cm = 5 m
        //   dash                  dashSpeed 800, dashTicks 12 (0.2 s), dashCancelTick 8
        //   capsule               42 / 96 — MUST equal OGBrawlerUECharacter.cpp:69's
        //                         InitCapsuleSize; the adopt-root factory checkf's agreement
        //                         rather than resizing the authored capsule.
        , m_movementStaticData(movementModel,
                               movementMaxWalkSpeed, 2048.f, 2000.f,
                               movementStepPeriodTicks, movementStepSpeed,
                               45.f, movementGravity, 2000.f,
                               10.f, 40.f,
                               46.f, 0.62f, 30000.f,
                               0.f,
                               4000.f, 2000.f,
                               800.f, 12u, 8u,
                               42.f, 96.f)
    {}

    // Non-copyable / non-movable, compiler-enforced. The sub-StaticData members
    // (m_attackSimulationStaticData / m_guardSimulationStaticData) hold references
    // into this object's own sibling members (m_attackSequences / m_attackCircle);
    // any copy or move would rebind those references to the source instance and
    // dangle once it is destroyed. This is the type-level guarantee behind the
    // "constructed in place once, never moved" ownership invariant documented at
    // ASimulationManagerUImpl::m_staticData (the canonical instance's home).
    StaticData(const StaticData&) = delete;
    StaticData(StaticData&&) = delete;
    StaticData& operator=(const StaticData&) = delete;
    StaticData& operator=(StaticData&&) = delete;

    DAttackCircle m_attackCircle;
    std::vector<DAttackRadialSequence> m_attackSequences;
    dAttackRadialSimulation::StaticData m_attackSimulationStaticData;
    dAttackGuardSimulation::StaticData m_guardSimulationStaticData;
    brawlerProjectileSimulation::StaticData m_projectileStaticData;
    brawlerMovementSimulation::StaticData m_movementStaticData;
};

// [Task 55/60] Execution order validation — declared order must satisfy dependency edges.
// The actual sub-sim integrate calls live in SimulatableBrawler::integrate; this
// assertion is a structural check on the composite's dependency graph.
using ExecutionOrder = std::tuple<
    dAttackMachineSimulation::Dependencies,
    dAttackGuardSimulation::Dependencies,
    dAttackRadialSimulation::Dependencies,
    brawlerProjectileSimulation::Dependencies,
    brawlerMovementSimulation::Dependencies>;
inline constexpr auto executionViolation_ =
    compositeDetail::findFirstViolation<ExecutionOrder>();
using ExecutionOrderDiagnostic_ =
    compositeDetail::DecodeViolation<ExecutionOrder, executionViolation_>;
static_assert(sizeof(ExecutionOrderDiagnostic_) >= 0);

// ---------------------------------------------------------------------------
// [movement-sim task 29, F-G6a] THE ORDERING EDGE THE VALIDATOR ABOVE CANNOT SEE.
//
// findFirstViolation walks `Dependencies`, and a dependency is only visible to it
// when it is DECLARED there. The guard declares only
// `External<const machine::State&>` — yet the projectile and radial sub-sims read
// the guard's POSE THIS TICK, and they read it through the physics adapter
// (`getBodyTransform(hit.bodyId)[0]` is the guard forward in the projectile block
// test; DAttackRadialSimulation does the same for the radial block test). An edge
// that flows through the adapter is invisible to a graph built from types, so
// reordering the integrate blocks would silently make both of them read LAST
// tick's guard facing, with nothing failing to compile and no test failing except
// by luck.
//
// Hence this: the same edge, restated as an index comparison, in the one place
// the declared order lives. It is deliberately a SEPARATE assertion from the
// generic one above rather than an entry in `Dependencies` — declaring a
// dependency the sub-sim does not actually take through the type system would be
// a lie the ownership validator then has to be taught to tolerate.
template <typename T, typename Tuple> struct ExecutionIndexOf_;
template <typename T, typename... Ts>
struct ExecutionIndexOf_<T, std::tuple<Ts...>>
    : std::integral_constant<std::size_t, compositeDetail::indexOfBareType<T, Ts...>()> {};

template <typename T>
inline constexpr std::size_t executionIndexOf_ = ExecutionIndexOf_<T, ExecutionOrder>::value;

static_assert(executionIndexOf_<dAttackGuardSimulation::Dependencies>
            < executionIndexOf_<brawlerProjectileSimulation::Dependencies>,
    "The guard sub-simulation must integrate BEFORE the projectile sub-simulation: the "
    "projectile block test reads the guard's pose off the physics body in the same tick. "
    "This edge flows through the physics adapter, not through Dependencies, so the "
    "generic execution-order validator above cannot see it.");

static_assert(executionIndexOf_<dAttackGuardSimulation::Dependencies>
            < executionIndexOf_<dAttackRadialSimulation::Dependencies>,
    "The guard sub-simulation must integrate BEFORE the radial sub-simulation: the radial "
    "block test reads the guard's pose off the physics body in the same tick. This edge "
    "flows through the physics adapter, not through Dependencies, so the generic "
    "execution-order validator above cannot see it.");

} // namespace simulatableBrawler

OGSIM_OPTIMIZE_ON

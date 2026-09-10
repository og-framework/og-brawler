#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGTypes.h"  // for BodyId
#include <vector>
#include <cstdint>
#include <type_traits>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/common.hpp"        // glm::abs / glm::clamp / glm::max
#include "glm/geometric.hpp"     // glm::dot / glm::cross / glm::normalize / glm::length
#include "glm/trigonometric.hpp" // glm::cos / glm::radians -- SETUP time only, never per tick
#include "glm/exponential.hpp"  // glm::sqrt
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/PhysicsDeclaration.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGSimulation/SpatialQueryResult.h"   // SweepHit -- the attachment probe's result
#include "OGSimulation/OGAssert.h"             // OG_CHECK -- the model-dispatch default arm
#include "OGBrawler/CollisionCategoryConstants.h"
// [movement-sim task 62] `simulatableBrawler::CharacterBindings` is DEFINED IN THE LEAF HEADER
// INCLUDED ON THE NEXT LINE, not in this file any more. It is the one type the machine sub-sim
// wants from here, and keeping it in a header whose only dependency is `BodyId` is what lets the
// machine header have it without dragging in this one. Consumers of THIS header still see the
// type, transitively, so no call site changed.
// ⚠ [movement-sim task 64] THE NAMESPACE IS `simulatableBrawler`, NOT this file's
// `brawlerMovementSimulation`. It read the other way until task 64. THIS SUB-SIM NEVER USES THE
// TYPE — every mention of it left in this file is a comment; movement's capsule id comes from
// its own `RuntimeBindings` below.
#include "OGBrawler/BrawlerCharacterBindings.h"
// ⭐ [movement-sim task 62] THE MACHINE HEADER, INCLUDED OUTRIGHT — the dependency points ONE
// WAY now: movement -> machine. Movement reads the machine's `State` (the flinch freeze, step 3)
// and its `PlayerInput` (the move stick is packed onto the machine slice), so
// `machineFreezesMovement` and `integrate` SPELL those types instead of deducing them.
// Before task 62 this include was impossible: `DAttackMachineSimulation.h` included THIS header
// for `CharacterBindings`, so including it back was a cycle — and `#pragma once` does not turn a
// cycle into an error, it silently leaves one side incomplete depending on which header the
// translation unit entered from, which is the worst available failure mode.
// ⛔ KEEP THE GRAPH ACYCLIC: nothing reachable from `DAttackMachineSimulation.h` may include
// `BrawlerMovementSimulation.h`. Re-verified at task 62 for all seven of its own includes
// (radial sequence, radial sim, sequence id, direction classifier, projectile, inbound hit,
// input sequence) — none of them reach this file.
//
// ⚠ THE INPUT EDGE IS STILL UNDECLARED, and this include does not fix that. `Dependencies`
// below declares `External<const dAttackMachineSimulation::State&>`, so `findFirstViolation`
// validates the STATE edge against `ExecutionOrder`. The `PlayerInput` edge is declared NOWHERE,
// and `InputType = brawlerMovementSimulation::PlayerInput` positively asserts this sub-sim reads
// only its own input slice — untrue since step 3 began reading `machineInput.moveDirectionWorld`.
// What this include buys is that the edge is visible in the signature and CHECKED BY THE
// COMPILER. Teaching the dependency graph about INPUT edges means changing
// `OGSimulation/SimulationDependencies.h`, which every sub-sim shares: a separate framework task.
#include "OGBrawler/DAttackMachineSimulation.h"
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

// Home of the brawler character-movement sub-sim.
//
// [movement-sim task 11, 2026-09-06] THE REAL SUB-SIM. The skeleton (task 1) is gone:
// this sub-simulation now owns the character CAPSULE, decides what it is standing on,
// runs a movement model in a surface-relative 2D frame, hovers at a fixed clearance and
// writes both halves of the body's motion every tick.
//
// ============================ WHO OWNS WHAT (user ruling #14(c)) ============================
// The sim owns `{position, velocity}` and RE-PLACES the body every tick
// (`setBodyTransform` + `setBodyLinearVelocity`). The solver is used for SEPARATION ONLY,
// and its contribution is read back as a POSITIONAL push-out. The captured
// `bodyState.linearVelocity` is NEVER READ — no contact impulse can enter simulation state.
//
//   position   sim, engine-corrected  we write it; the post-solve capture returns
//                                     `position + worldPushOut`; the sim adopts that.
//   velocity   sim, exclusively       model output, plus the ONE vertical law (gravity
//                                     always on, a spring-damper hover servo while
//                                     `Supported` — ruling #28), plus ONE authored contact
//                                     rule (kill the into-contact component, §step 6' below).
//   ground     sim                    hover servo on the `sweep` probe — the capsule does
//                                     not rest in contact, it floats `rideHeight` above.
//   walls,     Chaos, POSITION ONLY   the capsule blocks `world` and other `character`
//   brawlers                          capsules; the solver's push-out is the one thing
//                                     read back. Two equal-mass capsules separate 50/50.
//   rotation   locked                 `lockRotation` on the descriptor is what makes
//                                     `LinearBodyState`'s fabricated identity rotation TRUE.
//
// Why this shape and not "let Chaos integrate forces" (revision 5, retired): a refused or
// unapplied Chaos rewind cannot lose a correction when the body is re-placed from `State`
// on every replay tick. Task 9's q1(iii) measured that a sim-side correction converges
// identically whether Chaos grants the rewind or refuses ~90% of them.
//
// ============================ ADDING A MOVEMENT MODEL ============================
// A movement model is a PURE 2D FUNCTION of the surface frame. It never learns which plane
// it is in: on flat ground the frame is world XY, on a slope it is the slope, on a wall
// (task 20) it will be the wall. To add one:
//
//   1. Add an enumerator to `MovementModel` (append; the value is authored data).
//   2. Write one free function `computeDesiredVelocityUV_<Model>(sd, state, tick, stickUV,
//      currentUV, dt)` beside the two below, and add its `case` to the `switch (sd.model)`
//      in step 3. That switch is the ONLY place `sd.model` may be named — steps 2, 4 and 5
//      are model-agnostic by construction and an acceptance criterion greps for it.
//   3. If the model needs memory, APPEND fields to `State` and APPEND their `SIM_MEMBER`s.
//      ⛔ Never reorder existing ones: the wire layout is positional.
//      ⛔ A model may write ONLY its own `State` slice, and may not name `bodyState` —
//      position is the composition step's business, not the model's.
//   4. Add an LLT file / cases under `[BrawlerMovement]`.
//
// ⛔⛔ STANDING RULE ON PLAYER INPUT (lead, 2026-09-06) — A NEW PER-TICK INPUT FIELD IS A
// BIT IN A FLAGS BYTE, NEVER A NEW `bool` MEMBER. An input byte is about TEN TIMES more
// expensive than a state byte, because it is multiplied across every ring entry in the
// packet-budget scenario: `holdGuard`, one `bool`, moved the ring entry stride 81 -> 82 B and
// cost 10.264 B of the join-alone margin at the character cap, leaving **27.352 B of slack
// above the half-entry floor — 2.54 more input bytes before the half-entry floor guard in
// `RoundVsPacketBudgetTest.cpp`'s "the pre-diet cap is 4" case goes RED.** One bool spent a
// quarter of the remaining budget. Jump (task 21), dash (31), wall-grab (20) and ski-tuck
// (48) are all coming; as four `bool`s they blow that fence twice over, as four bits in one
// byte they cost nothing.
// ⭐ [movement-sim task 51, 2026-09-06] THAT SHAPE IS NOW THE SHIPPED ONE: `PlayerInput` is
// `uint8_t flags`, `holdGuard` is bit 0, and bits 1-7 are reserved for those four. It moved
// ZERO wire bytes — `bool` and `uint8_t` are both 1 B — so every pin in
// `RoundVsPacketBudgetTest.cpp` and `SimulatableBrawlerTest.cpp` was re-quoted UNCHANGED.
// That is precisely why it was worth landing BEFORE task 14, the field's only writer: a
// re-layout that moves no bytes is verifiable in isolation, and 14 is then written against
// the final shape instead of pinning a `bool` it would have to un-pin. The packing rule and
// the constant that carries it live AT THE TYPE, on `PlayerInput` below. The full
// arithmetic, with every term, is in `RoundVsPacketBudgetTest.cpp`'s pre-diet table block.
//
// Constraints on everything in this header: `glm` only (no `<cmath>`, no engine types), and
// NO PER-TICK TRANSCENDENTAL — the single `cos` is in `StaticData`'s constructor, which runs
// once per session. `OGBLOG_G` fires only on surface-kind CHANGES, Cadence commits and
// teleports; never unconditionally per tick.
namespace brawlerMovementSimulation
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Which movement law turns the stick into a surface-relative 2D velocity. Read once per
// session from `StaticData` so every peer runs the same one; it is NOT per-tick state and
// never travels on the wire.
enum class MovementModel : uint8_t
{
    // CMC-like: accelerate toward `stick * maxWalkSpeed` at `acceleration`, brake toward
    // zero at `brakingDeceleration`. Continuous, no committed direction.
    ContinuousAccelBrake = 0,
    // Direction is COMMITTED every `stepPeriodTicks` and held at a constant `stepSpeed`
    // in between — the "steps, not a joystick" feel.
    Cadence = 1
};

// ⭐⭐ WHETHER THE CHARACTER IS BEING HELD UP THIS TICK — movement-sim TASK 56 / user ruling
// #28, 2026-09-07. It replaces `SurfaceKind {Airborne, Floor}`, and it is NOT a rename of the
// same idea. `SurfaceKind` named a GEOMETRIC FACT (a walkable probe hit inside the band);
// this names a DECISION about support. They differ exactly where the old name lied: during a
// jump's ascent (task 21) the probe still finds walkable floor underneath, and the servo must
// nevertheless be OFF.
//
// ⛔ SO THE DETACH GATE LIVES IN STEP 2'S CLASSIFICATION, NOT IN STEP 4.
// `detachesFromSupport` below is the one predicate that can turn a walkable probe hit into
// `Unsupported`; step 4 only READS the answer. Putting the gate in step 4 would re-create the
// value-that-is-not-true this rename exists to remove, and would give the vertical law a second
// place to disagree with the flags byte a correction restores.
//
// Stored in `State::flags` bits 1-2 — the SAME TWO BITS `SurfaceKind` used, so this moves ZERO
// wire bytes and the enum may still grow to 4 values before the field has to widen.
enum class SupportState : uint8_t
{
    // Gravity ONLY. Ballistic flight, a jump's ascent (task 21), clearance beyond the probe
    // band, or a hit whose normal failed the walkable test. The servo term does not run.
    Unsupported    = 0,
    // The servo term runs, along `up` (user ruling #26 a).
    Supported      = 1,
    // RESERVED for the 45°-75° band (task 48), where the servo axis follows `n` per the
    // tasks-20/48 carry-forward. ⛔ v1 NEVER PRODUCES IT: step 2's walkable test is a single
    // `cosMaxSlope` comparison, so anything steeper than `maxSlopeAngleDeg` classifies
    // `Unsupported`. It is declared now so the wire field's value range is settled once,
    // rather than widening the flags byte later.
    SupportedSteep = 2
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⭐ [movement-sim task 62] `CharacterBindings` MOVED OUT of this header, to
// `OGBrawler/BrawlerCharacterBindings.h` — included at the top, so every consumer of this
// header still sees it and not one call site changed; its full provenance comment moved with
// it. It left because the machine sub-sim needs the type and must NOT include this header.
// ⭐ [movement-sim task 64] AND THEN THE NAMESPACE FOLLOWED THE TYPE: it is
// `simulatableBrawler::CharacterBindings` now. `brawlerMovementSimulation` was collateral from
// T35's file move, not a modelling call — this sub-sim never uses the type, and every real
// consumer sits in or under `simulatableBrawler`, where the struct started.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⭐⭐ THE SIM'S FIXED STEP, AND IT IS NOT AUTHORED HERE. `Config/DefaultEngine.ini`'s
// `AsyncFixedTimeStepSize=0.016667` is the authority; `integrate` is handed the real `dt` and
// uses THAT, never this. This constant exists for exactly one purpose: the discrete stability
// bound on the hover gains has to be checked where the gains are AUTHORED (`StaticData`'s
// constructor, once per session), and `dt` is not in scope there.
// ⚠ If the sim clock ever moves off 60 Hz this constant must move with it, or the check below
// silently guards the wrong region. That is why it is named, commented, and pinned by an LLT
// (`HoverGainsAreStableAtTheSimStep`) against the rig's own `kDt` rather than left a literal.
inline constexpr float kNominalSimStepSeconds = 1.f / 60.f;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The movement sub-sim's authored constants. Held by value as
// simulatableBrawler::StaticData::m_movementStaticData, which PhysicsDeclaration::
// staticDataOf returns to the generic registration fold.
//
// ⭐ R-P1: the walk-speed literal is spelled ONCE, at the construction site in
// SimulatableBrawlerTypes.h — not here, not in the UE layer, not in the CMC. Task 16
// replaces the tunables with one-time cvar reads; nothing else about this type changes.
class StaticData
{
public:
    StaticData(MovementModel model,
        float maxWalkSpeed, float acceleration, float brakingDeceleration,
        uint32_t stepPeriodTicks, float stepSpeed,
        float maxSlopeAngleDeg, float gravity, float terminalFallSpeed,
        float rideHeight, float snapDistance,
        float hoverFrequency, float hoverDampingRatio, float hoverMaxAccel,
        float hoverPullDownAccel,
        float launchDecel, float knockbackSpeed,
        float dashSpeed, uint32_t dashTicks, uint32_t dashCancelTick,
        float capsuleRadius, float capsuleHalfHeight)
        : model(model)
        , maxWalkSpeed(maxWalkSpeed)
        , acceleration(acceleration)
        , brakingDeceleration(brakingDeceleration)
        , stepPeriodTicks(stepPeriodTicks)
        , stepSpeed(stepSpeed)
        , maxSlopeAngleDeg(maxSlopeAngleDeg)
        // The ONE transcendental in this header, and it is SETUP time. Step 2 compares
        // `glm::dot(probe.normal, up) >= cosMaxSlope` — a dot product against the servo's own
        // axis (ruling #26 a) — so the walkable test costs no trigonometry per tick.
        , cosMaxSlope(glm::cos(glm::radians(maxSlopeAngleDeg)))
        , gravity(gravity)
        , terminalFallSpeed(terminalFallSpeed)
        , rideHeight(rideHeight)
        , snapDistance(snapDistance)
        , hoverFrequency(hoverFrequency)
        , hoverDampingRatio(hoverDampingRatio)
        , hoverMaxAccel(hoverMaxAccel)
        , hoverPullDownAccel(hoverPullDownAccel)
        // DERIVED ONCE, HERE, from the ω/ζ pair — the per-tick law multiplies and never squares.
        // k = ω², c = 2ζω: the standard second-order form, so a tuner moves a FREQUENCY and a
        // DAMPING RATIO rather than two gains whose units nobody remembers.
        , hoverStiffness(hoverFrequency * hoverFrequency)
        , hoverDamping(2.f * hoverDampingRatio * hoverFrequency)
        , launchDecel(launchDecel)
        , knockbackSpeed(knockbackSpeed)
        , dashSpeed(dashSpeed)
        , dashTicks(dashTicks)
        , dashCancelTick(dashCancelTick)
        , capsuleRadius(capsuleRadius)
        , capsuleHalfHeight(capsuleHalfHeight)
    {
        // ⭐⭐ THE DISCRETE STABILITY BOUND, CHECKED WHERE THE GAINS ARE AUTHORED — because the
        // alternative is finding it in PIE, and three people derived it wrong first.
        //
        // ⛔ THE OBVIOUS BOUND IS THE WRONG ONE. "Explicit Euler is stable for ω·dt < 2" gives
        // `hoverFrequency < 120` at 60 Hz, and that admits gains that DIVERGE. Two facts kill it:
        //   1. THE STEP IS SEMI-IMPLICIT (symplectic) EULER, not explicit. Step 4 computes the new
        //      velocity and step 5 hands it to the engine, which integrates position with the NEW
        //      velocity (`x += v'·dt`) — a different recurrence with a different stability region.
        //   2. THE DAMPING TERM IS PART OF THE BOUND. A frequency cap alone cannot express a
        //      two-parameter region, and ζ moves the limit by more than 30 %.
        //
        // THE ACTUAL RECURRENCE, with y = clearance − rideHeight, a = c·dt, b = (ω·dt)²:
        //     [y'; v'] = [[1 − b, dt(1 − a)], [−k·dt, 1 − a]] [y; v]
        //     trace T = 2 − a − b,  determinant D = 1 − a
        // Jury's criterion (|D| < 1 and |T| < 1 + D) collapses to  b + 2a < 4  and  0 < a < 2,
        // and the first implies the second. With W = ω·dt, b = W² and a = 2ζW that is:
        //
        //          ⭐ W² + 4ζW < 4 ⭐           W = hoverFrequency · dt
        //
        // → ζ = 1.00 admits ω < 49.71;  ζ = 0.62 admits ω < 66.79;  ζ = 0.50 admits ω < 74.16.
        // MEASURED against the recurrence itself, not just derived: ω = 60 ζ = 1 diverges
        // (1, 1, 2, 3, 5, 8, 13 …) and ω = 119 ζ = 1 reaches 10⁶ in eight ticks. Both PASS
        // `ω < 120`. The shipped pair sits at W² + 4ζW = 2.489.
        //
        // ⚠ STABLE IS NOT THE SAME AS SMOOTH — read `hoverDampingRatio`'s comment below for the
        // monotone region, which is tighter and is where the feel lives.
        const float W = hoverFrequency * kNominalSimStepSeconds;
        OG_CHECK(W * W + 4.f * hoverDampingRatio * W < 4.f,
            "brawlerMovementSimulation::StaticData - hover gains outside the discrete stability "
            "region W^2 + 4*zeta*W < 4 (W = hoverFrequency * the 60 Hz sim step). The servo would "
            "diverge. Lower hoverFrequency or raise hoverDampingRatio.");
        OG_CHECK(hoverFrequency > 0.f && hoverDampingRatio > 0.f && hoverMaxAccel > 0.f,
            "brawlerMovementSimulation::StaticData - hover gains must be positive");
        // ⛔ THE PULL-DOWN KNOB IS A MAGNITUDE, AND ZERO IS ITS SHIPPED VALUE — so this is the one
        // hover constant whose check is `>= 0` rather than `> 0`. A negative value would turn the
        // one-sided arm into a spring that pushes UP while the character is above ride height,
        // which is the opposite of the whole ruling.
        OG_CHECK(hoverPullDownAccel >= 0.f,
            "brawlerMovementSimulation::StaticData - hoverPullDownAccel is a magnitude and "
            "must not be negative (0 = gravity and nothing else above ride height)");
    }

    StaticData(const StaticData&) = default;

    MovementModel model;

    // SIM-LOCAL, NOT SERIALIZED and deliberately not a constructor parameter.
    // ⭐ `true` SINCE TASK 15: step 5 re-places the body every tick and this sub-simulation
    // IS the character's locomotion. `false` computes the whole of `State` but skips the two
    // adapter writes — the passenger arm, which now exists only for the LLT rig, and which the
    // rig PINS for itself (`Rig()` in `BrawlerMovementSimulationTest.cpp`) rather than
    // inheriting from here.
    //
    // ⛔⛔ THIS FLAG IS HALF OF A PAIR, AND THE PAIR IS LOAD-BEARING. `drivesBody` only says
    // what the SIM does; what the CMC does is decided by `PhysicsSetup::body.simulatePhysics`.
    // ⭐ BOTH WERE FLIPPED TOGETHER AT TASK 15, AND NEITHER MAY BE FLIPPED BACK ALONE:
    //   * `simulatePhysics = true` beside `drivesBody = false` leaves NOBODY driving the
    //     capsule. UE 5.6's `UCharacterMovementComponent` early-returns whenever
    //     `UpdatedComponent->IsSimulatingPhysics()`, on BOTH its sync and its async path, and
    //     the component the factory adopts IS the CMC's `UpdatedComponent`
    //     (`SimulationManagerUImpl.cpp` hands the factory `character->GetCapsuleComponent()`).
    //     The CMC switches itself off, step 5 is skipped, and engine gravity is off too: an
    //     inert no-gravity rigid body sitting where it spawned. THE PLAYER CANNOT MOVE.
    //   * `drivesBody = true` beside `simulatePhysics = false` leaves TWO AUTHORITIES — the sim
    //     writing the body while the CMC still drives the same component.
    //   Read `PhysicsSetup::body`'s comment before changing either one.
    // ⛔ The TELEPORT seed (step 0) writes the body regardless of this flag: a respawn must
    // move the capsule.
    //
    // ⚠ WHY A DEFAULTED MEMBER RATHER THAN A CTOR PARAMETER. This is a one-way architectural
    // switch, not a tunable: nothing in production should ever set it false again. The ctor
    // already takes 19 positional arguments, and a 20th trailing `bool` after a run of `float`s
    // is a live miswiring hazard for the tasks that extend that list (16, 20, 48). A test that
    // wants the passenger arm assigns it by name on its own instance, which reads better at the
    // call site than a positional `false` nineteen arguments deep.
    bool drivesBody = true;

    // ContinuousAccelBrake
    float maxWalkSpeed, acceleration, brakingDeceleration;
    // Cadence (20 ticks = 1/3 s at the 60 Hz sim clock)
    uint32_t stepPeriodTicks;
    float stepSpeed;

    // Attachment. `cosMaxSlope` is derived from `maxSlopeAngleDeg` in the ctor and is the
    // form step 2 actually uses; both are kept so a debugger shows the authored degrees.
    float maxSlopeAngleDeg;
    float cosMaxSlope;

    // The SIM's gravity law, per character. NEGATIVE (world −Z).
    // ⭐ [task 56 / ruling #28] APPLIED ON EVERY TICK, WITH NO BRANCH THAT TURNS IT OFF — the
    // servo HOLDS THE BODY UP AGAINST IT while `Supported` rather than replacing it, which is
    // what a suspension does and what makes the vertical velocity continuous at the support
    // boundary. It used to run only while `Airborne`, and the seam between that branch and the
    // dead-beat one was the defect ruling #28 removed.
    // ⛔ The BODY's `enableGravity` is FALSE — see PhysicsSetup. Under ruling #14(c) engine
    // gravity would double-apply against this term and corrupt the very push-out step 6' reads.
    float gravity;
    float terminalFallSpeed;   // POSITIVE magnitude; the fall speed is clamped to −this

    // Hover geometry. Steady state is `clearance == rideHeight` with `velocityUp == 0` —
    // CONTACT-FREE, which is why no standing contact manifold exists for the solver to fight.
    // ⭐ Ruling #13, closed 2026-09-06 ON MEASUREMENT (task 9 q3), and these two STAND: a 40 cm
    // drop at the exact edge of probe range was still detected and caught. They superseded the
    // revision-6 text's 20/150.
    float rideHeight, snapDistance;

    // ⭐⭐ THE HOVER SERVO'S GAINS, IN FEEL UNITS — movement-sim TASK 56 / ruling #28.
    // ⛔ `maxSnapSpeed` (400 cm/s, ruling #13's dead-beat clamp) IS GONE, and the whole law it
    // clamped went with it. It was a VELOCITY ceiling on a law that ASSIGNED velocity from
    // position error; there is no assignment left to clamp. `hoverMaxAccel` is its replacement in
    // role only — an ACCELERATION ceiling on a law that integrates. A stale ini or cvar naming
    // `maxSnapSpeed` must be rejected loudly rather than silently ignored; there is no cvar/ini
    // path in the tree yet (task 16 builds it), so that obligation is recorded HERE and belongs
    // to task 16 when it lands.
    //
    // ω in rad/s. `hoverStiffness = ω²` below. Bigger = snappier and less forgiving.
    float hoverFrequency;
    // ζ, dimensionless. `hoverDamping = 2ζω` below.
    //
    // ⭐⭐ ζ = 1 IS *NOT* CRITICAL DAMPING HERE, AND THAT IS THE WHOLE POINT OF THIS COMMENT.
    // ζ = 1 is the continuous-time answer. This servo is a DISCRETE recurrence, and the value
    // that makes ITS response monotone is
    //
    //          ⭐ ζ_crit = 1 − ω·dt / 2 ⭐            (0.6167 at ω = 46, 60 Hz)
    //
    // — the ζ at which the two eigenvalues coincide (T² = 4D ⇔ (a + b)² = 4b ⇔ ζ ≥ 1 − W/2).
    // MEASURED on a 1 cm perturbation, error per tick:
    //     ζ = 1.00   .412 .483 .161 .238 .057 .120 .016 .062   ← alternates sign: it RINGS
    //     ζ = 0.62   .412 .141 .045 .014 .004 .001 .000 .000   ← monotone, 95 % in ~4 ticks
    // At ζ_crit the repeated eigenvalue is λ = 1 − W, so the error decays ×λ per tick.
    //
    // ⚠ AND MONOTONE NEEDS λ > 0 AS WELL, WHICH IS A SECOND, TIGHTER BOUND: λ = 1 − ω·dt > 0
    // ⇒ ω < 1/dt = 60 rad/s at 60 Hz. Critical damping buys the FASTEST decay, not a monotone
    // one — ω = 70 (ζ_crit 0.417) and ω = 80 (ζ_crit 0.333) are both critically damped and both
    // alternate sign every tick. ⭐ SO THE SAFE TUNING REGION IS `ω < 60` WITH `ζ ≈ 1 − ω·dt/2`,
    // not a point. ω = 46 sits inside it with λ = +0.233.
    // ⚠ RECORDED AS A LANDMARK AND NOT A RECOMMENDATION: ω = 60, ζ = 0.5 puts BOTH eigenvalues
    // at exactly 0 — a dead-beat, reached in one tick. It is knife-edge (λ crosses zero there)
    // and it reintroduces the abruptness ruling #28 moved away from. Do not ship it.
    //
    // ⚠ THE VALUES THEMSELVES ARE UNMEASURED FEEL. The LAW is ruled; ω and ζ are provisional
    // defaults the user tunes in PIE and then records in ruling #28. Step 0 logs both, and the
    // ζ_crit for the configured ω beside them, so a tuning session can see which side it is on.
    float hoverDampingRatio;
    // The servo term's acceleration ceiling, cm/s². A fall that outruns it bottoms out into
    // contact and is caught by depenetration plus step 6' — the same safety net the dead-beat's
    // velocity clamp had under it.
    // ⚠ The TOTAL vertical acceleration is `gravity + clamp(servo, ±hoverMaxAccel)`, so the true
    // per-tick velocity bound is `(hoverMaxAccel + |gravity|)·dt`, not `hoverMaxAccel·dt`. The
    // feed-forward is inside the clamped term; that is what buys the exact steady state, and it
    // is what makes the bound asymmetric by exactly one `gravity`.
    float hoverMaxAccel;

    // ⭐⭐ THE ONE-SIDED SERVO'S ONE KNOB — movement-sim TASK 57 / USER RULING #29, 2026-09-07.
    // A CEILING ON THE DOWNWARD PULL, in cm/s², applied ONLY above ride height. `0` — the shipped
    // default, and the user's own request — means the vertical law above ride height is
    // **gravity and nothing else**: SUPPORTED MEANS HELD UP, NEVER PULLED DOWN.
    //
    // ⚠ IT IS NOT `hoverMaxAccel`'s twin. `hoverMaxAccel` bounds the servo's authority in BOTH
    // directions and stays 30 000; this bounds the SPRING TERM ALONE, and only on the arm where
    // the spring would act WITH gravity. Raising it makes step-downs snappier (980 ⇒ 2 g total
    // ⇒ a 40 cm step recaptured in ~12 ticks instead of ~17); it does not make landings firmer,
    // it does not change the upward bound, and it can never lift the character.
    // ⚠ WHAT IT COSTS AT 0, MEASURED (task 57): the character rides a hair below ride height while
    // walking, because a tick that lands a float hair ABOVE ride height gets gravity alone and
    // falls `|gravity|*dt²` = 0.272 cm before the servo takes it back. That ripple is one tick
    // deep and is the price of the one-sided law; the design predicted it (design_ledge_fall_
    // and_landing.md §1 "Why no chatter at e = 0") and PIE decides whether it is visible.
    float hoverPullDownAccel;
    // DERIVED in the constructor from the pair above. k = ω², c = 2ζω. Not constructor
    // parameters: a tuner must not be able to author a k and a c that no (ω, ζ) produces, which
    // is exactly how the stability check above would be bypassed.
    float hoverStiffness, hoverDamping;

    // Knockback / launch (task 27). Travel distance is the closed form
    // knockbackSpeed² / (2·launchDecel) = 2000² / 8000 = 500 cm = 5 m — the user's
    // "thrown 5 metres and then quickly come to a stop".
    float launchDecel, knockbackSpeed;

    // Dash / dodge (task 31). `dashSpeed` is ASSIGNED on the entry tick, never added.
    float dashSpeed;
    uint32_t dashTicks, dashCancelTick;

    // MUST equal the ACharacter's authored capsule (OGBrawlerUECharacter.cpp:69
    // InitCapsuleSize(42.f, 96.f)). The adopt-root factory path does not resize the
    // capsule — it `checkf`s that the descriptor AGREES with it.
    float capsuleRadius, capsuleHalfHeight;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⭐⭐ THE GROUND PROBE'S LATERAL INSET — movement-sim TASK 54, 2026-09-07, AND IT IS A
// DEFECT FIX, NOT A TUNING KNOB. The user reported, in PIE: *"driving a character into a
// static wall, the character gets pushed into the ground and then pops out and over corrects
// into the air, over and over again."* Client `_3` logged **755 `[Movement.surface]`
// transitions** in one session and **every single `-> Airborne` line read `clearance=0.000`**.
//
// ⛔ THE CAUSE WAS THIS DESCRIPTOR, and it is upstream of step 6' and of the servo — the two
// places the mechanism was looked for first, and three separate hypotheses about them were
// MEASURED WRONG (impl/impl_notes_seam_54.md §1.4). The probe used to be a capsule the SAME
// SIZE as the body at the body's OWN pose, so a body pressed against a wall made the PROBE
// overlap that wall. `ChaosSpatialQueryAdapter::sweep` sets `bFindInitialOverlaps = true`
// (:543-544) and then takes the MINIMUM-`Time` blocking hit (:564-571) — its own comment says
// *"An initial overlap comes back with Time == 0, so it naturally wins"* — so the wall BEAT
// the floor. Its normal is horizontal, step 2's `walkable` test failed, and the character was
// classified `Airborne` **while standing on the floor**. Gravity then ran (the sink), and the
// next Floor tick's clamped `maxSnapSpeed` correction was retained by the airborne branch into
// a ballistic launch (the pop). Measured, on the shipped `integrate` against an analytic
// floor+wall: 19 flips / 300 ticks and a `pos.z` excursion of 96 → 181 cm.
//
// ⭐ [movement-sim task 56] THE SECOND HALF OF THAT MECHANISM — the retention line — IS NOW GONE
// TOO: ruling #28 replaced the two vertical laws with one, so there is no dead-beat correction
// left to be retained as momentum. ⛔ THAT DOES NOT MAKE THIS SHRINK REMOVABLE, and the reasoning
// is not a judgement call: the shrink is what stops a touched wall from WINNING THE SWEEP at
// fraction 0, which is upstream of the vertical law entirely. A misclassified `Unsupported` tick
// under ruling #28 still switches the servo off and lets gravity pull the body into the floor —
// a softer sink with no pop, which is a worse defect to diagnose, not a better one.
//
// ⭐ THE FIX IS UE'S OWN `UCharacterMovementComponent::ComputeFloorDist` PATTERN: inset the
// floor probe from the body's silhouette so a body IN CONTACT with a wall does not overlap it.
//
// THE ARITHMETIC, and it depends on a convention that had to be ESTABLISHED rather than
// assumed — the other reading gives a different offset. `CapsuleGeometry::halfHeight` is the
// **TOTAL** half-height (centre to tip, hemisphere included). It carries no comment of its
// own, so both of its consumers were followed to an engine API Epic documents:
//   * the PROBE path — `ChaosSpatialQueryAdapter::registerVolume` calls
//     `FCollisionShape::MakeCapsule(radius, halfHeight)`; UE 5.6's `CollisionShape.h` says
//     *"Note: This is the full half-height (needs to include the sphere radius)"*.
//   * the BODY path — `ChaosPhysicsFactory` calls `UCapsuleComponent::InitCapsuleSize` and
//     `checkf`s against `GetUnscaledCapsuleHalfHeight()`; UE 5.6's `CapsuleComponent.h`
//     documents that field as *"Half-height, from center of capsule to the end of top or
//     bottom hemisphere."*
// So the body's bottom tip is at `centre.z - capsuleHalfHeight`, and for a probe of radius
// `r - s` and total half-height `hh - s` whose bottom tip must land in the SAME place:
//     probeCentre.z - (hh - s) == centre.z - hh   ⇒   probeCentre.z == centre.z - s
// ⇒ shrink by `s` AND drop by `s`. The probe's cylinder is untouched (its half-length is
//   `(hh - s) - (r - s) == hh - r`); only the hemisphere shrinks.
//
// ⚠ WHAT `s` BUYS AND WHAT IT COSTS — all three are for PIE to confirm, NOT measured in-engine:
//   + `s` cm of immunity: a body exactly touching a wall clears the probe by `s`.
//   − ground detection narrows by `s`: the capsule may overhang a ledge edge by up to `s` cm
//     before the probe stops finding floor.
//   − on a SLOPE the probe reads `s * (secθ - 1)` cm MORE clearance than the body has, so the
//     character settles that much lower: 0 flat, 0.155 cm at 30°, 0.414 cm at the 45°
//     `maxSlopeAngleDeg` cap. Second-order against the `rideHeight * cosθ` perpendicular gap
//     step 4 already documents (7.07 cm at 45°), and one-way.
//     ⛔ NOT removable by a different offset: zeroing it needs `offset.z = -s / n.z`, which is
//     slope-dependent, and a static descriptor cannot express it. It is the price of this fix.
//   `BrawlerMovement.GroundProbeSlopeBiasIsTheShrinkTerm` pins that closed form, and
//   `...MeasuresTheBodysClearanceOnFlatGround` pins the flat-ground reading as EXACT.
//
// ⛔ THIS IS THE PRAGMATIC FIX, NOT THE STRUCTURAL ONE, AND THE LEAD RULED IT KNOWING THAT
// (2026-09-07). `s` only buys `s`: a body the solver leaves genuinely PENETRATING a wall by
// more than `s` overlaps the probe again. The structural answer is for `sweep` to return the
// nearest WALKABLE blocking hit (or more than one hit), which needs a `SpatialQueryAdapter` /
// `SweepHit` change that FOUR sub-simulations share — so it gets its own scoped task rather
// than riding a defect fix. Do not fold it in here; do not delete this paragraph either.
inline constexpr float kProbeShrink = 1.f;

// All physics setup descriptors for the movement simulation.
//
// THE CHARACTER CAPSULE ITSELF, adopted — not a body this sub-sim creates. `isRoot` sends
// the factory down the adopt path (task 8): it configures the ACharacter's existing capsule
// component and asserts the descriptor's dimensions agree with the authored ones, so
// `bindings.ownBodyId` IS the capsule body id.
struct PhysicsSetup
{
    static inline const PhysicalObjectDescriptor body{
        BodyDescriptor{
            // ⭐⭐ TRUE SINCE TASK 15, and PAIRED WITH `StaticData::drivesBody`.
            // The component this descriptor adopts is the ACharacter's capsule, which is also
            // the CMC's `UpdatedComponent`. UE 5.6's CMC early-returns on
            // `UpdatedComponent->IsSimulatingPhysics()` in BOTH its synchronous and its async
            // path, so setting this true does not merely ADD physics — it SWITCHES THE CMC OFF.
            // ⭐ THAT IS NOW THE INTENT, not a side effect: task 15 retired the CMC from the
            // gameplay path (`MOVE_None`, component tick disabled, `bAutoActivate = false`, and
            // the `Move()` / `AddMovementInput` binding deleted) and flipped `drivesBody = true`
            // in the same change, so the movement sub-simulation takes over as the capsule's one
            // authority. The flags are applied here, by the factory's adopt-root pass, not in
            // the character's constructor.
            // ⛔ NEITHER MAY BE FLIPPED BACK ALONE. `simulatePhysics = false` here while
            // `drivesBody` stays true gives TWO authorities on one component; `simulatePhysics`
            // true with `drivesBody` false gives NONE — with `enableGravity` false the capsule
            // would not even fall, just a shovable no-gravity body, and the player could not
            // move at all. It would fire the moment a brawler registers; it is not latent.
            // ⭐ THE KINEMATIC CAPTURE NOTE, kept for provenance: while this shipped `false`,
            // `FConstGenericParticleHandle::GetP()` falling back to X/R for a non-dynamic
            // particle (`ChaosPhysicsBodyAdapter.h`, tasks 46/47) is what made the passenger arm
            // observable. The body is dynamic now, so P is the integrated pose and the capture
            // reads it directly.
            // ⚠ `p.AsyncCharacterMovement = 1` was DELETED from Config/DefaultEngine.ini by
            // task 15 — the async CMC path it enabled has no gameplay role left to play.
            .simulatePhysics = true,
            // ⛔ FALSE, and it is load-bearing. Gravity is the SIM's law (StaticData::gravity,
            // applied on EVERY tick since task 56). With the body re-placed from State every tick,
            // engine gravity would double-apply against that term AND perturb the post-solve
            // position the push-out in step 6' is measured from.
            .enableGravity = false,
            // ADOPT the ACharacter's own capsule instead of creating a body under it.
            .isRoot = true,
            // The upright capsule. This is also the flag that makes `LinearBodyState`'s
            // dropped rotation sound rather than a lie (PhysicsBodyState.h's CHOICE RULE).
            .lockRotation = true,
            // The engine RE-SOLVES this body from the pushed anchor state on a replay: it
            // is the one body in the game whose separation we read back, so replaying its
            // recorded motion would discard exactly the information step 6' consumes.
            .resimPolicy = BodyResimPolicy::Resimulate
        },
        {   // shapes
            ShapeDescriptor{
                CapsuleGeometry{42.f, 96.f},
                CollisionCategories::single(collisionCategory::character),
                // Ruling #5 (closed 2026-09-03) = BLOCK. Brawler-vs-brawler separation is
                // the solver's, mass-ratio 50/50, read back as position only.
                // ⚠ SAFE ONLY BECAUSE BOTH CATEGORIES ARE MAPPED. `world` (4) landed with
                // task 39 and `character` (5) with task 43, at BOTH construction sites in
                // SimulationManagerUImpl.cpp. An unmapped category resolves to
                // ECollisionChannel(0), which IS ECC_WorldStatic — a silent WorldStatic
                // block. Task 40's `[SpatialQuery.UnmappedCategory]` diagnostic is the
                // regression guard; it must not fire for category 4 or 5.
                collisionCategory::worldAndCharacter
            }
        }
    };

    // THE ATTACHMENT PROBE — one capsule volume, swept straight down
    // `rideHeight + snapDistance` in step 2. It searches `world` ONLY: other brawlers must not
    // be ground.
    //
    // ⭐ [movement-sim task 54] IT IS INSET `kProbeShrink` FROM THE BODY'S SILHOUETTE AND
    // DROPPED BY THE SAME AMOUNT, so its bottom tip coincides with the body's and every
    // `clearance` reading on flat ground is UNCHANGED. Read `kProbeShrink`'s comment above
    // before touching either term — the two move together or the probe stops measuring what
    // step 2 believes it measures, and the shrink is what stops a touched WALL from winning
    // this sweep at fraction 0.
    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& sd)
    {
        // Drop the volume by exactly the shrink: `probeBottom == bodyBottom` (see the
        // derivation above). `offsetTransform` is applied by the adapter as
        // `transform * offsetTransform` (ChaosSpatialQueryAdapter.cpp:503).
        glm::mat4 probeOffset(1.f);
        probeOffset[3] = glm::vec4(0.f, 0.f, -kProbeShrink, 1.f);

        return std::vector<QueryVolumeDescriptor>{
            QueryVolumeDescriptor{
                CapsuleGeometry{sd.capsuleRadius - kProbeShrink,
                                sd.capsuleHalfHeight - kProbeShrink},
                collisionCategory::worldOnly,
                probeOffset,
                collisionCategory::queryRouting
            }
        };
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The one shared definition lives in OGSimulation/PhysicsDeclaration.h. The
// PhysicsDeclaration concept requires `same_as<PhysicsRuntimeBindings&>`, so a
// field-identical per-sim copy is a DISTINCT type and does not conform; this
// alias keeps every existing `brawlerMovementSimulation::RuntimeBindings`
// spelling valid while making the type the shared one.
//
// For THIS declaration `parentBodyId == ownBodyId`: the adopted capsule is its own root.
using RuntimeBindings = PhysicsRuntimeBindings;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⭐ THE SERVO'S AXIS — THE ONE PLACE THE UP DIRECTION IS SPELLED IN THIS HEADER.
//
// ⛔ USER RULING #26 = (a), 2026-09-06: the hover servo MEASURES and ACTUATES on world-up.
// Everything that needs "up" reads this constant, or the `up` local step 2 seeds from it:
// the probe direction, the walkable test, the airborne frame fallback, `surfaceNormal`'s
// default below and step 5's support term. There is deliberately no second `(0, 0, 1)` in
// this file — `grep "0\.f, 0\.f, 1\.f"` returning only this line is a task-49 AC.
//
// WHY A NAME AND NOT A LITERAL: tasks 20 (wall run) and 48 (ski) will need the axis to
// follow `SupportState` — a world-up correction changes the NORMAL gap by only `cosθ`, which
// is exactly 0 on a wall — and that stays ONE BRANCH on step 2's local only while the axis
// has a single feeding point. ⛔ DO NOT BUILD THAT BRANCH HERE: every v1 surface is `Supported`,
// the user ruled (a) for the surfaces that exist, and widening a ruling is not this file's
// to do. It is recorded as backlog task 49's carry-forward to tasks 20 / 48.
inline constexpr glm::vec3 kWorldUp{0.f, 0.f, 1.f};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// OFF-WIRE per-tick scratch: visualization and debugging only. Nothing in `integrate`
// READS these — they are written at the end of the step they describe, so a consumer that
// started depending on one would be depending on last tick's value without saying so.
class DerivedState
{
public:
    glm::vec3 surfaceNormal{kWorldUp};       // the frame's `n` this tick — fed from the one axis
    glm::vec3 lastProbePoint{0.f};           // where the attachment sweep hit, world
    glm::vec3 lastPushOut{0.f};              // the engine's positional correction, step 6'
    uint8_t   lastSupportState = 0;          // SupportState, as a byte for the viz layer
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// [movement-sim task 11] ONE BIT, and it is a MOVEMENT concern rather than a guard one:
// holding guard freezes locomotion EXACTLY on the tick it is held (the genre's hitstop
// rule), which is a property of this sub-sim's step 1, not of the guard shield.
//
// ⭐⭐ [movement-sim task 51] THE PACKING RULE, STATED AT THE TYPE SO NOBODY HAS TO GO FIND
// A BACKLOG. PER-TICK INPUT IS THE SCARCE WIRE: an input byte is multiplied across every ring
// entry in the packet-budget scenario, so it costs roughly TEN TIMES what a state byte costs.
// Task 11's `holdGuard`, ONE `bool`, moved the ring entry stride 81 -> 82 B and ate 10.264 B
// of the ordinary-join margin — a quarter of everything that was left. ~2.5 bytes remain.
// ⛔ SO A NEW PER-TICK INPUT SIGNAL IS A BIT IN THIS BYTE, NEVER A NEW MEMBER:
//       bit 0    holdGuard (task 11).
//       bits 1-7 RESERVED, and already spoken for — wall-grab (task 20), jump (21), dash
//                (31), ski-tuck (48). As four `bool`s those blow the fence twice over; as
//                four bits they cost nothing beyond what `holdGuard` already spent.
// `bool` -> `uint8_t` is the same 1 B, which is why this re-layout moved zero wire bytes.
//
// ⛔ AND A BIT CONSTANT MUST BE **READ**, NOT ONLY WRITTEN. The State-side `kFlagHasCommand`
// below was defined, written in three places and read NOWHERE, because an off-wire `bool`
// twin was quietly doing the real work; it took task 50 to notice and repair it. A flag whose
// only consumer is its own writer is a wire byte that buys nothing — a fact the wire asserts
// and nothing checks. `kInputFlagHoldGuard`'s reader is step 1's `frozen` gate in `integrate`
// below, the single place this input is consumed, and it is named as the reader there.
class PlayerInput
{
public:
    // Bit 0 = holdGuard; bits 1-7 reserved. Read the packing rule above before adding one.
    uint8_t flags = 0u;

    // THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
    // SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
    // [movement-sim tasks 11, 51] ALL BITS CLEAR IS NEUTRAL: not guarding is not an action,
    // and neither is any signal a reserved bit will come to carry. Unlike the radial/machine
    // aim there is no (0,0,1)-style tag value here, so `zero()` and `PlayerInput{}`
    // deliberately coincide — the property task 22's `ZeroInputIsTheFold` rests on, and one
    // every future bit inherits for free precisely because each bit's neutral value is 0.
    static PlayerInput zero() { return PlayerInput{}; }
};

// `PlayerInput::flags` bit assignments — the INPUT flags byte. Deliberately `kInputFlag`-
// prefixed to keep it distinct from `State::flags`, whose own bit constants (`kFlagFrozen`,
// `kFlagSupportMask`, `kFlagHasCommand`) are declared further down this file: the two bytes
// ride DIFFERENT WIRES — the relayed input ring and the correction state buffer — and a mask
// applied across them would be silently wrong.
inline constexpr uint8_t kInputFlagHoldGuard = 1u << 0;   // bit 0

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
class IntegrationUtils
{
public:
    IntegrationUtils(float deltaTime,
        uint32_t currentTick,
        PhysicsBodyAdapterType& physicsBodyAdapter,
        SpatialQueryAdapterType& queryAdapter)
        : m_deltaTime(deltaTime)
        , m_currentTick(currentTick)
        , m_physicsBodyAdapter(physicsBodyAdapter)
        , m_queryAdapter(queryAdapter)
    {}

    float getDeltaTime() const { return m_deltaTime; }
    // Current simulation tick — the Cadence model's commit boundary and the teleport
    // stamp. Projectile's shape; plumbed from SimulationTimeStep at the
    // SimulatableBrawler::integrate call site.
    uint32_t getCurrentTick() const { return m_currentTick; }
    PhysicsBodyAdapterType& getPhysicsAdapter() const { return m_physicsBodyAdapter; }
    SpatialQueryAdapterType& getQueryAdapter() const { return m_queryAdapter; }

private:
    float m_deltaTime;
    uint32_t m_currentTick;
    PhysicsBodyAdapterType& m_physicsBodyAdapter;
    SpatialQueryAdapterType& m_queryAdapter;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsBodyAdapterType, SpatialQueryAdapterType>>;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// THE TELEPORT SEED — spawn and respawn. `teleportPending` is a COUNTER-FREE edge: the UE
// layer (task 13) sets it non-zero, step 0 consumes it and clears it back to zero in the
// same tick. It is on the wire because a respawn must replay identically.
class InitialConditions
{
public:
    uint32_t  teleportPending = 0;   // ON WIRE 4
    glm::vec3 teleportPos{0.f};      // ON WIRE 12 — wire size 16 B
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// THE ONLY STATE.
//
// `bodyState` is written by the generic post-solve capture pass
// (SimulationIntegrationExecutor::captureBodyStatesAll, via PhysicsDeclaration::bodyStateOf)
// and pushed back into Chaos's rewind timeline on a resim — neither of which this sub-sim
// writes a line of code for. Its presence in SerializableFields is what puts the body on the
// wire and in the checksum.
//
// [movement-sim task 5, 2026-09-02] `LinearBodyState`, NOT `PhysicsBodyState`: position +
// linearVelocity only, 24 B instead of 52 B. Both generic sites above keep compiling
// untouched — capture ASSIGNS a captured `PhysicsBodyState` in (the narrowing `operator=`),
// the rewind push CONVERTS one back out (the implicit widening `operator PhysicsBodyState()`),
// and neither site names a body-state type. See the CHOICE RULE at the top of
// OGSimulation/PhysicsBodyState.h. The rotation drop is sound because
// `PhysicsSetup::body`'s descriptor now sets `lockRotation` — the condition that header
// names, which did not exist when the swap landed.
class State
{
public:
    // ON WIRE 24. `position` is LIVE — it is last tick's post-solve capture, i.e. where the
    // engine left the body after integrating our velocity and pushing it out of whatever it
    // overlapped. That is what makes step 6's positional adoption FREE: there is nothing to
    // copy, the capture already put the solved position here.
    //
    // ⚠ `bodyState.linearVelocity` IS DEAD WEIGHT — 12 B captured every tick, on the wire,
    // in the checksum, and NEVER READ by a line of this sub-simulation. That is not an
    // oversight; it is ruling #17(b), taken deliberately: option (a) — a NEW position-only
    // 12 B body-state seam type — would have cost a mirror of task 4 across the capture
    // bridge, the rewind push and every adapter, to save 12 B. The waste is the price of not
    // introducing that seam type, and reclaiming it is a later wire-budget pass. (Its name is
    // deliberately not spelled here: ruling #17 is the searchable anchor, and an acceptance
    // criterion greps this file for the rejected type.)
    // ⛔ Reading it would silently re-admit contact impulses into simulation state, which is
    // the ONE property revision 6 exists to guarantee.
    LinearBodyState bodyState;

    // ON WIRE 12. THE SIM'S velocity, and the only velocity any code here reads: assigned by
    // the model, the hover servo, the gravity law and (later) dash / launch. No engine term
    // has ever touched it.
    glm::vec3 velocity{0.f};

    // ON WIRE 8 — the Cadence model's committed direction, in the surface frame. Zero under
    // every other model; the layout does NOT change with `sd.model`, so switching models
    // cannot move a wire byte.
    glm::vec2 committedStepDir{0.f};

    // ON WIRE 4 — the tick the current Cadence step began (also stamped by a teleport).
    uint32_t stepStartTick = 0;

    // ON WIRE 1 — bit 0 frozen, bits 1-2 SupportState, bit 3 "a body command was issued last
    // tick" (`kFlagHasCommand`, and step 6' below is its READER — see the note there).
    uint8_t flags = 0;

    // ON WIRE 12 — the pose we HANDED the engine last tick, and the one input step 6'
    // cannot re-derive: `bodyState.position` has since been overwritten by the capture, so
    // the command is gone unless it is remembered. Wire size 61 B.
    //
    // ⭐⭐ [movement-sim task 50] THIS WAS OFF-WIRE SCRATCH, AND THAT WAS A LATENT DEFECT —
    // ruling #27 (A). A correction destroys off-wire scratch on BOTH of its paths, which the
    // off-wire block this replaces got wrong: `SimulationReconciliation::injectCorrectionState`
    // DEFAULT-CONSTRUCTS the state before `readInto`, `StateCorrectionCache::
    // tryInsertingCorrectState` stores that whole struct on a DISAGREEING landing, and
    // `prepareResimAll` assigns it back WHOLE (`editState() = cache.getState(idx)`) — it does
    // not restore "the serialized fields only". So after an adoption the scratch was ZEROED,
    // step 6' skipped the first replayed tick, the into-contact velocity component was not
    // killed, and the replay ended the tick `acceleration·dt` — 34.133 cm/s at the shipped
    // tunables — above the authority, against `kDefaultSimilarityEpsilon` = 0.0001. The
    // authority's NEXT correction then disagreed as well: ONE RESIM PER TICK for the duration
    // of a wall press, each one a lossy Chaos rewind. 12 B buys the whole of that back.
    // Pinned by `SimulatableBrawlerTest.cpp`'s `ReplayAfterAdoptionReproducesContactClamp`.
    //
    // ⛔ THERE IS NO COMMANDED-VELOCITY MEMBER BESIDE IT, and that is not an omission.
    // Task 11 kept one, off-wire; it held exactly `state.velocity`, which is ALREADY on the
    // wire (second entry in SerializableFields). Step 5 copied `velocity` into it as its
    // last act, and NOTHING writes `state.velocity` between there and the next step 6' —
    // the capture pass writes `bodyState` only (`PhysicsDeclaration::bodyStateOf`), and no
    // file outside this header names the field at all. So step 6' reads `state.velocity`
    // directly: EXACT, and 0 B instead of 12.
    glm::vec3 positionCmd{0.f};
};

// `State::flags` bit assignments. Bits 1-2 hold a `SupportState`, so the enum may grow to 4
// values before this field has to widen.
// ⭐ [movement-sim task 56] THE BITS DID NOT MOVE. `SurfaceKind` used the same shift and the same
// mask; only the enumerator names and the third value changed, which is why this task re-quoted
// `kComposite == 321u` and `syncSize<State>() == 61u` UNCHANGED rather than re-measuring them.
inline constexpr uint8_t kFlagFrozen       = 1u << 0;
inline constexpr uint8_t kFlagSupportShift = 1u;
inline constexpr uint8_t kFlagSupportMask  = 0x06u;   // bits 1-2
inline constexpr uint8_t kFlagHasCommand   = 1u << 3;

// Below this magnitude a push-out is engine noise, not contact: task 9's q3 measured a
// CONTACT-FREE hover (`pushOutTicks == 0`) with peak interpenetration 0.000005 cm, and q6
// measured 0.6 microns of ride-up between two blocking capsules. 0.05 cm is two orders of
// magnitude above both and is the threshold the spike itself used.
inline constexpr float kPushOutEps = 0.05f;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PhysicsDeclaration
{
    static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }
    static constexpr const char* name = "CharacterCapsule";

    // Maps the GAME's aggregate static data to this sub-simulation's own slice.
    // This is what makes body creation generic: the engine-side fold asks each
    // declaration for its slice instead of branching on the declaration type.
    // A member TEMPLATE deliberately — this header cannot name
    // simulatableBrawler::StaticData, because the aggregate includes this header
    // (an include cycle). GameStaticDataType is deduced at the call site, where the aggregate is
    // complete.
    template <typename GameStaticDataType>
    static const StaticData& staticDataOf(const GameStaticDataType& gsd) { return gsd.m_movementStaticData; }

    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& sd) {
        return PhysicsSetup::queryVolumes(sd);
    }
    // Zero, and structurally so: an `isRoot` body is not attached under anything.
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
    // The FLINCH read (step 1) and, from tasks 27/31, the committed states. A SOFT edge:
    // the machine sub-sim is declared FIRST in simulatableBrawler::ExecutionOrder and this
    // one LAST, so `findFirstViolation` is satisfied without moving anything.
    using External = ExternalDeps<const dAttackMachineSimulation::State&>;
    using InputType = brawlerMovementSimulation::PlayerInput;
    Owned owned;
    External external;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pure helpers — no state, no adapter, no engine.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Move `current` toward `target` by at most `maxDelta`. The genre's law for every decaying
// quantity (launch decel, braking); exact at the endpoint, so a brake reaches EXACTLY zero
// rather than asymptotically approaching it.
inline glm::vec2 moveTowards(glm::vec2 current, glm::vec2 target, float maxDelta)
{
    const glm::vec2 delta = target - current;
    const float distanceSq = glm::dot(delta, delta);
    if (distanceSq <= maxDelta * maxDelta || distanceSq <= 0.f)
        return target;
    return current + delta * (maxDelta / glm::sqrt(distanceSq));
}

// THE SURFACE FRAME. Builds a right-handed orthonormal basis (u, v, n) with `n` as up.
//
// `u` is world +X projected into the plane and renormalised, so on FLAT GROUND the frame is
// exactly world XY (u = +X, v = +Y) — which is what makes "same stick, flat vs slope" a
// meaningful comparison rather than an arbitrary rotation. The +Y fallback is only reachable
// when `n` is within ~26° of world +X, i.e. a near-vertical wall (task 20).
//
// One `sqrt` per call and no transcendental. `n` MUST already be unit length.
inline void buildTangentFrame(const glm::vec3& n, glm::vec3& u, glm::vec3& v)
{
    const glm::vec3 reference = (glm::abs(n.x) < 0.9f)
        ? glm::vec3(1.f, 0.f, 0.f)
        : glm::vec3(0.f, 1.f, 0.f);
    u = glm::normalize(reference - n * glm::dot(reference, n));
    v = glm::cross(n, u);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⭐⭐ THE DUAL-BASIS DECOMPOSITION — movement-sim TASK 57 / USER RULING #29, 2026-09-07.
// The ONE place `state.velocity` is split into the two channels steps 3 and 4 consume.
//
// ⛔ `(u, v, up)` SPANS SPACE BUT IS NOT ORTHONORMAL. `u` is the slope tangent and `up` is world
// up (ruling #26 a), so `dot(u, up) = sin(theta)` on a theta-degree face; only `v` is perpendicular
// to both (`v = cross(n, u)` is horizontal). Splitting a vector with plain dot products therefore
// DOUBLE-COUNTS, and both halves of that were live defects under task 56:
//   * a FALL leaked into the tangential channel — `dot(V, u)` of a 2000 cm/s descent on 30° is
//     1000 cm/s of "walk" the model then brakes. Positive feedback, because the tangential term
//     has its own vertical component: MEASURED, the task-56 tree DIVERGES on a slope landing;
//   * a WALK leaked into the vertical channel — walking a 30° slope at 600 cm/s genuinely rises
//     at 300 cm/s, `dot(V, up)` reported exactly that, and the servo DAMPED it. The character
//     settled 0.4197 cm off ride height (sign following the walk direction) for no reason but
//     the arithmetic.
//
// Solve `V = a*u + b*v + c*up` exactly instead. `v` is perpendicular to the other two, so `b` is a
// plain dot and can be removed first; what is left lives in the (u, up) plane and is a 2x2 solve:
//     b      = dot(V, v)
//     planar = V - b*v
//     s      = dot(u, up)                                    // sin(theta); 0 on flat ground
//     a      = (dot(planar, u) - dot(planar, up)*s) / (1 - s*s)
//     c      =  dot(planar, up) - a*s
// Checks: a pure walk `V = 600*u` gives `(a, c) = (600, 0)` — the servo no longer sees the walk;
// a pure fall `V = -2000*up` gives `(a, c) = (0, -2000)` — nothing leaks into the model.
//
// ⭐ AT `s == 0` IT REDUCES TO TODAY'S DOTS *BIT-EXACTLY*, and that is the correctness check
// rather than a nicety: `x - y*0.f` is `x` and `x / 1.f` is `x` for every finite float, and on flat
// ground `buildTangentFrame` returns exactly `u = (1,0,0)`, `v = (0,1,0)`. So every flat-ground
// case in the suite is byte-identical across this change, and any that moved would have been
// reporting a bug in this function.
//
// ⚠ DEGENERATE ONLY AT `s == 1`, a vertical face, which `Supported` cannot classify: step 2's
// walkable test caps the normal at `cosMaxSlope`. The assert below is the statement of that
// coupling — if a future `SupportState` arm lets the frame tilt to vertical, this is where it
// surfaces, not in a silent division by zero.
struct VelocityChannels
{
    glm::vec2 uv;   // tangential, in the surface frame (u, v) — step 3's `currentUV`
    float     up;   // vertical, along the servo's axis     — step 4's `vUp`
};

inline VelocityChannels decomposeVelocity(const glm::vec3& velocity,
                                          const glm::vec3& u,
                                          const glm::vec3& v,
                                          const glm::vec3& up)
{
    const float     b      = glm::dot(velocity, v);
    const glm::vec3 planar = velocity - b * v;
    const float     s      = glm::dot(u, up);

    OG_CHECK(s * s < 1.f,
        "brawlerMovementSimulation::decomposeVelocity - the tangent frame is vertical "
        "(dot(u, up) == 1), which Supported cannot classify: step 2 caps the walkable normal "
        "at cosMaxSlope. The dual basis is degenerate there.");

    const float a = (glm::dot(planar, u) - glm::dot(planar, up) * s) / (1.f - s * s);
    const float c =  glm::dot(planar, up) - a * s;
    return VelocityChannels{ glm::vec2(a, b), c };
}

// The genre's hitstop: a flinch freezes locomotion EXACTLY on the tick it is in effect. It
// does not decay the velocity, it zeroes the model's contribution for that tick.
//
// [movement-sim task 62] This was a template purely to keep `.m_currentState` and the
// enumerator DEPENDENT names while the machine State type was incomplete here. The machine
// header is included now, so both are spelled out and the compiler checks them.
// ⚠ `DAttackState` is at FILE SCOPE in `DAttackMachineSimulation.h`, outside
// `namespace dAttackMachineSimulation` — that is why the enum is unqualified while the State is
// not. Behaviour is identical: the same two enumerators, in the same order.
inline bool machineFreezesMovement(const dAttackMachineSimulation::State& machineState)
{
    return machineState.m_currentState == DAttackState::HitFlinch
        || machineState.m_currentState == DAttackState::GuardFlinch;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// MOVEMENT MODELS — step 3's dispatch targets.
//
// Each is a pure function of the SURFACE FRAME: `stickUV` and the returned velocity are both
// 2D coordinates in (u, v). No model knows whether that plane is the floor, a slope or a
// wall, and no model may touch `state.bodyState`.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// CMC-like. Accelerate toward the stick's target velocity at `acceleration`; with no stick,
// brake toward zero at `brakingDeceleration`. Stateless — it reads `currentUV` and writes no
// State slice at all.
inline glm::vec2 computeDesiredVelocityUV_ContinuousAccelBrake(
    const StaticData& sd, State& state, uint32_t tick,
    glm::vec2 stickUV, glm::vec2 currentUV, float dt)
{
    (void)state;
    (void)tick;
    const float stickLengthSq = glm::dot(stickUV, stickUV);
    if (stickLengthSq <= 0.f)
        return moveTowards(currentUV, glm::vec2(0.f), sd.brakingDeceleration * dt);
    return moveTowards(currentUV, stickUV * sd.maxWalkSpeed, sd.acceleration * dt);
}

// "Steps, not a joystick". The direction is COMMITTED at a period boundary and held at a
// constant speed until the next one, so a mid-step stick flick cannot steer the character —
// which is the whole feel. A neutral stick at a boundary commits a STAND (zero direction);
// the timer keeps running either way, so the cadence stays on the beat.
//
// This model owns exactly two State fields, and touches nothing else.
inline glm::vec2 computeDesiredVelocityUV_Cadence(
    const StaticData& sd, State& state, uint32_t tick,
    glm::vec2 stickUV, glm::vec2 currentUV, float dt)
{
    (void)currentUV;
    (void)dt;
    // Unsigned arithmetic is deliberate and safe: `stepStartTick` is only ever stamped from
    // a tick that has already happened, so the difference never wraps.
    if (tick - state.stepStartTick >= sd.stepPeriodTicks)
    {
        const float stickLengthSq = glm::dot(stickUV, stickUV);
        state.committedStepDir = (stickLengthSq > 0.f)
            ? stickUV * (1.f / glm::sqrt(stickLengthSq))
            : glm::vec2(0.f);
        state.stepStartTick = tick;
        OGBLOG_G("[Movement.cadence] tick=%u commit dir=(%.3f, %.3f)",
            tick, state.committedStepDir.x, state.committedStepDir.y);
    }
    return state.committedStepDir * sd.stepSpeed;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⭐⭐ THE DETACH GATE — movement-sim TASK 56 / ruling #28. The ONE place a walkable probe hit
// can still be classified `Unsupported`, and the reason `SupportState`'s name is true rather
// than aspirational: support is a DECISION, and this is where it is taken.
//
// ⛔ IT RETURNS `false` TODAY, AND THAT IS A STATEMENT ABOUT THE TREE, NOT A PLACEHOLDER I
// FORGOT TO FILL. Both conditions the design names are unbuilt:
//   * `Jumping` (task 21) — a `State::flags` bit set on the take-off edge and cleared at the
//     apex. Bits 4-7 are free; the bit and its writer land WITH task 21, because a flag written
//     and never read (or read and never written) is the `kFlagHasCommand` trap this file already
//     paid for once. Task 21's whole vertical change becomes: set the bit, assign `velocityUp`,
//     let the law below run.
//   * an upward `Launched` (task 27) — `DAttackState` is {Attacking, Idle, GuardFlinch,
//     HitFlinch} in this tree. There is NO `Launched` enumerator to test, so the condition
//     cannot be written today without inventing the state it reads. ⚠ The Backlog's AC says
//     "today only upward `Launched`"; that clause is vacuous against the shipped machine, and it
//     is recorded as such in `impl/impl_notes_seam_56.md` rather than faked.
//
// ⚠ SO NO LLT CAN REACH THE `true` ARM, AND NONE PRETENDS TO. What IS verified is the WIRING —
// that step 2 consults this predicate at all — and it was verified by POISONING it to `return
// true` and watching `HoverHoldsRideHeight`, `HoverLiftsWhenLow` and
// `WallPressKeepsTheCharacterOnTheFloor` go RED together (the character falls); the run is
// recorded in the impl notes and the poison was reverted. `DetachGateIsVacuousUntilJumpLands`
// pins the vacuity itself, and says in the case what it does and does not prove.
//
// Parameters are the shape the filled version needs, so task 21 changes a body and not a
// signature: the state carrying the future `Jumping` bit, and the axis an upward `Launched`
// would be measured along.
inline bool detachesFromSupport(const State& state, const glm::vec3& up)
{
    (void)state;
    (void)up;
    return false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// THE MOVEMENT TICK — architecture §3.3, revision 6.
//
// Steps run 6' -> 0 -> 1 -> 2 -> 3 -> 4 -> 5. Step 6' is numbered for the tick it belongs to
// (it closes LAST tick's loop) and runs FIRST here, which is the only order that works: the
// engine's answer to last tick's command is not available until the capture has landed.
//
// ⚠ `machineInput` IS THE MACHINE'S INPUT SLICE, not this sub-sim's: the move stick is packed
// onto `dAttackMachineSimulation::PlayerInput`, and the single call site in
// `SimulatableBrawler.h` is the one place it can be supplied. See the undeclared-input-edge
// note at the machine include, at the top of this file.
// [movement-sim task 62] It is SPELLED here now rather than deduced through a `MachineInputT`
// template parameter; that indirection existed only to survive the include cycle. The default
// template argument it carried was documentation — the parameter was always deduced from the
// argument at that one call site, so the default never participated.
template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void integrate(float deltaSeconds,
    const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
    const dAttackMachineSimulation::PlayerInput& machineInput,
    const StaticData& sd,
    Dependencies deps,
    const RuntimeBindings& bindings,
    DerivedState& derivedState)
{
    auto& utils   = input.getIntegrationUtils();
    auto& physics = utils.getPhysicsAdapter();
    auto& query   = utils.getQueryAdapter();
    const float dt        = deltaSeconds;
    const uint32_t tick   = utils.getCurrentTick();

    State& state = deps.owned.edit<State>();
    InitialConditions& ic = deps.owned.edit<InitialConditions>();

    // ---- step 6': ADOPT THE ENGINE'S POSITIONAL PUSH-OUT --------------------------------
    // The POSITION half of the adoption is free and structural: `bodyState.position` IS the
    // post-solve capture, written by the generic capture pass before this function was
    // called. There is nothing to copy. What is NOT free is separating the engine's
    // contribution from our own command, which is the whole of the arithmetic below, and the
    // one authored rule that follows from it.
    {
        glm::vec3 pushOut(0.f);
        // ⭐ [movement-sim task 50] `kFlagHasCommand` IS READ HERE, and this is its only
        // reader. Until this task it was written by step 5 and consulted by nothing, while an
        // off-wire `bool` twin carried the decision — the write-only-flag anti-pattern.
        // Reading the ON-WIRE bit is what makes the gate survive a correction: the bit is
        // restored with the rest of `flags`, so a replayed tick reaches this branch exactly
        // when the authority's own tick did.
        if ((state.flags & kFlagHasCommand) != 0u)
        {
            // What `x += v·dt` alone would have produced from last tick's command. Anything
            // else in the captured position is the solver: a wall, or another brawler.
            // ⚠ `state.velocity` IS last tick's commanded velocity — see the
            // commanded-velocity paragraph on `State`. BOTH operands are serialized now,
            // which is the whole property this step needed and did not have.
            const glm::vec3 predicted = state.positionCmd + state.velocity * dt;
            pushOut = state.bodyState.position - predicted;
            if (glm::dot(pushOut, pushOut) > kPushOutEps * kPushOutEps)
            {
                // THE AUTHORED CONTACT RULE, and the only way contact reaches velocity at all.
                // It has TWO arms since movement-sim task 57 / ruling #29, chosen by the
                // ORIENTATION of the contact, and the arms remove DIFFERENT components:
                //
                //   LANDING (`dot(n, up) >= cosMaxSlope`) — a floor-like face absorbs the
                //     VERTICAL component and keeps the horizontal. That is the genre's rule
                //     (landing zeroes vertical, horizontal survives) and it is the fix for the
                //     user's *"if there is a slope where I landed my brawler goes flying"*: the
                //     into-contact kill below turns a straight-down 2000 cm/s fall onto a 30°
                //     face into 1000 cm/s DOWN THE SLOPE, because removing the normal component
                //     of a vertical vector leaves its whole tangential part behind. An inelastic
                //     bounce is right for a wall and wrong for the ground.
                //
                //   WALL / OTHER BRAWLER (everything else) — UNCHANGED: kill the component
                //     driving into the obstacle, keep the rest. On a wall this is the
                //     fighting-game corner clamp; on another brawler it means the pushed
                //     character's into-contact velocity is zeroed for the tick and NO MOMENTUM
                //     IS TRANSFERRED. Task 54's wall press and `WallPushOutZeroesIntoWallComponent`
                //     measure this arm and did not move.
                //
                // ⚠ `kWorldUp`, NOT step 2's `up` local: step 6' runs BEFORE step 2, so the local
                // does not exist yet. Same constant, same ruling #26 (a) axis — and this is the
                // one place in the file that needs it early.
                // ⚠ THIS IS NOT A VELOCITY DECOMPOSITION and must not be confused with F2's:
                // both arms remove ONE AUTHORED COMPONENT from a vector, which is exact for any
                // unit direction. There is no basis here to be non-orthogonal.
                // The captured `linearVelocity` — which is where the engine put its contact
                // impulse — is not read here or anywhere.
                const glm::vec3 contactNormal = glm::normalize(pushOut);
                if (glm::dot(contactNormal, kWorldUp) >= sd.cosMaxSlope)
                    state.velocity -= glm::dot(state.velocity, kWorldUp) * kWorldUp;
                else
                    state.velocity -= glm::dot(state.velocity, contactNormal) * contactNormal;
            }
        }
        derivedState.lastPushOut = pushOut;
    }

    // ---- step 0: TELEPORT SEED ----------------------------------------------------------
    // The ONLY body write that ignores `sd.drivesBody`: a spawn or respawn must move the
    // capsule even on an instance that is not driving it — a `drivesBody = false` peer or LLT
    // rig. (In shipped production `drivesBody` is TRUE and the CMC is retired; task 15 flipped
    // both together. The earlier wording here said "while the CMC is still driving it", which
    // has not been the case since. [movement-sim task 17])
    if (ic.teleportPending != 0u)
    {
        state.bodyState.position = ic.teleportPos;
        state.velocity           = glm::vec3(0.f);
        state.committedStepDir   = glm::vec2(0.f);
        state.stepStartTick      = tick;
        state.flags              = static_cast<uint8_t>(state.flags & ~kFlagHasCommand);
        ic.teleportPending       = 0u;

        glm::mat4 teleportPose(1.f);
        teleportPose[3] = glm::vec4(ic.teleportPos, 1.f);
        physics.setBodyTransform(bindings.ownBodyId, teleportPose);
        physics.setBodyLinearVelocity(bindings.ownBodyId, glm::vec3(0.f));

        OGBLOG_G("[Movement.teleport] tick=%u to=(%.2f, %.2f, %.2f)",
            tick, ic.teleportPos.x, ic.teleportPos.y, ic.teleportPos.z);

        // ⭐⭐ THE HOVER TUNING READOUT — movement-sim task 56. The first PIE run after ruling #28
        // is a TUNING session, and ω/ζ are provisional. This prints the configured pair, the
        // derived gains, and — the one number nobody guesses from continuous intuition — the
        // DISCRETE critical damping ratio for the configured ω, so a tuner can see at a glance
        // which side of the ringing threshold they are on.
        //
        // ⚠ `[Warning]` IS DELIBERATE AND IS THE ONLY VISIBILITY KNOB THIS LINE HAS. `OGBLOG_G`
        // does not enter `RouteOGMessage`; ogblog's own sink sends every message to
        // `LogOGBrawler`, reading only the leading severity token, and
        // `Config/DefaultEngine.ini` ships `LogOGBrawler=Warning`. A bare tag would be invisible
        // in exactly the session this line exists for.
        // ⚠ AND IT IS HERE, IN STEP 0, RATHER THAN IN `StaticData`'s CONSTRUCTOR, FOR AN
        // ORDERING REASON: `ASimulationManagerUImpl::m_staticData` is a by-value member, so it is
        // constructed with the actor — BEFORE `BeginPlay` installs the ogblog sink. A log from
        // there would be swallowed. The teleport seed runs once per spawn and respawn, with the
        // sink installed, which is exactly the cadence a tuning readout wants.
        OGBLOG_G("[Warning][Movement.hover] tick=%u omega=%.3f zeta=%.3f "
                 "(discrete critical zeta = %.3f; monotone needs zeta >= that AND omega < %.1f) "
                 "k=%.1f c=%.2f maxAccel=%.0f rideHeight=%.2f snapDistance=%.2f",
            tick, sd.hoverFrequency, sd.hoverDampingRatio,
            1.f - sd.hoverFrequency * dt * 0.5f, 1.f / dt,
            sd.hoverStiffness, sd.hoverDamping, sd.hoverMaxAccel,
            sd.rideHeight, sd.snapDistance);
    }

    // ---- step 1: GATE -------------------------------------------------------------------
    // ⭐ [movement-sim task 51] `kInputFlagHoldGuard` IS READ HERE, and this is its ONLY
    // reader — the one line that turns the input byte's bit 0 into behaviour. Nothing writes
    // the bit yet; task 14 (`buildPlayerInput` -> `makeSimPlayerInput`) will be its only
    // writer. Keep the pair visible: a bit written and never read is exactly the
    // `kFlagHasCommand` trap that took task 50 to repair.
    const bool frozen = (input.getPlayerInput().flags & kInputFlagHoldGuard) != 0u
        || machineFreezesMovement(deps.external.get<dAttackMachineSimulation::State>());

    // COMMITTED STATES own velocity outright — the model is suspended, not blended. ⛔ The
    // machine enumerators this reads (`Dashing`, task 31; `Launched`, task 27) DO NOT EXIST
    // YET: `DAttackState` is {Attacking, Idle, GuardFlinch, HitFlinch} today. The external
    // dependency that will carry them is already declared and already resolved above, so
    // those tasks add their enumerator, their branch in step 3 and nothing else here.
    const bool committed = false;

    // ---- step 2: ATTACH + CLEARANCE -----------------------------------------------------
    // Model-agnostic by construction: the model selector is not named in this step, in step 4
    // or in step 5 — only in step 3's dispatch, which is what an acceptance criterion greps for.
    //
    // ⭐ THE SERVO AXIS, SPELLED ONCE PER TICK. `up` is what this step measures `clearance`
    // along, what the walkable test dots against, and what step 5 actuates along — one name,
    // one value, user ruling #26 (a). Tasks 20/48 turn this line into a `SupportState` branch;
    // today every v1 surface is `Supported` and it is `kWorldUp` unconditionally.
    const glm::vec3 up = kWorldUp;

    const float probeLength = sd.rideHeight + sd.snapDistance;
    glm::mat4 probePose(1.f);
    probePose[3] = glm::vec4(state.bodyState.position, 1.f);

    SweepHit probe;
    if (!bindings.queryVolumeIds.empty())
    {
        probe = query.sweep(bindings.queryVolumeIds[0], probePose, -up * probeLength);
    }

    // `fraction` is only meaningful when `blocked` (SpatialQueryResult.h's field-validity
    // rule), so an unblocked probe has no clearance at all rather than a large one.
    // ⭐ `clearance` is a gap measured ALONG `up`, and step 4 closes it ALONG `up`. That the
    // two are the same axis is the whole of ruling #26 (a).
    const float clearance = probe.blocked ? probe.fraction * probeLength : probeLength;
    const bool walkable   = probe.blocked && glm::dot(probe.normal, up) >= sd.cosMaxSlope;

    // ⭐⭐ CLASSIFICATION IS WHERE SUPPORT IS DECIDED — movement-sim task 56 / ruling #28.
    // Geometry (`walkable`) is one input; the DETACH GATE is the other, and both are read here so
    // that step 4 can be a pure reader. `SupportedSteep` has no producer in v1 — the walkable
    // test is a single `cosMaxSlope` comparison, so a face past `maxSlopeAngleDeg` is
    // `Unsupported`, not steep-supported. Task 48 adds the second comparison HERE, not in step 4.
    const SupportState support = (walkable && !detachesFromSupport(state, up))
        ? SupportState::Supported
        : SupportState::Unsupported;

    // `n` is the SURFACE normal and is now used for exactly one thing: building the tangent
    // frame (u, v) that step 3's models live in (plus the viz readout below). It is NO LONGER
    // the actuation axis — step 5 puts the support term on `up`.
    const glm::vec3 n = walkable ? probe.normal : up;
    glm::vec3 u(1.f, 0.f, 0.f);
    glm::vec3 v(0.f, 1.f, 0.f);
    buildTangentFrame(n, u, v);

    // The tag the `[Movement.surface]` line prints. ⚠ THE STRINGS ARE THE ENUMERATOR SPELLINGS,
    // not prettier prose: the PIE runbook greps for them, and a readable synonym would make the
    // grep and the code disagree. `Floor` / `Airborne` are RETIRED tokens — a runbook or a saved
    // log filter still looking for them is looking for a build older than task 56.
    const auto supportName = [](uint8_t bits) -> const char*
    {
        switch (static_cast<SupportState>(bits))
        {
        case SupportState::Supported:      return "Supported";
        case SupportState::SupportedSteep: return "SupportedSteep";
        default:                           return "Unsupported";
        }
    };

    const uint8_t previousSupport =
        static_cast<uint8_t>((state.flags & kFlagSupportMask) >> kFlagSupportShift);
    const uint8_t supportBits = static_cast<uint8_t>(support);
    if (supportBits != previousSupport)
    {
        OGBLOG_G("[Movement.surface] tick=%u %s -> %s clearance=%.3f n=(%.3f, %.3f, %.3f)",
            tick, supportName(previousSupport), supportName(supportBits),
            clearance, n.x, n.y, n.z);
    }

    state.flags = static_cast<uint8_t>(state.flags & ~(kFlagFrozen | kFlagSupportMask));
    if (frozen)
        state.flags = static_cast<uint8_t>(state.flags | kFlagFrozen);
    state.flags = static_cast<uint8_t>(state.flags
        | ((supportBits << kFlagSupportShift) & kFlagSupportMask));

    derivedState.surfaceNormal    = n;
    derivedState.lastProbePoint   = probe.blocked ? probe.impactPoint : glm::vec3(0.f);
    derivedState.lastSupportState = supportBits;

    // ---- step 3: MODEL DISPATCH ---------------------------------------------------------
    // The stick arrives as a WORLD XY direction whose magnitude is the stick deflection
    // (BrawlerInputPackaging.h: `moveDirectionWorld` is the move stick rotated into camera
    // space, so rotation preserves its length). It is projected into the surface frame with
    // the DEFLECTION PRESERVED and the DIRECTION renormalised: on a slope the character
    // walks along the slope at the speed the stick asked for, instead of losing speed to the
    // cosine of the incline.
    const glm::vec3 stickWorld(machineInput.moveDirectionWorld.x, machineInput.moveDirectionWorld.y, 0.f);
    const float stickDeflection = glm::length(stickWorld);
    glm::vec2 stickUV(0.f);
    if (stickDeflection > 0.f)
    {
        const glm::vec2 projected(glm::dot(stickWorld, u), glm::dot(stickWorld, v));
        const float projectedLengthSq = glm::dot(projected, projected);
        if (projectedLengthSq > 0.f)
            stickUV = projected * (stickDeflection / glm::sqrt(projectedLengthSq));
    }

    // ⭐⭐ THE ONE READ OF `state.velocity` PER TICK — movement-sim task 57 / ruling #29.
    // Both channels come out of the SAME dual-basis solve, and there is deliberately no second
    // plain dot of the velocity against a frame axis anywhere below: `currentUV` here and `vUp` in
    // step 4 are the two halves of one decomposition, and they were the two defects R2 named. Read
    // `decomposeVelocity`'s comment before touching either. `state.velocity` is not written again
    // until step 5, so one solve serves both sites.
    const VelocityChannels channels = decomposeVelocity(state.velocity, u, v, up);
    const glm::vec2 currentUV = channels.uv;

    glm::vec2 velocityUV(0.f);
    if (committed)
    {
        // Tasks 31 (Dashing: ASSIGN `dashDir · dashSpeed`, replacing momentum) and 27
        // (Launched: `moveTowards(currentUV, 0, launchDecel·dt)`) land their branches here.
        velocityUV = currentUV;
    }
    else if (frozen)
    {
        // EXACT, this tick. Not a decay.
        velocityUV = glm::vec2(0.f);
    }
    else
    {
        switch (sd.model)
        {
        case MovementModel::ContinuousAccelBrake:
            velocityUV = computeDesiredVelocityUV_ContinuousAccelBrake(sd, state, tick, stickUV, currentUV, dt);
            break;
        case MovementModel::Cadence:
            velocityUV = computeDesiredVelocityUV_Cadence(sd, state, tick, stickUV, currentUV, dt);
            break;
        default:
            OG_CHECK(false, "brawlerMovementSimulation::integrate - unhandled MovementModel");
            velocityUV = glm::vec2(0.f);
            break;
        }
    }

    // ---- step 4: VERTICAL ---------------------------------------------------------------
    // ⭐ MEASURED AND ACTUATED ON THE SAME AXIS, `up`. That is the whole of user ruling #26.
    //
    // ⭐⭐ ONE LAW, NOT TWO — movement-sim TASK 56 / USER RULING #28, 2026-09-07. This step used
    // to be an `if` with two DIFFERENT KINDS of law in its arms, and the seam between them was a
    // defect that no amount of tuning either arm could reach:
    //     Floor:     velocityUp = clamp((rideHeight − clearance) / dt, ±maxSnapSpeed)   // ASSIGNED
    //                                                                                   // from position
    //                                                                                   // error; gravity OFF
    //     Airborne:  velocityUp = velocity·up + gravity·dt                              // INTEGRATED;
    //                                                                                   // gravity ON
    // Crossing the boundary handed a velocity PRODUCED BY A POSITION SERVO to a law that treats
    // its input as MOMENTUM. That single line is the mechanism behind the wall-press LAUNCH half
    // of task 54 (`impl_notes_seam_54.md` §1.3, the "airborne-retention line") and behind the
    // user's *"falls start at ~4 m/s instead of from rest"*. Task 54 removed the SPURIOUS
    // crossings; ruling #28 removes the second law, so there is nothing left to retain.
    // ⛔ THE AIRBORNE-RETENTION RULING IS THEREFORE MOOT, NOT ANSWERED. Do not reopen it: the
    // line it was about does not exist, and the concept has no name left to attach to.
    //
    // THE LAW. Gravity is unconditional; the servo is a term ADDED to it while supported:
    //     a = gravity                                              ← every tick, no branch
    //     if (support != Unsupported)
    //         a += clamp(k·(rideHeight − clearance) − c·vUp − gravity, ±hoverMaxAccel)
    //     velocityUp = max(vUp + a·dt, −terminalFallSpeed)
    //
    // ⭐ THE `− gravity` INSIDE THE CLAMPED TERM IS A FEED-FORWARD, AND IT IS WHAT KEEPS THE
    // STEADY STATE EXACT. At `clearance == rideHeight` with `vUp == 0` the servo evaluates to
    // exactly `−gravity`, so `a == 0` and the character neither rises nor sinks — the hover
    // height is a value, not an approximation. WITHOUT it the spring would have to SAG until
    // `k·sag == |gravity|` to hold itself up: 0.4631 cm at the shipped k, which every existing
    // `clearance == rideHeight` pin would have had to be loosened to accept. One subtraction.
    // ⚠ It is INSIDE the clamp deliberately: the clamp then bounds the servo's own authority, and
    // the total vertical acceleration is `gravity + clamp(…, ±hoverMaxAccel)`. So the honest
    // per-tick velocity bound is `(hoverMaxAccel + |gravity|)·dt`, not `hoverMaxAccel·dt`, and
    // `NoVelocityStepAtSupportBoundary` pins that form rather than the tidier wrong one.
    //
    // ⭐ WHAT THE ONE LAW BUYS, AND IT IS A PROPERTY RATHER THAN A TUNING: `velocityUp` is now
    // CONTINUOUS across the support boundary by construction — nothing is ever assigned, so no
    // classification flip can introduce a step. Measured on a 10 m fall, worst per-tick
    // `|Δ velocity·up|`: 483.67 cm/s under this law against the 516.33 cm/s bound, and 972.00
    // under the dead-beat, which destroyed 972 cm/s of real momentum in one tick on landing.
    // ⚠ AND WHAT IT DOES NOT BUY, RECORDED BECAUSE AN AC PREDICTED OTHERWISE: a walk-off does NOT
    // start "from rest" under either law, because in both the servo is TRACKING a receding floor
    // and the character genuinely has that velocity when the probe lets go. On a first-principles
    // ledge corner the detach speed is −101.61 cm/s under this law and −107.07 under the dead-beat.
    // What changed is provenance, not magnitude: the retained value is now something the body was
    // physically doing, not a one-tick position correction reinterpreted as momentum.
    // `LedgeFallStartsFromRest` says so in the case, and is GREEN in both arms on purpose.
    //
    // ⛔⛔ USER RULING #26 = (a), 2026-09-06 — CORRECT ALONG WORLD-UP. TASK 49.
    // Architecture §3.3 as revision 6 first wrote it measured `clearance` along the SWEEP
    // (world −Z) and applied the correction along `n`, the SURFACE normal: a control loop
    // whose measurement axis and actuation axis disagree. On a slope of angle θ that bought a
    // one-shot lateral slide while the servo converged, and an over-correction on every tick of
    // it. ⛔ Task 11 shipped §3.3 VERBATIM and was not at fault; task 49 changed §3.3's own
    // answer. Both axes are `up`, and ruling #28 did not touch either one.
    //
    // ⭐ MEASURED, NOT ARGUED (task 49 AC-1, 2026-09-06). The magnitude of that effect had been
    // derived wrong twice, so the PRE-FIX header was driven through
    // `SimulatableBrawler::integrate` against an analytic 30° plane, seeded 1 cm above ride
    // height, logging per-tick clearance change against the commanded step:
    //     clearance gained / clearance commanded   1.154700, 1.154716, 1.154879, 1.154875
    //                                              (1/cos30° = 1.154701)
    //     lateral X   0.433010 cm over the whole transient for e = 1 cm (= sinθ·cosθ)
    //   ⇒ THE PLANE DERIVATION IS THE RIGHT ONE. The probe is cast FROM THE CAPSULE, so a step
    //     along `n` also moved the probe origin sideways by `s·sinθ` and the ground under it fell
    //     away by `s·sinθ·tanθ`; the vertical gap gained `s/cosθ` and the servo OVER-corrected.
    //     ⛔ The superseded `e·tanθ` model — which predicted 0.866× here and a 0.577 cm lateral —
    //     is DEAD, and so is the 50.86 cm figure it produced.
    //   ⇒ At the shipped 45° cap the pre-fix figure was 1/cos45° = 1.414: the servo OVERSHOT and
    //     damped-OSCILLATED, dipping below ride height on alternate ticks, and would have
    //     DIVERGED past 60°. Under (a) `Δclearance` equals the commanded step at EVERY angle —
    //     the probe origin no longer moves horizontally — so the oscillation is gone and the 60°
    //     ceiling is LIFTED rather than tuned around. That is the argument for the ruling, and it
    //     is why task 49 was a fix and not a re-tune.
    //   ⚠ THAT MEASUREMENT WAS TAKEN ON THE DEAD-BEAT, whose loop gain was 1 by construction, so
    //     the RATIOS above are a property of the AXES and not of the law. Ruling #28 changed the
    //     law and left the axes alone: the servo term is still built from a scalar `clearance`
    //     error and still composed onto `up` in step 5, so the lateral component is still
    //     IDENTICALLY zero at every angle. What did move is CLOSURE — the error no longer goes to
    //     zero in one tick, by design — which is why `HoverOnRampDoesNotDrift` now pins the
    //     measured spring transient instead of a one-tick closure.
    // ⛔⛔ THIS IS *NOT* TASK 9'S 29 cm, AND NOTHING HERE MAY CLAIM IT IS (task 49, 2026-09-06).
    // Backlog 49, `impl/design_hover_slope_transient.md` and the comment task 11 shipped all
    // attribute the spike's `capX.X −9.558624 → −38.920441` to this coupling. That attribution
    // is FALSIFIED, on the spike's own artefacts, by two independent facts:
    //   1. THE SPIKE'S SERVO ALREADY ACTUATED ON WORLD-UP. `impl/Spike9Probe.cpp.{final,pre44,
    //      pre9b}` — all three archived revisions — build the hover velocity as
    //      `FVector Vel = ZeroVector; ... Vel.Z = Clamp(Err / Dt, ±maxSnapSpeed)`. There is no
    //      surface frame in the probe: no `u`, no `v`, no `n * velocityN`. The mismatch this
    //      task removes DID NOT EXIST in the code that produced the 29 cm, so it cannot be its
    //      cause. This is a structural fact, not a derivation.
    //   2. `impl/research_spike_9.md` §0.4a ACCOUNTS FOR THE WHOLE 29 cm AS SPAWN
    //      DEPENETRATION: the capsule was seeded 89.5 cm INSIDE the ramp, `penDepth` falls in
    //      four equal 22.4395 cm decrements, and Σ push.X over steps 0–3 = −38.920508 against a
    //      measured −38.920441 — an accounting identity to 7e−5 cm that stops dead the tick the
    //      penetration reaches zero. Servo motion is COMMANDED, so it lands in `predicted` and
    //      can never appear in `pushOut`; a push-out that explains 100 % of the lateral travel
    //      leaves the servo nothing to have contributed.
    //   Also: the motion took THREE ticks (50 ms), not "the first second".
    // ⇒ THE DEFECT BELOW IS REAL AND MEASURED, BUT ITS MAGNITUDE IN A SHIPPING SCENARIO IS
    //   UNMEASURED — no run has ever exercised this servo on a slope. ⛔ The vertical errors
    //   inferred from the 29 cm (67.81 cm here, 50.86 under the dead model) are artefacts of an
    //   attribution this file does not accept; do not repeat either figure.
    // ⛔ Task 9 observation #2 (127-step convergence) is not this either, and it is no longer
    // open: `research_spike_9.md` §0.1(1)/§0.2 WITHDREW it — `277 − 150` is distance-to-wall
    // divided by scripted speed (417.25 cm ÷ 3.3329 cm/step = 125.2), which is why all three
    // peers agree on it. Backlog 49 still says "#2 stays OPEN"; that entry is stale.
    //
    // ⚠ WHAT (a) COSTS, RECORDED SO TASKS 20/48 INHERIT IT. `rideHeight` and the probe reach
    // are both measured along `up`, so the PERPENDICULAR gap is `rideHeight·cosθ` — 7.07 cm at
    // the 45° cap, 2.59 cm at 75°. ⭐ That is NOT a regression from this task: the measurement
    // axis was already vertical, so the steady-state perpendicular gap is identical before and
    // after, and what changed is that the servo no longer undershoots it. ⛔ Do not compensate
    // by scaling `rideHeight` with slope — that is a design change and it is lead-scoped.
    // Steep faces need the axis to follow `SupportState` (see `kWorldUp` above) — that is what
    // `SupportedSteep` is reserved for, and it is a STEP 2 change when it lands, not a step 4 one.
    // The vertical channel of step 3's dual-basis solve. ⛔ NOT a plain dot of the velocity
    // against `up`, which is what this line was until task 57: on a slope that reads the WALK's own
    // vertical component as a fall and damps it (R2's mirror — walking a 30° face at 600 cm/s
    // asked for 17 112 cm/s² of lift). See `decomposeVelocity`.
    // ⚠ THE ACCEPTANCE GREP FOR THIS TASK is "no plain dot of `state.velocity` against `u`, `v`
    // or `up` remains in this header". It returns ZERO. The two `glm::dot(state.velocity, ...)`
    // calls that DO remain are both in step 6', are both authored single-component removals
    // against a unit direction (`kWorldUp` and the contact normal), and are not a basis
    // decomposition — the note at the branch says so at the site.
    const float vUp = channels.up;

    // AUTHORED gravity (ruling #16 a) — per character, so fall speed is a design knob rather
    // than a world constant. `sd.gravity` is a SIGNED SCALAR (−980), i.e. §3.3's
    // `dot(g_vec, up)` with `g_vec = (0, 0, gravity)` already collapsed; widening it to a vector
    // would be a `StaticData` change, which is an addition and not a hoist.
    // ⭐ UNCONDITIONAL. There is deliberately no branch here that could turn it off, which is what
    // `GravityRunsWhileSupported` measures by starving `hoverMaxAccel` below `|gravity|` and
    // watching a SUPPORTED character sink at exactly `(gravity + hoverMaxAccel)·dt`.
    float accelUp = sd.gravity;

    if (support != SupportState::Unsupported)
    {
        // ⭐⭐ THE SERVO IS ONE-SIDED — movement-sim TASK 57 / USER RULING #29, 2026-09-07.
        //
        //     SUPPORTED MEANS HELD UP, NEVER PULLED DOWN.
        //
        // At or below ride height (`e >= 0`) this is task 56's full spring-damper, unchanged and
        // still poison-tested. ABOVE ride height it is a BOUNDED SPRING PULL AND NOTHING ELSE,
        // capped at `sd.hoverPullDownAccel` — 0 in the shipped data, so the vertical law up there
        // is gravity and nothing else, which is what the user asked for.
        //
        // ⛔⛔ WHY THE OLD TWO-SIDED ARM WAS THE LEDGE DEFECT (R1). Its pull was bounded by
        // ACCELERATION (`hoverMaxAccel` = 30 g), not by speed, so a clearance that JUMPS into the
        // band cost -516.333 cm/s in ONE tick against gravity's -16.333: 31x too fast, MEASURED on
        // the task-56 tree at clearance 45 (`AboveRideHeightIsGravityOnly`). The dead-beat this
        // spring replaced capped the CHASE SPEED at 400 cm/s and so never had this failure mode;
        // that difference is what made "the spring is about as fast as the dead-beat" wrong.
        //
        // ⛔⛔ TWO VARIANTS ARE FORBIDDEN, AND BOTH WERE WRITTEN BEFORE BEING CAUGHT
        // (`impl/design_ledge_fall_and_landing.md` §1b):
        //   1. LEAVING THE DAMPER AND/OR THE FEED-FORWARD ON ABOVE RIDE HEIGHT. Both OPPOSE the
        //      fall (+22 800 cm/s² at 400 cm/s, plus 980), so the body FLOATS in the band instead
        //      of leaving it — the damper's terminal chase speed alone is `g/c` = 17 cm/s, 2.3 s
        //      to clear 40 cm. That is why this arm has neither term, not an oversight.
        //   2. GATING THE WHOLE SERVO ON A CHASE SPEED. Bang-bang: +/-380 cm/s per tick at the
        //      shipped gains.
        //
        // ⚠ `hoverMaxAccel` still bounds BOTH arms and stays 30 000: catching a landing needs it.
        // The pull-down knob bounds only the spring term on the upper arm.
        // ⚠ `support` is READ here and decided in step 2. Step 4 has no geometry of its own and
        // must not acquire any: the day a jump has to suppress the servo, step 2's detach gate is
        // the one place that changes.
        //
        // `k` and `c` are `ω²` and `2ζω`, derived once in `StaticData`'s constructor; read the ζ
        // comment there before changing either — ζ = 1 is NOT critical damping for this discrete
        // recurrence and rings visibly.
        const float e = sd.rideHeight - clearance;
        const float servo = (e >= 0.f)
            ? sd.hoverStiffness * e - sd.hoverDamping * vUp - sd.gravity
            : glm::max(sd.hoverStiffness * e, -sd.hoverPullDownAccel);
        accelUp += glm::clamp(servo, -sd.hoverMaxAccel, sd.hoverMaxAccel);
    }

    // ⚠ THE TERMINAL CLAMP IS ON THE WHOLE LAW NOW, not on an airborne branch. It never binds
    // while supported at the shipped gains — the servo turns a fall around long before 2000 cm/s —
    // and leaving it unconditional is what keeps this step free of a second `if` on `support`.
    const float velocityUp = glm::max(vUp + accelUp * dt, -sd.terminalFallSpeed);

    // ---- step 5: WRITE ------------------------------------------------------------------
    // Tangential motion stays in the SURFACE frame (u, v); support and gravity go on `up`, the
    // axis step 2 measured `clearance` along. ⭐ On a uniform slope the tangential term moves the
    // body ALONG the plane, so the vertical gap does not change, the servo sees ZERO error while
    // walking and therefore never contributes a horizontal component of its own. That property
    // is exactly what ruling #26 (a) buys.
    state.velocity = u * velocityUV.x + v * velocityUV.y + up * velocityUp;

    // Remembered for NEXT tick's step 6'. ⚠ BOTH of these are SERIALIZED — `positionCmd`
    // since task 50, `flags` since task 11 — and that is the point: step 6' reads exactly
    // what a correction restores, so a replayed tick reproduces the live one.
    //
    // ⭐ [movement-sim task 50] THERE IS ONE COMMAND MARKER NOW, and it is the wire bit.
    // Task 11's off-wire `bool` twin is DELETED, and with it the documented asymmetry in
    // which the two markers deliberately diverged for one replayed tick. `firstResimStep` no
    // longer touches this sub-simulation's state at all (architecture §3.5 always described it
    // as a no-op; it is one again), so there is no tick on which the bit and the behaviour
    // disagree. ⛔ Do not re-introduce an off-wire marker beside this one.
    state.positionCmd = state.bodyState.position;
    state.flags = sd.drivesBody
        ? static_cast<uint8_t>(state.flags | kFlagHasCommand)
        : static_cast<uint8_t>(state.flags & ~kFlagHasCommand);

    if (sd.drivesBody)
    {
        glm::mat4 pose(1.f);
        pose[3] = glm::vec4(state.bodyState.position, 1.f);
        physics.setBodyTransform(bindings.ownBodyId, pose);
        physics.setBodyLinearVelocity(bindings.ownBodyId, state.velocity);
    }
}

}

// SerializableFields specializations for brawlerMovementSimulation types.

// THE TELEPORT SEED, on the wire: a respawn has to replay identically on both peers.
template <>
struct SerializableFields<brawlerMovementSimulation::InitialConditions>
{
    static constexpr auto get()
    {
        using MovementIC = brawlerMovementSimulation::InitialConditions;
        return std::make_tuple(
            SIM_MEMBER(MovementIC, teleportPending),
            SIM_MEMBER(MovementIC, teleportPos));
    }
};

// ⛔ APPEND ONLY. The wire layout is positional; reordering these six entries is a wire
// format change even when the byte count is unmoved.
//
// ⭐ [movement-sim task 50] `positionCmd` APPENDED, +12 B (slice 49 → 61 B, composite
// 309 → 321 B). It is STATE, not scratch — the one input step 6' cannot re-derive — and the
// reason is written out at its declaration. Appending leaves every preceding field at its
// existing offset, so `correctionStateBuffer::kWireFormatVersion` is NOT bumped: the size
// moved, the layout of what came before did not. (Same call task 11 made when it grew this
// slice 24 → 49 B.) ⛔ Task 11's second, off-wire velocity copy and its off-wire `bool`
// command marker are GONE, not merely still absent: the first held exactly `velocity`, which
// is already the second entry below, and the second was a twin of `flags`' bit 3.
template <>
struct SerializableFields<brawlerMovementSimulation::State>
{
    static constexpr auto get()
    {
        using S = brawlerMovementSimulation::State;
        return std::make_tuple(
            SIM_MEMBER(S, bodyState),
            SIM_MEMBER(S, velocity),
            SIM_MEMBER(S, committedStepDir),
            SIM_MEMBER(S, stepStartTick),
            SIM_MEMBER(S, flags),
            SIM_MEMBER(S, positionCmd));
    }
};

template <>
struct SerializableFields<brawlerMovementSimulation::PlayerInput>
{
    static constexpr auto get()
    {
        using MovementInput = brawlerMovementSimulation::PlayerInput;
        return std::make_tuple(
            SIM_MEMBER(MovementInput, flags));
    }
};

static_assert(SimulationState<brawlerMovementSimulation::State>);
static_assert(SimulationInput<brawlerMovementSimulation::PlayerInput>);
static_assert(SimulationInitialConditions<brawlerMovementSimulation::InitialConditions>);

OGSIM_OPTIMIZE_ON

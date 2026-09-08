#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "BrawlerMovementSimulation.h"

#include "glm/vec3.hpp"
#include "glm/common.hpp"        // glm::abs
#include "glm/geometric.hpp"     // glm::dot / glm::length / glm::normalize
#include "glm/trigonometric.hpp" // glm::cos / glm::sin -- the capsule outline's ring, VIZ ONLY

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THE MOVEMENT DEBUG DRAW -- movement-sim task 18.
//
// Read `VISUALIZATION_DISCIPLINE.md` (this directory) before touching this file. Everything
// below is written to that document's §1 invariant, and one line of it is stronger than the
// document asks for: `visualize` has NO mutable parameter at all.
//
//////////////////////////////////////////////////////////////////////////////////////////////
// ⛔⛔ THIS FILE IS A PURE READER, AND THAT IS STRUCTURAL RATHER THAN PROMISED.
//
//   * Every simulation-side parameter is `const&`. There is no `State&` viz slice either --
//     nothing here persists between frames, so there is nothing to hold. §1's rule is
//     "exactly one non-`const` sim-adjacent parameter, and it is the visualization state";
//     ZERO is inside that rule, not an exception to it.
//   * It writes nothing, anywhere. No static, no singleton, no service call, no functor bound
//     to sim-mutating behaviour -- §1's last bullet, the side channel the lint cannot see.
//   * It adds no wire byte, no `StaticData` field and no `DerivedState` field. Deleting this
//     header and its ONE call site in `SimmableUpdateComponent.cpp` leaves the simulation
//     byte-identical. That removability is the task's acceptance condition, not a nicety.
//
// ⚠ THE ONE SIM SYMBOL THIS FILE CALLS is `brawlerMovementSimulation::buildTangentFrame`, and
// calling it is the point rather than a shortcut: it is a pure function of a unit normal with
// no state, no adapter and no engine, and reusing it is what makes the drawn frame THE SAME
// FRAME step 3's models live in. A viz-local re-implementation would be free to drift, and a
// frame that drifted from the sim's would make this instrument lie about the exact property it
// exists to show. §3's checklist question -- "is the include here to READ state, or to INVOKE
// sim behaviour?" -- answers READ: nothing in this file can reach `integrate`.
//////////////////////////////////////////////////////////////////////////////////////////////
//
// ⭐⭐ WHY THIS EXISTS. Every movement defect the user found in PIE during this initiative --
// task 54's wall limit cycle, task 57's slope divergence, the 31x ledge launch, the landing
// deflection -- was found BY FEEL and then diagnosed by reading logs and building out-of-tree
// rigs, because nothing drew the quantities the laws regulate. This draws them. It is also the
// instrument for the still-pending omega / zeta tuning session: `hoverFrequency` and
// `hoverDampingRatio` are provisional feel defaults (see their comments in
// `BrawlerMovementSimulation.h`), and the servo error below is the signal a tuner watches.
//
//////////////////////////////////////////////////////////////////////////////////////////////
// ⚠⚠ RENDER CLOCK READING A SIM-TICK SNAPSHOT -- WHAT IS STALE, AND WHAT IS NOT.
//
// `USimmableUpdateComponent::TickComponent` runs on the RENDER clock. The values it hands this
// function come from `SimulatableBrawler::getVizState()`, which is a whole-`AllState` COPY
// taken by `updateVisualizationAll` in `ASimulationManagerUImpl::OnPostPhysicsStep` -- i.e.
// once per completed SIMULATION tick, at 60 Hz (`AsyncFixedTimeStepSize`).
//
//   STALE BY UP TO ONE SIM TICK (16.67 ms):
//     `state.bodyState.position`, `state.velocity`, `state.flags` (frozen + support bits),
//     `derivedState.surfaceNormal`, `derivedState.lastProbePoint`,
//     `derivedState.lastSupportState`.
//   At a render rate above 60 Hz, consecutive frames redraw the SAME snapshot unchanged. A
//   marker that appears to freeze for a frame or two is the snapshot cadence, not the sim.
//
//   ⭐ NOT STALE, AND NOT AN APPROXIMATION: everything read out of `StaticData` -- `rideHeight`,
//   `snapDistance`, the capsule dimensions, the gains. Those are authored once per session
//   (task 16 made the cvar reads one-time by design), so no drawn band edge is ever a frame
//   behind the value the servo used.
//
//   ⭐⭐ AND THE SNAPSHOT IS INTERNALLY CONSISTENT, WHICH IS THE PROPERTY THE ARITHMETIC BELOW
//   RESTS ON. `State` and `DerivedState` are copied together, from one instant, so
//   `derivedState.lastProbePoint` was measured by step 2 of the SAME tick, cast from the SAME
//   `state.bodyState.position` this file reads: step 2 builds the probe pose from that member
//   and nothing writes it again before the next tick's capture pass. So the probe reading and
//   the body pose can be combined without a tick-skew term.
//
//   ⛔ WHAT IS *NOT* THE SIM'S POSITION: the mesh on screen. The capsule outline is drawn at
//   `state.bodyState.position` deliberately -- the visible character body is
//   `AOGBrawlerUECharacter::HumanoidMesh`, drawn at the ENGINE's current interpolated pose with
//   a `-(halfHeight + rideHeight)` offset. Outline and mesh disagreeing is expected; the
//   outline is the authority on what the simulation believes.
//////////////////////////////////////////////////////////////////////////////////////////////

namespace brawlerMovementVisualization
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THE PALETTE, ALLOCATED ONCE AND WRITTEN DOWN HERE.
//
// Ids index `DAttackRendererFunctorUImpl::idToColor` (Source/OGBrawlerUnreal/
// DAttackCircularVisualizationUImpl.cpp). This draw puts nine distinct glyphs in one place, so
// the allocation is a design decision rather than a per-call-site guess:
//
//     0  Red        capsule outline WHILE FROZEN -- and nothing else in this file
//     1  Green      SupportState::Supported
//     2  Blue       the `up` ray            -- the servo's actuation axis (ruling #26 a)
//     3  Yellow     the RIDE-HEIGHT band marks
//     4  Cyan       SupportState::Unsupported
//     5  Magenta    SupportState::SupportedSteep
//     6  Orange     the `u` ray             -- first surface tangent
//     7  Purple     the `n` ray             -- the SURFACE normal
//     8  Turquoise  the `v` ray             -- second surface tangent
//     9  Emerald    servo error, e >= 0     -- below ride height: the servo is LIFTING
//    10  Silver     the PROBE-REACH band marks (the detach edge)
//    12  White      the velocity vector
//    14  Amber      servo error, e <  0     -- above ride height: gravity-only arm (ruling #29)
//
// ⭐ THE THREE SUPPORT COLOURS ARE THE CONSTRAINED CHOICE, and `SupportedSteep` drove it.
// `SupportedSteep` is the state NOBODY HAS EVER SEEN -- v1's walkable test is a single
// `cosMaxSlope` comparison, so a face past `maxSlopeAngleDeg` classifies `Unsupported` and
// nothing produces `SupportedSteep` until task 48. If it ever appears, the one thing that must
// not happen is a reader mistaking it for `Supported`. Green (0,255,0) and Magenta (255,0,255)
// are exact complements in this palette -- they share no channel -- so the two cannot be
// confused under any lighting. Cyan (0,255,255) differs from Green by the whole blue channel.
//
// ⛔ THE FRAME RAYS DELIBERATELY DO **NOT** USE THE RED/GREEN/BLUE GIZMO CONVENTION. An
// X=red / Y=green / Z=blue triad reads as a right-angled gizmo before anyone looks at the
// geometry, and the whole point of the frame draw below is that it is NOT one. Orange /
// Turquoise / Purple carry no such prior; only `up` keeps blue, because it is the one ray that
// really is world Z.
inline constexpr unsigned int kColorFrozen        = 0u;
inline constexpr unsigned int kColorSupported     = 1u;
inline constexpr unsigned int kColorAxisUp        = 2u;
inline constexpr unsigned int kColorRideHeight    = 3u;
inline constexpr unsigned int kColorUnsupported   = 4u;
inline constexpr unsigned int kColorSteep         = 5u;
inline constexpr unsigned int kColorAxisU         = 6u;
inline constexpr unsigned int kColorNormal        = 7u;
inline constexpr unsigned int kColorAxisV         = 8u;
inline constexpr unsigned int kColorErrorLifting  = 9u;
inline constexpr unsigned int kColorProbeReach    = 10u;
inline constexpr unsigned int kColorVelocity      = 12u;
inline constexpr unsigned int kColorErrorAbove    = 14u;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// DRAW GEOMETRY CONSTANTS. Lengths are CENTIMETRES (the project's world unit) unless the name
// says otherwise, so every one of them is comparable to `rideHeight` (10) and the capsule
// radius (42) by eye.

// The velocity ray is drawn as a DISPLACEMENT, not as a scaled arrow: its length is where the
// body would be in `kVelocityRaySeconds` at this velocity. That keeps it a physical quantity a
// reader can measure against the capsule instead of an arbitrary gain -- a 600 cm/s walk draws
// 60 cm (~1.5 capsule radii), a 2000 cm/s terminal fall draws 200 cm.
inline constexpr float kVelocityRaySeconds = 0.1f;

// The four frame rays. `n` and `up` are drawn LONGER than `u` and `v` so the pair whose
// divergence matters most is the pair that reads first.
inline constexpr float kNormalRayLength  = 60.f;
inline constexpr float kTangentRayLength = 45.f;

// Band marks are drawn as two crossed horizontal arms; see `drawLevelMark` for what the two
// arms mean. Half-lengths, in the same units as the capsule radius.
inline constexpr float kLongArmHalfLength  = 42.f;
inline constexpr float kShortArmHalfLength = 21.f;

inline constexpr float kThinLine  = 1.f;
inline constexpr float kThickLine = 3.f;

// The capsule outline's tessellation. 12 segments per horizontal ring, 6 per cap quarter --
// enough to read as a capsule, cheap enough that the whole outline is ~50 line calls.
inline constexpr unsigned int kRingSegments = 12u;
inline constexpr unsigned int kCapSegments  = 6u;

// ⚠ NOT NAMED `PI`. `PI` is a MACRO in this tree (it arrives through the engine's own headers
// in the UE-side translation units that instantiate this template), so a `constexpr float PI`
// here expands into nonsense at the declaration and produces a misleading diagnostic cascade
// somewhere else entirely.
inline constexpr float kTwoPi = 6.28318530718f;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The renderer seam. Deliberately the SAME shape as
// `brawlerProjectileVisualization::Input` -- one functor, held by value, handed in by the UE
// adapter at the call site. `RendererFunctorType` is a template parameter so this header names
// no engine type and stays compilable outside a UE build.
template <typename RendererFunctorType>
class Input
{
public:
    explicit Input(RendererFunctorType rendererFunctorImpl)
        : m_rendererFunctorImpl(rendererFunctorImpl)
    {}

    RendererFunctorType getRendererFunctorImpl() const { return m_rendererFunctorImpl; }

private:
    RendererFunctorType m_rendererFunctorImpl;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Pure drawing helpers. No state, no sim reads -- they take points and colours.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// An upright capsule outline: two horizontal rings at the hemisphere centres, four vertical
// connectors, and four vertical quarter-arcs closing the top and bottom caps. `halfHeight` is
// the TOTAL half-height (centre to the tip of a hemisphere) -- the same convention
// `StaticData::capsuleHalfHeight` carries and the same one
// `UCapsuleComponent::GetUnscaledCapsuleHalfHeight` documents, so the outline traces the
// authored capsule and not a shape one radius taller or shorter.
//
// ⚠ The ring basis is world X / world Y rather than the surface frame's `u` / `v`. That is
// correct and not a shortcut: the body's descriptor sets `lockRotation`, so the capsule is
// upright in world space regardless of the surface it stands on. Tracing it with a tilted
// frame would draw a shape the simulation does not have.
template <typename RendererFunctorType>
void drawCapsuleOutline(const RendererFunctorType& renderer,
                        const glm::vec3& centre, const glm::vec3& up,
                        float radius, float halfHeight,
                        unsigned int colorId, float thickness)
{
    const glm::vec3 axisX(1.f, 0.f, 0.f);
    const glm::vec3 axisY(0.f, 1.f, 0.f);

    // Hemisphere centres. `halfHeight - radius` is the cylinder's half-length; it is zero for a
    // sphere-shaped capsule and never negative for a legal one.
    const float cylinderHalf = (halfHeight > radius) ? (halfHeight - radius) : 0.f;
    const glm::vec3 top    = centre + up * cylinderHalf;
    const glm::vec3 bottom = centre - up * cylinderHalf;

    const float ringStep = kTwoPi / static_cast<float>(kRingSegments);
    for (unsigned int i = 0u; i < kRingSegments; ++i)
    {
        const float a0 = ringStep * static_cast<float>(i);
        const float a1 = ringStep * static_cast<float>(i + 1u);
        const glm::vec3 r0 = axisX * (radius * glm::cos(a0)) + axisY * (radius * glm::sin(a0));
        const glm::vec3 r1 = axisX * (radius * glm::cos(a1)) + axisY * (radius * glm::sin(a1));
        renderer.drawLine(top + r0,    top + r1,    colorId, thickness);
        renderer.drawLine(bottom + r0, bottom + r1, colorId, thickness);
    }

    // Four vertical connectors, at the two ring axes.
    const glm::vec3 spokes[4] = { axisX * radius, axisY * radius, axisX * -radius, axisY * -radius };
    for (const glm::vec3& spoke : spokes)
        renderer.drawLine(bottom + spoke, top + spoke, colorId, thickness);

    // The caps: a quarter-arc from each spoke up over the top / down under the bottom.
    const float capStep = (kTwoPi * 0.25f) / static_cast<float>(kCapSegments);
    for (const glm::vec3& spoke : spokes)
    {
        const glm::vec3 outward = spoke * (1.f / radius);
        for (unsigned int i = 0u; i < kCapSegments; ++i)
        {
            const float a0 = capStep * static_cast<float>(i);
            const float a1 = capStep * static_cast<float>(i + 1u);
            const glm::vec3 flat0 = outward * (radius * glm::cos(a0));
            const glm::vec3 flat1 = outward * (radius * glm::cos(a1));
            const glm::vec3 rise0 = up * (radius * glm::sin(a0));
            const glm::vec3 rise1 = up * (radius * glm::sin(a1));
            renderer.drawLine(top    + flat0 + rise0, top    + flat1 + rise1, colorId, thickness);
            renderer.drawLine(bottom + flat0 - rise0, bottom + flat1 - rise1, colorId, thickness);
        }
    }
}

// A horizontal level mark on the body's own vertical line.
//
// ⭐⭐ THE TWO ARMS ARE TWO DIFFERENT DERIVATIONS OF THE SAME LEVEL, AND THEY ARE DRAWN AS TWO
// ARMS SO THAT THEY CAN DISAGREE. See `visualize`'s slope-bias block: on a slope the probe
// reads more clearance than the body has, so a probe-referenced level and a body-referenced
// one do not coincide. Averaging them into one mark would erase a real, measured property of
// task 54's inset probe.
//
//     LONG arm, along world X   -- the PROBE-referenced level (what `clearance` is compared to)
//     SHORT arm, along world Y  -- the BODY-referenced level (what the body's own silhouette does)
//
// Two cues, not one: axis AND length. On flat ground the two land at the same height and the
// mark reads as a clean plus sign; on a slope the plus sign splits.
template <typename RendererFunctorType>
void drawLevelMark(const RendererFunctorType& renderer,
                   const glm::vec3& probeReferenced, const glm::vec3& bodyReferenced,
                   unsigned int colorId, float thickness)
{
    const glm::vec3 axisX(1.f, 0.f, 0.f);
    const glm::vec3 axisY(0.f, 1.f, 0.f);
    renderer.drawLine(probeReferenced - axisX * kLongArmHalfLength,
                      probeReferenced + axisX * kLongArmHalfLength, colorId, thickness);
    renderer.drawLine(bodyReferenced - axisY * kShortArmHalfLength,
                      bodyReferenced + axisY * kShortArmHalfLength, colorId, thickness);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The colour a `SupportState` byte draws in. Takes the raw byte because that is what
// `DerivedState::lastSupportState` holds and what `State::flags` bits 1-2 carry; the enum may
// grow to 4 values before that field has to widen, so an unmapped value is possible and falls
// through to the `Unsupported` colour rather than to something that looks deliberate.
inline unsigned int supportColor(uint8_t supportBits)
{
    switch (static_cast<brawlerMovementSimulation::SupportState>(supportBits))
    {
    case brawlerMovementSimulation::SupportState::Supported:      return kColorSupported;
    case brawlerMovementSimulation::SupportState::SupportedSteep: return kColorSteep;
    default:                                                      return kColorUnsupported;
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⭐⭐ THE MOVEMENT DRAW.
//
// Signature per the task: the sim slices arrive as `const&` and there is no mutable parameter
// at all. Called once per render frame per character, from
// `USimmableUpdateComponent::TickComponent`, behind `OGBrawler.Viz.Movement` (default 0).
//
// ⚠ THE CVAR IS READ PER FRAME, AND THAT IS DELIBERATE -- DO NOT "FIX" IT INTO A ONE-TIME READ.
// Task 16 established the one-time-read discipline for `StaticData` cvars, and it is right
// there: a tunable the simulation captures once must not be able to change under a running
// session, because peers would then disagree. A VIZ cvar is the opposite case. Nothing here
// feeds the simulation, and the entire value of a debug draw is that a tuner can type
// `OGBrawler.Viz.Movement 1` mid-session and see the effect on the next frame. The two
// disciplines are not in tension; they apply to different kinds of value.
template <typename RendererFunctorType>
void visualize(const Input<RendererFunctorType>& input,
               const brawlerMovementSimulation::State& state,
               const brawlerMovementSimulation::DerivedState& derivedState,
               const brawlerMovementSimulation::StaticData& staticData)
{
    const auto renderer = input.getRendererFunctorImpl();

    // ⭐ ONE UP AXIS, AND IT IS THE SIM'S OWN CONSTANT. `kWorldUp` is the single feeding point
    // for the servo's measurement and actuation axis (user ruling #26 a); reading it here
    // rather than spelling `(0, 0, 1)` is what keeps this draw honest if tasks 20/48 ever turn
    // that line into a `SupportState` branch.
    const glm::vec3 up = brawlerMovementSimulation::kWorldUp;

    const glm::vec3 position   = state.bodyState.position;
    const float     radius     = staticData.capsuleRadius;
    const float     halfHeight = staticData.capsuleHalfHeight;

    // The body's bottom tip -- the point every vertical quantity below is measured from,
    // because it is also where the ATTACHMENT PROBE's bottom tip starts. Task 54 inset the
    // probe by `kProbeShrink` AND dropped it by the same amount precisely so that those two
    // tips coincide; the derivation is at `kProbeShrink` in `BrawlerMovementSimulation.h`.
    const glm::vec3 bodyBottom = position - up * halfHeight;

    const float probeLength = staticData.rideHeight + staticData.snapDistance;

    // ---- the capsule outline, at the SIM's position ------------------------------------
    // ⛔ Colour is the support state, EXCEPT while frozen, where the whole outline goes red.
    // Freezing (a held guard, or a Hit/Guard flinch) suspends step 3 entirely -- the model's
    // contribution is zeroed EXACTLY on that tick, not decayed -- so "why is my input doing
    // nothing" has a one-glance answer. The support state is still readable off the band marks
    // below while the outline is red.
    const bool frozen = (state.flags & brawlerMovementSimulation::kFlagFrozen) != 0u;
    const unsigned int outlineColor =
        frozen ? kColorFrozen : supportColor(derivedState.lastSupportState);
    drawCapsuleOutline(renderer, position, up, radius, halfHeight,
                       outlineColor, frozen ? kThickLine : kThinLine);

    // ---- the velocity vector ------------------------------------------------------------
    // `state.velocity` is THE SIM's velocity and the only one any movement code reads -- no
    // engine term has ever touched it, and the captured `bodyState.linearVelocity` beside it
    // is deliberately never read (ruling #17 b). Drawing the captured one instead would show
    // contact impulses the simulation does not act on.
    renderer.drawLine(position, position + state.velocity * kVelocityRaySeconds,
                      kColorVelocity, kThickLine);

    // ---- the surface frame, at the probe point -------------------------------------------
    // ⭐⭐⭐ FOUR SEPARATE RAYS FROM ONE ORIGIN, AND EMPHATICALLY NOT A RIGHT-ANGLED GIZMO.
    //
    // `(u, v, up)` SPANS SPACE BUT IS NOT ORTHONORMAL. Ruling #26 (a) made the servo's axis
    // WORLD UP while `(u, v)` stayed slope-relative, so on a face of angle theta:
    //
    //          dot(u, up) = sin(theta)          -- 0 flat, 0.5 at 30 degrees, 0.707 at the 45 cap
    //
    // `v = cross(n, u)` is horizontal and IS perpendicular to both; `u` is not. Task 57 had to
    // add a DUAL-BASIS decomposition (`decomposeVelocity`) because plain dot products against
    // this frame double-count, and the two halves of that were live defects: a fall leaked into
    // the tangential channel and DIVERGED to 1.5e5 cm/s on a slope landing, and a walk leaked
    // into the vertical channel where the servo damped it.
    //
    // ⛔ SO DRAWING A TIDY ORTHOGONAL TRIAD HERE WOULD HIDE THE EXACT PROPERTY BEHIND THIS
    // INITIATIVE'S WORST DEFECT. Four rays, four colours, one shared origin, and NOTHING
    // connecting them -- no right-angle ticks, no arcs, no boxes. The angle between the orange
    // `u` and the blue `up` opening away from 90 degrees as the character walks onto a ramp is
    // the thing worth seeing.
    //
    // ⚠ `n` is `derivedState.surfaceNormal`, which step 2 sets to the probe's normal only when
    // the hit was WALKABLE and to `up` otherwise. So on an `Unsupported` tick the purple ray
    // lying exactly along the blue one does NOT mean "the ground is flat" -- it means "there is
    // no walkable ground, and the frame fell back to world up". That fallback is the sim's, not
    // this draw's.
    const glm::vec3 n = derivedState.surfaceNormal;
    glm::vec3 u(1.f, 0.f, 0.f);
    glm::vec3 v(0.f, 1.f, 0.f);
    brawlerMovementSimulation::buildTangentFrame(n, u, v);

    // ⛔ `lastProbePoint` IS ZERO WHEN THE SWEEP FOUND NOTHING -- that is step 2's own spelling
    // (`probe.blocked ? probe.impactPoint : glm::vec3(0.f)`), and `SpatialQueryResult.h`'s
    // field-validity rule is why: an unblocked sweep has no impact point to report, so there is
    // no "far away" value to read. Treated here as the sentinel it is; a hit landing on the
    // exact world origin would alias, which for a debug draw is acceptable and is recorded
    // rather than defended.
    // Tested as a squared length rather than a vector `!=`: same predicate, no reliance on
    // glm's comparison operators and no float-equality expression for a warning gate to flag.
    const bool probeReported =
        glm::dot(derivedState.lastProbePoint, derivedState.lastProbePoint) > 0.f;

    // ⭐⭐ RECONSTRUCTING THE SIM'S `clearance`, AND WHY IT HAS TO BE RECONSTRUCTED AT ALL.
    //
    // `clearance` is a LOCAL in `integrate` -- it is not in `State` and not in `DerivedState`,
    // so this file cannot read it. Putting it in `DerivedState` would be a one-line simulation
    // change, and this task is a PURE READER by construction: the viz must be removable without
    // touching the simulation. So it is derived from what IS published.
    //
    // THE DERIVATION, exact given a lower-hemisphere contact against the reported plane. The
    // probe is a capsule of radius `r - s` and total half-height `hh - s`, dropped by `s`
    // (`PhysicsSetup::queryVolumes`), so its bottom tip starts at `bodyBottom` and descends
    // `clearance`. At contact its lower hemisphere centre sits `(r - s)` along `n` from the
    // impact point, and its bottom tip sits `(r - s)` along `-up` from that centre:
    //
    //     probeTipAtContact = impactPoint + (r - s) * (n - up)
    //     clearance         = dot(bodyBottom - probeTipAtContact, up)
    //
    // ⭐ ON FLAT GROUND `n == up`, the correction term is IDENTICALLY ZERO, and the reading is
    // simply the height of the body's bottom tip above the impact point -- exact, with no
    // floating-point slack at all.
    //
    // ⭐ VERIFIED NUMERICALLY, NOT ONLY DERIVED (task 18, 2026-09-08). The sim's own sweep
    // arithmetic and this reconstruction were run against an analytic plane at 0, 30 and 45
    // degrees with the shipped 42 / 96 / 1 geometry: the two agree to 3.6e-15 cm at every
    // angle, and the probe-minus-body gap comes out 0 / 0.154701 / 0.414214 cm -- the same
    // `s * (sec(theta) - 1)` closed form `BrawlerMovementSimulation.h`'s `kProbeShrink` block
    // states, reproduced independently here rather than transcribed from it.
    //
    // ⚠ TWO ASSUMPTIONS, STATED SO A READER CAN SEE WHEN TO DISTRUST THE NUMBER:
    //   1. the contact is on the probe's lower HEMISPHERE. A graze on the cylinder wall or a
    //      ledge lip is not, and this reads high or low by the difference.
    //   2. `n` is the surface the probe actually hit. True while `Supported`; on a blocked but
    //      NON-walkable tick `derivedState.surfaceNormal` is the `up` fallback, so the
    //      correction term evaluates to zero against a face that is not flat.
    //   In both cases the servo is off or the classification is `Unsupported` anyway, so the
    //   error marker is decorative there rather than load-bearing. ⭐ The clean fix is a
    //   `float lastClearance` in `DerivedState` written by step 2 -- one line, zero wire, and
    //   it would make this block a single read. It is deliberately NOT taken here.
    //
    // ⭐ AND WHEN THE SWEEP FOUND NOTHING, THERE IS NOTHING TO RECONSTRUCT: step 2 uses
    // `probeLength` itself as the clearance in that case, so this branch is EXACT, not a
    // fallback estimate.
    const glm::vec3 probeTipAtContact =
        derivedState.lastProbePoint + (radius - brawlerMovementSimulation::kProbeShrink) * (n - up);
    const float clearance = probeReported
        ? glm::dot(bodyBottom - probeTipAtContact, up)
        : probeLength;

    // The frame's origin: the measured contact point when there is one, otherwise the end of
    // the sweep -- the deepest point the probe looked at and did not find ground.
    const glm::vec3 frameOrigin = probeReported
        ? derivedState.lastProbePoint
        : bodyBottom - up * probeLength;

    renderer.drawLine(frameOrigin, frameOrigin + n  * kNormalRayLength,  kColorNormal, kThickLine);
    renderer.drawLine(frameOrigin, frameOrigin + up * kNormalRayLength,  kColorAxisUp, kThickLine);
    renderer.drawLine(frameOrigin, frameOrigin + u  * kTangentRayLength, kColorAxisU,  kThickLine);
    renderer.drawLine(frameOrigin, frameOrigin + v  * kTangentRayLength, kColorAxisV,  kThickLine);

    // ---- THE BAND, and task 54's slope bias left visible ---------------------------------
    // ⭐⭐ THE BAND IS "WHERE THE GROUND HAS TO BE". Two levels below the body's bottom tip:
    //
    //     rideHeight                 -- ground here means the servo error is exactly zero
    //     rideHeight + snapDistance  -- the probe's reach. Ground BELOW this is not found at
    //                                   all, the classification goes `Unsupported`, and the
    //                                   servo term stops running.
    //
    // The gap between the measured contact level and the lower mark is the answer to "am I
    // about to detach?", which is invisible without this draw.
    //
    // ⭐⭐ AND THE TWO DERIVATIONS OF EACH LEVEL DO NOT COINCIDE ON A SLOPE. THAT IS CORRECT AND
    // IS LEFT VISIBLE RATHER THAN RECONCILED. Task 54's probe is inset by `kProbeShrink`, so on
    // a face of angle theta it reads
    //
    //          s * (sec(theta) - 1)      MORE clearance than the body actually has
    //
    // -- 0 flat, 0.155 cm at 30 degrees, 0.414 cm at the 45-degree `maxSlopeAngleDeg` cap, at
    // the shipped `kProbeShrink`. Two consequences, and both are drawn:
    //   * the character SETTLES that much lower than `rideHeight` on a slope, because the servo
    //     is closing the PROBE's error, not the body's;
    //   * the probe LETS GO that much sooner, so the body-referenced detach edge sits that much
    //     closer to the body than the nominal reach.
    // ⛔ It is not removable by a different offset -- zeroing it needs a slope-dependent
    // `offset.z = -s / n.z` and a static descriptor cannot express one. A viz that averaged the
    // two marks would erase a real measured property of the shipped probe, so `drawLevelMark`
    // draws them as two arms of one glyph: long arm along world X is the PROBE-referenced
    // level, short arm along world Y is the BODY-referenced one. Flat ground draws a clean plus
    // sign. ⚠ The separation is sub-centimetre by construction, so it reads at close range and
    // not across the arena; the property the draw guarantees is that the two are never merged.
    const float cosTheta  = glm::dot(n, up);
    const float slopeBias = (cosTheta > 0.f)
        ? brawlerMovementSimulation::kProbeShrink * (1.f / cosTheta - 1.f)
        : 0.f;

    const glm::vec3 rideMarkProbe = bodyBottom - up * staticData.rideHeight;
    const glm::vec3 rideMarkBody  = bodyBottom - up * (staticData.rideHeight - slopeBias);
    drawLevelMark(renderer, rideMarkProbe, rideMarkBody, kColorRideHeight, kThickLine);

    const glm::vec3 reachMarkProbe = bodyBottom - up * probeLength;
    const glm::vec3 reachMarkBody  = bodyBottom - up * (probeLength - slopeBias);
    drawLevelMark(renderer, reachMarkProbe, reachMarkBody, kColorProbeReach, kThinLine);

    // ---- THE SERVO ERROR ------------------------------------------------------------------
    // ⭐⭐ `e = rideHeight - clearance` IS THE SCALAR THE HOVER LAW REGULATES, and it is the
    // quantity the pending omega / zeta tuning session is tuning AGAINST. Every hover defect in
    // this initiative was a wrong relationship between this error and the response to it, and
    // the user found all of them BY FEEL because nothing drew it.
    //
    // WHAT IS DRAWN: a bar along `up`, from the measured contact level to the ride-height mark.
    // Its LENGTH is |e| in centimetres, directly comparable to `rideHeight` (10 cm) and to the
    // capsule radius (42 cm) already on screen. Its COLOUR is the sign, and the sign is not a
    // cosmetic distinction -- the two arms of the law are genuinely different:
    //
    //     e >= 0   EMERALD   below ride height: the full spring-damper runs and the servo is
    //                        LIFTING (ruling #28's `k*e - c*vUp - gravity`, clamped).
    //     e <  0   AMBER     above ride height: the one-sided arm (ruling #29). At the shipped
    //                        `hoverPullDownAccel` of 0 the vertical law up here is GRAVITY AND
    //                        NOTHING ELSE -- supported means held up, never pulled down. Amber
    //                        therefore means "the servo is not holding you right now", which is
    //                        a different statement from emerald and deserves its own colour.
    //
    // ⚠ A ONE-TICK AMBER FLICKER WHILE WALKING IS EXPECTED AND IS NOT A DEFECT: a tick that
    // lands a float hair above ride height gets gravity alone and falls |gravity|*dt^2 = 0.272
    // cm before the servo takes it back. Task 57 predicted that ripple and priced it; this draw
    // is where it becomes visible.
    const float servoError = staticData.rideHeight - clearance;
    const glm::vec3 contactLevel = bodyBottom - up * clearance;
    const unsigned int errorColor = (servoError >= 0.f) ? kColorErrorLifting : kColorErrorAbove;
    renderer.drawLine(contactLevel, rideMarkProbe, errorColor, kThickLine);

    // The contact level itself, as a short cross, so the bar has a foot a reader can see even
    // when |e| is small enough that the bar is a dot.
    {
        const glm::vec3 axisX(1.f, 0.f, 0.f);
        const glm::vec3 axisY(0.f, 1.f, 0.f);
        renderer.drawLine(contactLevel - axisX * kShortArmHalfLength,
                          contactLevel + axisX * kShortArmHalfLength, errorColor, kThickLine);
        renderer.drawLine(contactLevel - axisY * kShortArmHalfLength,
                          contactLevel + axisY * kShortArmHalfLength, errorColor, kThickLine);
    }
}

} // namespace brawlerMovementVisualization

OGSIM_OPTIMIZE_ON
// pragma optimize on.

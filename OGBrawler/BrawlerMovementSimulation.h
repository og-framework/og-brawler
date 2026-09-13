#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerMovementSimulation-rationale.md · docs/BrawlerMovementSimulation-guards.md

#include "OGSimulation/OGTypes.h"
#include <vector>
#include <cstdint>
#include <type_traits>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "glm/exponential.hpp"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/PhysicsDeclaration.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGSimulation/SpatialQueryResult.h"
#include "OGSimulation/OGAssert.h"
#include "OGBrawler/CollisionCategoryConstants.h"
#include "OGBrawler/BrawlerCharacterBindings.h"
#include "OGBrawler/DAttackMachineSimulation.h"
#include "OGBrawler/BrawlerInboundHit.h"
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerMovementSimulation
{

enum class MovementModel : uint8_t
{
    ContinuousAccelBrake = 0,
    Cadence = 1
};

enum class SupportState : uint8_t
{
    Unsupported    = 0,
    Supported      = 1,
    SupportedSteep = 2
};

static_assert(static_cast<uint8_t>(SupportState::Unsupported)    == 0u
           && static_cast<uint8_t>(SupportState::Supported)      == 1u
           && static_cast<uint8_t>(SupportState::SupportedSteep) == 2u,
    "brawlerMovementSimulation::SupportState - these three values ride State::flags bits 1-2 "
    "and are ON THE WIRE. The enum may grow to FOUR values (0-3) before the flags byte has to "
    "widen; a fifth is a wire change, not an enumerator.");

inline constexpr float kNominalSimStepSeconds = 1.f / 60.f;

constexpr bool hoverGainsAreStable(float omega, float zeta)
{
    const float W = omega * kNominalSimStepSeconds;
    return W * W + 4.f * zeta * W < 4.f;
}

static_assert(!hoverGainsAreStable(60.f, 1.f) && !hoverGainsAreStable(119.f, 1.f),
    "brawlerMovementSimulation::StaticData - the hover stability bound is W^2 + 4*zeta*W < 4, "
    "NOT the textbook explicit-Euler `omega*dt < 2`. omega = 60 zeta = 1 DIVERGES "
    "(1, 1, 2, 3, 5, 8, 13 ...) and omega = 119 zeta = 1 reaches 1e6 in eight ticks; both PASS "
    "`omega < 120`. The step is semi-implicit and the damping term is part of the region - the "
    "Jury derivation is above StaticData's constructor. Was fence T3-5.");
static_assert(hoverGainsAreStable(46.f, 0.62f),
    "VACUITY CONTROL for the assertion above - the pair shipped when this landed (omega 46, "
    "zeta 0.62, W^2 + 4*zeta*W = 2.489) must PASS the bound. Without it a predicate that "
    "rejects EVERYTHING satisfies the assertion above. Was fence T3-5.");

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
        float launchDecel,
        float dashSpeed, uint32_t dashTicks, uint32_t dashCancelTick,
        float capsuleRadius, float capsuleHalfHeight)
        : model(model)
        , maxWalkSpeed(maxWalkSpeed)
        , acceleration(acceleration)
        , brakingDeceleration(brakingDeceleration)
        , stepPeriodTicks(stepPeriodTicks)
        , stepSpeed(stepSpeed)
        , maxSlopeAngleDeg(maxSlopeAngleDeg)
        , cosMaxSlope(glm::cos(glm::radians(maxSlopeAngleDeg)))
        , gravity(gravity)
        , terminalFallSpeed(terminalFallSpeed)
        , rideHeight(rideHeight)
        , snapDistance(snapDistance)
        , hoverFrequency(hoverFrequency)
        , hoverDampingRatio(hoverDampingRatio)
        , hoverMaxAccel(hoverMaxAccel)
        , hoverPullDownAccel(hoverPullDownAccel)
        , hoverStiffness(hoverFrequency * hoverFrequency)
        , hoverDamping(2.f * hoverDampingRatio * hoverFrequency)
        , launchDecel(launchDecel)
        , dashSpeed(dashSpeed)
        , dashTicks(dashTicks)
        , dashCancelTick(dashCancelTick)
        , capsuleRadius(capsuleRadius)
        , capsuleHalfHeight(capsuleHalfHeight)
    {
        // ⛔G-01  docs/BrawlerMovementSimulation-guards.md
        OG_CHECK(hoverGainsAreStable(hoverFrequency, hoverDampingRatio),
            "brawlerMovementSimulation::StaticData - hover gains outside the discrete stability "
            "region W^2 + 4*zeta*W < 4 (W = hoverFrequency * the 60 Hz sim step). The servo would "
            "diverge. Lower hoverFrequency or raise hoverDampingRatio.");
        OG_CHECK(hoverFrequency > 0.f && hoverDampingRatio > 0.f && hoverMaxAccel > 0.f,
            "brawlerMovementSimulation::StaticData - hover gains must be positive");
        // ⛔G-02  docs/BrawlerMovementSimulation-guards.md
        OG_CHECK(hoverPullDownAccel >= 0.f,
            "brawlerMovementSimulation::StaticData - hoverPullDownAccel is a magnitude and "
            "must not be negative (0 = gravity and nothing else above ride height)");
    }

    StaticData(const StaticData&) = default;

    MovementModel model;

    bool drivesBody = true;

    float maxWalkSpeed, acceleration, brakingDeceleration;
    uint32_t stepPeriodTicks;
    float stepSpeed;

    float maxSlopeAngleDeg;
    float cosMaxSlope;

    float gravity;
    float terminalFallSpeed;

    float rideHeight, snapDistance;

    // ∴D-05  docs/BrawlerMovementSimulation-rationale.md
    float hoverFrequency;
    // ∴D-03  docs/BrawlerMovementSimulation-rationale.md
    float hoverDampingRatio;
    float hoverMaxAccel;

    // ∴D-04  docs/BrawlerMovementSimulation-rationale.md
    float hoverPullDownAccel;
    float hoverStiffness, hoverDamping;

    float launchDecel;

    float dashSpeed;
    uint32_t dashTicks, dashCancelTick;

    float capsuleRadius, capsuleHalfHeight;
};

// ⛔G-03  docs/BrawlerMovementSimulation-guards.md
inline constexpr float kProbeShrink = 1.f;

inline constexpr BodyDescriptor kCharacterCapsuleBody{
    .simulatePhysics = true,
    .enableGravity = false,
    .isRoot = true,
    .lockRotation = true,
    .resimPolicy = BodyResimPolicy::Resimulate
};

static_assert(kCharacterCapsuleBody.enableGravity == false,
    "brawlerMovementSimulation::PhysicsSetup - the adopted capsule's ENGINE gravity must stay "
    "FALSE. Gravity is the SIM's law (StaticData::gravity, applied on EVERY tick since task 56); "
    "with the body re-placed from State every tick, engine gravity would double-apply against "
    "that term AND perturb the post-solve position the push-out in step 6' is measured from. "
    "Was fence T3-10.");

static_assert(kCharacterCapsuleBody.simulatePhysics
           && kCharacterCapsuleBody.isRoot
           && kCharacterCapsuleBody.lockRotation
           && kCharacterCapsuleBody.resimPolicy == BodyResimPolicy::Resimulate,
    "brawlerMovementSimulation::PhysicsSetup - the adopted capsule is a SIMULATED, ROTATION-"
    "LOCKED ROOT that the engine RE-SOLVES on a replay. `simulatePhysics` false here while "
    "StaticData::drivesBody stays true gives two authorities on one component; `isRoot` false "
    "sends the factory down the CREATE path instead of adopting the ACharacter's own capsule; "
    "`lockRotation` false makes LinearBodyState's dropped rotation a lie; and "
    "ReplayRecordedHistory would discard exactly the push-out step 6' consumes.");

struct PhysicsSetup
{
    static inline const PhysicalObjectDescriptor body{
        kCharacterCapsuleBody,
        {
            ShapeDescriptor{
                CapsuleGeometry{42.f, 96.f},
                CollisionCategories::single(collisionCategory::character),
                collisionCategory::worldAndCharacter
            }
        }
    };

    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& sd)
    {
        glm::mat4 probeOffset(1.f);
        // ∴D-02  docs/BrawlerMovementSimulation-rationale.md
        probeOffset[3] = glm::vec4(0.f, 0.f, -kProbeShrink, 1.f);

        const float probeRadius     = sd.capsuleRadius     - kProbeShrink;
        const float probeHalfHeight = sd.capsuleHalfHeight - kProbeShrink;

        OG_CHECK(probeRadius     == sd.capsuleRadius     + probeOffset[3].z
              && probeHalfHeight == sd.capsuleHalfHeight + probeOffset[3].z,
            "brawlerMovementSimulation::PhysicsSetup::queryVolumes - the ground probe's SHRINK "
            "and its DROP are ONE TERM: probeBottom == bodyBottom requires the drop to equal "
            "the inset exactly. Move one without the other and the probe stops measuring the "
            "body's own clearance. Was fence T3-12.");

        return std::vector<QueryVolumeDescriptor>{
            QueryVolumeDescriptor{
                CapsuleGeometry{probeRadius, probeHalfHeight},
                collisionCategory::worldOnly,
                probeOffset,
                collisionCategory::queryRouting
            }
        };
    }
};

using RuntimeBindings = PhysicsRuntimeBindings;

// ⛔G-04  docs/BrawlerMovementSimulation-guards.md
inline constexpr glm::vec3 kWorldUp{0.f, 0.f, 1.f};

static_assert(kWorldUp.x == 0.f && kWorldUp.y == 0.f && kWorldUp.z == 1.f,
    "brawlerMovementSimulation::kWorldUp - user ruling #26 (a): the hover servo MEASURES and "
    "ACTUATES on WORLD up. Tasks 20 and 48 make the axis follow SupportState; that is one "
    "branch on step 2's local, not a new value here.");

class DerivedState
{
public:
    glm::vec3 surfaceNormal{kWorldUp};
    glm::vec3 lastProbePoint{0.f};
    glm::vec3 lastPushOut{0.f};
    uint8_t   lastSupportState = 0;
};

// ⛔G-05  docs/BrawlerMovementSimulation-guards.md
class PlayerInput
{
public:
    uint8_t flags = 0u;

    static PlayerInput zero() { return PlayerInput{}; }
};

inline constexpr uint8_t kInputFlagHoldGuard = 1u << 0;

static_assert(kInputFlagHoldGuard == (1u << 0),
    "brawlerMovementSimulation::PlayerInput::flags - bit 0 is holdGuard, and it is ON THE "
    "RELAYED INPUT RING. Moving it desynchronises every peer that has not shipped the same "
    "edit; bits 1-7 are where jump, dash, wall-grab and ski-tuck go.");

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
    uint32_t getCurrentTick() const { return m_currentTick; }
    PhysicsBodyAdapterType& getPhysicsAdapter() const { return m_physicsBodyAdapter; }
    SpatialQueryAdapterType& getQueryAdapter() const { return m_queryAdapter; }

private:
    float m_deltaTime;
    uint32_t m_currentTick;
    PhysicsBodyAdapterType& m_physicsBodyAdapter;
    SpatialQueryAdapterType& m_queryAdapter;
};

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsBodyAdapterType, SpatialQueryAdapterType>>;

class InitialConditions
{
public:
    uint32_t  teleportPending = 0;
    glm::vec3 teleportPos{0.f};
};

class State
{
public:
    // ⛔G-06  docs/BrawlerMovementSimulation-guards.md
    LinearBodyState bodyState;

    glm::vec3 velocity{0.f};

    glm::vec2 committedStepDir{0.f};

    uint32_t stepStartTick = 0;

    uint8_t flags = 0;

    glm::vec3 positionCmd{0.f};
};

namespace detail
{
template <typename S>
concept StateHasExactlySixMembers = requires(S& s) {
    [](S& x) { auto& [bodyState, velocity, committedStepDir, stepStartTick, flags, positionCmd] = x;
               (void)bodyState; (void)velocity; (void)committedStepDir;
               (void)stepStartTick; (void)flags; (void)positionCmd; }(s); };
}

static_assert(detail::StateHasExactlySixMembers<State>,
    "brawlerMovementSimulation::State - THERE IS NO SEVENTH MEMBER, and that is not an omission. "
    "Task 11's off-wire commanded-velocity copy held exactly `state.velocity`, which is already "
    "on the wire; its off-wire command marker was a twin of `flags` bit 3. Both are GONE, not "
    "merely still absent, and an off-wire marker beside the wire bit is the reflex fix for a "
    "replay bug - it is the one this file already paid for. Was fences T3-16 and T3-25.");

inline constexpr uint8_t kFlagFrozen       = 1u << 0;
inline constexpr uint8_t kFlagSupportShift = 1u;
inline constexpr uint8_t kFlagSupportMask  = 0x06u;
inline constexpr uint8_t kFlagHasCommand   = 1u << 3;

static_assert(kFlagFrozen == (1u << 0) && kFlagSupportShift == 1u
           && kFlagSupportMask == 0x06u && kFlagHasCommand == (1u << 3),
    "brawlerMovementSimulation::State::flags - the bit assignment is ON THE WIRE and in the "
    "checksum. Moving a bit desynchronises every peer that has not shipped the same edit, and "
    "the mask and the shift have to move together or SupportState reads the wrong two bits.");

inline constexpr float kPushOutEps = 0.05f;

struct PhysicsDeclaration
{
    static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }
    static constexpr const char* name = "CharacterCapsule";

    template <typename GameStaticDataType>
    static const StaticData& staticDataOf(const GameStaticDataType& gsd) { return gsd.m_movementStaticData; }

    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& sd) {
        return PhysicsSetup::queryVolumes(sd);
    }
    static glm::vec3 attachmentOffset(const StaticData& sd) {
        (void)sd;
        return glm::vec3(0.f, 0.f, 0.f);
    }

    using StateType = brawlerMovementSimulation::State;
    static       LinearBodyState& bodyStateOf(      StateType& s) { return s.bodyState; }
    static const LinearBodyState& bodyStateOf(const StateType& s) { return s.bodyState; }

    RuntimeBindings bindings;
};

struct Dependencies {
    using Owned = OwnedDeps<
        brawlerMovementSimulation::InitialConditions,
        brawlerMovementSimulation::State>;
    using External = ExternalDeps<const dAttackMachineSimulation::State&>;
    using InputType = brawlerMovementSimulation::PlayerInput;
    Owned owned;
    External external;
};

inline glm::vec2 moveTowards(glm::vec2 current, glm::vec2 target, float maxDelta)
{
    const glm::vec2 delta = target - current;
    const float distanceSq = glm::dot(delta, delta);
    if (distanceSq <= maxDelta * maxDelta || distanceSq <= 0.f)
        return target;
    return current + delta * (maxDelta / glm::sqrt(distanceSq));
}

inline void buildTangentFrame(const glm::vec3& n, glm::vec3& u, glm::vec3& v)
{
    const glm::vec3 reference = (glm::abs(n.x) < 0.9f)
        ? glm::vec3(1.f, 0.f, 0.f)
        : glm::vec3(0.f, 1.f, 0.f);
    u = glm::normalize(reference - n * glm::dot(reference, n));
    v = glm::cross(n, u);
}

struct VelocityChannels
{
    glm::vec2 uv;
    float     up;
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

    // ∴D-01  docs/BrawlerMovementSimulation-rationale.md
    const float a = (glm::dot(planar, u) - glm::dot(planar, up) * s) / (1.f - s * s);
    const float c =  glm::dot(planar, up) - a * s;
    return VelocityChannels{ glm::vec2(a, b), c };
}

// ⛔G-22  docs/BrawlerMovementSimulation-guards.md
inline bool machineFreezesMovement(const dAttackMachineSimulation::State& machineState)
{
    return (machineState.m_currentState == DAttackState::HitFlinch
                && machineState.m_hitReaction == HitReactionKind::Stun)
        || machineState.m_currentState == DAttackState::GuardFlinch;
}

inline bool machineLaunchesMovement(const dAttackMachineSimulation::State& machineState)
{
    return machineState.m_currentState == DAttackState::HitFlinch
        && machineState.m_hitReaction == HitReactionKind::Knockback;
}

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

inline glm::vec2 computeDesiredVelocityUV_Cadence(
    const StaticData& sd, State& state, uint32_t tick,
    glm::vec2 stickUV, glm::vec2 currentUV, float dt)
{
    (void)currentUV;
    (void)dt;
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

namespace detail
{
template <typename E> inline constexpr bool kHasLaunched  = requires { E::Launched;  };
template <typename E> inline constexpr bool kHasHitFlinch = requires { E::HitFlinch; };
}

static_assert(!detail::kHasLaunched<DAttackState>,
    "brawlerMovementSimulation::detachesFromSupport - `DAttackState::Launched` NOW EXISTS, and "
    "THERE IS NO `Launched` BY DESIGN (task 27, user ruling 2026-09-12): `HitFlinch` PLUS "
    "`dAttackMachineSimulation::State::m_hitReaction` IS the hit-reaction state, and the arm "
    "below is keyed on VELOCITY - `committed && dot(velocity, up) > 0` - not on an enumerator, so "
    "it is false for every XY knockback and true the day a lift is authored, with no enumerator "
    "knowledge at all. A fifth DAttackState bumps `kDAttackStateCount`, moves the visualizer's "
    "`kMachineStateCellCount` fence and four switch sites in another initiative's files, and "
    "leaves `HitFlinch` with no writer. Re-read that decision before deleting this line. "
    "Was fence T3-17 (the Launched half).");
static_assert(detail::kHasHitFlinch<DAttackState>,
    "VACUITY CONTROL for the assertion above - if DAttackState is renamed, moved or gutted, "
    "`!kHasLaunched` goes SILENTLY TRUE and the tripwire is gone with no diagnostic anywhere. "
    "Was fence T3-17.");

// ⛔G-07  docs/BrawlerMovementSimulation-guards.md
inline bool detachesFromSupport(const State& state, const glm::vec3& up, bool committed)
{
    return committed && glm::dot(state.velocity, up) > 0.f;
}

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void integrate(float deltaSeconds,
    const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
    const dAttackMachineSimulation::PlayerInput& machineInput,
    const StaticData& sd,
    Dependencies deps,
    const RuntimeBindings& bindings,
    DerivedState& derivedState,
    const brawlerInboundHit::DerivedState& inboundHit)
{
    auto& utils   = input.getIntegrationUtils();
    auto& physics = utils.getPhysicsAdapter();
    auto& query   = utils.getQueryAdapter();
    const float dt        = deltaSeconds;
    const uint32_t tick   = utils.getCurrentTick();

    State& state = deps.owned.edit<State>();
    InitialConditions& ic = deps.owned.edit<InitialConditions>();

    {
        glm::vec3 pushOut(0.f);
        if ((state.flags & kFlagHasCommand) != 0u)
        {
            const glm::vec3 predicted = state.positionCmd + state.velocity * dt;
            pushOut = state.bodyState.position - predicted;
            if (glm::dot(pushOut, pushOut) > kPushOutEps * kPushOutEps)
            {
                // ⛔G-08  docs/BrawlerMovementSimulation-guards.md
                const glm::vec3 contactNormal = glm::normalize(pushOut);
                if (glm::dot(contactNormal, kWorldUp) >= sd.cosMaxSlope)
                    state.velocity -= glm::dot(state.velocity, kWorldUp) * kWorldUp;
                else
                    state.velocity -= glm::dot(state.velocity, contactNormal) * contactNormal;
            }
        }
        derivedState.lastPushOut = pushOut;
    }

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

        OGBLOG_G("[Warning][Movement.hover] tick=%u omega=%.3f zeta=%.3f "
                 "(discrete critical zeta = %.3f; monotone needs zeta >= that AND omega < %.1f) "
                 "k=%.1f c=%.2f maxAccel=%.0f rideHeight=%.2f snapDistance=%.2f",
            tick, sd.hoverFrequency, sd.hoverDampingRatio,
            1.f - sd.hoverFrequency * dt * 0.5f, 1.f / dt,
            sd.hoverStiffness, sd.hoverDamping, sd.hoverMaxAccel,
            sd.rideHeight, sd.snapDistance);
    }

    const dAttackMachineSimulation::State& machineState =
        deps.external.get<dAttackMachineSimulation::State>();

    const bool frozen = (input.getPlayerInput().flags & kInputFlagHoldGuard) != 0u
        || machineFreezesMovement(machineState);

    const bool committed = machineLaunchesMovement(machineState);

    const glm::vec3 up = kWorldUp;

    const float probeLength = sd.rideHeight + sd.snapDistance;
    glm::mat4 probePose(1.f);
    probePose[3] = glm::vec4(state.bodyState.position, 1.f);

    SweepHit probe;
    if (!bindings.queryVolumeIds.empty())
    {
        probe = query.sweep(bindings.queryVolumeIds[0], probePose, -up * probeLength);
    }

    const float clearance = probe.blocked ? probe.fraction * probeLength : probeLength;
    const bool walkable   = probe.blocked && glm::dot(probe.normal, up) >= sd.cosMaxSlope;

    const SupportState support = (walkable && !detachesFromSupport(state, up, committed))
        ? SupportState::Supported
        : SupportState::Unsupported;

    const glm::vec3 n = walkable ? probe.normal : up;
    glm::vec3 u(1.f, 0.f, 0.f);
    glm::vec3 v(0.f, 1.f, 0.f);
    buildTangentFrame(n, u, v);

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

    const VelocityChannels channels = decomposeVelocity(state.velocity, u, v, up);
    const glm::vec2 currentUV = channels.uv;

    glm::vec2 velocityUV(0.f);
    // ⛔G-23  docs/BrawlerMovementSimulation-guards.md
    if (committed)
    {
        if (inboundHit.wasHitThisTick)
            // ∴D-06  docs/BrawlerMovementSimulation-rationale.md
            velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed;
        else
            velocityUV = moveTowards(currentUV, glm::vec2(0.f), sd.launchDecel * dt);
    }
    else if (frozen)
    {
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

    // ⛔G-09  docs/BrawlerMovementSimulation-guards.md
    const float vUp = channels.up;

    float accelUp = sd.gravity;

    // ⛔G-11  docs/BrawlerMovementSimulation-guards.md
    if (support != SupportState::Unsupported)
    {
        // ⛔G-10  docs/BrawlerMovementSimulation-guards.md
        const float e = sd.rideHeight - clearance;
        const float servo = (e >= 0.f)
            ? sd.hoverStiffness * e - sd.hoverDamping * vUp - sd.gravity
            : glm::max(sd.hoverStiffness * e, -sd.hoverPullDownAccel);
        accelUp += glm::clamp(servo, -sd.hoverMaxAccel, sd.hoverMaxAccel);
    }

    const float velocityUp = glm::max(vUp + accelUp * dt, -sd.terminalFallSpeed);

    state.velocity = u * velocityUV.x + v * velocityUV.y + up * velocityUp;

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

static_assert(std::is_same_v<
        decltype(SerializableFields<brawlerMovementSimulation::State>::get()),
        std::tuple<
            MemberFieldDesc<&brawlerMovementSimulation::State::bodyState>,
            MemberFieldDesc<&brawlerMovementSimulation::State::velocity>,
            MemberFieldDesc<&brawlerMovementSimulation::State::committedStepDir>,
            MemberFieldDesc<&brawlerMovementSimulation::State::stepStartTick>,
            MemberFieldDesc<&brawlerMovementSimulation::State::flags>,
            MemberFieldDesc<&brawlerMovementSimulation::State::positionCmd>>>,
    "brawlerMovementSimulation::State - APPEND ONLY. The wire layout is POSITIONAL: reordering, "
    "inserting or removing an entry is a WIRE FORMAT CHANGE even when the byte count does not "
    "move, and every peer that has not shipped the same edit desynchronises. If you appended a "
    "field, append it here too and bump correctionStateBuffer::kWireFormatVersion if anything "
    "before it moved. Was fences T3-3 and T3-26.");

static_assert(std::is_same_v<
        decltype(SerializableFields<brawlerMovementSimulation::InitialConditions>::get()),
        std::tuple<
            MemberFieldDesc<&brawlerMovementSimulation::InitialConditions::teleportPending>,
            MemberFieldDesc<&brawlerMovementSimulation::InitialConditions::teleportPos>>>,
    "brawlerMovementSimulation::InitialConditions - APPEND ONLY, same rule as State above: the "
    "teleport seed rides the wire so a respawn replays identically on both peers. "
    "Was fences T3-3 and T3-26.");

static_assert(std::is_same_v<
        decltype(SerializableFields<brawlerMovementSimulation::PlayerInput>::get()),
        std::tuple<
            MemberFieldDesc<&brawlerMovementSimulation::PlayerInput::flags>>>,
    "brawlerMovementSimulation::PlayerInput - APPEND ONLY, same rule as State above, and an "
    "input byte costs about TEN TIMES a state byte: it is multiplied across every ring entry in "
    "the packet-budget scenario. A new per-tick signal is a BIT in this byte, never a new "
    "member. Was fences T3-3 and T3-26.");

static_assert(SimulationState<brawlerMovementSimulation::State>);
static_assert(SimulationInput<brawlerMovementSimulation::PlayerInput>);
static_assert(SimulationInitialConditions<brawlerMovementSimulation::InitialConditions>);

OGSIM_OPTIMIZE_ON

#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <array>
#include <vector>
#include <limits>
#include <cmath>
#include <cstdint>
#include <algorithm>   // std::remove_if (erase-remove prune)
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/geometric.hpp"      // dot, length, normalize
#include "glm/trigonometric.hpp"  // acos
#include "glm/common.hpp"         // clamp
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/PhysicsBodyAdapter.h"
#include "OGSimulation/SpatialQueryAdapter.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGSimulation/SpatialQueryResult.h"
#include "OGBrawler/CollisionCategoryConstants.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawlerLog.h"

#pragma optimize("", off)

namespace brawlerProjectileSimulation
{

// Compile-time CAPACITY of the projectile pool. Drives the template-instantiation
// count (PhysicsDeclaration<0..N-1>), the std::array<RuntimeBindings, N> sizing, and
// the SIM_VECTOR wire-buffer capacity — all of which must be compile-time-stable.
// The RUNTIME number of usable slots is the configurable StaticData::projectilePoolSize
// field (R-P1: a wire-affecting sizing knob is read from a config struct, never a second
// literal). projectilePoolSize must satisfy 0 < projectilePoolSize <= kMaxProjectilePoolSize.
static constexpr uint32_t kMaxProjectilePoolSize = 3;

// Off-world parking position — far below the playfield so parked bodies never interact.
static constexpr float kParkZ = -100000.f;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StaticData
{
public:
    // guardMiddleSectionHalfAngle is the 6th ctor parameter (T29; narrowed in T31). It
    // is defaulted to 0.25 rad (≈14.3°) to match the radial sim's middle-section block
    // threshold, so existing 5-arg construction sites and the projectile LLT fixtures
    // continue to compile unchanged while still getting the intended middle-section cone.
    StaticData(float projectileSpeed,
               float maxLifetime,
               float colliderRadius,
               float spawnForwardOffset,
               float spawnZOffset,
               float guardMiddleSectionHalfAngle = 0.25f,
               float innerCircleRadius = 90.f,
               uint32_t indicatorPersistTicks = 20)
        : projectileSpeed(projectileSpeed)
        , maxLifetime(maxLifetime)
        , colliderRadius(colliderRadius)
        , spawnForwardOffset(spawnForwardOffset)
        , spawnZOffset(spawnZOffset)
        , guardMiddleSectionHalfAngle(guardMiddleSectionHalfAngle)
        , innerCircleRadius(innerCircleRadius)
        , indicatorPersistTicks(indicatorPersistTicks)
    {}

    StaticData(const StaticData&) = default;

    float projectileSpeed;
    float maxLifetime;
    float colliderRadius;
    float spawnForwardOffset;
    float spawnZOffset;

    // T29/T31 — half-angle (radians) of the guard's MIDDLE block section.
    // Matches DAttackRadialSimulation.h:450 'shieldAngle' middle-section threshold.
    // Projectile direction within this half-angle of the target's guard forward = block;
    // outside it (within the outer cone OR beyond) = damage hit. Gameplay tuning value
    // (not TimeConfig-governed), so no R-P1 lint blacklist entry.
    float guardMiddleSectionHalfAngle;

    // T30 — radius (cm) of the target character's inner attack circle. A blocked
    // projectile's indicator is placed where the launch ray crosses this circle
    // (the edge facing the shooter) rather than at the character root, so the block
    // marker reads as "stopped at the guard". Duplicated explicitly here (set from
    // the brawler attack circle inner radius in SimulatableBrawlerTypes.h) so the
    // projectile sub-sim stays self-contained. Gameplay tuning — no R-P1 entry.
    float innerCircleRadius;

    // T30 — how long (in sim ticks) a hit/block indicator persists in DerivedState
    // before integrate prunes it. 20 ≈ 0.333 s at 60 Hz (matches the radial guard-hit
    // draw duration). Gameplay/viz tuning — not TimeConfig-governed, no R-P1 entry.
    uint32_t indicatorPersistTicks;

    // R-P1 (Synthesis Addendum Correction 5): the runtime pool size is a config-struct
    // field, the single source of truth read at runtime via sd.projectilePoolSize.
    // This is the ONLY place the default literal (3) is declared — the configurability
    // lint (tools/lint/configurability_lint.ps1) flags any second declaration. Must stay
    // 0 < projectilePoolSize <= kMaxProjectilePoolSize (integrate clamps defensively).
    uint32_t projectilePoolSize = 3;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PhysicsSetup
{
    static inline const PhysicalObjectDescriptor body{
        BodyDescriptor{
            .simulatePhysics = true,
            .enableGravity = false
        },
        {
            ShapeDescriptor{
                SphereGeometry{30.f},
                CollisionCategories::single(collisionCategory::body)
            }
        }
    };

    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& staticData)
    {
        return {
            QueryVolumeDescriptor{
                SphereGeometry{staticData.colliderRadius},
                collisionCategory::bodyAndGuard,
                glm::mat4(1.f),
                collisionCategory::queryRouting
            }
        };
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct RuntimeBindings
{
    BodyId ownBodyId;
    BodyId parentBodyId;
    glm::vec3 attachmentOffset;
    std::vector<ShapeId> shapeIds;
    std::vector<QueryVolumeId> queryVolumeIds;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Closed-form trajectory slot (Task 13). The on-wire state carries ONLY the
// launch parameters; the per-tick world position is DERIVED from the formula
//   pos(t) = spawnPos + spawnDir * projectileSpeed * dt * (currentTick - spawnTick)
// and snapped onto the physics body each tick. This (a) shrinks the per-slot
// wire footprint, (b) makes the projectile fully deterministic from launch (no
// per-tick position to re-sync during resim), and (c) honors R-T5 — spawnTick /
// spawnPos / spawnDir are append-only once a slot is live (never revised while
// the slot is active).
//
// `spawnTick == 0` is the FREE sentinel (never spawned, or already recycled).
// Production sim ticks are >= 1, so tick 0 is reserved and a projectile is never
// spawned on tick 0.
struct ProjectileSlot
{
    uint32_t  spawnTick      = 0;   // 0 == free slot (never spawned / recycled)
    glm::vec3 spawnPos       {};
    glm::vec3 spawnDir       {};    // unit vector; velocity = spawnDir * projectileSpeed
    uint32_t  endTick        = 0;   // 0 == still alive; >0 == ended at this tick
    uint8_t   endReason      = 0;   // 0=alive, 1=lifetimeExpired, 2=hit
    int32_t   hitObjectIndex = -1;  // meaningful only when endReason == 2

    // Transient, LOCAL-ONLY body state. Recomputed each tick from the closed
    // form and used by the physics composite's captureBodyStatesAll() (which
    // needs a PhysicsBodyState lvalue via PhysicsDeclaration::bodyStateOf) and
    // by the visualization layer. Deliberately EXCLUDED from SerializableFields
    // so it never hits the wire and never participates in correction/similarity.
    PhysicsBodyState bodyState;

    // A slot is alive iff it was spawned and the current tick is before its end.
    bool isAlive(uint32_t currentTick) const
    {
        return spawnTick != 0 && (endTick == 0 || currentTick < endTick);
    }

    // A slot is free for reuse iff it never spawned, or it has ended and the
    // current tick has reached its end tick.
    bool isFree(uint32_t currentTick) const
    {
        return spawnTick == 0 || (endTick != 0 && currentTick >= endTick);
    }

    // Equality compares ONLY the closed-form launch/end parameters — the
    // transient bodyState is local scratch and excluded by design.
    bool operator==(const ProjectileSlot& o) const
    {
        return spawnTick == o.spawnTick
            && endTick == o.endTick
            && endReason == o.endReason
            && hitObjectIndex == o.hitObjectIndex
            && isSimilarToField(spawnPos, o.spawnPos)
            && isSimilarToField(spawnDir, o.spawnDir);
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class State
{
public:
    // Sized to compile-time CAPACITY; integrate gates the active range by the runtime
    // StaticData::projectilePoolSize. Wire cost is watermark-trimmed to the used slots.
    State() : slots(kMaxProjectilePoolSize) {}

    std::vector<ProjectileSlot> slots;

    bool isSimilarTo(const State& other) const
    {
        if (slots.size() != other.slots.size()) return false;
        for (std::size_t i = 0; i < slots.size(); ++i)
            if (!(slots[i] == other.slots[i])) return false;
        return true;
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class InitialConditions
{
public:
    uint32_t spawnRequestPending = 0;
    glm::vec3 spawnPos{};
    glm::vec3 spawnDir{};   // unit launch direction; velocity is derived in the sim
                            // as spawnDir * sd.projectileSpeed so speed stays tunable
                            // via StaticData without revising captured trajectories.
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// T30 — tick-stamped indicator entry. `tickStamp` is the sim tick the hit/block
// occurred on; integrate prunes the entry once currentTick - tickStamp reaches
// sd.indicatorPersistTicks, and the viz derives its residual draw lifetime from it.
// DerivedState is NOT serialized (local scratch), so this POD never hits the wire.
struct ProjectileIndicator
{
    glm::vec3 position{};
    int       objectIndex = -1;
    uint32_t  tickStamp   = 0;
};

class DerivedState
{
public:
    // Damage hits — body hits, and guard hits OUTSIDE the front block cone.
    // Each entry placed at the impacted object's position (hit.objectPosition).
    std::vector<ProjectileIndicator> hits;
    // T29/T30 — blocked hits: guard hits INSIDE the front block cone. Position is
    // the launch-ray vs inner-circle intersection facing the shooter (T30), not the
    // character root. Persisted across ticks (pruned by tickStamp), wire-free.
    std::vector<ProjectileIndicator> blocks;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PlayerInput
{
public:
    glm::vec3 aimDirection{};
};

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
    // Current simulation tick — drives the closed-form trajectory derivation and
    // the alive/free slot predicates. Mirrors getDeltaTime; plumbed in from
    // SimulationTimeStep at the SimulatableBrawler::integrate call site (T15).
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// template <int Slot> PhysicsDeclaration — one instantiation per pool slot.
// StateType is always brawlerProjectileSimulation::State; bodyStateOf indexes slots[Slot].
template <int Slot>
struct PhysicsDeclaration
{
    static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }

    static constexpr const char* name = []() constexpr -> const char* {
        if constexpr (Slot == 0) return "Projectile0";
        else if constexpr (Slot == 1) return "Projectile1";
        else return "Projectile2";
    }();

    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& sd)
    {
        return PhysicsSetup::queryVolumes(sd);
    }

    static glm::vec3 attachmentOffset(const StaticData&) { return glm::vec3(0.f); }

    using StateType = brawlerProjectileSimulation::State;
    static       PhysicsBodyState& bodyStateOf(      StateType& s) { return s.slots[Slot].bodyState; }
    static const PhysicsBodyState& bodyStateOf(const StateType& s) { return s.slots[Slot].bodyState; }

    RuntimeBindings bindings;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct Dependencies
{
    using Owned = OwnedDeps<
        brawlerProjectileSimulation::InitialConditions,
        brawlerProjectileSimulation::State>;
    using External = ExternalDeps<>;
    using InputType = brawlerProjectileSimulation::PlayerInput;
    Owned owned;
    External external;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{

template <typename PhysicsBodyAdapterType>
void parkBody(PhysicsBodyAdapterType& physics, BodyId bodyId)
{
    glm::mat4 parkTransform(1.f);
    parkTransform[3] = glm::vec4(0.f, 0.f, kParkZ, 1.f);
    physics.setBodyTransform(bodyId, parkTransform);
    physics.setBodyLinearVelocity(bodyId, glm::vec3(0.f));
}

} // anonymous namespace

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void integrate(float dt,
               const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
               const StaticData& sd,
               Dependencies deps,
               const std::array<RuntimeBindings, kMaxProjectilePoolSize>& bindings,
               DerivedState& derived)
{
    InitialConditions& ic = deps.owned.edit<InitialConditions>();
    State& state = deps.owned.edit<State>();
    auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
    auto& queryAdapter = input.getIntegrationUtils().getQueryAdapter();
    const uint32_t currentTick = input.getIntegrationUtils().getCurrentTick();

    // T30 — prune expired indicator entries at the TOP of integrate. This replaces
    // the old "DerivedState reset each tick" assumption: hit/block markers now persist
    // for sd.indicatorPersistTicks ticks so the viz can keep redrawing them (radial-
    // style persistence). Signed-difference compare so a resim cursor landing on an
    // earlier tick than an entry's stamp does not underflow into a spurious prune.
    {
        const int64_t persistTicks = static_cast<int64_t>(sd.indicatorPersistTicks);
        const int64_t nowTick      = static_cast<int64_t>(currentTick);
        auto isExpired = [nowTick, persistTicks](const ProjectileIndicator& e) {
            return nowTick - static_cast<int64_t>(e.tickStamp) >= persistTicks;
        };
        derived.hits.erase(std::remove_if(derived.hits.begin(), derived.hits.end(), isExpired),
                           derived.hits.end());
        derived.blocks.erase(std::remove_if(derived.blocks.begin(), derived.blocks.end(), isExpired),
                             derived.blocks.end());
    }

    // R-P1: the runtime pool size comes from StaticData (single source of truth),
    // clamped to the compile-time capacity that sizes `bindings` and `state.slots`.
    uint32_t activeCount = sd.projectilePoolSize;
    if (activeCount > kMaxProjectilePoolSize)
        activeCount = kMaxProjectilePoolSize;

    // Snap a body to a world position with the derived launch velocity.
    auto snapBody = [&](const RuntimeBindings& b, const glm::vec3& pos, const glm::vec3& vel)
    {
        glm::mat4 t(1.f);
        t[3] = glm::vec4(pos, 1.f);
        physics.setBodyTransform(b.ownBodyId, t);
        physics.setBodyLinearVelocity(b.ownBodyId, vel);
    };

    // Advance each alive slot from its closed-form trajectory BEFORE consuming the
    // spawn request, so a slot spawned this tick is not advanced until next frame.
    for (uint32_t i = 0; i < activeCount; ++i)
    {
        ProjectileSlot& slot = state.slots[i];
        if (!slot.isAlive(currentTick))
            continue;

        // Closed form: pos(t) = spawnPos + spawnDir * speed * dt * (currentTick - spawnTick).
        const uint32_t elapsedTicks   = currentTick - slot.spawnTick;
        const float    elapsedSeconds = static_cast<float>(elapsedTicks) * dt;
        const glm::vec3 velocity   = slot.spawnDir * sd.projectileSpeed;
        const glm::vec3 derivedPos = slot.spawnPos + velocity * elapsedSeconds;

        snapBody(bindings[i], derivedPos, velocity);
        slot.bodyState.position = derivedPos;       // keep transient in sync for viz/capture

        // Lifetime expiry.
        if (elapsedSeconds >= sd.maxLifetime)
        {
            OGBLOG_G("[Projectile.lifetime] slot %u expired (elapsed=%.3f)", i, elapsedSeconds);
            slot.endTick   = currentTick;
            slot.endReason = 1;
            parkBody(physics, bindings[i].ownBodyId);
            continue;
        }

        // Overlap query at the derived position — find first non-parent hit.
        glm::mat4 derivedTransform(1.f);
        derivedTransform[3] = glm::vec4(derivedPos, 1.f);
        for (const auto& volumeId : bindings[i].queryVolumeIds)
            queryAdapter.setVolumeParentTransform(volumeId, derivedTransform);

        SpatialQueryReport report = queryAdapter.overlap(bindings[i].queryVolumeIds);
        for (const auto& hit : report)
        {
            // Parent-body filter runs first — the firing character never blocks or
            // takes damage from its own projectile.
            if (hit.bodyId == bindings[i].parentBodyId)
                continue;

            // T29 — classify guard hits inside the front cone as BLOCKS; everything
            // else (guard outside the cone, or a plain body hit) is a damage hit.
            bool blocked = false;
            glm::vec3 charPos(0.f);   // target character root (guard body translation)
            if (hit.objectCategories.contains(collisionCategory::guard))
            {
                // The guard sim rotates the guard body so its forward axis (the first
                // column of the rotation matrix) points along the target character's
                // aim direction (see DAttackGuardSimulation.h). The guard body sits at
                // the character root, so its translation IS the character position.
                const glm::mat4 guardTransform = physics.getBodyTransform(hit.bodyId);
                charPos = glm::vec3(guardTransform[3]);
                glm::vec3 guardForward = glm::vec3(guardTransform[0]);
                const float fwdLen = glm::length(guardForward);
                if (fwdLen > 0.0001f)
                    guardForward /= fwdLen;

                // Direction the projectile is travelling INTO the guard is -spawnDir
                // (spawnDir is the unit launch/travel direction). A guard facing the
                // incoming projectile (forward ≈ -spawnDir) yields a near-zero angle.
                const glm::vec3 incoming = -slot.spawnDir;
                const float d     = glm::clamp(glm::dot(guardForward, incoming), -1.f, 1.f);
                const float angle = glm::acos(d);
                blocked = (angle <= sd.guardMiddleSectionHalfAngle);
            }

            slot.endTick        = currentTick;
            slot.endReason      = 2;
            slot.hitObjectIndex = hit.objectIndex;
            if (blocked)
            {
                // T30 — place the block marker on the inner circle where the launch ray
                // enters it (the edge facing the shooter), not at the character root.
                // Solve |O + t*D - C|^2 = r^2 in the XY plane (O = spawnPos, D = spawnDir,
                // C = charPos, r = innerCircleRadius). Pick the smaller positive root —
                // the entry point. Fallback to hit.objectPosition if the ray misses.
                glm::vec3 blockPos = hit.objectPosition;
                bool foundIntersection = false;
                const glm::vec2 rayO = glm::vec2(slot.spawnPos.x, slot.spawnPos.y);
                const glm::vec2 rayD = glm::vec2(slot.spawnDir.x, slot.spawnDir.y);
                const glm::vec2 circC = glm::vec2(charPos.x, charPos.y);
                const glm::vec2 fromC = rayO - circC;
                const float aQ = glm::dot(rayD, rayD);
                const float bQ = 2.f * glm::dot(fromC, rayD);
                const float cQ = glm::dot(fromC, fromC) - sd.innerCircleRadius * sd.innerCircleRadius;
                const float disc = bQ * bQ - 4.f * aQ * cQ;
                if (aQ > 1e-8f && disc >= 0.f)
                {
                    const float sq = std::sqrt(disc);
                    float t = (-bQ - sq) / (2.f * aQ);
                    if (t < 0.f)
                        t = (-bQ + sq) / (2.f * aQ);
                    if (t >= 0.f)
                    {
                        // Preserve the projectile's z (constant-z flight by design).
                        blockPos = slot.spawnPos + t * slot.spawnDir;
                        foundIntersection = true;
                    }
                }
                if (!foundIntersection)
                    OGBLOG_G("[Projectile.block] ray missed inner circle, fallback to hit position");

                derived.blocks.push_back({ blockPos, hit.objectIndex, currentTick });
                OGBLOG_G("[Projectile.block] slot %u blocked by objectIndex=%d", i, hit.objectIndex);
            }
            else
            {
                derived.hits.push_back({ hit.objectPosition, hit.objectIndex, currentTick });
                OGBLOG_G("[Projectile.hit] slot %u hit objectIndex=%d", i, hit.objectIndex);
            }
            parkBody(physics, bindings[i].ownBodyId);
            break;
        }
    }

    // Consume spawn request — find lowest free slot.
    // NOTE (R-T5): we only ever write spawnTick on a FREE slot (never spawned or
    // already recycled). An active slot's spawnTick is never revised.
    if (ic.spawnRequestPending != 0)
    {
        bool spawned = false;
        for (uint32_t i = 0; i < activeCount; ++i)
        {
            ProjectileSlot& slot = state.slots[i];
            if (!slot.isFree(currentTick))
                continue;

            slot.spawnTick      = currentTick;
            slot.spawnPos       = ic.spawnPos;
            slot.spawnDir       = ic.spawnDir;
            slot.endTick        = 0;
            slot.endReason      = 0;
            slot.hitObjectIndex = -1;

            // Snap the body to the launch pose so it is correctly placed on the
            // spawn tick (elapsed == 0 ⇒ derivedPos == spawnPos).
            const glm::vec3 velocity = ic.spawnDir * sd.projectileSpeed;
            snapBody(bindings[i], ic.spawnPos, velocity);
            slot.bodyState.position = ic.spawnPos;

            spawned = true;
            break;
        }
        if (!spawned)
        {
            OGBLOG_G("[Projectile.poolFull] dropped spawn request");
        }
        ic.spawnRequestPending = 0;
    }
}

} // namespace brawlerProjectileSimulation

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SerializableFields specializations

// Closed-form wire shape: launch parameters + end markers ONLY. `bodyState` is a
// transient local-only field (recomputed each tick from the closed form) and is
// deliberately EXCLUDED so it never hits the wire and never drives correction.
// Per-slot wire size = 4 (spawnTick) + 12 (spawnPos) + 12 (spawnDir)
//                    + 4 (endTick) + 1 (endReason) + 4 (hitObjectIndex) = 37 bytes.
template <>
struct SerializableFields<brawlerProjectileSimulation::ProjectileSlot>
{
    static constexpr auto get()
    {
        using S = brawlerProjectileSimulation::ProjectileSlot;
        return std::make_tuple(
            SIM_MEMBER(S, spawnTick),
            SIM_MEMBER(S, spawnPos),
            SIM_MEMBER(S, spawnDir),
            SIM_MEMBER(S, endTick),
            SIM_MEMBER(S, endReason),
            SIM_MEMBER(S, hitObjectIndex));
    }
};

template <>
struct SerializableFields<brawlerProjectileSimulation::State>
{
    static constexpr auto get()
    {
        using S = brawlerProjectileSimulation::State;
        return std::make_tuple(
            SIM_VECTOR(S, slots, brawlerProjectileSimulation::kMaxProjectilePoolSize));
    }
};

template <>
struct SerializableFields<brawlerProjectileSimulation::InitialConditions>
{
    static constexpr auto get()
    {
        using IC = brawlerProjectileSimulation::InitialConditions;
        return std::make_tuple(
            SIM_MEMBER(IC, spawnRequestPending),
            SIM_MEMBER(IC, spawnPos),
            SIM_MEMBER(IC, spawnDir));
    }
};

template <>
struct SerializableFields<brawlerProjectileSimulation::PlayerInput>
{
    static constexpr auto get()
    {
        return std::make_tuple(
            MemberFieldDesc<&brawlerProjectileSimulation::PlayerInput::aimDirection>{});
    }
};

static_assert(SimulationState<brawlerProjectileSimulation::State>);
static_assert(SimulationInitialConditions<brawlerProjectileSimulation::InitialConditions>);
static_assert(SimulationInput<brawlerProjectileSimulation::PlayerInput>);

#pragma optimize("", on)

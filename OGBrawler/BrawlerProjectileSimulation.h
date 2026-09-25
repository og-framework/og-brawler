#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <array>
#include <vector>
#include <limits>
#include <cmath>
#include <cstdint>
#include <algorithm>   // std::remove_if (erase-remove prune)
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/geometric.hpp"      // dot
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/PhysicsBodyAdapter.h"
#include "OGSimulation/PhysicsDeclaration.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGSimulation/BodyId.h"
#include "OGBrawler/CollisionCategoryConstants.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

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
    // Matches the 'shieldAngle' middle-section threshold in dAttackRadialSimulation::wouldGuardBlock.
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
                // [hit-resolution T13] Projectile body has its own collision category
                // so a projectile-vs-projectile overlap can be distinguished from a
                // projectile-vs-character-body overlap. Before T13 this was
                // `collisionCategory::body`, causing the projectile-projectile case
                // to fall through the character-body branch and route a HitFlinch
                // to each owning character via T3/T11 rootBodyId lookup.
                CollisionCategories::single(collisionCategory::projectile)
            }
        }
    };

    static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& staticData)
    {
        return {
            QueryVolumeDescriptor{
                SphereGeometry{staticData.colliderRadius},
                // [hit-resolution T13] Include the projectile category so projectiles
                // still detect each other (both cancel with endReason=3: since
                // og-netcode-v2-field-defects task 17 the pair is decided by
                // brawlerHitDetection's projectile pass, BrawlerProjectileHitDetection.h).
                // Drop `projectile` from this mask if pass-through semantics are
                // preferred over mutual cancellation.
                collisionCategory::bodyGuardProjectile,
                glm::mat4(1.f),
                collisionCategory::queryRouting
            }
        };
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// The one shared definition lives in OGSimulation/PhysicsDeclaration.h. The
// PhysicsDeclaration concept requires `same_as<PhysicsRuntimeBindings&>`, so a
// field-identical per-sim copy is a DISTINCT type and does not conform; this
// alias keeps every existing `brawlerProjectileSimulation::RuntimeBindings`
// spelling valid while making the type the shared one.
using RuntimeBindings = PhysicsRuntimeBindings;

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
    uint8_t   endReason      = 0;   // 0=alive, 1=lifetimeExpired, 2=hit, 3=cancelledByProjectile [T13], 4=blockedByGuard [T14]
    // [og-netcode-v2-field-defects task 17] `hitRootBodyId` (4 B) LEFT THE WIRE here. Its one
    // reader was hit routing's branch 3, which now takes the struck character from
    // DerivedState::detectedThisTick in the same pass that detected it.

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
    BodyId    rootBodyId;        // actor-level id of the struck character (viz-only tag)
    uint32_t  tickStamp   = 0;
};

// [og-netcode-v2-field-defects task 17] What brawlerHitDetection::System's projectile pass
// (BrawlerProjectileHitDetection.h) found for one slot in THIS step's pre-integrate pass. The
// values are the endReason codes the slot is ended with, so integrate stores them unchanged.
enum class SlotOutcome : uint8_t
{
    None                  = 0,
    Hit                   = 2,
    CancelledByProjectile = 3,
    BlockedByGuard        = 4,
};
static_assert(static_cast<uint8_t>(SlotOutcome::Hit) == 2
           && static_cast<uint8_t>(SlotOutcome::CancelledByProjectile) == 3
           && static_cast<uint8_t>(SlotOutcome::BlockedByGuard) == 4,
    "brawlerProjectileSimulation::SlotOutcome - integrate writes the outcome into ProjectileSlot::endReason "
    "as is, and endReason's codes are 2 hit, 3 cancelledByProjectile, 4 blockedByGuard (1 is lifetime "
    "expiry, which integrate decides itself). Was the per-slot branch inside integrate that wrote the codes.");

struct SlotDetection
{
    SlotOutcome outcome = SlotOutcome::None;
    BodyId      struckRootBodyId;       // Hit / BlockedByGuard: the struck character's root body id
    glm::vec3   objectPosition{};       // Hit / BlockedByGuard: the struck shape's position
    glm::vec3   targetRootPosition{};   // BlockedByGuard: the guard body's translation (the character root)
};

class DerivedState
{
public:
    // Damage hits — body hits, and guard hits OUTSIDE the front block cone.
    // Each entry placed at the impacted object's position (SlotDetection::objectPosition).
    std::vector<ProjectileIndicator> hits;
    // T29/T30 — blocked hits: guard hits INSIDE the front block cone. Position is
    // the launch-ray vs inner-circle intersection facing the shooter (T30), not the
    // character root. Persisted across ticks (pruned by tickStamp), wire-free.
    std::vector<ProjectileIndicator> blocks;
    // [og-netcode-v2-field-defects task 17] One entry per pool slot, indexed like
    // State::slots. Written ONLY by brawlerHitDetection's projectile pass, which resets
    // every entry at the top of every pass; read in the same step by hit routing
    // (branches 3 and 4) and by integrate below, which ends the slot. Never on the wire:
    // a resim replay recomputes it from the restored slot.
    std::array<SlotDetection, kMaxProjectilePoolSize> detectedThisTick{};
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PlayerInput
{
public:
    glm::vec3 aimDirection{};

    // THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
    // SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
    // [movement-sim task 22] The value is copied VERBATIM from what that function
    // handed this type before the fold; it is a wire value, not something to re-derive.
    // ⚠ Unlike radial/machine/guard, this one IS PlayerInput{} today — the pre-fold
    // getZeroPlayerInput() passed a value-initialised projectile input, so a (0,0,0)
    // aim is the shipped wire value. Do not "fix" it to (0,0,1): that is a wire change.
    static PlayerInput zero() { return PlayerInput{}; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// [og-netcode-v2-field-defects task 17] No query adapter: detection left this sub-simulation
// for brawlerHitDetection::System, so integrate cannot query even by accident.
template <typename PhysicsBodyAdapterType>
class IntegrationUtils
{
public:
    IntegrationUtils(float deltaTime,
                     uint32_t currentTick,
                     PhysicsBodyAdapterType& physicsBodyAdapter)
        : m_deltaTime(deltaTime)
        , m_currentTick(currentTick)
        , m_physicsBodyAdapter(physicsBodyAdapter)
    {}

    float getDeltaTime() const { return m_deltaTime; }
    // Current simulation tick — drives the closed-form trajectory derivation and
    // the alive/free slot predicates. Mirrors getDeltaTime; plumbed in from
    // SimulationTimeStep at the SimulatableBrawler::integrate call site (T15).
    uint32_t getCurrentTick() const { return m_currentTick; }
    PhysicsBodyAdapterType& getPhysicsAdapter() const { return m_physicsBodyAdapter; }

private:
    float m_deltaTime;
    uint32_t m_currentTick;
    PhysicsBodyAdapterType& m_physicsBodyAdapter;
};

template <typename PhysicsBodyAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsBodyAdapterType>>;

// [og-netcode-v2-field-defects task 17] The closed-form position of a slot on `tick`,
// shared by integrate (which snaps the body there) and the detector (which queries there),
// so the two can never sample different points. For tick < slot.spawnTick the unsigned
// difference WRAPS to an elapsed time far above maxLifetime; nothing clamps it here.
// The detector never passes such a tick: it skips those slots explicitly and goes through
// elapsedSecondsFromSpawn (BrawlerProjectileHitDetection.h), so its gate is not this wrap.
// integrate CAN pass one (read, not measured): a resim NoSlot character keeps its
// un-restored frontier state, and its first replayed integrate may precede a slot's spawn
// tick. integrate then ends that slot as a lifetime expiry, as it did before task 17.
inline float elapsedSecondsAt(const ProjectileSlot& slot, float dt, uint32_t tick)
{
    const uint32_t elapsedTicks = tick - slot.spawnTick;
    return static_cast<float>(elapsedTicks) * dt;
}

inline glm::vec3 closedFormPosition(const ProjectileSlot& slot, const StaticData& sd, float elapsedSeconds)
{
    const glm::vec3 velocity = slot.spawnDir * sd.projectileSpeed;
    return slot.spawnPos + velocity * elapsedSeconds;
}

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

    // Maps the GAME's aggregate static data to this sub-simulation's own slice.
    // This is what makes body creation generic: the engine-side fold asks each
    // declaration for its slice instead of branching on the declaration type.
    // A member TEMPLATE deliberately — this header cannot name
    // simulatableBrawler::StaticData, because the aggregate includes this header
    // (an include cycle). GameStaticDataType is deduced at the call site, where the aggregate is
    // complete. All three pool slots share the one projectile slice.
    template <typename GameStaticDataType>
    static const StaticData& staticDataOf(const GameStaticDataType& gsd) { return gsd.m_projectileStaticData; }

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

template <typename PhysicsBodyAdapterType>
void integrate(float dt,
               const AllInput<PhysicsBodyAdapterType>& input,
               const StaticData& sd,
               Dependencies deps,
               const std::array<RuntimeBindings, kMaxProjectilePoolSize>& bindings,
               DerivedState& derived)
{
    InitialConditions& ic = deps.owned.edit<InitialConditions>();
    State& state = deps.owned.edit<State>();
    auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
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
        const float    elapsedSeconds = elapsedSecondsAt(slot, dt, currentTick);
        const glm::vec3 velocity   = slot.spawnDir * sd.projectileSpeed;
        const glm::vec3 derivedPos = closedFormPosition(slot, sd, elapsedSeconds);

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

        // [og-netcode-v2-field-defects task 17] DETECTION IS NOT HERE ANY MORE. The overlap,
        // the projectile-vs-projectile cancel and the guard classification run in
        // brawlerHitDetection::System's pre-integrate pass of THIS step
        // (BrawlerProjectileHitDetection.h), at this same closed-form position
        // (closedFormPosition above), over the state every character's previous integrate
        // left; so no peer's outcome depends on which character integrated first. This
        // integrate stays the ONLY writer of the wire slot: it reads the pass's outcome and
        // ends the slot on this, the contact, tick. derivedPos above IS the contact position,
        // not a step past it, and the viz draws no slot whose endTick is set.
        const SlotDetection& detected = derived.detectedThisTick[i];
        if (detected.outcome == SlotOutcome::None)
            continue;

        // [hit-resolution T13] Projectile-vs-projectile cancellation: ended WITHOUT a
        // routable hit (routing branch 3 routes Hit only), so neither owning character
        // enters HitFlinch. The pass cancels BOTH slots of an overlapping pair on the same
        // tick, whichever character integrates first. parkBody + endTick=currentTick
        // despawn the projectile the same way a regular hit does; the viz's
        // spawnTick/endTick guard skips rendering. The slot recycles naturally on the next
        // tick per isFree(currentTick) (endTick != 0 && currentTick >= endTick).
        if (detected.outcome == SlotOutcome::CancelledByProjectile)
        {
            slot.endTick   = currentTick;
            slot.endReason = static_cast<uint8_t>(detected.outcome);
            OGBLOG_G("[Projectile.cancel] slot %u cancelled by opposing projectile", i);
            parkBody(physics, bindings[i].ownBodyId);
            continue;
        }

        {
            const bool blocked = detected.outcome == SlotOutcome::BlockedByGuard;
            // T29 — the pass classified guard hits inside the front cone as BLOCKS;
            // everything else (guard outside the cone, or a plain body hit) is a hit.
            const glm::vec3 charPos = detected.targetRootPosition;   // target character root

            slot.endTick        = currentTick;
            // [hit-resolution T14] Distinguish blocked vs unblocked at the endReason
            // level so T3 routing (which routes Hit only) does NOT fire HitFlinch on a
            // target whose guard successfully absorbed the projectile.
            slot.endReason      = static_cast<uint8_t>(detected.outcome);   // 4 blockedByGuard / 2 hit
            if (blocked)
            {
                // T30 — place the block marker on the inner circle where the launch ray
                // enters it (the edge facing the shooter), not at the character root.
                // Solve |O + t*D - C|^2 = r^2 in the XY plane (O = spawnPos, D = spawnDir,
                // C = charPos, r = innerCircleRadius). Pick the smaller positive root —
                // the entry point. Fallback to the struck shape's position if the ray misses.
                glm::vec3 blockPos = detected.objectPosition;
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

                derived.blocks.push_back({ blockPos, detected.struckRootBodyId, currentTick });
                OGBLOG_G("[Projectile.block] slot %u blocked by rootBodyId=%u", i, detected.struckRootBodyId.value);
            }
            else
            {
                derived.hits.push_back({ detected.objectPosition, detected.struckRootBodyId, currentTick });
                OGBLOG_G("[Projectile.hit] slot %u hit rootBodyId=%u", i, detected.struckRootBodyId.value);
            }
            parkBody(physics, bindings[i].ownBodyId);
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
//                    + 4 (endTick) + 1 (endReason) = 33 bytes.
// [og-netcode-v2-field-defects task 17] 37 -> 33: hitRootBodyId (4 B) left the wire; the
// struck character now travels in DerivedState::detectedThisTick, inside one step.
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
            SIM_MEMBER(S, endReason));
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

OGSIM_OPTIMIZE_ON

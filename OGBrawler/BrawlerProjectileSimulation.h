#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <array>
#include <vector>
#include <limits>
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
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

static constexpr uint32_t kProjectilePoolSize = 3;

// Off-world parking position — far below the playfield so parked bodies never interact.
static constexpr float kParkZ = -100000.f;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StaticData
{
public:
    StaticData(float projectileSpeed,
               float maxLifetime,
               float colliderRadius,
               float spawnForwardOffset,
               float spawnZOffset)
        : projectileSpeed(projectileSpeed)
        , maxLifetime(maxLifetime)
        , colliderRadius(colliderRadius)
        , spawnForwardOffset(spawnForwardOffset)
        , spawnZOffset(spawnZOffset)
    {}

    StaticData(const StaticData&) = default;

    float projectileSpeed;
    float maxLifetime;
    float colliderRadius;
    float spawnForwardOffset;
    float spawnZOffset;
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

struct ProjectileSlot
{
    uint32_t isAlive = 0;
    float lifetime = 0.f;
    int hitObjectIndex = -1;
    PhysicsBodyState bodyState;

    bool operator==(const ProjectileSlot& o) const
    {
        return isAlive == o.isAlive
            && hitObjectIndex == o.hitObjectIndex
            && isSimilarToField(lifetime, o.lifetime)
            && fieldwiseIsSimilarTo(bodyState, o.bodyState);
    }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class State
{
public:
    State() : slots(kProjectilePoolSize) {}

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
    glm::vec3 velocity{};
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class DerivedState
{
public:
    std::vector<dAttackRadialSimulation::DAttackHit> hits;
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
               const std::array<RuntimeBindings, kProjectilePoolSize>& bindings,
               DerivedState& derived)
{
    InitialConditions& ic = deps.owned.edit<InitialConditions>();
    State& state = deps.owned.edit<State>();
    auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
    auto& queryAdapter = input.getIntegrationUtils().getQueryAdapter();

    // Tick each alive slot before consuming the spawn request so that a slot
    // spawned this tick starts with lifetime=0 and is not advanced until next frame.
    for (uint32_t i = 0; i < kProjectilePoolSize; ++i)
    {
        ProjectileSlot& slot = state.slots[i];
        if (slot.isAlive == 0)
            continue;

        slot.lifetime += dt;

        // Lifetime expiry.
        if (slot.lifetime >= sd.maxLifetime)
        {
            OGBLOG_G("[Projectile.lifetime] slot %u expired (lifetime=%.3f)", i, slot.lifetime);
            slot.isAlive = 0;
            parkBody(physics, bindings[i].ownBodyId);
            continue;
        }

        // Overlap query — find first non-parent hit.
        const glm::mat4 bodyTransform = physics.getBodyTransform(bindings[i].ownBodyId);
        for (const auto& volumeId : bindings[i].queryVolumeIds)
            queryAdapter.setVolumeParentTransform(volumeId, bodyTransform);

        SpatialQueryReport report = queryAdapter.overlap(bindings[i].queryVolumeIds);
        for (const auto& hit : report)
        {
            if (hit.bodyId == bindings[i].parentBodyId)
                continue;

            slot.isAlive = 0;
            slot.hitObjectIndex = hit.objectIndex;
            derived.hits.push_back({ hit.objectPosition, hit.objectIndex });
            parkBody(physics, bindings[i].ownBodyId);
            OGBLOG_G("[Projectile.hit] slot %u hit objectIndex=%d", i, hit.objectIndex);
            break;
        }
    }

    // Consume spawn request — find lowest free slot.
    if (ic.spawnRequestPending != 0)
    {
        bool spawned = false;
        for (uint32_t i = 0; i < kProjectilePoolSize; ++i)
        {
            if (state.slots[i].isAlive == 0)
            {
                glm::mat4 spawnTransform(1.f);
                spawnTransform[3] = glm::vec4(ic.spawnPos, 1.f);
                physics.setBodyTransform(bindings[i].ownBodyId, spawnTransform);
                physics.setBodyLinearVelocity(bindings[i].ownBodyId, ic.velocity);
                state.slots[i].isAlive = 1;
                state.slots[i].lifetime = 0.f;
                state.slots[i].hitObjectIndex = -1;
                spawned = true;
                break;
            }
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

template <>
struct SerializableFields<brawlerProjectileSimulation::ProjectileSlot>
{
    static constexpr auto get()
    {
        using S = brawlerProjectileSimulation::ProjectileSlot;
        return std::make_tuple(
            SIM_MEMBER(S, isAlive),
            SIM_MEMBER(S, lifetime),
            SIM_MEMBER(S, hitObjectIndex),
            SIM_MEMBER(S, bodyState));
    }
};

template <>
struct SerializableFields<brawlerProjectileSimulation::State>
{
    static constexpr auto get()
    {
        using S = brawlerProjectileSimulation::State;
        return std::make_tuple(
            SIM_VECTOR(S, slots, brawlerProjectileSimulation::kProjectilePoolSize));
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
            SIM_MEMBER(IC, velocity));
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

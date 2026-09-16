#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerRingoutSimulation-rationale.md · docs/BrawlerRingoutSimulation-guards.md

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include "glm/vec3.hpp"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGBrawler/BrawlerMovementSimulation.h"
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerRingout
{

inline constexpr uint32_t kMaxSpawnPoints = 4u;

class StaticData
{
public:
    StaticData(float killPlaneZ = -500.f,
               uint32_t respawnDelayTicks = 120u,
               std::array<glm::vec3, kMaxSpawnPoints> spawnPoints = {
                   glm::vec3(-200.f, -200.f, 200.f),
                   glm::vec3( 200.f, -200.f, 200.f),
                   glm::vec3(-200.f,  200.f, 200.f),
                   glm::vec3( 200.f,  200.f, 200.f)
               })
        : killPlaneZ(killPlaneZ)
        , respawnDelayTicks(respawnDelayTicks)
        , spawnPoints(spawnPoints)
    {}

    StaticData(const StaticData&) = default;

    float killPlaneZ;

    uint32_t respawnDelayTicks;

    std::array<glm::vec3, kMaxSpawnPoints> spawnPoints;
};

struct LevelSpawnPoint
{
    std::string name;
    glm::vec3   position{0.f, 0.f, 0.f};
};

inline uint32_t spawnOrderBits(float v)
{
    uint32_t bits = 0u;
    std::memcpy(&bits, &v, sizeof(bits));
    return bits;
}

inline bool levelSpawnPointOrderBefore(const LevelSpawnPoint& a, const LevelSpawnPoint& b)
{
    if (a.name != b.name)
        return a.name < b.name;
    if (spawnOrderBits(a.position.x) != spawnOrderBits(b.position.x))
        return spawnOrderBits(a.position.x) < spawnOrderBits(b.position.x);
    if (spawnOrderBits(a.position.y) != spawnOrderBits(b.position.y))
        return spawnOrderBits(a.position.y) < spawnOrderBits(b.position.y);
    return spawnOrderBits(a.position.z) < spawnOrderBits(b.position.z);
}

inline std::array<glm::vec3, kMaxSpawnPoints> spawnPointsFromLevelPlacements(
    std::vector<LevelSpawnPoint> placements,
    const std::array<glm::vec3, kMaxSpawnPoints>& authoredFallback)
{
    std::array<glm::vec3, kMaxSpawnPoints> merged = authoredFallback;

    std::sort(placements.begin(), placements.end(), levelSpawnPointOrderBefore);

    const size_t fillCount = placements.size() < static_cast<size_t>(kMaxSpawnPoints)
        ? placements.size()
        : static_cast<size_t>(kMaxSpawnPoints);
    for (size_t i = 0u; i < fillCount; ++i)
        merged[i] = placements[i].position;

    return merged;
}

class InitialConditions
{
public:
    uint32_t spawnSlot = 0u;
};

class State
{
public:
    uint8_t flags = 0u;

    uint32_t respawnAtTick = 0u;
};

inline constexpr uint8_t kFlagDead = 1u << 0;

static_assert(kFlagDead == (1u << 0),
    "brawlerRingout::State::flags - the bit assignment is ON THE WIRE and in the checksum. "
    "Moving it desynchronises every peer that has not shipped the same edit. Bits 1-7 are "
    "free and are where a future ring-out signal goes.");

inline bool isDead(const State& s) { return (s.flags & kFlagDead) != 0u; }

class DerivedState
{
public:
    bool diedThisTick = false;
};

class PlayerInput
{
public:
    static PlayerInput zero() { return PlayerInput{}; }
};

class IntegrationUtils
{
public:
    explicit IntegrationUtils(uint32_t currentTick)
        : m_currentTick(currentTick)
    {}

    uint32_t getCurrentTick() const { return m_currentTick; }

private:
    uint32_t m_currentTick;
};

template <typename T>
concept RingoutStepExposesDeltaTime = requires(const T& t) { t.getDeltaTime(); };

// ⛔G-02  docs/BrawlerRingoutSimulation-guards.md
static_assert(!RingoutStepExposesDeltaTime<IntegrationUtils>,
    "brawlerRingout::IntegrationUtils must NOT expose a delta time. The respawn countdown is "
    "an ABSOLUTE tick (State::respawnAtTick) compared with tick >= respawnAtTick, because a "
    "seconds accumulator lands on a different tick after a resim replays the same interval "
    "with a different number of steps. A sub-simulation that is never handed a delta cannot "
    "write one. Was the prose fence 'THERE IS NO getDeltaTime(), and its absence is the "
    "enforcement of ticks-never-float-seconds' - an absence enforces nothing, which is why "
    "this line exists. See guard G-02.");
static_assert(sizeof(IntegrationUtils) == sizeof(uint32_t),
    "brawlerRingout::IntegrationUtils carries the current tick and NOTHING ELSE. The assertion "
    "above only sees an accessor spelled getDeltaTime(); this one sees any added STORAGE "
    "whatever the accessor is called, and a delta that actually varies has to be stored. "
    "Measured: a private float member with no accessor at all fires this line. These two lines "
    "are ONE fence and G-02 tags both; deleting either without the other leaves half of it. "
    "See guard G-02.");

struct DeltaTimeConceptProbe { float getDeltaTime() const { return 0.f; } };

static_assert(RingoutStepExposesDeltaTime<DeltaTimeConceptProbe>,
    "VACUITY CONTROL for the two assertions above: the concept must be TRUE of something, or "
    "the first of them passes by being false of everything. Every other IntegrationUtils in "
    "this directory carries getDeltaTime() and ring-out's is the one that must not, but they "
    "are class TEMPLATES on the two adapters this sub-simulation is deliberately not handed, "
    "so none of them can be named here without an instantiation. Hence the probe type.");

using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils>;

struct Dependencies
{
    using Owned = OwnedDeps<
        brawlerRingout::InitialConditions,
        brawlerRingout::State>;
    using External = ExternalDeps<
        const brawlerMovementSimulation::State&,
        brawlerMovementSimulation::InitialConditions&>;
    using InputType = brawlerRingout::PlayerInput;
    Owned owned;
    External external;
};

inline void integrate(const AllInput& input,
                      const StaticData& sd,
                      Dependencies deps,
                      DerivedState& derived)
{
    const InitialConditions& ic = deps.owned.get<InitialConditions>();
    State& state = deps.owned.edit<State>();
    const brawlerMovementSimulation::State& movementState =
        deps.external.get<brawlerMovementSimulation::State>();
    brawlerMovementSimulation::InitialConditions& movementIc =
        deps.external.edit<brawlerMovementSimulation::InitialConditions>();

    const uint32_t tick = input.getIntegrationUtils().getCurrentTick();

    derived.diedThisTick = false;

    if (isDead(state) && tick >= state.respawnAtTick)
    {
        state.flags = static_cast<uint8_t>(state.flags & ~kFlagDead);

        if (ic.spawnSlot < kMaxSpawnPoints)
        {
            movementIc.teleportPending = 1u;
            movementIc.teleportPos     = sd.spawnPoints[ic.spawnSlot];

            OGBLOG_G("[Ringout.respawn] tick=%u slot=%u to=(%.2f, %.2f, %.2f)",
                tick, ic.spawnSlot,
                movementIc.teleportPos.x, movementIc.teleportPos.y, movementIc.teleportPos.z);
        }
        else
        {
            OGBLOG_G("[Warning][Ringout.spawnSlot] tick=%u slot=%u >= %u - respawn cleared the "
                     "dead bit but wrote NO teleport seed",
                tick, ic.spawnSlot, kMaxSpawnPoints);
        }
    }
    // ∴D-01  docs/BrawlerRingoutSimulation-rationale.md
    else if (!isDead(state)
             && movementIc.teleportPending == 0u
             && movementState.bodyState.position.z < sd.killPlaneZ)
    {
        state.flags         = static_cast<uint8_t>(state.flags | kFlagDead);
        state.respawnAtTick = tick + sd.respawnDelayTicks;
        derived.diedThisTick = true;

        OGBLOG_G("[Ringout.death] tick=%u z=%.2f killPlaneZ=%.2f respawnAtTick=%u",
            tick, movementState.bodyState.position.z, sd.killPlaneZ, state.respawnAtTick);
    }
}

class SpawnSlotAllocator
{
public:
    static constexpr uint32_t kNoFreeSlot = kMaxSpawnPoints;

    static_assert(kNoFreeSlot == kMaxSpawnPoints,
        "brawlerRingout::SpawnSlotAllocator::kNoFreeSlot is ONE value meaning both 'this "
        "character holds no slot' and 'there was no slot to give', and its VALUE is "
        "kMaxSpawnPoints on purpose: written straight into InitialConditions::spawnSlot it "
        "lands on integrate's out-of-range branch, which logs [Warning][Ringout.spawnSlot] "
        "and writes no teleport seed, instead of needing a second guard at the UE call site. "
        "Giving it a value of its own makes an over-capacity join respawn to slot 0, on top "
        "of whoever legitimately holds it. Was the prose fence at this declaration. "
        "BrawlerRingoutSimulationTest.cpp carries the same identity as a STATIC_REQUIRE, so "
        "deleting this line alone still leaves the property checked - which is why this line "
        "carries no guard id, where the two checks that nothing else asserts (G-01 and G-02) "
        "do.");

    uint32_t acquire(uint32_t characterId)
    {
        const uint32_t existing = slotOf(characterId);
        if (existing != kNoFreeSlot)
            return existing;

        for (uint32_t slot = 0u; slot < kMaxSpawnPoints; ++slot)
        {
            if (!m_slots[slot].held)
            {
                m_slots[slot].held        = true;
                m_slots[slot].characterId = characterId;
                return slot;
            }
        }
        return kNoFreeSlot;
    }

    void release(uint32_t characterId)
    {
        for (auto& entry : m_slots)
        {
            if (entry.held && entry.characterId == characterId)
            {
                entry.held        = false;
                entry.characterId = 0u;
                return;
            }
        }
    }

    uint32_t slotOf(uint32_t characterId) const
    {
        for (uint32_t slot = 0u; slot < kMaxSpawnPoints; ++slot)
        {
            if (m_slots[slot].held && m_slots[slot].characterId == characterId)
                return slot;
        }
        return kNoFreeSlot;
    }

private:
    struct SlotEntry
    {
        bool     held        = false;
        uint32_t characterId = 0u;
    };

    std::array<SlotEntry, kMaxSpawnPoints> m_slots{};
};

} // namespace brawlerRingout

template <>
struct SerializableFields<brawlerRingout::InitialConditions>
{
    static constexpr auto get()
    {
        using IC = brawlerRingout::InitialConditions;
        return std::make_tuple(
            SIM_MEMBER(IC, spawnSlot));
    }
};

template <>
struct SerializableFields<brawlerRingout::State>
{
    static constexpr auto get()
    {
        using S = brawlerRingout::State;
        return std::make_tuple(
            SIM_MEMBER(S, flags),
            SIM_MEMBER(S, respawnAtTick));
    }
};

template <>
struct SerializableFields<brawlerRingout::PlayerInput>
{
    static constexpr auto get()
    {
        return std::make_tuple();
    }
};

static_assert(!Serializable<brawlerRingout::DerivedState>,
    "brawlerRingout::DerivedState is OFF THE WIRE and must stay there. diedThisTick is a "
    "single-tick EDGE recomputed from scratch at the top of every step including every "
    "replayed step of a resim; an edge that can ARRIVE on a correction is an edge the "
    "authority-side score system can count twice. Was the prose fence 'DerivedState has NO "
    "specialization, deliberately - the absence of a specialization is what enforces that "
    "rather than a comment'. An absence enforces nothing; this line does. NOTE: it fires on "
    "a SerializableFields specialization, not on a member added to the type and left "
    "unregistered - which costs no wire bytes and is correctly silent. "
    "BrawlerRingoutSimulationTest.cpp carries the same assertion as a STATIC_REQUIRE_FALSE, "
    "so deleting this line alone still leaves the property checked.");

// ⛔G-01  docs/BrawlerRingoutSimulation-guards.md
static_assert(!Serializable<brawlerRingout::SpawnSlotAllocator>,
    "brawlerRingout::SpawnSlotAllocator is authority-side bookkeeping that lives BESIDE the "
    "simulation, the same shape the score system uses and for the same reason: a rollback "
    "must not be able to reach it. The slot it produces rides the wire; the table that "
    "produced it does not, and simulatableBrawler::State measures 335 B either way. Was the "
    "prose fence 'THIS IS NOT A SUB-SIMULATION TYPE ... it has no SerializableFields "
    "specialization, and it costs ZERO wire bytes'. See guard G-01.");

static_assert(std::is_same_v<
        decltype(SerializableFields<brawlerRingout::State>::get()),
        std::tuple<
            MemberFieldDesc<&brawlerRingout::State::flags>,
            MemberFieldDesc<&brawlerRingout::State::respawnAtTick>>>,
    "brawlerRingout::State - APPEND ONLY. If you appended a field, append it here too, and "
    "bump correctionStateBuffer::kWireFormatVersion if anything before it moved.");

static_assert(std::is_same_v<
        decltype(SerializableFields<brawlerRingout::InitialConditions>::get()),
        std::tuple<
            MemberFieldDesc<&brawlerRingout::InitialConditions::spawnSlot>>>,
    "brawlerRingout::InitialConditions - APPEND ONLY, same rule as State above. The spawn slot "
    "rides the wire so that the authority's assignment is the one every peer respawns to.");

static_assert(SimulationState<brawlerRingout::State>);
static_assert(SimulationInitialConditions<brawlerRingout::InitialConditions>);
static_assert(SimulationInput<brawlerRingout::PlayerInput>);

static_assert(ValidDependencies<brawlerRingout::Dependencies>);

static_assert(syncSize<brawlerRingout::InitialConditions>() == 4u,
    "brawlerRingout::InitialConditions is one uint32_t spawnSlot = 4 B.");
static_assert(syncSize<brawlerRingout::State>() == 5u,
    "brawlerRingout::State is one uint8_t flags + one uint32_t respawnAtTick = 5 B.");
static_assert(syncSize<brawlerRingout::PlayerInput>() == 0u,
    "brawlerRingout::PlayerInput carries NO per-tick signal and must cost nothing. An input "
    "byte is multiplied across every entry of every relayed input ring: measured at the "
    "pre-diet character cap of 4, ONE byte here closes 10.264 B of the join-alone margin "
    "and 10.764 B of the 27.352 B of slack above the half-entry floor - about ten times "
    "what a STATE byte costs. Was the ten-line ring-out prose block at the "
    "makeSimPlayerInput call in BrawlerInputPackaging.h, deleted by ringout task 11 "
    "because this line already forbade the edit that block described. The arithmetic is "
    "RoundVsPacketBudgetTest.cpp's pre-diet table.");

OGSIM_OPTIMIZE_ON

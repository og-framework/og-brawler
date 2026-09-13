#pragma once
// SPDX-License-Identifier: BUSL-1.1

// brawlerRingout — death by kill plane, and respawn after a fixed tick countdown.
//
// THE SHAPE IS BORROWED, NOT INVENTED. This file is modelled on
// `BrawlerProjectileSimulation.h`: StaticData / InitialConditions / State / DerivedState /
// PlayerInput / Dependencies / integrate, then the `SerializableFields` specializations and
// the three role `static_assert`s at the bottom. Everything that differs from that template
// is called out where it differs.
//
// ⛔ THIS SUB-SIM OWNS NO PHYSICS BODY. There is no `PhysicsSetup`, no `PhysicsDeclaration`
// and no `RuntimeBindings`, and that is deliberate: ring-out READS the movement sub-sim's
// solved body position and WRITES the movement sub-sim's teleport seed. It never touches the
// physics adapter, so it is handed neither adapter and cannot.

#include <algorithm>
#include <array>
#include <cstdint>
// [ringout task 9] `spawnPointsFromLevelPlacements` below: <string>/<vector> for the level
// placement list, <cstring> for the bit-pattern tie-break, <algorithm> for the sort that
// destroys the engine's arrival order.
#include <cstring>
#include <string>
#include <vector>
#include "glm/vec3.hpp"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
// The movement sub-simulation, for `State::bodyState.position` (read) and
// `InitialConditions::teleportPending` / `teleportPos` (written). INCLUDED, NEVER EDITED —
// the live `brawler-movement-simulation` initiative holds that header exclusively.
#include "OGBrawler/BrawlerMovementSimulation.h"
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerRingout
{

// Compile-time CAPACITY of the authored spawn table, and the reason it is 4:
// `ASimulationManagerUImpl::kPreDietCharacterCap == 4` is the largest character count whose
// join-alone round still fits one packet (`RoundVsPacketBudgetTest.cpp`,
// `largestFittingCharacterCountUnderJoin()` — the runtime fence MIRRORS that derivation, it
// does not define it). A table with fewer entries than the cap would hand two characters the
// same spawn point at a legal player count; more would be entries nothing can ever index.
//
// ⚠ THIS IS THE THIRD MIRROR OF THAT 4, and the other two are in files this header cannot
// reach: `SimulationManagerUImpl.h` is UE-side and og-brawler is engine-free, and the budget
// test is a test. When the wire diet deletes `kPreDietCharacterCap` (item 40), this constant
// does NOT die with it — it becomes sized by the supported player count directly, and the
// paragraph above is what has to be rewritten rather than the number silently kept.
inline constexpr uint32_t kMaxSpawnPoints = 4u;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class StaticData
{
public:
    // Every constant is DEFAULTED to its authored value, exactly as
    // `brawlerProjectileSimulation::StaticData::projectilePoolSize` is: this is the one place
    // each literal is declared, so a default-constructed StaticData — which is what the LLT
    // rigs and `simulatableBrawler::StaticData`'s member both get — is the shipped object
    // rather than a second set of numbers to keep in step.
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

    // Ruling 6: the kill plane is a SINGLE authored world-Z plane. Strictly below it you are
    // dead. Not a volume, not a per-level shape — one float, compared once per character per
    // tick.
    float killPlaneZ;

    // ⛔ TICKS, NEVER FLOAT SECONDS. The sim is a fixed 60 Hz step
    // (`AsyncFixedTimeStepSize = 0.016667`), and the countdown is stored as the ABSOLUTE tick
    // to respawn at (`State::respawnAtTick`), not as a remaining duration. A seconds
    // accumulator would land on a different tick after a resim replayed the same interval
    // with a different number of steps; `tick >= respawnAtTick` cannot.
    // 120 ticks == 2.0 s at 60 Hz.
    uint32_t respawnDelayTicks;

    // Ruling 7: an AUTHORED spawn table with a deterministic per-character index. There is no
    // penalty for dying — a respawn goes to this character's own slot, every time.
    //
    // ⛔ THE INDEX IS `InitialConditions::spawnSlot`, WHICH IS NOT A UE PLAYER SLOT.
    // `UEConnectionHandle::GetPlayerSlotForActor` is PER-WIRE: it returns 0 for the primary
    // pawn of EVERY remote client and 1..N only for couch-co-op siblings on one machine, so
    // indexing this table by it would put two remote players on the same point. The AUTHORITY
    // assigns the slot and it rides the wire (see `InitialConditions` below, and task 3).
    //
    // The positions are placeholder authoring, sized to the level this mode ships on. They
    // are tuning values, not derived ones — change them here and nowhere else.
    std::array<glm::vec3, kMaxSpawnPoints> spawnPoints;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// ⭐⭐ [ringout task 9, 2026-09-13] WHERE THE SPAWN POINTS ACTUALLY COME FROM: THE LEVEL.
//
// USER RULING, 2026-09-13. The authored table above is placeholder authoring at
// (±200, ±200, Z=200) and the platform this mode ships on is somewhere else entirely, so a
// respawn dropped fighters off the map. Respawn slot N is now PLAYER START N — placement
// becomes level design instead of a code edit, and the table above degrades into the FALLBACK
// for a level that has not been dressed yet.
//
// ⭐ WHY THE MERGE LIVES IN THIS ENGINE-FREE HEADER AND NOT IN THE UE LAYER THAT CALLS IT.
// The same reason `SpawnSlotAllocator` does — see the block at the bottom of this file.
// `SimulationManagerUImpl.cpp` is a UE module file and the low-level-test target cannot reach
// it, so any claim made there is verified by review and by a PIE run and by nothing else. The
// three properties that actually matter here — that the result does NOT depend on the order
// the engine handed the actors over, that zero placements leave the authored table intact, and
// that a short list fills what it can and leaves the rest authored — are therefore ASSERTIONS
// in `BrawlerRingoutSimulationTest.cpp`. The UE layer holds the actor walk and no policy.
//
// ══ ⛔⛔ TRAP 1: THE ENGINE'S ORDER IS NOT AN ORDER ═════════════════════════════════════════
// `UGameplayStatics::GetAllActorsOfClass` and `TActorIterator` both hand back actors in
// UNSPECIFIED order — it is the level's actor array, whose order depends on load order, on
// streaming, and (under One File Per Actor, which this project uses) on the order the
// external-actor packages resolved. Two peers running the same map can see different orders.
//
// This table is authored data every peer must agree on ENTRY FOR ENTRY: `integrate` indexes it
// with the wire-carried `spawnSlot`, so if the server's slot 0 and a client's slot 0 are
// different points, the client predicts a respawn to the wrong place, the server corrects it,
// and the fighter visibly snaps on every single respawn. Nothing fails to compile, no wire
// fence moves — the wire carries the INDEX, not the POINT — and no existing test notices.
//
// ⛔ SO THE ARRIVAL ORDER IS DISCARDED AND THE LIST IS SORTED BY A KEY EVERY PEER COMPUTES
// IDENTICALLY: THE LEVEL-PLACED ACTOR'S NAME. It is saved in the map package, so every peer
// that loaded the map holds the same string; it is the one property of a placed actor that is
// neither derived from load order nor recomputed at runtime.
//
// ⛔⛔ AND THE OBVIOUS UE SPELLING OF THAT SORT IS THE BUG WEARING THE FIX'S COSTUME.
// `FName::FastLess`, `FNameFastLess` and `FNameEntryId::operator<` all compare the NAME TABLE
// INDEX, and the engine states the consequence at the declaration itself:
// *"Fast non-alphabetical order that is only stable during this process' lifetime"*
// (`Runtime/Core/Public/UObject/NameTypes.h` — `FName::FastLess` / `CompareIndexes` /
// `FNameEntryId::CompareFast`). That index is the order in which the string was first interned
// in THIS process, and a server and a client never intern in the same order. Sorting FNames
// with `<` reintroduces precisely the disagreement this sort exists to remove.
// ⇒ the UE caller hands this function the name as BYTES (`TCHAR_TO_UTF8`) and the comparator
// below is a plain byte-lexicographic `std::string` compare, which no process state can reach.
//
// ⚠ BYTE-LEXICOGRAPHIC, NOT NUMERIC: `PlayerStart_10` sorts BEFORE `PlayerStart_2`. That is
// deterministic and identical on every peer, which is the property that matters — but a level
// carrying more than `kMaxSpawnPoints` player starts keeps the first four IN THAT ORDER, so
// name them such that the order you want is also the order you read.
//
// ⚠ PLACE THEM IN THE PERSISTENT LEVEL. A player start inside a STREAMED sublevel may not be
// loaded at the moment the manager seeds this table, and may be loaded at different moments on
// different peers — which is the same disagreement again, arriving through a different door.
//
// ══ ⛔ TRAP 2: NOT ENOUGH PLAYER STARTS ════════════════════════════════════════════════════
// ZERO placements returns `authoredFallback` UNCHANGED — the level simply has not been dressed
// and the placeholder table is the best answer available. FEWER than `kMaxSpawnPoints` fills
// the low slots from the level and leaves the remainder at their authored values.
//
// ⛔ NO SLOT IS EVER LEFT AT THE ORIGIN, AND THE SHAPE OF THE FUNCTION IS WHAT GUARANTEES IT.
// The result STARTS as a copy of the authored table and is overwritten downwards; there is no
// path through it that default-constructs an entry. A fighter respawning at (0,0,0) is the
// same class of bug as a fighter respawning off the platform, and a `std::array<glm::vec3, N>`
// built up rather than copied down is exactly how that bug gets written.
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ONE PLAYER START, AS THIS ENGINE-FREE HEADER SEES IT: an ordering key and a position.
//
// ⛔ THE NAME IS A `std::string` OF BYTES, NOT AN `FName` AND NOT A WIDE STRING, and that is
// the whole point — see the ⛔⛔ above. The conversion happens once, in the UE layer, at the
// only place an `FName` is in scope at all.
struct LevelSpawnPoint
{
    std::string name;
    glm::vec3   position{0.f, 0.f, 0.f};
};

// The bit pattern of a float, as an ordering token.
//
// ⛔ THE TIE-BREAK BELOW COMPARES BIT PATTERNS, NOT FLOATS, AND THAT IS A CORRECTNESS CHOICE.
// `a < b` is FALSE in both directions for a NaN, which makes a float-comparing tie-break not a
// strict weak ordering and makes `std::sort` UNDEFINED BEHAVIOUR rather than merely wrong. A
// bit pattern is a total order on every input there is. It is not a NUMERIC order (−0.f sorts
// away from +0.f, negatives sort above positives) and it does not need to be: it is a
// tie-break, its only job is to be TOTAL and to be identical on every peer, and the values it
// orders come from the same asset on every peer.
//
// ⚠ AND A FLOAT-COMPARING VERSION COULD NOT BE RED-PROBED ON THIS BUILD ANYWAY. The
// low-level-test target compiles `/fp:fast` (current_state, task 6's toolchain findings), under
// which MSVC may assume no NaN exists and rewrites the comparison; the defect would be real and
// the probe would stay green. This spelling is not a float comparison, so the flag cannot reach
// it and the question does not arise.
inline uint32_t spawnOrderBits(float v)
{
    uint32_t bits = 0u;
    std::memcpy(&bits, &v, sizeof(bits));
    return bits;
}

// THE ORDERING. NAME FIRST — that is the peer-identical part, and in a UE world actor names are
// unique within a level, so in practice the name alone decides every comparison.
//
// ⛔ THE POSITION TIE-BREAK IS A TOTALITY GUARD, NOT A SECOND KEY. `std::sort` over a key with
// ties leaves the tied elements in an ARRIVAL-DEPENDENT order, which is the exact property this
// function exists to destroy — a comparator that can answer "equivalent" for two distinct
// actors has a hole in it the size of the original bug. Two placed actors CAN share a name
// across two different sublevel outers, so the possibility is not zero, and "actor names are
// unique" written in a comment is documentation, not enforcement.
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

// THE MERGE. Takes the placements BY VALUE because it sorts them: the caller's arrival order is
// scratch by construction, and nothing upstream wants it back.
//
// `authoredFallback` is the table to keep wherever the level says nothing. In production it is
// `StaticData::spawnPoints` exactly as the defaults above left it, so "leave the rest authored"
// is literally "leave the rest alone".
inline std::array<glm::vec3, kMaxSpawnPoints> spawnPointsFromLevelPlacements(
    std::vector<LevelSpawnPoint> placements,
    const std::array<glm::vec3, kMaxSpawnPoints>& authoredFallback)
{
    // ⛔ COPY DOWN, NEVER BUILD UP — this line is the (0,0,0) guard, and it is structural
    // rather than a check that could be forgotten. See TRAP 2 above.
    std::array<glm::vec3, kMaxSpawnPoints> merged = authoredFallback;

    // ⛔ THE ARRIVAL ORDER DIES HERE. TRAP 1 above is why this single line is the whole task.
    //
    // `std::sort` and not `std::stable_sort`, and the choice is INERT rather than lucky:
    // `levelSpawnPointOrderBefore` is a TOTAL order, so no two distinct elements are ever
    // equivalent and there is no tied run for stability to preserve. (It would also be inert
    // for a second reason — MSVC runs insertion sort at N <= 32 and this list is at most a
    // handful — but that reason is a toolchain accident and this one is a property of the
    // comparator, so this is the one written down.)
    std::sort(placements.begin(), placements.end(), levelSpawnPointOrderBefore);

    const size_t fillCount = placements.size() < static_cast<size_t>(kMaxSpawnPoints)
        ? placements.size()
        : static_cast<size_t>(kMaxSpawnPoints);
    for (size_t i = 0u; i < fillCount; ++i)
        merged[i] = placements[i].position;

    return merged;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ON THE WIRE (4 B). Seeded ONCE, at registration, by the authority.
//
// It is on the wire for the same reason the movement sub-sim's teleport seed is: so the
// AUTHORITY'S number is the one every peer uses, and a correction RESTORES it instead of a
// client RECOMPUTING it. A recomputed slot is a slot that can differ between peers, and two
// peers disagreeing about a spawn point is a desync you only see on the respawn tick.
//
// ⛔ UNLIKE the movement teleport seed, this is NOT a counter-free edge — nothing consumes and
// clears it. It is written once and read on every respawn for the life of the character.
class InitialConditions
{
public:
    uint32_t spawnSlot = 0u;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ON THE WIRE (5 B): one flags byte plus the absolute respawn tick.
class State
{
public:
    uint8_t flags = 0u;

    // The tick at which `kFlagDead` is cleared and the teleport seed is written. Meaningful
    // only while `kFlagDead` is set; left at its last value once cleared, deliberately — an
    // extra "reset to 0 on respawn" write would be a second thing the wire has to agree on
    // for nothing, and nothing reads it while the dead bit is clear.
    uint32_t respawnAtTick = 0u;
};

// Bit 0 of `State::flags` — dead, awaiting respawn. It is READ by both arms of the law in
// `integrate` (the respawn arm requires it set, the death arm requires it clear), which is the
// point: `brawlerMovementSimulation::kFlagHasCommand` shipped as a bit that was written and
// never read, and this tree has already paid for that once.
inline constexpr uint8_t kFlagDead = 1u << 0;

static_assert(kFlagDead == (1u << 0),
    "brawlerRingout::State::flags - the bit assignment is ON THE WIRE and in the checksum. "
    "Moving it desynchronises every peer that has not shipped the same edit. Bits 1-7 are "
    "free and are where a future ring-out signal goes.");

// ⛔ THESE ARE THIS SUB-SIM'S OWN FLAGS, AND THAT IS THE WHOLE POINT OF THE TYPE.
// Two neighbouring bytes were considered and are both wrong:
//   * `brawlerMovementSimulation::kFlagFrozen` is NOT a "disable the character" flag. It is a
//     per-tick DERIVED output of the support probe, cleared and re-set inside every single
//     movement step. Anything a ring-out task writes there is gone on the next tick.
//   * `brawlerMovementSimulation::State::flags` bits 4-7 ARE free, but they are inside the one
//     header another live initiative holds exclusively, and putting ring-out semantics there
//     would couple two initiatives through a byte neither of them owns.
// A separate slice costs 9 B of the correction buffer's headroom and zero conflict.

inline bool isDead(const State& s) { return (s.flags & kFlagDead) != 0u; }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// OFF THE WIRE. Recomputed from scratch at the top of every step, including every replayed
// step of a resim, which is exactly why the scoring system may read it: a derived edge can
// never arrive on a correction and can never be stale.
class DerivedState
{
public:
    // TRUE on the single tick the character crossed the plane, FALSE on every other tick —
    // including the ticks it spends dead afterwards. This is the EDGE the authority-side
    // score system consumes (ruling 5: the award fires on the authority tick that detects
    // the death, because the authority never rewinds).
    bool diedThisTick = false;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// THE EMPTY PLAYER INPUT — zero serialized fields, zero wire bytes, mirroring
// `dAttackGuardSimulation::InitialConditions`'s empty specialization.
//
// It exists because `ValidDependencies` requires `Dependencies::InputType` and the ownership
// validator treats that type as OWNED by this sub-sim: naming any other sub-sim's input here
// would report an ownership overlap. It does NOT exist because ring-out wants a per-tick
// signal — death is positional and respawn is a tick countdown, so there is nothing for a
// player to press. An input byte costs about ten times a state byte (it is multiplied across
// every entry of every relayed ring); this one costs none.
class PlayerInput
{
public:
    static PlayerInput zero() { return PlayerInput{}; }
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// NO ADAPTERS, so no template parameters — compare
// `brawlerProjectileSimulation::IntegrationUtils`, which is a template purely because it
// carries the physics and query adapters.
//
// ⛔ THERE IS NO `getDeltaTime()`, and its absence is the enforcement of "ticks, never float
// seconds". A countdown in seconds is the reflex implementation of a respawn delay, and a
// sub-sim that is never handed a delta cannot write one.
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

using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils>;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ⛔⛔ RING-OUT MUST INTEGRATE **BEFORE** THE MOVEMENT SUB-SIM, and this is a correctness
// requirement, not a preference. Both edges below point at the movement sub-sim, so the
// dependency validator resolves them the way it resolves every read/write pair: the HARD edge
// (the non-const `InitialConditions&` write) wins and the SOFT edge (the const `State&` read)
// is dropped — `validateOneExternalRef` drops a soft edge whenever `hasHardEdge` holds
// between the same two sims. The declared order that survives is ringout -> movement.
//
// WHAT THAT BUYS, concretely. On the respawn tick T, ring-out clears the dead bit and writes
// the teleport seed; movement consumes that seed LATER IN THE SAME TICK T and puts the body on
// the spawn point. By tick T+1 the position ring-out reads is already above the plane.
// Integrated the other way round, the seed would sit unconsumed until T+1, ring-out would read
// the still-below-plane position at T+1 with the dead bit clear, and the character would die
// again immediately — an infinite respawn loop with nothing failing to compile.
//
// WHAT IT COSTS. Ring-out reads the body position as the movement sub-sim left it at the END
// of tick T-1. A death is therefore detected one tick after the crossing. That is a fixed,
// deterministic one-tick latency on a 60 Hz step, identical on every peer and through every
// replay, and it is the cheaper half of the trade.
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

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// THE LAW, and the order of its three steps is load-bearing.
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

    // 1. THE EDGE IS CLEARED FIRST, EVERY TICK, UNCONDITIONALLY — before either arm can set
    //    it. DerivedState is per-tick scratch that nothing else resets, so a `diedThisTick`
    //    left true from the previous tick would score the same death on every subsequent tick
    //    until the next one happened to overwrite it.
    derived.diedThisTick = false;

    // 2. THE RESPAWN ARM.
    if (isDead(state) && tick >= state.respawnAtTick)
    {
        state.flags = static_cast<uint8_t>(state.flags & ~kFlagDead);

        // ⛔ DEFENSIVE INDEX, AND IT IS A REAL RUNTIME BRANCH — not an `OG_CHECK`.
        // `OG_CHECK` forwards to `checkf`, which COMPILES OUT IN SHIPPING: a guard spelled
        // that way would leave the Shipping build reading past the end of the array, which is
        // the one build where nobody is watching.
        //
        // ⛔ AND THERE IS NO `OG_CHECK` RIDING ALONGSIDE IT EITHER, which is the deliberate
        // half. An assert here fires in Development and takes the process with it, so the
        // recovery branch below would be code no test could ever execute — the defensive arm
        // would be as unread as the bit `kFlagHasCommand` once was. The `[Warning]` log in the
        // else arm is the loudness, and `Ringout.SpawnSlotOutOfRange` is the case that proves
        // the arm runs. A bad slot is an authority assignment bug; it must not also be a crash.
        if (ic.spawnSlot < kMaxSpawnPoints)
        {
            // ⛔ REUSE THE MOVEMENT TELEPORT SEED. DO NOT INVENT A SECOND TELEPORT PATH.
            // Its own contract says it is on the wire "because a respawn must replay
            // identically", and its consumer is the ONE body write in the movement sub-sim
            // that ignores `drivesBody`: it assigns the position, zeroes the velocity, calls
            // `setBodyTransform` + `setBodyLinearVelocity(0)` on the capsule, and clears
            // `teleportPending` in the same tick. A respawn wants every one of those.
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
    // 3. THE DEATH ARM, and `else if` is load-bearing twice over.
    //
    //    Against the RESPAWN arm: on the respawn tick the position ring-out can see is still
    //    last tick's below-the-plane position — movement has not consumed the seed yet. A
    //    plain `if` would re-arm the countdown on the very tick it expired.
    //
    //    Against ITSELF: `!isDead` is what makes a death happen exactly once. A character
    //    that keeps falling below the plane while dead must not re-die or re-arm the
    //    countdown, or the respawn never arrives and the score system counts the same death
    //    on every tick of the fall.
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

// THE `teleportPending == 0` TERM IN THE DEATH ARM is belt-and-braces, and it is worth saying
// exactly what it does and does not do. A pending teleport means the position in
// `movementState` is about to be REPLACED and is therefore not where the character is going to
// be; declining to kill on a stale position is correct on its own terms, and it also covers
// the registration seed (`SimulationManagerUImpl.cpp` seeds a teleport at spawn, before the
// movement sub-sim has integrated once).
//
// ⛔ IT IS NOT A SUBSTITUTE FOR THE EXECUTION ORDER. It stops the immediate re-death under a
// ringout-after-movement order; it does NOT stop the respawn itself arriving a tick late
// there. The ordering argument above is still the thing that has to hold.


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// THE AUTHORITY'S SPAWN-SLOT TABLE (task 3) — what PRODUCES `InitialConditions::spawnSlot`.
//
// ⛔ THIS IS NOT A SUB-SIMULATION TYPE. It is in no composite, it has no `SerializableFields`
// specialization, and it costs ZERO wire bytes — `simulatableBrawler::State` is 335 B with it
// and 335 B without it. The value it produces rides the wire; the table that produced it does
// not. It is authority-side bookkeeping that lives BESIDE the simulation, the same shape the
// score system uses, and for the same reason: a rollback must not be able to reach it.
//
// ⭐ WHY IT IS IN THIS ENGINE-FREE HEADER RATHER THAN IN THE UE LAYER THAT CALLS IT.
// `SimulationManagerUImpl.cpp` is a UE module file and the low-level-test target cannot reach
// it. The two properties that actually matter here — that two separate remote clients get
// DIFFERENT slots, and that a released slot is reused by a later join — are therefore
// ASSERTIONS in `BrawlerRingoutSimulationTest.cpp` instead of prose in a review note. The UE
// layer holds the two calls and no logic at all.
//
// ══ THE RULE ═══════════════════════════════════════════════════════════════════════════════
// The AUTHORITY assigns each character the LOWEST index no live character currently holds,
// once, at registration. Arrival order decides who gets which number.
//
// ⛔ ARRIVAL ORDER IS ACCEPTABLE HERE. DO NOT "FIX" THIS INTO A PER-PEER DERIVATION.
// The determinism rule this looks like it bends exists to stop exactly two things: peers
// DISAGREEING about a spawn point, and a resim producing a DIFFERENT answer than the original
// run. Neither can happen to a value the authority assigns once and the wire carries:
//   * a client never calls this. It READS `spawnSlot` out of an ordinary correction; it never
//     derives it, so there is no second derivation to disagree with the first.
//   * a resim RESTORES `InitialConditions` by whole-struct assignment before replaying, so a
//     replay cannot recompute it differently. It is restored, not re-derived — exactly like
//     the movement sub-sim's `teleportPos`, which is arrival-ordered in the same way and for
//     the same reason.
// The arrival-order dependence is confined to ONE authority-side assignment that is never
// resimulated. That is the same argument that licenses the score itself.
//
// ⛔ AND THE THING A "DETERMINISTIC" REPLACEMENT WOULD BREAK IS NOT HYPOTHETICAL. The obvious
// per-peer derivation is `UEConnectionHandle::GetPlayerSlotForActor`, and it is WRONG: it is
// PER-WIRE, returning 0 for the primary pawn of EVERY remote client and 1..N only for
// couch-co-op siblings on one machine. Two remote clients would both index spawn point 0 and
// respawn on top of each other. See the ⛔ on `StaticData::spawnPoints` above; that is the
// collision this design exists to prevent, and a derivation reintroduces it.
//
// ══ WHEN THE TABLE IS FULL ═════════════════════════════════════════════════════════════════
// `acquire` returns `kNoFreeSlot`, whose VALUE is `kMaxSpawnPoints` — deliberately the exact
// value `integrate`'s defensive index branch already treats as out of range. So an
// over-capacity join respawns to NO teleport seed and logs `[Warning][Ringout.spawnSlot]`,
// rather than silently taking slot 0 and landing on whoever legitimately holds it. LOUD AND
// PLACELESS BEATS QUIET AND COLLIDING: the character still lives, still dies, and still clears
// its dead bit. `Ringout.SpawnSlotOutOfRangeWritesNoTeleportSeed` already covers that arm.
class SpawnSlotAllocator
{
public:
    // "This character holds no slot", and "there was no slot to give". ONE value for both,
    // and it is `kMaxSpawnPoints` so that writing it straight into `spawnSlot` lands on the
    // defensive branch described above instead of needing a second guard at the call site.
    static constexpr uint32_t kNoFreeSlot = kMaxSpawnPoints;

    // ⛔ IDEMPOTENT BY CONTRACT. A character that already holds a slot gets the SAME one back
    // and nothing is consumed. The UE caller's own first-call guard means this is reached once
    // per character, but registration is RETRIED (`tryRegister` returns `Pending` until the
    // bodies resolve), so an allocator that handed out a second entry on a second call would
    // leak the first one with no symptom until the table ran dry.
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

    // ⛔ THE OTHER HALF OF THE CONTRACT, AND IT IS NOT OPTIONAL. Without it a session that
    // churns characters exhausts a four-entry table and every later join respawns nowhere.
    //
    // A NO-OP for an id holding nothing, and that is load-bearing: the UE unregister path runs
    // on BOTH roles, and on a client nothing ever acquired. That is the same shape as the
    // `m_authorityRegisteredIds.erase(id)` it sits beside — an ungated erase whose emptiness on
    // the non-authority role is a property of the inserts, not of a role test.
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

    // The slot this character holds, or `kNoFreeSlot` if it holds none.
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
    // ⛔ A `held` FLAG, NOT A SENTINEL CHARACTER ID. Character ids arrive from the engine as
    // `UObject::GetUniqueID()`; reserving one of their values to mean "free" would be a guess
    // about a range this engine-free header cannot see.
    struct SlotEntry
    {
        bool     held        = false;
        uint32_t characterId = 0u;
    };

    std::array<SlotEntry, kMaxSpawnPoints> m_slots{};
};

} // namespace brawlerRingout

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// SerializableFields specializations
//
// Wire cost: InitialConditions 4 B + State 5 B = 9 B on `simulatableBrawler::State`.
// DerivedState has NO specialization, deliberately — it is off-wire scratch, and the absence
// of a specialization is what enforces that rather than a comment.

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

// ZERO FIELDS, and the empty tuple is the whole declaration — `syncSize` sums an empty field
// list to 0, exactly as it does for `dAttackGuardSimulation::InitialConditions`.
template <>
struct SerializableFields<brawlerRingout::PlayerInput>
{
    static constexpr auto get()
    {
        return std::make_tuple();
    }
};

// APPEND ONLY, the same rule the movement and machine slices carry: the wire layout is
// POSITIONAL, so reordering, inserting or removing an entry is a WIRE FORMAT CHANGE even when
// the byte count does not move, and every peer that has not shipped the same edit desynchronises.
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

// The two wire slices, pinned at their declaration. `SimulatableBrawlerTest.cpp` re-quotes the
// COMPOSITE total when task 2 wires these in; these two assert the SLICES, so a break there
// says which of the two moved instead of only that the total did.
static_assert(syncSize<brawlerRingout::InitialConditions>() == 4u,
    "brawlerRingout::InitialConditions is one uint32_t spawnSlot = 4 B.");
static_assert(syncSize<brawlerRingout::State>() == 5u,
    "brawlerRingout::State is one uint8_t flags + one uint32_t respawnAtTick = 5 B.");
static_assert(syncSize<brawlerRingout::PlayerInput>() == 0u,
    "brawlerRingout::PlayerInput carries NO per-tick signal and must cost nothing. An input "
    "byte is multiplied across every entry of every relayed input ring.");

OGSIM_OPTIMIZE_ON

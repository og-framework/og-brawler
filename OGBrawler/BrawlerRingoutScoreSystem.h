#pragma once
// SPDX-License-Identifier: BUSL-1.1

// brawlerRingout::ScoreSystem — the ring-out point award [ringout task 4].
//
// THE SHAPE IS BORROWED, NOT INVENTED. This file is modelled on
// `BrawlerHitRoutingSystem.h`, the shipped `SimulationSystem` in this tree: the four hooks,
// the `RequiredSimulatables` alias, and the collect-into-a-vector / sort-by-id / then-apply
// body of `postIntegrate`. Everything that differs from that template is called out where it
// differs, and there are exactly two differences that matter — the ROLE GATE and the
// SNAPSHOT.
//
// ══ THE LAW ════════════════════════════════════════════════════════════════════════════════
// On every tick, one point to every fighter that is ALIVE, per fighter that DIED this tick.
// Two simultaneous deaths therefore award TWO points to each survivor (user ruling 3), and
// each dier is absent from the other's award. The dier gains nothing and loses nothing
// (rulings 2 and 7: there is no penalty for dying).
//
// ══ ⛔ SCORES ARE NOT SIMULATION STATE, AND THAT IS THE ENTIRE POINT OF RULING 1'S SPLIT ═══
// `m_scores` is authority-side bookkeeping that lives BESIDE the simulation. It is in no
// composite, it has no `SerializableFields` specialization, it is never serialized, never
// corrected and never rewound — `simulatableBrawler::State` measures 335 B with this system
// compiled in and 335 B without it, which `RingoutScore.TheAwardCostsTheCompositeNothing`
// re-quotes from task 2 unchanged.
//
// ⛔ A SCORE IS A MONOTONIC SIDE EFFECT AND A ROLLBACK IS NOT. Every other quantity in this
// project is RECOMPUTED on a resim; a point is not. Put it in the composite and a correction
// either double-counts it or silently un-awards it, and nothing in the wire fence or the test
// suite would notice. That is why the DETECTION (`brawlerRingout::DerivedState::diedThisTick`)
// is in the sim and replays identically, and the AWARD is here, outside it, on one role.
//
// ══ ⛔ THE ROLE GATE, AND WHY THE STEP CANNOT SUPPLY IT ════════════════════════════════════
// `SimulationSystemsExecutor` fires these hooks on ALL THREE roles — the authority tick, a
// client's forward prediction tick, and EVERY resim replay tick (`SimulationManager.h`, grep
// `or rollback routing stops being deterministic`). An ungated award double-counts on every
// client, once per replayed tick, forever.
//
// ⛔ AND `SimulationTimeStep` CANNOT TELL YOU THE ROLE. `StepKind` is
// `{Normal, Stall, Skip, HardResync}` — CLOCK behaviour, not role — and `getIsResimulating()`
// is FALSE on BOTH the authority tick and a client's forward prediction tick, so it separates
// replay from live, never server from client. The flag has to be plumbed in explicitly, which
// is what `setIsAuthority` exists for.
//
// ⭐ IT DEFAULTS TO FALSE, AND THAT IS THE DESIGN. A `ScoreSystem` nobody configured awards
// NOTHING. The failure mode of a forgotten wiring call is a scoreboard stuck at zero — loud,
// harmless, and repairable — rather than a client quietly inventing points that the server
// never granted and the replicated value keeps overwriting.
//
// ⭐ WHY THE GATE MECHANISM IS HERE AND THE ROLE DERIVATION IS IN THE UE LAYER. This is the
// same split task 3 made for `SpawnSlotAllocator`, and for the same reason:
// `SimulationManagerUImpl.cpp` is a UE module file the low-level-test target CANNOT REACH, so
// a gate implemented there is a gate no test can drive. The bool is DERIVED in the UE layer
// (`worldNetMode != NM_Client`, the world-level test — ⛔ NEVER `HasAuthority()`, which
// `bReplicates = false` makes permanently true on that actor) and APPLIED here, where
// `RingoutScore.ANonAuthorityRoleAwardsNothing` can drive both a prediction tick and a resim
// tick against it.
//
// Layer: OGBrawler core — engine-agnostic, game-specific. NO UE or Godot symbols.
//
// NAMESPACE NOTE: OGSim primitives (`SimulatableList`, `StorageView`, `SimulationTimeStep`)
// are named UNQUALIFIED — the whole OGSim core lives in the GLOBAL namespace (lead D12).

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "OGSimulation/SimulatableList.h"       // SimulatableList
#include "OGSimulation/StorageView.h"           // StorageView
#include "OGSimulation/SimulationTimeContext.h" // SimulationTimeStep
#include "OGSimulation/SystemsExecutor.h"     // the SimulationSystem concept, for the static_assert below
#include "OGBrawler/SimulatableBrawler.h"       // SimulatableBrawler, simulatableBrawler::StaticData
#include "OGBrawler/BrawlerRingoutSimulation.h" // brawlerRingout::State / DerivedState / isDead
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerRingout
{

// The award system. Owns the authority-side score table: character id -> points.
class ScoreSystem
{
public:
    // The subset of the game's simulatables this system observes. The executor projects the
    // full storage down to exactly this list before calling each hook. UNQUALIFIED
    // `SimulatableList` — global namespace (D12).
    using RequiredSimulatables = SimulatableList<SimulatableBrawler>;

    // ---- THE ROLE GATE ---------------------------------------------------------------
    //
    // Supplied by the composition root once, before the manager that fires these hooks
    // exists. ⛔ THE ARGUMENT MUST BE THE WORLD-LEVEL TEST (`GetNetMode() != NM_Client`),
    // never `HasAuthority()`.
    //
    // ⭐ WHY A SETTER RATHER THAN THE EXECUTOR'S `std::piecewise_construct` CTOR, which
    // exists for exactly this (`SystemsExecutor.h`). Three reasons, in order of weight:
    //   1. `ASimulationManagerUImpl::m_systemsExec` is a plain VALUE member, constructed with
    //      the actor. The piecewise ctor would force it to `std::optional` and make the
    //      fourth peer NULLABLE for the first time — `m_manager`'s `Params` take it by
    //      reference, so every fire path would gain a dereference that can be wrong.
    //   2. The piecewise ctor does not actually close the gap it looks like it closes: the
    //      executor's DEFAULT ctor still exists and still compiles, so forgetting to pass the
    //      flag stays a silent runtime fact either way. What makes it safe is the default
    //      being `false`, and that is a property of this class, not of the ctor used.
    //   3. The window in which the flag is unset is PROVABLY EMPTY, not merely short: these
    //      hooks are reached only through `SimulationManager`, `m_manager` is emplaced in
    //      `BeginPlay`, and the call site sits ABOVE the role branch that emplaces it. There
    //      is no tick, and no registration, between construction and the set.
    void setIsAuthority(bool isAuthority) { this->m_isAuthority = isAuthority; }
    bool getIsAuthority() const { return this->m_isAuthority; }

    // ---- READOUT ---------------------------------------------------------------------
    //
    // `scoreOf` answers 0 for an id with no entry, which is the same answer it gives for a
    // registered id that has not scored. Task 5 replicates this value onto the character.
    uint32_t scoreOf(unsigned int characterId) const
    {
        const auto found = this->m_scores.find(characterId);
        return found == this->m_scores.end() ? 0u : found->second;
    }

    // Whether this id has a ROSTER ENTRY at all — distinct from "scores zero". The
    // unregister contract is about this, not about the value.
    bool hasScoreEntry(unsigned int characterId) const
    {
        return this->m_scores.find(characterId) != this->m_scores.end();
    }

    size_t scoreEntryCount() const { return this->m_scores.size(); }

    // ---- HOOKS -----------------------------------------------------------------------

    // preIntegrate — no work. The award reads `diedThisTick`, which the ring-out sub-sim
    // produces DURING integrate, so there is nothing to do before it. Present to satisfy the
    // four-hook `SimulationSystem` concept, exactly as `brawlerHitRouting::System`'s is.
    void preIntegrate(const SimulationTimeStep& /*step*/,
                      StorageView<SimulatableBrawler> /*view*/,
                      const simulatableBrawler::StaticData& /*staticData*/)
    {
    }

    // postIntegrate — the award, authority only.
    void postIntegrate(const SimulationTimeStep& step,
                       StorageView<SimulatableBrawler> view,
                       const simulatableBrawler::StaticData& /*staticData*/)
    {
        // ⛔ THE GATE. See the banner: a client reaches this on its forward prediction tick
        // AND on every replayed tick of every resim.
        if (!this->m_isAuthority)
            return;

        // ---- ONE WALK, AND IT IS ALSO THE SNAPSHOT ------------------------------------
        //
        // ⛔ COLLECT BEFORE AWARDING. Ruling 3 says two simultaneous deaths award two points
        // to each SURVIVOR, and that both diers are absent from each other's award. The
        // implementation that walks and awards in the same pass cannot express that: having
        // retired the first dier, it either awards the second dier a point for the first (if
        // it tests liveness before its own bookkeeping) or not (if after), and WHICH of those
        // two wrong answers it gives depends on unordered-map order. `rows` is read once,
        // before a single point is added, so there is no such dependence to have.
        //
        // No `reserve`: neither the view nor the storage exposes a cheap count, and the
        // populations this runs against are `kPreDietCharacterCap` = 4 characters.
        struct Row
        {
            unsigned int id;
            bool         died;   // brawlerRingout::DerivedState::diedThisTick — the EDGE
            bool         alive;  // !isDead(State) — the LEVEL
        };
        std::vector<Row> rows;
        uint32_t deathCount = 0u;
        view.forEachSimulatable<SimulatableBrawler>(
            [&rows, &deathCount](unsigned int id, SimulatableBrawler& brawler)
            {
                const auto& allState = brawler.getAllState();
                const bool died =
                    allState.getDerivedState().get<brawlerRingout::DerivedState>().diedThisTick;
                // ⭐ ONE TEST RETIRES BOTH EXCLUSIONS. `integrate` sets `kFlagDead` on the
                // same tick it raises `diedThisTick`, so a dier reads NOT ALIVE here and is
                // excluded from its own award and from its twin's by the same expression that
                // excludes a fighter still waiting out an EARLIER death. "Alive at the death
                // tick" (ruling 2) is a LEVEL, and this is it; being airborne is not part of
                // it, which is the whole reason ruling 2 needs nothing from the movement sim's
                // support state.
                const bool alive =
                    !brawlerRingout::isDead(allState.getState().get<brawlerRingout::State>());
                rows.push_back(Row{ id, died, alive });
                if (died)
                    ++deathCount;
            });

        // ⭐ THE EARLY-OUT, AND IT IS THE COMMON CASE. Nobody dies on the overwhelming
        // majority of ticks; that tick costs one vector walk and no sort.
        if (deathCount == 0u)
            return;

        // ---- THE ORDERED APPLY -----------------------------------------------------------
        //
        // ⛔ SORT BY ID. `SystemsExecutor.h` item 81 states this as a LIBRARY CONTRACT, not a
        // style note: character order within a sweep is unordered-map order — unspecified, and
        // machine-varying with registration history — so a system whose per-character effects
        // are non-commutative "MUST impose its own order over the view (e.g. sort by id)" or it
        // "manufactures a permanent, input-independent server/client divergence that no amount
        // of gate or verdict work downstream can repair".
        //
        // ⚠ BE HONEST ABOUT WHAT THIS BUYS TODAY: the effect below is `+=` on a per-id counter,
        // which is COMMUTATIVE, so the walk order cannot change the resulting scores while the
        // law stays this shape. The sort is discharging the contract IN ADVANCE, and the
        // things that would make it load-bearing are all one edit away — a per-tick cap on how
        // many points exist, a first-blood bonus to whoever sorts first, any tie-break, or an
        // award whose amount READS the score table while the table is being written.
        // ⛔ DO NOT DELETE IT AS DEAD WEIGHT. It is deleted correctly only together with a
        // proof that the award is still commutative, and that proof is not in the type system.
        // `RingoutScore.TwoIterationOrdersProduceIdenticalScores` pins the composition of this
        // sort and the snapshot above; the snapshot is the half that bites today.
        std::sort(rows.begin(), rows.end(),
            [](const Row& a, const Row& b) { return a.id < b.id; });

        for (const Row& row : rows)
        {
            if (!row.alive)
                continue;
            // `operator[]`, deliberately: it inserts at 0 and then adds. The roster is seeded
            // at registration below, so in a legal session the entry already exists — but an
            // award to an id the roster has not got is a real award, not a dropped one, and a
            // `find`-and-skip here would hide a registration defect AND make the gate's own
            // RED PROBE come back green on an empty table.
            this->m_scores[row.id] += deathCount;
        }

        // ⚠ `deaths` and `pointsEach` are the SAME NUMBER and are printed separately on
        // purpose: they are the same only because the award is one point per death, and the
        // day a bonus or a cap changes that, the line that has to show it already exists.
        OGBLOG_G("[Ringout.score] tick=%u deaths=%u pointsEach=%u survivors=%u of %u",
            step.getTick(), deathCount, deathCount,
            static_cast<unsigned int>(std::count_if(rows.begin(), rows.end(),
                [](const Row& r) { return r.alive; })),
            static_cast<unsigned int>(rows.size()));
    }

    // onCharacterRegistered — seed this character's roster entry at zero.
    //
    // ⛔ AUTHORITY ONLY, like the award itself. On a client the table stays empty for the life
    // of the session — the same property `ASimulationManagerUImpl::m_authorityRegisteredIds`
    // and task 3's `m_spawnSlots` both rely on, and for the same reason: nothing on that role
    // ever writes it, so nothing on that role can read a wrong answer out of it. A client
    // learns scores from the replicated `UPROPERTY` task 5 adds, never from here.
    //
    // WHY SEED AT ALL rather than let the first award insert. A fighter who has never scored
    // must be DISTINGUISHABLE from a fighter who is not in the match — the scoreboard (task 6)
    // draws a row of `0`, not a missing row — and the unregister contract below needs
    // something to drop for a character that died to nobody.
    void onCharacterRegistered(unsigned int id,
                               StorageView<SimulatableBrawler> /*view*/,
                               const simulatableBrawler::StaticData& /*staticData*/)
    {
        if (!this->m_isAuthority)
            return;
        this->m_scores.emplace(id, 0u);   // emplace, not [] — a re-registration keeps the score
    }

    // onCharacterUnregistered — forget the id, or the table grows for the life of the session
    // as players join and leave.
    //
    // ⛔ UNGATED, and that is deliberate rather than an oversight — the same call task 3 made
    // for `SpawnSlotAllocator::release`. `notifyCharacterUnregistered` fires on BOTH roles;
    // nothing on the client role ever inserted, so the erase is a no-op there BY CONSTRUCTION.
    // A role gate here would be a branch no test could ever make fail.
    //
    // ⚠ THE SCORE DIES WITH THE CHARACTER. There is no match-level ledger keyed by player —
    // ruling 4 puts no match end in this slice, and inventing a persistent one here would be
    // inventing a scope the user has not asked for.
    void onCharacterUnregistered(unsigned int id,
                                 StorageView<SimulatableBrawler> /*view*/,
                                 const simulatableBrawler::StaticData& /*staticData*/)
    {
        this->m_scores.erase(id);
    }

private:
    // ⛔ THE ROLE GATE, DEFAULTED CLOSED. See `setIsAuthority`.
    bool m_isAuthority = false;

    // character id -> points. ⛔ NOT SIM STATE. In no composite, on no wire, never corrected,
    // never rewound — see the banner. `uint32_t` at one point per death is unreachable
    // overflow; it is sized to match the replicated property task 5 adds rather than to a
    // bound anyone has to defend.
    std::unordered_map<unsigned int, uint32_t> m_scores;
};

} // namespace brawlerRingout

// The concept, checked at the definition rather than only where the executor instantiates it:
// `SimulationSystemsExecutor`'s requires-clause would catch a non-conforming system too, but it
// would catch it in `SimulationManagerUImpl.h` — a UE module file the low-level-test target
// cannot compile — so the diagnostic would appear only in an Editor build. Here it appears in
// every translation unit that includes this header, including the LLT ones.
static_assert(SimulationSystem<brawlerRingout::ScoreSystem, simulatableBrawler::StaticData>,
    "brawlerRingout::ScoreSystem must satisfy the SimulationSystem concept: a "
    "RequiredSimulatables alias naming a SimulatableList<>, plus preIntegrate / postIntegrate / "
    "onCharacterRegistered / onCharacterUnregistered taking (step|id, view, staticData).");

OGSIM_OPTIMIZE_ON

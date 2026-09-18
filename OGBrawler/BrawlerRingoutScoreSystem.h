#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerRingoutScoreSystem-rationale.md · docs/BrawlerRingoutScoreSystem-guards.md

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "OGSimulation/SimulatableList.h"
#include "OGSimulation/StorageView.h"
#include "OGSimulation/SimulationTimeContext.h"
#include "OGSimulation/SimulationSerialization.h"
#include "OGSimulation/SystemsExecutor.h"
#include "OGSimulation/SystemRoleAffinity.h"
#include "OGSimulation/OGAssert.h"
#include "OGBrawler/SimulatableBrawler.h"
#include "OGBrawler/BrawlerRingoutSimulation.h"
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerRingout
{

class ScoreSystem
{
public:
    using RequiredSimulatables = SimulatableList<SimulatableBrawler>;

    static constexpr SystemRoleAffinity kRoleAffinity = SystemRoleAffinity::AuthorityOnly;

    uint32_t scoreOf(unsigned int characterId) const
    {
        const auto found = this->m_scores.find(characterId);
        return found == this->m_scores.end() ? 0u : found->second;
    }

    bool hasScoreEntry(unsigned int characterId) const
    {
        return this->m_scores.find(characterId) != this->m_scores.end();
    }

    size_t scoreEntryCount() const { return this->m_scores.size(); }

    void preIntegrate(const SimulationTimeStep& /*step*/,
                      StorageView<SimulatableBrawler> /*view*/,
                      const simulatableBrawler::StaticData& /*staticData*/)
    {
    }

    void postIntegrate(const SimulationTimeStep& step,
                       StorageView<SimulatableBrawler> view,
                       const simulatableBrawler::StaticData& /*staticData*/)
    {
        struct Row
        {
            unsigned int id;
            bool         died;
            bool         alive;
        };
        // ⛔G-03  docs/BrawlerRingoutScoreSystem-guards.md
        std::vector<Row> rows;
        uint32_t deathCount = 0u;
        view.forEachSimulatable<SimulatableBrawler>(
            [&rows, &deathCount](unsigned int id, SimulatableBrawler& brawler)
            {
                const auto& allState = brawler.getAllState();
                const bool died =
                    allState.getDerivedState().get<brawlerRingout::DerivedState>().diedThisTick;
                // ∴D-01  docs/BrawlerRingoutScoreSystem-rationale.md
                const bool alive =
                    !brawlerRingout::isDead(allState.getState().get<brawlerRingout::State>());
                rows.push_back(Row{ id, died, alive });
                if (died)
                    ++deathCount;
            });

        if (deathCount == 0u)
            return;

        std::sort(rows.begin(), rows.end(),
            [](const Row& a, const Row& b) { return a.id < b.id; });

        OG_CHECK(std::is_sorted(rows.begin(), rows.end(),
                     [](const Row& a, const Row& b) { return a.id < b.id; }),
            "brawlerRingout::ScoreSystem::postIntegrate - the rows are not in id order, so the "
            "std::sort above has been deleted or moved. Was guard G-10 (SORT BY ID), retired "
            "into this check by ringout task 14. SystemsExecutor.h item 81 makes the ordering a "
            "LIBRARY CONTRACT: a system whose per-character effects are non-commutative MUST "
            "impose its own order over the view, or it manufactures a permanent, "
            "input-independent server/client divergence. Deleting the sort ALONE leaves the "
            "whole suite green (measured, tasks 4 and 14) - this check is the only thing that "
            "turns it red.");

        for (const Row& row : rows)
        {
            if (!row.alive)
                continue;
            // ⛔G-04  docs/BrawlerRingoutScoreSystem-guards.md
            this->m_scores[row.id] += deathCount;
        }

        OGBLOG_G("[Ringout.score] tick=%u deaths=%u pointsEach=%u survivors=%u of %u",
            step.getTick(), deathCount, deathCount,
            static_cast<unsigned int>(std::count_if(rows.begin(), rows.end(),
                [](const Row& r) { return r.alive; })),
            static_cast<unsigned int>(rows.size()));
    }

    void onCharacterRegistered(unsigned int id,
                               StorageView<SimulatableBrawler> /*view*/,
                               const simulatableBrawler::StaticData& /*staticData*/)
    {
        // ⛔G-06  docs/BrawlerRingoutScoreSystem-guards.md
        this->m_scores.emplace(id, 0u);
    }

    void onCharacterUnregistered(unsigned int id,
                                 StorageView<SimulatableBrawler> /*view*/,
                                 const simulatableBrawler::StaticData& /*staticData*/)
    {
        this->m_scores.erase(id);
    }

private:
    // ∴D-02  docs/BrawlerRingoutScoreSystem-rationale.md
    std::unordered_map<unsigned int, uint32_t> m_scores;
};

} // namespace brawlerRingout

namespace brawlerRingout::detail
{
template <typename Step>
concept StepExposesARole =
       requires(const Step& s) { { s.getIsAuthority() }; }
    || requires(const Step& s) { { s.getIsServer() }; }
    || requires(const Step& s) { { s.getRole() }; }
    || requires(const Step& s) { { s.isAuthority() }; };

struct RoleBearingStep { bool getIsAuthority() const { return true; } };
}

static_assert(SimulationSystem<brawlerRingout::ScoreSystem, simulatableBrawler::StaticData>,
    "brawlerRingout::ScoreSystem must satisfy the SimulationSystem concept: a "
    "RequiredSimulatables alias naming a SimulatableList<>, a static constant SystemRoleAffinity "
    "kRoleAffinity, plus preIntegrate / postIntegrate / onCharacterRegistered / "
    "onCharacterUnregistered taking (step|id, view, staticData). "
    "Asserted at the definition, not only where SimulationSystemsExecutor instantiates it, "
    "because that instantiation is in SimulationManagerUImpl.h - a UE module file the "
    "low-level-test target cannot compile - so the diagnostic would otherwise appear only in "
    "an Editor build.");

static_assert(!Serializable<brawlerRingout::ScoreSystem>,
    "brawlerRingout::ScoreSystem must never gain a SerializableFields specialization. Was guard "
    "G-08 (SCORES ARE NOT SIMULATION STATE), retired into this assertion by ringout task 14. A "
    "score is a MONOTONIC SIDE EFFECT and a rollback is not: put the table in a composite and a "
    "correction either double-counts it or silently un-awards it, and neither the wire fence nor "
    "the test suite would notice.");

static_assert(!brawlerRingout::detail::StepExposesARole<SimulationTimeStep>,
    "SimulationTimeStep has grown a role accessor. Was guard G-09 (SimulationTimeStep CANNOT "
    "TELL YOU THE ROLE), retired into this assertion by ringout task 14. StepKind describes the "
    "CLOCK and getIsResimulating() is FALSE on both the authority tick and a client's forward "
    "prediction tick, so the step separates replay from live, never server from client. Ringout "
    "task 19 moved the role OUT of this system: the executor reads kRoleAffinity above and is "
    "handed the role by SimulationManager on every fire, so there is exactly ONE role source. If "
    "this fires, the step has become a SECOND one - re-read the guards doc before letting any "
    "hook here branch on it.");

static_assert(brawlerRingout::detail::StepExposesARole<brawlerRingout::detail::RoleBearingStep>,
    "VACUITY CONTROL for the assertion above: StepExposesARole must be TRUE of a step that does "
    "expose a role, or the negative assertion is true of everything and enforces nothing.");

OGSIM_OPTIMIZE_ON

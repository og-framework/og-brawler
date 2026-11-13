#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerRingoutRespawnSystem-rationale.md

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#include "glm/geometric.hpp"
#include "glm/vec3.hpp"
#include "OGSimulation/SimulatableList.h"
#include "OGSimulation/StorageView.h"
#include "OGSimulation/SimulationTimeContext.h"
#include "OGSimulation/SimulationSerialization.h"
#include "OGSimulation/SystemsExecutor.h"
#include "OGSimulation/SystemRoleAffinity.h"
#include "OGSimulation/OGAssert.h"
#include "OGBrawler/BrawlerMovementSimulation.h"
#include "OGBrawler/BrawlerRingoutSimulation.h"
#include "OGBrawler/SimulatableBrawler.h"
#include "OGBrawlerLog.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerRingout
{

class RespawnSlotSystem
{
public:
    using RequiredSimulatables = SimulatableList<SimulatableBrawler>;

    static constexpr SystemRoleAffinity kRoleAffinity = SystemRoleAffinity::AuthorityOnly;

    void preIntegrate(const SimulationTimeStep& step,
                      StorageView<SimulatableBrawler> view,
                      const simulatableBrawler::StaticData& staticData)
    {
        const uint32_t tick = step.getTick();
        chooseFor(view, staticData.m_ringoutStaticData, tick, "respawn",
            [tick](const State& ringout, const DerivedState&) { return respawnIsDue(ringout, tick); });
    }

    void postIntegrate(const SimulationTimeStep& step,
                       StorageView<SimulatableBrawler> view,
                       const simulatableBrawler::StaticData& staticData)
    {
        chooseFor(view, staticData.m_ringoutStaticData, step.getTick(), "death",
            [](const State&, const DerivedState& derived) { return derived.diedThisTick; });
    }

    void onCharacterRegistered(unsigned int /*id*/,
                               StorageView<SimulatableBrawler> /*view*/,
                               const simulatableBrawler::StaticData& staticData)
    {
        OG_CHECK(kSpawnPointOccupiedRadius > 2.f * staticData.m_movementStaticData.capsuleRadius,
            "brawlerRingout::kSpawnPointOccupiedRadius must exceed two capsule radii: below that, two "
            "capsules standing side by side at one spawn point read as 'free' and a respawn lands in "
            "the other body. Raise the radius with the capsule.");
    }

    void onCharacterUnregistered(unsigned int /*id*/,
                                 StorageView<SimulatableBrawler> /*view*/,
                                 const simulatableBrawler::StaticData& /*staticData*/)
    {
    }

private:
    struct Row
    {
        unsigned int        id;
        SimulatableBrawler* brawler;
    };

    static std::vector<Row> rowsInIdOrder(StorageView<SimulatableBrawler>& view)
    {
        std::vector<Row> rows;
        view.forEachSimulatable<SimulatableBrawler>(
            [&rows](unsigned int id, SimulatableBrawler& brawler) { rows.push_back(Row{ id, &brawler }); });

        std::sort(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.id < b.id; });
        return rows;
    }

    static std::vector<glm::vec3> occupantsOtherThan(const std::vector<Row>& rows, unsigned int id,
                                                     const StaticData& sd)
    {
        std::vector<glm::vec3> occupants;
        for (const Row& row : rows)
        {
            if (row.id == id)
                continue;

            const auto& state = row.brawler->getAllState().getState();
            if (isDead(state.get<State>()))
            {
                const uint32_t reserved = state.get<InitialConditions>().spawnSlot;
                if (reserved < kMaxSpawnPoints)
                    occupants.push_back(sd.spawnPoints[reserved]);
                continue;
            }

            const auto& movementIc = state.get<brawlerMovementSimulation::InitialConditions>();
            occupants.push_back(movementIc.teleportPending != 0u
                ? movementIc.teleportPos
                : state.get<brawlerMovementSimulation::State>().bodyState.position);
        }
        return occupants;
    }

    static float nearestOccupantDistance(const glm::vec3& point, const std::vector<glm::vec3>& occupants)
    {
        float nearest = -1.f;
        for (const glm::vec3& occupant : occupants)
        {
            const float distance = std::sqrt(glm::dot(occupant - point, occupant - point));
            if (nearest < 0.f || distance < nearest)
                nearest = distance;
        }
        return nearest;
    }

    template <typename Selects>
    static void chooseFor(StorageView<SimulatableBrawler>& view, const StaticData& sd, uint32_t tick,
                          const char* when, Selects selects)
    {
        const std::vector<Row> rows = rowsInIdOrder(view);
        OG_CHECK(std::is_sorted(rows.begin(), rows.end(), [](const Row& a, const Row& b) { return a.id < b.id; }),
            "brawlerRingout::RespawnSlotSystem - the rows are not in id order, so the std::sort in "
            "rowsInIdOrder has been deleted or moved. Two characters choosing in the same tick compete "
            "for the same free slot, and the view walks an unordered map: without the sort the winner "
            "is the walk order's, not the id's. This check is that sort-by-id rule, in the shape"
            " ScoreSystem's postIntegrate uses.");

        for (const Row& row : rows)
        {
            const auto& allState = row.brawler->getAllState();
            if (!selects(allState.getState().get<State>(), allState.getDerivedState().get<DerivedState>()))
                continue;

            const std::vector<glm::vec3> occupants = occupantsOtherThan(rows, row.id, sd);
            InitialConditions& ic = row.brawler->editAllState().editState().edit<InitialConditions>();
            const uint32_t before = ic.spawnSlot;
            const uint32_t chosen = chooseRespawnSlot(before, sd.spawnPoints, occupants);
            ic.spawnSlot = chosen;

            if (chosen >= kMaxSpawnPoints)
                continue;

            const bool occupied = spawnPointIsOccupied(sd.spawnPoints[chosen], occupants);
            OGBLOG_G("%s[Ringout.respawnSlot] tick=%u id=%u at=%s slot=%u->%u nearestOccupant=%.3f "
                     "radius=%.3f occupied=%u",
                occupied ? "[Warning]" : "", tick, row.id, when, before, chosen,
                nearestOccupantDistance(sd.spawnPoints[chosen], occupants), kSpawnPointOccupiedRadius,
                occupied ? 1u : 0u);
        }
    }
};

} // namespace brawlerRingout

static_assert(SimulationSystem<brawlerRingout::RespawnSlotSystem, simulatableBrawler::StaticData>,
    "brawlerRingout::RespawnSlotSystem must satisfy the SimulationSystem concept. Asserted at the "
    "definition because the game's executor alias is in a UE module file the low-level-test target "
    "cannot compile.");

static_assert(brawlerRingout::RespawnSlotSystem::kRoleAffinity == SystemRoleAffinity::AuthorityOnly,
    "brawlerRingout::RespawnSlotSystem must stay AuthorityOnly. It writes the wire-carried "
    "InitialConditions::spawnSlot from the OTHER characters' positions; on a client those are "
    "predictions, and on a replayed tick they are a rewind's. AllRoles makes every peer choose its "
    "own slot and the authority's correction fight it. This assertion is that rule; there is no "
    "guard entry for it.");

static_assert(!Serializable<brawlerRingout::RespawnSlotSystem>,
    "brawlerRingout::RespawnSlotSystem holds no state and must stay off the wire: the slot it chooses "
    "rides InitialConditions::spawnSlot, which is already serialized.");

OGSIM_OPTIMIZE_ON

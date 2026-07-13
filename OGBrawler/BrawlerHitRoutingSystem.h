#pragma once
// SPDX-License-Identifier: BUSL-1.1

// brawlerHitRouting::System — the OGSim-system-api "system" form of the
// cross-character combat-event routing pass (formerly a free function in an
// engine-adapter-owned routing wrapper, since removed). A system is a
// cross-simulatable coordinator: it observes
// the whole SimulatableBrawler population once per tick — AFTER every per-
// character integrate has run — and fans out combat signals (target-side
// HitFlinch, shooter-side GuardFlinch on a blocked projectile) via the
// brawlerInboundHit::DerivedState slice on each character's DerivedState
// composite (see current_state.md §D1 for the off-wire discipline).
//
// This class satisfies the engine-core `SimulationSystem<T, StaticDataT>`
// concept (Plugins/OGSimulation/.../SystemsExecutor.h) with
// StaticDataT = simulatableBrawler::StaticData. The SimulationSystemsExecutor
// peer fires its hooks; the manager wires the peer around integrateAll.
//
// Layer: OGBrawler core — engine-agnostic, game-specific. Depends on
// SimulatableBrawler, BrawlerProjectileSimulation, brawlerInboundHit, and the
// engine-agnostic OGSim primitives (SimulatableList, StorageView,
// SimulationTimeStep). NO UE or Godot symbols.
//
// NAMESPACE NOTE: OGSim primitives (SimulatableList, StorageView) are named
// UNQUALIFIED here — the entire OGSim core lives in the GLOBAL namespace, and
// this initiative ratified that convention (lead D12, 2026-07-07). The design
// corpus's `ogsim::` qualification is schematic.
//
// PRAGMA NOTE (N-1): the `#pragma optimize("", off/on)` convention is scoped to
// OGSim-core headers (tasks 1-5). OGBrawler-core headers — this one included —
// do not use it; match the existing OGBrawler convention (no pragma).
//
// Deterministic order (D4): iterates attackers in ascending SimulatableBrawler
// id — sorts the storage-view snapshot internally so the routing outcome is
// byte-identical across machines (StorageView iteration order is unspecified).

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "OGSimulation/SimulatableList.h"       // SimulatableList
#include "OGSimulation/StorageView.h"           // StorageView
#include "OGSimulation/SimulationTimeContext.h" // SimulationTimeStep
#include "OGBrawler/SimulatableBrawler.h"       // SimulatableBrawler, simulatableBrawler::StaticData
#include "OGBrawler/BrawlerProjectileSimulation.h"

namespace brawlerHitRouting
{
    // The hit-routing system. Owns the actor-level root-body-id -> registered
    // brawler map (moved here from the engine adapter): the per-character routing
    // table the postIntegrate pass keys inbound hits against. Populate/erase is
    // driven by the onCharacterRegistered / onCharacterUnregistered lifecycle
    // hooks; per-machine-local ids make the map correctly system-owned (§3.9,
    // parent-initiative D8 — non-rollback-affecting, per-machine).
    class System
    {
    public:
        // The subset of the game's simulatables this system observes. The
        // executor projects the full storage down to exactly this list before
        // calling each hook. UNQUALIFIED SimulatableList — global namespace (D12).
        using RequiredSimulatables = SimulatableList<SimulatableBrawler>;

        // preIntegrate — no work in v1. Routing is a post-integrate reduction
        // (it reads each character's just-produced attackHits[] / projectile slot
        // state), so nothing needs to run before integrateAll. Present to satisfy
        // the four-hook SimulationSystem concept.
        void preIntegrate(const SimulationTimeStep& /*step*/,
                          StorageView<SimulatableBrawler> /*view*/,
                          const simulatableBrawler::StaticData& /*staticData*/)
        {
        }

        // postIntegrate — the per-tick routing pass (T16 logic, relocated). Four
        // branches:
        //   1. Reset every character's inboundHit flags (wasHitThisTick,
        //      wasProjectileBlockedThisTick) — both are one-shots owned here.
        //   2. Radial swing hits (T3): route HitFlinch to the struck character.
        //   3. Projectile damage hits (T3, endReason==2): route HitFlinch to the
        //      struck character (endTick guard makes it fire exactly once).
        //   4. Projectile guard-blocks (T15, endReason==4): route GuardFlinch to
        //      the shooter (self-flag; no map lookup).
        //
        // D5 self-hit filter (SimulatableBrawler* pointer identity — NOT
        // rootBodyId) applies on the target-routing branches (2, 3) only; branch 4
        // is inherently self-directed.
        void postIntegrate(const SimulationTimeStep& step,
                           StorageView<SimulatableBrawler> view,
                           const simulatableBrawler::StaticData& /*staticData*/)
        {
            const uint32_t currentTick = step.getTick();

            // Deterministic walk order (D4): StorageView iteration order is
            // unspecified; sort by ascending id for cross-machine reproducibility.
            // This sort — not the hash-map iteration — is the authoritative per-
            // tick ordering the routing contract depends on.
            std::vector<std::pair<unsigned int, SimulatableBrawler*>> ordered;
            view.forEachSimulatable<SimulatableBrawler>(
                [&ordered](unsigned int id, SimulatableBrawler& brawler)
                {
                    ordered.emplace_back(id, &brawler);
                });
            std::sort(ordered.begin(), ordered.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });

            // 1. Reset every character's inbound-signal flags before repopulating
            //    this tick. The routing pass owns the reset/set lifecycle so each
            //    tick's signal is fresh (the machine sim never sees a stale flag).
            for (const auto& [id, brawlerPtr] : ordered)
            {
                auto& slice = brawlerPtr->editAllState().editDerivedState().m_inboundHitDerivedState;
                slice.wasHitThisTick               = false;   // T3
                slice.wasProjectileBlockedThisTick = false;   // T15
            }

            // 2. Radial swing hits — each attacker's post-integrate attackHits[]
            //    carries the stable root body id of every character its weapon
            //    overlapped this tick.
            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                const auto& radialDerived =
                    attackerPtr->getAllState().getDerivedState().m_attackDerivedState;
                for (const auto& hit : radialDerived.getAttackHits())
                {
                    auto found = this->m_byRootBodyId.find(hit.hitRootBodyId.value);
                    if (found == this->m_byRootBodyId.end())
                        continue;
                    SimulatableBrawler* target = found->second;
                    if (target == attackerPtr)   // D5 self-hit filter (pointer identity, not rootBodyId)
                        continue;
                    target->editAllState().editDerivedState()
                        .m_inboundHitDerivedState.wasHitThisTick = true;
                }
            }

            // 3. Projectile damage hits — a slot that ENDED this exact tick with
            //    endReason==2 (hit) routes a one-shot inbound hit to the struck
            //    character. The endTick guard makes it fire once: a hit slot keeps
            //    endReason==2 until it recycles, so without this the target would
            //    re-flinch every tick until then.
            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                const auto& projState =
                    attackerPtr->getAllState().getState().get<brawlerProjectileSimulation::State>();
                for (const auto& slot : projState.slots)
                {
                    if (slot.endTick != currentTick || slot.endReason != 2)
                        continue;
                    auto found = this->m_byRootBodyId.find(slot.hitRootBodyId.value);
                    if (found == this->m_byRootBodyId.end())
                        continue;
                    SimulatableBrawler* target = found->second;
                    if (target == attackerPtr)
                        continue;
                    target->editAllState().editDerivedState()
                        .m_inboundHitDerivedState.wasHitThisTick = true;
                }
            }

            // 4. [T15] Shooter-side projectile-blocked routing — any projectile
            //    slot that ended this exact tick with endReason==4 (blockedByGuard,
            //    per T14) routes a GuardFlinch trigger to the slot's OWNING
            //    character (the shooter). No map lookup — the shooter IS the
            //    attacker whose sim is being iterated. No self-hit filter: self is
            //    exactly the right target here.
            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                const auto& projState =
                    attackerPtr->getAllState().getState().get<brawlerProjectileSimulation::State>();
                for (const auto& slot : projState.slots)
                {
                    if (slot.endTick != currentTick || slot.endReason != 4)
                        continue;
                    attackerPtr->editAllState().editDerivedState()
                        .m_inboundHitDerivedState.wasProjectileBlockedThisTick = true;
                }
            }
        }

        // onCharacterRegistered — index the just-registered character for routing.
        // §3.11 timing: the character IS already in storage when this fires, so the
        // view resolves it. Key = the character's CAPSULE (root) body id — the
        // value the query adapter emits as SpatialQueryHit::rootBodyId for hits on
        // ANY of the character's shapes (hurtbox or guard), hence carried by radial
        // attackHits[].hitRootBodyId / projectile slot.hitRootBodyId. Value = the
        // storage-stable pointer to the SimulatableBrawler (unique_ptr-backed, so
        // its address is stable for the registered lifetime).
        void onCharacterRegistered(unsigned int id,
                                   StorageView<SimulatableBrawler> view,
                                   const simulatableBrawler::StaticData& /*staticData*/)
        {
            SimulatableBrawler& brawler = view.get<SimulatableBrawler>(id);
            const uint32_t rootBodyId = brawler.getCharacterBindings().capsuleBodyId.value;
            this->m_byRootBodyId[rootBodyId] = &brawler;
        }

        // onCharacterUnregistered — drop this character's routing entry BEFORE it
        // is erased from storage (§3.11 timing: the character is still in storage
        // here, so the view resolves it). Erase by stored-pointer identity (not by
        // recomputed key) so no stale entry can survive regardless of how the key
        // was derived.
        void onCharacterUnregistered(unsigned int id,
                                     StorageView<SimulatableBrawler> view,
                                     const simulatableBrawler::StaticData& /*staticData*/)
        {
            const SimulatableBrawler* removed = &view.get<SimulatableBrawler>(id);
            for (auto mapIt = this->m_byRootBodyId.begin(); mapIt != this->m_byRootBodyId.end(); )
            {
                if (mapIt->second == removed)
                    mapIt = this->m_byRootBodyId.erase(mapIt);
                else
                    ++mapIt;
            }
        }

    private:
        // Actor-level root-body-id -> registered brawler, for cross-character
        // routing. Key = the character capsule's BodyId.value
        // (CharacterBindings::capsuleBodyId); value = raw pointer into storage's
        // unique_ptr (stable across the character's registered lifetime). Moved
        // out of the engine adapter (ASimulationManagerUImpl::m_byRootBodyId) so
        // the routing system owns its own bookkeeping (§3.9 / D8).
        std::unordered_map<uint32_t /* BodyId.value */, SimulatableBrawler*> m_byRootBodyId;
    };
} // namespace brawlerHitRouting

#pragma once
// SPDX-License-Identifier: BUSL-1.1

// Cross-character combat-event routing pass. Called by engine adapters once per
// physics tick, AFTER every per-character SimulatableBrawler::integrate has run,
// to fan out combat signals — HitFlinch (target-side), GuardFlinch (shooter-side
// on a blocked projectile) — via the brawlerInboundHit::DerivedState slice on
// each character's DerivedState composite (see current_state.md §D1 for the
// off-wire discipline).
//
// Layer: OGBrawler core — engine-agnostic, game-specific. Depends on
// SimulatableBrawler, BrawlerProjectileSimulation, brawlerInboundHit, and the
// engine-agnostic SimulationObjectStorage template. NO UE or Godot symbols.
// The UE adapter (Source/OGBrawlerUnreal/SimulationManagerUImpl) calls this
// from onGameSimulation; a future Godot adapter would call it from the
// equivalent post-integrate hook. Both adapters own their own m_byRootBodyId
// map (populated at engine-timed character registration events) and pass it in.
//
// Deterministic order (D4): iterates attackers in ascending SimulatableBrawler
// id — sorts the storage snapshot internally so engine adapters don't have to
// think about the invariant.

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "OGSimulation/SimulationObjectStorage.h"
#include "OGBrawler/SimulatableBrawler.h"
#include "OGBrawler/BrawlerProjectileSimulation.h"

namespace brawlerHitRouter
{
    // Canonical map type engine adapters use to declare their per-character
    // registration table. Keyed on `SimulatableBrawler`'s root body id
    // (BodyId.value), value = raw pointer into the SimulationObjectStorage
    // (stable across the character's registered lifetime). The map's
    // populate/erase is engine-timed and stays adapter-side; only const
    // access is needed here.
    using RootBodyIdMap = std::unordered_map<uint32_t /* BodyId.value */, SimulatableBrawler*>;

    // The per-tick routing pass. Four branches:
    //   1. Reset every character's inboundHit flags (wasHitThisTick, wasProjectileBlockedThisTick).
    //   2. Radial swing hits (T3): route HitFlinch to struck character (attackHits[]).
    //   3. Projectile damage hits (T3, endReason==2): route HitFlinch to struck character.
    //   4. Projectile guard-blocks (T15, endReason==4): route GuardFlinch to shooter (self-flag).
    //
    // D5 self-hit filter (compare SimulatableBrawler* pointer identity — NOT
    // rootBodyId) is applied on the target-routing branches (2, 3) only.
    // Branch 4 is inherently self-directed and needs no filter.
    inline void routeInboundHitsAll(
        SimulationObjectStorage<SimulatableBrawler>& storage,
        const RootBodyIdMap& byRootBodyId,
        uint32_t currentTick)
    {
        // Deterministic walk order (D4): unordered_map iteration is
        // implementation-defined; sort by ascending id for cross-machine
        // reproducibility. This sort — not the hash-map iteration — is the
        // authoritative per-tick ordering the routing contract depends on.
        std::vector<std::pair<unsigned int, SimulatableBrawler*>> ordered;
        storage.forEachSimulatable([&ordered](unsigned int id, SimulatableBrawler& brawler)
        {
            ordered.emplace_back(id, &brawler);
        });
        std::sort(ordered.begin(), ordered.end(),
            [](const auto& a, const auto& b) { return a.first < b.first; });

        // 1. Reset every character's inbound-signal flags before repopulating this tick.
        //    Both flags are one-shots — the routing pass owns the reset/set lifecycle so
        //    each tick's signal is fresh (avoids the machine sim ever seeing a stale flag).
        for (const auto& [id, brawlerPtr] : ordered)
        {
            auto& slice = brawlerPtr->editAllState().editDerivedState().m_inboundHitDerivedState;
            slice.wasHitThisTick               = false;   // T3
            slice.wasProjectileBlockedThisTick = false;   // T15
        }

        // 2. Radial swing hits — each attacker's post-integrate attackHits[] carries the
        //    stable root body id of every character its weapon overlapped this tick.
        for (const auto& [attackerId, attackerPtr] : ordered)
        {
            const auto& radialDerived =
                attackerPtr->getAllState().getDerivedState().m_attackDerivedState;
            for (const auto& hit : radialDerived.getAttackHits())
            {
                auto found = byRootBodyId.find(hit.hitRootBodyId.value);
                if (found == byRootBodyId.end())
                    continue;
                SimulatableBrawler* target = found->second;
                if (target == attackerPtr)   // D5 self-hit filter (pointer identity, not rootBodyId)
                    continue;
                target->editAllState().editDerivedState()
                    .m_inboundHitDerivedState.wasHitThisTick = true;
            }
        }

        // 3. Projectile damage hits — a slot that ENDED this exact tick with
        //    endReason==2 (hit) routes a one-shot inbound hit to the struck character.
        //    The endTick guard makes it fire once: a hit slot keeps endReason==2 until
        //    it recycles, so without this the target would re-flinch every tick until then.
        for (const auto& [attackerId, attackerPtr] : ordered)
        {
            const auto& projState =
                attackerPtr->getAllState().getState().get<brawlerProjectileSimulation::State>();
            for (const auto& slot : projState.slots)
            {
                if (slot.endTick != currentTick || slot.endReason != 2)
                    continue;
                auto found = byRootBodyId.find(slot.hitRootBodyId.value);
                if (found == byRootBodyId.end())
                    continue;
                SimulatableBrawler* target = found->second;
                if (target == attackerPtr)
                    continue;
                target->editAllState().editDerivedState()
                    .m_inboundHitDerivedState.wasHitThisTick = true;
            }
        }

        // 4. [T15] Shooter-side projectile-blocked routing — any projectile slot that ended
        //    this exact tick with endReason==4 (blockedByGuard, per T14) routes a GuardFlinch
        //    trigger to the slot's OWNING character (the shooter). No map lookup needed —
        //    the shooter IS the attacker whose sim is being iterated; the slot belongs to
        //    their per-character pool. Mirrors the radial swing's attacker-side hasHitGuard
        //    recoil, but decoupled in time from the shoot moment. No self-hit filter check —
        //    self is exactly the right target here.
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
}

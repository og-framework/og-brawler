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
// PRAGMA NOTE (N-1, corrected by item 77 2026-08-17): the backlog previously
// claimed OGBrawler-core headers don't use the debugger-friendliness pragma;
// that was stale. They do — 16 of the 45 files in this directory carry the
// pair (this file is one of the 29 that don't), same as OGSim-core, and as of
// item 77 all three subtrees (og-simulation, og-brawler, the UE adapters)
// share ONE macro pair, OGSIM_OPTIMIZE_OFF/ON, defined in
// OGSimulation/CompilerControl.h — not a per-subtree convention anymore. This
// file specifically has no pair to convert; it simply doesn't wrap its
// routing pass in one.
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
#include "OGBrawler/HitReaction.h"
#include "OGSimulation/OGAssert.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/geometric.hpp"
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"

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
        // (it reads each character's just-produced hitsThisTick[] / projectile slot
        // state), so nothing needs to run before integrateAll. Present to satisfy
        // the four-hook SimulationSystem concept.
        void preIntegrate(const SimulationTimeStep& /*step*/,
                          StorageView<SimulatableBrawler> /*view*/,
                          const simulatableBrawler::StaticData& /*staticData*/)
        {
        }

        // postIntegrate — the per-tick routing pass (T16 logic, relocated). Four
        // branches:
        //   1. Reset every character's whole inboundHit slice — the two one-shot
        //      bools and the resolved reaction beside them are owned here.
        //   2. Radial swing hits (T3): route HitFlinch to the struck character.
        //      [movement-sim task 83] Fires exactly once per hit — the per-TICK
        //      hitsThisTick[], never the per-SWING attackHits[] ledger.
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
                           const simulatableBrawler::StaticData& staticData)
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

            // 1. Reset every character's inbound-signal slice before repopulating
            //    this tick. The routing pass owns the reset/set lifecycle so each
            //    tick's signal is fresh (the machine sim never sees a stale flag).
            //    ⚠ [movement-sim task 27] WHOLE-SLICE, not field-by-field: the slice
            //    now carries the resolved reaction (kind, speed, direction, dwell)
            //    beside the two bools, and a reset that names fields is a reset that
            //    the next field added is silently missing from.
            for (const auto& [id, brawlerPtr] : ordered)
            {
                auto& slice = brawlerPtr->editAllState().editDerivedState()
                    .edit<brawlerInboundHit::DerivedState>();
                slice = brawlerInboundHit::DerivedState{};
            }

            // 2. Radial swing hits — each attacker's post-integrate hitsThisTick[]
            //    carries the stable root body id of every character its weapon
            //    registered a hit on THIS TICK, and the direction the weapon was
            //    travelling through each of those hits.
            //
            // ⭐⭐ [movement-sim task 83] hitsThisTick, NOT attackHits, AND THE
            //    DIFFERENCE IS THE WHOLE OF THE USER'S 12 METRES. attackHits is the
            //    radial sim's per-SWING DEDUP LEDGER: it accumulates for the whole
            //    swing and is cleared only in deactivate(). Iterating it here meant
            //    ONE hit re-fired on EVERY remaining tick of the swing — the target's
            //    velocity was re-assigned at full launch speed with no decay for the
            //    ~0.4 s the swing had left (8.0 m of constant travel, then 5.17 m of
            //    decay = 13.2 m against an authored 5 m), the lockout timer restarted
            //    every tick, a stun re-entered every tick, and the direction was
            //    re-resolved from positions that had MOVED, so the throw curved.
            //    ⛔ The bug was not that the container was wrong; it was that the
            //    per-SWING container was being read as a per-TICK signal. Both still
            //    exist and both are still needed.
            //    ⭐ Branch 3 below already had the right shape — a projectile slot
            //    keeps endReason==2 until it recycles, so it fires on
            //    slot.endTick == currentTick and nowhere else. This is that idiom.
            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                const auto& radialDerived =
                    attackerPtr->getAllState().getDerivedState()
                        .get<dAttackRadialSimulation::DerivedState>();
                if (radialDerived.getHitsThisTick().empty())
                    continue;

                const unsigned int sequenceId =
                    attackerPtr->getAllState().getState()
                        .get<dAttackRadialSimulation::State>().currenSequenceId;
                OG_CHECK(isRealAttackSequence(sequenceId)
                      && sequenceId < staticData.m_hitReactions.size(),
                    "brawlerHitRouting::System::postIntegrate - a radial DAMAGING hit was "
                    "registered while the attacker's wire currenSequenceId is not a row of "
                    "m_hitReactions. The reaction table is indexed by sequence id and the "
                    "constructor asserts it covers m_attackSequences, so this is either a hit "
                    "raised under the Hadouken sentinel (the radial sim early-returns on it and "
                    "must raise none) or a table that stopped matching the sequence list.");
                if (!isRealAttackSequence(sequenceId)
                    || sequenceId >= staticData.m_hitReactions.size())
                    continue;
                const HitReactionSpec& spec = staticData.m_hitReactions[sequenceId];

                for (const auto& hit : radialDerived.getHitsThisTick())
                {
                    auto found = this->m_byRootBodyId.find(hit.hitRootBodyId.value);
                    if (found == this->m_byRootBodyId.end())
                        continue;
                    SimulatableBrawler* target = found->second;
                    if (target == attackerPtr)   // D5 self-hit filter (pointer identity, not rootBodyId)
                        continue;
                    // ⭐ [movement-sim task 83] THE DIRECTION IS THE SWING TANGENT — the way
                    // the weapon was travelling through the hit, which is the user's "orthogonal
                    // to the weapon at the moment of the hit". It is computed at the push site,
                    // where the projection onto the swing plane and the sequence's authored
                    // angular velocity are both already in hand; nothing here re-derives it.
                    // ⛔ The away-from-attacker rule task 27 shipped is now the FALLBACK, and it
                    // is still load-bearing: a swing whose axis is horizontal has a VERTICAL
                    // tangent whose XY projection is degenerate, and normalisedXY would otherwise
                    // hand a NaN straight into a velocity that never leaves the body. Both
                    // arguments are evaluated, which costs two wire reads and buys a rule that
                    // cannot be reached with a half-initialised fallback.
                    resolveHitReaction(
                        staticData, spec,
                        normalisedXY(hit.swingTangent,
                                     directionAwayFromAttacker(*attackerPtr, *target)),
                        target->editAllState().editDerivedState()
                            .edit<brawlerInboundHit::DerivedState>());
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
                    resolveHitReaction(
                        staticData, staticData.m_projectileHitReaction,
                        normalisedXY(slot.spawnDir, glm::vec2(1.f, 0.f)),
                        target->editAllState().editDerivedState()
                            .edit<brawlerInboundHit::DerivedState>());
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
                        .edit<brawlerInboundHit::DerivedState>().wasProjectileBlockedThisTick = true;
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
        static glm::vec2 normalisedXY(const glm::vec3& v, const glm::vec2& fallback)
        {
            const glm::vec2 xy(v.x, v.y);
            const float lengthSq = glm::dot(xy, xy);
            return lengthSq > 0.f ? xy * (1.f / glm::sqrt(lengthSq)) : fallback;
        }

        static glm::vec2 attackerAimXY(const SimulatableBrawler& attacker)
        {
            const dAttackRadialSimulation::InitialConditions& ic =
                attacker.getAllState().getState()
                    .get<dAttackRadialSimulation::InitialConditions>();
            const glm::vec3 axis =
                (glm::dot(ic.initialAimRotationAxis, ic.initialAimRotationAxis) > 0.f)
                    ? ic.initialAimRotationAxis
                    : glm::vec3(0.f, 0.f, 1.f);
            const glm::vec3 aim = glm::vec3(
                glm::rotate(glm::mat4(1.f), ic.initialAimAngle, axis)
                    * glm::vec4(1.f, 0.f, 0.f, 0.f));
            return normalisedXY(aim, glm::vec2(1.f, 0.f));
        }

        static glm::vec2 directionAwayFromAttacker(const SimulatableBrawler& attacker,
                                                   const SimulatableBrawler& target)
        {
            const glm::vec3 attackerPosition =
                attacker.getAllState().getState()
                    .get<brawlerMovementSimulation::State>().bodyState.position;
            const glm::vec3 targetPosition =
                target.getAllState().getState()
                    .get<brawlerMovementSimulation::State>().bodyState.position;
            return normalisedXY(targetPosition - attackerPosition, attackerAimXY(attacker));
        }

        static void resolveHitReaction(const simulatableBrawler::StaticData& staticData,
                                       const HitReactionSpec& spec,
                                       const glm::vec2& directionXY,
                                       brawlerInboundHit::DerivedState& slice)
        {
            OG_CHECK(staticData.m_movementStaticData.launchDecel > 0.f,
                "brawlerHitRouting::System::resolveHitReaction - launchDecel must be POSITIVE. "
                "A knockback's lockout is knockbackSpeed / launchDecel, so a zero decel is an "
                "infinite slide paired with a zero-length lockout - the character would be sliding "
                "and free to attack on the same tick, which is the exact window the lockout "
                "exists to close.");

            const bool knockback = spec.kind == HitReactionKind::Knockback;
            slice.wasHitThisTick = true;
            slice.reactionKind   = spec.kind;
            slice.knockbackSpeed = knockback ? spec.knockbackSpeed : 0.f;
            slice.hitDirectionXY = knockback ? directionXY : glm::vec2(0.f);
            slice.flinchDuration = knockback
                ? glm::max(spec.lockoutDuration,
                           spec.knockbackSpeed / staticData.m_movementStaticData.launchDecel)
                : spec.lockoutDuration;
        }

        // Actor-level root-body-id -> registered brawler, for cross-character
        // routing. Key = the character capsule's BodyId.value
        // (CharacterBindings::capsuleBodyId); value = raw pointer into storage's
        // unique_ptr (stable across the character's registered lifetime). Moved
        // out of the engine adapter (ASimulationManagerUImpl::m_byRootBodyId) so
        // the routing system owns its own bookkeeping (§3.9 / D8).
        std::unordered_map<uint32_t /* BodyId.value */, SimulatableBrawler*> m_byRootBodyId;
    };
} // namespace brawlerHitRouting

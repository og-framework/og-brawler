#pragma once
// SPDX-License-Identifier: BUSL-1.1

// brawlerHitRouting::System — the OGSim-system-api "system" form of the
// cross-character combat-event routing pass (formerly a free function in an
// engine-adapter-owned routing wrapper, since removed). A system is a
// cross-simulatable coordinator: it observes
// the whole SimulatableBrawler population once per tick — in preIntegrate of
// T+1, a pre-integrate reduction over the previous tick's end state (T's
// integrate and physics step have run, T+1's integrate has not) — and fans out
// combat signals (target-side HitFlinch, shooter-side GuardFlinch on a blocked
// projectile) via the brawlerInboundHit::DerivedState slice on each
// character's DerivedState composite (see current_state.md §D1 for the
// off-wire discipline). The slice is produced and consumed inside T+1, so a
// resim replay of T+1 recomputes it from the restored end-of-T state
// [og-netcode-v2-field-defects task 20; it was a post-integrate pass of T,
// consumed by T+1, which a replay starting at T+1 never re-ran].
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
#include "OGSimulation/SystemRoleAffinity.h"    // SystemRoleAffinity
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
    // [og-netcode-v2-field-defects task 17] No tick arithmetic lives in this system. Task 20
    // Rework (1) kept a per-step-kind tick offset here so branches 3 and 4 could match a
    // projectile slot's endTick against the tick the timeline integrated last; the user ruled
    // that a system must not need to know which tick ran last. Branches 3 and 4 now read the
    // projectile outcome brawlerHitDetection::System produced in this same pass, exactly as
    // branch 5 reads guardBlockedThisTick, so every step kind (Normal, Skip, Stall, HardResync,
    // a resim replay) routes what this pass detected and nothing else.

    // The hit-routing system. Owns the actor-level root-body-id -> registered
    // brawler map (moved here from the engine adapter): the per-character routing
    // table the preIntegrate pass keys inbound hits against. Populate/erase is
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

        static constexpr SystemRoleAffinity kRoleAffinity = SystemRoleAffinity::AllRoles;

        // postIntegrate — no work. Routing is a PRE-integrate reduction over the
        // previous tick's end state (see preIntegrate). Present to satisfy the
        // four-hook SimulationSystem concept.
        void postIntegrate(const SimulationTimeStep& /*step*/,
                           StorageView<SimulatableBrawler> /*view*/,
                           const simulatableBrawler::StaticData& /*staticData*/)
        {
        }

        // preIntegrate — the per-tick routing pass (T16 logic, relocated; moved from
        // postIntegrate of T to preIntegrate of T+1 by og-netcode-v2-field-defects
        // task 20). It routes what brawlerHitDetection::System detected earlier in THIS
        // pass onto the slice that this step's integrate reads. Five branches:
        //   1. Reset every character's whole inboundHit slice — the two one-shot
        //      bools and the resolved reaction beside them are owned here.
        //   2. Radial swing hits (T3): route HitFlinch to the struck character.
        //      [movement-sim task 83] Fires exactly once per hit — the per-TICK
        //      hitsThisTick[], never the per-SWING hit ledger.
        //   3. Projectile damage hits (T3; SlotOutcome::Hit): route HitFlinch to the
        //      struck character. Fires once: the detector resets the outcome every
        //      pass, and the shooter's integrate ends the slot in the same step.
        //      [movement-sim task 88] The spec is m_projectileHitReaction (Stun), or
        //      m_projectileHitOnFlinchReaction (Knockback) when the target is in HitFlinch.
        //   4. Projectile guard-blocks (T15; SlotOutcome::BlockedByGuard): route
        //      GuardFlinch to the shooter (self-flag; no map lookup).
        //   5. Radial guard-blocks (og-netcode-v2-field-defects task 9): route
        //      GuardFlinch to the attacker whose swing was blocked in the produced tick
        //      (self-flag; no map lookup). The block itself is DETECTED by
        //      brawlerHitDetection::System, which fires before this system.
        //
        // D5 self-hit filter (SimulatableBrawler* pointer identity — NOT
        // rootBodyId) applies on the target-routing branches (2, 3) only; branches 4
        // and 5 are inherently self-directed.
        //
        // [og-netcode-v2-field-defects task 9] Branch 2 reads hitsThisTick, which is
        // written by brawlerHitDetection::System (BrawlerHitDetectionSystem.h) in the SAME
        // pass, before this system — firing order is template order in
        // SimulationSystemsExecutor, and the manager's BrawlerSystemsExec lists detection
        // first. [task 20] Both run in preIntegrate(T+1) over the end state of T, so the
        // machine still reacts on T+1 — no added latency — and the signal never crosses a
        // tick boundary off the wire. [task 17] Branches 3 and 4 read the projectile
        // DerivedState's detectedThisTick, written by the same detector in the same pass at
        // the slot's closed-form position on THIS step's tick, so the reaction lands in this
        // step's integrate: the tick the shot reaches the target.
        void preIntegrate(const SimulationTimeStep& /*step*/,
                          StorageView<SimulatableBrawler> view,
                          const simulatableBrawler::StaticData& staticData)
        {
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

            // 2. Radial swing hits — each attacker's hitsThisTick[] (written by the
            //    detector earlier in this pass) carries the stable root body id of
            //    every character its weapon registered a hit on in the PRODUCED
            //    tick, and the direction the weapon was
            //    travelling through each of those hits.
            //
            // ⭐⭐ [movement-sim task 83] hitsThisTick, NOT THE PER-SWING LEDGER, AND
            //    THE DIFFERENCE IS THE WHOLE OF THE USER'S 12 METRES. The ledger (a
            //    derived vector then; the synced State::hitTargets of target ids since
            //    og-netcode-v2-field-defects task 27) accumulates for the whole swing
            //    and is cleared only in deactivate(). Iterating it here meant
            //    ONE hit re-fired on EVERY remaining tick of the swing — the target's
            //    velocity was re-assigned at full launch speed with no decay for the
            //    ~0.4 s the swing had left (8.0 m of constant travel, then 5.17 m of
            //    decay = 13.2 m against an authored 5 m), the lockout timer restarted
            //    every tick, a stun re-entered every tick, and the direction was
            //    re-resolved from positions that had MOVED, so the throw curved.
            //    ⛔ The bug was not that the container was wrong; it was that the
            //    per-SWING container was being read as a per-TICK signal. Both still
            //    exist and both are still needed.
            //    ⭐ Branch 3 below has the same shape since og-netcode-v2-field-defects
            //    task 17: a per-pass outcome the detector resets, never the slot's
            //    persistent endReason.
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
                    "brawlerHitRouting::System::preIntegrate - a radial DAMAGING hit was "
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

            // 3. Projectile damage hits — a slot the detector found HIT in this pass
            //    routes a one-shot inbound hit to the struck character. One-shot
            //    because the detector resets detectedThisTick at the top of every pass
            //    and the shooter's integrate ends the slot in this same step.
            //    [og-netcode-v2-field-defects task 17] It used to match the wire slot's
            //    endTick (and endReason 2) against the tick integrated last.
            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                const auto& projState =
                    attackerPtr->getAllState().getState().get<brawlerProjectileSimulation::State>();
                const auto& projDetected =
                    attackerPtr->getAllState().getDerivedState()
                        .get<brawlerProjectileSimulation::DerivedState>().detectedThisTick;
                for (std::size_t slotIndex = 0; slotIndex < projDetected.size(); ++slotIndex)
                {
                    const auto& detected = projDetected[slotIndex];
                    if (detected.outcome != brawlerProjectileSimulation::SlotOutcome::Hit)
                        continue;
                    const auto& slot = projState.slots[slotIndex];
                    auto found = this->m_byRootBodyId.find(detected.struckRootBodyId.value);
                    if (found == this->m_byRootBodyId.end())
                        continue;
                    SimulatableBrawler* target = found->second;
                    if (target == attackerPtr)
                        continue;
                    // [movement-sim task 88] A target ALREADY in HitFlinch (either reaction
                    // kind, user ruling 2026-09-21) is LAUNCHED rather than re-stunned; the
                    // machine's cross-kind rules (design_hit_reactions.md §3) do the rest.
                    // ⛔ m_currentState ONLY: GuardFlinch is a separate enumerator, so a
                    // guard-flinching target still takes the authored Stun.
                    // ⭐ Read in THIS pass, before any integrate: a melee hit routed in the same
                    // pass has not reached the target's machine yet, so a same-pass projectile
                    // still sees the pre-hit state and stays a Stun (pinned in
                    // HitRouting.ProjectileOnFlinchSameTickStaysAStun). Both inputs are on the
                    // wire, so a resim reproduces the choice.
                    const bool targetFlinching =
                        target->getAllState().getState()
                            .get<dAttackMachineSimulation::State>().m_currentState
                        == DAttackState::HitFlinch;
                    // The direction below is the projectile's travel direction; a Stun
                    // discards it, a Knockback launches along it.
                    resolveHitReaction(
                        staticData,
                        targetFlinching ? staticData.m_projectileHitOnFlinchReaction
                                        : staticData.m_projectileHitReaction,
                        normalisedXY(slot.spawnDir, glm::vec2(1.f, 0.f)),
                        target->editAllState().editDerivedState()
                            .edit<brawlerInboundHit::DerivedState>());
                }
            }

            // 4. [T15] Shooter-side projectile-blocked routing — any projectile
            //    slot the detector found BLOCKED by a guard in this pass (T14's
            //    blockedByGuard) routes a GuardFlinch trigger to the slot's OWNING
            //    character (the shooter). No map lookup — the shooter IS the
            //    attacker whose sim is being iterated. No self-hit filter: self is
            //    exactly the right target here.
            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                const auto& projDetected =
                    attackerPtr->getAllState().getDerivedState()
                        .get<brawlerProjectileSimulation::DerivedState>().detectedThisTick;
                for (const auto& detected : projDetected)
                {
                    if (detected.outcome != brawlerProjectileSimulation::SlotOutcome::BlockedByGuard)
                        continue;
                    attackerPtr->editAllState().editDerivedState()
                        .edit<brawlerInboundHit::DerivedState>().wasProjectileBlockedThisTick = true;
                }
            }

            // 5. [og-netcode-v2-field-defects task 9] Radial guard-blocks — an attacker whose
            //    swing brawlerHitDetection::System found blocked by a guard in the produced tick (the
            //    radial DerivedState's per-tick guardBlockedThisTick) gets GuardFlinch routed
            //    to itself, exactly as branch 4 does for a blocked projectile. Self-directed,
            //    so no map lookup and no self-hit filter. The flag it copies is derived and
            //    is recomputed on every replayed tick — including the first one after a
            //    restore, since task 20 put this pass inside the consuming tick; the radial
            //    State's hasHitGuard, which this replaces, rode the wire.
            //    ⛔ It must be COPIED here, not written onto the slice by the detector: branch
            //    1 above resets the whole slice every tick, and detection fires before routing,
            //    so a bit the detector set on the slice would be wiped before anyone read it.
            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                if (attackerPtr->getAllState().getDerivedState()
                        .get<dAttackRadialSimulation::DerivedState>().getGuardBlockedThisTick())
                {
                    attackerPtr->editAllState().editDerivedState()
                        .edit<brawlerInboundHit::DerivedState>().wasGuardBlockedThisTick = true;
                }
            }
        }

        // onCharacterRegistered — index the just-registered character for routing.
        // §3.11 timing: the character IS already in storage when this fires, so the
        // view resolves it. Key = the character's CAPSULE (root) body id — the
        // value the query adapter emits as SpatialQueryHit::rootBodyId for hits on
        // ANY of the character's shapes (hurtbox or guard), hence carried by radial
        // hitsThisTick[].hitRootBodyId / the projectile DerivedState's
        // detectedThisTick[].struckRootBodyId. Value = the
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

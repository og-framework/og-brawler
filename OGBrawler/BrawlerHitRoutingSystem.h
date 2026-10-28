#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerHitRoutingSystem-rationale.md · docs/BrawlerHitRoutingSystem-guards.md

#include <algorithm>
#include <cstdint>
#include <unordered_map>
#include <utility>
#include <vector>

#include "OGSimulation/SimulatableList.h"
#include "OGSimulation/StorageView.h"
#include "OGSimulation/SimulationTimeContext.h"
#include "OGSimulation/SystemRoleAffinity.h"
#include "OGBrawler/SimulatableBrawler.h"
#include "OGBrawler/BrawlerProjectileSimulation.h"
#include "OGBrawler/HitReaction.h"
#include "OGSimulation/OGAssert.h"
#include "OGSimulation/SimulationFrame.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "glm/geometric.hpp"
#include "glm/common.hpp"
#include "glm/gtc/matrix_transform.hpp"

namespace brawlerHitRouting
{
    class System
    {
    public:
        using RequiredSimulatables = SimulatableList<SimulatableBrawler>;

        static constexpr SystemRoleAffinity kRoleAffinity = SystemRoleAffinity::AllRoles;

        void postIntegrate(const SimulationTimeStep& /*step*/,
                           StorageView<SimulatableBrawler> /*view*/,
                           const simulatableBrawler::StaticData& /*staticData*/)
        {
        }

        // ⛔G-01  docs/BrawlerHitRoutingSystem-guards.md
        void preIntegrate(const SimulationTimeStep& /*step*/,
                          StorageView<SimulatableBrawler> view,
                          const simulatableBrawler::StaticData& staticData)
        {
            std::vector<std::pair<unsigned int, SimulatableBrawler*>> ordered;
            view.forEachSimulatable<SimulatableBrawler>(
                [&ordered](unsigned int id, SimulatableBrawler& brawler)
                {
                    ordered.emplace_back(id, &brawler);
                });
            // ⛔G-02  docs/BrawlerHitRoutingSystem-guards.md
            std::sort(ordered.begin(), ordered.end(),
                [](const auto& a, const auto& b) { return a.first < b.first; });

            for (const auto& [id, brawlerPtr] : ordered)
            {
                auto& slice = brawlerPtr->editAllState().editDerivedState()
                    .edit<brawlerInboundHit::DerivedState>();
                // ⛔G-03  docs/BrawlerHitRoutingSystem-guards.md
                slice = brawlerInboundHit::DerivedState{};
            }

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

                // ⛔G-04  docs/BrawlerHitRoutingSystem-guards.md
                for (const auto& hit : radialDerived.getHitsThisTick())
                {
                    auto found = this->m_byRootBodyId.find(hit.hitRootBodyId.value);
                    if (found == this->m_byRootBodyId.end())
                        continue;
                    SimulatableBrawler* target = found->second;
                    if (target == attackerPtr)
                        continue;
                    // ⛔G-05  docs/BrawlerHitRoutingSystem-guards.md
                    resolveHitReaction(
                        staticData, spec,
                        normalisedXY(hit.swingTangent,
                                     directionAwayFromAttacker(*attackerPtr, *target)),
                        meleeDirectionWorld(spec, hit.swingTangent, *attackerPtr, *target),
                        target->editAllState().editDerivedState()
                            .edit<brawlerInboundHit::DerivedState>());
                }
            }

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
                    // ⛔G-06  docs/BrawlerHitRoutingSystem-guards.md
                    const bool targetFlinching =
                        target->getAllState().getState()
                            .get<dAttackMachineSimulation::State>().m_currentState
                        == DAttackState::HitFlinch;
                    resolveHitReaction(
                        staticData,
                        targetFlinching ? staticData.m_projectileHitOnFlinchReaction
                                        : staticData.m_projectileHitReaction,
                        normalisedXY(slot.spawnDir, glm::vec2(1.f, 0.f)),
                        projectileDirectionWorld(slot.spawnDir),
                        target->editAllState().editDerivedState()
                            .edit<brawlerInboundHit::DerivedState>());
                }
            }

            for (const auto& [attackerId, attackerPtr] : ordered)
            {
                if (attackerPtr->getAllState().getDerivedState()
                        .get<dAttackRadialSimulation::DerivedState>().getGuardBlockedThisTick())
                {
                    // ⛔G-07  docs/BrawlerHitRoutingSystem-guards.md
                    attackerPtr->editAllState().editDerivedState()
                        .edit<brawlerInboundHit::DerivedState>().wasGuardBlockedThisTick = true;
                }
            }
        }

        void onCharacterRegistered(unsigned int id,
                                   StorageView<SimulatableBrawler> view,
                                   const simulatableBrawler::StaticData& /*staticData*/)
        {
            SimulatableBrawler& brawler = view.get<SimulatableBrawler>(id);
            const uint32_t rootBodyId = brawler.getCharacterBindings().capsuleBodyId.value;
            this->m_byRootBodyId[rootBodyId] = &brawler;
        }

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
            const glm::vec2 xy = simFrame::planar(v);
            const float lengthSq = glm::dot(xy, xy);
            return lengthSq > 0.f ? xy * (1.f / glm::sqrt(lengthSq)) : fallback;
        }

        static glm::mat4 attackerAimRotation(const SimulatableBrawler& attacker)
        {
            const dAttackRadialSimulation::InitialConditions& ic =
                attacker.getAllState().getState()
                    .get<dAttackRadialSimulation::InitialConditions>();
            constexpr glm::vec3 aimFallbackAxis = simFrame::withHeight(glm::vec3(0.f), -1.f);
            static_assert(simFrameDetail::sameBits(aimFallbackAxis, glm::vec3(0.f, -1.f, 0.f)),
                "aimFallbackAxis must stay (0,-1,0) with +0 zeros: it is the machine's near-pole fallback axis, used here when the stored initialAimRotationAxis is zero. "
                "Not simFrame::kTurnAxis or kUp: they are +Y since og-sim-metres-yup task 26, and rotate(a, +Y) is rotate(-a, -Y), a mirrored fallback aim. "
                "Not -simFrame::kUp or kUp * -1.f: their -0 zeros differ from the machine's axis. "
                "Was the chart-P simFrame::kTurnAxis.");
            const glm::vec3 axis =
                (glm::dot(ic.initialAimRotationAxis, ic.initialAimRotationAxis) > 0.f)
                    ? ic.initialAimRotationAxis
                    : aimFallbackAxis;
            return glm::rotate(glm::mat4(1.f), ic.initialAimAngle, axis);
        }

        static glm::vec2 attackerAimXY(const SimulatableBrawler& attacker)
        {
            constexpr glm::vec4 forward4(simFrame::kForward, 0.f);
            const glm::vec3 aim = glm::vec3(attackerAimRotation(attacker) * forward4);
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

        static glm::vec3 meleeDirectionWorld(const HitReactionSpec& spec,
                                             const glm::vec3& swingTangent,
                                             const SimulatableBrawler& attacker,
                                             const SimulatableBrawler& target)
        {
            const bool authored = spec.directionSource == HitDirectionSource::AuthoredInSwingFrame;
            const bool authoredUsable =
                glm::dot(spec.authoredDirection, spec.authoredDirection) > 0.f;
            OG_CHECK(!authored || authoredUsable,
                "brawlerHitRouting::System::meleeDirectionWorld - a row whose directionSource is "
                "AuthoredInSwingFrame must author a NON-ZERO authoredDirection. Routing normalises "
                "it, so a zero vector is a NaN knockback direction; the row falls back to the "
                "swing tangent instead.");
            if (authored && authoredUsable)
                return glm::normalize(glm::mat3(attackerAimRotation(attacker)) * spec.authoredDirection);
            const glm::vec2 xy = simFrame::planar(swingTangent);
            // ⛔G-08  docs/BrawlerHitRoutingSystem-guards.md
            return glm::dot(xy, xy) > 0.f
                ? glm::normalize(swingTangent)
                : simFrame::compose(directionAwayFromAttacker(attacker, target), 0.f);
        }

        static glm::vec3 projectileDirectionWorld(const glm::vec3& spawnDir)
        {
            return glm::dot(spawnDir, spawnDir) > 0.f
                ? glm::normalize(spawnDir)
                : simFrame::compose(glm::vec2(1.f, 0.f), 0.f);
        }

        static void resolveHitReaction(const simulatableBrawler::StaticData& staticData,
                                       const HitReactionSpec& spec,
                                       const glm::vec2& directionXY,
                                       const glm::vec3& directionWorld,
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
            slice.hitDirectionWorld = knockback ? directionWorld : glm::vec3(0.f);
            slice.flinchDuration = knockback
                ? glm::max(spec.lockoutDuration,
                           spec.knockbackSpeed / staticData.m_movementStaticData.launchDecel)
                : spec.lockoutDuration;
        }

        std::unordered_map<uint32_t, SimulatableBrawler*> m_byRootBodyId;
    };
} // namespace brawlerHitRouting

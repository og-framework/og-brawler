#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "glm/vec2.hpp"
#include "OGBrawler/HitReaction.h"

namespace brawlerInboundHit
{
    // Per-character inbound-signal slice for cross-character combat events.
    // Populated once per tick by brawlerHitRouting::System::preIntegrate (T3, T15,
    // og-netcode-v2-field-defects task 9) from what brawlerHitDetection::System
    // found earlier in the same pass (the previous tick's end state; for a
    // projectile, its closed-form position on this tick, task 17), and
    // consumed in the SAME tick by dAttackMachineSimulation::integrate3 as a plain
    // by-ref parameter (NOT an ExternalDep — see current_state.md §D7). Produced
    // and consumed inside one tick (task 20), so a resim replay recomputes it on
    // its first replayed tick from the restored state; nothing about it crosses a
    // tick boundary off the wire.
    //
    // Three signals, one slice — same routing shape, different transitions:
    //
    //   wasHitThisTick               (T3)  — this character was struck by an
    //                                        opposing attacker's damaging hit
    //                                        (radial hitsThisTick[], or a projectile
    //                                        slot the detector found Hit this
    //                                        step, og-netcode-v2-field-defects
    //                                        task 17). Drives the
    //                                        Idle/Attacking/GuardFlinch ->
    //                                        HitFlinch transition.
    //   wasProjectileBlockedThisTick (T15) — this character owns a projectile
    //                                        slot the detector found blocked by a
    //                                        guard this step (BlockedByGuard; the
    //                                        slot ends with endReason=4).
    //                                        Drives any-state -> GuardFlinch,
    //                                        mirroring the radial swing's
    //                                        attacker-side guard-block recoil
    //                                        (wasGuardBlockedThisTick below).
    //                                        Fires from Idle too, because a
    //                                        projectile can be blocked long
    //                                        after the shooter's Hadouken
    //                                        commitment window has expired.
    //   wasGuardBlockedThisTick (og-netcode-v2-field-defects task 9)
    //                                      — this character's radial swing was
    //                                        blocked by another character's
    //                                        guard in the previous tick. Set by routing
    //                                        from the radial DerivedState's
    //                                        guardBlockedThisTick, which
    //                                        brawlerHitDetection::System wrote.
    //                                        Drives Attacking -> GuardFlinch
    //                                        only (the Attacking case reads it).
    //                                        Replaces the radial State's
    //                                        hasHitGuard, which rode the wire.
    //
    // Lives on simulatableBrawler::DerivedState (NOT serialized State) per
    // architectural decision D1 — see current_state.md §D1 for why. Follows the
    // same shape convention as dAttackGuardSimulation::DerivedState (an
    // intentionally-empty class kept for structural symmetry); this one is just
    // non-empty. Off-wire: no SerializableFields specialization, zero bytes on
    // the FSimulationStateSyncBuffer, no kWireFormatVersion bump.
    class DerivedState
    {
    public:
        bool wasHitThisTick               = false;   // T3
        bool wasProjectileBlockedThisTick = false;   // T15
        bool wasGuardBlockedThisTick      = false;   // og-netcode-v2-field-defects task 9

        HitReactionKind reactionKind   = HitReactionKind::Stun;
        float           knockbackSpeed = 0.f;
        float           flinchDuration = 0.f;
        glm::vec2       hitDirectionXY{0.f};
    };
}

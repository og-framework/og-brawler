#pragma once
// SPDX-License-Identifier: BUSL-1.1

namespace brawlerInboundHit
{
    // Per-character inbound-signal slice for cross-character combat events.
    // Populated once per tick by SimulationManagerUImpl's post-integrate routing
    // pass (T3, T15); consumed the following tick by dAttackMachineSimulation::
    // integrate3 as a plain by-ref parameter (NOT an ExternalDep — see
    // current_state.md §D7).
    //
    // Two signals, one slice — same routing shape, different transitions:
    //
    //   wasHitThisTick               (T3)  — this character was struck by an
    //                                        opposing attacker's damaging hit
    //                                        (radial attackHits[] or projectile
    //                                        slot with endReason=2). Drives the
    //                                        Idle/Attacking/GuardFlinch ->
    //                                        HitFlinch transition.
    //   wasProjectileBlockedThisTick (T15) — this character owns a projectile
    //                                        slot that just ended with
    //                                        endReason=4 (blockedByGuard).
    //                                        Drives any-state -> GuardFlinch,
    //                                        mirroring the radial swing's
    //                                        attacker-side hasHitGuard recoil.
    //                                        Fires from Idle too, because a
    //                                        projectile can be blocked long
    //                                        after the shooter's Hadouken
    //                                        commitment window has expired.
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
    };
}

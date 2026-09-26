<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerInboundHit.h` — rationale

<!-- lint-external-ref: hasHitGuard -- RETIRED by og-netcode-v2-field-defects task 9: the radial State's wire flag wasGuardBlockedThisTick replaced. It must NOT resolve -->

What the inbound-hit slice is, who writes and reads each field, and why it is shaped as it is. The
header has **no guards file**: every prohibition its comments carried is held by the compiler (§3),
and the rest is below. Routing's own prohibitions, including the whole-slice reset, are in
`BrawlerHitRoutingSystem-guards.md`.

**Provenance.** Converted by og-netcode-v2-field-defects task 29 (2026-09-26). The header, as
committed at og-brawler `967a958`, was 72 lines, md5 `e7c30776…`, and carried 54 comment lines in 4
runs (the SPDX line aside): one 51-line block and three trailing labels. They are kept VERBATIM in
[§A](#a-the-removed-comments-verbatim). **Where a section and §A disagree, the section wins.**

⚠ **This file is not the source of truth for any VALUE.** Where it quotes a number, the code and the
tests win.

---

## 1. What the slice is

`brawlerInboundHit::DerivedState` is each character's per-tick inbound combat signal: the hits and
blocks other characters' actions caused it this tick, and the reaction resolved for a hit. It is an
element of the off-wire `simulatableBrawler::DerivedState` composite.

`brawlerHitRouting::System::preIntegrate` resets and fills it once per tick (hit-resolution T3 and
T15, og-netcode-v2-field-defects task 9), from what `brawlerHitDetection::System` found earlier in the
same pass: for melee, in the end state the previous tick's `integrate` left; for a projectile, at its
closed-form position on this tick (og-netcode-v2-field-defects task 17). It is consumed in the SAME
tick by that character's `integrate`. So it is produced and consumed inside one tick
(og-netcode-v2-field-defects task 20), a resim replay recomputes it on its first replayed tick from
the restored state, and nothing about it crosses a tick boundary off the wire.

It follows the shape of the sub-simulations' own `DerivedState` classes (`dAttackGuardSimulation::DerivedState`
is an intentionally empty one, kept for symmetry); this one is not empty. It is the one element of
the derived composite that is not a sub-simulation's own scratch: routing, a system, owns its
lifecycle. That is why `integrate3` takes it as a plain by-reference parameter rather than through its
`Dependencies` (§3).

---

## 2. The fields

| field | set by (routing branch, `BrawlerHitRoutingSystem-rationale.md` §3) | read by | effect |
|---|---|---|---|
| `wasHitThisTick` (T3) | 2 (a radial hit in `hitsThisTick`) or 3 (a projectile slot the detector found `Hit`) | `dAttackMachineSimulation::integrate3`; `brawlerMovementSimulation::integrate` | machine: any state → `HitFlinch`, taking the reaction kind and dwell. Movement: launches at `hitDirectionXY * knockbackSpeed` |
| `wasProjectileBlockedThisTick` (T15) | 4 (a slot this character owns, found `BlockedByGuard`; `integrate` ends it with `endReason` 4) | `integrate3` | any state except `GuardFlinch` → `GuardFlinch` (`DAttackMachineSimulation-guards.md` G-04). It fires from `Idle` too, because a projectile can be blocked long after the shooter's Hadouken commitment has ended. Mirrors the melee recoil below |
| `wasGuardBlockedThisTick` (og-netcode-v2-field-defects task 9) | 5 (copied from the radial `DerivedState`'s `guardBlockedThisTick`, which the detector wrote: this character's swing was blocked by another's guard in the state the previous tick left) | `integrate3`, `Attacking` case only | `Attacking` → `GuardFlinch`. Replaces the radial `State`'s `hasHitGuard`, which rode the wire |
| `reactionKind`, `knockbackSpeed`, `flinchDuration`, `hitDirectionXY` (movement-sim task 27) | 2 and 3, through `resolveHitReaction` (`BrawlerHitRoutingSystem-rationale.md` §4) | `integrate3` (kind, dwell); movement (speed, direction) | the whole reaction, resolved where both the attack table and `launchDecel` are in hand |

The three bools are one-shot: routing's branch 1 resets the whole slice to its value-initialised
state every pass (`BrawlerHitRoutingSystem-guards.md` G-03), and every default above means "no
signal".

⚠ When a hit and a projectile block land on one tick, `integrate3` applies both in order: `HitFlinch`,
then `GuardFlinch` (`DAttackMachineSimulation-guards.md` G-04, a task-18 finding). When two hits reach
one target in one pass, the reaction fields hold the one routed last
(`BrawlerHitRoutingSystem-rationale.md` §6, observed and awaiting a ruling).

---

## 3. Held by the compiler

Measured by compiling (task 29 unless noted), under the standalone flags and the `OGBrawlerTests`
LLT flags:

| the pristine's rule | what holds it | the edit, and the error |
|---|---|---|
| off-wire: "no SerializableFields specialization, zero bytes on the FSimulationStateSyncBuffer" | `SimulationDerivedComposite`'s `requires (!(Serializable<Ts> \|\| ...))` (og-simulation, movement-sim task 23), plus `STATIC_REQUIRE_FALSE(Serializable<brawlerInboundHit::DerivedState>)` in two brawler test files | add a `SerializableFields` specialization → `C7602` at the `DerivedState` alias in `SimulatableBrawlerTypes.h` |
| "Lives on simulatableBrawler::DerivedState (NOT serialized State)" (its decision D1) | the `State` composite's `isSimilarTo` and serialization helpers require every element to be `Serializable` | move the slice onto `simulatableBrawler::State` → `C7500` at the correction gate's `isSimilarTo`, which every production build instantiates. ⚠ The alias itself still compiles; the error is at the first use |
| "NOT an ExternalDep" (its decision D7) | `ExternalDeps` resolves out of the serialized `State` composite | add it to `dAttackMachineSimulation`'s `Dependencies::External` → `C2338` "UNOWNED EXTERNAL REF" (measured by task 18, `DAttackMachineSimulation-rationale.md` §1) |

"No kWireFormatVersion bump" was history: the slice never took wire bytes, so nothing about it ever
bumped the version.

---

## 4. R0 — what task 29 found false in this header, and what is true

| # | where (pristine) | the claim | what is true |
|---|---|---|---|
| 1 | 14-15 | the slice is "consumed in the SAME tick by dAttackMachineSimulation::integrate3" | also by `brawlerMovementSimulation::integrate`, which reads `wasHitThisTick`, `hitDirectionXY` and `knockbackSpeed` (§2) |
| 2 | 15, 54-55 | the reasons are at "current_state.md §D7" and "§D1" | an initiative workspace file outside this repository; both reasons are compile errors (§3) |
| 3 | 20 | "Three signals, one slice" | three signals and the resolved reaction, four more fields, since movement-sim task 27 (§2) |
| 4 | 28-29 | `wasHitThisTick` "Drives the Idle/Attacking/GuardFlinch -> HitFlinch transition" | from EVERY state, `HitFlinch` included: a re-hit restarts the dwell (§2) |
| 5 | 34 | `wasProjectileBlockedThisTick` "Drives any-state -> GuardFlinch" | every state except `GuardFlinch` (§2) |

**Checked and true:** routing fills it in `preIntegrate` from what the detector found earlier in the
pass; the projectile's closed-form position on this tick; same-tick consumption and replay
recomputation; `endReason` 4 on a blocked slot; the `Attacking`-only melee recoil; `hasHitGuard` having
ridden the wire; `dAttackGuardSimulation::DerivedState` being empty; `integrate3` taking the slice by
reference.

---

## §A The removed comments, VERBATIM

Each run is the shipped bytes of the header at md5 `e7c30776…`, with the section its content now
lives in. ⚠ Several are false: read the section, not the archive (§4).

<!-- pristine lines 9-59 --> → §1, §2 and §3 (corrected: §4)
```
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
```

<!-- pristine lines 63-63 --> → §2 (the provenance in the field column)
```
        bool wasHitThisTick               = false;   // T3
```

<!-- pristine lines 64-64 --> → §2 (the provenance in the field column)
```
        bool wasProjectileBlockedThisTick = false;   // T15
```

<!-- pristine lines 65-65 --> → §2 (the provenance in the field column)
```
        bool wasGuardBlockedThisTick      = false;   // og-netcode-v2-field-defects task 9
```

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerHitRoutingSystem.h` — guards

Every prohibition for the hit-routing system that the compiler does not already hold. Each entry
has an **opaque, stable id**. In the header, a single line `// ⛔G-nn` sits exactly where the wrong
edit would be typed.

**If this file and `BrawlerHitRoutingSystem.h` disagree, the header is authoritative and this file
is stale.** Fix this file. Do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or run-time
check, moves to [§R, Retired ids](#r-retired-ids) and its number is spent forever.

⛔ **Nothing in this file is a rationale.** The law, the five branches, the provenance and every
removed comment, verbatim, live in `BrawlerHitRoutingSystem-rationale.md`. "Branch N" in this file
is the numbering that document's §3 defines.

**Provenance.** Converted by og-netcode-v2-field-defects task 29 (2026-09-26) from the header as
committed at og-brawler `967a958` (442 lines, md5 `db7ce95d…`). Quoted fence text is the shipped
bytes of that header, cited as `<!-- pristine lines A-B -->`. The full comment archive is rationale
§A.

**Not here, because the compiler holds them** (rationale §2, each measured by compiling): the
four-hook `SimulationSystem` concept, detection firing before routing, and the inbound slice staying
off the wire. Branch 2's literal substitution of the ledger for the per-tick signal does not compile
either, but a rewrite would, so G-04 stands.

---

## G-01 — No tick arithmetic: the pass never reads its step

**Tag site:** `BrawlerHitRoutingSystem.h`, on `void preIntegrate(const SimulationTimeStep& /*step*/, …)`,
whose first parameter is unnamed.

**The fence, verbatim:**

<!-- pristine lines 72-78 -->
```
    // [og-netcode-v2-field-defects task 17] No tick arithmetic lives in this system. Task 20
    // Rework (1) kept a per-step-kind tick offset here so branches 3 and 4 could match a
    // projectile slot's endTick against the tick the timeline integrated last; the user ruled
    // that a system must not need to know which tick ran last. Branches 3 and 4 now read the
    // projectile outcome brawlerHitDetection::System produced in this same pass, exactly as
    // branch 5 reads guardBlockedThisTick, so every step kind (Normal, Skip, Stall, HardResync,
    // a resim replay) routes what this pass detected and nothing else.
```

**The prohibition.** ⛔ Do not name the `step` parameter to compute a tick: not the tick integrated
last, not a per-step-kind offset, not a projectile slot's `endTick` compared against anything. User
ruling, og-netcode-v2-field-defects task 17 (2026-09-24): a system must not need to know which tick
ran last. Branches 3 and 4 route the projectile outcome `brawlerHitDetection::System` wrote in this
same pass, and branch 5 the guard block it wrote. The detector's half of the same ruling is
`BrawlerHitDetectionSystem-guards.md` G-14.

**What breaks if the edit is made.** The shape the ruling removed needed a different offset for
each step kind and had no answer at all for a HardResync, whose jump the step cannot see
(`BrawlerProjectileHitDetection-rationale.md` §3). Measured by poison (task 29, the real header swapped in, LLT rebuilt, `[@og]` run, restored): naming the parameter and computing `step.getTick() - 1u` in the body compiles and leaves all 616 cases green. Nothing but this tag stands at the edit.

**Score (§9.1):** substitution on the tagged token (`/*step*/` → `step`) → `yes`. The arithmetic
itself is typed in the body, but it cannot compile until the parameter is named on the tagged
statement (measured, rationale §2). ⚠ Clause C treats a realization's lines as typed at the tag
only when they are contiguous, and these are not. Scored `yes` on the substitution the compiler
forces; the shape is recorded as a finding in rationale §9.

---

## G-02 — Walk the characters sorted by id, never in `StorageView` order

**Tag site:** `BrawlerHitRoutingSystem.h`, on the `std::sort` over `ordered` at the top of
`preIntegrate`.

**The fence, verbatim:**

<!-- pristine lines 144-147 -->
```
            // Deterministic walk order (D4): StorageView iteration order is
            // unspecified; sort by ascending id for cross-machine reproducibility.
            // This sort — not the hash-map iteration — is the authoritative per-
            // tick ordering the routing contract depends on.
```

**The prohibition.** ⛔ Do not delete the sort, and do not sort by anything but the storage key. The
view walks unordered-map order, which is unspecified and machine-varying (the
`SimulationSystemsExecutor` library contract). Routing is not commutative: `resolveHitReaction`
assigns the whole reaction onto the target's slice, so when two hits reach one target in one pass,
the one routed last wins (rationale §6). Within branch 2 that is the attacker with the higher id.

**What breaks if the edit is made.** Two peers that walk the storage in different orders resolve a
different reaction for the same target on the same tick: a divergence no input explains, which only
a correction repairs. ⚠ The sort is cross-peer only because the key is: it is the peer-stable
`SimCharacterId` since og-netcode-v2-field-defects task 25 (rationale §5). Measured by poison (task 29, the real header swapped in, LLT rebuilt, `[@og]` run, restored): deleting the sort compiles and leaves all 616 cases green. No rig registers two characters in an order the map walks differently from the sorted one, so this tag is the only thing that stands at the edit.

**Score (§9.1):** deletion → `yes`; substitution of the comparator → `yes`.

---

## G-03 — The reset assigns a value-initialised slice, never field by field

**Tag site:** `BrawlerHitRoutingSystem.h`, on `slice = brawlerInboundHit::DerivedState{};`, branch 1.

**The fence, verbatim:**

<!-- pristine lines 157-163 -->
```
            // 1. Reset every character's inbound-signal slice before repopulating
            //    this tick. The routing pass owns the reset/set lifecycle so each
            //    tick's signal is fresh (the machine sim never sees a stale flag).
            //    ⚠ [movement-sim task 27] WHOLE-SLICE, not field-by-field: the slice
            //    now carries the resolved reaction (kind, speed, direction, dwell)
            //    beside the two bools, and a reset that names fields is a reset that
            //    the next field added is silently missing from.
```

**The prohibition.** ⛔ Do not replace the assignment with per-field resets, and do not delete it.
The slice holds three one-shot bools and the resolved reaction beside them (kind, speed, dwell,
direction; `BrawlerInboundHit-rationale.md` §2). Routing owns its whole lifecycle, and the
assignment is what makes a field added later reset with the rest.

**What breaks if the edit is made.** A field the reset does not name keeps the value an earlier
pass wrote. A stale bool re-fires its transition on every later tick (a stale
`wasGuardBlockedThisTick` recoils the attacker into `GuardFlinch` again whenever it attacks), and a
stale reaction field is read by the next hit that does not overwrite it. Measured by poison (task 29, the real header swapped in, LLT rebuilt, `[@og]` run, restored): resetting only `wasHitThisTick` and `wasProjectileBlockedThisTick` fails 4 cases: `HitDetection.GuardBlockRoutesThroughTheInboundSlice`, `HitDetection.Behaviour.ReplayAnchoredAtTheEndOfTheBlockTickRecoils`, `HitDetection.Behaviour.ReplayWithoutItsOwnPreIntegrateLosesTheRecoil` and `HitRouting.PerAttackReactionTableIsAuthoritative`.

**Score (§9.1):** substitution on the tagged statement → `yes`; deletion → `yes`.

---

## G-04 — Branch 2 routes the per-TICK signal, never the per-SWING ledger

**Tag site:** `BrawlerHitRoutingSystem.h`, on `for (const auto& hit : radialDerived.getHitsThisTick())`,
branch 2.

**The fence, verbatim:**

<!-- pristine lines 177-190 -->
```
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
```

**The prohibition.** ⛔ Route from `hitsThisTick` only. Do not iterate the radial's per-swing ledger
(`dAttackRadialSimulation::State::hitTargets`, synced since og-netcode-v2-field-defects task 27),
and do not iterate anything else that accumulates over a swing. The ledger is the detector's dedup
record; the per-tick signal is the only thing that says a hit landed THIS pass.

**What breaks if the edit is made.** One hit re-fires on every remaining tick of the swing: movement-sim
task 83's 13 m throw against an authored 5 m, a lockout restarted every tick and a stun re-entered
every tick. `HitRouting.RadialHitFiresOnceAcrossTheSwing` is the end-to-end pin. ⚠ Measured by
compiling (rationale §2): the literal substitution does not compile, because a ledger entry is a
`SimCharacterId` and has no `hitRootBodyId` (C2228). A rewrite that resolves the id to a character
would compile, which is why this entry stands.

**Moved here by task 29.** The same edit ("do not route from the ledger") was a clause of
`BrawlerHitDetectionSystem-guards.md` G-17, tagged on the detector's `registerAttackHit`. That is a
line in another file from where the edit is typed (§9.1 `no (elsewhere)`), so the clause moved to
this site and G-17 keeps its own edit.

**Score (§9.1):** substitution on the tagged range expression → `yes`.

---

## G-05 — The knockback direction is the swing tangent WITH the away-from-attacker fallback

**Tag site:** `BrawlerHitRoutingSystem.h`, on the `resolveHitReaction(…)` call in branch 2, whose
third argument is `normalisedXY(hit.swingTangent, directionAwayFromAttacker(*attackerPtr, *target))`.

**The fence, verbatim:**

<!-- pristine lines 226-236 -->
```
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
```

**The prohibition.** ⛔ Keep `directionAwayFromAttacker` as `normalisedXY`'s fallback. Do not
normalise the tangent directly, and do not drop the fallback because the detector already returns
a zero tangent for a degenerate hit (`BrawlerHitDetectionSystem-guards.md` G-06): a zero XY
projection is exactly the case the fallback exists for.

**What breaks if the edit is made.** A tangent whose XY projection is zero (or NaN) normalises to
NaN. That NaN becomes the slice's `hitDirectionXY`, and the movement sub-simulation multiplies it
into the knockback velocity. ⚠ **R0, task 29: not reachable with the shipped table.** The tangent's
XY projection is degenerate only for a swing whose world axis is not vertical. Sequences 0-3, the
Knockback rows, rotate about the local Z axis, and the machine's aim rotation axis is ±Z, so their
tangent is horizontal and at least the inner radius long. The only horizontal-axis sequence, 4
(forward/overhead), is a Stun, and `resolveHitReaction` discards a Stun's direction. The fence
becomes load-bearing the day row 4 is re-tuned to a Knockback, which is a one-line data edit
(rationale §4.2). `HitRouting.KnockbackDirectionIsTheSwingTangentWithFallback` drives the fallback
with a planted vertical tangent. Measured by poison (task 29, the real header swapped in, LLT rebuilt, `[@og]` run, restored): normalising the tangent's XY directly fails 3 cases: `HitRouting.KnockbackDirectionIsTheSwingTangentWithFallback`, `HitRouting.KnockbackDirectionFromPositionsWithAimFallback` and `HitRouting.PerAttackReactionTableIsAuthoritative`, all on planted tangents.

**Score (§9.1):** substitution on the tagged statement (its continuation lines, clause B) → `yes`.

---

## G-06 — "Already flinching" means `HitFlinch` only

**Tag site:** `BrawlerHitRoutingSystem.h`, on `const bool targetFlinching = …;`, branch 3.

**The fence, verbatim:**

<!-- pristine lines 271-275 -->
```
                    // [movement-sim task 88] A target ALREADY in HitFlinch (either reaction
                    // kind, user ruling 2026-09-21) is LAUNCHED rather than re-stunned; the
                    // machine's cross-kind rules (design_hit_reactions.md §3) do the rest.
                    // ⛔ m_currentState ONLY: GuardFlinch is a separate enumerator, so a
                    // guard-flinching target still takes the authored Stun.
```

**The prohibition.** ⛔ Compare `m_currentState` with `DAttackState::HitFlinch` and nothing else. A
target in `GuardFlinch` is not "already flinching" (movement-sim task 88, user ruling 2026-09-21):
it takes the authored projectile Stun. Do not widen the test to `!= Idle`, to `GuardFlinch`, or to
the dwell, and do not read the target's `m_hitReaction`: either reaction kind of a `HitFlinch`
launches.

**What breaks if the edit is made.** A projectile on a guard-flinching (or attacking) target
launches it at `m_projectileHitOnFlinchReaction`'s speed instead of stunning it in place.
Measured by poison (task 29, the real header swapped in, LLT rebuilt, `[@og]` run, restored): widening the test to `!= DAttackState::Idle` fails 1 case, `HitRouting.ProjectileOnFlinchLaunchesElseStuns`.

**Score (§9.1):** substitution on the tagged statement → `yes`.

---

## G-07 — Branch 5 COPIES the guard block onto the slice; the detector never writes it

**Tag site:** `BrawlerHitRoutingSystem.h`, on the write
`attackerPtr->editAllState().editDerivedState().edit<brawlerInboundHit::DerivedState>().wasGuardBlockedThisTick = true;`
inside branch 5.

**The fence, verbatim:**

<!-- pristine lines 325-327 -->
```
            //    ⛔ It must be COPIED here, not written onto the slice by the detector: branch
            //    1 above resets the whole slice every tick, and detection fires before routing,
            //    so a bit the detector set on the slice would be wiped before anyone read it.
```

**The prohibition.** ⛔ Do not move this write into `brawlerHitDetection::System`, and do not
delete it. Detection fires before routing (compile-held, rationale §2), and branch 1 resets the
whole slice after that (G-03), so a bit the detector set on the slice would be wiped before the
machine read it. The detector's output is the radial `DerivedState`'s `guardBlockedThisTick`; this
statement is the only path from it to the machine.

**What breaks if the edit is made.** No melee guard block ever recoils the attacker: the machine's
`Attacking` case never sees `wasGuardBlockedThisTick`. Measured by poison (task 29, the real header swapped in, LLT rebuilt, `[@og]` run, restored): disabling branch 5 fails 6 cases, among them `HitDetection.GuardBlockRoutesThroughTheInboundSlice`, `HitDetection.StunExitTickOutcomeIsIndependentOfIntegrateOrder` and `HitDedup.GuardBlockEndsTheSwingAndTheTargetIsNotHitLaterInIt`.

**Score (§9.1):** moving it into the detector moves the tagged statement to a destination, a cut at
the tag (clause E) → `yes`; deletion → `yes`.

---

## §R — Retired ids

*(None.)*

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerProjectileHitDetection.h` — guards

Every prohibition for the projectile hit detector. Each entry has an **opaque, stable id**; in the
header a single line `// ⛔G-nn` sits exactly where the wrong edit would be typed.

**If this file and `BrawlerProjectileHitDetection.h` disagree, the header is authoritative and
this file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or run-time
check, moves to [§R, Retired ids](#r-retired-ids) and its number is spent forever.

⛔ **Nothing in this file is a rationale.** Why projectile detection is a pass of the detection
system, its timing and its limits live in `BrawlerProjectileHitDetection-rationale.md`.

**Provenance.** The header was written NEW by og-netcode-v2-field-defects task 17 (2026-09-24),
under the comment convention. Its body is the per-slot overlap, projectile-vs-projectile cancel
and guard classification that ran inside `brawlerProjectileSimulation::integrate`, moved. That
function's comments were in the old prose convention; the ones that were fences are carried
VERBATIM from og-brawler `feee368` (the commit the move was made against), marked
`<!-- BrawlerProjectileSimulation.h lines A-B at feee368 -->`. Their "below" and "above" describe
that file, not this one.

**Enforced by the compiler, so no entry here.** The pass cannot write the wire slot or move a
body: it holds each shooter's `State` through a `const` pointer and the body adapter as a
`const` reference, and neither `setBodyTransform` nor a slot field assignment compiles through
them. That is R1 (b)'s "the projectile sub-simulation stays the only writer of its slot", held by
types. The sub-simulation's side of the same split is also held by types: its
`IntegrationUtils` no longer carries a query adapter, so `integrate` cannot query.

---

## G-01 — Reset every shooter's outcomes FIRST, before any slot is queried

**Tag site:** `BrawlerProjectileHitDetection.h`, on the `for` over `shooters` that fills every
`detectedThisTick` with an empty `SlotDetection`, the first statement of the pass.

**The prohibition.** ⛔ Do not delete this reset. `detectedThisTick` must say what THIS pass
found for every slot and nothing a previous pass found.

**What breaks if the edit is made.** The outcome of a slot that ended survives into every later
pass. Routing reads it again on each one and flinches the target, or recoils the shooter, on
every step until the slot is reused. Measured by poison (task 17): with the reset deleted, 8
cases fail, among them `HitRouting.ProjectilePointBlankFollowUpWindow` (the hit is routed on more
than one tick) and `HitDetection.Projectile.AShotEndingOnThePreJumpTickIsRoutedOnceAcrossAHardResync`.

This is the projectile half of `BrawlerHitDetectionSystem-guards.md` G-13: the detector owns
the per-tick signals it writes.

---

## G-02 — A slot `integrate` is about to EXPIRE is not in flight

**Tag site:** `BrawlerProjectileHitDetection.h`, on
`if (elapsedSeconds >= staticData.maxLifetime) continue;` in the first loop.

**The prohibition.** ⛔ Do not delete this skip. The pass must not query a slot that
`brawlerProjectileSimulation::integrate` expires on the same tick: an elapsed time at or above
`maxLifetime`, the same elapsed time `integrate` computes for any tick at or after the spawn tick.
This skip is for lifetime EXPIRY only. A slot from a later tick than the step is G-07's, and the
pass's elapsed time no longer wraps for one (G-08), so this skip does not see such a slot.

**What breaks if the edit is made.** A shot in contact on its expiry tick is routed as a hit or a
block, while `integrate` checks the lifetime first and ends the wire slot with endReason 1. The
wire says the shot expired and the target flinched anyway. Measured by poison:
`BrawlerProjectile.LifetimeExpiryWinsOverContactOnTheSameTick` goes RED.

⚠ **Task 17 Rework (1), 2026-09-25.** Until then this skip ALSO excluded a slot from a later tick,
through the unsigned wrap of `elapsedSecondsAt` for `tick < spawnTick`: the load-bearing gate was
a wrap (review_defect_17.md N2). That half moved to G-07 and G-08, and deleting this skip no
longer turns the NoSlot case RED (re-measured).

---

## G-03 — The FIRST non-parent entry of the report decides the slot, preserved as-is

**Tag site:** `BrawlerProjectileHitDetection.h`, on the `break;` after `contacts.push_back(...)`.

**The fence, verbatim** (the loop header it closed, and the filter above it):

<!-- BrawlerProjectileSimulation.h lines 463-463 at feee368 -->
```
        // Overlap query at the derived position — find first non-parent hit.
```

**The prohibition.** ⛔ Do not delete the `break` and do not make the pass pick a "best" entry.
A report can list the target's guard and its body; whichever comes first decides between a
block and a hit, exactly as it did inside `integrate`. It is a quirk, and task 17 moved it
unchanged: a move that also changed behaviour could not be verified as a move.

**What breaks if the edit is made.** Every later entry also becomes a contact for the same slot,
and the last one processed wins, so a guard listed before the body stops blocking. Measured by
poison (task 17): `HitDetection.Projectile.TheTickAfterTheStunExitBlocksInBothOrders` and the
two re-homed projectile-block cases in `BrawlerHitDetectionBehaviourTest.cpp` go RED.

---

## G-04 — A cancel ends BOTH slots of the pair

**Tag site:** `BrawlerProjectileHitDetection.h`, on the `if` that marks the in-flight slot whose
own body the contact names (the partner) as `CancelledByProjectile`.

**The fence it replaces, verbatim, and why it was false:**

<!-- BrawlerProjectileSimulation.h lines 477-487 at feee368 -->
```
            // [hit-resolution T13] Projectile-vs-projectile cancellation. When the
            // overlap returns another projectile (own or opposing), end this slot
            // WITHOUT recording a routable hit: endReason=3 excludes the slot from
            // T3's routing filter (which only routes endReason==2), so neither
            // owning character enters HitFlinch. Both sides detect the collision
            // independently on the same tick — each character's own projectile sim
            // processes its own slot, so no cross-character mutation is needed.
            // parkBody + endTick=currentTick despawn the projectile the same way a
            // regular hit does; the viz's spawnTick/endTick guard skips rendering.
            // The slot recycles naturally on the next tick per isFree(currentTick)
            // (endTick != 0 && currentTick >= endTick).
```

⚠ **R0, 2026-09-24.** "Both sides detect the collision independently on the same tick ... no
cross-character mutation is needed" was FALSE. Measured on the pre-task tree
(`HitDetection.Projectile.CancelEndsBothShotsOnTheSameTickInBothOrders`, RED): the shooter that
integrated first cancelled and PARKED its body, so the second shooter's query no longer saw it
and its shot flew on (target-first: A `0/0`, B `23/3`; shooter-first: A `23/3`, B `0/0`).
"routes endReason==2" is now "routes `SlotOutcome::Hit`".

**The prohibition.** ⛔ Do not delete the partner mark on the ground that the partner's own query
will find this slot anyway. A query is asymmetric whenever the partner's first entry is
something else, and then only one of the two shots ends.

**What breaks if the edit is made.** A shot whose report names the target before the other
projectile hits the target while its partner is cancelled. Measured by poison (task 17):
`HitDetection.Projectile.CancelEndsBothShotsOnTheSameTickInBothOrders` goes RED in its "shot A's
report also names the target" half.

---

## G-05 — A cancelled slot neither hits nor blocks

**Tag site:** `BrawlerProjectileHitDetection.h`, on the
`if (detection.outcome == SlotOutcome::CancelledByProjectile) continue;` that opens the
classification loop.

**The prohibition.** ⛔ Do not delete this skip. A slot the pair cancelled may still have a
character as its OWN first entry (G-03); classifying it would overwrite the cancel with a hit or
a block, and route it, in the same pass.

**What breaks if the edit is made.** Measured by poison (task 17): the partner's cancel is
overwritten with a hit on the target, and
`HitDetection.Projectile.CancelEndsBothShotsOnTheSameTickInBothOrders` goes RED.

---

## G-06 — The projectile keeps its OWN block predicate; never `wouldGuardBlock`

**Tag site:** `BrawlerProjectileHitDetection.h`, on
`blocked = projectileGuardBlocks(guardTransform, slot.spawnDir, staticData.guardMiddleSectionHalfAngle);`.

**The prohibition.** ⛔ Do not substitute `dAttackRadialSimulation::wouldGuardBlock`, and do not
change the half-angle argument. User ruling R2 = (i), 2026-09-24: the projectile's predicate
moved verbatim (∴D-01). Unifying the two block rules is Backlog task 22, which carries its own
gameplay change and its own PIE check.

**What breaks if the edit is made.** Projectile blocks change outcome at the edges of the middle
section, silently, in a task that was meant to change only where detection runs. Measured by
poison (task 17): widening the angle to four times the middle section turns
`BrawlerProjectile.DamageOnGuardSide` (a guard pi/4 off the shot) from a hit into a block.

---

## G-07 — A slot from a LATER tick than the step is not in flight

**Tag site:** `BrawlerProjectileHitDetection.h`, on `if (tick < slot.spawnTick) continue;` in the
first loop, after the `isAlive` skip and before any elapsed time is computed.

**The prohibition.** ⛔ Do not delete this skip, and do not fold it into the lifetime skip (G-02).
A resim NoSlot row leaves a character in storage, not integrated, holding its UN-RESTORED frontier
state (`prepareResimAll` restores only the characters whose cache holds the anchor). That state can
carry a slot that spawned on a later tick than the one being replayed. On this tick the shot does
not exist yet.

**What breaks if the edit is made.** The pass places the not-yet-fired shot at its spawn position
(G-08 gives it zero elapsed time) and detects it on every NoSlot replay tick before its spawn tick,
so the target is flinched by a shot that was fired later. Measured by poison (task 17 Rework (1)):
`HitDetection.Projectile.AnUnintegratedShooterRoutesItsShotOnce`, section "NoSlot steps precede the
slot's spawn tick: one routed hit", goes RED.

---

## G-08 — The pass's elapsed time does not wrap before the spawn tick

**Tag site:** `BrawlerProjectileHitDetection.h`, on the `return` of `elapsedSecondsFromSpawn`.

**The prohibition.** ⛔ Do not replace `elapsedSecondsFromSpawn` with a direct call to
`brawlerProjectileSimulation::elapsedSecondsAt`, and do not drop its `tick < slot.spawnTick`
branch. For `tick < spawnTick` the unsigned difference in `elapsedSecondsAt` wraps to an elapsed
time far above `maxLifetime`, and G-02 would then skip the slot a second time, behind G-07.

**What breaks if the edit is made.** Nothing goes RED by itself, and that is the harm: G-07 stops
being load-bearing, because the wrap hides its deletion. Measured by poison (task 17 Rework (1)):
with this branch replaced by the direct call, the suite stays green; with it replaced AND G-07
deleted, the suite still stays green. The G-07 case can no longer see G-07. For
`tick >= spawnTick` the function returns exactly `elapsedSecondsAt`, so the pass and `integrate`
still sample the same closed-form point (∴D-02).

---

## §R — Retired ids

*(None.)*

<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: GroundAlignedProjectileBlock.FlatGuardReproducesTodaysOutcomeAndMarkerBits -- RETIRED case name: og-ground-aligned-attacks task 20b re-stated that case as FlatGuardBlocksInItsPlaneAndKeepsTodaysMarkerBits; it is named only in retired G-09's poison record, kept as it stood. It must NOT resolve -->
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
every step until the slot is reused. (Since og-attackstatetransition-cleanup task 4 routing ignores a
block outcome, so only the target half applies.) Measured by poison (task 17): with the reset deleted, 8
cases fail, among them HitRouting.ProjectilePointBlankFollowUpWindow (the hit is routed on more
than one tick; that assertion is in `HitRouting.ProjectilePointBlankHitStunsOnce` since
og-attackstatetransition-cleanup task 4 retired the case's follow-up half, user ruling 2026-10-04)
and `HitDetection.Projectile.AShotEndingOnThePreJumpTickIsRoutedOnceAcrossAHardResync`.

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
character as its OWN first entry (G-10); classifying it would overwrite the cancel with a hit or
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

**The guard's-plane projection is the projectile's own too** (og-ground-aligned-attacks tasks 8 and
20b). Since task 8 `projectileGuardBlocks` projects the incoming direction onto the guard's plane, and
since task 20b (OQ17 (b)) it does so for every guard, a flat one included
(`BrawlerProjectileHitDetection-rationale.md` §1); the clamp, `glm::acos` and half-angle test follow the
projection. This prohibition covers the projection too: it is not replaced by `wouldGuardBlock`, and the
half-angle argument stays. Re-measured at task 20b with the same poison (four times the middle section):
`BrawlerProjectile.DamageOnGuardSide` goes from a hit to a block, `REQUIRE( state.slots[0].endReason == 2u )`
with expansion `4 == 2`. Task 20b turned that case's shot pi/4 to the guard's side in the horizontal plane:
its old shot was pitched pi/4, and since OQ17 (b) a facing guard blocks a pitched shot.

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

## G-10 — One contact per slot: the FIRST non-parent entry, with the struck character's guard entry

**Tag site:** `BrawlerProjectileHitDetection.h`, on the `break;` after
`contacts.push_back({ { s, i }, hit, guardEntryOf(report, hit) });` in the first loop.

**The prohibition.** ⛔ Do not delete the `break`, and do not make the pass keep a contact per entry or
pick a "best" entry. A slot has one contact, the first non-parent entry of its report: it says which
character the shot struck, or which projectile cancels it, and the cancel pass (G-04, G-05) reads only
that. Whether the struck character blocks is not that entry's category. The contact carries the same
report's guard entry for that character (`guardEntryOf`, ∴D-03), and the classification reads only that.
This replaces G-03 (retired, §R), whose rule was that the first entry's category decided.

**What breaks if the edit is made.** Every later entry also becomes a contact for the same slot, and
the last one processed writes the outcome. A projectile listed after the target cancels a shot that
struck the target, and another character's guard listed after the target blocks it in that character's
name. Measured by poison (task 20b, the `break` deleted):
`HitDetection.Projectile.AProjectileListedAfterTheTargetDoesNotCancel` goes RED (`3 == 2` and `3 == 4`:
both shots cancelled), and `HitDetection.Projectile.NoGuardEntryOfTheStruckCharacterIsAHit` goes RED
(`4 == 2`, struck root `7 == 4`: the other character's guard blocked the shot). G-03's task-17 pin,
`HitDetection.Projectile.TheTickAfterTheStunExitBlocksInBothOrders`, stays green under this poison since
task 20b: its report lists the guard, then the body, and either contact blocks.

---

## G-11 — The guard entry is the STRUCK character's: match the root

**Tag site:** `BrawlerProjectileHitDetection.h`, in `guardEntryOf`, on the `if` that tests
`entry.rootBodyId == first.rootBodyId` together with the `guard` category.

**The prohibition.** ⛔ Do not drop the root test, and do not take the report's first guard entry
whoever owns it. A report can name more than one character, and only the guard of the character the
first entry names may block the shot.

**What breaks if the edit is made.** A shot that struck one character is blocked by a neighbour's
facing guard listed later in the same report. The slot still names the struck character
(`struckRootBodyId` is the first entry's root), so it ends `BlockedByGuard` against a character whose
own guard played no part. Measured by poison (task 20b, the root test removed):
`HitDetection.Projectile.NoGuardEntryOfTheStruckCharacterIsAHit` goes RED on the
`[body, otherGuard]` report, `CHECK( r.outcome == SlotOutcome::Hit )` with expansion `4 == 2`.

---

## G-12 — A zero projection is NOT blocked: keep the explicit return

**Tag site:** `BrawlerProjectileHitDetection.h`, in `projectileGuardBlocks`, on
`if (glm::dot(incoming, incoming) <= 0.f)` (`return false;`) after the projection.

**The prohibition.** ⛔ Do not delete this return on the ground that the angle test says "not
blocked" for a degenerate shot anyway. A shot along the guard's up projects to the zero vector, and
`glm::normalize` of it is not a direction; what the clamp, `glm::acos` and the comparison then make of
it is the compiler's floating-point model, not a rule.

**What breaks if the edit is made.** Measured by poison (task 20b, the return deleted):
`GroundAlignedProjectileBlock.AShotAlongAFlatGuardsUpIsAHitNotANaN` goes RED. Its
`projectileGuardBlocks(guard, ±guardUp, pi)` returns `true` for both signs (`CHECK_FALSE( predicate )`
fails, `predicateAtHalfAnglePi=1`): the degenerate shot came out blocked. With the shipped half angle
(0.25 rad) the same shot still came out a hit in that build, so the outcome rested on the compiler.
Since task 20b every guard runs the projection, so a vertical shot onto a flat guard reaches this
return; before it only a tilted guard could, and task 8 measured the deletion inert there.

---

## §R — Retired ids

### G-03 — RETIRED by og-ground-aligned-attacks task 20b (2026-11-03)

**Was:** "The FIRST non-parent entry of the report decides the slot, preserved as-is", on the `break;`
after `contacts.push_back(...)`. The prohibition: do not delete the `break` and do not make the pass
pick a "best" entry; a report can list the target's guard and its body, and whichever comes first
decides between a block and a hit, "a quirk" that og-netcode-v2-field-defects task 17 moved unchanged
so that the move could be verified as a move. The fence it carried, verbatim (the loop header it
closed, and the filter above it):

<!-- BrawlerProjectileSimulation.h lines 463-463 at feee368 -->
```
        // Overlap query at the derived position — find first non-parent hit.
```

**Why retired.** Its rule changed (full-body guarding, the user 2026-10-31; Backlog task 20b). The
`break` stays, but the first entry no longer decides between a block and a hit: the struck character's
guard entry does, wherever the report lists it. In Jolt the target's weapon `body` entry sorts before
its guard entry (declaration index 0 before 1 under `peerStableOrderOf`;
`BrawlerProjectileHitDetection-rationale.md` §6), so under G-03 a shot that reached both spheres on
one tick hit through a raised, facing guard. The `break` is now **G-10**, the root match **G-11**. G-03's
task-17 poison record (deleting the `break` turned `TheTickAfterTheStunExitBlocksInBothOrders` RED) no
longer holds: re-measured at task 20b, that case stays green (G-10). ⛔ The number 3 is spent.

### G-09 — RETIRED by og-ground-aligned-attacks task 20b (2026-11-03)

**Was:** "A flat guard keeps the 3-D angle; the projection runs only off flat", on
`if (!surfaceFrame::isFlat(guardUp))` in `projectileGuardBlocks`. The prohibition: do not drop the gate
so that every guard projects the incoming direction, and do not move the flat case into the
projection; by the initiative's flat rule a flat normal ran the pre-initiative 3-D angle, and
projecting on flat ground too was a gameplay change that needed its own user ruling. Measured by
poison (task 8, `if (true || !surfaceFrame::isFlat(guardUp))`):
`GroundAlignedProjectileBlock.FlatGuardReproducesTodaysOutcomeAndMarkerBits` went RED,
`outcomeMismatches == 0` with expansion `504 == 0`, every one of its 504 shots pitched ±0.3 rad.

**Why retired.** That ruling came: OQ17 (b), the user 2026-11-01 ("yes go ahead with b"). The gate is
deleted and every guard projects the shot onto its own plane, so a shot pitched along a slope is
blocked head-on by a facing guard on flat ground. The zero projection a flat guard can now reach is
**G-12**. The task-8 case is re-stated as
`GroundAlignedProjectileBlock.FlatGuardBlocksInItsPlaneAndKeepsTodaysMarkerBits`: its 504 pitched shots
now block (`blockedOnlyInThePlane`). ⛔ The number 9 is spent.

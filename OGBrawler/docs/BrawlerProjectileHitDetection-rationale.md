<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerProjectileHitDetection.h` — rationale

<!-- lint-external-ref: impl/design_hit_detection_system.md -- the task-9 design document, in the og-netcode-v2-field-defects initiative workspace; that workspace is not part of this repository -->
<!-- lint-external-ref: impl/review_defect_20_c1.md -- the task-20 Rework (1) review, in the same initiative workspace -->
<!-- lint-external-ref: ProjectileSlot::hitRootBodyId -- RETIRED by og-netcode-v2-field-defects task 17: the projectile slot field routing branch 3 read. It must NOT resolve; the struck character now travels in brawlerProjectileSimulation::SlotDetection::struckRootBodyId -->

The law, the provenance and the derivations. The **prohibitions** are in
`BrawlerProjectileHitDetection-guards.md`; nothing here is a fence.

**Provenance.** Written by og-netcode-v2-field-defects task 17 (2026-09-24), user rulings R1 (b),
R2 (i) and "check pos(t), reaction one tick earlier". The design is §2.1 and §7.1 of
`impl/design_hit_detection_system.md` in that initiative's workspace (outside this repository).
Blocks marked `<!-- BrawlerProjectileSimulation.h lines A-B at feee368 -->` are the old in-integrate
detection's own comments, carried VERBATIM from og-brawler `feee368`; their "above" and "below"
describe that file.

⚠ **This file is not the source of truth for any VALUE.** Where it quotes a number, the code and
the tests win.

---

## 1. The block predicate, moved verbatim ∴D-01

`projectileGuardBlocks` is the classification `brawlerProjectileSimulation::integrate` ran,
statement for statement. Its operands:

<!-- BrawlerProjectileSimulation.h lines 498-499 at feee368 -->
```
            // T29 — classify guard hits inside the front cone as BLOCKS; everything
            // else (guard outside the cone, or a plain body hit) is a damage hit.
```
<!-- BrawlerProjectileSimulation.h lines 504-507 at feee368 -->
```
                // The guard sim rotates the guard body so its forward axis (the first
                // column of the rotation matrix) points along the target character's
                // aim direction (see DAttackGuardSimulation.h). The guard body sits at
                // the character root, so its translation IS the character position.
```
<!-- BrawlerProjectileSimulation.h lines 515-517 at feee368 -->
```
                // Direction the projectile is travelling INTO the guard is -spawnDir
                // (spawnDir is the unit launch/travel direction). A guard facing the
                // incoming projectile (forward ≈ -spawnDir) yields a near-zero angle.
```

⚠ **R0, 2026-09-24.** The guard transform claim was re-read from `dAttackGuardSimulation::integrate`:
it writes `rotate(aimAngle, aimRotationAxis)`, the rotation that takes the default forward to the
aim, with the root translation in column 3, on every tick. Column 0 is the aim only because the
default forward is the X axis; that value was not re-read here. "The front cone" is the MIDDLE
section since T31 (`StaticData::guardMiddleSectionHalfAngle`, 0.25 rad), not the radial's outer
cone.

**Why it is not `wouldGuardBlock`.** The radial's predicate falls through on NaN and tests two
sections; this one length-guards the forward and clamps the dot product. User ruling R2 = (i):
move it unchanged, and unify the two in Backlog task 22 (guards G-06). The pins are
`BrawlerProjectile.BlockOnGuardFront`, `DamageOnGuardBack`, `DamageOnGuardSide` and
`BlockIndicatorAtInnerCircle`, whose assertions did not change; only their helper now runs this
pass before `integrate`.

The block MARKER (T30, the inner-circle entry point) is not computed here. The pass hands the
guard body's translation to `integrate` in `SlotDetection::targetRootPosition`, and `integrate`
places the marker as before, next to the log line it prints.

---

## 2. The pass: three loops, and why the result does not depend on walk order ∴D-02

**The defect it closes (task 17).** Detection used to run inside the SHOOTER's `integrate`. It
made the melee phantom's two cross-character reads per slot: the overlap sees the target's guard
shape only if the target's `integrate` has enabled it this tick, and the guard transform is
written in that same `integrate`. The projectile added a third, the cancel: the shooter that
integrated first parked its body, and the second shooter's query no longer saw it. Measured on the
pre-task tree:
* `HitDetection.Projectile.StunExitTickOutcomeIsIndependentOfIntegrateOrder`: target-first
  `endReason=4` (blocked), shooter-first `endReason=2` (hit).
* `HitDetection.Projectile.CancelEndsBothShotsOnTheSameTickInBothOrders`: only the
  first-integrated shot cancelled.

**The shape.**
1. **Query every slot in flight.** In flight means spawned on or before the step's tick (G-07; a
   resim NoSlot row can hold a slot from a later one) and not expired by `integrate` on it (G-02).
   Each slot's volumes are parented to its closed-form position on the STEP's tick
   (`brawlerProjectileSimulation::closedFormPosition`, the function `integrate` snaps the body
   with), and the first non-parent entry of its report is kept (G-03). Nothing is written in this
   loop except the slot's own volume parents, so no query sees another slot's outcome.
2. **Cancel.** Every slot whose first entry is a projectile ends `CancelledByProjectile`, and so
   does the in-flight slot whose own body that entry names (G-04). The pair is cancelled if EITHER
   query saw the other, a union, so the walk order cannot matter.
3. **Classify** every contact that was not cancelled (G-05): a guard entry is a block when
   `projectileGuardBlocks` says so, anything else is a hit.

The detector walks the shooters in the sorted order the system hands it, but no step above reads
anything an earlier shooter wrote, so the outcome is the same in any order. The sort
(`BrawlerHitDetectionSystem-guards.md` G-11) is kept as a fence, not because this pass needs it.

**What the pass writes.** Only `DerivedState::detectedThisTick` of each shooter, reset first
(G-01). `integrate` reads it in the same step, ends the slot with `endReason` equal to the outcome
code and parks the body. Routing reads it in the same pass (branches 3 and 4). Nothing about it
crosses a tick boundary, so a resim replay recomputes it from the restored slot.

The parent filter, carried from the old loop (unchanged in the code):

<!-- BrawlerProjectileSimulation.h lines 472-473 at feee368 -->
```
            // Parent-body filter runs first — the firing character never blocks or
            // takes damage from its own projectile.
```

⚠ **R0, 2026-09-24.** "never blocks" is wider than the code. The filter drops only the parent
body (the character's capsule). The shooter's own GUARD is a different body, and the guard
sub-simulation enables its shape whenever the machine is `Idle`, whatever the guard input. In the
task-17 LLT rig a shot's query reported its own shooter's guard and was classified a block. Whether
the UE query adapter can report a shooter's own guard to its projectile volume depends on the
collision-channel setup, which was read, not measured. The code did not change; the claim is noted.

---

## 3. Timing: the step's OWN tick, and the reaction lands in the same step

The pass queries each slot at its closed-form position on `step.getTick()`, in that step's
`preIntegrate` (`BrawlerHitDetectionSystem-guards.md` G-14). `integrate` of the same step snaps
the body to that same position and ends the slot there. Consequences, measured in
`HitRouting.ProjectilePointBlankFollowUpWindow` (fire on tick 1, target at point blank):

| | before task 17 | after |
|---|---|---|
| contact tick (the closed-form position that overlaps) | 2 | 2 |
| slot `endTick` | 2 | 2 |
| target enters `HitFlinch` | 3 | **2** |
| target leaves `HitFlinch` | 42 | **41** |
| follow-up swing's first damaging tick | 38 | 38 |
| slack | 4 ticks | **3 ticks** |

The shot is never drawn a step past the target: `endTick` is the contact tick, as before, and the
visualization draws no slot whose `endTick` is set.

**Why not the position of the tick integrated last.** The spec first assumed the pass would test
the position the shot held during the previous tick. Knowing that tick is the "which tick ran
last" arithmetic the user ruled out: it is `step.getTick() - 2` on a graduated Skip, and on a
HardResync the step cannot see the jump at all (`impl/review_defect_20_c1.md` §1.3). The step's own
tick needs nothing. On every step kind the pass tests exactly the positions `integrate` visits: a
Skip tests tick L+2, which is where `integrate(L+2)` snaps the shot, and neither of them ever
visits the backfilled L+1.

**The reaction moved one tick earlier, and that aligns the projectile with melee.** In UE a live
shot's body is snapped with its velocity (gravity off), so after physics step t-1 it already sits
at about pos(t) (read from `integrate`'s `snapBody`, not measured in PIE). The pass reads that position against the guard `integrate(t-1)` left. Melee reads the
torque-driven weapon where physics step T left it, against the guard `integrate(T)` left, and
reacts in `integrate(T+1)`. Both read the world as the previous tick's physics step left it, and
both react in the next `integrate`. Before task 17 the projectile reacted one tick later than that,
because the shooter's `integrate(t)` detected it and the reaction waited for the next step's
routing. So a shot that meets a target on the target's first `Idle` tick after a stun is a HIT on
every peer: the guard is still off in the state `integrate(t-1)` left. One tick later it is a
block (`HitDetection.Projectile.TheTickAfterTheStunExitBlocksInBothOrders`).

---

## 4. Hazard: a shooter that is not integrated on a step

The pass walks every character in storage, but only the shooter's own `integrate` ends its slot,
and nothing carries "already routed" across a tick (a cross-tick ledger and a system writing wire
state were both ruled out). So a slot in flight on a shooter that skips its `integrate` is
detected again on the next pass.

Production reaches "in storage, not integrated" in two ways:
* a resim NoSlot row: the character has no correction-cache slot for the replayed tick
  (`SimulationReconciliation::getAppliedCaptureTickRef` returns NoSlot when the cache is missing or
  does not hold the tick), which means it was registered after that tick;
* the game-thread registration tear, a character added between two phases of one step.

Both precede the character's first `integrate` on the steps concerned. A live slot on such a
shooter is EITHER from a later tick than the step OR was restored from a server correction:
* **From a later tick.** The NoSlot character keeps its un-restored frontier state, which can hold
  a shot it fired after the replayed tick. G-07 skips it explicitly. Pinned by
  `HitDetection.Projectile.AnUnintegratedShooterRoutesItsShotOnce`, section 1: two un-integrated
  steps before the spawn tick, target in contact throughout, exactly one routed hit.
* **Restored from a server correction: a REACHABLE residual on a client replay.** A resim restore
  writes the whole `State`, projectile slots included (`prepareResimAll`), so the slot's
  `spawnTick` is the SERVER's tick and can precede the tick at which this client registered the
  character. The path (review_defect_17.md B1):
  1. A proxy registers on the client at tick R, with a default state.
  2. A resim anchored at or after R restores it from its correction. The server's state carries a
     live shot spawned on s < R.
  3. A later correction for another character, anchored at A < R, lands after that resim (nothing
     orders arrivals across actors, and the anchor is raised per cache), so the shared-min resim
     anchors before R.
  4. On the replay ticks in [max(A, s), R) the proxy is NoSlot and is not integrated, so its slot
     is not ended. It passes `isAlive`, G-07 and G-02, and if its closed-form position overlaps a
     character, the SAME shot is detected and routed on EVERY such tick. The proxy's first
     `integrate`, at R, ends it.

  **Conditions.** Rare. It takes all of: a proxy within about one RTT of registering, a shot fired
  on the server before that, cross-actor correction reordering, and a replay contact before R. The
  server's own state says that contact did not happen (the slot is still alive at the restore), so
  it also takes a replay misprediction of the target.
  **Effect.** Client-only and transient: an extra flinch on the replay, healed by the target's next
  correction. The server never resims and never has a NoSlot row. Before task 17 nothing was routed
  there, because detection needed the shooter's own `integrate`.
  **Status.** Accepted by the user on 2026-09-25 as a documented residual. It is the projectile
  analogue of melee's F9 (the task 9 behaviour review: a mid-swing brawler with NoSlot on a replay),
  which melee bounds with its `attackHits` ledger; projectiles have no equivalent. The gate is
  deferred as Backlog task 23. Section 2 of the same case pins the shape: 3 routed hits, one per
  NoSlot replay tick and one on the proxy's first `integrate`, which ends the slot on R.

---

## 5. What the wire lost

`ProjectileSlot::hitRootBodyId` (4 B per slot, 3 slots) left the wire: its only reader was routing
branch 3, which now takes the struck character from `SlotDetection::struckRootBodyId` in the same
pass. The slot keeps `endTick` (it keeps a hit slot ended on later ticks, and `isAlive`, `isFree`
and the visualization read it) and `endReason`. Nothing in production reads `endReason` since
task 17, but the correction gate compares it (`ProjectileSlot::operator==`), so a peer that
disagrees about a shot's outcome diverges on the shooter's own slot on the tick it happens, and
the `BrawlerProjectile.*` cases pin it. The carried history of the removed field:

<!-- BrawlerProjectileSimulation.h lines 536-538 at feee368 -->
```
            // hitRootBodyId is meaningful only for endReason==2 (routing consumer).
            // Clear it defensively on block so a future consumer that forgets the
            // filter doesn't accidentally route a block to HitFlinch.
```

The per-slot footprint is 37 -> 33 B, the composite 338 -> 326 B, and
`correctionStateBuffer::kWireFormatVersion` 4 -> 5 (the slots sit in the middle of the composite).

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerHitDetectionSystem.h` — rationale

<!-- lint-external-ref: impl/design_hit_detection_system.md -- the task-9 design document, in the og-netcode-v2-field-defects initiative workspace; that workspace is not part of this repository -->
<!-- lint-external-ref: ActorInstanceHandle.cpp -- Unreal Engine 5.6 source (Engine/Source/Runtime/Engine/Private/Engine/), outside every scan root; read with gh api at commit cdda65ce -->
<!-- lint-external-ref: dAttackRadialSimulation::collisionCheck -- RETIRED by og-netcode-v2-field-defects task 9: the function this header's detectRadialHits replaces. It must NOT resolve; the day it does, detection has grown back into the radial's integrate -->

The law, the provenance and the derivations. The **prohibitions** are in
`BrawlerHitDetectionSystem-guards.md`; nothing here is a fence.

**Provenance.** Written by og-netcode-v2-field-defects task 9 (2026-09-23), user ruling "option C".
The design is `impl/design_hit_detection_system.md` in that initiative's workspace (outside this
repository). Blocks marked `<!-- DAttackRadialSimulation.h lines A-B at b572456 -->` are the old
`collisionCheck`'s own comments, carried VERBATIM from og-brawler `b572456`; their "above" and
"below" describe that file.

⚠ **This file is not the source of truth for any VALUE.** Where it quotes a number, the code and
the tests win.

---

## 1. Why melee detection is a system ∴D-01

**The defect it closes (task 9, D1).** Detection used to run inside the ATTACKER's integrate, as
`dAttackRadialSimulation::collisionCheck`. It made two reads of another character's state
mid-tick: (a) whether the target's guard shape was query-enabled, which the target's guard
sub-simulation toggles in the target's own integrate, and (b) the target's guard transform, which
the same integrate writes. `SimulationIntegrationExecutor::integrateAll` walks an
`unordered_map`, so a peer that integrated the attacker first saw last tick's guard while a peer
that integrated the target first saw this tick's. On a target's first `Idle` tick after a stun,
that is the difference between the attacker recoiling (`GuardFlinch`) and the target being thrown
(`Knockback`). The field capture showed it tick-exact on 7 of 7 phantoms.

**The fix.** Detection runs in `brawlerHitDetection::System::postIntegrate`, which the systems
executor fires after EVERY character's integrate and before the physics step, on the prediction
path and on the resim replay path alike. By then every character's machine state, guard toggle,
guard transform and weapon pose for tick T are written on every peer, so both reads see the same
thing everywhere. The guard is queryable on T on every peer; that was the authority's behaviour
before, and it is now everyone's. Hit timing to the machine is unchanged: detected and routed on
T, consumed on T+1.

⭐ **Task 20 (2026-09-24) moved the pass to `preIntegrate` of T+1** (§7). The read-order argument
above is unchanged: `preIntegrate(T+1)` also runs after every character's `integrate(T)`.

**Measured.** `HitDetection.StunExitTickOutcomeIsIndependentOfIntegrateOrder` (read a) and
`HitDetection.DetectionSeesThisTicksGuardTransform` (read b) were RED on the pre-move tree
(target-first `GuardFlinch`, attacker-first `Knockback`) and are GREEN through this header.

**What moved, and the three places it is not verbatim.** `detectRadialHits` (slice level) is
`collisionCheck`'s body, statement for statement, except:
1. **The gate** (G-01). `collisionCheck` was CALLED only from the swing branch, so it needed no
   swing test. The detector is called for every character and reads the answer off the state
   `integrate` left. Some characters in storage have not been integrated yet, so the gate is
   all returns and asserts nothing until the pair has passed (G-01, task 9 Rework (1)).
2. **The timer** (G-04). `collisionCheck` read `state.attackTimer` before `integrate` advanced it.
   The detector reads it after, so it subtracts the step.
3. **The guard-block write.** `collisionCheck` set the radial State's `hasHitGuard`, which rode the
   wire. The detector sets the radial DerivedState's per-tick `guardBlockedThisTick`.
   `brawlerHitRouting::System` copies it onto the attacker's inbound slice as
   `wasGuardBlockedThisTick` (routing branch 5), and the machine's `Attacking` case reads that on
   T+1. A derived signal is recomputed on every replayed tick, so it never needs restoring. The
   removal took 1 B out of the MIDDLE of the state composite, so
   `correctionStateBuffer::kWireFormatVersion` went 3 -> 4.

The per-character overload reads the radial slices, bindings and `DerivedState` off a
`SimulatableBrawler` and calls the slice-level one. The single-character test rigs call the two
directly, after `integrate`, which is the production tick's order.

<!-- DAttackRadialSimulation.h lines 497-497 at b572456 -->
```
	// Update parent transforms before querying
```

---

## 2. Body and guard hits merge by actor ∴D-02

<!-- DAttackRadialSimulation.h lines 585-594 at b572456 -->
```
				// [hit-resolution T11] Merge body and guard hits by ACTOR-level identity
				// (rootBodyId). Both the hurtbox and the guard shape on a character now
				// report the same rootBodyId (the capsule), so the two shape hits merge
				// into ONE RootHitData with both bodyHitIndex and guardHitIndex set — the
				// guard directional check below then runs on the correct pairing.
				// The 1337 sentinels guard the body-only vs guard-case fork below
				// (`bodyHitIndex == 1337` continue; `guardHitIndex == 1337` body-only
				// branch). T10 fixed these initializers from 0 to 1337; T11 restores the
				// body+guard merge that pre-D8 relied on, so the sentinel path is no
				// longer load-bearing but is kept correct for standalone-body cases.
```

The order-swap rig in `BrawlerHitDetectionSystemTest.cpp` reports the target's body and guard
with the same root id, which is this merge's precondition.

---

## 3. The guard-hit indicator position ∴D-03

<!-- DAttackRadialSimulation.h lines 668-678 at b572456 -->
```
			// Compute the indicator position on the attacker's weapon line. Two cases:
			//   1. If the weapon line (rootTranslation + t * currentDirection) crosses the
			//      opponent's inner circle, use the near intersection (entry side, closer
			//      to the attacker). Math: quadratic t² + 2bt + c = 0 with b = dot(v, d),
			//      c = |v|² - r², v = attacker - opponent; smaller root = -b - sqrt(...).
			//   2. If the weapon line misses the inner circle (guard hits can register via
			//      spatial overlap even when the weapon direction is off to one side), fall
			//      back to the closest point on the weapon line to the opponent — the foot
			//      of the perpendicular from opponent onto the line: t = dot(opponent - attacker, d).
			// In both cases t is clamped to the visible weapon segment [0, outerRadius] so
			// the indicator can never appear off the end of the blade or behind the attacker.
```

The two trailing comments on the branches of `t`, carried:

<!-- DAttackRadialSimulation.h lines 691-691 at b572456 -->
```
					t = -b - std::sqrt(discriminant); // near intersection on inner circle
```
<!-- DAttackRadialSimulation.h lines 693-693 at b572456 -->
```
					t = glm::dot(opponentXY - attackerXY, weaponDirXY); // closest point on weapon line
```

---

## 4. The block classification is the shared pure predicate ∴D-04

<!-- DAttackRadialSimulation.h lines 698-702 at b572456 -->
```
			// Guard directional block classification now lives in the shared
			// dAttackRadialSimulation::wouldGuardBlock predicate (above) so the sim and
			// the attacker-side block-prediction viz cannot drift. Side effects stay at
			// the call site: weaponHitIndicatorPosition() depends on currentDirection,
			// which is not an input to the pure predicate.
```

`wouldGuardBlock` stays in `DAttackRadialSimulation.h`, because the block-prediction visualisation
includes that header and not this one. Its guard-transform argument is read through the body
adapter by the hit's own body id, which is read (b) of §1:

<!-- DAttackRadialSimulation.h lines 665-665 at b572456 -->
```
			// Query guard body transform via PhysicsBodyAdapter using hit's BodyId
```

---

## 5. The system: order, roles, adapters ∴D-05

* **Firing order.** `SimulationSystemsExecutor` fires systems in template order. The manager's
  `BrawlerSystemsExec` lists this system FIRST, before `brawlerHitRouting::System`, whose branch 2
  reads the `hitsThisTick` written here and whose branch 5 reads `guardBlockedThisTick`. Both
  fire in `preIntegrate` since task 20, so the order holds in that pass.
  `SimulationManagerUImpl.h` pins the order with a `static_assert` on `firesBefore` (§6), and that
  assert was seen to FAIL on a swapped order.
* **Roles.** `AllRoles`, see G-09.
* **Adapters.** The executor constructs each system from the matching argument of its
  `std::piecewise_construct` constructor. The manager passes a constructed `System` built from
  `*m_physAdapter` and `*m_queryAdapter`, which are the same two adapters `m_integrationLayer` uses.
  They are `std::optional`s emplaced per role, so the executor is too (G-10 here, G-75 in
  `SimulationManagerUImpl-guards.md`). The system holds POINTERS rather than references so it
  stays copy- and move-assignable inside the executor's tuple.
* **Lifecycle hooks** fill and drain one map (task 27): `m_targetIdByRootBodyId`, the capsule
  `BodyId` of every registered character to its `SimCharacterId` (the storage key, G-18). It is the
  same shape as routing's `m_byRootBodyId`. The detector resolves each report hit's root through it,
  and `DAttackHit::targetId` carries the result. Unregistration erases by value. The map is
  per-process and never on the wire; only the id it yields is. Until task 27 the hooks were empty
  and the per-swing ledger was the radial's derived vector, keyed by `BodyId`. The ledger is now the
  radial's synced `State::hitTargets` (`DAttackRadialSimulation-rationale.md` §4.4), which this
  system only reads. ⚠ R0 (task 20): the visualisation reads `guardHits`, not the ledger;
  `guardHits` is per-tick since task 20 and cleared here (G-13).
* **What the radial is after this.** The radial poses its own weapon (task 2 will make that pose a
  closed-form function of wire state), does the attachment math and runs `deactivate`. It no longer
  takes a query adapter at all: `dAttackRadialSimulation::IntegrationUtils` lost its
  `SpatialQueryAdapterType` parameter, because detection was the only thing that queried.

---

## 6. `firesBefore` lives here, not in og-simulation ∴D-06

The natural home for "does executor type X fire system A before system B" is
`OGSimulation/SystemsExecutor.h`, next to the executor. Task 9 was allowed exactly one
og-simulation edit (the wire-format version), so the trait lives beside its only user. It
pattern-matches the executor's template arguments and never instantiates the executor. Both arms
are proved in `HitDetection.FiresBeforeTraitSeesTheExecutorOrder`: the shipped order reads true,
the reversed and the missing orders read false.

---

## 7. The pass runs in `preIntegrate` of T+1 (og-netcode-v2-field-defects task 20)

```
before:  integrate(T) -> postIntegrate(T): detect+route -> [physics step T] -> integrate(T+1): machine reacts
after:   integrate(T) -> [physics step T] -> preIntegrate(T+1): detect+route -> integrate(T+1): machine reacts
```

**Why.** Every inbound-slice bit was written in the post-integrate pass of T and read by
`integrate(T+1)`: state carried across a tick boundary off the wire. A resim replay starts at T+1,
so a replay anchored at the end of T never re-ran T's pass and read the frontier's stale slice (the
task 9 behaviour review's F7 for the guard block; the same hole in `wasHitThisTick` since T3 and in
`wasProjectileBlockedThisTick` since T15). After the move the signal is produced and consumed inside
T+1 and is recomputed from the restored end-of-T state. The machine reacts on the same tick as
before: no added latency. og-simulation fires `preIntegrate` before `integrateAll` on the
prediction, authority and resim step functions alike (`SimulationManager.h`), so it needed no edit.
Measured RED -> GREEN: `HitDetection.Behaviour.ReplayAnchoredAtTheEndOfThe{Block,BodyHit,ProjectileBlock}Tick…`.

**What the gate reads.** The pair `integrate(T)` left, the same values the post-integrate pass read
(G-01, with the check for the correction-restored first replay step).

**What the reads sample.**
* LLT rigs: body transforms and query answers are fakes written by the sub-simulations during
  `integrate`. Identical at both points; the rigs schedule contact, so they cannot show a
  physics-step difference.
* UE (code read, not measured in PIE). `ChaosPhysicsBodyAdapter::getBodyTransform` reads the
  physics-thread handle's `GetX()`/`GetR()`. At `postIntegrate(T)` that was the weapon pose
  `integrate(T)` wrote: translation re-attached to the capsule, rotation as physics step T-1
  committed it. At `preIntegrate(T+1)` it is the pose physics step T committed. Until task 2 poses
  the weapon closed-form it is TORQUE-driven, so its rotation is one step further along, and a
  contact first reached during step T is detected one pass earlier: reacted to on T+1 instead of
  T+2. That is earlier, never later, relative to the swing edge, and it is the same on every peer.
  The weapon's translation is whatever step T left, which can trail the capsule by one step of the
  capsule's own motion.
* UE, the candidate set and target positions: `ChaosSpatialQueryAdapter::overlap` sweeps
  `FPhysicsInterface::GeomSweepMulti` against the world's scene-query structure, which the game
  thread updates, and takes `objectPosition` from `FHitResult::HitObjectHandle.GetLocation()`,
  which is `AActor::GetActorLocation()` (UE 5.6 `ActorInstanceHandle.cpp`, lines 220-231 at
  commit `cdda65ce`). Neither sees a physics-thread write made mid-frame, at either point. On a resim
  every replayed step runs inside one physics-thread burst, so the query answers from the same
  frontier-era game-thread view in both designs, while the weapon pose is the Chaos-rewound one
  (task 10: UE rewinds its own recording; OG's anchor push is not read). The move changes neither
  source's staleness.

**Timing, measured in the LLT rigs** (the contact tick is scheduled in both, so these pin that the
move shifted nothing in the rig; the UE shift above is not visible to them):

| rig | swing edge | contact | reaction | before | after |
|---|---|---|---|---|---|
| order-swap (`StunExitTickOutcomeIsIndependentOfIntegrateOrder`) | 15 | 39 (swing tick 24) | `GuardFlinch` 40 | same | same |
| task 86 follow-up (`HitRouting.StunHitFiresOnce`) | 0 | 18 | `HitFlinch` 19 → 58; follow-up damaging 52; slack 6 | same | same |
| task 87 projectile (`HitRouting.ProjectilePointBlankFollowUpWindow`) | fire 1 | 2 | follow-up damaging 38; stun ends 42; slack 4 | same | same |

**Which tick the pass reduces.** The melee pass reads whatever end state the timeline holds,
whatever the step kind. ⚠ Task 20 Rework (1) made routing branches 3 and 4 match a projectile slot's
`endTick` against the tick integrated last (`step.getTick() - 1`, or `- 2` on a graduated Skip);
task 17 removed that at the user's ruling, and no system in this pass uses tick arithmetic any more
(§8).

**Where the per-tick signals live.** Cleared and filled by the detector (G-13), read by routing in
the same pass, visible until the next pass. The radial still clears `hitsThisTick` and
`guardBlockedThisTick` at the top of `integrate` (its G-04, G-05), after routing has read them.

**What it did not fix, and task 27 did.** A BODY hit on the anchor tick was re-detected on the
replay only when the attacker's frontier ledger (then derived, not restored) did not hold the
target. Mid-swing, the stale ledger suppressed it, and `…BodyHitTickFlinchesTheTarget`, section
"frontier mid-swing", pinned the target `Idle`. Task 27 (candidate A of the initiative's hit-dedup
design) put the ledger on the wire: the restore brings back end-of-T's empty ledger, and the
section now reads `HitFlinch`, equal to live (RED on the tree before task 27, GREEN after). The
guard block is not affected: a blocked swing never records the target (the G-08 `break` precedes
`registerAttackHit`), and it ends the swing anyway.

---

## 8. The projectile pass (og-netcode-v2-field-defects task 17)

`preIntegrate` calls `detectProjectileHits` (`BrawlerProjectileHitDetection.h`) after the melee
loop, over the same sorted `ordered` walk. One system, two passes: the lead's recommendation,
discussed with the user. The two passes write disjoint `DerivedState` slices (radial and
projectile), so their order decides nothing today; melee first keeps the existing pass unchanged.

* **What it is.** The projectile's per-slot overlap, projectile-vs-projectile cancel and guard
  classification, moved out of the SHOOTER's `integrate`, where they read the target's guard and
  the other shots mid-tick (the same class as the melee phantom). Its law, timing and limits are in
  `BrawlerProjectileHitDetection-rationale.md`.
* **Its tick.** The step's own tick (guards G-14). The shot is tested where this step's `integrate`
  puts it, and the reaction lands in this step: one tick earlier than when the shooter's
  `integrate` detected it, which aligns the projectile with melee's latency.
* **What it hands on.** Each shooter's `brawlerProjectileSimulation::DerivedState::detectedThisTick`.
  Routing branches 3 and 4 read it in the same pass (`firesBefore` still pins detection before
  routing), and the shooter's `integrate` ends the slot from it. The sub-simulation stays the only
  writer of its wire slot (user ruling R1 (b)).
* **Adapters.** The same two this system was constructed with (G-10). The pass reads the guard
  transform through the read-only body adapter and parents only the projectile's own query volumes.

---

## 9. The retired G-07, kept (og-netcode-v2-field-defects task 27)

G-07 of the guards document read "Every accepted hit is recorded TWICE: the per-swing ledger AND
the per-tick signal", tagged on the `registerAttackHit` lambda. Its fence, verbatim:

<!-- DAttackRadialSimulation.h lines 642-644 at b572456 -->
```
	// [movement-sim task 83] Every accepted hit is recorded TWICE, and the two containers mean
	// different things: attackHits is the per-swing dedup ledger the loop above reads back,
	// hitsThisTick is the one-tick signal hit routing consumes. See DerivedState.
```

Its "what breaks" read: *"Drop the ledger push and the same target is hit on every Damaging tick of
the swing. Drop the signal push and routing never sees the hit. Route from the ledger instead and
one hit re-fires on every remaining tick (the 13 m knockback of movement-sim task 83)."*

What is still true, and where it went: the ledger push left this header, because the ledger is
synced and its writer is the radial's `integrate` (radial G-07). "Drop the signal push" and "route
from the ledger" are G-17. The dedup key is G-16 and the cap is G-15.

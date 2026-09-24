<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerHitDetectionSystem.h` — rationale

<!-- lint-external-ref: impl/design_hit_detection_system.md -- the task-9 design document, in the og-netcode-v2-field-defects initiative workspace; that workspace is not part of this repository -->
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
  reads the `hitsThisTick` written here and whose branch 5 reads `guardBlockedThisTick`.
  `SimulationManagerUImpl.h` pins the order with a `static_assert` on `firesBefore` (§6), and that
  assert was seen to FAIL on a swapped order.
* **Roles.** `AllRoles`, see G-09.
* **Adapters.** The executor constructs each system from the matching argument of its
  `std::piecewise_construct` constructor. The manager passes a constructed `System` built from
  `*m_physAdapter` and `*m_queryAdapter`, which are the same two adapters `m_integrationLayer` uses.
  They are `std::optional`s emplaced per role, so the executor is too (G-10 here, G-75 in
  `SimulationManagerUImpl-guards.md`). The system holds POINTERS rather than references so it
  stays copy- and move-assignable inside the executor's tuple.
* **Lifecycle hooks** are empty. Detection keeps no per-character bookkeeping: the per-swing ledger
  stays in the radial `DerivedState`, which the visualisation reads, and the radial's `deactivate`
  still clears it.
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

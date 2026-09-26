<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerHitDetectionSystem.h` — guards

<!-- lint-external-ref: impl/review_defect_9_rw1.md -- the task-9 Rework (1) review, in the og-netcode-v2-field-defects initiative workspace; that workspace is not part of this repository -->

Every prohibition for the hit detector: the melee pass in this header, and the call into the
projectile pass (task 17), whose own prohibitions are in `BrawlerProjectileHitDetection-guards.md`.
Each entry has an **opaque, stable id**; in the header a single line `// ⛔G-nn` sits exactly
where the wrong edit would be typed.

**If this file and `BrawlerHitDetectionSystem.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or run-time
check, moves to [§R, Retired ids](#r-retired-ids) and its number is spent forever.

⛔ **Nothing in this file is a rationale.** Why detection is a system, and the provenance of the
code, live in `BrawlerHitDetectionSystem-rationale.md`.

**Provenance.** The header was written NEW by og-netcode-v2-field-defects task 9 (2026-09-23),
under the comment convention. Its detection body is the old `collisionCheck` of
`DAttackRadialSimulation.h`, moved. That function's comments were in the old prose convention;
the ones that were fences are carried below VERBATIM from og-brawler `b572456` (the commit the
move was made against), marked `<!-- DAttackRadialSimulation.h lines A-B at b572456 -->`. Their
line references to "below" and "above" describe that file, not this one.

---

## G-01 — The swing gate is the sequence PAIR that `integrate` left, and it returns BEFORE anything asserts

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `if (state.currenSequenceId == InvalidAttackSequenceId || initialConditions.activeAttackSequence == InvalidAttackSequenceId || state.currenSequenceId != initialConditions.activeAttackSequence || state.currenSequenceId >= staticData.getAttackSequences().size()) return;`
that opens the slice-level `detectRadialHits` (one statement over four lines), and the `OG_CHECK(state.attackTimer > 0.f, …)` beneath it.

**The prohibition.** Detection runs after the radial's `integrate` has returned (since task 20, in
`preIntegrate` of T+1, over the state `integrate(T)` left), so "was T a swing tick" must be read
off the state that `integrate` LEFT. In that state the radial's
`currenSequenceId` is valid if and only if `integrate` took its swing branch on T, which is
exactly the branch that used to call `collisionCheck`: the Hadouken sentinel, the two
`deactivate` exits and the idle pose all leave it `InvalidAttackSequenceId`. ⛔ Do not gate on
`activeAttackSequence` alone, and do not re-derive the answer from `attackTimer`: a swing's first
tick ends with `attackTimer == deltaSeconds`, and the sequence pair, not the timer, is the
semantic gate.

⛔ **Every condition is a `return`. Nothing is asserted until all of them have passed.** The
detector walks EVERY character in storage, and not every character in storage has been
integrated. A brawler that meets the detector before its first `integrate` still carries the
radial's default pair: State `currenSequenceId = 0`, InitialConditions
`activeAttackSequence = InvalidAttackSequenceId`. It reaches the detector in three ways:
* ⭐ **(task 20) every newly registered character, on its first tick.** The pass runs in
  `firePreIntegrate`, which precedes `integrateAll`, on a live tick and on a resim replay alike.
  This is the common path now, not a race;
* a client registers a pawn (the cache is created with NO slot) and the next physics frame opens
  with a resim. Every replayed tick is NoSlot for the new id, `integrateAll` skips it, and the
  systems executor still walks it;
* the game-thread registration tear: `storage.add` lands between two phases of one step, in any
  role.

⛔ Do not "fix" this by changing `dAttackRadialSimulation::State`'s default `currenSequenceId = 0`.
It is wire-visible initial state on both peers, so changing it is a separate decision, not part of
this guard.

**Verified 2026-09-23, from `dAttackRadialSimulation::integrate`.** The deactivate branch taken
when `attackTimer >= getDuration()` leaves the InitialConditions sequence VALID and sets the
State's to Invalid. A gate on the InitialConditions sequence would therefore detect once more on
every swing's deactivation tick, against the idle pose `deactivate` just wrote, which
`collisionCheck` never did.

**Verified 2026-09-24 (task 9 Rework (1)), the invariant behind the one remaining `OG_CHECK`.**
`integrate` ends in exactly one of these states:
* Hadouken sentinel → `currenSequenceId = Invalid`.
* InitialConditions Invalid, State active → `deactivate` → `attackTimer = 0`, `currenSequenceId = Invalid`.
* InitialConditions Invalid, State Invalid → the idle pose, still Invalid.
* `currenSequenceId != activeAttackSequence` → `setInitialConditions` (`attackTimer = 0`,
  `currenSequenceId = activeAttackSequence`), and the SAME call continues into one of the next two.
* The swing branch (`attackTimer < getDuration()`) → `attackTimer += deltaSeconds`, so
  `attackTimer >= deltaSeconds > 0`.
* `attackTimer >= getDuration()` → `deactivate` → Invalid.

So any pair that passes the gate after an `integrate` has `attackTimer > 0`, and the default pair
is stopped by the gate before the check. A correction restore writes an end-of-tick State and
InitialConditions, which is a state some `integrate` left, so it satisfies the same invariant.
The check is still live: a mid-swing pair with `attackTimer` forced to 0 fires it.

**Re-checked 2026-09-24 for task 20, the one new reading point.** At `preIntegrate(T+1)` the
detector reads the end-of-T pair, which is exactly what the post-integrate pass of T read: nothing
writes the radial `State` or `InitialConditions` between those two points (the systems write only
`DerivedState`; the physics step writes bodies; `captureBodyStatesAll` writes `bodyState`, not the
pair or the timer). The new case is the FIRST step of a replay, where the pair was not left by an
`integrate` in this process but restored by `prepareResimAll`. It still holds, for the reason the
task-9 Rework (1) review gave (`impl/review_defect_9_rw1.md` §2): the radial `InitialConditions` and
`State` are both wire slices of the same composite (`SimulatableBrawlerTypes.h`), restored together
from ONE authority tick; that tick's pair was left by the authority's `integrate`, so it obeys the
invariant; and `attackTimer` travels as raw float bytes (no quantisation, `SimulationSerialization.h`
Tier 3), so a restored `dt` cannot round to 0. A replay anchored on a tick where the authority had
not yet integrated the character carries the default pair, which the gate returns on. The restore
completes before `preIntegrate` of the first replayed step runs, so the detector never sees half
of it. Pinned before `integrate` by
`HitDetection.Behaviour.FreshCharacterMeetsTheDetectorBeforeItsFirstIntegrate` (live and resim).

**What breaks if the edit is made.**
* Gate on the InitialConditions sequence alone: an extra detection pass on each swing's last
  tick, against a weapon pose that is no longer the swing's. Nothing fails to compile and the
  existing hit tests mostly still pass, because their overlap schedules rarely reach the final tick.
* Gate on `currenSequenceId != InvalidAttackSequenceId` alone, or on that plus `attackTimer > 0`
  without the pair equality (task 20's spec step 1, superseded): the default `currenSequenceId = 0`
  is not Invalid, so the first form lets the default pair through and fires the
  `attackTimer > 0` check (a `checkf` in a UE Development build) on EVERY new character's first
  `preIntegrate`; the second returns on it but drops the pair equality, which is the semantic gate.
* Assert on the pair before the returns (the shape shipped in task 9): a **crash at join time**.
  The default pair fails `currenSequenceId == activeAttackSequence`, and `OG_CHECK` is a `checkf`
  in a UE Development build, so a client crashes when a pawn joins during a resim. Shipping
  compiles the check out and falls through to the return. Pinned by
  `HitDetection.Behaviour.NeverIntegratedBrawlerIsSkippedByTheDetector` (it goes RED with the
  checkf message) and `HitDetection.Behaviour.MidSwingBrawlerStillDetectsBehindTheGate` (the gate
  is not a no-op).

---

## G-02 — The projection onto the swing plane uses the SIGNED axial distance

**Tag site:** `BrawlerHitDetectionSystem.h`, on `const float signedDistanceAlongRotationAxis = glm::dot(...)`.

**The fence, verbatim:**

<!-- DAttackRadialSimulation.h lines 527-538 at b572456 -->
```
		// [movement-sim task 33] SIGNED, AND IT MUST STAY SIGNED. This is hitDirection's
		// component ALONG the swing axis; subtracting it below is what projects the hit onto
		// the swing plane. This site used the ABSOLUTE value for that subtraction until task
		// 33, which is correct only for a hit ABOVE the plane -- for one BELOW it the axial
		// component is doubled AWAY from the plane instead of removed: (75, 0, -35) became
		// (75, 0, -70), not (75, 0, 0). hitDistance was therefore inflated for below-plane
		// hits only, and since it gates BOTH ends of the annulus the strike zone below the
		// plane was displaced INWARD -- the swing lost outward reach below the plane and
		// landed phantom hits inside the inner hole. A real behavioural fix, not hardening.
		// Pinned by DAttackRadialSwingPlaneTest.cpp
		// "DAttackRadial.MirroredHitsAreTheSameDistanceFromTheSwingAxis" (mirror-image hits
		// measured 75.0 above vs 102.5914 below before the fix; 75.0 / 75.0 after).
```

**Still pinned** by `DAttackRadial.MirroredHitsAreTheSameDistanceFromTheSwingAxis`, which since
task 9 drives this header's `detectRadialHits` after the radial's `integrate`.

---

## G-03 — `glm::abs`, never unqualified `abs`, and the magnitude feeds the half-thickness gate only

**Tag site:** `BrawlerHitDetectionSystem.h`, on `const float lengthAlongRotationAxis = glm::abs(...)`.

**The fence, verbatim:**

<!-- DAttackRadialSimulation.h lines 541-561 at b572456 -->
```
		// [movement-sim task 32] glm::abs, NOT unqualified abs. glm::dot returns a float
		// here, and an unqualified `abs` in a non-dependent expression binds at this
		// header's POINT OF DEFINITION -- so which overload wins is a property of the
		// include set, i.e. of the toolchain. og-brawler targets a Godot port and a Jolt
		// adapter where only C's `::abs(int)` may be in scope; under that overload this
		// line becomes `(float)abs((int)dot(...))` and throws away the FRACTION.
		// ⚠ This dot is a SIGNED DISTANCE in cm along the rotation axis, NOT a cosine:
		// hitDirection is a raw world-space delta, not a unit vector. Truncation therefore
		// bites at EVERY magnitude, not only inside (-1,1) -- 5.7 cm reads as 5 cm -- and the
		// value gates `< getHalfThickness()` below, so bodies the swing passes cleanly under
		// would start registering hits.
		// Task 32 measured that this was ALREADY binding the float overload on this
		// toolchain (MSVC 14.38), so the change is PORTABILITY HARDENING and not a
		// behaviour fix. glm::abs cannot resolve to an integer overload for a float.
		// Pinned by DAttackAbsQualificationTest.cpp
		// "DAttackAbs.RadialAxisDistanceKeepsItsFraction" (0 hits vs 1 under an int overload).
		// ⛔ [movement-sim task 33] The UNSIGNED magnitude, and it feeds the half-thickness
		// gate below and NOTHING else. A distance FROM a plane has no sign, so glm::abs is
		// correct there and was never the defect. Do not reuse this for the projection.
		// (Task 32's note above still describes this call; only the dot product it wraps
		// moved one declaration up, so the same operand reaches the same glm::abs.)
```

⚠ **R0, 2026-09-23.** "This header's POINT OF DEFINITION" now means `BrawlerHitDetectionSystem.h`,
whose include set is much richer than the radial header's (it includes `SimulatableBrawler.h`).
`DAttackAbsQualificationTest.cpp` says so at its include of this header. The prohibition is
unchanged and the qualified call makes the include set irrelevant.

---

## G-04 — The swing timer is the PRE-advance timer: `attackTimer - deltaSeconds`

**Tag site:** `BrawlerHitDetectionSystem.h`, on `const float swingTimer = state.attackTimer - deltaSeconds;`.

**The prohibition.** ⛔ Do not sample the sequence at `state.attackTimer`. `collisionCheck` ran
inside `integrate` BEFORE `state.attackTimer = state.attackTimer + deltaSeconds`; the detector
runs after it. The pose it tests was produced at the pre-advance timer, so that is the timer the
authored angular velocity must be read at.

**Task 20.** The detector now runs in `preIntegrate(T+1)`, so `deltaSeconds` is T+1's step. The
subtraction recovers T's pre-advance timer only because the step is fixed (the same
`SimulationTimeStep` delta on every tick of a session); a variable step would need T's own delta.

**What breaks if the edit is made.** The swing tangent's sign is read one tick into the future.
It flips wherever the authored angular velocity changes sign between two consecutive ticks, which
throws the target the wrong way on that tick.

**Measured, and what was not.** `DAttackRadial.SwingTangentFollowsTheSwingDirection` compares the
tangent's sign against `getAngularVelocity` sampled at the timer read BEFORE `integrate`, and
passes through this detector. ⚠ The subtraction can differ from the pre-advance value by float
rounding. Only the SIGN of the result is read, so that matters only at an exact zero crossing of
the authored angular velocity. That was not surveyed across the shipped sequences.

---

## G-05 — Sign the tangent with the AUTHORED angular velocity, never the captured one

**Tag site:** `BrawlerHitDetectionSystem.h`, on `const float authoredAngularVelocity = activeAttackSequence.getAngularVelocity(swingTimer);`.

**The fence, verbatim:**

<!-- DAttackRadialSimulation.h lines 606-625 at b572456 -->
```
	// [movement-sim task 83] THE SWING TANGENT AT A HIT -- the direction the weapon is travelling
	// through the hit point, which is the direction the hit throws its target.
	//
	// cross(axis, r) is the direction of INCREASING angle: glm::rotate is right-handed and this
	// sim drives the weapon with setBodyAngularVelocity(axis * w) / addBodyTorque(axis * a * I),
	// so +w carries a radial vector r toward cross(axis, r). Signing that by the sequence's
	// angular velocity turns "the tangent" into "the direction of travel" -- the opposite sign
	// would throw the target INTO the weapon. Sequence 0 runs -pi/2 -> +3pi/8 (w > 0) and
	// sequence 1 mirrors it (w < 0), so left and right throws mirror without a second rule.
	//
	// AUTHORED w, NEVER the captured body angularVelocity. The captured value is produced by the
	// engine's own integration, so it differs between peers and between a live tick and its
	// replay; the table value is identical everywhere. A gameplay decision must not read an
	// engine number.
	//
	// Recomputed here from whichever query hit is actually pushed, rather than carried down from
	// the loop above: the body-only branch and the guard branch below push DIFFERENT hits, and a
	// tangent belonging to a different shape than the recorded position would be a silent lie.
	// The projection is the same one the annulus test uses, so the tangent is taken at the
	// target's own hit radius -- the direction the swing actually touched it.
```

⚠ **R0, 2026-09-23.** "the sequence's angular velocity" is now sampled at `swingTimer`, not at
`state.attackTimer`. See G-04 for why the two differ once `integrate` has advanced the timer.

---

## G-06 — A hit on the rotation axis gets the ZERO tangent, never `normalize()`'s NaN

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `if (glm::dot(onPlane, onPlane) <= 0.f)` early return inside `swingTangentAt`.

**The fence, verbatim:**

<!-- DAttackRadialSimulation.h lines 633-635 at b572456 -->
```
		// A hit sitting exactly on the rotation axis has no tangent. Return the zero vector
		// rather than normalize()'s NaN; hit routing reads that as degenerate and falls back to
		// the away-from-attacker direction.
```

**What breaks if the edit is made.** `DAttackHit::swingTangent`'s declaration promises the field
is never a NaN, and its one reader (routing branch 2) relies on that promise.

⚠ **R0, 2026-09-23, read from `BrawlerHitRoutingSystem.h`.** Routing would survive a NaN today,
because of how its comparison is written, and the fence is what keeps that comparison's direction
from mattering. The note describes routing's code, so task 29 moved it, verbatim, to
`BrawlerHitRoutingSystem-rationale.md` §4.3.

---


## G-08 — The `break` after a guard block is preserved AS-IS

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `break;` that follows the guard-block push.

**The prohibition.** ⛔ Do not delete it and do not turn it into a `continue`. A blocked swing
stops evaluating EVERY remaining actor for that attacker on that tick, including actors the swing
would otherwise have hit. That is a quirk, and task 9 preserves it on purpose (its AC: "preserved
as-is and named in the notes"): a move that also changed behaviour could not be verified as a
move.

**What breaks if the edit is made.** Multi-target swings change outcome on the tick one target
blocks. The two actors' order in the overlap report then decides which of them the swing still
hits, and nothing in this header makes that order deterministic across peers. Retiring the quirk
needs its own ruling and its own test.

---

## G-09 — `kRoleAffinity` stays `AllRoles`

**Tag site:** `BrawlerHitDetectionSystem.h`, on `static constexpr SystemRoleAffinity kRoleAffinity`.

**The prohibition.** ⛔ Never `AuthorityOnly`. Clients PREDICT: the machine's recoil into
`GuardFlinch` and a target's `HitFlinch` are predicted from this detector's output on every role,
and a resim replay re-detects through it.

**What breaks if the edit is made.** Clients detect nothing: `SimulationSystemsExecutor` skips an
`AuthorityOnly` system on every non-authority role, silently. Every predicted swing whiffs until a
correction arrives, and a client's replay re-detects nothing either.

---

## G-10 — Constructed from the SAME adapters the integration layer uses

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `System` constructor.

**The prohibition.** The system stores pointers to the body adapter and the query adapter it is
constructed with, and both must be the objects the per-character integrate uses: the body adapter
that holds the guard transform the target's integrate wrote, and the query adapter whose shapes the
guard sub-simulation toggled. ⛔ Never a game-thread reader. The UE composition root's version of this
guard is `SimulationManagerUImpl-guards.md` G-75, which names the concrete wrong adapter.

**What breaks if the edit is made.** The detector reads a DIFFERENT view of the target's guard
than the one the target's integrate wrote, which is task 9's read (b) reintroduced.
`HitDetection.ExecutorFiresDetectionWithTheAdaptersItWasConstructedWith` pins the identity on the
LLT side.

---

## G-11 — Walk the attackers sorted by id, never in `StorageView` order

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `std::sort` over `ordered`.

**The prohibition.** ⛔ Do not iterate the view directly. Its order is unordered-map order,
unspecified and machine-varying (the `SimulationSystemsExecutor` library contract).

**Read 2026-09-23, and why the sort is still required.** Today each attacker writes only its OWN
radial `DerivedState` and the parent transforms of its OWN query volumes. It reads other
characters' guard transforms and shape toggles, which nothing in this walk writes. Its overlap
asks for `bodyAndGuard` hits, and query volumes carry `queryRouting`, so one attacker's volume
placement should not change another's report. On that reading the walk is commutative and the
sort changes no outcome YET. The adapter's filtering is not re-measured here. It
is here because the first cross-attacker effect makes it load-bearing without anyone noticing:
Backlog task 17 moves projectile detection into this walk, including the
projectile-versus-projectile cancel, in which two characters' slots decide each other's fate.

**Re-read 2026-09-24, task 17 landed.** The projectile pass walks the shooters in this sorted
order, but it was built so the order cannot matter: it queries every slot in flight first,
writing nothing another query reads, then cancels a pair if EITHER query saw the other (a union),
then classifies (`BrawlerProjectileHitDetection-rationale.md` ∴D-02). So the sort still decides
no outcome. It stays, because the next cross-attacker effect may not be built that way.

---

## G-12 — Nothing in the walk logs without printing `id=` itself

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `for` loop over `ordered`.

**The prohibition.** The `id=` / `tick=` prefix on `OGBLOG_G` lines comes from
`simulationLog::IntegrateScope`, which `SimulationIntegrationExecutor::integrateAll` sets around
each character's `integrate` and around nothing else. This walk runs in `firePreIntegrate` (since
task 20; `firePostIntegrate` before it), outside that scope. ⛔ A log line added here must print the character id (and the tick, from
`step`) itself, as the routing system's lines are expected to.

**What breaks if the edit is made.** The line is emitted with no id prefix, and the per-character
greps every PIE capture in this initiative relies on silently lose it.

**Verified 2026-09-23.** The old `collisionCheck` had zero log sites, so the move lost nothing. The
detector has none either, and task 17's projectile pass (`BrawlerProjectileHitDetection.h`) added
none: its `[Projectile.*]` lines stay in the projectile sub-simulation's `integrate`, which prints
them when it ends the slot, inside the scope. ⚠ A line added here should print `step.getTick()` and
say which half it belongs to: the melee pass reads the end state the previous `integrate` left, and
the projectile pass tests each shot at its closed-form position on `step.getTick()` itself.

⚠ **Retired from this entry, 2026-09-24 (task 17; task 20 review N1).** Task 20 Rework (1) put a
per-step-kind table here: how far back routing branches 3 and 4 had to look for a projectile slot's
`endTick` (a per-step-kind offset function in `BrawlerHitRoutingSystem.h`: Normal, Stall and a replay 1, a graduated Skip
2, and a HardResync it could not see). The table was never this guard's subject; the task-20
review found it under the wrong heading. Task 17 removed the lookup itself at the user's ruling
("a system must not need to know which tick ran last"): branches 3 and 4 read the projectile
outcome this detector produces in the same pass, so the table has no subject left and was deleted,
not moved. Its pins were retired or re-homed with it (the routing table case retired, the Skip
cases re-homed onto the detector, a HardResync case added); see `BrawlerHitRoutingTest.cpp` and
`BrawlerProjectileHitDetection-rationale.md` §3. This entry keeps only its own rule, above.

---

## G-13 — The detector clears its own per-tick outputs FIRST, before any return

**Tag site:** `BrawlerHitDetectionSystem.h`, on the three clears (`hitsThisTick`,
`guardBlockedThisTick`, `guardHits`) that open the slice-level `detectRadialHits`, above G-01.

**The prohibition.** ⛔ Do not move these below G-01 or any other `return`, and do not delete them
on the ground that the radial's `integrate` already clears two of them
(`DAttackRadialSimulation-guards.md` G-04, G-05). Every pass must publish exactly what it detected
for its character, and nothing left from an earlier pass.

**Why the detector must own them (task 20, 2026-09-24).** The pass runs in `preIntegrate(T+1)` and
the signals must live from there to the next pass:
* `guardHits` is what the visualization draws (the blue guard-hit sphere). The snapshot
  (`updateVisualizationAll`) is taken after T+1's physics step. A guard block makes the machine go
  `GuardFlinch` on T+1, and the radial's `deactivate` on T+1 used to clear `guardHits`, so after
  the move the sphere would have been erased before any snapshot saw it. `deactivate` no longer
  clears it; this clear is its only one.
* A character the replay does not integrate (the resim NoSlot row, task 9 behaviour review F9)
  never runs the radial's top-of-`integrate` clears, so its `hitsThisTick` from the previous pass
  would be routed a second time.
  ⚠ **R0, task 27 (user ruling 2026-09-26).** Since the ledger is synced and recorded only by
  `integrate`, a mid-swing NoSlot attacker RE-DETECTS its target on the next pass and routes it
  again anyway (`DAttackRadialSimulation-rationale.md` §4.4). What this clear still guarantees is
  ONE entry per pass instead of the stale one plus the new one.

**What breaks if the edit is made.** Both measured by poison (task 20): with the three clears
removed, `HitDetection.Behaviour.TheDetectorOwnsThePerTickSignalsItWrites` fails both sections —
`guardHits` stays 1 forever, and the un-integrated attacker's hit reaches the target's slice again
(`wasHit=1`). Re-measured after task 27, where the second section is re-pinned: `guardHits` 1, and
`hitsThisTick` holds 2 entries instead of 1. With `deactivate` still clearing `guardHits`, its first section fails instead
(`guardHits after integrate(T+1)=0`). The whole `[@og]` suite was green without the clears before
that case was written.

**The projectile outcome follows the same rule (task 17, 2026-09-24).** The projectile pass writes
each shooter's `brawlerProjectileSimulation::DerivedState::detectedThisTick`, and it resets every
shooter's entries before it queries anything. That reset has its own site and id in the other
header: `BrawlerProjectileHitDetection-guards.md` G-01 (it is a separate statement in a separate
file, so it cannot share this tag).

---

## G-14 — The projectile pass is handed the step's OWN tick

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `detectProjectileHits(step.getDeltaSeconds(), step.getTick(), ...)`
call at the end of `preIntegrate`.

**The prohibition.** ⛔ Do not pass anything but `step.getTick()`: not `step.getTick() - 1`, not a
tick adjusted by the step kind, not a "last integrated" tick kept anywhere. The pass tests each
shot at its closed-form position on that tick, which is the position this step's `integrate` snaps
it to and ends it at. User ruling 2026-09-24 (task 17): a system must not need to know which tick
ran last, and the reaction lands in this step (`BrawlerProjectileHitDetection-rationale.md` §3).

**What breaks if the edit is made.** With `step.getTick() - 1` the pass tests where the shot was
one tick ago while `integrate` ends it where it is now; on a graduated Skip it tests a tick
`integrate` never visits. Measured by poison (task 17): only
`HitDetection.Projectile.CancelEndsBothShotsOnTheSameTickInBothOrders` fails (12 assertions). ⚠ The
other projectile cases schedule their contact rather than place it, so they cannot see the
position move; this is a weak pin.

---

## G-15 — The cap reads the SYNCED ledger, and the pass stops at three

**Tag site:** `BrawlerHitDetectionSystem.h`, on `if (recordedTargets >= kMaxHitTargetsPerSwing) return;`
(og-netcode-v2-field-defects task 27).

**The prohibition.** The swing's hit count is the number of non-`None` entries in the radial's
synced `State::hitTargets`, read off the `State` that `integrate` left (the same read point as the
G-01 pair). Do not count anything derived instead: not `hitsThisTick`, not `guardHits`, and not a
per-character container kept in this system. A derived count is not restored by a correction or
adopted from the authority, which is the defect task 27 removed. Do not relax the comparison to `>`
and do not raise it: the cap is **exactly at most `kMaxHitTargetsPerSwing` (3) per swing, including
within one pass** (user ruling R3). The pass loop's `recordedTargets + hitsThisTick.size() >= kMaxHitTargetsPerSwing`
`break` is the in-pass half of the same rule.

**What breaks if the edit is made.** A wider cap, or dropping the in-pass `break`, makes the fourth
target of `HitDedup.ASwingHitsAtMostThreeDistinctTargets` register. Without the in-pass `break`, the
radial's `recordHitTargets` `OG_CHECK` ("more hits than the ledger holds") aborts on the
four-in-one-pass section (measured, task 27). A derived count brings back the replay and adoption
misses that `…BodyHitTickFlinchesTheTarget` ("frontier mid-swing") and
`HitDedup.AdoptingAnIdleAttackerMidSwingLetsTheNextSwingHit` pin.

**Score (§9.1):** substitution on the tagged comparison → `yes`. The in-pass `break` is a separate
statement, not covered (clause B). It is held by the test and the `OG_CHECK`, not by this tag.

---

## G-16 — Dedup is by the peer-stable `SimCharacterId`, and an unresolved root is not a character

**Tag site:** `BrawlerHitDetectionSystem.h`, on
`if (targetId == SimCharacterId::None || isRecorded(targetId)) continue;` in the report loop.

**The prohibition.** Skip a report hit whose root resolves to a target already in `hitTargets`, and
skip one whose root resolves to no registered character. ⛔ Do not dedup by `rootBodyId` (a
per-process engine handle; it is the pre-task-27 key), and do not register a hit with
`targetId == None`. Routing cannot deliver it (routing's own map has no entry for it), and the
radial's `recordHitTargets` `OG_CHECK`s that every recorded id is real.

**Behaviour change, recorded (task 27).** Before task 27 a hit on a body that was not a registered
character's capsule was still registered: it went into the derived ledger (using up a cap slot) and
into `hitsThisTick`, and routing then dropped it. It is now skipped here. Routing never delivered
it, so a target sees no difference, but it no longer consumes one of the swing's three slots.

**What breaks if the edit is made.** Dedup by `rootBodyId` against a ledger of ids compares
unrelated numbers, so the same target would be registered on every Damaging tick, and the radial's
duplicate check would abort (derived, not run). Letting `None` through aborts on the radial's
`None` check (derived, not run).

**Score (§9.1):** substitution on the tagged condition → `yes`.

---

## G-17 — `registerAttackHit` writes the per-tick signal ONLY

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `registerAttackHit` lambda (task 27; it replaces
retired G-07 at the same site).

**The prohibition.** The lambda pushes to `hitsThisTick` and nothing else. ⛔ Do not add a write to
the radial's `State::hitTargets` here, or to any other ledger. A system never writes wire state.
The ledger's only writer is the radial's own `integrate` (`DAttackRadialSimulation-guards.md` G-07),
which records `hitsThisTick[].targetId` on the attacker's next integrate.

⚠ **Moved by task 29.** This entry also said "⛔ Do not route from the ledger either". That edit is
typed in routing's branch 2, not at this tag (§9.1 `no (elsewhere)`), so it is now
`BrawlerHitRoutingSystem-guards.md` G-04, at the loop it forbids. This id keeps its own edit.

**What breaks if the edit is made.** A second writer here would record the target a tick early and
twice. The radial's `recordHitTargets` would then `OG_CHECK` on the duplicate ("the detector
registered a target the ledger already holds"; derived, not run). It would also change the NoSlot behaviour the user
ruled on (`DAttackRadialSimulation-rationale.md` §4.4): an attacker the step does not integrate
would record its hits.

**Score (§9.1):** addition on the tagged statement → `yes`.

---

## G-18 — The target-id map is keyed by the capsule `BodyId`, and its value is the STORAGE KEY

**Tag site:** `BrawlerHitDetectionSystem.h`, on `m_targetIdByRootBodyId[rootBodyId] = static_cast<SimCharacterId>(id);`
in `onCharacterRegistered`.

**The prohibition.** The value is the character's storage key, which is its peer-stable
`SimCharacterId` since task 25. ⛔ Do not store a registration counter, a storage index, a pointer,
or any other number that depends on the order this process registered characters in. Two peers
register in different orders (task 26's two-rig pin reverses it), so such a value would differ
between them and reach the wire through the ledger. The key is the capsule `BodyId`, which is what
the query adapter reports as `rootBodyId` for every shape of the character. Routing's
`m_byRootBodyId` uses the same key.

**What breaks if the edit is made.** `HitDedup.SyncedBytesMatchAcrossPeersWhoseEngineHandlesDiffer`
would go RED, because its peer B registers the target first (derived, not run). The `OG_CHECK` above the tag enforces only
that the key fits in the 1-byte id.

**Score (§9.1):** substitution on the tagged token → `yes`.

---

## §R — Retired ids

### G-07 — RETIRED by og-netcode-v2-field-defects task 27 (2026-09-26)

**Was:** "Every accepted hit is recorded TWICE: the per-swing ledger AND the per-tick signal", on the
`registerAttackHit` lambda. The lambda pushed to the radial's derived per-swing ledger and to
`hitsThisTick`.

**Why retired.** Its subject is gone. The derived ledger was deleted, and the per-swing record is the
synced `State::hitTargets`, written by the radial's `integrate`. The lambda now records once. Its
remaining prohibitions moved to new ids with a different subject: **G-17** (the lambda writes the
per-tick signal only; "do not route from the ledger" moved on to `BrawlerHitRoutingSystem-guards.md`
G-04 in task 29), **G-15** (the cap), **G-16** (the dedup key),
and the radial's **G-07** (the ledger's writer). The shipped fence text and the rest of the entry are
kept in `BrawlerHitDetectionSystem-rationale.md` §9. ⛔ The number 7 is spent.

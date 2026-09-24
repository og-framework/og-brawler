<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerHitDetectionSystem.h` — guards

Every prohibition for the melee hit detector. Each entry has an **opaque, stable id**; in the
header a single line `// ⛔G-nn` sits exactly where the wrong edit would be typed.

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

## G-01 — The swing gate is the post-integrate sequence PAIR, and it returns BEFORE anything asserts

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `if (state.currenSequenceId == InvalidAttackSequenceId || initialConditions.activeAttackSequence == InvalidAttackSequenceId || state.currenSequenceId != initialConditions.activeAttackSequence || state.currenSequenceId >= staticData.getAttackSequences().size()) return;`
that opens the slice-level `detectRadialHits` (one statement over four lines), and the `OG_CHECK(state.attackTimer > 0.f, …)` beneath it.

**The prohibition.** Detection runs after the radial's `integrate` has returned, so "was this a
swing tick" must be read off the state that `integrate` LEFT. Post-integrate, the radial's
`currenSequenceId` is valid if and only if `integrate` took its swing branch this tick, which is
exactly the branch that used to call `collisionCheck`: the Hadouken sentinel, the two
`deactivate` exits and the idle pose all leave it `InvalidAttackSequenceId`. ⛔ Do not gate on
`activeAttackSequence` alone, and do not re-derive the answer from `attackTimer`: a swing's first
tick ends with `attackTimer == deltaSeconds`, and the sequence pair, not the timer, is the
semantic gate.

⛔ **Every condition is a `return`. Nothing is asserted until all of them have passed.** The
detector walks EVERY character in storage, and not every character in storage has been
integrated. A brawler added to storage whose first `firePostIntegrate` precedes its first
`integrate` still carries the radial's default pair: State `currenSequenceId = 0`, InitialConditions
`activeAttackSequence = InvalidAttackSequenceId`. It reaches the detector in two ways:
* a client registers a pawn (the cache is created with NO slot) and the next physics frame opens
  with a resim. Every replayed tick is NoSlot for the new id, `integrateAll` skips it, and
  `firePostIntegrate` still runs;
* the game-thread registration tear: `storage.add` lands between `integrateAll` and
  `firePostIntegrate`, in any role.

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

**What breaks if the edit is made.**
* Gate on the InitialConditions sequence alone: an extra detection pass on each swing's last
  tick, against a weapon pose that is no longer the swing's. Nothing fails to compile and the
  existing hit tests mostly still pass, because their overlap schedules rarely reach the final tick.
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
`state.attackTimer`. See G-04 for why the two differ post-integrate.

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

⚠ **R0, 2026-09-23, read from `BrawlerHitRoutingSystem.h`.** Routing would survive a NaN TODAY,
by accident: `normalisedXY` returns its fallback unless `lengthSq > 0.f`, and that comparison is
false for a NaN. The equivalent rewrite `lengthSq <= 0.f ? fallback : ...` would not fall back,
and the NaN would reach the target's knockback velocity. The fence is what keeps routing's
comparison direction from mattering.

---

## G-07 — Every accepted hit is recorded TWICE: the per-swing ledger AND the per-tick signal

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `registerAttackHit` lambda.

**The fence, verbatim:**

<!-- DAttackRadialSimulation.h lines 642-644 at b572456 -->
```
	// [movement-sim task 83] Every accepted hit is recorded TWICE, and the two containers mean
	// different things: attackHits is the per-swing dedup ledger the loop above reads back,
	// hitsThisTick is the one-tick signal hit routing consumes. See DerivedState.
```

**What breaks if the edit is made.** Drop the ledger push and the same target is hit on every
Damaging tick of the swing. Drop the signal push and routing never sees the hit. Route from the
ledger instead and one hit re-fires on every remaining tick (the 13 m knockback of movement-sim
task 83). The other half is `DAttackRadialSimulation-guards.md` G-04 and `DAttackRadialSimulation-rationale.md` §4.

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
constructed with, and both must be the objects the per-character integrate uses this tick: the
body adapter that sees this tick's guard transform, and the query adapter whose shapes the guard
sub-simulation toggled. ⛔ Never a game-thread reader. The UE composition root's version of this
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

---

## G-12 — Nothing in the walk logs without printing `id=` itself

**Tag site:** `BrawlerHitDetectionSystem.h`, on the `for` loop over `ordered`.

**The prohibition.** The `id=` / `tick=` prefix on `OGBLOG_G` lines comes from
`simulationLog::IntegrateScope`, which `SimulationIntegrationExecutor::integrateAll` sets around
each character's `integrate` and around nothing else. This walk runs in `firePostIntegrate`,
outside that scope. ⛔ A log line added here must print the character id (and the tick, from
`step`) itself, as the routing system's lines are expected to.

**What breaks if the edit is made.** The line is emitted with no id prefix, and the per-character
greps every PIE capture in this initiative relies on silently lose it.

**Verified 2026-09-23.** The old `collisionCheck` had zero log sites, so the move lost nothing. The
detector has none either.

---

## §R — Retired ids

*(None.)*

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `DAttackRadialSimulation.h` — guards

Every prohibition for the radial (weapon-swing) sub-simulation that is still a prohibition after
the compiler and the checks took their share. Each entry has an **opaque, stable id**. In the
header, a single line `// ⛔G-nn` sits exactly where the wrong edit would be typed.

**If this file and `DAttackRadialSimulation.h` disagree, the header is authoritative and this
file is stale.** Fix this file. Do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or run-time
check, moves to [§R, Retired ids](#r-retired-ids) and its number is spent forever.

⛔ **Nothing in this file is a rationale.** The law, the provenance and every removed comment,
verbatim, live in `DAttackRadialSimulation-rationale.md`.

**Provenance.** Converted by og-netcode-v2-field-defects task 19 (2026-09-23) from the header as it
stood after task 9 (672 lines, md5 `5fce5c9f…`). Quoted fence text is the shipped bytes of that
header, cited as `<!-- pristine lines A-B -->`. The full comment archive is rationale §A.

**Not here, because a check holds them now** (rationale §6): `StaticData` non-copyable
(`static_assert`), `DerivedState` reserve-not-resize (`OG_CHECK`), and the Hadouken-sentinel
return staying ahead of `setInitialConditions` (`OG_CHECK`). Two more were already compiler-held
before this task: the `RuntimeBindings` alias and the `staticDataOf` member template.

---

## G-01 — `PlayerInput::zero()` carries `(0,0,1)`, never `PlayerInput{}`

**Tag site:** `DAttackRadialSimulation.h`, on `static PlayerInput zero()`.

**The fence, verbatim** <!-- pristine lines 251-253 -->:
```
	// ⛔ (0,0,1) forwards, NOT PlayerInput{}: a value-initialised (0,0,0) aim would
	// reach normalize(), and the difference is also the TAG the input-resolution and
	// net-sync anti-vacuity tests discriminate on. Keep zero() != PlayerInput{}.
```

**The prohibition.** Do not substitute `zero()`'s return value with `PlayerInput{}`, or with any
aim other than `glm::vec3(0.f, 0.f, 1.f)`. The value was copied verbatim from what
`getZeroPlayerInput()` returned before movement-sim task 22 folded the neutral input into
`SimulationComposite::zero()`. It is a wire value, not something to re-derive.

**What breaks if the edit is made.**
* ⛔ **Not a NaN hazard; corrected at task 19.** Both halves of the fence's first sentence are false.
  `(0,0,1)` is +Z, which is `defaultUp`, not forwards: `DAttackRadialSequence::defaultForward()` is
  `(1,0,0)`. And no reader normalizes the raw aim. The machine, the guard and
  `DAttackDirectionClassifier.h` all project to XY first, where `(0,0,1)` and `(0,0,0)` are the same zero
  vector, and the guard substitutes `defaultForward` below length 1e-4. The radial never reads its own
  slice's aim. Task 18 found the same sentence false in the machine header, and task 19 confirmed it here.
* `SimulationInputResolutionTest.cpp`'s `isGameZeroInput` tells the injected neutral input apart
  from a value-initialised one only by this aim. If the two are equal, every anti-vacuity pair it
  anchors keeps passing while testing nothing.
* The zero input's wire bytes change. `DAttack.SimulatableBrawler.ZeroInputIsTheFold` pins them
  byte for byte, so that case goes RED. This was derived from the test's byte-identity assertion
  and was not run as a mutation.

**Verified 2026-09-23 (R0).** Both consumers and the test were read in the tree.

**Score (§9.1): substitution on the tagged statement → `yes`.**

---

## G-02 — `aimDirection` keeps a `(0,0,0)` default

**Tag site:** `DAttackRadialSimulation.h`, on `glm::vec3 aimDirection{};` in `PlayerInput`.

**Source.** This is the second way to break G-01's `zero() != PlayerInput{}`, and G-01's fence
does not name it. The fence text lives in `SimulatableBrawlerTest.cpp`'s banner for
`DAttack.SimulatableBrawler.ZeroInputIsTheFold`, verbatim:
```
//     every one of those anti-vacuity pairs keeps passing while testing nothing.
//     DO NOT change a default member initialiser to close this gap.
```
Task 19 split it out as its own id. §9.1 clause A says to score an entry only as written and to
record an under-described entry as a finding, and this is that finding. The edit is typed on a
different statement from G-01's, so it needs its own site (§9.3, the `no (elsewhere)` row: one id
per site).

**The prohibition.** Do not give `aimDirection` a default member initialiser of `(0,0,1)`, or of
any value equal to `zero()`'s aim. That would make `PlayerInput{}` equal to `zero()`.

**What breaks if the edit is made.** The same anti-vacuity blind spot as G-01. Nothing fails, and
the neutral-input tests stop being able to see the defect they exist to catch.

**Score (§9.1): substitution on the tagged token → `yes`.**

---

## G-03 — `wouldGuardBlock`'s outer gate stays POSITIVE (`td < outerShieldAngle`)

**Tag site:** `DAttackRadialSimulation.h`, on `if (td < outerShieldAngle)` in `wouldGuardBlock`.

**The fence, verbatim** <!-- pristine lines 373-374 -->:
```
// preserved verbatim), and the POSITIVE `td < outerShieldAngle` gate (NOT a negated
// early-return) preserves the sim's "NaN falls through to attack-lands" semantics.
```

**The prohibition.** Do not rewrite the gate as an early return such as
`if (td >= outerShieldAngle) return false;`.

**Why, re-derived at task 19.** `td = std::acos(dot(...))`. If rounding pushes the dot of two unit
vectors past 1, `td` is NaN while `normalizedCollisionDirection` is still finite. With the
positive gate, `NaN < x` is false, the function returns `false`, and the attack lands. With the
negated early return, `NaN >= x` is also false. Control then falls into the inner test,
`td < shieldAngle` is false, and the `else` arm reads `guardAxis.z` from a finite, near-zero
`glm::cross`. That z can have either sign, so the swing can be reported as **blocked** on NaN.
(A NaN from `normalize` of a zero vector propagates into `guardAxis` and returns `false` under
both forms. Only the rounding NaN separates them.)

**What breaks if the edit is made.** Near-collinear guard contacts become blocks where the
shipped predicate lets them through. The detector and the block-prediction visualization both
change, and no test pins this case.

**Score (§9.1): substitution on the tagged statement → `yes`.**

---

## G-04 — The per-tick hit signal is cleared at the TOP of `integrate`, not in the detector

**Tag site:** `DAttackRadialSimulation.h`, on `derivedState.editHitsThisTick().clear();`, the first
statement of `integrate` after the two dependency reads.

**The fence, verbatim** <!-- pristine lines 203-207 and 539-542 -->:
```
	// ⛔ Do NOT move that clear into the detector: detectRadialHits early-returns when no swing
	// is active, when the swing is not Damaging, at the four-target cap and on an empty
	// overlap -- it returns early PER ATTACKER exactly as this header's detector did -- so a clear there
	// would leave the last Damaging tick's entries live for the rest of the swing -- the same
	// bug in a smaller window.
```
```
	// ⛔ Not in the detector: detectRadialHits early-returns per attacker when no swing is
	// active, when the swing is not Damaging, at the four-target cap and on an empty overlap,
	// so a clear there would leave the last Damaging tick's entries live and hit routing
	// would keep re-firing them.
```

**The prohibition.** Do not move this clear into `brawlerHitDetection::detectRadialHits`, and do
not delete it. `hitsThisTick` must mean "hits registered on THIS tick" on every tick, including
ticks that leave `integrate` early.

**Corrected at task 19 (R0).** Two things in the shipped text above are wrong:
* *"exactly as this header's detector did"* is false. The old `collisionCheck` had no swing gate,
  because it was only called from the swing branch. It also had no sequence-mismatch return. The
  detector adds both, so it returns early in five places, not three.
* *"a clear there would leave …"* is true only for a clear placed after the detector's first
  return. A clear at the very top of `detectRadialHits` would run every tick in production, because
  `brawlerHitDetection::System::postIntegrate` calls it for every character. It would still be
  wrong for three reasons. Each single-character rig calls `integrate` and then the detector
  directly, so they would still pass. Nothing else guarantees the call. And it would move the clear
  into a file that owns no part of this slice's lifetime.

**What breaks if the edit is made.** Routing (`BrawlerHitRoutingSystem.h`) iterates
`getHitsThisTick()` every pass. A stale entry re-fires the knockback, lockout and stun on
every remaining tick of the swing: the 13 m throw of movement-sim task 83.
`HitRouting.RadialHitFiresOnceAcrossTheSwing` is the end-to-end pin.

**R0, og-netcode-v2-field-defects task 20 (2026-09-24).** Detection and routing now run in
`preIntegrate` of T+1, and the detector ALSO clears `hitsThisTick` and `guardBlockedThisTick`, at
its very top, before any return (`BrawlerHitDetectionSystem-guards.md` G-13). That is an addition,
not the move this guard forbids, and it answers the "nothing else guarantees the call" objection:
the pass now runs for every character in storage on every tick, including characters `integrate`
skips. Consequences for this entry, stated plainly:
* In production this clear now runs AFTER routing has read the signal (routing is in
  `preIntegrate(T+1)`, this clear at the top of `integrate(T+1)`). Deleting it would no longer
  re-fire a hit in production, because the detector's own clear runs first on every pass. What it
  still protects: every single-character rig that calls `integrate` and the detector directly, and
  the reversed-order outcome the `SimulationManagerUImpl.h` `firesBefore` message names (routing
  before detection reads an EMPTY list because this clear ran; without it, routing would read the
  previous pass's list and route every hit one tick late instead of never).

**Score (§9.1): the move names a destination ("into the detector"), not a statement to cross, so
by clause E it is a cut at the tag, scored as deletion → `yes`.** The same entry's *"attackHits is
deliberately NOT cleared here"* is an addition of a new statement, which no tag can cover
(clause B). It lives in rationale §4, untagged.

---

## G-05 — `guardBlockedThisTick` is cleared beside `hitsThisTick`, for the same reason

**Tag site:** `DAttackRadialSimulation.h`, on `derivedState.editGuardBlockedThisTick() = false;`.

**The fence, verbatim** <!-- pristine lines 542-543 and 218 -->:
```
	// would keep re-firing them. guardBlockedThisTick is the same kind of signal and is
	// cleared here for the same reason.
```
```
	// Cleared beside hitsThisTick at the top of integrate(), for the same reason.
```

**The prohibition.** The same as G-04, for the per-tick guard-block flag: do not move it into the
detector and do not delete it. It is a separate id because it is a separate statement. G-04's tag
covers the first statement of the run only (§9.1 clause B).

**What breaks if the edit is made.** `brawlerHitRouting::System` branch 5 copies the flag onto the
attacker's inbound slice as `wasGuardBlockedThisTick`, and `dAttackMachineSimulation::integrate3`
recoils into `GuardFlinch` on the next tick. A stale `true` recoils the attacker again on every
tick until something else clears it. Verified 2026-09-23 against `BrawlerHitRoutingSystem.h`
branch 5. ⚠ Since task 20, branch 5 runs in `preIntegrate` of T+1 and the recoil is on T+1, the
tick after the block; the detector also clears the flag itself (G-04's R0 note applies here too).

**Score (§9.1): cut at the tag (clause E, destination) → `yes`.**

---

## §R Retired ids

*(None.)*

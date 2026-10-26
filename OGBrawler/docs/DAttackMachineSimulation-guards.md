<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: wasProjectileBlockedThisTick -- RETIRED by og-attackstatetransition-cleanup task 4: the inbound-slice flag that drove the projectile-block recoil (guard G-04 of DAttackMachineSimulation-guards.md). It must NOT resolve -->
# `DAttackMachineSimulation.h` — guards

Every prohibition the attack state machine's header still needs a person to read. Each entry has an
**opaque, stable id**; in the header a single line `// ⛔G-nn` sits exactly where the wrong edit would
be typed.

**If this file and `DAttackMachineSimulation.h` disagree, the header is authoritative and this file is
stale.** Fix this file; do not soften the header to match it.

⚠ **Not to be confused with `DAttackMachineSimulationRuntimeTweakables-guards.md`**, which belongs to a
different header and has its own `G-01…`. The id space is per document: a tag carries the path of the
document it resolves against. Never cite a bare `G-nn` without naming this file.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or run-time check,
moves to [§R, Retired ids](#r-retired-ids) and its number is spent forever.

⛔ **Nothing in this file is a rationale.** The derivations, the provenance and the facts that forbid no
particular edit live in `DAttackMachineSimulation-rationale.md`.

**Provenance.** Converted by og-netcode-v2-field-defects task 18 (2026-09-23). The pre-conversion file
is og-brawler `b572456` plus task 9's four uncommitted hunks (907 lines, 210 comment lines). Every
entry opens with the comment it replaces, **verbatim**, marked `<!-- header lines A-B -->` with the
pre-conversion line numbers; "above" and "below" inside a quote describe that file.

**How each entry was placed.** Each forbidden edit was scored by where it is typed relative to the tag
(substitution, addition or deletion on the tagged statement; an absence tag; a reorder; typed
elsewhere). Only edits typed on the tagged statement got a tag. Ordering constraints got none: a tag
moves with its own line, so it is not in view when the other line of the pair is moved. Those, and
the prohibitions the compiler already holds, are listed in [§C, the census](#c-census--every-prohibition-the-pre-conversion-header-stated).

---

## G-02 — `swingTickCount` repeats the radial's float additions; never `ceil(duration / dt)`, never `k * dt`

**Tag site:** `DAttackMachineSimulation.h`, `dAttackMachineSimulation::swingTickCount`, on
`t = t + dt;` inside the loop.

<!-- header lines 149-159 -->
```cpp
// This is a LOOP, and it is a loop deliberately: `ceil(duration / dt)` is NOT this number.
// DAttackRadialSimulation::integrate ends a swing on the float predicate
// `state.attackTimer < activeAttackSequence.getDuration()` with `attackTimer` accumulated as
// `attackTimer = attackTimer + deltaSeconds` from `0.f`. Repeated float addition is not
// multiplication: the shipped side swings run 0.7 s and 42 * (1/60) is 0.7 in real arithmetic but
// lands BELOW 0.7f in float, so the radial takes 43 steps where the division says 42. Nothing a
// reader can inspect tells them which side a given (duration, dt) pair falls on, and the helper
// must not guess -- so it performs THE SAME float operations in THE SAME order as the radial and
// returns the count the radial itself will reach. The agreement is pinned by
// DAttack.Integrate3.AttackEndTickMatchesTheFirstIdleTick, which drives the whole
// SimulatableBrawler for every authored sequence and the Hadouken.
```

**The prohibition.** Two edits are forbidden. (1) Replacing the loop with a division such as
`ceil(duration / dt)`. (2) Replacing the repeated addition with a multiplication (`t = k * dt`, or
counting by division). The helper must perform the same float additions, in the same order, from the
same `0.f`, with the same `<` predicate as `dAttackRadialSimulation::integrate`, whose swing ends when
`state.attackTimer < activeAttackSequence.getDuration()` first fails.

**What breaks if the edit is made.** The predicted end tick (`State::m_attackEndTick`) disagrees with
the tick the machine really returns to `Idle` on, for any duration where the two computations land on
different sides. The movement sub-simulation's attack slide then stops a tick early or late.
`DAttack.Integrate3.AttackEndTickMatchesTheFirstIdleTick` catches it at run time, for every authored
sequence and the Hadouken.

**R0 (verified 2026-09-23, measured on MSVC 14.38 with `/fp:fast /O2`).** The quote's conclusion is
true and its stated cause is not. Sequences 0 and 1 have `getDuration()` = `0.6f + 0.1f` =
`0.70000005f` (bits `3f333334`), because the constructor appends the zero-velocity point. 42 float
additions of `1.f / 60.f` land **exactly on** `0.7f` (bits `3f333333`), not below it. That is one ulp
under the duration, so the loop takes **43** steps, while `ceilf(0.70000005f / (1.f / 60.f))` is
**42**. The Hadouken's `swingTickCount(0.3f, 1.f / 60.f)` is 18 both ways.

⚠ **Why this is not a `static_assert`.** Task 18 tried
`static_assert(swingTickCount(0.6f + 0.1f, 1.f / 60.f) == 43u)` on a `constexpr` helper. It compiled,
and then **so did the poison**, a `ceil`-shaped body: MSVC's constant evaluator gives 43 where the
same division at run time gives 42. A compile-time pin cannot witness run-time float behaviour here.
It was rejected as vacuous.

**Placement.** Both edits are typed on the tagged statement. Replacing the loop deletes it; the
multiplication substitutes it.

---

## G-03 — `glm::abs`, never unqualified `abs`, in the near-pole test

**Tag site:** `DAttackMachineSimulation.h`, `setRadialSimulationInitialConditions`, on
`const bool aimEqualsForward = glm::abs(glm::abs(aimDot) - 1.f) < 0.0001f;`.

<!-- header lines 235-244 -->
```cpp
// [movement-sim task 32] glm::abs, NOT unqualified abs -- byte-identical to the
// guard site task 29 fixed in DAttackGuardSimulation.h, and fixed for the same
// reason. Under C's `::abs(int)`, which may be the only overload visible at this
// header's point of definition on the Godot/Jolt toolchains, the expression
// collapses to `|aimDot| == 1` EXACTLY: the near-pole epsilon band disappears and
// the normalize(cross(...)) below is handed a near-zero vector.
// Task 32 measured that this was ALREADY binding the float overload on this
// toolchain (MSVC 14.38): PORTABILITY HARDENING, not a behaviour fix. Pinned by
// DAttackAbsQualificationTest.cpp
// "DAttackAbs.MachineNearPoleAimTakesTheEpsilonBandBranch" (axis.z +1 vs -1).
```

**The prohibition.** Do not replace either `glm::abs` with an unqualified `abs`.

**What breaks if the edit is made.** On a toolchain where C's `::abs(int)` is the overload that binds,
the expression collapses to `|aimDot| == 1` exactly, the 1e-4 near-pole band disappears, and
`normalize(cross(defaultForward, aimDirection))` is handed a near-zero vector. On this toolchain the
edit compiles (task 18 compiled it: clean), and the quote records that MSVC 14.38 already binds the
float overload, so no build or test here fails on it. That is why a person has to read it.

**R0 (verified 2026-09-23).** The guard sub-simulation's copy
(`DAttackGuardSimulation.h`, `aimEqualsForward`) is byte-identical. The test exists:
`DAttackAbs.MachineNearPoleAimTakesTheEpsilonBandBranch` drives the legacy `integrate`, which calls
the same `setRadialSimulationInitialConditions`, and it expects the axis's `simFrame::turnComponent` +1 under the
float overload and -1 under the int one.

**Placement.** A substitution on the tagged statement.

---

## G-05 — One forward/side decision, shared with the indicator: `dAttackDirection::classify`

**Tag site:** `DAttackMachineSimulation.h`, `integrate3`, `Idle` case, on
`state.m_activeAttackSequence = dAttackDirection::classify(`.

<!-- header lines 697-700 -->
```cpp
// ⛔ ONE DEFINITION, SHARED WITH THE INDICATOR. The forward/side decision
// lives in dAttackDirection::classify so the drawn indicator and the attack
// that fires cannot disagree. This path is AUTHORITATIVE; the classifier
// knows nothing about rendering.
```

**The prohibition.** Do not replace the `classify` call with a local forward/side decision, and do
not give the drawn indicator a decision of its own.

**What breaks if the edit is made.** The attack indicator (`DAttackVisualizationUtils.h`, which calls
the same `dAttackDirection::classify`) and the attack that fires can disagree: the player is shown one
swing and gets another.

**R0 (verified 2026-09-23).** The two production callers are this statement and
`DAttackVisualizationUtils.h`. The only other caller is the classifier's own test file.

⚠ **R0 (2026-10-03, og-attackstatetransition-cleanup task 2).** There are now three production callers:
this statement, `dAttackMachineSimulation::followUpPressSequence` in this header (which side a press
during a swing asks for, guard **G-09**), and `DAttackVisualizationUtils.h`. The same prohibition
holds at the second site, and G-09 carries it there, because one tag stands on one statement. Test
files that call `classify` today: `DAttackDirectionClassifierTest.cpp`,
`IntegrateThreeAttackSelectionTest.cpp` and `AttackStateTransitionTest.cpp`, not the classifier's own
test file alone.

**Placement.** A substitution on the tagged statement.

---

## G-06 — The dual-tap branch writes no end tick

**Tag site:** `DAttackMachineSimulation.h`, `integrate3`, `Attacking` case, the
`attackState.attackTimer < 0.1` branch of the dual-press test. It is an **absence tag**: the line under
it is blank, and that is where the forbidden write would go.

<!-- header lines 752-764 -->
```cpp
// [movement-sim task 84] NO END TICK IS WRITTEN HERE, AND THAT IS DELIBERATE.
// This branch re-enters setRadialSimulationInitialConditions on EVERY tick the
// buttons stay down inside the 0.1 s window. When the active sequence is already 4
// the radial's edge predicate (`currenSequenceId != activeAttackSequence`) is
// FALSE, so the swing is NOT restarted -- the log line says "restart" and the
// weapon does not move. That no-op is pre-existing and is filed as its own Backlog
// item; it is not fixed here. Writing `tick + swingTickCount(...) + 1` on this
// branch would push the predicted end LATER on every one of those held ticks while
// the radial ended on its original schedule: an attack that never ends and a slide
// that never stops. Pinned by
// DAttack.Integrate3.DualTapRestartLeavesTheEndTickAlone.
// If that Backlog item is ever fixed so the branch DOES restart the radial, this
// non-write becomes wrong and must be revisited in the same change.
```

**The prohibition.** Do not add a `State::m_attackEndTick` write to this branch, while the branch
re-runs every tick that both buttons stay down inside the first 0.1 s of a swing.

**What breaks if the edit is made.** For a swing that is already sequence 4, every held tick pushes the
predicted end later, while the radial keeps its original schedule: an attack that never ends and a
slide that never stops. `DAttack.Integrate3.DualTapRestartLeavesTheEndTickAlone` pins it for that
path.

**R0 (verified 2026-09-23 by reading the code; not run).** The quote is true for the path it
describes: when the active sequence is **already 4**, the radial's edge predicate
(`currenSequenceId != activeAttackSequence`) is false and nothing restarts. ⚠ **It is not the only
path into this branch.** `dAttackDirection::classify` decides the `Idle` entry from the stick alone,
so both buttons pressed with the stick sideways, or a second button inside 0.1 s of a side swing,
enters `Attacking` as sequence 0 or 1. On the next tick this branch sets 4, the edge predicate is
**true**, and the radial **does** restart as sequence 4, while `m_attackEndTick` still holds the side
swing's prediction. On that path the missing write is wrong on the first branch tick, and still
correct on every later one. The pin drives a neutral stick only (sequence 4 from `Idle`), so it does
not cover that path. The owner is brawler-movement-simulation Backlog task 85; see the rationale,
§8.4.

**Placement.** Clause D: the forbidden addition is typed directly under the tag.

---

## G-07 — The chain MUST rewrite the end tick

**Tag site:** `DAttackMachineSimulation.h`, `integrate3`, `Attacking` case, the queued-chain branch,
on `state.m_attackEndTick = currentTick + swingTickCount(`.

<!-- header lines 808-812 -->
```cpp
// [movement-sim task 84] WRITE SITE 3 of 3, and the formula is site 1's. The radial
// is Invalid on this tick -- that is the gate this block sits behind -- so assigning
// a new activeAttackSequence really does re-edge it, and the new swing has its own
// duration. The end tick MUST be rewritten here: leaving the previous swing's value
// would stop the slide at a tick that has already passed.
```

**The prohibition.** Do not delete this write. The chained swing has its own duration, and the value
left over from the previous swing is a tick that has already passed.

**What breaks if the edit is made.** The movement sub-simulation reads a past end tick, clamps the
remaining count to one, and stops the slide for the whole chained swing.
`DAttack.Integrate3.ChainedSequenceRewritesTheEndTick` and the chained routes of
`DAttack.Integrate3.AttackEndTickMatchesTheFirstIdleTick` catch it at run time.

**R0 (verified 2026-09-23).** The block sits behind
`attackState.currenSequenceId == InvalidAttackSequenceId`, so the radial is idle on this tick and the
new active sequence does re-edge it. The formula is the `Idle` swing entry's, derived in the
rationale, §8.2.

**Placement.** A deletion of the tagged statement.

---

## G-08 — "Holding back" is the matcher's step and the classifier's epsilon, never a local cone or a client tweakable

**Tag site:** `DAttackMachineSimulation.h`, `dAttackMachineSimulation::wantsHeldProjectile`, on its
`return` statement (three lines; both tokens below are on its continuation line).

Added by og-attackstatetransition-cleanup task 1 (2026-10-03). It replaces no comment; there was no
held trigger before it.

**The prohibition.** Two substitutions are forbidden on the tagged statement.
1. Replacing `kBackShortcutStep` with a step, cone or angle typed here, or with another constant such
   as the classifier's `dAttackDirection::kBackpedalConeHalfAngle`.
2. Replacing `dAttackDirection::kMoveMagnitudeEpsilon` with `g_moveStickDeadzone` or any other value
   from `DAttackMachineSimulationRuntimeTweakables.h`.

**What breaks if the edit is made.**
1. "Holding back" gets two definitions. The motion matcher fires the first projectile on the button's
   rising edge with motion 3 of `kGameMotions`, whose only step is `kBackShortcutStep`. The machine
   fires every later projectile with its own cone. A stick between the two cones gives a projectile
   on the press and a classified swing on the hold, or the reverse. Changing the cone in
   `GameMotions.h` changes both, which is the point.
2. `g_moveStickDeadzone` is a per-process atomic, set from a console variable
   (`MovementSchemeCVar.cpp`). The server, each client and every resimulation run this function on
   the same wire input; with a tweakable in it, peers with different values decide differently, and
   one fires a projectile where another swings. A simulation reads wire input and static data only.

`DAttack.AttackTransitions.BackConeIsTheMatchersCone` catches a cone change that moves the edge past
15° or 30° off dead-back. A change between those, and edit 2, are caught by no test.

**R0 (verified 2026-10-03).** The client zeroes `moveDirectionWorld` below `g_moveStickDeadzone`
(`UOGBrawlerInputCollectionComponent::buildMoveDirectionWorld`), so a stick under the deadzone reaches
the simulation as the zero vector, and the epsilon only has to reject that zero.
`inputSequence::stickMatchesStep` gates on the length of `moveDirectionWorld`'s ground-plane part. ⚠ The 2D stick
(`PlayerInputView::moveDirection`, the wire's `moveStick`) is **not** zeroed: the client packs the
routed raw stick (`dInput::stickRouting::routeSticks` applies the deadzone only to pick a source in
one scheme). This predicate does not read it.

**Placement.** Both edits are substitutions on the tagged statement.

---

## G-09 — A follow-up press's side is the classifier's, never the button's, and a projectile input is no press

**Tag site:** `DAttackMachineSimulation.h`, `dAttackMachineSimulation::followUpPressSequence`, on its
`return` statement (five lines: the `wantsProjectile` test and the `dAttackDirection::classify` call
are both on it).

Added by og-attackstatetransition-cleanup task 2 (2026-10-03). It replaces no comment. It replaces
the button-keyed follow-up test that stood in `integrate3`'s `Attacking` case (`attackLeft` with an
active 0 or 2 queued 2, `attackRight` with an active 1 or 3 queued 3).

**The prohibition.** Two edits are forbidden on the tagged statement.
1. Replacing the `dAttackDirection::classify` call with a side taken from the button
   (`attackLeft` / `attackRight`) or from any decision made here.
2. Deleting the `wantsProjectile(playerInput)` test, or narrowing it to one of its two halves.

**What breaks if the edit is made.**
1. The side of a swing comes from `classify` (the `Idle` entry, guard **G-05**). A follow-up keyed on
   the button chains only when the button happens to match the side: with the old rule a left swing
   chained only on attack 2 and a right swing only on attack 1, which is the PIE report this task
   fixed (rationale §14.1). A decision of its own can also disagree with
   the indicator, as G-05 says.
2. Back + attack 2 is the projectile input. The classifier calls a stick between its 10° backpedal
   cone and the matcher's π/8 cone a SIDE, so without the test a projectile input during a left
   swing queues the left follow-up (3), and the projectile the player asked for never comes.

`DAttack.AttackTransitions.LeftSwingFollowUpWithAttack1` and `RightSwingFollowUpWithAttack2` fail on
edit 1 (both were seen failing on the button-keyed rule). `ProjectileInputMidSwingIsNotAFollowUp` fails
on edit 2 (seen failing: the swing chained into 3). `FollowUpPressIsTheClassifiedSideWithoutProjectileIntent`
pins the function directly, for every whole degree around a tilted aim.

**R0 (verified 2026-10-03).** At 15° off dead-back on the side `classify` calls left, `classify`
returns `kLeftSequenceId` and `wantsHeldProjectile` is true (both `REQUIRE`d by the test).
`wantsProjectile` is `triggeredActionId == kHadoukenActionId || wantsHeldProjectile(playerInput)`,
the same test the `Idle` case fires on. Edit 2 was made on a poison build (the test replaced by
`false`; restored byte-identical, md5 checked): `ProjectileInputMidSwingIsNotAFollowUp` and
`FollowUpPressIsTheClassifiedSideWithoutProjectileIntent` failed, and nothing else in
`[AttackTransitions]`.

**Placement.** Edit 1 substitutes a token on the tagged statement, edit 2 deletes or substitutes a
token on it.

---

## G-10 — The dual press acts on a real swing only, never under the projectile sentinel

**Tag site:** `DAttackMachineSimulation.h`, `integrate3`, `Attacking` case, on
`if (isRealAttackSequence(state.m_activeAttackSequence) && playerInput.attackLeft && playerInput.attackRight)`.

Added by og-attackstatetransition-cleanup task 2 (2026-10-03). It replaces no comment.

**The prohibition.** Do not delete `isRealAttackSequence(state.m_activeAttackSequence)` from the
tagged condition.

**What breaks if the edit is made.** Under `kHadoukenSequenceSentinel` the radial's `attackTimer` is 0
(the sentinel path returns before advancing it), so a dual press takes the `attackTimer < 0.1` restart
branch. That overwrites the sentinel with 4, the commitment test then reads false, and the exit below
runs on the same tick: the machine goes `Idle` in the middle of the commitment, and the held buttons
start sequence 4 on the next tick. With the stick back as well, `Idle` fires again on that next tick,
so holding both buttons and back fires a projectile **every second tick** and fills the pool.
`DAttack.AttackTransitions.DualPressCannotHijackProjectile` was seen failing both ways on the code
before this guard: `Idle` on the press tick and sequence 4 from the next; shots on ticks 1, 3, 5, …
with 35 `[Projectile.poolFull]` lines.

**A second path the same token closes.** The swing's guard-block recoil, earlier in the same case,
clears the active sequence, so on a guard-block tick the condition is false too. Without the token, a
dual press on that tick reaches a character that is already in `GuardFlinch`: inside the first 0.1 s
it sets the active sequence to 4 and hands it to the radial; past 0.3 s it queues 4, the queue
survives the recoil and `Idle`, and the next swing chains into 4 instead of its follow-up.
`DAttack.AttackTransitions.DualPressOnAGuardBlockTickQueuesNothing` pins both.

**R0 (measured 2026-10-03).** Both paths were run with this token deleted (a poison build of the
header; restored byte-identical, md5 checked). `DualPressCannotHijackProjectile` failed in both
sections and `DualPressOnAGuardBlockTickQueuesNothing` failed on both rows: active 4 on the
block tick inside the window; queued 4 on the block tick past 0.3 s, still queued after the recoil,
and the next swings `{ 0, 4, 0, 2, … }`. Nothing else in `[AttackTransitions]` failed. Guard **G-06**
(no end-tick write on the restart branch) is unchanged and still holds for every real swing.

**Placement.** A deletion of a token from the tagged condition.

---

## C. Census — every prohibition the pre-conversion header stated

Closed, so nobody reads a missing id as a deletion. "Measured" means task 18 made the forbidden edit
on a shadow copy and compiled it (single translation unit, the `OGBrawlerTests` flags).

| # | prohibition (pre-conversion lines) | disposition | shape |
|---|---|---|---|
| 1 | never include `BrawlerMovementSimulation.h`, directly or transitively (14-17, 559-563) | **compiler-held, measured**: `C2653` in every TU that reaches this header first, `SimulatableBrawler.h` included. Clean only when the movement header is entered first. No entry; rationale §1 | addition, typed in the include list or in another header |
| 2 | `PlayerInput` fields are not `const` (73) | **compiler-held, measured**: `C2678`/`C3892` wherever the input is deserialized (`readFromSyncedBuffer`), as `InputRedundancyBundleCodec.h` and `RelayedInputRingCodec.h` do. Rationale §3. ⚠ Since og-syncedInput-rework task 4 the type is deleted; the deserialized input is `simulatableBrawler::SyncedPlayerInput`, which the same codecs read | substitution |
| 3 | `triggeredActionId` stays last (79-83) | **compiler-held, measured**: `C2440` in `zero()`'s five-argument initializer. ⚠ Since og-syncedInput-rework task 4 that `zero()` is deleted; the wire field order is held by the descriptor-tuple `static_assert` in `BrawlerSyncedPlayerInput.h` | reorder |
| 4 | `kDAttackStateCount` bumped with the enum (59-60) | **converted by task 18**: a `static_assert` plus C4062-as-error around `dAttackStateName`. Every poison fired (add only, add + name, add + bump, a mid-enum insert), and the full edit compiled clean | addition, typed at the enum |
| 5 | write `spawnDir`, NOT velocity (660-661) | **compiler-held, measured**: `C2039`, there is no velocity field | substitution |
| 6 | `inboundHit` is a plain parameter, NOT an `ExternalDeps` entry (19-21, 586-591) | **compiler-held, measured**: `C2338` "UNOWNED EXTERNAL REF" | addition, typed in `Dependencies` |
| 7 | the neutral input is `(0,0,1)`, never `PlayerInput{}` (85-91) | **G-01**, retired by og-syncedInput-rework task 4 (§R); now `static_assert`s in `BrawlerSyncedPlayerInput.h` | substitution |
| 8 | `swingTickCount` is the radial's loop (147-163) | **G-02** (a `static_assert` was tried and rejected as vacuous) | deletion + substitution |
| 9 | `glm::abs`, never unqualified `abs` (235-244) | **G-03** | substitution |
| 10 | projectile-block recoil from any origin state (610-622) | **G-04**, retired by og-attackstatetransition-cleanup task 4 (§R): the transition itself was deleted at the user's ruling | addition |
| 11 | one direction decision, shared with the indicator (697-700) | **G-05** | substitution |
| 12 | the dual-tap branch writes no end tick (752-764) | **G-06** | absence |
| 13 | the chain rewrites the end tick (808-812) | **G-07** | deletion |
| 14 | the inbound-hit veto sits AHEAD of the switch, the Hadouken trigger and the attack inputs (591-596) | **ordering — no tag.** Rationale §7.1 | reorder |
| 15 | the Hadouken trigger sits AHEAD of the button handling (636-640) | **ordering — no tag.** Rationale §6.1 | reorder |
| 16 | the Hadouken commitment gate holds the exit to `Idle` (793-797) | a fact that names no forbidden edit. Rationale §6.3 | — |
| 17 | the sentinel is handed to the radial so it early-returns (668-669) | a fact that names no forbidden edit. Rationale §6.2 | — |
| 18 | `m_attackEndTick` appended last (202-203) | a record of how the field was added, not a live prohibition. Rationale §5 | — |

**By shape:** 7 tagged, all typed on the tagged statement (3 substitution, 1 addition, 1 deletion,
1 deletion + substitution, 1 absence). 2 ordering constraints, untagged. 5 compiler-held (measured).
1 converted by a new assertion. 3 facts with no forbidden edit, in the rationale.
⚠ Since og-syncedInput-rework task 4: 6 tagged; G-01 retired with the type it stood on.
⚠ Since og-attackstatetransition-cleanup task 4: 5 tagged; G-04 retired with the transition it stood on.

**Added after the conversion** (not in the census above, which closes over the pre-conversion
header): **G-08** (og-attackstatetransition-cleanup task 1), two substitutions on one statement.
**G-09** (task 2), a substitution and a deletion on one statement. **G-10** (task 2), a deletion. The
follow-up mapping itself (`followUpSequenceFor`) is held by two `static_assert`s, with no entry.

---

## §R — Retired ids

⛔ **Spent forever.** This number may not appear as a `⛔G-nn` tag again.

---

### G-01 — RETIRED (og-syncedInput-rework task 4): the machine input slice was deleted

**Was:** *The neutral input is `(0,0,1)` aim, NOT `PlayerInput{}`, and its value is a wire value*.
The entry below is the text as task 18 and og-syncedInput-rework task 3 left it.

**Tag site:** `DAttackMachineSimulation.h`, inside `PlayerInput::zero()`, on its `return` statement.

<!-- header lines 85-91 -->
```cpp
// THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
// SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
// [movement-sim task 22] The value is copied VERBATIM from what that function
// handed this type before the fold; it is a wire value, not something to re-derive.
// ⛔ (0,0,1) forwards, NOT PlayerInput{}: a value-initialised (0,0,0) aim would
// reach normalize(), and the difference is also the TAG the input-resolution and
// net-sync anti-vacuity tests discriminate on. Keep zero() != PlayerInput{}.
```

**The prohibition.** Do not make `zero()` return `PlayerInput{}`, and do not change the value it
returns. The value was copied verbatim from what the pre-fold `getZeroPlayerInput()` handed this
slice; it is a wire value, not something to re-derive.

**What breaks if the edit is made.** The neutral input is what the resolution peers substitute for a
tick that has no input, so a changed value changes the bytes every peer fills a gap with. The
anti-vacuity assertions that tell "the game zero" apart from a default-constructed input lose the
field they discriminate on. Until og-syncedInput-rework task 3, the case
DAttack.SimulatableBrawler.ZeroInputIsTheFold pinned both halves at run time: the zero input's bytes
against a captured copy, and this slice's aim `== (0,0,1)`. A `static_assert` was tried and cannot be
written: `glm::vec3` has no `constexpr` constructor in this build (`C2131`, measured by task 18).

⚠ **Since og-syncedInput-rework task 3 (2026-10-03) this paragraph is history.** Task 3 deleted that
case with the six-slice input composite. This slice's `zero()` is no longer on the wire and nothing
calls it: `simulatableBrawler::getZeroPlayerInput()` returns `simulatableBrawler::SyncedPlayerInput::zero()`,
and the resolution tests' `isGameZeroInput` compares against that. The same two halves are now
pinned on that type: `SimulatableBrawler.SyncedPlayerInput.ZeroSerializesToTheCapturedBytes` (its
39 bytes) and `SimulatableBrawler.SyncedPlayerInput.ZeroIsNotValueInitialised` (aim `== (0,0,1)`, and
unequal to the value-initialised input). The prohibition on the wire value is carried by
`BrawlerSyncedPlayerInput-guards.md` G-02. Task 4 retires this id with the slice.

**R0 (verified 2026-09-23).** `SimulationComposite::zero()` folds each slice's `zero()`, and
`simulatableBrawler::getZeroPlayerInput()` is `return PlayerInput::zero();`: true. Two parts of the
quote are not:
* `(0,0,1)` was **+Z, the up axis** in the Z-up frame this was written in (`defaultForward` was `(1,0,0)`). It is
  not "forwards". (Since og-sim-metres-yup task 19 the aim is `simFrame::kUp` = `(0,1,0)`, and the local the fence
  names is `turnAxis`, `simFrame::kTurnAxis`.)
* For **this** slice, "a value-initialised (0,0,0) aim would reach normalize()" is not what separates
  the two values. Every read of the machine aim projects it onto the ground plane first (the Hadouken trigger,
  `setRadialSimulationInitialConditions` and `dAttackDirection::classify`), and `(0,0,1)` and `(0,0,0)`
  project to the same zero vector. The neutral input also presses no attack button, so it reaches
  none of those reads. The prohibition stands on the other two reasons: the wire value and the test
  tag.

**Placement.** A substitution on the tagged `return`: typed under the tag.

⛔ **Why it retired.** og-syncedInput-rework task 4 (2026-10-03) deleted `class PlayerInput`, its
`zero()`, its `SerializableFields` specialization and its `SimulationInput` assertion from
`DAttackMachineSimulation.h`. The tag stood on the `return` inside `zero()` and went with it. The
machine reads its input through its own `PlayerInputView`, filled from `simulatableBrawler::SyncedPlayerInput`.

⚠ **R0 on the text above (2026-10-03).**
* *"The neutral input also presses no attack button, so it reaches none of those reads"* is
  **FALSE**. The chain branch of `integrate3` starts `m_queuedAttackSequence` when the current
  sequence ends, whatever this tick's buttons are, and calls `setRadialSimulationInitialConditions`,
  which normalizes the aim's XY projection with no length check. A neutral-filled tick at the end
  of a swing with a queued chain therefore reaches that read and normalizes a zero vector. Both
  candidate aims project to the same zero vector there, so the conclusion stands: the choice between
  them changes nothing for that reader, and the prohibition rested on the wire value and the test
  tag. The missing length check is a pre-existing defect, not fixed by this retirement. Found by
  og-syncedInput-rework task 2; re-verified on this tree.
* *"A `static_assert` was tried and cannot be written: `glm::vec3` has no `constexpr` constructor"*
  is not what the compiler does. `glm::vec3(0.f, 0.f, 1.f)` is a constant expression; a
  value-initialised `glm::vec3{}` member read in a constant expression is `C2131`. The successor is
  exactly such an assertion.

**Successor.** The prohibition is a compile-time check now: the `static_assert`s at the foot of
`BrawlerSyncedPlayerInput.h`, whose messages end *"Was guard G-02 of BrawlerSyncedPlayerInput-guards.md."*
(the `(0,0,1)` aim of `simulatableBrawler::SyncedPlayerInput::zero()`) and *"Was guard G-01 of
BrawlerSyncedPlayerInput-guards.md."* (the value-initialised aim differs from it). Those two guards,
which carried this entry's prohibition from task 2 on, were retired into the assertions in the same
change (`BrawlerSyncedPlayerInput-guards.md` §R).

---

### G-04 — RETIRED (og-attackstatetransition-cleanup task 4): the projectile-block recoil was deleted

**Was:** *The projectile-block recoil fires from ANY origin state*. The entry below is the text as
og-attackstatetransition-cleanup task 4 found it.

**Tag site:** `DAttackMachineSimulation.h`, `integrate3`, on
`if (inboundHit.wasProjectileBlockedThisTick && state.m_currentState != DAttackState::GuardFlinch)`.

<!-- header lines 610-622 -->
```cpp
// [hit-resolution T15] Shooter-side GuardFlinch from a blocked projectile.
// The manager routing pass sets wasProjectileBlockedThisTick=true on THIS character
// (the shooter) when any of its projectile slots ended the prior tick with
// endReason=4 (blockedByGuard, per T14). Fires the same recoil the radial swing's
// attacker-side guard-block path produces (wasGuardBlockedThisTick, read in the
// Attacking case below; it was the radial State's hasHitGuard until
// og-netcode-v2-field-defects task 9). Unlike the guard-block path, this one intentionally fires from any origin state — including
// Idle — because a projectile can be blocked long after the shooter's Hadouken
// commitment window has expired and they've returned to Idle. Same cancellation
// as the HitFlinch veto above: active/queued sequences cleared so the switch below
// lands in the GuardFlinch case with m_timeInCurrentState freshly reset. The
// `!= GuardFlinch` guard prevents re-transition if the character is already
// flinching (rapid successive blocks coalesce to a single flinch window).
```

**The prohibition.** Do not narrow this condition to an origin state, for example by gating it on
`Attacking` the way the swing's guard-block recoil is gated.

**What breaks if the edit is made.** A projectile can be blocked long after its shooter's Hadouken
commitment window has ended and the shooter is back in `Idle`. With an origin gate, that block
produces no recoil at all.

**R0 (verified 2026-09-23).** The signal is set by `brawlerHitRouting::System::postIntegrate`, branch
4, on the prior tick, for any of the shooter's projectile slots that ended that tick with
`endReason == 4` (blocked by guard). ⚠ **Since og-netcode-v2-field-defects task 20** it is set by
`preIntegrate` of the consuming tick, for slots whose `endTick` is the tick integrated last
(task 20 Rework (1)). ⚠ **Since og-netcode-v2-field-defects task 17** no slot `endTick` is matched
at all: `brawlerHitDetection::System`'s projectile pass finds the slot `BlockedByGuard` at its
closed-form position on the step's own tick, routing branch 4 copies that onto this slice in the
same `preIntegrate`, and the machine reads it in that same step's `integrate`, the tick the shot
reaches the guard, while the projectile sub-simulation ends the slot with `endReason` 4. That is
the systems executor's routing system, not "the manager routing pass" the quote names. ⚠ **A same-tick interaction the quote does not state:** the inbound-hit
veto runs first. When `wasHitThisTick` and `wasProjectileBlockedThisTick` are both set on one tick,
the veto moves the machine to `HitFlinch`, and this block then moves it on to `GuardFlinch` (the
`!= GuardFlinch` test passes). The hit's `m_hitReaction` and `m_flinchDuration` stay written. That is
found by reading the code, not by running it; it is recorded in the rationale, §7.3.

**Placement.** An addition to the tagged condition: typed on the tagged statement.

⛔ **Why it retired.** User ruling, 2026-10-03 (og-attackstatetransition-cleanup Backlog task 4):
the shooter must NOT enter `GuardFlinch` when its projectile is blocked. The punish window comes
from a longer lock after every shot instead: `kHadoukenCommitmentSeconds` (0.875 s, 53 ticks at
60 Hz, from task 4; 0.74167 s, 45 ticks, since task 6: `DAttackMachineSimulation-rationale.md` §15). Task 4 deleted the `if` the tag stood on, and
with it the only reader of its input. The input went too: the slice field
`wasProjectileBlockedThisTick` and the routing loop that set it (`brawlerHitRouting::System`, the
branch the routing rationale numbers 4). A block now only ends the slot: the projectile integrate
still writes `endReason` 4 from the detector's `BlockedByGuard` outcome. The melee guard-block
recoil (`wasGuardBlockedThisTick`, read in the `Attacking` case) is unchanged.

⚠ **R0 on the text above (2026-10-04).** The same-tick interaction in its R0 paragraph (a hit and
a block on one tick end in `GuardFlinch` with the hit's reaction stranded) cannot happen any more:
a hit on a block tick now stands, with its own reaction and dwell. Verified by reading the header.

**Successor.** No prohibition: the forbidden edit (narrowing the transition to an origin state) has
no statement left to be typed on. The behaviour is pinned instead:
`DAttack.AttackTransitions.ProjectileBlockDoesNotFlinchShooter` (a block while `Idle`, mid-commitment
and in rapid fire: no `GuardFlinch`, the slot ends with `endReason` 4, the schedule is unchanged) and
`DAttack.AttackTransitions.MeleeGuardBlockRecoilIsUnchanged` (the swing's recoil is still 18 ticks
at both shipped `dt` values). Both were seen against the code before task 4: the first failed there
(`GuardFlinch` on the block tick), the second passed (18).

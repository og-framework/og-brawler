<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- ------------------------------------------------------------------ -->
<!-- LINT ESCAPES for `tools/lint/doc_anchor_lint.ps1`.                  -->
<!--                                                                     -->
<!-- Every name below is one this document deliberately mentions and     -->
<!-- which deliberately does not resolve: an engine symbol outside the   -->
<!-- scan root, a RETIRED token, a name that does not exist YET and      -->
<!-- whose fence exists for that reason, a spike-probe identifier, or a  -->
<!-- private initiative document. Each is declared WITH ITS REASON,      -->
<!-- because an escape nobody reads is a way to silence a real hit -- and-->
<!-- an escape that matches NOTHING is itself a lint violation.          -->
<!-- ------------------------------------------------------------------ -->
<!-- lint-external-ref: Jumping -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: kFlagJumping -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: commandIssued -- an identifier from an archived spike probe or a poison arm, never in shipped code -- it is the member poison arm P6 ADDS, to prove sizeof is blind -->
<!-- lint-external-ref: kProbeOffset -- an identifier from an archived spike probe or a poison arm, never in shipped code -- the hoist task 76 probed and REJECTED on narrowness -->
<!-- lint-external-ref: impl/impl_notes_seam_27.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: DetachGateIsVacuousUntilJumpLands -- a RETIRED token -- task 27 DELETED this Catch2 case and replaced it with XYKnockbackDoesNotDetach; the prose exists in order to say the name is gone, so a resolving name would falsify it -->
<!-- lint-external-ref: impl/impl_notes_seam_56.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl/design_ledge_fall_and_landing.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: design_ledge_fall_and_landing.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
# `BrawlerMovementSimulation.h` — guards

Every fence that stood in the header. Each entry has an **opaque, stable id**; in the header a
single line `// ⛔G-nn` sits exactly where the fence’s text used to sit, on the same
declaration.

**If this file and `BrawlerMovementSimulation.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check, is
moved to §R and its number is spent forever. Reusing one silently re-points every reference that
ever named it. A retired id may be **named** in prose or in a `static_assert` message; it may
never again appear as a `⛔G-nn` **tag**.

⭐ **The join is machine-checked, in both directions**, by `tools/lint/guard_tag_lint.ps1`: every
tag resolves to an entry here, every live entry is referenced by exactly one tag, no id is
duplicated, and no retired id reappears as a tag. It is a hard gate. ⚠ It checks that an entry
EXISTS. It never checks that this text is TRUE.

⛔ **Nothing in this file is a rationale.** The narrative, the provenance and the derivations
live in `BrawlerMovementSimulation-rationale.md`. A guard is a prohibition plus the consequence of
ignoring it, and nothing else.

⚠ **This file is not the source of truth for any VALUE.** The constants, the gains, the flag
bits and the wire layout live in the header; the authored tunables live in
`SimulatableBrawlerTypes.h` and in the UE layer’s `MovementSchemeCVar.cpp`. Where a value appears
below it is there to make an argument readable.

⭐⭐ **Eleven of this header’s twenty-six fences are live guards. The other fifteen are in
§R or in the rationale doc** — six became compile-time checks (task 76), four were deleted
because a working copy already stood at the site where the forbidden edit is typed, and five were
re-triaged as **rationale**: they are refutations, attributions and a paragraph defending its own
placement, with no prohibition in them at all. Those five carry no id.

| disposition | rows | count |
|---|---|---|
| **live guard, one tag each** | T3-5, T3-6, T3-7, T3-13, T3-14, T3-15, T3-17, T3-18, T3-22, T3-23, T3-24 | **11** |
| retired — became a compile-time check | T3-3, T3-10, T3-12, T3-16, T3-25, T3-26 | 6 |
| retired — deleted, working copy elsewhere | T3-1, T3-2, T3-4, T3-11 | 4 |
| re-triaged as rationale, no id | T3-8, T3-9, T3-19, T3-20, T3-21 | 5 |
| | | **26** |

⚠ **THE TABLE ABOVE IS THE CONVERSION’S CENSUS AND IT IS CLOSED.** It accounts for the
twenty-six fences task 68 converted, and it must not be edited to absorb later work — a census
that keeps growing stops being a record of what was converted.
⭐ **Guards added AFTER the conversion are listed here instead**, each with the task that added
it, so the live count is `11 + this list`:

| id | added by | site |
|---|---|---|
| `G-22` | task 27 | the two machine predicates that read `State::m_hitReaction` |
| `G-23` | task 27 | step 3’s dispatch — `committed` is tested before `frozen` |

**Live guards today: 13.**

---

## G-01 — The obvious stability bound is the wrong one

**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`OG_CHECK(hoverGainsAreStable(hoverFrequency, hoverDampingRatio), ...)` in `StaticData`’s
constructor body.

**Was must-never-move:** `T3-5`. **Taxonomy clause:** F2 + F1a.

**The fence, verbatim — these are the bytes it occupied in the header (lines 303-309 of the
pre-conversion file):**

```
        // ⛔ THE OBVIOUS BOUND IS THE WRONG ONE. "Explicit Euler is stable for ω·dt < 2" gives
        // `hoverFrequency < 120` at 60 Hz, and that admits gains that DIVERGE. Two facts kill it:
        //   1. THE STEP IS SEMI-IMPLICIT (symplectic) EULER, not explicit. Step 4 computes the new
        //      velocity and step 5 hands it to the engine, which integrates position with the NEW
        //      velocity (`x += v'·dt`) — a different recurrence with a different stability region.
        //   2. THE DAMPING TERM IS PART OF THE BOUND. A frequency cap alone cannot express a
        //      two-parameter region, and ζ moves the limit by more than 30 %.
```

**What breaks if it moves.** The **obvious** stability bound (`ω·dt < 2`) is what a reader derives in ten seconds, admits gains that diverge, and is what three people derived first. The refutation only works on the line where the wrong bound would be typed.

⚠ **PARTIALLY CONVERTED — this entry is the half the compiler cannot hold.** Task 76
landed `constexpr bool hoverGainsAreStable(float omega, float zeta)` and two `static_assert`s
beside it: `!hoverGainsAreStable(60.f, 1.f) && !hoverGainsAreStable(119.f, 1.f)` (message
ending *“Was fence T3-5.”*) and a vacuity control on the shipped pair. Those pin the two
MEASURED counterexamples, so any bound that admits either — `omega < 120` included —
breaks the build. The constructor’s `OG_CHECK` calls the same predicate, so the runtime check
and the compile-time witnesses cannot drift: there is one body.

⛔ **What they do NOT pin, and why this guard is still live:** a bound that is wrong and
*conservative* — `omega < 50` — passes both witnesses. The **derivation** (the step is
semi-implicit, and the damping term is inside the region) is what tells a reader which
direction is safe, and it is not expressible. The Jury-criterion recurrence itself is
rationale: `BrawlerMovementSimulation-rationale.md` §6.

---

## G-02 — `hoverPullDownAccel`’s check is `>= 0`, and that is not a typo

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `OG_CHECK(hoverPullDownAccel >= 0.f, ...)`
in `StaticData`’s constructor.

**Was must-never-move:** `T3-6`. **Taxonomy clause:** F1b + F5a.

**The fence, verbatim — these are the bytes it occupied in the header (lines 335-338 of the
pre-conversion file):**

```
        // ⛔ THE PULL-DOWN KNOB IS A MAGNITUDE, AND ZERO IS ITS SHIPPED VALUE — so this is the one
        // hover constant whose check is `>= 0` rather than `> 0`. A negative value would turn the
        // one-sided arm into a spring that pushes UP while the character is above ride height,
        // which is the opposite of the whole ruling.
```

**What breaks if it moves.** `hoverPullDownAccel` is the one hover constant whose check is `>= 0`, not `> 0`, and that asymmetry looks like a typo beside its three siblings. A “consistency” fix to `> 0` rejects the shipped value.

⛔ **Measured DOES-NOT-CONVERT** (task 75, arm `ARM 7`). `constexpr StaticData` is
`C2131 … call of undefined function or one not declared 'constexpr'`, and the shipped `0.f`
is a positional argument nineteen deep in `SimulatableBrawlerTypes.h`, supplied at run time.
And the fence is about the OPERATOR — `>=` against `>` — which has no value to compare in
the first place.

---

## G-03 — The probe shrink is still load-bearing after ruling #28

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `inline constexpr float kProbeShrink = 1.f;`.

**Was must-never-move:** `T3-7`. **Taxonomy clause:** F1b + F2.

**The fence, verbatim — these are the bytes it occupied in the header (lines 514-518 of the
pre-conversion file):**

```
// left to be retained as momentum. ⛔ THAT DOES NOT MAKE THIS SHRINK REMOVABLE, and the reasoning
// is not a judgement call: the shrink is what stops a touched wall from WINNING THE SWEEP at
// fraction 0, which is upstream of the vertical law entirely. A misclassified `Unsupported` tick
// under ruling #28 still switches the servo off and lets gravity pull the body into the floor —
// a softer sink with no pop, which is a worse defect to diagnose, not a better one.
```

**What breaks if it moves.** Task 56 removed the *other* half of the mechanism this shrink was introduced for, which makes the shrink look like leftover scaffolding. This is the only text saying the shrink is upstream of the vertical law entirely and still load-bearing. Deleting it re-opens a defect that now presents as a *softer* sink with no pop — harder to diagnose, not easier.

⛔ **A DELETION FENCE CAN NEVER CONVERT, and this is the phase’s textbook case.** Task
75’s probe `C8` shows `static_assert(kProbeShrink > 0.f, …)` is expressible and that poison
`P3` fires it — and it is still not equivalent, because **the edit this fence forbids is
deleting the constant**, and the assertion is deleted with it. ⭐ Task 75 added this to the
VALUE/SPELLING predictor as a third, cross-cutting rule: *a fence whose forbidden edit removes
the very declaration the assertion reads is structurally unconvertible.*

⚠ **Two sites outside this header point a reader at `kProbeShrink`’s derivation by name**
(`BrawlerMovementVisualization.h`, twice). That derivation is now in
`BrawlerMovementSimulation-rationale.md` §8, **not here** — this tag is a prohibition,
not the geometry. See §C.

---

## G-04 — Do not build the `SupportState` axis branch here

**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`inline constexpr glm::vec3 kWorldUp{0.f, 0.f, 1.f};`.

**Was must-never-move:** `T3-13`. **Taxonomy clause:** F1a + F1d.

**The fence, verbatim — these are the bytes it occupied in the header (lines 732-734 of the
pre-conversion file):**

```
// has a single feeding point. ⛔ DO NOT BUILD THAT BRANCH HERE: every v1 surface is `Supported`,
// the user ruled (a) for the surfaces that exist, and widening a ruling is not this file's
// to do. It is recorded as backlog task 49's carry-forward to tasks 20 / 48.
```

**What breaks if it moves.** Guards a branch that **does not exist yet** against being built here, and says why (the ruling covers only the surfaces that exist, and widening a ruling is not this file’s to do). There is no code to grep from — only this line.

⚠ The quoted span begins mid-sentence because the prohibition is the second clause of the
`WHY A NAME AND NOT A LITERAL` paragraph; the first clause is orientation and lives in
`BrawlerMovementSimulation-rationale.md` §11. The bytes above are exactly lines 732-734 of
the pre-conversion file, unaltered.

⛔ **Measured DOES-NOT-CONVERT** (task 75, arm `ARM 11`, one translation unit that MAKES
this and seven sibling forbidden edits and compiles clean, exit 0). An absence of an ACTION is
not expressible. ⭐ Contrast T3-16 / T3-25, absences of a MEMBER, which **are** — both
retired into a build break (§R, G-16 and G-17).

---

## G-05 — A bit constant must be READ, not only written

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `class PlayerInput`.

**Was must-never-move:** `T3-14`. **Taxonomy clause:** F1a + F5c.

**The fence, verbatim — these are the bytes it occupied in the header (lines 782-787 of the
pre-conversion file):**

```
// ⛔ AND A BIT CONSTANT MUST BE **READ**, NOT ONLY WRITTEN. The State-side `kFlagHasCommand`
// below was defined, written in three places and read NOWHERE, because an off-wire `bool`
// twin was quietly doing the real work; it took task 50 to notice and repair it. A flag whose
// only consumer is its own writer is a wire byte that buys nothing — a fact the wire asserts
// and nothing checks. `kInputFlagHoldGuard`'s reader is step 1's `frozen` gate in `integrate`
// below, the single place this input is consumed, and it is named as the reader there.
```

**What breaks if it moves.** Generalises a defect the tree actually paid for: a bit written in three places and read nowhere. The prohibition belongs beside the bit constants, which is where the next one will be added.

⚠ **R0, and it is flagged rather than silently repaired.** The last sentence of the fence
above says `kInputFlagHoldGuard`’s reader is *“the single place this input is
consumed”*. Measured 2026-09-11, the constant is READ at **six** sites outside this header
— four in `MakeSimPlayerInputFlagsTest.cpp`, two in `SimulatableBrawlerTest.cpp`. The claim
survives only on the narrow reading *“consumed as input, in production code”*, which is
true: step 1’s `frozen` gate is the one line that turns bit 0 into behaviour.

⛔ The fence is quoted **verbatim and uncorrected** — extraction is a move, not a rewrite.
⭐ The corrected wording is in `BrawlerMovementSimulation-rationale.md` §13 (note R0-09), which also
records that the symmetrical WRITER claim in `BrawlerInputPackaging.h` was found false by task
66 and deleted by task 74: **the same defect shape, in the sibling file, days earlier.**

---

## G-06 — Reading `bodyState.linearVelocity` re-admits contact impulses

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `LinearBodyState bodyState;` in `class State`.

**Was must-never-move:** `T3-15`. **Taxonomy clause:** F1a.

**The fence, verbatim — these are the bytes it occupied in the header (lines 899-900 of the
pre-conversion file):**

```
    // ⛔ Reading it would silently re-admit contact impulses into simulation state, which is
    // the ONE property revision 6 exists to guarantee.
```

**What breaks if it moves.** The one-sentence statement of revision 6’s central guarantee, sitting on the field that would break it. In a document it is a design principle; here it is a stop sign.

⭐ **This entry absorbed a second must-never-move row.** T3-2 said the same thing in the
file’s orientation header, 828 lines above the field — a region v2 §1.2 replaces with two
doc paths, so it has no declaration to be tagged at. Task 75 §7 found it a duplicate of this
one and ruled it a DELETE rather than a relocation; the copy quoted above is the one that was
already at the right site. See §R, G-19.

⛔ **Measured DOES-NOT-CONVERT** (task 75, arm `ARM 11`, which reads
`bodyState.linearVelocity` into `state.velocity` and compiles clean). No expression
distinguishes that read from a correct one.

---

## G-07 — `detachesFromSupport` returning `false` is a statement about the tree

**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`inline bool detachesFromSupport(const State& state, const glm::vec3& up)`.

**Was must-never-move:** `T3-17`. **Taxonomy clause:** F4.

**The fence, verbatim — these are the bytes it occupied in the header (lines 1238-1249 of the
pre-conversion file):**

```
// ⛔ IT RETURNS `false` TODAY, AND THAT IS A STATEMENT ABOUT THE TREE, NOT A PLACEHOLDER I
// FORGOT TO FILL. Both conditions the design names are unbuilt:
//   * `Jumping` (task 21) — a `State::flags` bit set on the take-off edge and cleared at the
//     apex. Bits 4-7 are free; the bit and its writer land WITH task 21, because a flag written
//     and never read (or read and never written) is the `kFlagHasCommand` trap this file already
//     paid for once. Task 21's whole vertical change becomes: set the bit, assign `velocityUp`,
//     let the law below run.
//   * an upward `Launched` (task 27) — `DAttackState` is {Attacking, Idle, GuardFlinch,
//     HitFlinch} in this tree. There is NO `Launched` enumerator to test, so the condition
//     cannot be written today without inventing the state it reads. ⚠ The Backlog's AC says
//     "today only upward `Launched`"; that clause is vacuous against the shipped machine, and it
//     is recorded as such in `impl/impl_notes_seam_56.md` rather than faked.
```

**What breaks if it moves.** `detachesFromSupport` returned `false` unconditionally — the exact shape of an unfinished stub, and coverage tooling reported the body dead. This line was the only thing saying the constant `false` was a **statement about the tree** (neither condition it would test existed yet) rather than a `TODO`.

⭐⭐ **REVISED BY TASK 27, 2026-09-12 — THE PREDICATE IS NO LONGER VACUOUS, AND THE
PROHIBITION IT CARRIES HAS CHANGED WITH IT.** The id is unchanged because the SITE is unchanged;
what the guard forbids is now this:

> ⛔ **THE ARM IS KEYED ON VELOCITY, NEVER ON AN ENUMERATOR.**
> `return committed && glm::dot(state.velocity, up) > 0.f;`
> Do not replace it with a test against a machine state. **There is no `Launched` enumerator and
> there is not going to be one** — user ruling 2026-09-12: the hit reaction is DATA on the hit
> (`HitReactionKind` + `HitReactionSpec`), carried through one generalised `HitFlinch` state, so
> that `DAttackState` stays at four values and the visualizer initiative’s
> `kMachineStateCellCount` fence and its four `case` sites do not move in **another
> initiative’s files**. A velocity-keyed arm needs no enumerator knowledge at all: it is false
> for every shipped XY knockback and true the day a lift is authored.

**What breaks if THAT moves.** An enumerator-keyed arm has to be extended by every future task
that adds a committed state, and each such task then has a reason to add an enumerator here —
which is the pressure this design exists to remove. It also cannot see an authored upward
component that arrives through data rather than through a state.

⚠ **WHAT THE ARM DOES NOT DO, MEASURED AND ROUTED (task 27).** `dot(state.velocity, up)` is
WORLD z, and on a SLOPE the tangential channel carries `knockbackSpeed · sin θ` of world z —
1000 cm/s on a 30° face at the shipped speed — so an up-slope knockback DOES reach the true arm
on the tick after the shove and loses its hover hold for the rest of the slide. The frame does not
move with it (`n` is keyed on `walkable`, not on `support`), so the slide stays on the face.
`BrawlerMovement.SlopeKnockbackStaysOnSlope` prints both readings and records the support state;
the finding and its recommended fix — key the arm on the vertical CHANNEL, which is computable
here because the frame does not depend on the detach decision — are routed to the lead in
`impl/impl_notes_seam_27.md`. **Not changed unilaterally: the arm is the architect’s, ruled.**

⚠ **THE `Jumping` HALF IS UNCHANGED AND STILL UNBUILT.** Task 76 landed
`static_assert(!detail::kHasLaunched<DAttackState>, … "Was fence T3-17 (the Launched half).")`
with a `kHasHitFlinch` vacuity control beside it, and task 27 **restated its message** rather than
deleting it: it no longer says "fill the arm", it says there is no `Launched` by design and the
arm is velocity-keyed. That is still a **remote tripwire** — the build breaks for the person
editing `DAttackMachineSimulation.h`, in the file that makes the sentence false, and not here.

⛔ **The `Jumping` half does not convert and cannot.** Adding
`inline constexpr uint8_t kFlagJumping = 1u << 4;` is invisible to any expression over the
three existing flag constants — the new name is simply not in it. There is nothing to assert
until the bit and its writer exist, which is exactly what the fence is about.

✅ **Re-verified 2026-09-12 against the shipped bytes:** `DAttackState` is
`{Attacking, Idle, GuardFlinch, HitFlinch}`, in that declared order, and `kDAttackStateCount` is
still `4u`; bits 4-7 of `State::flags` are free — `kFlagFrozen` is bit 0, `SupportState` rides
bits 1-2, `kFlagHasCommand` is bit 3. **Both arms of the predicate are now driven by
`BrawlerMovement.XYKnockbackDoesNotDetach`**, which replaced
`DetachGateIsVacuousUntilJumpLands` — that case’s own header asked to be deleted the day the
vacuity ended.

---

## G-08 — Step 6′’s component removal is NOT a velocity decomposition

**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`const glm::vec3 contactNormal = glm::normalize(pushOut);` in step 6′.

**Was must-never-move:** `T3-18`. **Taxonomy clause:** F6.

**The fence, verbatim — these are the bytes it occupied in the header (lines 1350-1352 of the
pre-conversion file):**

```
                // ⚠ THIS IS NOT A VELOCITY DECOMPOSITION and must not be confused with F2's:
                // both arms remove ONE AUTHORED COMPONENT from a vector, which is exact for any
                // unit direction. There is no basis here to be non-orthogonal.
```

**What breaks if it moves.** Two nearby operations look like the same operation and are not: step 6’s authored single-component removal against step 3’s basis decomposition. The identifiers are near-identical; the correction only works between them.

⛔ **Measured DOES-NOT-CONVERT** (task 75, arm `ARM 11`, which takes a plain dot of the
velocity against a frame axis and compiles clean). This is a **SPELLING** fence — it is about
which of two visually identical operations is the right one here — and SPELLING is **0 for
4** on this file and **0 for 8** across the three files measured in this phase.

⭐ The acceptance grep this fence protects (*“no plain dot of `state.velocity` against
`u`, `v` or `up` remains in this header”*) is recorded in
`BrawlerMovementSimulation-rationale.md` §23, with the count it returns and the two
surviving calls it deliberately does not count.

---

## G-09 — Do not scale `rideHeight` with slope — lead-scoped

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `const float vUp = channels.up;` in step 4.

**Was must-never-move:** `T3-22`. **Taxonomy clause:** F1a + F2.

**The fence, verbatim — these are the bytes it occupied in the header (lines 1676-1679 of the
pre-conversion file):**

```
    // after, and what changed is that the servo no longer undershoots it. ⛔ Do not compensate
    // by scaling `rideHeight` with slope — that is a design change and it is lead-scoped.
    // Steep faces need the axis to follow `SupportState` (see `kWorldUp` above) — that is what
    // `SupportedSteep` is reserved for, and it is a STEP 2 change when it lands, not a step 4 one.
```

**What breaks if it moves.** Names the obvious compensation (scale `rideHeight` with slope), its status (a design change) and its owner (lead-scoped). The alternative is a two-character edit at the constant.

⛔ **Measured DOES-NOT-CONVERT.** `rideHeight` is a runtime `StaticData` field authored in
another file, and *“do not scale it with slope”* is a design-scope ruling about an edit
nobody has made. There is no value to compare.

⚠ **This tag sits at the tail of the longest block in the file** — step 4’s 126-line
comment. The first code line at or after the fence is `const float vUp = channels.up;`, which
is where the tag now sits. The `rideHeight·cosθ` arithmetic the fence follows from —
7.07 cm at the 45° cap, 2.59 cm at 75° — is rationale, §23.

---

## G-10 — The two forbidden one-sided-servo variants, both written before being caught

**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`const float e = sd.rideHeight - clearance;` in step 4’s supported arm.

**Was must-never-move:** `T3-23`. **Taxonomy clause:** F2 + F1a.

**The fence, verbatim — these are the bytes it occupied in the header (lines 1718-1725 of the
pre-conversion file):**

```
        // ⛔⛔ TWO VARIANTS ARE FORBIDDEN, AND BOTH WERE WRITTEN BEFORE BEING CAUGHT
        // (`impl/design_ledge_fall_and_landing.md` §1b):
        //   1. LEAVING THE DAMPER AND/OR THE FEED-FORWARD ON ABOVE RIDE HEIGHT. Both OPPOSE the
        //      fall (+22 800 cm/s² at 400 cm/s, plus 980), so the body FLOATS in the band instead
        //      of leaving it — the damper's terminal chase speed alone is `g/c` = 17 cm/s, 2.3 s
        //      to clear 40 cm. That is why this arm has neither term, not an oversight.
        //   2. GATING THE WHOLE SERVO ON A CHASE SPEED. Bang-bang: +/-380 cm/s per tick at the
        //      shipped gains.
```

**What breaks if it moves.** Two variants that were **actually written before being caught**, each with its measured consequence. A prohibition that names a mistake somebody already made is the one most likely to be repeated.

⚠ **R0, flagged not fixed.** The fence’s `+/-380 cm/s per tick at the shipped gains` is
quoted from `design_ledge_fall_and_landing.md` §1b, which states the same figure and **does
not derive it either** — a circular citation, found by task 81’s audit (row 152).
Re-derived here so the number stops depending on itself: it is `c·dt` times the chase speed,
`2ζω · dt · 400` = `57.04 × (1/60) × 400` = **380.27**. ⭐ Name the TERM and the
figure survives a gain change instead of silently lying about one.

✅ Re-verified 2026-09-11: `design_ledge_fall_and_landing.md` §1b exists and is titled
*“Why not a chase-speed gate (the first draft)”*. ⚠ That document is initiative working
material and is **not distributed with this submodule**; the fence names it because the fence
named it, and the quotation is verbatim.

---

## G-11 — Step 4 has no geometry of its own — the detach decision is step 2’s

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `if (support != SupportState::Unsupported)`
in step 4. ⚠ **Moved up one declaration** — see the note.

**Was must-never-move:** `T3-24`. **Taxonomy clause:** F1d + F5a.

**The fence, verbatim — these are the bytes it occupied in the header (lines 1729-1731 of the
pre-conversion file):**

```
        // ⚠ `support` is READ here and decided in step 2. Step 4 has no geometry of its own and
        // must not acquire any: the day a jump has to suppress the servo, step 2's detach gate is
        // the one place that changes.
```

**What breaks if it moves.** Defends the placement of the detach decision in step 2 against the natural instinct to handle it where the servo runs. Putting it here re-creates the “value that is not true” the `SupportState` rename exists to remove.

⚠ **THIS IS THE ONE TAG THAT IS NOT WHERE ITS FENCE SAT, and the move is deliberate.** The
fence stood *inside* step 4’s supported arm, so the first code line at or after it is
`const float e = sd.rideHeight - clearance;` — which is also **G-10’s** site. Two tags on
one declaration is the single biggest source of UNCERTAIN in this phase’s reader tests, so the
tag was moved up to the branch whose condition **reads `support`** — the declaration this
fence is actually about.

⚠ Task 75 §6 prescribed that move and named the target as line `:1563`. Re-verified against
the shipped bytes, `:1563` is a comment line: the `if` was at `:1529` of the pre-task-76 file
and is at **`:1700`** of the pre-conversion file. **The prescription’s intent was applied;
its line number was wrong and is corrected here.**

⛔ **Measured DOES-NOT-CONVERT** (task 75, arm `ARM 11`). A defence of a decision’s
PLACEMENT has no expression: both placements compile.

---

## G-22 — `m_hitReaction` is meaningless outside `HitFlinch`

**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`inline bool machineFreezesMovement(const dAttackMachineSimulation::State& machineState)` — the
FIRST of the two sibling predicates that read the byte. **One tag, one entry, two sites**: the
prohibition is the same sentence at both, and splitting it would have put two ids on one idea.
The second site is `machineLaunchesMovement`, directly below.

**Added by:** task 27, 2026-09-12. Not part of task 68’s conversion census.

> ⛔ **NEVER TEST `m_hitReaction` WITHOUT TESTING `m_currentState == DAttackState::HitFlinch`
> IN THE SAME EXPRESSION.** The byte is written ONLY by the hit veto, on entry, and it is never
> cleared on exit — outside `HitFlinch` it is the STALE kind of the last hit this character
> took, and it is read by nothing.

**What breaks if it moves.** `HitReactionKind::Stun` is 0, so a character that has never been hit
reads `Stun` and a `machineFreezesMovement` shortened to `m_hitReaction == Stun` would freeze
**every idle character in the game** — a total-failure mode that no slope, wall or replay case
would be needed to notice. The other direction is the quiet one: a character that took a knockback
an hour ago still reads `Knockback`, so a `committed` test shortened the same way would suspend the
movement model and hold whatever velocity the body had, permanently. ⭐ The pair is written as
two whole predicates, each spelling the state test out again, rather than as one shared boolean
local: a local can be read without the test that produced it, and a predicate cannot.

⚠ **It does not convert to a `static_assert`.** The claim is about what a runtime byte MEANS in
a state the compiler cannot see. What the compiler does pin, in `OGBrawler/HitReaction.h`, is the
half that is expressible: `Stun == 0`, so a default-constructed `State` — which is exactly what
`injectCorrectionState` reads a correction into — reads as the pre-task-27 behaviour.

✅ **Re-verified 2026-09-12:** the only writer of `m_hitReaction` is the inbound-hit veto in
`dAttackMachineSimulation::integrate3`; the only readers are the two predicates this tag sits on.

---

## G-23 — `committed` is tested BEFORE `frozen`, and the order is the rule

**Tag site:** `BrawlerMovementSimulation.h`, on step 3’s `if (committed)` in `integrate`.

**Added by:** task 27, 2026-09-12. Not part of task 68’s conversion census.

> ⛔ **DO NOT REORDER THESE TWO BRANCHES, AND DO NOT MERGE THEM INTO ONE CONDITION.** A
> committed state owns the velocity OUTRIGHT; `frozen` is the model’s gate, not a veto over a
> commitment. Holding guard during a knockback must NOT stop the slide.

**What breaks if it moves.** `frozen` is true whenever bit 0 of the input flags byte is held, and a
player who is being thrown five metres is very likely holding guard. Test `frozen` first and the
slide stops dead on the tick the button goes down — a 5 m throw becomes a 0 m throw, in the one
situation the player is most likely to create. ⭐ **Guarding is already blocked, and not by this
branch:** `DAttackGuardSimulation` disables every guard shape whenever
`attackMachineSimulation.m_currentState != DAttackState::Idle`, which covers the whole lockout. The
shape gate is the mechanism; freezing the body would be a second, wrong one.

⚠ **The `frozen` BIT still records `frozen`.** `State::flags`’ bit 0 is set from the `frozen`
local regardless of which branch ran, so during a guard-held knockback the wire bit reads 1 while
the body slides. Its only reader is `BrawlerMovementVisualization.h`, so this is a display
inaccuracy and nothing more — recorded here so the next reader does not treat the bit as the
answer to "did the model run".

✅ **Pinned by `BrawlerMovement.HoldGuardDoesNotFreezeASlide`**, which holds
`kInputFlagHoldGuard` through the launch tick and the tick after it, with a control arm proving the
bit is live in the same fixture.

---

## §R — Retired ids

⛔ **Spent forever.** None of the numbers below may ever appear as a `⛔G-nn` tag again. They
may be NAMED here, in prose, or in a `static_assert` message — which is exactly what the six
converted rows do, through the `Was fence T3-nn` convention (lead ruling F-2): **delete the
assertion and the row’s pin goes with it**, so the message names the row it replaces and a grep
finds the pair.

⚠ **These ids were assigned AT retirement.** Not one of them was ever a live tag in this header,
because the rows retired before the conversion ran. They exist so each fence’s text is preserved
at a stable address instead of surviving only in version control, and so this file records **why**
each one is not a tag.

### G-12 — RETIRED, converted to a compile-time check

Was must-never-move **T3-3**.

**The text it replaced, verbatim (lines 105-105 of the pre-conversion file):**

```
//      ⛔ Never reorder existing ones: the wire layout is positional.
```

**What broke if it moved.** Field order **is** the wire layout. A reorder compiles, passes every unit test, and desynchronises every peer that has not shipped the same reorder.

**Now enforced by**, at the foot of the header,
`static_assert(std::is_same_v<decltype(SerializableFields<brawlerMovementSimulation::State>::get()), std::tuple<…>>, …)`
and the two sibling specialisations, each message ending *“Was fences T3-3 and T3-26.”*
`SIM_MEMBER(C, m)` expands to `MemberFieldDesc<&C::m>{}`, so every member pointer is a
non-type template parameter and the whole sequence is carried in the tuple’s TYPE. A reorder,
an insertion and a removal all move that type; a rename does not compile at all.

⭐⭐ **Before task 76 this was the loudest unguarded hole in the file.** `velocity` and
`positionCmd` are both 12 B, `syncSize` is a SUM over the field list and therefore
permutation-invariant, and a transposition left **all eleven** wire pins in the tree green
while desynchronising every peer — demonstrated on a shadow copy before it was closed.

---

### G-13 — RETIRED, converted to a compile-time check

Was must-never-move **T3-26**.

**The text it replaced, verbatim (lines 1797-1798 of the pre-conversion file):**

```
// ⛔ APPEND ONLY. The wire layout is positional; reordering these six entries is a wire
// format change even when the byte count is unmoved.
```

**What broke if it moved.** `APPEND ONLY` over six positional wire entries. The byte count does not move on a reorder, so every size pin stays green and every peer disagrees.

**Now enforced by** the same three `static_assert`s as G-12. T3-3 and T3-26 are one
prohibition stated at two sites 1,692 lines apart, and they retired together into one
mechanism.

---

### G-14 — RETIRED, converted to a compile-time check

Was must-never-move **T3-10**.

**The text it replaced, verbatim (lines 598-601 of the pre-conversion file):**

```
    // ⛔ FALSE, and it is load-bearing. Gravity is the SIM's law (StaticData::gravity,
    // applied on EVERY tick since task 56). With the body re-placed from State every tick,
    // engine gravity would double-apply against that term AND perturb the post-solve
    // position the push-out in step 6' is measured from.
```

**What broke if it moved.** `enableGravity = false` on a body the sim moves reads like a mistake — “the character should fall”. Flipping it double-applies gravity against the sim’s own term **and** perturbs the post-solve position step 6′ measures its push-out from. Two consequences, neither visible at the flag.

**Now enforced by** `static_assert(kCharacterCapsuleBody.enableGravity == false, … "Was fence T3-10.")`,
reading a hoisted `inline constexpr BodyDescriptor`. `PhysicalObjectDescriptor` holds a
`std::vector<ShapeDescriptor>` and is therefore not a literal type, so the flag had to be
hoisted out of `body`’s initializer before the compiler could read it. **Not one flag’s
value changed.** The hoist also made `simulatePhysics`, `isRoot`, `lockRotation` and
`resimPolicy` assertable, and task 76 pinned all four in a bonus assertion whose message
claims **no** row — correctly, because those four were never on the list.

---

### G-15 — RETIRED, converted to a compile-time check

Was must-never-move **T3-12**.

**The text it replaced, verbatim (lines 663-666 of the pre-conversion file):**

```
    // `clearance` reading on flat ground is UNCHANGED. Read `kProbeShrink`'s comment above
    // before touching either term — the two move together or the probe stops measuring what
    // step 2 believes it measures, and the shrink is what stops a touched WALL from winning
    // this sweep at fraction 0.
```

**What broke if it moved.** The shrink and the drop are **two terms that must move together**; either alone leaves the probe measuring something other than what step 2 believes. Nothing in the two lines of code says they are a pair.

**Now enforced by** an `OG_CHECK` inside `PhysicsSetup::queryVolumes` reading BOTH terms
against the drop, message ending *“Was fence T3-12.”*

⛔ **The `static_assert` form was REJECTED on narrowness, and the second arm is why.** With
the offset hoisted to an `inline constexpr glm::mat4`,
`static_assert(kProbeOffset[3].z == -kProbeShrink)` compiles and FIRES when the drop moves
alone — but the capsule shrink is `sd`-dependent and invisible to it, so changing *that* half
alone leaves it clean. An assertion equivalent to one instance of its own prohibition,
licensing the other half while looking enforced.

⚠ It is a RUNTIME check. Its coverage is every body registration in PIE **and** two movement
LLT cases that call `PhysicsSetup::queryVolumes(sd)` directly. Weaker than a build break,
stronger than a sentence.

---

### G-16 — RETIRED, converted to a compile-time check

Was must-never-move **T3-16**.

**The text it replaced, verbatim (lines 938-944 of the pre-conversion file):**

```
    // ⛔ THERE IS NO COMMANDED-VELOCITY MEMBER BESIDE IT, and that is not an omission.
    // Task 11 kept one, off-wire; it held exactly `state.velocity`, which is ALREADY on the
    // wire (second entry in SerializableFields). Step 5 copied `velocity` into it as its
    // last act, and NOTHING writes `state.velocity` between there and the next step 6' —
    // the capture pass writes `bodyState` only (`PhysicsDeclaration::bodyStateOf`), and no
    // file outside this header names the field at all. So step 6' reads `state.velocity`
    // directly: EXACT, and 0 B instead of 12.
```

**What broke if it moved.** **Absence fence for a member that is not there**, on a class full of members. Task 11 had one; it held exactly `state.velocity`, which is already on the wire. Re-adding it costs 12 B and buys nothing, and nothing in the class records that the experiment was run.

**Now enforced by** `static_assert(detail::StateHasExactlySixMembers<State>, … "Was fences T3-16 and T3-25.")`.

⭐ **The surprise of the phase: an absence of a MEMBER is expressible.** A structured-binding
decomposition compiles only if the aggregate has exactly that many public data members.

⛔ `sizeof` and `offsetof` are **both blind**: `flags` is a `uint8_t` before a 4-aligned
`glm::vec3`, so a `bool commandIssued` added in the three padding bytes after it leaves
`sizeof(State) == 64` and every offset (0/24/36/44/48/52) unchanged. Measured, both
candidates, on that exact edit.

⚠ **MEASURED LIMIT:** the binding introduces FRESH names, so the concept is blind to a
rename. G-12 / G-13’s tuple assertion is what catches a rename, as a hard `C2039`.

---

### G-17 — RETIRED, converted to a compile-time check

Was must-never-move **T3-25**.

**The text it replaced, verbatim (lines 1765-1765 of the pre-conversion file):**

```
    // disagree. ⛔ Do not re-introduce an off-wire marker beside this one.
```

**What broke if it moved.** **Absence fence:** the off-wire twin marker is gone, and its absence is the property that makes replay reproduce the live tick. A second marker beside the wire bit is the reflex fix for a replay bug.

**Now enforced by** the same `StateHasExactlySixMembers` assertion as G-16 — this is the
step-5 statement of the same absence, 827 lines from the other one. Recorded under its own id
because it occupied its own bytes at its own declaration, and an id is cheaper than a lost
sentence.

---

### G-18 — RETIRED, deleted, the working copy is in another file

Was must-never-move **T3-1**.

**The text it replaced, verbatim (lines 43-46 of the pre-conversion file):**

```
// ⛔ KEEP THE GRAPH ACYCLIC: nothing reachable from `DAttackMachineSimulation.h` may include
// `BrawlerMovementSimulation.h`. Re-verified at task 62 for all seven of its own includes
// (radial sequence, radial sim, sequence id, direction classifier, projectile, inbound hit,
// input sequence) — none of them reach this file.
```

**What broke if it moved.** The cycle it forbids **cannot be made an error**: `#pragma once` silently leaves one side incomplete depending on which header the TU entered from. There is no compiler diagnostic to fall back on, so the only guard is a sentence beside the include that would re-create it.

**Deleted, not relocated — the edit it forbids is typed in another file, and that file
already says so.** `DAttackMachineSimulation.h:475-476` carries the working copy: *“the
dependency now points one way — movement -> machine — and THIS HEADER MUST NEVER INCLUDE
`BrawlerMovementSimulation.h`, directly or through any of its other includes.”* Presence
re-verified on the shipped bytes **before** this deletion.

⛔ **R0: the deleted text was FALSE, and this is where that is recorded.** *“all seven of
its own includes”* — `DAttackMachineSimulation.h` has **nine** og-brawler includes (eight
sub-simulation headers plus `OGBrawlerLog.h`, which the header and task 65’s own correction
both missed) and **thirteen** non-`glm`, non-`std` includes in total. The fence quantifies
over *every* include, not the og-brawler subset. ⭐ A bare count in prose is a claim with a
maintenance cost and no enforcement, and this one drifted twice in two consecutive tasks. It
is **not replaced with a new number**; the acyclicity argument lives in
`BrawlerMovementSimulation-rationale.md` §1 without one.

⚠ `BrawlerCharacterBindings-guards.md:230` quotes these exact lines as
`BrawlerMovementSimulation.h:43-44`. **Routed, not fixed** — see §C.

---

### G-19 — RETIRED, deleted, the working copy is in another file

Was must-never-move **T3-2**.

**The text it replaced, verbatim (lines 73-73 of the pre-conversion file):**

```
// `bodyState.linearVelocity` is NEVER READ — no contact impulse can enter simulation state.
```

**What broke if it moved.** An **absence fence over a field that is present**: `bodyState.linearVelocity` sits in `State`, on the wire, in the checksum, and reading it is one keystroke. Nothing in the type says it is off limits; this line is the only thing keeping contact impulses out of simulation state — the one property revision 6 exists to guarantee.

**Deleted as a duplicate.** It stood in the file’s orientation header — 828 lines above the
field it is about, in a region v2 §1.2 replaces with two doc paths, so it has no declaration
to be tagged at. The copy at the declaration is **G-06**, on `LinearBodyState bodyState;`.
Presence at the right site re-verified **before** this deletion.

---

### G-20 — RETIRED, deleted, the working copy is in another file

Was must-never-move **T3-4**.

**The text it replaced, verbatim (lines 106-107 of the pre-conversion file):**

```
//      ⛔ A model may write ONLY its own `State` slice, and may not name `bodyState` —
//      position is the composition step's business, not the model's.
```

**What broke if it moved.** A model that names `bodyState` compiles and reads naturally. It also takes position away from the composition step, which is where the engine’s push-out is adopted — so the model silently starts fighting step 6′.

**Deleted as a duplicate.** Also in the orientation header. The copy at the right site is the
MOVEMENT MODELS banner (line 1162 of the pre-conversion file): *“and no model may touch
`state.bodyState`”*, which sits on the block the models actually live in. Presence
re-verified **before** this deletion.

⚠ **Stated rather than hidden:** this is the one deletion whose surviving copy is now in a
doc rather than at a declaration, because that banner is narrative and was never on the
must-never-move list — it moved to `BrawlerMovementSimulation-rationale.md` §21. The
consequence is that the prohibition has **no tag anywhere in the tree**. Task 75 §7 ruled the
delete; the cost of that ruling is recorded here rather than absorbed silently, and the lead
may want it re-opened as its own guard the first time a model gains a `bodyState` read.

---

### G-21 — RETIRED, deleted, the working copy is in another file

Was must-never-move **T3-11**.

**The text it replaced, verbatim (lines 646-651 of the pre-conversion file):**

```
                // ⚠ SAFE ONLY BECAUSE BOTH CATEGORIES ARE MAPPED. `world` (4) landed with
                // task 39 and `character` (5) with task 43, at BOTH construction sites in
                // SimulationManagerUImpl.cpp. An unmapped category resolves to
                // ECollisionChannel(0), which IS ECC_WorldStatic — a silent WorldStatic
                // block. Task 40's `[SpatialQuery.UnmappedCategory]` diagnostic is the
                // regression guard; it must not fire for category 4 or 5.
```

**What broke if it moved.** The diagnostic that guards the category map must **not** fire for categories 4 and 5, and `world`(4) is deliberately mapped to the same value as the unmapped fallback. Anyone tightening the diagnostic by value rather than by map presence breaks it silently.

**Deleted, not relocated — the edit it forbids is typed in
`Source/OGSimulationUnreal/ChaosSpatialQueryAdapter.h`, and that file already says it
better.** At `:224-231` there: *“‘index in range’ and ‘category mapped’ are different
questions, and neither the stored value nor the vector’s size can separate them. Every T40
diagnostic keys on THIS mask; none of them keys on `m_toEngine.size()`.”* Presence
re-verified on the shipped bytes **before** this deletion.

⚠ Task 81’s audit cites that block as `:223-231`; measured today it is `:224-231`. A
one-line drift in a one-day-old audit, recorded because that audit predicted exactly this.

⛔ Task 75’s probe `C9` shows the category NUMERALS are constexpr-readable (`world == 4`,
`character == 5`), so an assertion here was expressible — and it would have pinned the
**numbering**, not the diagnostic’s **keying**, which is what the fence is about. Rejected on
narrowness, not on expressibility.

---

## §C — What this conversion COST, recorded here because a guards doc is where a reader looks

⚠⚠ **Nine pointers in five other files aim at prose in this header that no longer exists.**
They were swept **before** the conversion ran and are **routed, never fixed** — not one of those
files is this task’s to edit. Seven of the nine name a declaration that gets **no tag**, because
its subject was re-triaged as rationale rather than a fence, so they now land on nothing at all.

| inbound pointer | what it names | after conversion |
|---|---|---|
| `BrawlerCharacterBindings-guards.md:79` | `BrawlerMovementSimulation.h:4`’s `// for BodyId` | the trailing label is gone (v2 §1.4 item 5) — **dangles** | <!-- lint-anchor-ignore: this row CITES a citation -- the pair is `<another doc>:line :: <this header>:line`, i.e. exactly the dangling inbound pointer the row exists to report. The first element is a .md by construction, which the PAIR rule cannot accept. -->
| `BrawlerCharacterBindings-guards.md:230` | `BrawlerMovementSimulation.h:43-44`, quoted verbatim | the fence is deleted (G-18) — **dangles** | <!-- lint-anchor-ignore: this row CITES a citation -- the pair is `<another doc>:line :: <this header>:line`, i.e. exactly the dangling inbound pointer the row exists to report. The first element is a .md by construction, which the PAIR rule cannot accept. -->
| `BrawlerMovementVisualization.h:334` | *“the derivation is at `kProbeShrink`”* | lands on **G-03**, which is the prohibition; the derivation is in the rationale doc — **two hops, wrong doc** |
| `BrawlerMovementVisualization.h:424` | *“`BrawlerMovementSimulation.h`’s `kProbeShrink` block”* | same |
| `SimulatableBrawlerTypes.h:254` | *“Read `hoverPullDownAccel`’s comment in …”* | that member carries no tag — **dangles** |
| `SimulatableBrawlerTypes.h:250-255` | *“Read `hoverPullDownAccel`’s comment … before moving it”* | same |
| `SimulatableBrawlerTypes.h:272` | *“read `hoverDampingRatio`’s comment in …”* | that member carries no tag — **dangles** |
| `RoundVsPacketBudgetTest.cpp:475` | *“recorded at the “ADDING A MOVEMENT MODEL” block”* | the block is gone — **dangles** |
| `OgTagAliases.cpp:30` | *“Task 11 shipped an “ADDING A MOVEMENT MODEL” instruction”* | past tense; survives as history, but the block it names is gone |

⭐ **The mechanism, stated so it is not re-discovered.** The measured lesson from the phase so far
is that the one thing surviving every scale of decay is *a tag at the named declaration*. That
protection is **conditional on the cited declaration carrying a FENCE**, and here is the split:

| | pointers | what happens |
|---|---|---|
| names text that was DELETED outright | **2** | nothing is left at the site — `// for BodyId` and the acyclicity fence |
| names a declaration or block whose comment was ORIENTATION or a DERIVATION | **5** | v2 gives it no tag — `hoverPullDownAccel` (×2), `hoverDampingRatio`, the `ADDING A MOVEMENT MODEL` block (×2) |
| names a declaration that DOES carry a fence | **2** | a tag survives — both are `kProbeShrink`, and both still degrade, because what they cite is the GEOMETRY and the tag points at the PROHIBITION |

⛔ **So v2 protects an inbound citation to a FENCE and drops an inbound citation to a
DERIVATION** — seven of nine here land on nothing at all. This is the first file in the phase
carrying enough derivation-shaped prose to show it.

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
<!-- lint-external-ref: detachesFromSupport -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: attackGroundNormal -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: estimateSupportV2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history (its body is estimateSupport's) -->
<!-- lint-external-ref: classifySupportV2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history (renamed classifySupport) -->
<!-- lint-external-ref: writesKnockbackVelocityV2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history (renamed writesKnockbackVelocity) -->
<!-- lint-external-ref: SupportLaw::V2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: SupportLaw::Legacy -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: BrawlerMovement.SlopeKnockbackStaysOnSlope -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: BrawlerMovement.XYKnockbackDoesNotDetach -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.FlatGroundPredicateKeepsTodaysAnswer -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.LiftTestIsSurfaceRelativeOnASlope -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.DirectionLeavingTheFaceLaunchesOnTheHitTick -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (its v2 twin ...UnderV2 remains) -->
<!-- lint-external-ref: GroundAlignedKnockback.FlatTargetWithAHorizontalDirectionKeepsTodaysBits -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.UnwalkableExactFaceOverAWalkableSweepIsWorldUp -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: BrawlerGroundSample.AlignedSampleIsTheSharedGroundNormal -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.V2EqualsLegacyOnEveryPlaneWithoutAnExactEdge -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
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
| `G-24` | task 84 | step 3’s `locked` branch — the guarded end-tick subtraction |
| `G-25` | og-ground-aligned-attacks task 4 | `sampleAttackGround`’s sweep — the sample comes from the State it is given |
| `G-26` | og-ground-aligned-attacks task 9b | step 2’s hit-tick knockback write — before the redirect (before the classification until task 37) |
| `G-27` | og-ground-aligned-attacks task 9b, rework 1 | `writesKnockbackVelocity`’s flat-rule `return` — the flat rule kept. **RETIRED by task 37 (§R)** |
| `G-28` | og-ground-aligned-attacks task 13b | step 2’s `estimateSupport` call (since task 37; step 2’s `face` until task 32, the Legacy arm until 37) — one ground for the movement and the attack |
| `G-29` | og-ground-aligned-attacks task 33 | `estimateSupport`’s face-contact branch — the centre probe is swept only on an edge contact |
| `G-30` | og-ground-aligned-attacks task 34 | `classifySupport`’s `previous` — the hysteresis reads the restored `State::flags`, never `DerivedState` |
| `G-31` | og-ground-aligned-attacks task 34 | `classifySupport`’s separation row — no velocity test detaches a body below its hover |
| `G-32` | og-ground-aligned-attacks task 35 | `redirectsIntoFace`’s `return` — a landing, a `Lip` and a body above its hover are never redirected, except on a hit tick above the hover, and `previous` comes from `State::flags` |

**Retired after the census:** G-07 (census row T3-17) and G-27, both by og-ground-aligned-attacks task 37 with
the Legacy support law (§R). **Live guards today: 20.**

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

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `inline constexpr float kProbeShrink = 0.01f;`
(`= 1.f;`, in centimetres, until og-sim-metres-yup task 5a wrapped the length; task 22 wrote the metre value).

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
`inline constexpr glm::vec3 kWorldUp = simFrame::kUp;` (`kWorldUp{0.f, 0.f, 1.f}` until og-sim-metres-yup
task 5a; `(0, 1, 0)` since task 19's flip to Y up).

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

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `inline constexpr uint8_t kInputFlagHoldGuard`.

⚠ **Re-sited by og-syncedInput-rework task 4 (2026-10-03).** The tag stood immediately above
`class PlayerInput`, the movement input slice whose `flags` byte held the bit. Task 4 deleted that
class (the byte is `simulatableBrawler::SyncedPlayerInput::flags` since task 3) and moved the tag to
the input bit constant, where the next one will be typed (comment rule §9.3: move a tag to the site
where its edit is typed). The prohibition is unchanged.

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
0.0707 m at the 45° cap, 0.0259 m at 75° — is rationale, §23.

---

## G-10 — The two forbidden one-sided-servo variants, both written before being caught

**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`const float e = sd.rideHeight - ground.probe.clearance;` in step 4’s supported arm (`clearance` was a local of
step 2 until og-ground-aligned-attacks task 32; the site did not move).

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

**Tag site:** `BrawlerMovementSimulation.h`, immediately above `if (decision.support != SupportState::Unsupported)`
in step 4. ⚠ **Moved up one declaration** — see the note. Since og-ground-aligned-attacks task 32 the branch
reads `SupportDecision::support`, which step 2's `classifySupport(state, ground, committed)` returned; the site did
not move, and the decision is still step 2's. Since og-ground-aligned-attacks task 34 the support law v2 took the
decision from `classifySupportV2`, which task 37 renamed `classifySupport(state, sd, ground, committed, written, d)`
when it deleted the Legacy law (`BrawlerMovementSimulation-rationale.md` §34, §37); it is still in step 2.

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
`const float e = sd.rideHeight - ground.probe.clearance;` (`- clearance` until task 32) — which is also **G-10’s** site. Two tags on
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

> ⛔ **DO NOT REORDER THESE BRANCHES, AND DO NOT MERGE THEM INTO ONE CONDITION.** A
> committed state owns the velocity OUTRIGHT; `frozen` is the model’s gate, not a veto over a
> commitment. Holding guard during a knockback must NOT stop the slide.

⭐⭐ **[task 84, 2026-09-20] THERE ARE THREE BRANCHES NOW, NOT TWO, AND THE ORDER IS
`committed` → `locked` → `frozen` → the walk models.** `locked` is
`machineLocksMovement(machineState)` — the attack slide — and it is tested before `frozen` for
the SAME argument, not a new one: guarding during an attack is already blocked by
`DAttackGuardSimulation`, which disables every guard shape while `m_currentState != Idle`, and
that covers the whole swing. Freezing the body on the `holdGuard` bit during a swing would be the
same second, wrong mechanism this entry already forbids during a knockback, and it would stop the
slide dead on the tick the button goes down.
⛔ **`locked` is NOT folded into `committed`, and that is a separate prohibition with its own
consequence** — see the paragraph below.

**What breaks if it moves.** `frozen` is true whenever bit 0 of the input flags byte is held, and a
player who is being thrown five metres is very likely holding guard. Test `frozen` first and the
slide stops dead on the tick the button goes down — a 5 m throw becomes a 0 m throw, in the one
situation the player is most likely to create. ⭐ **Guarding is already blocked, and not by this
branch:** `DAttackGuardSimulation` disables every guard shape whenever
`attackMachineSimulation.m_currentState != DAttackState::Idle`, which covers the whole lockout. The
shape gate is the mechanism; freezing the body would be a second, wrong one.

⚠ **[og-brawler-3rdControllerMode task 5, 2026-10-02] Read "the tick the button goes down" above
as "the tick bit 0 is raised".** Bit 0 is now the client's freeze request: guard held **and** the
movement input neutral (`BrawlerMovementSimulation-rationale.md`, note R0-18). A thrown player who
holds guard without steering still raises it, so the prohibition and its consequence are unchanged.

⛔⛔ **[task 84] DO NOT MERGE `locked` INTO THE `committed` ARM, EITHER — and the reason is
not tidiness.** `committed` was also the last argument of `detachesFromSupport(state, up, face,
committed)` (`face` since og-ground-aligned-attacks task 13; deleted with the Legacy law in task 37, when the
detach decision became `classifySupport`'s rows), whose flat arm was
`committed && dot(velocity, up) > 0`. The attack slide writes the two
TANGENT-PLANE channels, and on a slope the `u` axis carries `sin(theta)` of world z — so a slide
UP a face would satisfy that arm, lose its hover hold and fly off the surface. That is precisely
the finding ⛔G-07 records for the up-slope knockback, and reproducing it for every ordinary
uphill swing would be a far larger defect than the one it records. `locked` therefore has its own
predicate and never touches `committed`. Pinned by
`BrawlerMovement.AttackSlideDoesNotDetachOnASlope`, whose control arm reproduces G-07’s detach
on the SAME fixture.

⚠ **The `frozen` BIT still records `frozen`.** `State::flags`’ bit 0 is set from the `frozen`
local regardless of which branch ran, so during a guard-held knockback — and, since task 84,
during a guard-held ATTACK SLIDE — the wire bit reads 1 while the body moves. Its only reader is
`BrawlerMovementVisualization.h`, so this is a display inaccuracy and nothing more — recorded
here so the next reader does not treat the bit as the answer to "did the model run".

✅ **Pinned by `BrawlerMovement.HoldGuardDoesNotFreezeASlide`**, which holds
`kInputFlagHoldGuard` through the launch tick and the tick after it, with a control arm proving the
bit is live in the same fixture, and — since task 84 — by
`BrawlerMovement.HoldGuardDoesNotFreezeAnAttackSlide`, its mirror for the swing, which additionally
asserts the frozen bit is set while the body is still sliding.

---

## G-24 — The end-tick subtraction is guarded, and the guard IS the stop

**Tag site:** `BrawlerMovementSimulation.h`, on the line above
`const uint32_t remaining = (machineState.m_attackEndTick > tick) ? … : 1u;` in step 3’s
`locked` branch.

**Added by:** task 84, 2026-09-20. Not part of task 68’s conversion census.

> ⛔ **DO NOT SIMPLIFY THIS TO `machineState.m_attackEndTick - tick`.** ⛔ **DO NOT REPLACE
> THE GUARD WITH AN `OG_CHECK(m_attackEndTick > tick)`.** ⛔ **DO NOT SATURATE AT 0.** The
> subtraction is UNSIGNED and its left operand is WIRE STATE. It must saturate at **1**, whose
> ratio `(R-1)/R` is exactly zero, because the only correct degrade for an end tick that has
> already passed is **stop now**.

**What breaks if it moves.** `m_attackEndTick` is `uint32_t` and so is `tick`. When the end tick is
at or before the current tick, `m_attackEndTick - tick` wraps to roughly 2³², and the ratio
`(R-1)/R` becomes `1 - 2⁻³²`, which rounds to **1.0f**. The slide is then multiplied by one on
every tick: it never decays, never stops, and **nothing crashes, nothing logs and no other fence
fires**. It is the quietest possible failure, and it happens on exactly the inputs a networked
build produces — a remote proxy that adopts a mid-swing correction carrying an end tick from a
prediction the local sim never ran, a hand-built test `State` (whose `m_attackEndTick` defaults to
`0`), or a future variable step that lets the machine stay `Attacking` past its own prediction.
Saturating at **0** instead would be a division by zero.

⛔ **It is a `G` and NOT an `OG_CHECK`, deliberately.** A stale end tick beside a live
`Attacking` is LEGITIMATE WIRE INPUT, not a programming error: the sim must not terminate the
process on a value a peer sent it. (The `OG_CHECK`s that do belong to this feature are on the
machine side, at the three write sites and in `swingTickCount`, where the operands are authored
data and the invariant really is a construction error.)

⚠ **It does not convert to a compile-time check.** The forbidden edit is a SIMPLIFICATION of a
runtime expression — deleting a ternary whose operands are both well-typed. Nothing about it is
visible to the compiler, and no type makes it unrepresentable without putting a wrapper around a
tick. Taxonomy: F1a + F5a, the "obvious simplification" class, like `G-02`.

✅ **Pinned by `BrawlerMovement.AttackSlideSaturatesWhenTheEndTickIsInThePast`**, which drives
both the end-tick-in-the-past arm and the exact `m_attackEndTick == tick` boundary with the stick
held IN the direction of travel — so neither the walk law nor a wrapped factor could have produced
the zero it requires — and carries a control arm one tick further out that reads half speed.

---

## G-25 — The attack ground sample comes from its own sweep of the State it is given

**Tag site:** `BrawlerMovementSimulation.h`, in `sampleAttackGround`'s body, immediately above
`return brawlerGround::GroundSample{estimateSupport(state, sd, bindings, query).faceNormal};`. Until
og-ground-aligned-attacks task 32 the body called `probeGround` itself, under the same tag; since then its one
sweep is the one `estimateSupport` makes, the movement's own support estimate. Since og-ground-aligned-attacks task 33
(the support law v2, the only law since task 37) an edge contact makes `estimateSupport` sweep the centre probe too, from the
same pose (guard G-29), so the sample then makes the same two sweeps the movement makes.

**Added by:** og-ground-aligned-attacks task 4, 2026-10-28. Not part of task 68's conversion census.

**The prohibition.** Do not replace this sweep with a value the movement already computed: not
`DerivedState::surfaceNormal`, not `DerivedState::contactNormal` (task 33), not a normal cached from an earlier tick. The sample must be a sweep from
the `State` it is given.

**The consequence.** `DerivedState` is off-wire scratch written during the movement step and not restored
on rollback, so on the first tick of a replay it holds the pre-rollback frontier's value, and on every
other tick it is the previous tick's. A sample read from it can differ between the authority and a
replaying client, and so can the swing plane frozen from it into the radial initial conditions (a wire
field): a correction per affected swing, with nothing failing locally. It is also the sweep's `normal`,
where the sample wants the exact face normal when the adapter provides one.

**What breaks if it moves.** This line makes the only sweep the sample makes (until task 14 a knob-off
branch above it issued none). `BrawlerGroundSample.AlignedPrefersTheExactFaceNormal` checks that the sample's
sweep is the movement probe's call, and the face selection.

---

## G-26 — The hit-tick knockback write comes before the redirect

**Tag site:** `BrawlerMovementSimulation.h`, in `integrate`'s step 2, immediately above
`const bool knockbackWritten = writesKnockbackVelocity(inboundHit, committed);`, the first of the write's two
statements; the second is `if (knockbackWritten) state.velocity = inboundHit.hitDirectionWorld * inboundHit.knockbackSpeed;`.
The site is between `estimateSupport` and `classifySupport`, where it has been since og-ground-aligned-attacks task 32.

**Added by:** og-ground-aligned-attacks task 9b, 2026-10-28 (user ruling OQ14). Not part of task 68's conversion
census. Task 9b's rework 1 moved the flat rule out of this entry into guard G-27 (retired by task 37, §R).
**Restated by** og-ground-aligned-attacks task 37, 2026-11-05: the entry's text until then is quoted at the end.

**The prohibition.**

> ⛔ **DO NOT MOVE THE WRITE BELOW THE REDIRECT.** `state.velocity = inboundHit.hitDirectionWorld *
> inboundHit.knockbackSpeed;` must run before `redirectsIntoFace(...)` and the assignment under it, which read and
> project the velocity, and so before the tangent frame and the decomposition that follow them.

**The consequence.** A hit whose direction points into the target's face is not projected into it: the written
velocity keeps its part into the face, and the push-out and the servo take that part instead of the face carrying
the slide. Seen failing in task 37 (the write moved below the redirect's assignment, nothing else changed):
`[@og]` 14 failed assertions.
- `GroundAlignedKnockback.V2HitTickIntoTheFaceGivesTheProjectedVelocity`: `6.29443f <= 0.002f`, with
  `d=(0, 0, -1) v=(0, 4.83667, -20) expected=(0, 8.66025, -15)`;
- `GroundAlignedKnockback.V2HitTickIntoTheFaceJustAboveTheHoverIsRedirected`, all eight headings: `10.0f <= 0.002f`;
- the slope golden's exact compare, first mismatch at tick 300 (`G.body5.CharacterCapsule.position`), and
  `BrawlerJoltSlopeGoldenTrace.KnockbacksFollowTheAttackersSwing`: G's hit on F at 410 travels 2.6684 m against
  3.9861 m, with an apex of 1.2414 m against 1.8964 m.

**Why it is the redirect and no longer the classification.** Until task 37 this entry forbade moving the write below
the support classification, because the Legacy `classifySupport` called `detachesFromSupport`, which read the
written velocity. The classification the header has now decides a written tick from the predicate's value and the
direction `d` (row 3 of `BrawlerMovementSimulation-rationale.md` §34), and its velocity rows are reached only on a
tick that wrote nothing, so it reads nothing the write changes. Measured in task 37: the write moved between the
classification and the redirect passes all of `[@og]` (939 cases, 256,336 assertions).

**What else this site carries.** `classifySupport` and `redirectsIntoFace` read the same `knockbackWritten`
local (the launch row, and the hit-tick terms of guard G-32). Step 3's hit tick is `velocityUV = currentUV` on
every hit tick, so it reads the write only through the velocity.

**The entry as it stood until task 37:**

>
> **Tag site:** `BrawlerMovementSimulation.h`, in `integrate`'s step 2, immediately above
> `const bool knockbackWritten = writesKnockbackVelocity(inboundHit, ground.faceNormal, ground.eligible, committed);`
> (`face` and `walkable` until og-ground-aligned-attacks task 32 put step 2's ground in a `GroundSupport`; the site
> did not move: it is between `estimateSupport` and `classifySupport`).
>
> **Added by:** og-ground-aligned-attacks task 9b, 2026-10-28 (user ruling OQ14). Not part of task 68's
> conversion census. Task 9b's rework 1 moved the flat rule out of this entry into guard G-27, whose
> wrong edit is typed in the predicate, not here.
>
> **The prohibition.**
>
> > ⛔ **DO NOT MOVE THE WRITE BELOW THE SUPPORT CLASSIFICATION.** `state.velocity =
> > inboundHit.hitDirectionWorld * inboundHit.knockbackSpeed;` must run before
> > `classifySupport(state, ground, committed)`, whose `detachesFromSupport` call reads the velocity. Written
> > after it, the
> > hit tick is classified with the pre-hit velocity: a direction that leaves the target's ground stays
> > `Supported` on its own hit tick, the frame keeps the probe normal, and the servo removes up to
> > `hoverMaxAccel · dt` (5 m/s) of the up channel. Seen failing in task 9b:
> > `GroundAlignedKnockback.DirectionLeavingTheFaceLaunchesOnTheHitTick` reports `1 == 0` for the support
> > check and `vertical -5.16333 expected -0.163333 lost 5 m/s` on all five headings.
>
> **The consequence.** A crease launch becomes a clipped hop, and the launch the user ruled for (OQ6)
> does not happen on the hit tick.
>
> **What else this site carries.** The hit-tick channel branch (step 3, inside guard G-23's `committed`
> arm) reads the same `knockbackWritten` local: a written tick takes `velocityUV = currentUV` (the
> decomposition of the written velocity), every other hit tick today's line. The derivation is in
> `BrawlerMovementSimulation-rationale.md` §26.
>
> **Under `SupportLaw::V2`** (og-ground-aligned-attacks task 34) the same local takes
> `writesKnockbackVelocityV2(inboundHit, committed)`, which has no flat rule, and the classification is
> `classifySupportV2`, which reads the written velocity and the predicate's value. The order is the same rule: the
> write before the classification. Every V2 hit tick takes `velocityUV = currentUV` (§34).

---

## G-28 — Step 2's ground is the one `estimateSupport` call the attack sample also makes

**Tag site:** `BrawlerMovementSimulation.h`, in `integrate`'s step 2, immediately above
`const GroundSupport ground = estimateSupport(state, sd, bindings, query);`. The attack sample's half of the rule
is guard G-25's site, `sampleAttackGround`'s `return`, which makes the same call on the same `State`.

**Added by:** og-ground-aligned-attacks task 13b, 2026-10-28 (task 10's open point 2, confirmed by its review as
L1). Not part of task 68's conversion census. **Moved and restated by** og-ground-aligned-attacks task 37,
2026-11-05: the helper the entry named, `attackGroundNormal`, was the Legacy law's and is deleted, and the rule now
covers the whole estimate (the support-law design's "kept and widened"). This is the site's fourth position: step 2's
`face` until task 32, `estimateSupport`'s body until task 33, its Legacy arm until task 37. The entry's text until
then is quoted at the end.

**The prohibition.**

> ⛔ **DO NOT RE-DERIVE THE GROUND IN STEP 2.** Step 2's face, frame, kind and clearance are this call's
> `GroundSupport`, the same function `sampleAttackGround` calls on the same `State`. Do not compute any of them in
> place from `ground.probe.hit`'s fields (for example the hit's `surfaceNormal` when it is non-zero, else its
> `normal`), and do not give step 2 an estimate of its own.

**The consequence.** At an edge contact the probe's hit is not the ground the estimate decides on. Measured in
task 37's trace on PlatformEdge (`BrawlerMovementSimulation-rationale.md` §37): 0.225 m inside the arc top's rim the
capsule probe's exact face is the arc's 60° hull facet and its GJK normal is tilted −29°, while the estimate takes
the platform's 30° top face from the centre probe. A face read from the hit there hands an unwalkable facet to the
classification, the redirect and the tangent frame, while the attack sample gets the top face, so the attack and the
movement disagree about one brawler's ground. Seen failing in task 37 (step 2 overwriting the estimate's face and
frame with the hit's exact face, else its sweep normal, on every tick that saw ground): `[@og]` 83 failed
assertions. PlatformEdge's mirror, which takes `estimateSupport` itself, reports `mirrorMismatches=325
supportMismatches=194` (and 27 lifts and 32 down-pulls against 4 and 19); both goldens fail their exact compare (the
flat one at tick 266, `A.body5.CharacterCapsule.position`; the slope one at tick 990, F); and
`BrawlerJoltSlopeGoldenTrace.KnockbacksFollowTheAttackersSwing` fails (G's hit on F at 1100 is first Unsupported
at 1101).

**What breaks if it moves.** PlatformEdge's mirror (`PlatformEdgeTest-rationale.md`) requires `integrate`'s support
and velocity to equal a mirror built from `estimateSupport`'s value on every mirrored tick (`mirrorMismatches=0`,
`supportMismatches=0`). `BrawlerGroundSample.V2SampleIsTheEstimatesFaceNormal` requires the sample to equal the
estimate's face, with the same sweeps.

**The entry as it stood until task 37:**

>
> **Tag site:** `BrawlerMovementSimulation.h`, in `estimateSupport`'s body, immediately above
> `const glm::vec3 faceNormal = attackGroundNormal(probe.hit, sd);`. Since og-ground-aligned-attacks task 32 both
> readers take that one call's value, `GroundSupport::faceNormal`: `integrate`'s step 2 as `ground.faceNormal`, and
> `sampleAttackGround` as `estimateSupport(state, sd, bindings, query).faceNormal` under guard G-25's tag. Until
> task 32 the tag stood in `integrate`'s step 2 above `face`'s own call, and `sampleAttackGround` made a second
> call to the helper. Since og-ground-aligned-attacks task 33 the tag stands in `estimateSupport`'s Legacy arm, below
> the `SupportLaw::V2` return: under V2 both readers take the face of `estimateSupportV2` instead
> (`BrawlerMovementSimulation-rationale.md` §33), which is again one value for both readers. The prohibition below is
> the Legacy law's, which shipped until og-ground-aligned-attacks task 36 (rationale §36).
>
> **Added by:** og-ground-aligned-attacks task 13b, 2026-10-28 (task 10's open point 2, confirmed by its review
> as L1). Not part of task 68's conversion census.
>
> **The prohibition.**
>
> > ⛔ **DO NOT RE-DERIVE THE GROUND FACE.** Step 2's face (`GroundSupport::faceNormal`) and the attack ground
> > sample are one value,
> > `inline glm::vec3 attackGroundNormal(const SweepHit& hit, const StaticData& sd)`: the hit's `surfaceNormal`
> > when it is non-zero, else its `normal`, kept only when the hit is blocked and
> > `glm::dot(face, kWorldUp) >= sd.cosMaxSlope`, else `kWorldUp`. Do not compute either reader's face from the
> > `SweepHit` fields in place, and do not drop the walkable test for step 2 because step 2 tests `walkable`:
> > `walkable` tests the sweep's `normal`, not the face. Seen failing in task 13b: step 2 back on
> > `glm::dot(probe.surfaceNormal, probe.surfaceNormal) > 0.f ? probe.surfaceNormal : probe.normal` fails
> > `GroundAlignedKnockback.UnwalkableExactFaceOverAWalkableSweepIsWorldUp` on tick 1 (`first differing field
> > velocity`, sweep normal (0, 1, −0), face (0, 0.5, −0.866), direction (0, 0, −1)) and
> > `BrawlerJoltSlopeGoldenTrace.KnockbacksFollowTheAttackersSwing` (`1101 == 1100`: G's hit on F at 1100 stays
> > `Supported` on its hit tick).
>
> **The consequence.** Near a convex edge the sweep's `normal` is walkable while the exact face is the box's
> 60° end face. A face taken without the walkable test feeds that end face to `writesKnockbackVelocity` and
> `detachesFromSupport`, while `sampleAttackGround` gives the attack `kWorldUp` for the same hit, so the attack
> and the movement disagree about one brawler's ground. In task 10's slope golden, G's hit on F at 990 (onto the
> plateau) detached through the slope arm with no height gained, and the one at 1100 (down the slope) counted as
> entering the ground.
>
> **What breaks if it moves.** `BrawlerGroundSample.AlignedSampleIsTheSharedGroundNormal` compares the sample
> with the helper and with the pre-task expression, bit for bit, on 1049 scripted hits. The slope golden trace's
> measurement (`groundOf` in `BrawlerJoltSlopeGoldenTraceTest.cpp`) classifies each knockback against the same
> helper.

---

## G-29 — The centre probe is swept only on an edge contact

**Tag site:** `BrawlerMovementSimulation.h`, in `estimateSupport`'s body (`estimateSupportV2`'s until og-ground-aligned-attacks task 37), immediately above
`if (contact.faceContact)`, the branch that returns the capsule probe's own exact face (or `None`) before the centre
probe's sweep, `query.sweep(bindings.queryVolumeIds[1], probePose, -up * probeLength)`, further down. The unblocked
return above it also comes before that sweep.

**Added by:** og-ground-aligned-attacks task 33, 2026-11-03 (support law v2; the task's spec: "The centre volume is swept
only on an edge contact"). Not part of task 68's conversion census.

**The prohibition.**

> ⛔ **DO NOT CONSULT THE CENTRE PROBE ON A FACE CONTACT, AND DO NOT MOVE ITS SWEEP ABOVE THIS BRANCH.** A contact
> whose exact face is within `kEdgeContactCos` (2°) of the sweep's GJK normal, or one whose adapter reports no exact
> face, is the face the capsule probe's clearance is measured to, so it is the frame
> (`BrawlerMovementSimulation-rationale.md` §33, the derivation of why the frame must be the face). The centre probe
> answers only where the capsule's contact is an edge.

**The consequence.**
- At a concave crease the capsule's first contact with the rising face comes while the centre is still over the flat
  face: measured in `SupportEstimate.ConcaveCreaseTakesTheRisingFaceFromItsFirstContact`, 0.105 m before the crease
  line. A centre that decided there would give the flat face as the frame while the clearance is measured to the
  rising face, and the servo would no longer act on the gap.
- An adapter or mock that gives no `surfaceNormal` (zero) never makes an edge contact, so it gets the probe's own face
  (the estimate the Legacy law gave, until task 37).
  With the sweep moved up it would sweep a second volume on every tick, and its estimate could take the centre's
  face.
- Every face tick would pay a second sweep per character.

**What breaks if it moves.** These cases count the centre's sweeps:
`SupportEstimate.FaceInteriorIsTheExactFaceWithNoCentreSweep` (none in 732 face-interior sweeps on Jolt) and
`BrawlerGroundSample.V2SampleIsTheEstimatesFaceNormal` (one sweep on a face contact, two on an edge). Until task 37
`GroundAlignedKnockback.V2EqualsLegacyOnEveryPlaneWithoutAnExactEdge` also counted them (none on any scripted plane
without an exact face); it compared the two laws and went with the Legacy one.

---

## G-30 — The hysteresis reads the restored `State::flags`, never `DerivedState`

**Tag site:** `BrawlerMovementSimulation.h`, in `classifySupport`'s body (`classifySupportV2`'s until
og-ground-aligned-attacks task 37), immediately above
`const SupportState previous = static_cast<SupportState>((state.flags & kFlagSupportMask) >> kFlagSupportShift);`.

**Added by:** og-ground-aligned-attacks task 34, 2026-11-03 (support law v2; the task's spec: "`previous` is
`(state.flags & kFlagSupportMask) >> kFlagSupportShift`; never `DerivedState::lastSupportState`"). Not part of task
68's conversion census.

**The prohibition.**

> ⛔ **DO NOT TAKE `previous` FROM `DerivedState::lastSupportState`, OR FROM ANY OTHER OFF-WIRE COPY.** V2's
> hysteresis row (`committed && previous == Unsupported && clearance > rideHeight`) decides this tick from last
> tick's SupportState. The separation row does not read `previous`: it is reached only when the hysteresis row did
> not fire. That value must be the one in `State::flags` bits 1–2, which ride the
> wire and the rollback snapshot (`SIM_MEMBER(S, flags)`). `DerivedState` is off-wire scratch that a rollback does
> not restore: after a restore it holds the value of the last tick that ran, not of the restored tick.

**The consequence.** On a resimulation the classification would read the support of a tick from the discarded
future. An airborne knockback that the replayed tick should keep `Unsupported` (row 2) could re-attach, or a
supported one could fall into row 2, so the replay diverges from the original run and the correction never
converges. A single-world rig cannot show it, because there both values always agree (the support-law scorecard
counts `supportSourceMismatches=0`).

**What breaks if it moves.** `GroundAlignedKnockback.V2HysteresisReadsTheRestoredFlagsNotDerivedState` sets the
two to opposite values and requires the decision to follow `flags`, both ways round.

---

## G-31 — No velocity test detaches a body below its hover

**Tag site:** `BrawlerMovementSimulation.h`, in `classifySupport`'s body (`classifySupportV2`'s until og-ground-aligned-attacks
task 37), immediately above the separation row
`else if (committed && clearance >= sd.rideHeight`, whose condition goes on to
`glm::dot(state.velocity, n) > glm::max(kMinLaunchSine * glm::length(state.velocity), kMinSeparationSpeed)`.

**Added by:** og-ground-aligned-attacks task 34, 2026-11-03 (support law v2, design rule "below its hover a body is
never detached by velocity"; it takes in the initiative's task 24). Not part of task 68's conversion census.

**The prohibition.**

> ⛔ **DO NOT DROP `clearance >= sd.rideHeight` FROM THE SEPARATION ROW, AND DO NOT ADD ANOTHER VELOCITY TEST THAT
> CAN DETACH A SUPPORTED BODY BELOW ITS HOVER.** Below `rideHeight` the servo lifts a supported body, so an upward
> velocity there is the servo's own correction, not a launch. Only the launch row (a written hit tick, decided by
> the direction) and the end of the ground (`kind == None`) may detach a body there.

**The consequence.** The servo's recovery lift detaches the body it is lifting. Seen failing in task 34 (the term
deleted, everything else kept): the support-law scorecard's crease rig, run under V2, hops at the entering crease
again: `[SupportLaw.crease.departures] law=v2 convexDepartures=22 worstBallisticErrMps=0.0000 gravityStepMps=0.1633
concaveHops=18`, with `[SupportLaw.scorecard] law=v2 rig=crease runs=56 flips=37 hops=40 apexMaxM=5.0532
slideEndTicks=30` (with the term: 0 concave hops, 17 flips, 22 hops, 12 slide-end ticks). The flat rig stays at 0
flips with or without the term: there the separation floor and the cone are larger than the servo's lift.

**What breaks if it moves.** `SupportLawScorecard.CreaseCrossingsUnderV2` requires 0 concave hops. The constant's
`static_assert` holds `kMinSeparationSpeed` between three times the measured crossing residue and half the smallest
intended separation (`BrawlerMovementSimulation-rationale.md` §34).

---

## G-32 — A landing, a `Lip` and a body above its hover are never redirected, except on a hit tick above the hover, and `previous` comes from `State::flags`

**Tag site:** `BrawlerMovementSimulation.h`, in `redirectsIntoFace`'s body, immediately above its
`return support == SupportState::Supported && committed`, whose condition goes on to
`ground.kind == SupportKind::Face && (written || ground.probe.clearance <= sd.rideHeight)` and
`(static_cast<SupportState>((state.flags & kFlagSupportMask) >> kFlagSupportShift) == SupportState::Supported || written)`.

**Added by:** og-ground-aligned-attacks task 35, 2026-11-03 (support law v2, the redirect at a concave crease; the
task's spec: "Landings (`previous == Unsupported`, not written) are not redirected"). Task 35's rework 1, the same
day, added the `Face` and hover terms (a lead ruling on the redirect's scope). Rework 2, the same day, added
`written ||` to the hover term (a lead ruling: a hit tick is not a fall). Not part of task 68's conversion
census. No compile-time form exists: every term is a runtime decision on this tick's estimate and the restored State.

**The prohibition.**

> ⛔ **DO NOT DROP OR WIDEN `(previous == Supported || written)`, `ground.kind == SupportKind::Face` OR
> `(written || ground.probe.clearance <= sd.rideHeight)`, DO NOT DROP THAT TERM'S `written ||`, AND DO NOT TAKE
> `previous` FROM `DerivedState::lastSupportState`.** The redirect removes a committed body's velocity into its
> face only when the body was already on the ground last tick (a crossing onto a new face) or when this tick's hit
> wrote the velocity (a hit into the face), only on a `Face`, and, unless this tick's hit wrote the velocity, only
> at or below the hover. A committed body whose last tick was Unsupported and that this tick did not write is
> landing: its velocity into the face is the fall that the hover servo and step 6′ absorb (ruling #28's soft
> landing). Above the hover the same holds for every body this tick did not write: there the one-sided servo is
> gravity plus a pull-down, so a velocity into the ground is a fall that the hysteresis and the servo own, as for a
> landing (the mirror of G-31). A written velocity is not a fall, so that reason does not cover a hit tick; and a
> body resting at its hover has a clearance equal to `rideHeight` only to within rounding, so without `written ||`
> a hit into the face on a body at rest would be redirected or not by that rounding.
> A `Lip` has no surface to slide along: its face is `kWorldUp` (rationale §33), so on a `Lip` the redirect would
> take the downward velocity of every body that slides over a low edge or a tip. `previous` is `State::flags`
> bits 1–2 for the reason G-30 gives: `DerivedState` is not restored by a rollback.

**The consequence.** With the landing term dropped, a landing loses its velocity into the face on the landing tick,
so the servo's damping never acts on it: the fall stops in one tick instead of being caught by the servo. With
`previous` taken from `DerivedState`, a resimulated tick decides the redirect from the support of a discarded tick.
With the hover term dropped, the redirect removes the one-sided servo's pull-down on every tick above the hover where
it exceeds the 2° cone, which at a slow speed is nearly every tick: a slow body over a convex crest floats and bleeds
its speed. With the `Face` term dropped, a body that slides down a face onto a `Lip` loses its downward velocity
there. With `written ||` dropped, a hit into the face on a body at rest a rounding above `rideHeight` is not
redirected: the body meets the face, and the push-out and the servo take the velocity into the face instead. Seen
failing:
- task 35, the landing term deleted: `GroundAlignedKnockback.V2LandingIsNotRedirected` fails
  `REQUIRE( !movement::redirectsIntoFace(landing.state, landing.estimate(), movement::SupportState::Supported, true, false) )`
  on flat ground at a clearance of 0.04 m (the call as it read before rework 1 added its `sd` argument);
- task 35, `integrate` handing the redirect a State whose flags carry `DerivedState::lastSupportState`: the same case
  fails `REQUIRE( glm::dot(landing.state.velocity, n) < glm::dot(sliding.state.velocity, n) )` with
  `2.116f < 1.83667f` (the landing was redirected and the slide was not);
- rework 1, the hover term deleted: `SupportLawScorecardTest.cpp(516): FAILED: CHECK( measured == pinned )` with
  `12 == 14` and the message `rig crease field slopeToPlateau.crossed measured 12 pinned 14`, the only failure in
  `[@og]`. The two slopeToPlateau runs at d = 4.5 m, ±30°, float over the crest (clearance 0.1437 m against
  0.1224 m at tick 114) at 0.3103 m/s against 0.6650 m/s, and stop short of the crease line;
- rework 1, the `Face` term deleted: `PlatformEdgeTest.cpp(1013): FAILED: CHECK( e.estimateRedirect == 0.f )` with
  `1.67458f == 0.0f`, on the one allowed estimate lift (tipCornerPlus, d = 4.9 m, oblique 0°: anomaly 1.85156, servo
  0.176981, redirect 1.67458 m/s), the only failure in `[@og]`. Of PlatformEdge's other V2 checks none fails: the 48
  tip and low-edge `Lip` departures that the redirect turns into lifts under the spec's expression need the hover
  term dropped too (task 35's measurement), because their `Lip` tick is above the hover;
- rework 2, `written ||` deleted: `[@og]` `940 | 938 passed | 2 failed` (17 assertions). On all eight headings
  `GroundAlignedKnockbackTest.cpp(1539): FAILED: CHECK( movement::redirectsIntoFace(written, rig.sd, rig.estimate(),
  movement::SupportState::Supported, true, true) )` with `false`, and `GroundAlignedKnockbackTest.cpp(1552): FAILED:
  CHECK( err <= 1e-4f * speed )` with `10.0f <= 0.002f` (heading 0°: `v=(0, -0.163333, -20)` against
  `expected=(0, 8.49692, -15)`; the 10 m/s is the velocity into the face), at 9.98e-6 m above `rideHeight`; and
  `BrawlerJoltSlopeGoldenTraceTest.cpp(799): FAILED: CHECK( std::string(printed) == "tick=410 travelM=3.9861
  apexM=1.8964" )` with `"tick=410 travelM=2.6684 apexM=1.2414"`. In that golden G's target, F, rests at a
  clearance that prints 0.100000; unredirected, F meets the face (clearance 0 on ticks 411 and 412) and slides at
  14.77 m/s on tick 412 against 16.90 m/s, with a smallest `minClearanceM` of 0.8101 m against 0.9706 m. No rig line
  moves.

The landing and `previous` poisons move no line of the support-law scorecard or a golden v2 print: no rig run has a
committed landing with a velocity into the face beyond the cone, so only the unit case guards those two.

**What breaks if it moves.** `GroundAlignedKnockback.V2LandingIsNotRedirected` sets the landing and the slide up
identically except for `previous`, with `DerivedState::lastSupportState` set opposite to `flags` in both, and requires
the predicate and the integrated velocity to follow `flags`. `SupportLawScorecard.CreaseCrossingsUnderV2` pins the
slopeToPlateau crossings at 14 (the hover term). `PlatformEdge.KnockbacksAcrossTheImportedPlatformRimsUnderV2` allows
at most one estimate lift, at tipCornerPlus d = 4.9 m along the fall line, with no part of it from the redirect (the
`Face` term), and no estimate down-pull. `GroundAlignedKnockback.V2HitTickIntoTheFaceJustAboveTheHoverIsRedirected`
places a committed, previously Supported body 1e-5 m above `rideHeight` on a 30° face and requires the predicate
false unwritten and true written, and the hit tick's velocity to be the projected one plus the above-hover servo's
vertical step; `BrawlerJoltSlopeGoldenTrace.KnockbacksFollowTheAttackersSwing` checks G's hit at 410 to within
5e-5 m of its travel and apex (both pin the `written ||`; until task 36 the V2 print case checked it at the print's
precision).

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

⚠ **G-07 and G-27, at the end of this section, are the exception.** Both were live tags until
og-ground-aligned-attacks task 37 deleted their sites with the Legacy support law.

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

**What broke if it moved.** A re-created include cycle. It is not silent: measured by og-netcode-v2-field-defects task 18 (2026-09-23): re-adding the include to `DAttackMachineSimulation.h` fails with `C2653` in every translation unit that reaches the machine header first, `SimulatableBrawler.h` included, and compiles only when the movement header is entered first. So the compiler holds it, in the include order the product uses.

**Deleted, not relocated — the edit it forbids is typed in another file, and that file
already said so.** `DAttackMachineSimulation.h` carried the working copy until og-netcode-v2-field-defects
task 18 converted that header; its text is now in `DAttackMachineSimulation-rationale.md` §1: *“the
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
`Plugins/OGSimulation/Source/OGSimulationUnreal/ChaosSpatialQueryAdapter.h`, and that file already says it
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

### G-07 — RETIRED, deleted with the Legacy support law (og-ground-aligned-attacks task 37)

**Retired 2026-11-05.** The site, `detachesFromSupport`, is deleted with the Legacy support law
(`BrawlerMovementSimulation-rationale.md` §37). Since og-ground-aligned-attacks task 34 the shipped law's detach
decision is `classifySupport`'s rows (rationale §34); task 36 made that law the default, and task 37 removed the
predicate's last caller. Where each part of the entry went:
- *the arm is keyed on velocity, never on an enumerator*: still true of the shipped law's rows, which read the
  direction and the velocity. The compile-time tripwire `!detail::kHasLaunched<DAttackState>` holds it, and since
  task 37 its message names `classifySupport`;
- *do not fold the flat arm into the slope arm*: retired with the flat arm. The shipped law has one formula on every
  face;
- *do not test the slope arm against `up`; do not drop the `|velocity|` scale or lower `kMinLaunchSine`*: the bound
  on `kMinLaunchSine` is the `static_assert` beside the constant. The separation row's use of the face normal and of
  the speed scale is recorded in rationale §37; no guard carries it.

**Was must-never-move:** `T3-17`. The census table at the top of this file is closed and still lists that row as a
live guard; this entry is its retirement. The entry as it stood until task 37 follows, unchanged.


**Tag site:** `BrawlerMovementSimulation.h`, immediately above
`inline bool detachesFromSupport(const State& state, const glm::vec3& up, const glm::vec3& face, bool committed)`
(the signature since og-ground-aligned-attacks task 13, a template on the initiative's compile-time knob
until task 14 retired it; before task 13, `(state, up, committed)`, and before task 27, `(state, up)`).

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
WORLD height, and on a SLOPE the tangential channel carries `knockbackSpeed · sin θ` of world height —
10 m/s on a 30° face at the shipped speed — so an up-slope knockback DOES reach the true arm
on the tick after the shove and loses its hover hold for the rest of the slide. The frame does not
move with it (`n` is keyed on `walkable`, not on `support`), so the slide stays on the face.
`BrawlerMovement.SlopeKnockbackStaysOnSlope` prints both readings and records the support state;
the finding and its recommended fix — key the arm on the vertical CHANNEL, which is computable
here because the frame does not depend on the detach decision — are routed to the lead in
`impl/impl_notes_seam_27.md`. **Not changed unilaterally: the arm is the architect’s, ruled.**
⭐ **[og-ground-aligned-attacks task 13]** The slope arm below answers this finding for a sloped
face: it tests the velocity against the face, not against world up. Until the flip (task 10) the
initiative's compile-time knob kept that arm off and the finding above described the build; task 14
retired the knob, so the slope arm is the only one off flat ground.

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

⭐⭐ **REVISED BY og-ground-aligned-attacks TASK 13, 2026-10-28 — THE LIFT TEST IS SURFACE-RELATIVE
ON A SLOPE (user ruling OQ6, 2026-10-28; old OQ7 (a)).** The site and the id are unchanged; the
predicate takes the
probe's face normal `face` (the hit's `surfaceNormal` when non-zero, else its `normal`: the face rule
of `sampleAttackGround`; since og-ground-aligned-attacks task 13b it is `attackGroundNormal(probe, sd)`,
which also gives `kWorldUp` for a miss or a face past the walkable cap, guard G-28; since task 32 it is
`GroundSupport::faceNormal`, and `classifySupport` makes the call). Two arms:

- **Flat** (`surfaceFrame::isFlat(face)`): `committed && glm::dot(state.velocity, up) > 0.f`,
  the task-27 arm, verbatim.
- **Slope** (a non-flat face): `committed && glm::dot(state.velocity, face) >
  kMinLaunchSine * glm::length(state.velocity)`. A knockback whose direction leaves the target's
  ground by more than 2° detaches (a launch); one in the ground does not (a slide).

What this guard now forbids, in addition to the enumerator rule above:

> ⛔ **DO NOT FOLD THE FLAT ARM INTO THE SLOPE ARM.** On a flat face the slope formula is not the
> flat one: `> kMinLaunchSine · |v|` is a 2° cone, `> 0.f` is a half-space, so a knockback rising at
> less than 2° on flat ground would stop detaching. Flat ground must run
> `committed && glm::dot(state.velocity, up) > 0.f` exactly (the initiative's flat rule).
> `GroundAlignedKnockback.FlatGroundPredicateKeepsTodaysAnswer` pins the predicate to that answer on
> five flat faces (10,000 cases). Its integrate twin, which compared the knob-off and the knob-on
> `integrate` on flat ground, went with the knob in task 14.
>
> ⛔ **DO NOT TEST THE SLOPE ARM AGAINST `up`.** An in-plane up-slope knockback on a 30° face carries
> `0.5 · |v|` of world height, so a world-up test detaches every same-face up-slope shove (the hop
> task 27 measured). Seen failing in task 13: with `up` in the slope arm,
> `GroundAlignedKnockback.LiftTestIsSurfaceRelativeOnASlope` fails on the in-plane case
> (`dot(v, n)=0 dot(v, up)=10`).
>
> ⛔ **DO NOT DROP THE `|velocity|` SCALE OR LOWER `kMinLaunchSine` TO JOLT'S NORMAL ERROR.** The
> threshold is a sine, relative to the speed, so the decision depends on the direction and not on
> how fast the knockback is. It must stay above 0.0221, the bound √(2·1e-4/0.41) on the Jolt
> sweep-normal error (design §5 of the initiative): at or below it a normal outlier decides a
> launch. The compiler holds the bound (`static_assert(kMinLaunchSine > 0.0221f, …)` beside the
> constant); `kMinLaunchSine` is sin 2° = 0.0348995.

The `!detail::kHasLaunched<DAttackState>` tripwire stays as it was: there is still no `Launched`
enumerator, and its message now names both arms. The tangent frame's change while detached is a
derivation, not a prohibition, and is in `BrawlerMovementSimulation-rationale.md` §30.

### G-27 — RETIRED, deleted with the Legacy support law (og-ground-aligned-attacks task 37)

**Retired 2026-11-05.** The site, the Legacy `writesKnockbackVelocity`'s flat-rule `return`, is deleted with the
Legacy support law (`BrawlerMovementSimulation-rationale.md` §26.1, §37). The flat rule kept a flat-ground hit with a
horizontal direction on the pre-initiative expressions verbatim until the flip. The shipped law's write has no flat
rule by design (rationale §34), it has been the default since task 36, and both goldens were re-recorded under it, so
nothing is left for the prohibition to protect. The name `writesKnockbackVelocity` now belongs to that write,
`writesKnockbackVelocity(inboundHit, committed)`. The entry as it stood until task 37 follows, unchanged.


**Tag site:** `BrawlerMovementSimulation.h`, in `writesKnockbackVelocity`'s body, immediately
above `return !(surfaceFrame::isFlat(g) && simFrame::height(d) == 0.f);`. The predicate is
`inline bool
writesKnockbackVelocity(const brawlerInboundHit::DerivedState& inboundHit, const glm::vec3& face, bool walkable, bool committed)`
(a template on the initiative's compile-time knob until task 14 retired it).

**Added by:** og-ground-aligned-attacks task 9b's rework 1, 2026-10-28, split out of guard G-26 (review
B1: the flat-rule edit is typed in this `return`, which carried no tag). The rule itself is task 9b's
(user ruling OQ14). Not part of task 68's conversion census.

**The prohibition.**

> ⛔ **DO NOT DROP THE FLAT RULE.** `writesKnockbackVelocity` returns false when `g` (the face when
> `walkable`, else `simFrame::kUp`) is flat and `simFrame::height(d) == 0.f`. That tick must run today's
> `velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed;` and the pre-hit up channel,
> verbatim (the initiative's flat rule). Writing the velocity there rebuilds the hit tick from
> `d · speed` and drops the pre-hit up channel (a horizontal `d` has none): other bits, and a pre-hit
> vertical velocity is lost. Seen failing in task 9b (`return true;` in place of the flat test),
> already on a target at rest: `GroundAlignedKnockback.FlatTargetWithAHorizontalDirectionKeepsTodaysBits`
> fails on tick 1 with `first differing field velocity`. Since task 14 that case's reference is the same
> rig with its world direction zeroed (the zero-`d` `false` return), which is what the knob-off rig it
> replaced computed on flat ground; the same poison was seen failing again on it in task 14
> (`first differing field velocity` on tick 1; the horizontal section of
> `GroundAlignedKnockback.UnwalkableExactFaceOverAWalkableSweepIsWorldUp` failed with it).

**The consequence.** Every flat-ground knockback changes its bits after the flip, and a target hit
with a non-zero vertical velocity (falling or rising) loses it on the hit tick.
The flat golden and the flat-ground tests would have to absorb both.

**What else this site carries.** The predicate's other `false` returns (a tick that is not a committed
hit tick, a zero `d`) are not the flat rule. The line above the tag picks `g`, the
plane the rule tests. `integrate`'s step 2 reads the predicate (guard G-26).

**The law it belongs to.** Since og-ground-aligned-attacks task 34 this predicate is
`SupportLaw::Legacy`'s only (the shipped law until task 36). `SupportLaw::V2` writes through `writesKnockbackVelocityV2`, which has no flat rule
by design (`BrawlerMovementSimulation-rationale.md` §34): the prohibition above stands for Legacy until the
initiative's task 37 retires it.

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

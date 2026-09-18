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
<!-- lint-external-ref: impl/impl_notes_seam_27.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: DetachGateIsVacuousUntilJumpLands -- a RETIRED token -- task 27 DELETED this Catch2 case and replaced it with XYKnockbackDoesNotDetach; the prose exists in order to say the name is gone, so a resolving name would falsify it -->
<!-- lint-external-ref: paramName -- not an identifier at all: it is the PLACEHOLDER inside the `/*paramName=*/` call-site annotation convention (CommentExtractionRule v2 section 1.4), and the real names are the constructor parameters it stands in for -->
<!-- lint-external-ref: dWorld -- an identifier from a REJECTED alternative spelling, never in shipped code -- section 26 names `glm::vec2(dot(dWorld, u), dot(dWorld, v))` as the wrong edit the derivation tag exists to stop; a resolving name would mean the wrong edit had landed -->
<!-- lint-external-ref: UCharacterMovementComponent -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: UCharacterMovementComponent::ComputeFloorDist -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: UCapsuleComponent::InitCapsuleSize -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: UpdatedComponent -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: AddMovementInput -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: bAutoActivate -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: CollisionShape.h -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: CapsuleComponent.h -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: SurfaceKind -- a RETIRED token -- this prose exists in order to say the name is gone (task 56 replaced SurfaceKind{Airborne, Floor} with SupportState) -->
<!-- lint-external-ref: Airborne -- a RETIRED token -- this prose exists in order to say the name is gone (task 56 replaced SurfaceKind{Airborne, Floor} with SupportState) -->
<!-- lint-external-ref: MachineInputT -- a RETIRED template parameter -- task 62 deleted it, and this prose records why the indirection existed and why it went -->
<!-- lint-external-ref: Jumping -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: kFlagJumping -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: Dashing -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: commandIssued -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: capX -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: penDepth -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: velocityN -- an identifier from an archived spike probe or a poison arm, never in shipped code -- the superseded surface-normal actuation ruling #26 removed -->
<!-- lint-external-ref: pushOutTicks -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: worldPushOut -- a name for a QUANTITY in an argument, not a declaration in code -->
<!-- lint-external-ref: dashDir -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence (task 31 supplies it) -->
<!-- lint-external-ref: kInputFlag -- a PREFIX, not an identifier -- the sentence is about the naming convention that keeps the input flags byte distinct from State::flags -->
<!-- lint-external-ref: Done -- a Backlog STATUS word, not a symbol -->
<!-- lint-external-ref: Backlog.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl_notes_seam_54.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl/design_hover_slope_transient.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl/research_spike_9.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: research_spike_9.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: design_ledge_fall_and_landing.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: BrawlerInputPackaging.h:153 -- a DEAD citation, quoted DELIBERATELY: note R0-13 exists in order to record that task 65's published correction cites a line past the end of a file since rewritten 197 -> 128 lines. The lint is RIGHT and so is the note -->
# `BrawlerMovementSimulation.h` — rationale

The narrative, the provenance and the **derivations** for the brawler character-movement
sub-simulation. The header carries the code and nothing else; the fences live in
`BrawlerMovementSimulation-guards.md`; everything below is the *why*.

**If this file and `BrawlerMovementSimulation.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **Do not move a guard into this file.** Every `⛔G-nn` tag in the header points at
`BrawlerMovementSimulation-guards.md`, and that separation is a hard gate under
`tools/lint/guard_tag_lint.ps1`. A prohibition filed as narrative is read after the edit, not
before it.

⛔ **This file is not the source of truth for any VALUE.** The constants, the gains, the flag
bits and the wire layout are in the header. The authored tunables are in
`SimulatableBrawlerTypes.h` and, for the cvar path, in the UE layer's `MovementSchemeCVar.cpp`.
Where a value appears below it is there to make an argument readable.

---

## How to read this file

⭐⭐ **Every quoted paragraph below is the header's own text, carried across unchanged.** The
only transformation applied is the removal of the `// ` comment prefix. Nothing is paraphrased,
re-worded or summarised, and each block carries an HTML comment giving the line range it
occupied in the pre-conversion header, so any paragraph can be diffed against version control.

⛔ **The reason is not tidiness.** This header's comments are not justifications — they are
**derivations a reader needs while reading the code**: a Jury-criterion stability recurrence, a
2×2 dual-basis solve, a capsule-probe geometry with a `sec θ` error term. Retyping a derivation
is how a derivation acquires a defect. Extraction is a **move**.

⚠ **R0 corrections are NOT edited into the carried text.** Every false or stale claim found by
the three audits behind this conversion — and three found by this conversion itself — is an
editorial note **immediately before** the paragraph it corrects, marked `R0-nn`. A false sentence
can therefore never be read here without its correction attached, and nothing has been laundered
by being moved. ⛔ Where the correction belongs to another task or needs a user ruling, the note
says so and the text is left alone.

**Origin:** `brawler-movement-simulation` tasks 1, 5, 8, 11, 13, 14, 15, 16, 17, 19, 22, 27, 31,
39, 43, 46, 47, 49, 50, 51, 54, 56, 57, 62, 64, 65, 68, 75, 76 and 81; user rulings #5, #13,
#14(c), #16(a), #17(b), #26(a), #27(A), #28 and #29.

> ⚠ **The initiative names in that Origin line are private working material from initiative
> archives and are NOT distributed with this submodule.** They are named as provenance,
> deliberately unlinked; every claim this file *asserts* is anchored to a file in this repository
> and only to those. ⚠ A small number of the carried paragraphs quote such a document by path
> (`impl/…`). Those quotations are **verbatim** and were not edited, because the text they are
> part of is a fence or a derivation that must move unchanged.

---

---

## 1. The include graph — what this header pulls in, and why

The three include-site paragraphs, in file order. The acyclicity fence that stood at
lines 43-46 is **not** here: it is guard **G-18**, retired and deleted, because
`DAttackMachineSimulation.h` already carries the working copy at the site where the
forbidden include would be typed.


<!-- header lines 25-33 -->
> [movement-sim task 62] `simulatableBrawler::CharacterBindings` is DEFINED IN THE LEAF HEADER
> INCLUDED ON THE NEXT LINE, not in this file any more. It is the one type the machine sub-sim
> wants from here, and keeping it in a header whose only dependency is `BodyId` is what lets the
> machine header have it without dragging in this one. Consumers of THIS header still see the
> type, transitively, so no call site changed.
> ⚠ [movement-sim task 64] THE NAMESPACE IS `simulatableBrawler`, NOT this file's
> `brawlerMovementSimulation`. It read the other way until task 64. THIS SUB-SIM NEVER USES THE
> TYPE — every mention of it left in this file is a comment; movement's capsule id comes from
> its own `RuntimeBindings` below.

> ### R0-01 — ROUTED — not this task's to fix
>
> ⚠ The paragraph below is the one the deleted acyclicity fence (guard **G-18**) sat
> under. `BrawlerCharacterBindings-guards.md:230` quotes lines 43-44 of the pre-conversion
> file verbatim; those lines no longer exist. **Routed to the lead, not fixed** — that
> file belongs to another task.


<!-- header lines 35-42 -->
> ⭐ [movement-sim task 62] THE MACHINE HEADER, INCLUDED OUTRIGHT — the dependency points ONE
> WAY now: movement -> machine. Movement reads the machine's `State` (the flinch freeze, step 3)
> and its `PlayerInput` (the move stick is packed onto the machine slice), so
> `machineFreezesMovement` and `integrate` SPELL those types instead of deducing them.
> Before task 62 this include was impossible: `DAttackMachineSimulation.h` included THIS header
> for `CharacterBindings`, so including it back was a cycle — and `#pragma once` does not turn a
> cycle into an error, it silently leaves one side incomplete depending on which header the
> translation unit entered from, which is the worst available failure mode.


<!-- header lines 47-55 -->
>
> ⚠ THE INPUT EDGE IS STILL UNDECLARED, and this include does not fix that. `Dependencies`
> below declares `External<const dAttackMachineSimulation::State&>`, so `findFirstViolation`
> validates the STATE edge against `ExecutionOrder`. The `PlayerInput` edge is declared NOWHERE,
> and `InputType = brawlerMovementSimulation::PlayerInput` positively asserts this sub-sim reads
> only its own input slice — untrue since step 3 began reading `machineInput.moveDirectionWorld`.
> What this include buys is that the edge is visible in the signature and CHECKED BY THE
> COMPILER. Teaching the dependency graph about INPUT edges means changing
> `OGSimulation/SimulationDependencies.h`, which every sub-sim shares: a separate framework task.

---

## 2. Orientation — what this sub-simulation is and who owns what

The header opened with a 72-line orientation block (lines 62-133 of the pre-conversion
file). ⭐ It is **split, not moved whole**, exactly as this task's acceptance criterion
requires: THREE of its lines were must-never-move rows and went to the guards doc — **G-19**
(T3-2), **G-12** (T3-3) and **G-20** (T3-4), all three retired; the rest is below, in five
parts.

> ### R0-02 — the include count is deliberately not restated
>
> ⛔ The deleted fence said the graph was *"Re-verified at task 62 for all seven of its own
> includes"*. Measured 2026-09-11, `DAttackMachineSimulation.h` has **nine** og-brawler
> includes — eight sub-simulation headers plus `OGBrawlerLog.h`, which both the header and
> task 65's own published correction missed — and **thirteen** non-`glm`, non-`std`
> includes in total. The fence quantified over *every* include, not the og-brawler subset.
> ⭐ **No new number is written here.** A bare count in prose is a claim with a maintenance
> cost and no enforcement, and this one drifted twice in two consecutive tasks. The property
> that matters is that nothing reachable from that header reaches this one, and it is
> re-verified by walking the includes, not by counting them.


<!-- header lines 62-72 -->
> Home of the brawler character-movement sub-sim.
>
> [movement-sim task 11, 2026-09-06] THE REAL SUB-SIM. The skeleton (task 1) is gone:
> this sub-simulation now owns the character CAPSULE, decides what it is standing on,
> runs a movement model in a surface-relative 2D frame, hovers at a fixed clearance and
> writes both halves of the body's motion every tick.
>
> ============================ WHO OWNS WHAT (user ruling #14(c)) ============================
> The sim owns `{position, velocity}` and RE-PLACES the body every tick
> (`setBodyTransform` + `setBodyLinearVelocity`). The solver is used for SEPARATION ONLY,
> and its contribution is read back as a POSITIONAL push-out. The captured


<!-- header lines 74-104 -->
```text

  position   sim, engine-corrected  we write it; the post-solve capture returns
                                    `position + worldPushOut`; the sim adopts that.
  velocity   sim, exclusively       model output, plus the ONE vertical law (gravity
                                    always on, a spring-damper hover servo while
                                    `Supported` — ruling #28), plus ONE authored contact
                                    rule (kill the into-contact component, §step 6' below).
  ground     sim                    hover servo on the `sweep` probe — the capsule does
                                    not rest in contact, it floats `rideHeight` above.
  walls,     Chaos, POSITION ONLY   the capsule blocks `world` and other `character`
  brawlers                          capsules; the solver's push-out is the one thing
                                    read back. Two equal-mass capsules separate 50/50.
  rotation   locked                 `lockRotation` on the descriptor is what makes
                                    `LinearBodyState`'s fabricated identity rotation TRUE.

Why this shape and not "let Chaos integrate forces" (revision 5, retired): a refused or
unapplied Chaos rewind cannot lose a correction when the body is re-placed from `State`
on every replay tick. Task 9's q1(iii) measured that a sim-side correction converges
identically whether Chaos grants the rewind or refuses ~90% of them.

============================ ADDING A MOVEMENT MODEL ============================
A movement model is a PURE 2D FUNCTION of the surface frame. It never learns which plane
it is in: on flat ground the frame is world XY, on a slope it is the slope, on a wall
(task 20) it will be the wall. To add one:

  1. Add an enumerator to `MovementModel` (append; the value is authored data).
  2. Write one free function `computeDesiredVelocityUV_<Model>(sd, state, tick, stickUV,
     currentUV, dt)` beside the two below, and add its `case` to the `switch (sd.model)`
     in step 3. That switch is the ONLY place `sd.model` may be named — steps 2, 4 and 5
     are model-agnostic by construction and an acceptance criterion greps for it.
  3. If the model needs memory, APPEND fields to `State` and APPEND their `SIM_MEMBER`s.
```

> ### R0-03 — FALSE — an arithmetic elision, and the routing that came with it is CANCELLED
>
> ⛔ The paragraph below quotes the **margin** cost (10.264 B) and then the quotient
> produced by the **net closure** cost (10.764 B), dropping the one sentence that
> reconciles them. A reader who divides the two numbers it gives them gets 2.665 and
> concludes one of them is wrong. `RoundVsPacketBudgetTest.cpp:466-473` carries the missing
> term: the half-entry floor itself rises 0.5 B per stride byte, so net closure is
> 10.764 B per input byte and 27.352 / 10.764 = 2.5411 ✓.
>
> ⛔ **The defect is HEADER-ONLY.** Task 65 concluded the root cause was in
> `RoundVsPacketBudgetTest.cpp` and routed it there. That routing is **cancelled**: the test
> file is internally consistent and says so in the line the audit skipped. Re-verified
> 2026-09-11.
>
> ⭐ The safe form of the same figures is 15 lines further down this header, at
> §13: *"ate 10.264 B of the ordinary-join margin — a quarter of everything that was left.
> ~2.5 bytes remain."* It rounds and never states the quotient, so it never commits to the
> inconsistency. **The same author elided the same term twice and only the precise site went
> false.** Do not "fix" the other one.
>
> ⛔ **AND THE SECOND HALF OF THIS BLOCK IS FALSE TOO, AND IS TASK 58'S.**
> *"bits 1-7 are reserved for those four"* is one of **three** sites in this header (the
> others are in §13) and **eleven across six files** making the same claim. The corrected
> form is already shipped, by name, in `BrawlerInputPackaging-rationale.md`: **bits 1-7 are
> UNASSIGNED.** Task 48 adds *no* input field; tasks 20, 21 and 31 each say *"if this task
> adds an input field"*. ⚠ And **task 20 is Wall run**, not "wall-grab" — this header
> spells it correctly once (§4) and wrongly twice. **Owner: task 58. Carried verbatim,
> flagged, not rewritten.**


<!-- header lines 108-133 -->
```text
  4. Add an LLT file / cases under `[BrawlerMovement]`.

⛔⛔ STANDING RULE ON PLAYER INPUT (lead, 2026-09-06) — A NEW PER-TICK INPUT FIELD IS A
BIT IN A FLAGS BYTE, NEVER A NEW `bool` MEMBER. An input byte is about TEN TIMES more
expensive than a state byte, because it is multiplied across every ring entry in the
packet-budget scenario: `holdGuard`, one `bool`, moved the ring entry stride 81 -> 82 B and
cost 10.264 B of the join-alone margin at the character cap, leaving **27.352 B of slack
above the half-entry floor — 2.54 more input bytes before the half-entry floor guard in
`RoundVsPacketBudgetTest.cpp`'s "the pre-diet cap is 4" case goes RED.** One bool spent a
quarter of the remaining budget. Jump (task 21), dash (31), wall-grab (20) and ski-tuck
(48) are all coming; as four `bool`s they blow that fence twice over, as four bits in one
byte they cost nothing.
⭐ [movement-sim task 51, 2026-09-06] THAT SHAPE IS NOW THE SHIPPED ONE: `PlayerInput` is
`uint8_t flags`, `holdGuard` is bit 0, and bits 1-7 are reserved for those four. It moved
ZERO wire bytes — `bool` and `uint8_t` are both 1 B — so every pin in
`RoundVsPacketBudgetTest.cpp` and `SimulatableBrawlerTest.cpp` was re-quoted UNCHANGED.
That is precisely why it was worth landing BEFORE task 14, the field's only writer: a
re-layout that moves no bytes is verifiable in isolation, and 14 is then written against
the final shape instead of pinning a `bool` it would have to un-pin. The packing rule and
the constant that carries it live AT THE TYPE, on `PlayerInput` below. The full
arithmetic, with every term, is in `RoundVsPacketBudgetTest.cpp`'s pre-diet table block.

Constraints on everything in this header: `glm` only (no `<cmath>`, no engine types), and
NO PER-TICK TRANSCENDENTAL — the single `cos` is in `StaticData`'s constructor, which runs
once per session. `OGBLOG_G` fires only on surface-kind CHANGES, Cadence commits and
teleports; never unconditionally per tick.
```

---

## 3. `MovementModel` — which law turns the stick into a 2D velocity


<!-- header lines 139-141 -->
> Which movement law turns the stick into a surface-relative 2D velocity. Read once per
> session from `StaticData` so every peer runs the same one; it is NOT per-tick state and
> never travels on the wire.


<!-- header lines 144-145 -->
> CMC-like: accelerate toward `stick * maxWalkSpeed` at `acceleration`, brake toward
> zero at `brakingDeceleration`. Continuous, no committed direction.


<!-- header lines 147-148 -->
> Direction is COMMITTED every `stepPeriodTicks` and held at a constant `stepSpeed`
> in between — the "steps, not a joystick" feel.

---

## 4. `SupportState` — whether the character is being held up this tick


<!-- header lines 152-166 -->
> ⭐⭐ WHETHER THE CHARACTER IS BEING HELD UP THIS TICK — movement-sim TASK 56 / user ruling
> #28, 2026-09-07. It replaces `SurfaceKind {Airborne, Floor}`, and it is NOT a rename of the
> same idea. `SurfaceKind` named a GEOMETRIC FACT (a walkable probe hit inside the band);
> this names a DECISION about support. They differ exactly where the old name lied: during a
> jump's ascent (task 21) the probe still finds walkable floor underneath, and the servo must
> nevertheless be OFF.
>
> ⛔ SO THE DETACH GATE LIVES IN STEP 2'S CLASSIFICATION, NOT IN STEP 4.
> `detachesFromSupport` below is the one predicate that can turn a walkable probe hit into
> `Unsupported`; step 4 only READS the answer. Putting the gate in step 4 would re-create the
> value-that-is-not-true this rename exists to remove, and would give the vertical law a second
> place to disagree with the flags byte a correction restores.
>
> Stored in `State::flags` bits 1-2 — the SAME TWO BITS `SurfaceKind` used, so this moves ZERO
> wire bytes and the enum may still grow to 4 values before the field has to widen.


<!-- header lines 169-170 -->
> Gravity ONLY. Ballistic flight, a jump's ascent (task 21), clearance beyond the probe
> band, or a hit whose normal failed the walkable test. The servo term does not run.


<!-- header lines 172-172 -->
> The servo term runs, along `up` (user ruling #26 a).


<!-- header lines 174-178 -->
> RESERVED for the 45°-75° band (task 48), where the servo axis follows `n` per the
> tasks-20/48 carry-forward. ⛔ v1 NEVER PRODUCES IT: step 2's walkable test is a single
> `cosMaxSlope` comparison, so anything steeper than `maxSlopeAngleDeg` classifies
> `Unsupported`. It is declared now so the wire field's value range is settled once,
> rather than widening the flags byte later.


<!-- header lines 182-187 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row — it pins a VALUE claim the comments
> above make and nothing checked.
> ⚠ AND IT IS NARROW, DELIBERATELY SO: it pins the three enumerators that EXIST, each of which
> rides State::flags bits 1-2 and is therefore a wire value. It does NOT catch a FIFTH
> enumerator — nothing expressible here does, short of a count constant this enum does not have
> — so the sentence above keeps that half.

---

## 5. `CharacterBindings` left this header


<!-- header lines 197-204 -->
> ⭐ [movement-sim task 62] `CharacterBindings` MOVED OUT of this header, to
> `OGBrawler/BrawlerCharacterBindings.h` — included at the top, so every consumer of this
> header still sees it and not one call site changed; its full provenance comment moved with
> it. It left because the machine sub-sim needs the type and must NOT include this header.
> ⭐ [movement-sim task 64] AND THEN THE NAMESPACE FOLLOWED THE TYPE: it is
> `simulatableBrawler::CharacterBindings` now. `brawlerMovementSimulation` was collateral from
> T35's file move, not a modelling call — this sub-sim never uses the type, and every real
> consumer sits in or under `simulatableBrawler`, where the struct started.

---

## 6. The sim's fixed step, and the discrete stability bound

⭐⭐ **This is the file's longest derivation and the reason task 75 argued this file may not
earn a v2 conversion.** The Jury-criterion recurrence below sat directly above the check it
justifies. It is now here, and the header carries `⛔G-01` in its place.
⚠ Whether that trade works for a reader standing at
`OG_CHECK(hoverGainsAreStable(…))` is the open question this conversion was run to answer.


<!-- header lines 208-215 -->
> ⭐⭐ THE SIM'S FIXED STEP, AND IT IS NOT AUTHORED HERE. `Config/DefaultEngine.ini`'s
> `AsyncFixedTimeStepSize=0.016667` is the authority; `integrate` is handed the real `dt` and
> uses THAT, never this. This constant exists for exactly one purpose: the discrete stability
> bound on the hover gains has to be checked where the gains are AUTHORED (`StaticData`'s
> constructor, once per session), and `dt` is not in scope there.
> ⚠ If the sim clock ever moves off 60 Hz this constant must move with it, or the check below
> silently guards the wrong region. That is why it is named, commented, and pinned by an LLT
> (`HoverGainsAreStableAtTheSimStep`) against the rig's own `kDt` rather than left a literal.


<!-- header lines 220-223 -->
> ⭐ [movement-sim task 76] THE DISCRETE STABILITY BOUND, AS A PREDICATE — so the derivation
> written out above `StaticData`'s constructor can be CHECKED BY THE COMPILER and not only read.
> The constructor's `OG_CHECK` calls this same function on the AUTHORED pair, so the runtime
> check and the two compile-time witnesses below cannot drift apart: there is one body.


<!-- header lines 230-232 -->
> ⛔ THE OBVIOUS BOUND IS THE WRONG ONE, AND THIS IS THE BUILD BREAK THAT SAYS SO. Rewrite the
> body above as the explicit-Euler `omega * dt < 2` — the reflex edit, and the one three people
> derived first — and BOTH witnesses below start passing it, so this assertion fires.


<!-- header lines 239-240 -->
> ⚠ AND THE CONTROL IS LOAD-BEARING, not decoration: `return false` satisfies the assertion
> above and would leave the bound un-guarded while looking enforced.

---

## 7. `StaticData` — the authored constants, field by field

> ### R0-04 — FALSE — the walk-speed default is spelled in two files and four places
>
> ⛔ *"the walk-speed literal is spelled ONCE, at the construction site in
> `SimulatableBrawlerTypes.h`"* is false, and was already false when task 65 measured it as
> "two locations that agree". Re-measured 2026-09-11, there are **four spellings in two
> files**, plus a fifth inside a log string:
>
> | site | what it is |
> |---|---|
> | `SimulatableBrawlerTypes.h:150` | `movementMaxWalkSpeed = 100.f` — the parameter default |
> | `MovementSchemeCVar.cpp:121` | `GMoveSpeed = 100.f` — the cvar default |
> | `MovementSchemeCVar.cpp:128` | `"… default 100. READ ONCE when …"` — the help string |
> | `MovementSchemeCVar.cpp:346` | `MaxWalkSpeed = 100.f` — ⛔ **the REFUSAL fallback** |
> | `MovementSchemeCVar.cpp:344` | `"Falling back to 100."` — inside the refusal log |
>
> ⛔ `:346` is the one that matters and no artefact in this initiative names it: it is what
> the character actually walks at when `OGBrawler.MoveSpeed` is set negative and refused. A
> change to the authored default that does not also change the refusal fallback leaves a
> cvar typo silently walking at the old speed. **That is a UE-layer hazard, not a comment
> defect — ROUTED to the owner of `MovementSchemeCVar.cpp`, not fixed here.**
>
> ⚠ `SimulatableBrawlerTypes.h:152` `movementStepSpeed = 100.f` is **not** a sixth site — a
> different knob that happens to share the value. Recorded so the next sweep does not fold
> it in.
>
> ⚠ Task 16 is `Done` (2026-09-08). The paragraph's *"Task 16 replaces the tunables with
> one-time cvar reads"* is a future tense about shipped work.


<!-- header lines 248-254 -->
> The movement sub-sim's authored constants. Held by value as
> simulatableBrawler::StaticData::m_movementStaticData, which PhysicsDeclaration::
> staticDataOf returns to the generic registration fold.
>
> ⭐ R-P1: the walk-speed literal is spelled ONCE, at the construction site in
> SimulatableBrawlerTypes.h — not here, not in the UE layer, not in the CMC. Task 16
> replaces the tunables with one-time cvar reads; nothing else about this type changes.


<!-- header lines 275-277 -->
> The ONE transcendental in this header, and it is SETUP time. Step 2 compares
> `glm::dot(probe.normal, up) >= cosMaxSlope` — a dot product against the servo's own
> axis (ruling #26 a) — so the walkable test costs no trigonometry per tick.


<!-- header lines 287-289 -->
> DERIVED ONCE, HERE, from the ω/ζ pair — the per-tick law multiplies and never squares.
> k = ω², c = 2ζω: the standard second-order form, so a tuner moves a FREQUENCY and a
> DAMPING RATIO rather than two gains whose units nobody remembers.


<!-- header lines 300-302 -->
> ⭐⭐ THE DISCRETE STABILITY BOUND, CHECKED WHERE THE GAINS ARE AUTHORED — because the
> alternative is finding it in PIE, and three people derived it wrong first.
>


<!-- header lines 310-328 -->
```text

THE ACTUAL RECURRENCE, with y = clearance − rideHeight, a = c·dt, b = (ω·dt)²:
    [y'; v'] = [[1 − b, dt(1 − a)], [−k·dt, 1 − a]] [y; v]
    trace T = 2 − a − b,  determinant D = 1 − a
Jury's criterion (|D| < 1 and |T| < 1 + D) collapses to  b + 2a < 4  and  0 < a < 2,
and the first implies the second. With W = ω·dt, b = W² and a = 2ζW that is:

         ⭐ W² + 4ζW < 4 ⭐           W = hoverFrequency · dt

→ ζ = 1.00 admits ω < 49.71;  ζ = 0.62 admits ω < 66.79;  ζ = 0.50 admits ω < 74.16.
MEASURED against the recurrence itself, not just derived: ω = 60 ζ = 1 diverges
(1, 1, 2, 3, 5, 8, 13 …) and ω = 119 ζ = 1 reaches 10⁶ in eight ticks. Both PASS
`ω < 120`. The shipped pair sits at W² + 4ζW = 2.489.

⚠ STABLE IS NOT THE SAME AS SMOOTH — read `hoverDampingRatio`'s comment below for the
monotone region, which is tighter and is where the feel lives.
[movement-sim task 76] ONE PREDICATE, TWO READERS: this check on the AUTHORED pair,
and the two static assertions beside `hoverGainsAreStable` that pin the bound's SHAPE.
They cannot disagree, because the bound is written once.
```

> ### R0-05 — FALSE ×2 — the argument count, and `ACharacter`
>
> ⛔ *"The ctor already takes 19 positional arguments, and a 20th trailing `bool`"* —
> counted on the shipped bytes 2026-09-11, `StaticData`'s constructor takes **22**
> parameters, so a trailing `bool` would be the **23rd**. The argument the sentence makes is
> unaffected and gets stronger; only the numbers are wrong.
>
> ⛔ *"`UCharacterMovementComponent` … the component the factory adopts"* and every other
> `ACharacter` in this file describe a type this game stopped using at task 19:
> `AOGBrawlerUECharacter : public APawn` (`OGBrawlerUECharacter.h:49`), and the CMC was
> retired from the gameplay path at task 15. ⭐ **The useful framing, from task 81's audit:
> this is not a lone stale file, it is the LAST present-tense site.** Every other place in
> the tree already reads *pawn* or past tense — `SimulationManagerUImpl.cpp:593` and `:1287`
> both say *"the factory ADOPTS the pawn's own root capsule"*. **Owner: task 58.**
>
> ⚠ **One `ACharacter` is NOT correctable by any comment pass**: it is inside the message of
> a `static_assert` task 76 landed (*"…instead of adopting the ACharacter's own capsule…"*).
> That is code, this task may not reword it, and a message is exactly where a stale noun is
> hardest to notice. **Routed.**


<!-- header lines 348-376 -->
```text
SIM-LOCAL, NOT SERIALIZED and deliberately not a constructor parameter.
⭐ `true` SINCE TASK 15: step 5 re-places the body every tick and this sub-simulation
IS the character's locomotion. `false` computes the whole of `State` but skips the two
adapter writes — the passenger arm, which now exists only for the LLT rig, and which the
rig PINS for itself (`Rig()` in `BrawlerMovementSimulationTest.cpp`) rather than
inheriting from here.

⛔⛔ THIS FLAG IS HALF OF A PAIR, AND THE PAIR IS LOAD-BEARING. `drivesBody` only says
what the SIM does; what the CMC does is decided by `PhysicsSetup::body.simulatePhysics`.
⭐ BOTH WERE FLIPPED TOGETHER AT TASK 15, AND NEITHER MAY BE FLIPPED BACK ALONE:
  * `simulatePhysics = true` beside `drivesBody = false` leaves NOBODY driving the
    capsule. UE 5.6's `UCharacterMovementComponent` early-returns whenever
    `UpdatedComponent->IsSimulatingPhysics()`, on BOTH its sync and its async path, and
    the component the factory adopts IS the CMC's `UpdatedComponent`
    (`SimulationManagerUImpl.cpp` hands the factory `character->GetCapsuleComponent()`).
    The CMC switches itself off, step 5 is skipped, and engine gravity is off too: an
    inert no-gravity rigid body sitting where it spawned. THE PLAYER CANNOT MOVE.
  * `drivesBody = true` beside `simulatePhysics = false` leaves TWO AUTHORITIES — the sim
    writing the body while the CMC still drives the same component.
  Read `PhysicsSetup::body`'s comment before changing either one.
⛔ The TELEPORT seed (step 0) writes the body regardless of this flag: a respawn must
move the capsule.

⚠ WHY A DEFAULTED MEMBER RATHER THAN A CTOR PARAMETER. This is a one-way architectural
switch, not a tunable: nothing in production should ever set it false again. The ctor
already takes 19 positional arguments, and a 20th trailing `bool` after a run of `float`s
is a live miswiring hazard for the tasks that extend that list (16, 20, 48). A test that
wants the passenger arm assigns it by name on its own instance, which reads better at the
call site than a positional `false` nineteen arguments deep.
```


<!-- header lines 379-379 -->
> ContinuousAccelBrake


<!-- header lines 381-381 -->
> Cadence (20 ticks = 1/3 s at the 60 Hz sim clock)


<!-- header lines 385-386 -->
> Attachment. `cosMaxSlope` is derived from `maxSlopeAngleDeg` in the ctor and is the
> form step 2 actually uses; both are kept so a debugger shows the authored degrees.


<!-- header lines 390-397 -->
> The SIM's gravity law, per character. NEGATIVE (world −Z).
> ⭐ [task 56 / ruling #28] APPLIED ON EVERY TICK, WITH NO BRANCH THAT TURNS IT OFF — the
> servo HOLDS THE BODY UP AGAINST IT while `Supported` rather than replacing it, which is
> what a suspension does and what makes the vertical velocity continuous at the support
> boundary. It used to run only while `Airborne`, and the seam between that branch and the
> dead-beat one was the defect ruling #28 removed.
> ⛔ The BODY's `enableGravity` is FALSE — see PhysicsSetup. Under ruling #14(c) engine
> gravity would double-apply against this term and corrupt the very push-out step 6' reads.


<!-- header lines 401-405 -->
> Hover geometry. Steady state is `clearance == rideHeight` with `velocityUp == 0` —
> CONTACT-FREE, which is why no standing contact manifold exists for the solver to fight.
> ⭐ Ruling #13, closed 2026-09-06 ON MEASUREMENT (task 9 q3), and these two STAND: a 40 cm
> drop at the exact edge of probe range was still detected and caught. They superseded the
> revision-6 text's 20/150.


#### `hoverFrequency` — the hover-gains group banner ∴D-05

<!-- header lines 408-417 -->
> ⭐⭐ THE HOVER SERVO'S GAINS, IN FEEL UNITS — movement-sim TASK 56 / ruling #28.
> ⛔ `maxSnapSpeed` (400 cm/s, ruling #13's dead-beat clamp) IS GONE, and the whole law it
> clamped went with it. It was a VELOCITY ceiling on a law that ASSIGNED velocity from
> position error; there is no assignment left to clamp. `hoverMaxAccel` is its replacement in
> role only — an ACCELERATION ceiling on a law that integrates. A stale ini or cvar naming
> `maxSnapSpeed` must be rejected loudly rather than silently ignored; there is no cvar/ini
> path in the tree yet (task 16 builds it), so that obligation is recorded HERE and belongs
> to task 16 when it lands.
>
> ω in rad/s. `hoverStiffness = ω²` below. Bigger = snappier and less forgiving.


#### `hoverDampingRatio` — the DISCRETE critical damping ratio ∴D-03

<!-- header lines 419-444 -->
```text
ζ, dimensionless. `hoverDamping = 2ζω` below.

⭐⭐ ζ = 1 IS *NOT* CRITICAL DAMPING HERE, AND THAT IS THE WHOLE POINT OF THIS COMMENT.
ζ = 1 is the continuous-time answer. This servo is a DISCRETE recurrence, and the value
that makes ITS response monotone is

         ⭐ ζ_crit = 1 − ω·dt / 2 ⭐            (0.6167 at ω = 46, 60 Hz)

— the ζ at which the two eigenvalues coincide (T² = 4D ⇔ (a + b)² = 4b ⇔ ζ ≥ 1 − W/2).
MEASURED on a 1 cm perturbation, error per tick:
    ζ = 1.00   .412 .483 .161 .238 .057 .120 .016 .062   ← alternates sign: it RINGS
    ζ = 0.62   .412 .141 .045 .014 .004 .001 .000 .000   ← monotone, 95 % in ~4 ticks
At ζ_crit the repeated eigenvalue is λ = 1 − W, so the error decays ×λ per tick.

⚠ AND MONOTONE NEEDS λ > 0 AS WELL, WHICH IS A SECOND, TIGHTER BOUND: λ = 1 − ω·dt > 0
⇒ ω < 1/dt = 60 rad/s at 60 Hz. Critical damping buys the FASTEST decay, not a monotone
one — ω = 70 (ζ_crit 0.417) and ω = 80 (ζ_crit 0.333) are both critically damped and both
alternate sign every tick. ⭐ SO THE SAFE TUNING REGION IS `ω < 60` WITH `ζ ≈ 1 − ω·dt/2`,
not a point. ω = 46 sits inside it with λ = +0.233.
⚠ RECORDED AS A LANDMARK AND NOT A RECOMMENDATION: ω = 60, ζ = 0.5 puts BOTH eigenvalues
at exactly 0 — a dead-beat, reached in one tick. It is knife-edge (λ crosses zero there)
and it reintroduces the abruptness ruling #28 moved away from. Do not ship it.

⚠ THE VALUES THEMSELVES ARE UNMEASURED FEEL. The LAW is ruled; ω and ζ are provisional
defaults the user tunes in PIE and then records in ruling #28. Step 0 logs both, and the
ζ_crit for the configured ω beside them, so a tuning session can see which side it is on.
```


<!-- header lines 446-452 -->
> The servo term's acceleration ceiling, cm/s². A fall that outruns it bottoms out into
> contact and is caught by depenetration plus step 6' — the same safety net the dead-beat's
> velocity clamp had under it.
> ⚠ The TOTAL vertical acceleration is `gravity + clamp(servo, ±hoverMaxAccel)`, so the true
> per-tick velocity bound is `(hoverMaxAccel + |gravity|)·dt`, not `hoverMaxAccel·dt`. The
> feed-forward is inside the clamped term; that is what buys the exact steady state, and it
> is what makes the bound asymmetric by exactly one `gravity`.


#### `hoverPullDownAccel` — the one-sided servo's one knob ∴D-04

<!-- header lines 455-469 -->
```text
⭐⭐ THE ONE-SIDED SERVO'S ONE KNOB — movement-sim TASK 57 / USER RULING #29, 2026-09-07.
A CEILING ON THE DOWNWARD PULL, in cm/s², applied ONLY above ride height. `0` — the shipped
default, and the user's own request — means the vertical law above ride height is
**gravity and nothing else**: SUPPORTED MEANS HELD UP, NEVER PULLED DOWN.

⚠ IT IS NOT `hoverMaxAccel`'s twin. `hoverMaxAccel` bounds the servo's authority in BOTH
directions and stays 30 000; this bounds the SPRING TERM ALONE, and only on the arm where
the spring would act WITH gravity. Raising it makes step-downs snappier (980 ⇒ 2 g total
⇒ a 40 cm step recaptured in ~12 ticks instead of ~17); it does not make landings firmer,
it does not change the upward bound, and it can never lift the character.
⚠ WHAT IT COSTS AT 0, MEASURED (task 57): the character rides a hair below ride height while
walking, because a tick that lands a float hair ABOVE ride height gets gravity alone and
falls `|gravity|*dt²` = 0.272 cm before the servo takes it back. That ripple is one tick
deep and is the price of the one-sided law; the design predicted it (design_ledge_fall_
and_landing.md §1 "Why no chatter at e = 0") and PIE decides whether it is visible.
```


<!-- header lines 471-473 -->
> DERIVED in the constructor from the pair above. k = ω², c = 2ζω. Not constructor
> parameters: a tuner must not be able to author a k and a c that no (ω, ζ) produces, which
> is exactly how the stability check above would be bypassed.


<!-- header lines 476-478 -->
> Knockback / launch (task 27). Travel distance is the closed form
> knockbackSpeed² / (2·launchDecel) = 2000² / 8000 = 500 cm = 5 m — the user's
> "thrown 5 metres and then quickly come to a stop".

⛔ **`knockbackSpeed` IS NO LONGER A MEMBER OF THIS TYPE — task 27, 2026-09-12**, and the
sentence above is kept because it is the provenance of the 5 m, not because the field is here.
The speed became **per attack**: `HitReactionSpec::knockbackSpeed`, authored in
`simulatableBrawler::StaticData::m_hitReactions` beside the sequence table it is indexed by. Only
`launchDecel` remains here, and that is the split the design argues for — **the decel is the LAW
and the speed is the HIT**. The closed form therefore reads
`spec.knockbackSpeed² / (2·sd.launchDecel)` and is per attack; at the shipped right/left rows it
is still exactly 500 cm.

⚠ **THE CONSTRUCTOR CALL SHIFTED, AND IT IS NEARLY ALL `float`.** Removing one argument from a
21-argument positional call reassigns every constant after it and **compiles cleanly**. The call in
`SimulatableBrawlerTypes.h` now carries a `/*paramName=*/` annotation on EVERY argument — an
explicit exemption under the comment rule, and the only thing that makes this class of error
visible at the site forever. It was also checked by measurement: every remaining member was printed
with its value before and after the edit and the two lists diffed byte-identical
(`impl/task27/sd_before.txt`, `sd_after.txt`).


<!-- header lines 481-481 -->
> Dash / dodge (task 31). `dashSpeed` is ASSIGNED on the entry tick, never added.

> ### R0-06 — FALSE — a line number that is wrong at two sites
>
> ⛔ `OGBrawlerUECharacter.cpp:69` does not contain `InitCapsuleSize`; `:69` is a `// ====`
> separator. The call is at **`:96`**, `InitCapsuleSize(42.f, 96.0f)`, re-verified
> 2026-09-11. ⚠ The identical wrong number is duplicated verbatim at
> `SimulatableBrawlerTypes.h:277` — **routed, not fixed.**
>
> ⭐ The `42 / 96` contract is stated at **four** sites and the values agree at all four:
> this header, `SimulatableBrawlerTypes.h:277`, and `OGBrawlerUECharacter.{h:62,cpp:82}` —
> the last two opening *"⛔⛔ 42 / 96 IS A CONTRACT"*. Two carry the stale line number and
> two do not.


<!-- header lines 485-487 -->
> MUST equal the ACharacter's authored capsule (OGBrawlerUECharacter.cpp:69
> InitCapsuleSize(42.f, 96.f)). The adopt-root factory path does not resize the
> capsule — it `checkf`s that the descriptor AGREES with it.

---

## 8. The ground probe's lateral inset — task 54 ∴D-02

The second of the four blocks this task's acceptance criterion names (66 lines,
493-558 of the pre-conversion file). ⭐ **Split:** the prohibition at 514-518 is guard
**G-03**, tagged on `kProbeShrink`; the defect report, the arithmetic and the cost table
are below.

> ### R0-07 — FALSE — three line numbers into `ChaosSpatialQueryAdapter.cpp`
>
> ⛔ The block below cites `bFindInitialOverlaps = true` at `:543-544` and the
> minimum-`Time` blocking hit at `:564-571`. Measured 2026-09-11 they are at **`:547`** and
> **`:565-574`**. The quoted sentence is verbatim-correct; only the numbers drifted.
> ⭐ **Do not write new numbers.** Three `grep -F`-unique content anchors exist and survive
> an edit above them:
> `localParams.bFindInitialOverlaps = true;` ·
> `// overlap comes back with Time == 0, so it naturally wins.` ·
> `glm::mat4 worldMat = transform * vol.offsetTransform;`
> ⚠ Even task 65's replacement numbers for these three had drifted again within a day,
> which is the argument for anchors, made by the correction itself.


<!-- header lines 493-513 -->
```text
⭐⭐ THE GROUND PROBE'S LATERAL INSET — movement-sim TASK 54, 2026-09-07, AND IT IS A
DEFECT FIX, NOT A TUNING KNOB. The user reported, in PIE: *"driving a character into a
static wall, the character gets pushed into the ground and then pops out and over corrects
into the air, over and over again."* Client `_3` logged **755 `[Movement.surface]`
transitions** in one session and **every single `-> Airborne` line read `clearance=0.000`**.

⛔ THE CAUSE WAS THIS DESCRIPTOR, and it is upstream of step 6' and of the servo — the two
places the mechanism was looked for first, and three separate hypotheses about them were
MEASURED WRONG (impl/impl_notes_seam_54.md §1.4). The probe used to be a capsule the SAME
SIZE as the body at the body's OWN pose, so a body pressed against a wall made the PROBE
overlap that wall. `ChaosSpatialQueryAdapter::sweep` sets `bFindInitialOverlaps = true`
(:543-544) and then takes the MINIMUM-`Time` blocking hit (:564-571) — its own comment says
*"An initial overlap comes back with Time == 0, so it naturally wins"* — so the wall BEAT
the floor. Its normal is horizontal, step 2's `walkable` test failed, and the character was
classified `Airborne` **while standing on the floor**. Gravity then ran (the sink), and the
next Floor tick's clamped `maxSnapSpeed` correction was retained by the airborne branch into
a ballistic launch (the pop). Measured, on the shipped `integrate` against an analytic
floor+wall: 19 flips / 300 ticks and a `pos.z` excursion of 96 → 181 cm.

⭐ [movement-sim task 56] THE SECOND HALF OF THAT MECHANISM — the retention line — IS NOW GONE
TOO: ruling #28 replaced the two vertical laws with one, so there is no dead-beat correction
```


<!-- header lines 519-558 -->
```text

⭐ THE FIX IS UE'S OWN `UCharacterMovementComponent::ComputeFloorDist` PATTERN: inset the
floor probe from the body's silhouette so a body IN CONTACT with a wall does not overlap it.

THE ARITHMETIC, and it depends on a convention that had to be ESTABLISHED rather than
assumed — the other reading gives a different offset. `CapsuleGeometry::halfHeight` is the
**TOTAL** half-height (centre to tip, hemisphere included). It carries no comment of its
own, so both of its consumers were followed to an engine API Epic documents:
  * the PROBE path — `ChaosSpatialQueryAdapter::registerVolume` calls
    `FCollisionShape::MakeCapsule(radius, halfHeight)`; UE 5.6's `CollisionShape.h` says
    *"Note: This is the full half-height (needs to include the sphere radius)"*.
  * the BODY path — `ChaosPhysicsFactory` calls `UCapsuleComponent::InitCapsuleSize` and
    `checkf`s against `GetUnscaledCapsuleHalfHeight()`; UE 5.6's `CapsuleComponent.h`
    documents that field as *"Half-height, from center of capsule to the end of top or
    bottom hemisphere."*
So the body's bottom tip is at `centre.z - capsuleHalfHeight`, and for a probe of radius
`r - s` and total half-height `hh - s` whose bottom tip must land in the SAME place:
    probeCentre.z - (hh - s) == centre.z - hh   ⇒   probeCentre.z == centre.z - s
⇒ shrink by `s` AND drop by `s`. The probe's cylinder is untouched (its half-length is
  `(hh - s) - (r - s) == hh - r`); only the hemisphere shrinks.

⚠ WHAT `s` BUYS AND WHAT IT COSTS — all three are for PIE to confirm, NOT measured in-engine:
  + `s` cm of immunity: a body exactly touching a wall clears the probe by `s`.
  − ground detection narrows by `s`: the capsule may overhang a ledge edge by up to `s` cm
    before the probe stops finding floor.
  − on a SLOPE the probe reads `s * (secθ - 1)` cm MORE clearance than the body has, so the
    character settles that much lower: 0 flat, 0.155 cm at 30°, 0.414 cm at the 45°
    `maxSlopeAngleDeg` cap. Second-order against the `rideHeight * cosθ` perpendicular gap
    step 4 already documents (7.07 cm at 45°), and one-way.
    ⛔ NOT removable by a different offset: zeroing it needs `offset.z = -s / n.z`, which is
    slope-dependent, and a static descriptor cannot express it. It is the price of this fix.
  `BrawlerMovement.GroundProbeSlopeBiasIsTheShrinkTerm` pins that closed form, and
  `...MeasuresTheBodysClearanceOnFlatGround` pins the flat-ground reading as EXACT.

⛔ THIS IS THE PRAGMATIC FIX, NOT THE STRUCTURAL ONE, AND THE LEAD RULED IT KNOWING THAT
(2026-09-07). `s` only buys `s`: a body the solver leaves genuinely PENETRATING a wall by
more than `s` overlaps the probe again. The structural answer is for `sweep` to return the
nearest WALKABLE blocking hit (or more than one hit), which needs a `SpatialQueryAdapter` /
`SweepHit` change that FOUR sub-simulations share — so it gets its own scoped task rather
than riding a defect fix. Do not fold it in here; do not delete this paragraph either.
```

---

## 9. `PhysicsSetup` — the adopted capsule and the attachment probe


<!-- header lines 561-572 -->
> All physics setup descriptors for the movement simulation.
>
> THE CHARACTER CAPSULE ITSELF, adopted — not a body this sub-sim creates. `isRoot` sends
> the factory down the adopt path (task 8): it configures the ACharacter's existing capsule
> component and asserts the descriptor's dimensions agree with the authored ones, so
> `bindings.ownBodyId` IS the capsule body id.
> ⭐ [movement-sim task 76] THE CAPSULE'S BODY FLAGS, HOISTED OUT OF `body`'s initializer SO THE
> COMPILER CAN READ THEM. `PhysicalObjectDescriptor` holds a `std::vector<ShapeDescriptor>` and
> is therefore NOT a literal type — `PhysicsSetup::body.body.enableGravity` is
> `error C2131: expression did not evaluate to a constant`. `BodyDescriptor` alone IS one: four
> `bool`s and a scoped enum. The hoist costs one name and buys five compile-time reads; not one
> flag's value changed.


<!-- header lines 574-596 -->
> ⭐⭐ TRUE SINCE TASK 15, and PAIRED WITH `StaticData::drivesBody`.
> The component this descriptor adopts is the ACharacter's capsule, which is also
> the CMC's `UpdatedComponent`. UE 5.6's CMC early-returns on
> `UpdatedComponent->IsSimulatingPhysics()` in BOTH its synchronous and its async
> path, so setting this true does not merely ADD physics — it SWITCHES THE CMC OFF.
> ⭐ THAT IS NOW THE INTENT, not a side effect: task 15 retired the CMC from the
> gameplay path (`MOVE_None`, component tick disabled, `bAutoActivate = false`, and
> the `Move()` / `AddMovementInput` binding deleted) and flipped `drivesBody = true`
> in the same change, so the movement sub-simulation takes over as the capsule's one
> authority. The flags are applied here, by the factory's adopt-root pass, not in
> the character's constructor.
> ⛔ NEITHER MAY BE FLIPPED BACK ALONE. `simulatePhysics = false` here while
> `drivesBody` stays true gives TWO authorities on one component; `simulatePhysics`
> true with `drivesBody` false gives NONE — with `enableGravity` false the capsule
> would not even fall, just a shovable no-gravity body, and the player could not
> move at all. It would fire the moment a brawler registers; it is not latent.
> ⭐ THE KINEMATIC CAPTURE NOTE, kept for provenance: while this shipped `false`,
> `FConstGenericParticleHandle::GetP()` falling back to X/R for a non-dynamic
> particle (`ChaosPhysicsBodyAdapter.h`, tasks 46/47) is what made the passenger arm
> observable. The body is dynamic now, so P is the integrated pose and the capture
> reads it directly.
> ⚠ `p.AsyncCharacterMovement = 1` was DELETED from Config/DefaultEngine.ini by
> task 15 — the async CMC path it enabled has no gameplay role left to play.


<!-- header lines 603-603 -->
> ADOPT the ACharacter's own capsule instead of creating a body under it.


<!-- header lines 605-606 -->
> The upright capsule. This is also the flag that makes `LinearBodyState`'s
> dropped rotation sound rather than a lie (PhysicsBodyState.h's CHOICE RULE).


<!-- header lines 608-610 -->
> The engine RE-SOLVES this body from the pushed anchor state on a replay: it
> is the one body in the game whose separation we read back, so replaying its
> recorded motion would discard exactly the information step 6' consumes.


<!-- header lines 621-624 -->
> ⭐ FREE WITH THE HOIST, and it RETIRES NO must-never-move row — the four flags this pins were
> never on that list, and claiming otherwise would leave a gate green on a row that still needs
> its prose. `simulatePhysics`/`drivesBody` is the loudest un-enforced pair in this file;
> `drivesBody` is authored data and unreachable from here, so this pins the half that is not.


<!-- header lines 644-645 -->
> Ruling #5 (closed 2026-09-03) = BLOCK. Brawler-vs-brawler separation is
> the solver's, mass-ratio 50/50, read back as position only.


<!-- header lines 657-662 -->
> THE ATTACHMENT PROBE — one capsule volume, swept straight down
> `rideHeight + snapDistance` in step 2. It searches `world` ONLY: other brawlers must not
> be ground.
>
> ⭐ [movement-sim task 54] IT IS INSET `kProbeShrink` FROM THE BODY'S SILHOUETTE AND
> DROPPED BY THE SAME AMOUNT, so its bottom tip coincides with the body's and every

> ### R0-08 — FALSE — one more line number, same file
>
> ⛔ `ChaosSpatialQueryAdapter.cpp:503` is the `m_volumes` lookup. The line the paragraph
> means, `glm::mat4 worldMat = transform * vol.offsetTransform;`, is at **`:506`**.
> Re-verified 2026-09-11. Use the anchor, not the number.


<!-- header lines 669-671 -->
> Drop the volume by exactly the shrink: `probeBottom == bodyBottom` (see the
> derivation above). `offsetTransform` is applied by the adapter as
> `transform * offsetTransform` (ChaosSpatialQueryAdapter.cpp:503).


<!-- header lines 678-689 -->
> ⛔ [movement-sim task 76] THE SHRINK AND THE DROP ARE ONE TERM, AND THIS READS BOTH.
> `probeBottom == bodyBottom` holds only while the volume is dropped by exactly the
> amount it was inset; move either alone and the probe stops measuring the body's own
> clearance while every reading still looks plausible.
> ⚠ IT IS AN `OG_CHECK` AND NOT A `static_assert` ON PURPOSE. The `static_assert` form
> — hoisting the offset to an `inline constexpr glm::mat4` and pinning `[3].z` against
> `kProbeShrink` — compiles, and it FIRES when the drop moves alone. But the capsule
> shrink is `sd`-dependent and therefore invisible to it: change THAT half alone and it
> stays clean. That is an assertion equivalent to ONE INSTANCE of this prohibition,
> licensing the other half while looking enforced. Measured, both arms, at task 75.
> The comparison is written as `+ probeOffset[3].z` rather than `- (-...)` so it is
> EXACT in IEEE for any authored capsule: `a - s` and `a + (-s)` are the same float.

---

## 10. `RuntimeBindings`


<!-- header lines 710-716 -->
> The one shared definition lives in OGSimulation/PhysicsDeclaration.h. The
> PhysicsDeclaration concept requires `same_as<PhysicsRuntimeBindings&>`, so a
> field-identical per-sim copy is a DISTINCT type and does not conform; this
> alias keeps every existing `brawlerMovementSimulation::RuntimeBindings`
> spelling valid while making the type the shared one.
>
> For THIS declaration `parentBodyId == ownBodyId`: the adopted capsule is its own root.

---

## 11. `kWorldUp` — the servo's axis


<!-- header lines 721-731 -->
> ⭐ THE SERVO'S AXIS — THE ONE PLACE THE UP DIRECTION IS SPELLED IN THIS HEADER.
>
> ⛔ USER RULING #26 = (a), 2026-09-06: the hover servo MEASURES and ACTUATES on world-up.
> Everything that needs "up" reads this constant, or the `up` local step 2 seeds from it:
> the probe direction, the walkable test, the airborne frame fallback, `surfaceNormal`'s
> default below and step 5's support term. There is deliberately no second `(0, 0, 1)` in
> this file — `grep "0\.f, 0\.f, 1\.f"` returning only this line is a task-49 AC.
>
> WHY A NAME AND NOT A LITERAL: tasks 20 (wall run) and 48 (ski) will need the axis to
> follow `SupportState` — a world-up correction changes the NORMAL gap by only `cosθ`, which
> is exactly 0 on a wall — and that stays ONE BRANCH on step 2's local only while the axis


<!-- header lines 737-744 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row — it pins user ruling #26 (a), which
> the paragraph above states and nothing checked.
> ⚠ WRITTEN COMPONENT-WISE ON PURPOSE. Task 49's acceptance criterion greps this file for
> `0\.f, 0\.f, 1\.f` and requires exactly ONE hit — the declaration on the line above. The
> obvious whole-object form, comparing `kWorldUp` against a freshly spelled `glm::vec3` of the
> same three components, would be a SECOND hit and would break that AC while asserting the same
> thing. (It compiles: whole-object glm equality is constexpr in this tree. It is the AC, not
> the language, that rules it out.)

---

## 12. `DerivedState` — off-wire per-tick scratch


<!-- header lines 752-754 -->
> OFF-WIRE per-tick scratch: visualization and debugging only. Nothing in `integrate`
> READS these — they are written at the end of the step they describe, so a consumer that
> started depending on one would be depending on last tick's value without saying so.

---

## 13. `PlayerInput` — the input flags byte

> ### R0-09 — FALSE — "bits 1-7 RESERVED, and already spoken for", twice more
>
> ⛔ Sites 2 and 3 of the three in this header (site 1 is in §2, R0-03). Same correction:
> **bits 1-7 are UNASSIGNED**, not spoken for; task 48 adds no input field, and 20, 21 and
> 31 each say *"if"*. ⚠ Task 20 is **Wall run**, not "wall-grab". **Owner: task 58**, and
> `BrawlerInputPackaging-rationale.md` already carries the corrected wording that both ends
> should converge on rather than inventing a third version. Carried verbatim, flagged.
>
> ⚠ The last sentence of the fence that used to close this block — now guard **G-05** —
> calls step 1's gate *"the single place this input is consumed"*. Measured, the constant
> is READ at six sites outside this header. See guard **G-05**'s note in
> `BrawlerMovementSimulation-guards.md`; the claim survives only on the narrow
> production-code reading.


<!-- header lines 766-781 -->
```text
[movement-sim task 11] ONE BIT, and it is a MOVEMENT concern rather than a guard one:
holding guard freezes locomotion EXACTLY on the tick it is held (the genre's hitstop
rule), which is a property of this sub-sim's step 1, not of the guard shield.

⭐⭐ [movement-sim task 51] THE PACKING RULE, STATED AT THE TYPE SO NOBODY HAS TO GO FIND
A BACKLOG. PER-TICK INPUT IS THE SCARCE WIRE: an input byte is multiplied across every ring
entry in the packet-budget scenario, so it costs roughly TEN TIMES what a state byte costs.
Task 11's `holdGuard`, ONE `bool`, moved the ring entry stride 81 -> 82 B and ate 10.264 B
of the ordinary-join margin — a quarter of everything that was left. ~2.5 bytes remain.
⛔ SO A NEW PER-TICK INPUT SIGNAL IS A BIT IN THIS BYTE, NEVER A NEW MEMBER:
      bit 0    holdGuard (task 11).
      bits 1-7 RESERVED, and already spoken for — wall-grab (task 20), jump (21), dash
               (31), ski-tuck (48). As four `bool`s those blow the fence twice over; as
               four bits they cost nothing beyond what `holdGuard` already spent.
`bool` -> `uint8_t` is the same 1 B, which is why this re-layout moved zero wire bytes.

```


<!-- header lines 791-791 -->
> Bit 0 = holdGuard; bits 1-7 reserved. Read the packing rule above before adding one.


<!-- header lines 794-800 -->
> THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
> SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
> [movement-sim tasks 11, 51] ALL BITS CLEAR IS NEUTRAL: not guarding is not an action,
> and neither is any signal a reserved bit will come to carry. Unlike the radial/machine
> aim there is no (0,0,1)-style tag value here, so `zero()` and `PlayerInput{}`
> deliberately coincide — the property task 22's `ZeroInputIsTheFold` rests on, and one
> every future bit inherits for free precisely because each bit's neutral value is 0.


<!-- header lines 804-808 -->
> `PlayerInput::flags` bit assignments — the INPUT flags byte. Deliberately `kInputFlag`-
> prefixed to keep it distinct from `State::flags`, whose own bit constants (`kFlagFrozen`,
> `kFlagSupportMask`, `kFlagHasCommand`) are declared further down this file: the two bytes
> ride DIFFERENT WIRES — the relayed input ring and the correction state buffer — and a mask
> applied across them would be silently wrong.


<!-- header lines 811-811 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row.

---

## 14. `IntegrationUtils`


<!-- header lines 834-836 -->
> Current simulation tick — the Cadence model's commit boundary and the teleport
> stamp. Projectile's shape; plumbed from SimulationTimeStep at the
> SimulatableBrawler::integrate call site.

---

## 15. `InitialConditions` — the teleport seed


<!-- header lines 855-857 -->
> THE TELEPORT SEED — spawn and respawn. `teleportPending` is a COUNTER-FREE edge: the UE
> layer (task 13) sets it non-zero, step 0 consumes it and clears it back to zero in the
> same tick. It is on the wire because a respawn must replay identically.

---

## 16. `State` — the only state, field by field


<!-- header lines 867-882 -->
> THE ONLY STATE.
>
> `bodyState` is written by the generic post-solve capture pass
> (SimulationIntegrationExecutor::captureBodyStatesAll, via PhysicsDeclaration::bodyStateOf)
> and pushed back into Chaos's rewind timeline on a resim — neither of which this sub-sim
> writes a line of code for. Its presence in SerializableFields is what puts the body on the
> wire and in the checksum.
>
> [movement-sim task 5, 2026-09-02] `LinearBodyState`, NOT `PhysicsBodyState`: position +
> linearVelocity only, 24 B instead of 52 B. Both generic sites above keep compiling
> untouched — capture ASSIGNS a captured `PhysicsBodyState` in (the narrowing `operator=`),
> the rewind push CONVERTS one back out (the implicit widening `operator PhysicsBodyState()`),
> and neither site names a body-state type. See the CHOICE RULE at the top of
> OGSimulation/PhysicsBodyState.h. The rotation drop is sound because
> `PhysicsSetup::body`'s descriptor now sets `lockRotation` — the condition that header
> names, which did not exist when the swap landed.


<!-- header lines 886-898 -->
> ON WIRE 24. `position` is LIVE — it is last tick's post-solve capture, i.e. where the
> engine left the body after integrating our velocity and pushing it out of whatever it
> overlapped. That is what makes step 6's positional adoption FREE: there is nothing to
> copy, the capture already put the solved position here.
>
> ⚠ `bodyState.linearVelocity` IS DEAD WEIGHT — 12 B captured every tick, on the wire,
> in the checksum, and NEVER READ by a line of this sub-simulation. That is not an
> oversight; it is ruling #17(b), taken deliberately: option (a) — a NEW position-only
> 12 B body-state seam type — would have cost a mirror of task 4 across the capture
> bridge, the rewind push and every adapter, to save 12 B. The waste is the price of not
> introducing that seam type, and reclaiming it is a later wire-budget pass. (Its name is
> deliberately not spelled here: ruling #17 is the searchable anchor, and an acceptance
> criterion greps this file for the rejected type.)


<!-- header lines 903-905 -->
> ON WIRE 12. THE SIM'S velocity, and the only velocity any code here reads: assigned by
> the model, the hover servo, the gravity law and (later) dash / launch. No engine term
> has ever touched it.


<!-- header lines 908-910 -->
> ON WIRE 8 — the Cadence model's committed direction, in the surface frame. Zero under
> every other model; the layout does NOT change with `sd.model`, so switching models
> cannot move a wire byte.


<!-- header lines 913-913 -->
> ON WIRE 4 — the tick the current Cadence step began (also stamped by a teleport).


<!-- header lines 916-917 -->
> ON WIRE 1 — bit 0 frozen, bits 1-2 SupportState, bit 3 "a body command was issued last
> tick" (`kFlagHasCommand`, and step 6' below is its READER — see the note there).

> ### R0-10 — ⛔⛔ NEW — FALSE, and neither R0 audit inventoried it
>
> ⛔ The paragraph below ends *"and no file outside this header names the field at all"*, of
> `state.velocity`. Measured 2026-09-11 that is **false**:
> `BrawlerMovementVisualization.h:356` reads it (`position + state.velocity * …`), and
> `BrawlerMovementSimulationTest.cpp` names it at dozens of sites — `:378` READS it (the
> write on that line targets `bodyState.position`) and `:447` is a genuine **write**.
>
> ⭐ The argument the sentence supports — that nothing writes `state.velocity` between step
> 5 and the next step 6′ **inside the tick** — is sound, and that is the claim to keep. The
> tree-wide quantifier is what is wrong.
>
> ⚠ **This is the third instance of one defect shape in this single header** (the others:
> `kInputFlagHoldGuard`'s *"ONLY reader"*, §23; `kFlagHasCommand`'s *"its only reader"*,
> §23), and a fourth lives in the sibling file's history — task 66 found
> *"THE ONE AND ONLY WRITER … anywhere in the tree"* false in `BrawlerInputPackaging.h` and
> task 74 deleted it. **Found by classifying on the QUANTIFIER rather than on whether a path
> appears**, which is precisely what task 81's audit recommended a third audit should do.


<!-- header lines 920-937 -->
> ON WIRE 12 — the pose we HANDED the engine last tick, and the one input step 6'
> cannot re-derive: `bodyState.position` has since been overwritten by the capture, so
> the command is gone unless it is remembered. Wire size 61 B.
>
> ⭐⭐ [movement-sim task 50] THIS WAS OFF-WIRE SCRATCH, AND THAT WAS A LATENT DEFECT —
> ruling #27 (A). A correction destroys off-wire scratch on BOTH of its paths, which the
> off-wire block this replaces got wrong: `SimulationReconciliation::injectCorrectionState`
> DEFAULT-CONSTRUCTS the state before `readInto`, `StateCorrectionCache::
> tryInsertingCorrectState` stores that whole struct on a DISAGREEING landing, and
> `prepareResimAll` assigns it back WHOLE (`editState() = cache.getState(idx)`) — it does
> not restore "the serialized fields only". So after an adoption the scratch was ZEROED,
> step 6' skipped the first replayed tick, the into-contact velocity component was not
> killed, and the replay ended the tick `acceleration·dt` — 34.133 cm/s at the shipped
> tunables — above the authority, against `kDefaultSimilarityEpsilon` = 0.0001. The
> authority's NEXT correction then disagreed as well: ONE RESIM PER TICK for the duration
> of a wall press, each one a lossy Chaos rewind. 12 B buys the whole of that back.
> Pinned by `SimulatableBrawlerTest.cpp`'s `ReplayAfterAdoptionReproducesContactClamp`.
>


<!-- header lines 950-959 -->
> ⭐ [movement-sim task 76] A STRUCTURED-BINDING DECOMPOSITION IS AN EXPRESSIBLE STATEMENT ABOUT
> MEMBER COUNT: it compiles only if the aggregate has exactly that many public data members.
> ⚠ THE BINDING NAMES ARE FRESH NAMES, NOT THE MEMBERS' — they are spelled after the members so
> the order is readable here, but a RENAME is INVISIBLE to this assertion. The wire-order
> assertion at the bottom of this file is what catches that, as a hard error on the member
> pointer. Verified, both ways, at task 76.
> ⛔ `sizeof` AND `offsetof` ARE BOTH BLIND HERE, and that is the finding. `flags` is a `uint8_t`
> followed by a 4-aligned `glm::vec3`, so there are THREE PADDING BYTES after it: adding
> `bool commandIssued` in that slot leaves `sizeof(State) == 64` and every offset (0/24/36/44/
> 48/52) UNCHANGED. Measured at task 75, both candidates, on that exact edit.


<!-- header lines 974-978 -->
> `State::flags` bit assignments. Bits 1-2 hold a `SupportState`, so the enum may grow to 4
> values before this field has to widen.
> ⭐ [movement-sim task 56] THE BITS DID NOT MOVE. `SurfaceKind` used the same shift and the same
> mask; only the enumerator names and the third value changed, which is why this task re-quoted
> `kComposite == 321u` and `syncSize<State>() == 61u` UNCHANGED rather than re-measuring them.


<!-- header lines 984-984 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row.


<!-- header lines 991-994 -->
> Below this magnitude a push-out is engine noise, not contact: task 9's q3 measured a
> CONTACT-FREE hover (`pushOutTicks == 0`) with peak interpenetration 0.000005 cm, and q6
> measured 0.6 microns of ride-up between two blocking capsules. 0.05 cm is two orders of
> magnitude above both and is the threshold the spike itself used.

---

## 17. `PhysicsDeclaration` and `Dependencies`


<!-- header lines 1004-1010 -->
> Maps the GAME's aggregate static data to this sub-simulation's own slice.
> This is what makes body creation generic: the engine-side fold asks each
> declaration for its slice instead of branching on the declaration type.
> A member TEMPLATE deliberately — this header cannot name
> simulatableBrawler::StaticData, because the aggregate includes this header
> (an include cycle). GameStaticDataType is deduced at the call site, where the aggregate is
> complete.


<!-- header lines 1017-1017 -->
> Zero, and structurally so: an `isRoot` body is not attached under anything.


<!-- header lines 1024-1025 -->
> [movement-sim task 5] Return type follows State::bodyState. The generic
> consumers are written against `BodyStateLike`, not against a concrete type.


<!-- header lines 1038-1040 -->
> The FLINCH read (step 1) and, from tasks 27/31, the committed states. A SOFT edge:
> ⚠ [task 27, 2026-09-12] The task-27 half of that sentence has LANDED: step 1 reads
> `m_hitReaction` through this same dependency and `committed` is now
> `machineLaunchesMovement(machineState)`. Task 31’s `Dashing` is the half still outstanding.
> the machine sub-sim is declared FIRST in simulatableBrawler::ExecutionOrder and this
> one LAST, so `findFirstViolation` is satisfied without moving anything.

---

## 18. Pure helpers — `moveTowards` and `buildTangentFrame`


<!-- header lines 1048-1048 -->
> Pure helpers — no state, no adapter, no engine.


<!-- header lines 1051-1053 -->
> Move `current` toward `target` by at most `maxDelta`. The genre's law for every decaying
> quantity (launch decel, braking); exact at the endpoint, so a brake reaches EXACTLY zero
> rather than asymptotically approaching it.


<!-- header lines 1063-1070 -->
> THE SURFACE FRAME. Builds a right-handed orthonormal basis (u, v, n) with `n` as up.
>
> `u` is world +X projected into the plane and renormalised, so on FLAT GROUND the frame is
> exactly world XY (u = +X, v = +Y) — which is what makes "same stick, flat vs slope" a
> meaningful comparison rather than an arbitrary rotation. The +Y fallback is only reachable
> when `n` is within ~26° of world +X, i.e. a near-vertical wall (task 20).
>
> One `sqrt` per call and no transcendental. `n` MUST already be unit length.

---

## 19. The dual-basis decomposition — task 57 / ruling #29 ∴D-01

The third of the four named blocks (35 lines, 1082-1116). ⚠ **Nothing in it was a fence**, so
it carries **no guard id**: `⛔G-nn` marks a prohibition and this block states none.
⛔ **As task 68 shipped it, `decomposeVelocity` had no marker of any kind, and that was the
sharpest single case in this file's reader test** —
`a = (dot(planar, u) - dot(planar, up) * s) / (1 - s * s)` stood with nothing above it to say
the shape was deliberate rather than clumsy, and the reflex edit — *"simplify this back to three
dot products"* — leaves every flat-ground case in the suite green, by the `s == 0` identity
below.
⭐ **Task 82 closed that case.** This heading ends in `∴D-01`, and that tag is the line
immediately above the expression in `BrawlerMovementSimulation.h`. It is a DERIVATION tag, not a
fence: it prohibits nothing and says only *the reasoning for this expression is here*.
`tools/lint/guard_tag_lint.ps1` gates the join in both directions — delete the expression and
the tag goes with it, leaving this section an ORPHAN under CHECK 2; strip `∴D-01` from this
heading and CHECK 1 rejects the tag that still names it.


<!-- header lines 1082-1116 -->
```text
⭐⭐ THE DUAL-BASIS DECOMPOSITION — movement-sim TASK 57 / USER RULING #29, 2026-09-07.
The ONE place `state.velocity` is split into the two channels steps 3 and 4 consume.

⛔ `(u, v, up)` SPANS SPACE BUT IS NOT ORTHONORMAL. `u` is the slope tangent and `up` is world
up (ruling #26 a), so `dot(u, up) = sin(theta)` on a theta-degree face; only `v` is perpendicular
to both (`v = cross(n, u)` is horizontal). Splitting a vector with plain dot products therefore
DOUBLE-COUNTS, and both halves of that were live defects under task 56:
  * a FALL leaked into the tangential channel — `dot(V, u)` of a 2000 cm/s descent on 30° is
    1000 cm/s of "walk" the model then brakes. Positive feedback, because the tangential term
    has its own vertical component: MEASURED, the task-56 tree DIVERGES on a slope landing;
  * a WALK leaked into the vertical channel — walking a 30° slope at 600 cm/s genuinely rises
    at 300 cm/s, `dot(V, up)` reported exactly that, and the servo DAMPED it. The character
    settled 0.4197 cm off ride height (sign following the walk direction) for no reason but
    the arithmetic.

Solve `V = a*u + b*v + c*up` exactly instead. `v` is perpendicular to the other two, so `b` is a
plain dot and can be removed first; what is left lives in the (u, up) plane and is a 2x2 solve:
    b      = dot(V, v)
    planar = V - b*v
    s      = dot(u, up)                                    // sin(theta); 0 on flat ground
    a      = (dot(planar, u) - dot(planar, up)*s) / (1 - s*s)
    c      =  dot(planar, up) - a*s
Checks: a pure walk `V = 600*u` gives `(a, c) = (600, 0)` — the servo no longer sees the walk;
a pure fall `V = -2000*up` gives `(a, c) = (0, -2000)` — nothing leaks into the model.

⭐ AT `s == 0` IT REDUCES TO TODAY'S DOTS *BIT-EXACTLY*, and that is the correctness check
rather than a nicety: `x - y*0.f` is `x` and `x / 1.f` is `x` for every finite float, and on flat
ground `buildTangentFrame` returns exactly `u = (1,0,0)`, `v = (0,1,0)`. So every flat-ground
case in the suite is byte-identical across this change, and any that moved would have been
reporting a bug in this function.

⚠ DEGENERATE ONLY AT `s == 1`, a vertical face, which `Supported` cannot classify: step 2's
walkable test caps the normal at `cosMaxSlope`. The assert below is the statement of that
coupling — if a future `SupportState` arm lets the frame tilt to vertical, this is where it
surfaces, not in a silent division by zero.
```

---

## 20. `machineFreezesMovement`

> ### R0-11 — UNRESOLVABLE — and it misreads at the site
>
> ⚠ *"Behaviour is identical: the same two enumerators, in the same order."* compares
> today's spelled-out `machineFreezesMovement` to a **template task 62 deleted**, which
> cannot be read. ⛔ And it misreads against code that *can* be read: the enum declares
> `GuardFlinch` before `HitFlinch` — `DAttackMachineSimulation.h:53` and `:54`,
> re-verified 2026-09-11 — while this function tests `HitFlinch` first, so *"in the same
> order"* is the reverse of the declaration it sits beside.
> ⇒ **Recommend deleting the clause** when this text next has an owner; it
> defends a property against code nobody can see and is actively misleading against code
> they can. Carried verbatim here rather than silently dropped.


<!-- header lines 1142-1150 -->
> The genre's hitstop: a flinch freezes locomotion EXACTLY on the tick it is in effect. It
> does not decay the velocity, it zeroes the model's contribution for that tick.
>
> ⭐⭐ [task 27, 2026-09-12] **`HitFlinch` ALONE NO LONGER FREEZES.** The unconditional
> "`HitFlinch` ⇒ frozen" became "`HitFlinch` AND `m_hitReaction == Stun` ⇒ frozen", and a
> SIBLING predicate, `machineLaunchesMovement`, answers the other half: `HitFlinch` AND
> `Knockback` ⇒ **committed**, which step 3 tests FIRST. A stun is therefore exactly what a
> `HitFlinch` was before this task — the hitstop above, unchanged — and a knockback is the
> other branch of the same state. `HitReactionKind::Stun` is 0, so a default-constructed machine
> `State` in `HitFlinch` still freezes, which is what keeps every pre-task-27 rig honest.
> ⛔ The prohibition that rides both predicates is `G-22` in the guards doc: the reaction byte is
> meaningless outside `HitFlinch` and must never be tested without it.
>
> [movement-sim task 62] This was a template purely to keep `.m_currentState` and the
> enumerator DEPENDENT names while the machine State type was incomplete here. The machine
> header is included now, so both are spelled out and the compiler checks them.
> ⚠ `DAttackState` is at FILE SCOPE in `DAttackMachineSimulation.h`, outside
> `namespace dAttackMachineSimulation` — that is why the enum is unqualified while the State is
> not. Behaviour is identical: the same two enumerators, in the same order.

---

## 21. The movement models — step 3's dispatch targets


<!-- header lines 1158-1162 -->
> MOVEMENT MODELS — step 3's dispatch targets.
>
> Each is a pure function of the SURFACE FRAME: `stickUV` and the returned velocity are both
> 2D coordinates in (u, v). No model knows whether that plane is the floor, a slope or a
> wall, and no model may touch `state.bodyState`.


<!-- header lines 1165-1167 -->
> CMC-like. Accelerate toward the stick's target velocity at `acceleration`; with no stick,
> brake toward zero at `brakingDeceleration`. Stateless — it reads `currentUV` and writes no
> State slice at all.


<!-- header lines 1180-1185 -->
> "Steps, not a joystick". The direction is COMMITTED at a period boundary and held at a
> constant speed until the next one, so a mid-step stick flick cannot steer the character —
> which is the whole feel. A neutral stick at a boundary commits a STAND (zero direction);
> the timer keeps running either way, so the cadence stays on the beat.
>
> This model owns exactly two State fields, and touches nothing else.


<!-- header lines 1192-1193 -->
> Unsigned arithmetic is deliberate and safe: `stepStartTick` is only ever stamped from
> a tick that has already happened, so the difference never wraps.

---

## 22. The detach gate


<!-- header lines 1211-1213 -->
> ⛔ [movement-sim task 76] THE REQUIRES-EXPRESSION MUST BE DEPENDENT. A bare `requires { ... }`
> at namespace scope is not a SFINAE context in MSVC: the ill-formed id emits a hard error AND
> fires the assertion, which reads exactly like "not expressible" and is not.


<!-- header lines 1218-1223 -->
> ⭐ [movement-sim task 76] THE `Launched` HALF OF THE FENCE BELOW, AS A TRIPWIRE FOR THE PERSON
> EDITING `DAttackMachineSimulation.h` — the build breaks there, in the file that makes the
> statement false, and not here.
> ⚠ ONLY THAT HALF CONVERTS, so the fence below keeps its `Jumping` clause. Adding
> `kFlagJumping = 1u << 4` is INVISIBLE to any expression over the three flag constants: the new
> name is simply not in it. There is nothing to assert until the bit and its writer exist.


<!-- header lines 1234-1237 -->
> ⭐⭐ THE DETACH GATE — movement-sim TASK 56 / ruling #28. The ONE place a walkable probe hit
> can still be classified `Unsupported`, and the reason `SupportState`'s name is true rather
> than aspirational: support is a DECISION, and this is where it is taken.
>


<!-- header lines 1250-1260 -->
>
> ⚠ SO NO LLT CAN REACH THE `true` ARM, AND NONE PRETENDS TO. What IS verified is the WIRING —
> that step 2 consults this predicate at all — and it was verified by POISONING it to `return
> true` and watching `HoverHoldsRideHeight`, `HoverLiftsWhenLow` and
> `WallPressKeepsTheCharacterOnTheFloor` go RED together (the character falls); the run is
> recorded in the impl notes and the poison was reverted. `DetachGateIsVacuousUntilJumpLands`
> pins the vacuity itself, and says in the case what it does and does not prove.
>
> Parameters are the shape the filled version needs, so task 21 changes a body and not a
> signature: the state carrying the future `Jumping` bit, and the axis an upward `Launched`
> would be measured along.

⭐⭐ **THE TWO PARAGRAPHS ABOVE ARE NOW FALSE, AND THEY ARE KEPT BECAUSE THEY RECORD WHY —
task 27, 2026-09-12.** The gate is filled and BOTH arms are reachable:
`return committed && glm::dot(state.velocity, up) > 0.f;`.
* "No LLT can reach the `true` arm" — **superseded**. `BrawlerMovement.XYKnockbackDoesNotDetach`
  drives three arms: uncommitted at any vertical speed (false), committed and horizontal or
  settling (false), committed and rising (**true**), plus a discriminator that repeats the rising
  velocity UNCOMMITTED so the case is a statement about `committed` and not about the sign of
  `vz`. It replaced `DetachGateIsVacuousUntilJumpLands`, whose own header instructed exactly that:
  *"a vacuity pin that outlives its vacuity is a false comfort."*
* "Task 21 changes a body and not a signature" — **wrong about which task, and about the
  signature.** Task 27 changed both: the predicate gained a third parameter, `bool committed`,
  because the decision is no longer derivable from `State` alone. It is well-defined at the call
  site: `committed` is computed in step 1 and the surface normal `n` is keyed on `walkable`, NOT on
  `support`, so the detach decision sits strictly between them and feeds nothing the frame depends
  on.
* ⚠ **AND IT IS NOT A CLEAN WIN ON A SLOPE.** `dot(state.velocity, up)` is world z, and on a
  slope the TANGENTIAL channel contributes `knockbackSpeed · sin θ` to it — exactly the
  world-z-is-not-the-vertical-channel trap section 19 and task 57 both paid for. An up-slope
  knockback therefore reaches the true arm on the tick after the shove. The arm is the
  architect’s and is built as ruled; the measurement, and the recommendation to key it on the
  vertical CHANNEL instead, are in `impl/impl_notes_seam_27.md` and in guard `G-07`.

---

## 23. The movement tick — steps 6′, 0, 1, 2, 3, 4, 5

Step 4's comment block is the longest in the file (126 lines, 1563-1688) and the fourth
block the acceptance criterion names, alongside `THE SERVO IS ONE-SIDED` (34 lines,
1702-1735). ⭐ **Both are split:** three prohibitions left for the guards doc — **G-09**
(do not scale `rideHeight` with slope), **G-10** (the two forbidden servo variants) and
**G-11** (step 4 has no geometry of its own) — and everything else is below.


<!-- header lines 1270-1283 -->
> THE MOVEMENT TICK — architecture §3.3, revision 6.
>
> Steps run 6' -> 0 -> 1 -> 2 -> 3 -> 4 -> 5. Step 6' is numbered for the tick it belongs to
> (it closes LAST tick's loop) and runs FIRST here, which is the only order that works: the
> engine's answer to last tick's command is not available until the capture has landed.
>
> ⚠ `machineInput` IS THE MACHINE'S INPUT SLICE, not this sub-sim's: the move stick is packed
> onto `dAttackMachineSimulation::PlayerInput`, and the single call site in
> `SimulatableBrawler.h` is the one place it can be supplied. See the undeclared-input-edge
> note at the machine include, at the top of this file.
> [movement-sim task 62] It is SPELLED here now rather than deduced through a `MachineInputT`
> template parameter; that indirection existed only to survive the include cycle. The default
> template argument it carried was documentation — the parameter was always deduced from the
> argument at that one call site, so the default never participated.


<!-- header lines 1302-1307 -->
> ---- step 6': ADOPT THE ENGINE'S POSITIONAL PUSH-OUT --------------------------------
> The POSITION half of the adoption is free and structural: `bodyState.position` IS the
> post-solve capture, written by the generic capture pass before this function was
> called. There is nothing to copy. What is NOT free is separating the engine's
> contribution from our own command, which is the whole of the arithmetic below, and the
> one authored rule that follows from it.

> ### R0-12 — ⛔⛔ NEW — FALSE, and neither R0 audit inventoried it
>
> ⛔ *"`kFlagHasCommand` IS READ HERE, and this is its only reader."* Measured 2026-09-11,
> the constant is READ at **four** sites outside this header —
> `BrawlerMovementSimulationTest.cpp:343`, `:1512`, `:1532` and `SimulatableBrawlerTest.cpp:903`.
> ⭐ The true and useful form is the one the initiative already converged on for the
> symmetrical claim in the sibling file: **this is the one line in production code that
> turns the bit into behaviour.** Carried verbatim, flagged.


<!-- header lines 1310-1315 -->
> ⭐ [movement-sim task 50] `kFlagHasCommand` IS READ HERE, and this is its only
> reader. Until this task it was written by step 5 and consulted by nothing, while an
> off-wire `bool` twin carried the decision — the write-only-flag anti-pattern.
> Reading the ON-WIRE bit is what makes the gate survive a correction: the bit is
> restored with the rest of `flags`, so a replayed tick reaches this branch exactly
> when the authority's own tick did.


<!-- header lines 1318-1322 -->
> What `x += v·dt` alone would have produced from last tick's command. Anything
> else in the captured position is the solver: a wall, or another brawler.
> ⚠ `state.velocity` IS last tick's commanded velocity — see the
> commanded-velocity paragraph on `State`. BOTH operands are serialized now,
> which is the whole property this step needed and did not have.


<!-- header lines 1327-1349 -->
```text
THE AUTHORED CONTACT RULE, and the only way contact reaches velocity at all.
It has TWO arms since movement-sim task 57 / ruling #29, chosen by the
ORIENTATION of the contact, and the arms remove DIFFERENT components:

  LANDING (`dot(n, up) >= cosMaxSlope`) — a floor-like face absorbs the
    VERTICAL component and keeps the horizontal. That is the genre's rule
    (landing zeroes vertical, horizontal survives) and it is the fix for the
    user's *"if there is a slope where I landed my brawler goes flying"*: the
    into-contact kill below turns a straight-down 2000 cm/s fall onto a 30°
    face into 1000 cm/s DOWN THE SLOPE, because removing the normal component
    of a vertical vector leaves its whole tangential part behind. An inelastic
    bounce is right for a wall and wrong for the ground.

  WALL / OTHER BRAWLER (everything else) — UNCHANGED: kill the component
    driving into the obstacle, keep the rest. On a wall this is the
    fighting-game corner clamp; on another brawler it means the pushed
    character's into-contact velocity is zeroed for the tick and NO MOMENTUM
    IS TRANSFERRED. Task 54's wall press and `WallPushOutZeroesIntoWallComponent`
    measure this arm and did not move.

⚠ `kWorldUp`, NOT step 2's `up` local: step 6' runs BEFORE step 2, so the local
does not exist yet. Same constant, same ruling #26 (a) axis — and this is the
one place in the file that needs it early.
```


<!-- header lines 1353-1354 -->
> The captured `linearVelocity` — which is where the engine put its contact
> impulse — is not read here or anywhere.


<!-- header lines 1365-1370 -->
> ---- step 0: TELEPORT SEED ----------------------------------------------------------
> The ONLY body write that ignores `sd.drivesBody`: a spawn or respawn must move the
> capsule even on an instance that is not driving it — a `drivesBody = false` peer or LLT
> rig. (In shipped production `drivesBody` is TRUE and the CMC is retired; task 15 flipped
> both together. The earlier wording here said "while the CMC is still driving it", which
> has not been the case since. [movement-sim task 17])


<!-- header lines 1388-1403 -->
> ⭐⭐ THE HOVER TUNING READOUT — movement-sim task 56. The first PIE run after ruling #28
> is a TUNING session, and ω/ζ are provisional. This prints the configured pair, the
> derived gains, and — the one number nobody guesses from continuous intuition — the
> DISCRETE critical damping ratio for the configured ω, so a tuner can see at a glance
> which side of the ringing threshold they are on.
>
> ⚠ `[Warning]` IS DELIBERATE AND IS THE ONLY VISIBILITY KNOB THIS LINE HAS. `OGBLOG_G`
> does not enter `RouteOGMessage`; ogblog's own sink sends every message to
> `LogOGBrawler`, reading only the leading severity token, and
> `Config/DefaultEngine.ini` ships `LogOGBrawler=Warning`. A bare tag would be invisible
> in exactly the session this line exists for.
> ⚠ AND IT IS HERE, IN STEP 0, RATHER THAN IN `StaticData`'s CONSTRUCTOR, FOR AN
> ORDERING REASON: `ASimulationManagerUImpl::m_staticData` is a by-value member, so it is
> constructed with the actor — BEFORE `BeginPlay` installs the ogblog sink. A log from
> there would be swallowed. The teleport seed runs once per spawn and respawn, with the
> sink installed, which is exactly the cadence a tuning readout wants.

> ### R0-13 — FALSE ×2 — a dead tense and a tree-wide quantifier
>
> ⛔ *"Nothing writes the bit yet; task 14 … **will be** its only writer."* Task 14 is
> `Done`. `simulatableBrawler::makeSimPlayerInput` holds the only line in the tree that ORs
> `kInputFlagHoldGuard` into a flags accumulator — `BrawlerInputPackaging.h:59`, inside
> `makeSimPlayerInput` at `:48`.
>
> ⛔ *"this is its ONLY reader"* is false tree-wide: six reads outside this header, four in
> `MakeSimPlayerInputFlagsTest.cpp` and two in `SimulatableBrawlerTest.cpp`.
>
> ⚠ **Task 65's published replacement for this paragraph would ship a NEW false claim.** It
> prescribed quoting `BrawlerInputPackaging.h:153`'s *"THE ONE AND ONLY WRITER of
> `kInputFlagHoldGuard`, anywhere in the tree"* — a sentence task 66 found FALSE and task 74
> DELETED, at a line number that is now past the end of a file rewritten 197 → 128 lines.
> ⭐ **That is the single clearest demonstration in this phase that an R0 correction decays
> exactly like the claim it corrects.** The correction to adopt is: *the bit is READ here,
> the one production line that turns it into behaviour; it is WRITTEN by
> `makeSimPlayerInput`, which holds the only OR in the tree; test rigs assign it directly.*


<!-- header lines 1413-1418 -->
> ---- step 1: GATE -------------------------------------------------------------------
> ⭐ [movement-sim task 51] `kInputFlagHoldGuard` IS READ HERE, and this is its ONLY
> reader — the one line that turns the input byte's bit 0 into behaviour. Nothing writes
> the bit yet; task 14 (`buildPlayerInput` -> `makeSimPlayerInput`) will be its only
> writer. Keep the pair visible: a bit written and never read is exactly the
> `kFlagHasCommand` trap that took task 50 to repair.


<!-- header lines 1422-1426 -->
> COMMITTED STATES own velocity outright — the model is suspended, not blended. ⛔ The
> machine enumerators this reads (`Dashing`, task 31; `Launched`, task 27) DO NOT EXIST
> YET: `DAttackState` is {Attacking, Idle, GuardFlinch, HitFlinch} today. The external
> dependency that will carry them is already declared and already resolved above, so
> those tasks add their enumerator, their branch in step 3 and nothing else here.

⭐⭐ **TASK 27 LANDED, AND IT DID NOT ADD AN ENUMERATOR — 2026-09-12.** The paragraph above
predicted the shape and got it half right. `committed` is now
`machineLaunchesMovement(machineState)`, which is `HitFlinch` **plus** `m_hitReaction ==
Knockback`: the reaction is DATA on the hit, not a fifth machine state. That is a user ruling, and
the cost it avoids is concrete — a fifth `DAttackState` bumps `kDAttackStateCount`, moves the
visualizer initiative’s `kMachineStateCellCount` fence and four `case` sites **in that
initiative’s files**, and leaves `HitFlinch` with no writer at all, since after task 27 every
inbound hit would have entered the new state.
⛔ `DAttackState` is therefore still {Attacking, Idle, GuardFlinch, HitFlinch} and
`kDAttackStateCount` is still `4u`. **Task 31’s `Dashing` is the only enumerator this line is
still waiting for**, and when it arrives `committed` becomes
`machineLaunchesMovement(...) || machine == Dashing`, with its own branch inside step 3’s committed
arm beside the knockback’s assign-or-decay pair. ⛔ There is deliberately NO dead `Dashing` arm
standing there today: `committed` is exactly `machineLaunchesMovement` and the code says so, because
an unreachable branch carrying no comment (the rule forbids one) reads as a bug rather than as a
reservation.


<!-- header lines 1429-1436 -->
> ---- step 2: ATTACH + CLEARANCE -----------------------------------------------------
> Model-agnostic by construction: the model selector is not named in this step, in step 4
> or in step 5 — only in step 3's dispatch, which is what an acceptance criterion greps for.
>
> ⭐ THE SERVO AXIS, SPELLED ONCE PER TICK. `up` is what this step measures `clearance`
> along, what the walkable test dots against, and what step 5 actuates along — one name,
> one value, user ruling #26 (a). Tasks 20/48 turn this line into a `SupportState` branch;
> today every v1 surface is `Supported` and it is `kWorldUp` unconditionally.


<!-- header lines 1449-1452 -->
> `fraction` is only meaningful when `blocked` (SpatialQueryResult.h's field-validity
> rule), so an unblocked probe has no clearance at all rather than a large one.
> ⭐ `clearance` is a gap measured ALONG `up`, and step 4 closes it ALONG `up`. That the
> two are the same axis is the whole of ruling #26 (a).


<!-- header lines 1456-1460 -->
> ⭐⭐ CLASSIFICATION IS WHERE SUPPORT IS DECIDED — movement-sim task 56 / ruling #28.
> Geometry (`walkable`) is one input; the DETACH GATE is the other, and both are read here so
> that step 4 can be a pure reader. `SupportedSteep` has no producer in v1 — the walkable
> test is a single `cosMaxSlope` comparison, so a face past `maxSlopeAngleDeg` is
> `Unsupported`, not steep-supported. Task 48 adds the second comparison HERE, not in step 4.


<!-- header lines 1465-1467 -->
> `n` is the SURFACE normal and is now used for exactly one thing: building the tangent
> frame (u, v) that step 3's models live in (plus the viz readout below). It is NO LONGER
> the actuation axis — step 5 puts the support term on `up`.


<!-- header lines 1473-1476 -->
> The tag the `[Movement.surface]` line prints. ⚠ THE STRINGS ARE THE ENUMERATOR SPELLINGS,
> not prettier prose: the PIE runbook greps for them, and a readable synonym would make the
> grep and the code disagree. `Floor` / `Airborne` are RETIRED tokens — a runbook or a saved
> log filter still looking for them is looking for a build older than task 56.

> ### R0-14 — ⛔⛔ FALSE, KNOWN-FALSE SINCE 2026-09-09 — ROUTED TO TASK 63, NOT REWORDED
>
> ⛔ *"The stick arrives as a WORLD XY direction whose magnitude is the stick deflection
> (`BrawlerInputPackaging.h`: `moveDirectionWorld` is the move stick rotated into camera
> space, so rotation preserves its length)."* — **both halves are false.**
>
> | step | measured 2026-09-11 |
> |---|---|
> | the normalise | `OGBrawlerInputCollectionComponent.cpp:243` — `glm::normalize(inputDirection)` |
> | the rotation | `:236-245` is a **Z-rotation only** (`camForwardNormalized.z = 0.f`), so `z` stays 0 |
> | the deadzone | `:212-213` returns `(0,0,0)` below `g_moveStickDeadzone` = **0.15f** |
> | ⇒ consequence | `stickDeflection` is **{0, 1}**, never analog — a stick at 0.16 and a stick at 1.0 both request full walk speed |
> | ⇒ attribution | the rotation *does* preserve length. The PRODUCER normalises first, and the parenthesis blames the wrong file: `BrawlerInputPackaging.h` only **carries** the field |
>
> ⛔ **NOT reworded here, and that is deliberate.** `Backlog.md` task 63 is a **user FEEL
> decision** — analog walk speed against binary — marked *"Do not implement without a user
> ruling"*, and its two outcomes have different fixes. Rewording this paragraph to be true
> would silently ratify the binary answer the user has not given. **ROUTED to task 63.**
>
> ⚠ Task 63's own line pins into this header are stale and need refreshing before anyone
> acts on it.
>
> ⚠ The following paragraph — *"projected … with the DEFLECTION PRESERVED and the DIRECTION
> renormalised"* — is **TRUE of the code and vacuous in effect**: `stickDeflection` is 1, so
> the scale is a renormalisation to 1. Not false; **dead**. Task 63 owns that too.


<!-- header lines 1507-1513 -->
> ---- step 3: MODEL DISPATCH ---------------------------------------------------------
> The stick arrives as a WORLD XY direction whose magnitude is the stick deflection
> (BrawlerInputPackaging.h: `moveDirectionWorld` is the move stick rotated into camera
> space, so rotation preserves its length). It is projected into the surface frame with
> the DEFLECTION PRESERVED and the DIRECTION renormalised: on a slope the character
> walks along the slope at the speed the stick asked for, instead of losing speed to the
> cosine of the incline.


<!-- header lines 1525-1530 -->
> ⭐⭐ THE ONE READ OF `state.velocity` PER TICK — movement-sim task 57 / ruling #29.
> Both channels come out of the SAME dual-basis solve, and there is deliberately no second
> plain dot of the velocity against a frame axis anywhere below: `currentUV` here and `vUp` in
> step 4 are the two halves of one decomposition, and they were the two defects R2 named. Read
> `decomposeVelocity`'s comment before touching either. `state.velocity` is not written again
> until step 5, so one solve serves both sites.


<!-- header lines 1537-1538 -->
> Tasks 31 (Dashing: ASSIGN `dashDir · dashSpeed`, replacing momentum) and 27
> (Launched: `moveTowards(currentUV, 0, launchDecel·dt)`) land their branches here.
>
> ⭐ [task 27, 2026-09-12] **TASK 27’S BRANCH IS IN, AND IT IS TWO LINES, NOT ONE.** The decay
> the sentence above names is the `else`; the `if` is the **ASSIGNMENT** on the hit tick,
> `velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed`. Revision 6 / ruling #14(c):
> the sim OWNS the velocity and a hit REPLACES it — there is no impulse and nothing sums. The
> assignment is keyed on `inboundHit.wasHitThisTick`, **not** on the machine’s
> `m_timeInCurrentState == 0`: both consume the same derived signal on the same tick, and keying on
> the hit keeps this sub-simulation independent of how the machine represents its timer. Section 26
> derives the mapping. Task 31’s `Dashing` becomes a third arm of the same inner dispatch.


<!-- header lines 1543-1543 -->
> EXACT, this tick. Not a decay.

> ### R0-15 — UNRESOLVABLE-as-stated — and the real defect is in `Backlog.md`
>
> ⚠ Near the end of the block below: *"Backlog 49 still says \"#2 stays OPEN\"; that entry
> is stale."* Task 65 verdicted this FALSE on a literal grep for `stays OPEN`, which found
> only the retraction. ⛔ That grep missed a **second spelling of its own subject**:
> `Backlog.md` task 49 also contains, 71 lines later, *"**#2 stays fully open and this task
> must not touch it.**"* — lower case, in a surviving Description bullet.
>
> ⇒ **The header's *"still says"* clause is TRUE in substance** (the Backlog entry
> contradicts itself, and a reader sent there finds the assertion this file warns about)
> and wrong to call it a single sentence. ⛔ **The real defect is in `Backlog.md`, not in
> this header** — task 49 states #2 both closed and open. **ROUTED to the lead.**
>
> ⭐ The retraction itself re-derives correctly: 417.25 ÷ 3.3329 = 125.19 ✓.


<!-- header lines 1563-1675 -->
```text
---- step 4: VERTICAL ---------------------------------------------------------------
⭐ MEASURED AND ACTUATED ON THE SAME AXIS, `up`. That is the whole of user ruling #26.

⭐⭐ ONE LAW, NOT TWO — movement-sim TASK 56 / USER RULING #28, 2026-09-07. This step used
to be an `if` with two DIFFERENT KINDS of law in its arms, and the seam between them was a
defect that no amount of tuning either arm could reach:
    Floor:     velocityUp = clamp((rideHeight − clearance) / dt, ±maxSnapSpeed)   // ASSIGNED
                                                                                  // from position
                                                                                  // error; gravity OFF
    Airborne:  velocityUp = velocity·up + gravity·dt                              // INTEGRATED;
                                                                                  // gravity ON
Crossing the boundary handed a velocity PRODUCED BY A POSITION SERVO to a law that treats
its input as MOMENTUM. That single line is the mechanism behind the wall-press LAUNCH half
of task 54 (`impl_notes_seam_54.md` §1.3, the "airborne-retention line") and behind the
user's *"falls start at ~4 m/s instead of from rest"*. Task 54 removed the SPURIOUS
crossings; ruling #28 removes the second law, so there is nothing left to retain.
⛔ THE AIRBORNE-RETENTION RULING IS THEREFORE MOOT, NOT ANSWERED. Do not reopen it: the
line it was about does not exist, and the concept has no name left to attach to.

THE LAW. Gravity is unconditional; the servo is a term ADDED to it while supported:
    a = gravity                                              ← every tick, no branch
    if (support != Unsupported)
        a += clamp(k·(rideHeight − clearance) − c·vUp − gravity, ±hoverMaxAccel)
    velocityUp = max(vUp + a·dt, −terminalFallSpeed)

⭐ THE `− gravity` INSIDE THE CLAMPED TERM IS A FEED-FORWARD, AND IT IS WHAT KEEPS THE
STEADY STATE EXACT. At `clearance == rideHeight` with `vUp == 0` the servo evaluates to
exactly `−gravity`, so `a == 0` and the character neither rises nor sinks — the hover
height is a value, not an approximation. WITHOUT it the spring would have to SAG until
`k·sag == |gravity|` to hold itself up: 0.4631 cm at the shipped k, which every existing
`clearance == rideHeight` pin would have had to be loosened to accept. One subtraction.
⚠ It is INSIDE the clamp deliberately: the clamp then bounds the servo's own authority, and
the total vertical acceleration is `gravity + clamp(…, ±hoverMaxAccel)`. So the honest
per-tick velocity bound is `(hoverMaxAccel + |gravity|)·dt`, not `hoverMaxAccel·dt`, and
`NoVelocityStepAtSupportBoundary` pins that form rather than the tidier wrong one.

⭐ WHAT THE ONE LAW BUYS, AND IT IS A PROPERTY RATHER THAN A TUNING: `velocityUp` is now
CONTINUOUS across the support boundary by construction — nothing is ever assigned, so no
classification flip can introduce a step. Measured on a 10 m fall, worst per-tick
`|Δ velocity·up|`: 483.67 cm/s under this law against the 516.33 cm/s bound, and 972.00
under the dead-beat, which destroyed 972 cm/s of real momentum in one tick on landing.
⚠ AND WHAT IT DOES NOT BUY, RECORDED BECAUSE AN AC PREDICTED OTHERWISE: a walk-off does NOT
start "from rest" under either law, because in both the servo is TRACKING a receding floor
and the character genuinely has that velocity when the probe lets go. On a first-principles
ledge corner the detach speed is −101.61 cm/s under this law and −107.07 under the dead-beat.
What changed is provenance, not magnitude: the retained value is now something the body was
physically doing, not a one-tick position correction reinterpreted as momentum.
`LedgeFallStartsFromRest` says so in the case, and is GREEN in both arms on purpose.

⛔⛔ USER RULING #26 = (a), 2026-09-06 — CORRECT ALONG WORLD-UP. TASK 49.
Architecture §3.3 as revision 6 first wrote it measured `clearance` along the SWEEP
(world −Z) and applied the correction along `n`, the SURFACE normal: a control loop
whose measurement axis and actuation axis disagree. On a slope of angle θ that bought a
one-shot lateral slide while the servo converged, and an over-correction on every tick of
it. ⛔ Task 11 shipped §3.3 VERBATIM and was not at fault; task 49 changed §3.3's own
answer. Both axes are `up`, and ruling #28 did not touch either one.

⭐ MEASURED, NOT ARGUED (task 49 AC-1, 2026-09-06). The magnitude of that effect had been
derived wrong twice, so the PRE-FIX header was driven through
`SimulatableBrawler::integrate` against an analytic 30° plane, seeded 1 cm above ride
height, logging per-tick clearance change against the commanded step:
    clearance gained / clearance commanded   1.154700, 1.154716, 1.154879, 1.154875
                                             (1/cos30° = 1.154701)
    lateral X   0.433010 cm over the whole transient for e = 1 cm (= sinθ·cosθ)
  ⇒ THE PLANE DERIVATION IS THE RIGHT ONE. The probe is cast FROM THE CAPSULE, so a step
    along `n` also moved the probe origin sideways by `s·sinθ` and the ground under it fell
    away by `s·sinθ·tanθ`; the vertical gap gained `s/cosθ` and the servo OVER-corrected.
    ⛔ The superseded `e·tanθ` model — which predicted 0.866× here and a 0.577 cm lateral —
    is DEAD, and so is the 50.86 cm figure it produced.
  ⇒ At the shipped 45° cap the pre-fix figure was 1/cos45° = 1.414: the servo OVERSHOT and
    damped-OSCILLATED, dipping below ride height on alternate ticks, and would have
    DIVERGED past 60°. Under (a) `Δclearance` equals the commanded step at EVERY angle —
    the probe origin no longer moves horizontally — so the oscillation is gone and the 60°
    ceiling is LIFTED rather than tuned around. That is the argument for the ruling, and it
    is why task 49 was a fix and not a re-tune.
  ⚠ THAT MEASUREMENT WAS TAKEN ON THE DEAD-BEAT, whose loop gain was 1 by construction, so
    the RATIOS above are a property of the AXES and not of the law. Ruling #28 changed the
    law and left the axes alone: the servo term is still built from a scalar `clearance`
    error and still composed onto `up` in step 5, so the lateral component is still
    IDENTICALLY zero at every angle. What did move is CLOSURE — the error no longer goes to
    zero in one tick, by design — which is why `HoverOnRampDoesNotDrift` now pins the
    measured spring transient instead of a one-tick closure.
⛔⛔ THIS IS *NOT* TASK 9'S 29 cm, AND NOTHING HERE MAY CLAIM IT IS (task 49, 2026-09-06).
Backlog 49, `impl/design_hover_slope_transient.md` and the comment task 11 shipped all
attribute the spike's `capX.X −9.558624 → −38.920441` to this coupling. That attribution
is FALSIFIED, on the spike's own artefacts, by two independent facts:
  1. THE SPIKE'S SERVO ALREADY ACTUATED ON WORLD-UP. `impl/Spike9Probe.cpp.{final,pre44,
     pre9b}` — all three archived revisions — build the hover velocity as
     `FVector Vel = ZeroVector; ... Vel.Z = Clamp(Err / Dt, ±maxSnapSpeed)`. There is no
     surface frame in the probe: no `u`, no `v`, no `n * velocityN`. The mismatch this
     task removes DID NOT EXIST in the code that produced the 29 cm, so it cannot be its
     cause. This is a structural fact, not a derivation.
  2. `impl/research_spike_9.md` §0.4a ACCOUNTS FOR THE WHOLE 29 cm AS SPAWN
     DEPENETRATION: the capsule was seeded 89.5 cm INSIDE the ramp, `penDepth` falls in
     four equal 22.4395 cm decrements, and Σ push.X over steps 0–3 = −38.920508 against a
     measured −38.920441 — an accounting identity to 7e−5 cm that stops dead the tick the
     penetration reaches zero. Servo motion is COMMANDED, so it lands in `predicted` and
     can never appear in `pushOut`; a push-out that explains 100 % of the lateral travel
     leaves the servo nothing to have contributed.
  Also: the motion took THREE ticks (50 ms), not "the first second".
⇒ THE DEFECT BELOW IS REAL AND MEASURED, BUT ITS MAGNITUDE IN A SHIPPING SCENARIO IS
  UNMEASURED — no run has ever exercised this servo on a slope. ⛔ The vertical errors
  inferred from the 29 cm (67.81 cm here, 50.86 under the dead model) are artefacts of an
  attribution this file does not accept; do not repeat either figure.
⛔ Task 9 observation #2 (127-step convergence) is not this either, and it is no longer
open: `research_spike_9.md` §0.1(1)/§0.2 WITHDREW it — `277 − 150` is distance-to-wall
divided by scripted speed (417.25 cm ÷ 3.3329 cm/step = 125.2), which is why all three
peers agree on it. Backlog 49 still says "#2 stays OPEN"; that entry is stale.

⚠ WHAT (a) COSTS, RECORDED SO TASKS 20/48 INHERIT IT. `rideHeight` and the probe reach
are both measured along `up`, so the PERPENDICULAR gap is `rideHeight·cosθ` — 7.07 cm at
the 45° cap, 2.59 cm at 75°. ⭐ That is NOT a regression from this task: the measurement
axis was already vertical, so the steady-state perpendicular gap is identical before and
```


<!-- header lines 1680-1688 -->
> The vertical channel of step 3's dual-basis solve. ⛔ NOT a plain dot of the velocity
> against `up`, which is what this line was until task 57: on a slope that reads the WALK's own
> vertical component as a fall and damps it (R2's mirror — walking a 30° face at 600 cm/s
> asked for 17 112 cm/s² of lift). See `decomposeVelocity`.
> ⚠ THE ACCEPTANCE GREP FOR THIS TASK is "no plain dot of `state.velocity` against `u`, `v`
> or `up` remains in this header". It returns ZERO. The two `glm::dot(state.velocity, ...)`
> calls that DO remain are both in step 6', are both authored single-component removals
> against a unit direction (`kWorldUp` and the contact normal), and are not a basis
> decomposition — the note at the branch says so at the site.


<!-- header lines 1691-1697 -->
> AUTHORED gravity (ruling #16 a) — per character, so fall speed is a design knob rather
> than a world constant. `sd.gravity` is a SIGNED SCALAR (−980), i.e. §3.3's
> `dot(g_vec, up)` with `g_vec = (0, 0, gravity)` already collapsed; widening it to a vector
> would be a `StaticData` change, which is an addition and not a hoist.
> ⭐ UNCONDITIONAL. There is deliberately no branch here that could turn it off, which is what
> `GravityRunsWhileSupported` measures by starving `hoverMaxAccel` below `|gravity|` and
> watching a SUPPORTED character sink at exactly `(gravity + hoverMaxAccel)·dt`.

> ### R0-16 — a circular citation, re-derived so it stops depending on itself
>
> ⚠ The `+/-380 cm/s per tick` in the forbidden-variants fence — now guard **G-10** — is
> quoted from `design_ledge_fall_and_landing.md` §1b, which states the same figure and does
> not derive it either. Re-derived here: it is `c·dt` times the chase speed,
> `2ζω · dt · 400` = `57.04 × (1/60) × 400` = **380.27**. ⭐ Name the TERM and the number
> survives a gain change instead of silently lying about one.


<!-- header lines 1702-1717 -->
```text
⭐⭐ THE SERVO IS ONE-SIDED — movement-sim TASK 57 / USER RULING #29, 2026-09-07.

    SUPPORTED MEANS HELD UP, NEVER PULLED DOWN.

At or below ride height (`e >= 0`) this is task 56's full spring-damper, unchanged and
still poison-tested. ABOVE ride height it is a BOUNDED SPRING PULL AND NOTHING ELSE,
capped at `sd.hoverPullDownAccel` — 0 in the shipped data, so the vertical law up there
is gravity and nothing else, which is what the user asked for.

⛔⛔ WHY THE OLD TWO-SIDED ARM WAS THE LEDGE DEFECT (R1). Its pull was bounded by
ACCELERATION (`hoverMaxAccel` = 30 g), not by speed, so a clearance that JUMPS into the
band cost -516.333 cm/s in ONE tick against gravity's -16.333: 31x too fast, MEASURED on
the task-56 tree at clearance 45 (`AboveRideHeightIsGravityOnly`). The dead-beat this
spring replaced capped the CHASE SPEED at 400 cm/s and so never had this failure mode;
that difference is what made "the spring is about as fast as the dead-beat" wrong.

```


<!-- header lines 1726-1728 -->
>
> ⚠ `hoverMaxAccel` still bounds BOTH arms and stays 30 000: catching a landing needs it.
> The pull-down knob bounds only the spring term on the upper arm.


<!-- header lines 1732-1735 -->
>
> `k` and `c` are `ω²` and `2ζω`, derived once in `StaticData`'s constructor; read the ζ
> comment there before changing either — ζ = 1 is NOT critical damping for this discrete
> recurrence and rings visibly.


<!-- header lines 1743-1745 -->
> ⚠ THE TERMINAL CLAMP IS ON THE WHOLE LAW NOW, not on an airborne branch. It never binds
> while supported at the shipped gains — the servo turns a fall around long before 2000 cm/s —
> and leaving it unconditional is what keeps this step free of a second `if` on `support`.


<!-- header lines 1748-1753 -->
> ---- step 5: WRITE ------------------------------------------------------------------
> Tangential motion stays in the SURFACE frame (u, v); support and gravity go on `up`, the
> axis step 2 measured `clearance` along. ⭐ On a uniform slope the tangential term moves the
> body ALONG the plane, so the vertical gap does not change, the servo sees ZERO error while
> walking and therefore never contributes a horizontal component of its own. That property
> is exactly what ruling #26 (a) buys.


<!-- header lines 1756-1764 -->
> Remembered for NEXT tick's step 6'. ⚠ BOTH of these are SERIALIZED — `positionCmd`
> since task 50, `flags` since task 11 — and that is the point: step 6' reads exactly
> what a correction restores, so a replayed tick reproduces the live one.
>
> ⭐ [movement-sim task 50] THERE IS ONE COMMAND MARKER NOW, and it is the wire bit.
> Task 11's off-wire `bool` twin is DELETED, and with it the documented asymmetry in
> which the two markers deliberately diverged for one replayed tick. `firstResimStep` no
> longer touches this sub-simulation's state at all (architecture §3.5 always described it
> as a no-op; it is one again), so there is no tick on which the bit and the behaviour

---

## 24. `SerializableFields` and the wire field order


<!-- header lines 1782-1782 -->
> SerializableFields specializations for brawlerMovementSimulation types.


<!-- header lines 1784-1784 -->
> THE TELEPORT SEED, on the wire: a respawn has to replay identically on both peers.


<!-- header lines 1799-1807 -->
```text

⭐ [movement-sim task 50] `positionCmd` APPENDED, +12 B (slice 49 → 61 B, composite
309 → 321 B). It is STATE, not scratch — the one input step 6' cannot re-derive — and the
reason is written out at its declaration. Appending leaves every preceding field at its
existing offset, so `correctionStateBuffer::kWireFormatVersion` is NOT bumped: the size
moved, the layout of what came before did not. (Same call task 11 made when it grew this
slice 24 → 49 B.) ⛔ Task 11's second, off-wire velocity copy and its off-wire `bool`
command marker are GONE, not merely still absent: the first held exactly `velocity`, which
is already the second entry below, and the second was a twin of `flags`' bit 3.
```


<!-- header lines 1835-1852 -->
> ⭐⭐ [movement-sim task 76] THE WIRE FIELD ORDER, PINNED BY NAME — the successor to the
> APPEND-ONLY sentences above, which until now were guarded by PROSE ALONE.
>
> ⛔ WHY NOT A SIZE OR A COUNT. `velocity` and `positionCmd` are BOTH 12 B. Transposing them
> leaves `syncSize<State>() == 61`, `FCompositeWireSize<simulatableBrawler::State> == 321`,
> `kStateWireBytes == 332`, the 82 B ring entry stride and every other pin in the tree GREEN —
> and it desynchronises every peer that has not shipped the same edit. Measured: under that one
> edit all eleven existing wire pins compile clean, a same-peer serialize/deserialize round trip
> still agrees with itself, and the same 61 bytes decode with the two vectors swapped.
> ⛔ WHY NOT `offsetof`. An offset chain measures the STRUCT, and the struct's own member order
> is wire-NEUTRAL — the tuple below decides the byte order. So it would both miss the edit that
> matters and fire on one that does not.
> ⭐ WHY THIS WORKS. `SIM_MEMBER(C, m)` expands to `MemberFieldDesc<&C::m>{}`, so each entry's
> member pointer is a NON-TYPE TEMPLATE PARAMETER and the whole sequence is carried in the
> tuple's TYPE. Comparing that type is equivalent to the prohibition: a reorder, an insertion
> and a removal all MOVE it, and a rename does not compile at all — the member pointer below
> stops naming a member. APPENDING a field is the one edit that still needs a human: it moves
> this assertion too, and that is the deliberate, versioned act the comment demands.

---

## 25. The trailing labels — the loss you feel on every line

⛔ **v2 §1.4 item 5, user ruling 2026-09-11: a trailing one-line label GOES, and the loss is
ACCEPTED rather than overlooked.** Its content belongs here, and the reader follows the pointer.

⚠ **This header had nineteen of them.** The pilot that established the rule had five, 15 to 58
characters, and called them *"the single most-felt loss in the converted file"*. Nineteen is the
largest set in the phase so far, and it includes the whole `ON WIRE n` column of `State` and
`InitialConditions` — the only place the wire size of each field was written down beside the
field. They cannot go stale in any interesting way, and they still go.

| declaration, after conversion | the label it carried |
|---|---|
| `#include "OGSimulation/OGTypes.h"` | `// for BodyId` |
| `#include "glm/common.hpp"` | `// glm::abs / glm::clamp / glm::max` |
| `#include "glm/geometric.hpp"` | `// glm::dot / glm::cross / glm::normalize / glm::length` |
| `#include "glm/trigonometric.hpp"` | `// glm::cos / glm::radians -- SETUP time only, never per tick` |
| `#include "glm/exponential.hpp"` | `// glm::sqrt` |
| `#include "OGSimulation/SpatialQueryResult.h"` | `// SweepHit -- the attachment probe's result` |
| `#include "OGSimulation/OGAssert.h"` | `// OG_CHECK -- the model-dispatch default arm` |
| `float terminalFallSpeed;` | `// POSITIVE magnitude; the fall speed is clamped to −this` |
| `{` | `// shapes` |
| `glm::vec3 surfaceNormal{kWorldUp};` | `` // the frame's `n` this tick — fed from the one axis `` |
| `glm::vec3 lastProbePoint{0.f};` | `// where the attachment sweep hit, world` |
| `glm::vec3 lastPushOut{0.f};` | `// the engine's positional correction, step 6'` |
| `uint8_t   lastSupportState = 0;` | `// SupportState, as a byte for the viz layer` |
| `inline constexpr uint8_t kInputFlagHoldGuard = 1u << 0;` | `// bit 0` |
| `uint32_t  teleportPending = 0;` | `// ON WIRE 4` |
| `glm::vec3 teleportPos{0.f};` | `// ON WIRE 12 — wire size 16 B` |
| `inline constexpr uint8_t kFlagSupportMask  = 0x06u;` | `// bits 1-2` |
| `glm::vec2 uv;` | `` // tangential, in the surface frame (u, v) — step 3's `currentUV` `` |
| `float     up;` | `` // vertical, along the servo's axis     — step 4's `vUp` `` |

⛔ **R0-17 — the first row was FALSE, and it is repaired by deletion.** `// for BodyId` on the
`OGTypes.h` include is wrong: that header does not define `BodyId`, and `BrawlerCharacterBindings-guards.md`
closes the stronger claim with a pickaxe over the whole file history —
`git log -S BodyId -- OGSimulation/OGTypes.h` returns **zero**, so it has **never** been true.
`BodyId` is declared in `OGSimulation/BodyId.h`. ⭐ The label is recorded above for the record
and is **not** reinstated; this is the one R0 finding in this file that the v2 conversion repairs
simply by removing the sentence. ⚠ `BrawlerCharacterBindings-guards.md:79` cites this exact line
as `BrawlerMovementSimulation.h:4` — **routed, not fixed**.

⭐ **The wire sizes those labels carried are not lost as FACTS** — `syncSize<State>() == 61u`
and `syncSize<InitialConditions>() == 16u` are pinned in `SimulatableBrawlerTest.cpp`, and task
76's three `SerializableFields` assertions pin the ORDER by name. What is lost is the reading of
them *at the field*, which is where a person adding a field looks.

---

## 26. The knockback assignment’s tangent-frame mapping — task 27 ∴D-06

`velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed;`

**Where the formula comes from, and why the obvious alternative is wrong.**

`hitDirectionXY` is a **world XY unit vector**, resolved once by `brawlerHitRouting::System` as
`normalise(XY(target position − attacker position))`. `velocityUV` is a pair of **tangent-frame
channels**, `(a, b)` such that step 5 composes `a·u + b·v + c·up`. Writing `d.x` into `a` and
`d.y` into `b` therefore looks like a type error, and it is not — it is exact, and it is exact
because of what `buildTangentFrame` produces.

For any normal `n` this step can reach, `|n.x| < 0.9`: a walkable hit satisfies
`dot(n, up) >= cosMaxSlope` = cos 45°, so `|n.x| <= 0.707`, and a non-walkable tick uses
`n = kWorldUp`, where `n.x = 0`. So the reference vector is always `(1, 0, 0)` and

```
u = normalise( (1,0,0) − n·dot((1,0,0), n) )      ⇒  u = (cos θ, 0, sin θ)  for n = (−sin θ, 0, cos θ)
v = cross(n, u)                                    ⇒  v = (0, 1, 0)
```

**`u`’s horizontal projection is X-aligned by construction and `v` is horizontal.** The map from
world XY to `(a, b)` is therefore the identity on components, and because `u` and `v` are
orthonormal the MAGNITUDE is preserved exactly: a 2000 cm/s shove is 2000 cm/s along the face.

⛔ **THE REFLEX EDIT THIS TAG EXISTS TO STOP** is the projection,
`glm::vec2(dot(dWorld, u), dot(dWorld, v))`. It is the spelling every other world→frame
conversion in this file uses, it is dimensionally innocent, and it is WRONG here: `dot(dWorld, u)`
is `d.x·cos θ`, so on a 30° face the character launches at 1732 cm/s instead of 2000 and
covers 3.75 m instead of the user’s 5. ⭐ **It is also invisible on flat ground**, where
`θ = 0` and the two spellings agree exactly — which is why the case that discriminates them is
`BrawlerMovement.SlopeKnockbackStaysOnSlope` and not any of the seven flat-ground knockback cases.

**What it buys, stated as the user’s requirement:** a shove on a slope stays on the slope. The
launched velocity is perpendicular to `n`, its horizontal speed is `knockbackSpeed · cos θ`, and
the character covers five metres **along the face** rather than five metres of ground.

⚠ **THE STICK IS PROJECTED, AND THAT IS NOT AN INCONSISTENCY.** Step 3’s stick handling a few
lines above genuinely does `glm::vec2(dot(stickWorld, u), dot(stickWorld, v))` and then rescales to
the stick deflection. The two are different questions: the stick asks *which way along the surface
is the player pointing*, which is a projection followed by a renormalisation; the hit asks *which
way was the character shoved*, which is already a direction in the surface’s own horizontal
plane. Reading one as a precedent for the other is the mistake.

**Decay, the other half of the arm:** `moveTowards(currentUV, glm::vec2(0.f), sd.launchDecel * dt)`
— the Smash law, constant deceleration to rest, and `moveTowards` returns the TARGET once the
remaining distance is inside one step, so the slide lands on exactly 0.000000 instead of
overshooting into a backwards crawl. `v² / (2a)` = 500 cm at the shipped pair. ⭐ The step
count is a FLOAT fact, not an arithmetic one: `4000·(1/60)` is 66.666664, so thirty steps leave
8e-5 cm/s behind and the thirty-FIRST takes it to zero.
`BrawlerMovement.LaunchedDecelsAtLaunchDecel` pins that, and asserts the closed form against the
shipped constants rather than restating 500.

---

## §X — Coverage

This document carries **every** comment line that left `BrawlerMovementSimulation.h`, except:

| excluded | lines | where it went |
|---|---|---|
| the SPDX licence line | 1 | stayed in the header |
| horizontal rules (`////…`) | 23 | deleted — a rule carries no claim |
| the 11 live fences | 57 | `BrawlerMovementSimulation-guards.md`, G-01…G-11 |
| the 10 retired fences | 32 | `BrawlerMovementSimulation-guards.md`, §R, G-12…G-21 |
| **carried here** | **1011** | this document |
| | **1124** | = every comment line in the pre-conversion header |

⛔ **That table is derived from the same script that wrote this file and therefore proves
nothing on its own.** The check that does prove it is a separate reconciliation that reads only
the shipped header and these two shipped documents, and asserts in both directions that no
comment line was lost and that every fence quoted in the guards doc is byte-identical to the
bytes it left. It has four poison arms and all four fire. ⚠ That instrument is initiative
working material and is not distributed with this submodule; what ships here is the result.

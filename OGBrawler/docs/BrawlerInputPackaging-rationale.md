<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerInputPackaging.h` — rationale

This is the narrative, the derivations and the provenance for the engine-free input packing:
`ContinuousInputFields`, `readContinuousInputFields`, `InputFlagFields`, `makeSimPlayerInput` and
`makeVisualizationPlayerInput`. Under `CommentExtractionRule_v2.md` the header keeps **no prose at all**: a licence line, a
two-line pointer, code, and one `⛔G-nn` tag per guard. Every fence is in
`BrawlerInputPackaging-guards.md`, each with a stable id, and **six of the sixteen are no longer
text anywhere**: five became `static_assert`s in the header, and one turned out to be
enforced by the language already. This file carries everything else:
the narrative, the derivations, the provenance and the orientation.

**If this file and `BrawlerInputPackaging.h` disagree, the header is authoritative and this file is
stale.** Fix this file; do not soften the header to match it.

⛔ **Do not put a fence in this file.** A prohibition belongs in `BrawlerInputPackaging-guards.md`,
with an id and a `⛔G-nn` tag on the declaration where the wrong line would be typed — or, better,
in a `static_assert` so that it cannot be skimmed past at all. `tools/lint/guard_tag_lint.ps1` is a
hard gate on that join in both directions; a fence filed here has no site and no lint row.

⛔ **This file is not the source of truth for any VALUE.** The neutral aims live in each sub-
simulation's own `zero()`; the flag constants live in `BrawlerMovementSimulation.h`. Where a value
appears below it is there to make an argument readable.

**Read alongside:** `BrawlerInputPackaging-guards.md` — the prohibitions, by id.
`VISUALIZATION_DISCIPLINE.md` — the mesh-only invariant that §6's render-echo
rule serves. `SimmableUpdateComponent-rationale.md` — the UE-side caller that consumes both packers.

Origin: the render-side input echo work of `og-netcode-v2-arch-latency` (T12, the packaging half of
D5.4, and T13, the caller half), the design notes `proposal_ogbrawler_netcode.md` §2.3 and
`risks_and_plan.md` D5.4 / `R-UE1`, and `brawler-movement-simulation` tasks T1, 14, 17, 22, 51, 52,
66 and 74.

> ⚠ **Every document named in that Origin line is private working material from initiative
> archives and is NOT distributed with this submodule.** They are named as provenance, deliberately
> unlinked; every claim this file *asserts* is anchored to a file in this repository and only to
> those. The declarations below tell `tools/lint/doc_anchor_lint.ps1` that these names are
> intentionally unresolvable.

<!-- lint-external-ref: proposal_ogbrawler_netcode.md -- og-netcode-v2 initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: risks_and_plan.md -- og-netcode-v2 initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: CommentExtractionRule_v2.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->

---

## 1. The seam: where the live read stops and the pure assembly starts

The rule the two bullets below state — the header's orientation block carried it until
task 74 removed it — was settled as **T12 of
`og-netcode-v2-arch-latency`, the packaging half of D5.4** — *"render-side input echo:
`buildLatestVisualizationInput()` render-safe live sampler"*. Its caller half is T13.

**UE side.** `UOGBrawlerInputCollectionComponent` owns everything that has to touch the engine or
the frame: the Enhanced Input subsystem read, the camera-forward and mouse-aim caches
(`m_camForwardCache`, `m_mouseAimCache`) and the mouse-aim line-plane intersection. It exposes
three accessors — `buildAimDirection`, `getMoveStick` and `buildMoveDirectionWorld` — and nothing
else of that machinery escapes into this header.

**Two packers, one continuous read.** `makeSimPlayerInput` is the per-tick path; its
discrete and edge-derived fields — the attack buttons, the holdGuard button, and
`triggeredActionId` from the tick-stateful motion matcher — are passed in explicitly by the
caller, because sampling any of them needs edge state, a tick or the matcher, none of which may
enter this header (guard **G-08**). `makeVisualizationPlayerInput` is the render-rate path; §6
has it.

**Core side.** This header takes those three values and nothing more. It includes `<cstdint>`, two
`glm` vector headers, `SimulatableBrawlerTypes.h` and `InputSequence.h`; no UE type appears in it,
and it compiles in `og-brawler-tests`, a tree that cannot link UE.

**The shape `Src` must satisfy.** The header stated it as a three-line table above
`readContinuousInputFields`; under v2 it is here, and this is the only copy. These are the
bytes it occupied:

```cpp
// Reads the continuous fields from any source exposing the input-collection
// accessor shape:
//     glm::vec3 buildAimDirection() const;
//     glm::vec2 getMoveStick() const;
//     glm::vec3 buildMoveDirectionWorld() const;
```

Nothing else is required of a source, which is what lets `MockInputSource.h` stand in for
`UOGBrawlerInputCollectionComponent` in a tree that cannot link UE.

**Why that split is a template and not an interface** — the fact the header carries as guard
**G-04**:
`readContinuousInputFields` is templated on its source, so `UOGBrawlerInputCollectionComponent`
satisfies it *structurally*, without `UObject` being dragged into the core and without UE being
dragged into the test tree. The test double that stands in for it is `MockInputSource.h` in the
`og-brawler-tests` submodule.

**And it is the SINGLE source of truth for the continuous read.** Both UE builders —
`buildPlayerInput` on the per-tick path and `buildLatestVisualizationInput` on the render path —
call it, in `OGBrawlerInputCollectionComponent.cpp`. That is what makes drift between the two paths
structurally impossible rather than a thing to remember. It is pinned by a whole test file,
whose three cases assert the two packers agree on every continuous field, that discrete
fields are allowed to differ, and that the agreement survives a sim call carrying no
discrete input:

`Source/OGBrawlerTests/extern/og-brawler-tests/Source/OGBrawlerTests/InputPackaging/BuildLatestVisualizationInputContinuousFieldsMatchBuildPlayerInputTest.cpp` <!-- lint-anchor-ignore: the file EXISTS and this path is exact. doc_anchor_lint.ps1:250 applies its `build*` exclusion to EVERY path part, which is meant for build-output DIRECTORIES but also matches this FILENAME, so the file is absent from the index. Probe: impl/task66/probe_lint_index_output.txt -->

---

## 2. `ContinuousInputFields`' defaults, and the one slice they are not neutral for

**What the defaults are.** `aimDirection` is initialised from `dAttackRadialSimulation`'s own
`PlayerInput::zero().aimDirection` — `(0,0,1)`, which is also the machine and guard sub-simulations'
neutral aim, each spelled once in its own `zero()`. `moveStick` and `moveDirectionWorld` default to
the zero vectors those types carry.

⭐ **The neutral aim is spelled ONCE per sub-simulation, and this header spells none of its own.**
It is visible here transitively: `SimulatableBrawlerTypes.h` includes `DAttackRadialSimulation.h`,
so reading the default off the radial sub-simulation cost no new include. That is the whole reason
the initialiser is written as a function call rather than as a literal.

**The slice that is not neutral.** `brawlerProjectileSimulation::PlayerInput::zero()` returns
`PlayerInput{}`, so its shipped neutral aim is `(0,0,0)`, not `(0,0,1)`. That type's own comment
says so and calls fixing it **a wire change** — the pre-fold `getZeroPlayerInput()` passed a
value-initialised projectile input, and `(0,0,0)` is therefore the shipped wire value.

**⚠ So a default-constructed `ContinuousInputFields` packs to `getZeroPlayerInput()` in four slices
of five, and differs from it in the projectile slice's aim.** An earlier version of the header's
comment claimed all five matched; that sentence was corrected by `brawler-movement-simulation`
task 17, following the task-22 review's N-2.

⛔ **And the four-of-five figure rests on a coincidence the reader cannot see.** The *machine*
slice matches only because `inputSequence::kNoMatch` is `0`, which happens to equal that slice's own
defaulted `triggeredActionId`. **If `kNoMatch` ever becomes non-zero the machine slice diverges too
and the four-of-five sentence goes false.** Until task 74 that happened with no compile error
anywhere in the tree. ⭐ **That dependency is no longer a sentence.** It is a `static_assert` in the header, directly
below `ContinuousInputFields`, so a non-zero `kNoMatch` is a translation failure in every unit
that includes this header rather than a claim that quietly goes false. It was guard **G-02**, now
retired. ⚠ Note what that changes: the edit is typed in `InputSequence/InputSequence.h`, and the
person making it now learns about this file from the compiler instead of not learning at all.

**Nothing depends on the equality.** `getZeroPlayerInput()` is built by the composite's own
`zero()` — the alias is spelled `SimulationInputComposite` at its declaration in
`SimulatableBrawlerTypes.h` — never from this struct. The two are related by construction, not by
an assertion.

---

## 3. Why the continuous read is one function

See §1 for the templating argument. The operational consequence, stated here because it is the
reason the function exists rather than a rule anyone can break at a call site: **there is exactly
one place in the tree that turns an input source into `ContinuousInputFields`,** so a change to what
"the continuous input" *means* lands on both the simulated path and the render-echo path in one
edit. The alternative — each caller reading the three accessors itself — is two copies that agree
today and diverge the first time one of them gains a field.

---

## 4. `InputFlagFields`: why the type exists and what it replaced

**Task 14's shape.** When the tree had exactly one input flag, `makeSimPlayerInput` took it as a
trailing defaulted `bool holdGuard = false`. That was correct for one flag, and task 14's reviewer
endorsed it as such.

**Why it did not survive contact with a second flag.** Four post-v1 features are candidates for a
flag of their own — wall run (task 20), jump (21), dash (31) and ski mode (48). Four more trailing
defaulted `bool`s would have been:

1. **four more silent-omission traps** — a caller that forgets one compiles, runs, and quietly sends
   the neutral value forever; and
2. **a five-`bool` positional call site in which transposing two arguments type-checks** — every
   argument has the same type, so the compiler cannot tell `leftAttack, rightAttack` from
   `rightAttack, leftAttack`.

Both hazards are *structural*: no amount of care at the call site removes them, because the type
system is not being asked anything. **Task 52 replaced the trailing `bool` with a named-field
aggregate**, and both hazards are gone by construction:

* a field is set **by name** (`{.holdGuard = true}`), so no flag can be written positionally and
  transposing two of them is a compile error rather than a wrong bit; and
* the parameter has **no default**, so a caller cannot omit it.

Both of those are pinned as **compile-time** facts, not as runtime assertions, by
`MakeSimPlayerInputFlagsTest.cpp`'s
`InputWriter.CallShapeRejectsOmittedAndPositionalFlagArguments`, which requires the
omitted-argument call and the bare-`bool`-in-the-flags-slot call to be **ill-formed** rather than
merely unwise.

⭐ **Task 74 asserted the same two facts in the header** (retired guards **G-06**/**G-07** and
**G-05**), plus a third the test does not cover: that **no trailing `bool` has been appended** —
retired guard **G-11**, which was must-never-move row T1-2. The header assertions are not a
second opinion; they fail in every translation unit that includes the header, so the edit is
caught without building the test target. ⚠ All three are negative assertions, so a fourth one
asserts that the SHIPPED five-argument call still compiles — without it, renaming the
function would make all three vacuously true.

⚠ **The mandatory `{}` is the deliberate part, and it is the part a tidying edit removes.** Because
the parameter has no default, every call site spells the neutral explicitly — so *"no flags"* stays
a decision somebody wrote down rather than something that happened by default. The fence that
defended that inconvenience was must-never-move row T1-4, guard **G-07**; it is now part of the
same `static_assert` that forbids the default, because restoring the default **is** the edit it
was written against.

**The mirror property.** `InputFlagFields` is the named-field mirror of
`brawlerMovementSimulation::PlayerInput::flags`, that type's `flags` byte: one `bool` field per bit, each defaulted to
that bit's neutral value. Today the mirror has exactly one row: `holdGuard` ↔
`brawlerMovementSimulation::kInputFlagHoldGuard`, bit 0. Every bit's neutral is `false`, by the
packing rule at the type — all bits clear **is** `PlayerInput::zero()` — which is why
`InputFlagFields{}` is guaranteed to stay the neutral argument as flags are added, with no edit at any call site that does not model the new
signal.

---

## 5. The flags byte at the write site

### 5.1 One writer, and a reader that shipped first

`makeSimPlayerInput` holds **the only line in the tree that ORs `kInputFlagHoldGuard` into a `flags`
byte**. ⚠ Until task 74 the header said something stronger and false — *"THE ONE AND ONLY WRITER
of `kInputFlagHoldGuard` in the tree"* — while this paragraph, corrected by task 66, said the
true thing. `SimulatableBrawlerTest.cpp:463-464` assigns the constant into a `flags` field and
`BrawlerMovementSimulationTest.cpp` hands it to a rig as a flags byte, so the tree has other
writers; what is unique is the **OR**. Guard **G-09** now carries the corrected sentence. Task 51 shipped the flags byte and its *reader* — step 1's `frozen` gate in
`brawlerMovementSimulation::integrate`, which names itself the only reader — **deliberately without
a writer**, so until task 14 added this line the gate was inert and `frozen` could never come from
input. This line is what makes holding guard actually freeze movement.

That asymmetry is worth keeping in mind when reading either site: a reader with no writer is not a
bug there, it is a staged landing.

**The one production caller that models a real button press** is the simulated path,
`UOGBrawlerInputCollectionComponent::buildPlayerInput`. ⚠ *Production* is the denominator that
matters: `MakeSimPlayerInputFlagsTest.cpp` raises the bit too, legitimately, and an earlier
version of the header's sentence left that unstated.

### 5.2 The bit budget — what is actually reserved, and what is not

⛔ **Bits 1-7 are UNASSIGNED.** `kInputFlagHoldGuard` (bit 0) is the only `kInputFlag*` constant in
the tree. Four post-v1 features are *candidates* — wall run (task 20), jump (21), dash (31), ski
mode (48) — and **not one of them is committed to a bit**: task 48's backlog entry records that it
adds **no input field** at all, and tasks 20, 21 and 31 each say *"if this task adds an input
field"*.

⚠ **This paragraph exists because the opposite claim was in the tree, in the present tense, at
eleven sites across six files, and task 66 could only correct the two in this header.** The
surviving nine are in `BrawlerMovementSimulation.h` (three), `BrawlerMovementSimulationTest.cpp`,
`MakeSimPlayerInputFlagsTest.cpp`, `RoundVsPacketBudgetTest.cpp` (two) and
`SimulatableBrawlerTest.cpp`. They say bits 1-7 are *"already spoken for"* and they name task 20
*"wall-grab"*; task 20 is **Wall run**. **If you are reading one of those and this file, this file
is the corrected one.** ⚠ The header no longer says anything about the bit budget either way
— task 74 removed its prose — so this paragraph is now the only corrected statement of it
in og-brawler.

⚠ **The enumeration above is one site short and the count is one low, and that is task 71's
to fix, not this file's.** Task 66's reviewer measured **three** sites in
`RoundVsPacketBudgetTest.cpp`, not two (its F-1); task 74 re-ran the sweep and confirms three
(`:306`, `:477`, `:483`), so the survivors number **ten**. Left as task 66 shipped it because
the correction is routed elsewhere and a half-fix here would hide it.

⭐ **The rule the false claim was attached to is true and unaffected:** a new per-tick input signal
is a **bit in the existing byte**, never a new member. One `bool` would spend far more of the input
wire budget than one bit does, and `RoundVsPacketBudgetTest.cpp` prices it.

### 5.3 Why the accumulator is written the way it is

Every `|=` into the `uint8_t` accumulator goes through an explicit `static_cast<uint8_t>`, because
`flags | k` undergoes integral promotion to `int` and assigning that back to a `uint8_t` is a
narrowing conversion — MSVC reports it as **C4244**.

⚠ **Earlier wording here asserted that "the -Werror UE modules reject" it.** No `*.Build.cs` in this
repository sets warnings-as-errors; the only warning setting in the tree turns one *off*. The
`static_cast` is right regardless, because the narrowing is real C++ and does not depend on a build
setting. Task 66 corrected the reason clause and left the rule alone.

The same reasoning is why task 14's conditional spelled its zero arm `uint8_t{0}` rather than `0u`.
That code is gone; the reasoning is not.

### 5.4 Adding a flag

⛔ **The recipe used to be in the header, at the write site. Under v2 it is here, and this is
the only copy.** These are the bytes it occupied in the header before task 74 removed them:

```cpp
    // ⭐ ADDING A FLAG — THE WHOLE RECIPE, TWO LINES, NO SIGNATURE CHANGE (§5.4):
    //   1. add a `bool <name> = false;` field to InputFlagFields above, beside holdGuard;
    //   2. add one line below, in the SHIPPED form — ⛔ never a bare `|=`, see the narrowing note:
    //        if (flagFields.<name>) flags = static_cast<uint8_t>(flags | kInputFlag<Name>);
    //
    // ⚠ Callers that don't model the signal need NO edit; those that do name it `{.<name> = true}`.
```

⚠ The cross-references in those bytes are the pre-conversion ones: *"the narrowing note"* is
now guard **G-12**, and *"InputFlagFields above"* is still directly above the write site in the
header. ⭐ And step 1 is now load-bearing in a way it was not: adding a FIELD is silent to the
header’s call-shape assertions, while adding a trailing parameter fires **G-11**’s. The recipe
and the compiler now point the same way.

The *reason the recipe is that short*: the signature does not change, so callers that do not model the new signal need no
edit at all (`{}` already value-initialises the new field to its neutral `false`), and callers that
do name it (`{.<name> = true}`). Adding a flag is therefore a two-line change in one file plus one
constant in `BrawlerMovementSimulation.h` — and that property is the entire return on task 52's
aggregate.

⚠ Until task 66, step 2 of that recipe told the reader to write a bare `|=` — the exact form
guard **G-12** forbids, and not the form the shipped line takes. The copy quoted above is the
corrected one.

---

## 6. The render-rate packer

`makeVisualizationPlayerInput` is defined **in terms of** `makeSimPlayerInput`, on purpose: the
continuous packing then has exactly one implementation, and the two paths can only ever differ in
the discrete arguments they pass. The header no longer states it — it no longer states anything — so
this is the only place it is written down, and it is the premise of everything below.

**Every discrete field is pinned neutral at that call:** no attack buttons, no `holdGuard`, and
`triggeredActionId` at `inputSequence::kNoMatch`, because the tick-stateful motion matcher is never
invoked on this path. `holdGuard` is a **button**, so it is a discrete field and is pinned by
exactly the rule that pins the attack buttons.

⭐ **The consequence is structural, and that is the whole point of the design.** A discrete input
edge *cannot* render-echo — not "should not". There is no per-caller judgment involved and no code
path that could get it wrong, because the render packer has no parameter to pass a discrete value
through. This is the continuous-vs-discrete split of `proposal_ogbrawler_netcode.md` §2.3, enforced
at the data level; `VISUALIZATION_DISCIPLINE.md` states the mesh-only invariant it serves, and
`RenderRateInputEchoTest.cpp` and `MakeSimPlayerInputFlagsTest.cpp`'s
`InputWriter.VisualizationPackerNeverRaisesAFlagBit` pin it.

⭐ **And it stays true for free as flags are added.** Because every flag's neutral value is `false`
and `{}` value-initialises them all, a future flag is neutral at this call site with no edit. The
brace is mandatory — the parameter has no default — which is why the render-echo rule ends up
tagged (**G-14**) at the one place it is applied.

⛔ The result of this function is **cosmetic only**. The fence saying so is guard **G-15**
(must-never-move row T1-3) and its tag sits on the function that produces it, because the render packer returns the *same type* as the simulation packer and nothing
in the signature stops a caller feeding a cosmetic `PlayerInput` to the wire.

---

## 7. The composite is positional

The `return` statement assembles a `simulatableBrawler::PlayerInput` from its sub-simulation
slices, **by position**. There is exactly one site in the tree that does this, which is why guard
**G-13** is tagged on the `movementInput` argument of the `return`: appending a sub-simulation costs one line
here and **no UE edit at all**, because both UE builders route through this function.
⚠ The sentence this replaces said the header *"names each slice in a comment at the call"*.
It named ONE — the movement slice, the one the fence was about; the other four carried no
comment. Corrected rather than moved.

⚠ **Three sentences in this section were true of a five-slice composite and had gone stale.**
The sixth slice (§ 7.1) landed after this document was written: the composite is no longer five
slices, the movement slice is no longer the newest, and `G-13` no longer sits on the LAST
argument of the `return` — it sits, as it always did, on `movementInput`, which stopped being
last. The guard entry itself always named the right site; only these sentences drifted.

The movement slice was the newest of the five. At T1 it was empty — the movement sub-simulation
consumed no input then, and the header said so. That ended when task 14 landed the writer above it:
the slice now carries the input flags byte, and `SerializableFields<brawlerMovementSimulation::PlayerInput>`
carries `flags` on the wire.

### 7.1 The sixth slice, and why nothing explains it at the call

`brawlerRingout::PlayerInput{}` sits after `movementInput` in the `return`, and it is the only
slice here that can never be anything but default-constructed: the type has no fields at all. The
others are each handed something — the radial, machine, guard and projectile slices take
`fields.aimDirection` (the machine slice takes five further values beside it), and the movement
slice takes the flags byte assembled above.

It is on the composite because `ValidDependencies` requires every sub-simulation to name a
`Dependencies::InputType`, and the composite's ownership validator treats that type as OWNED by
the sub-simulation that names it — pointing it at a neighbour's input type is an
`OwnershipOverlap` translation failure, not a style choice. It is **not** there because ring-out
reads a button: a death is positional (the body's Z against an authored kill plane) and a respawn
is a tick countdown, so there is nothing for a player to press.

⛔ **The prohibition that used to stand here as a comment is a `static_assert`, and this document
deliberately does not restate it.** `BrawlerRingoutSimulation.h` asserts
`syncSize<brawlerRingout::PlayerInput>() == 0u` — in the same file as the type, below its
`SerializableFields` specialization, which is the other half of the edit it forbids. ⚠ It does
**not** sit at the class declaration; it stands far below it, past that specialization, so it
is caught at BUILD time rather than at the keyboard. An input byte is multiplied
across every entry of every relayed input ring, so one of them costs roughly **ten times** what a
state byte costs; the exact arithmetic is in that assertion's message and is derived in
`RoundVsPacketBudgetTest.cpp`'s pre-diet table. Ring-out task 11 deleted a ten-line block from the
header that said all of this in prose, because the assertion already forbade the edit the prose
described and cannot be skimmed past.

⚠ Note the boundary of what the assertion covers, because it is exactly the right one: a member
added to that class but left out of `SerializableFields<brawlerRingout::PlayerInput>` does not
fire it — and does not ride the wire either, so it costs no margin. The assertion fences the
wire cost, which is the whole of the prohibition.

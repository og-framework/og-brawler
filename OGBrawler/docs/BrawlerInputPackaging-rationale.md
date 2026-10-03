<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerInputPackaging.h` — rationale

This is the narrative, the derivations and the provenance for the engine-free input packing:
`ContinuousInputFields`, `readContinuousInputFields`, `InputFlagFields`, `makeSimPlayerInput` and
`makeVisualizationPlayerInput`. Under `CommentExtractionRule_v2.md` the header keeps **no prose at all**: a licence line, a
two-line pointer, code, and one `⛔G-nn` tag per guard. Every fence is in
`BrawlerInputPackaging-guards.md`, each with a stable id, and **eight of the sixteen are no longer
live**: five became `static_assert`s in the header, one turned out to be
enforced by the language already, and two (G-01, G-13) retired in og-syncedInput-rework task 3
because the thing each guarded left the wire. This file carries everything else:
the narrative, the derivations, the provenance and the orientation.

**If this file and `BrawlerInputPackaging.h` disagree, the header is authoritative and this file is
stale.** Fix this file; do not soften the header to match it.

⛔ **Do not put a fence in this file.** A prohibition belongs in `BrawlerInputPackaging-guards.md`,
with an id and a `⛔G-nn` tag on the declaration where the wrong line would be typed — or, better,
in a `static_assert` so that it cannot be skimmed past at all. `tools/lint/guard_tag_lint.ps1` is a
hard gate on that join in both directions; a fence filed here has no site and no lint row.

⛔ **This file is not the source of truth for any VALUE.** The neutral aim lives in
`SyncedPlayerInput::zero()` (`BrawlerSyncedPlayerInput.h`); the flag constants live in
`BrawlerMovementSimulation.h`. Where a value
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
discrete and edge-derived fields — the attack buttons, the holdGuard freeze request, and
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

## 2. `ContinuousInputFields`' defaults, and why they now pack to the neutral field for field

**What the defaults are.** `aimDirection` is initialised from `SyncedPlayerInput::zero().aimDirection`
— `(0,0,1)`, the neutral aim of the flat wire struct `simulatableBrawler::SyncedPlayerInput`, spelled
once in `zero()` in `BrawlerSyncedPlayerInput.h` (guard **G-03** forbids re-spelling it here).
`moveStick` and `moveDirectionWorld` default to the zero vectors that struct carries.

⭐ **This header spells no neutral of its own.** `SyncedPlayerInput` is visible here through
`SimulatableBrawlerTypes.h`, which includes `BrawlerSyncedPlayerInput.h`, so reading the default off
it cost no new include. That is the whole reason the initialiser is written as a function call
rather than as a literal.

**Packed with neutral discrete arguments** (no buttons, `triggeredActionId = inputSequence::kNoMatch`,
`InputFlagFields{}` — exactly what `makeVisualizationPlayerInput` passes), **a default-constructed
`ContinuousInputFields` equals `getZeroPlayerInput()` field for field.**

⛔ **And that rests on a coincidence the reader cannot see.** `triggeredActionId` matches only
because `inputSequence::kNoMatch` is `0`, which happens to equal `SyncedPlayerInput`'s own defaulted
`triggeredActionId`. **If `kNoMatch` ever becomes non-zero that field diverges.** Until task 74
that happened with no compile error anywhere in the tree. ⭐ **That dependency is no longer a
sentence.** It is a `static_assert` in the header, directly below `ContinuousInputFields`, so a
non-zero `kNoMatch` is a translation failure in every unit that includes this header rather than a
claim that quietly goes false. It was guard **G-02**, now retired. ⚠ Note what that changes: the
edit is typed in `InputSequence/InputSequence.h`, and the person making it now learns about this
file from the compiler instead of not learning at all.

**Nothing depends on the equality.** `getZeroPlayerInput()` returns `SyncedPlayerInput::zero()`
(`SimulatableBrawlerTypes.h`), never anything built from this struct. The two are related by
construction, not by an assertion.

### 2.1 History: "four slices of five", and the projectile slice that is gone

Until og-syncedInput-rework task 3 the packer built a SIX-slice composite
(`SimulationInputComposite` over the radial, machine, guard, projectile, movement and ring-out
input types), and this section said a default `ContinuousInputFields` packed to
`getZeroPlayerInput()` **in four slices of five**. The fifth was the projectile slice: its
`zero()` returned `PlayerInput{}`, so its shipped neutral aim was `(0,0,0)`, not the `(0,0,1)` the
radial, machine and guard slices used — the pre-fold `getZeroPlayerInput()` had passed a
value-initialised projectile input, which made `(0,0,0)` a shipped wire value. The radial `zero()`
was where the neutral aim was read from, and the machine slice matched only through the
`kNoMatch == 0` coincidence above. An earlier version of the header's comment claimed all five
matched; that was corrected by `brawler-movement-simulation` task 17, following the task-22
review's N-2. Guard **G-01** stood on `struct ContinuousInputFields` to stop someone "tidying" the
projectile neutral into agreement, because that edit moved a shipped wire value.

Task 3 put ONE aim on the wire. The projectile sub-simulation reads no input at all — its
`Dependencies::InputType` is an empty `PlayerInputView` — so no projectile aim is serialized, the
divergence cannot reach a peer, and **G-01** is retired (see the guards file, §R). The legacy
projectile `PlayerInput` type stood, unused, until og-syncedInput-rework task 4 deleted it.

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
`SyncedPlayerInput::flags`, the wire struct's `flags` byte (since og-syncedInput-rework task 3; it
was `brawlerMovementSimulation::PlayerInput::flags` before): one `bool` field per bit, each defaulted to
that bit's neutral value. Today the mirror has exactly one row: `holdGuard` ↔
`brawlerMovementSimulation::kInputFlagHoldGuard`, bit 0. Every bit's neutral is `false`, by the
packing rule at the type — all bits clear is what `SyncedPlayerInput::zero()` carries — which is why
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
input. This line is what makes holding guard actually freeze movement, under the condition the
next paragraph states.

**What the bit means since og-brawler-3rdControllerMode task 5.** `flagFields.holdGuard` is no
longer the raw guard button. It is a **client-resolved freeze request**:
`UOGBrawlerInputCollectionComponent::buildPlayerInput` sets it to
`dInput::stickRouting::guardFreezeRequested(getHoldGuard(), ...)`, which is true only while the
guard is held **and** either the scheme's own movement input (the move routed with the
move-stick-feeds-aim fallback off) or the actual routed move is below the move deadzone. So guard
plus movement walks and guard alone roots. In `AimRelativeSwapped`, guard plus the left stick
alone roots even though the single-stick fallback routes the left stick to move. The simulation
cannot tell such a fallback move from a real one, which is why the decision is made on the client.
The bit's name, position and packing are unchanged and no wire byte moved. This packer still only
copies the bool into the bit.

That asymmetry is worth keeping in mind when reading either site: a reader with no writer is not a
bug there, it is a staged landing.

**The one production caller that models a real button press** is the simulated path,
`UOGBrawlerInputCollectionComponent::buildPlayerInput` (which, as above, passes the press through
`guardFreezeRequested` first). ⚠ *Production* is the denominator that
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

⭐ **Since og-syncedInput-rework task 4 (2026-10-03) no present-tense site says otherwise.** Task 4
absorbed task 71 and task 58's item 3 and ran a joined-text sweep (claims wrap across lines) over
og-brawler, og-brawler-tests, og-simulation and the UE modules. The sites it corrected:
`kInputFlagHoldGuard`'s `static_assert` message in `BrawlerMovementSimulation.h` (the header's
other prose sites had already moved into `BrawlerMovementSimulation-rationale.md` as verbatim
quotes, flagged by its R0-03 and R0-09), `BrawlerMovementSimulationTest.cpp` (one),
`MakeSimPlayerInputFlagsTest.cpp` (two), `RoundVsPacketBudgetTest.cpp` (two, both dated ledger
entries now carrying the correction in place) and a jump comment in
`OGBrawlerInputCollectionComponent.cpp`. On the tree task 4 stood on, the sweep found two sites in
`RoundVsPacketBudgetTest.cpp`, not three, and none in `SimulatableBrawlerTest.cpp`. The two
paragraphs above are kept as the record of what task 66 and task 74 saw.

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
invoked on this path. `holdGuard` comes from a **button** (the guard press, gated by the movement
input on the sim path since og-brawler-3rdControllerMode task 5), so it is a discrete field and is
pinned by exactly the rule that pins the attack buttons.

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

## 7. The wire struct is built by name

The `return` statement builds a `simulatableBrawler::PlayerInput` — since og-syncedInput-rework task 3
an alias of the flat `SyncedPlayerInput` — with **C++20 designated initializers**, one per field, in
declaration order. There is exactly one site in the tree that turns packer arguments into the wire
struct. The compiler rejects a designator written out of declaration order, so the positional
transposition the old composite allowed is gone. ⚠ It does **not** reject an OMITTED designator —
that field silently takes its default — which is why `InputViewSpecTest.cpp` asserts every packer
argument against the view field that reads it.

**What a new input costs** (this is the cost estimate guard **G-13** used to hand on, re-derived for
the flat struct): a new per-tick input is a field on `SyncedPlayerInput` (append only — its
layout `static_assert` names the codec version constants to bump), one designator here, and the
UE builder that captures it (`buildPlayerInput`), plus a field on each sub-simulation `PlayerInputView`
that reads it, written in that sub-simulation's own header. The recipe is in
`BrawlerSyncedPlayerInput-rationale.md`. A new **flag** stays cheaper: a named field on
`InputFlagFields` and a bit in the existing `flags` byte (§5.4), with no wire-struct edit at all.

### 7.1 History: the positional composite, and the slice that could only be default

Until task 3 the `return` assembled a six-slice `SimulationInputComposite` **by position**, and
guard **G-13** was tagged on its `movementInput` argument: appending a sub-simulation's input slice
cost one line here and no UE edit, because both UE builders route through this function. The sixth
slice, `brawlerRingout::PlayerInput{}`, had no fields at all; it was on the composite because
`ValidDependencies` requires every sub-simulation to name a `Dependencies::InputType`, and the
ownership validator treats that type as OWNED — naming a neighbour's input type there is an
`OwnershipOverlap` translation failure. That requirement still holds, and ring-out now satisfies
it with an empty `brawlerRingout::PlayerInputView` that is not on the wire: a death is positional and a
respawn is a tick countdown, so there is still nothing for a player to press.

The `static_assert(syncSize<brawlerRingout::PlayerInput>() == 0u)` in `BrawlerRingoutSimulation.h`
stood beside the unused legacy type until og-syncedInput-rework task 4 deleted both. The wire cost it priced
— an input byte is multiplied across every entry of every relayed input ring — is now carried by
`SyncedPlayerInput`'s size assertion and by `RoundVsPacketBudgetTest.cpp`.

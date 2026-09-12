<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: CommentExtractionRule_v2.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
# `BrawlerInputPackaging.h` — guards

Every fence that stood in the header. Each entry has an **opaque, stable id**; in the header a
single line `// ⛔G-nn` sits exactly where the fence's text used to sit, on the same
declaration.

**If this file and `BrawlerInputPackaging.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check,
is moved to [§R, Retired ids](#r-retired-ids) and its number is spent forever. Reusing one
silently re-points every reference that ever named it. A retired id may be **named** in prose or
in a `static_assert` message; it may never again appear as a `⛔G-nn` **tag**.

⚠ **The id space is PER DOCUMENT, not per tree.** `CollisionCategoryConstants-guards.md`
also has a `G-01`, and that is not a collision: a tag carries the path of the document it
resolves against, and `tools/lint/guard_tag_lint.ps1` resolves and counts per document. Never
cite a bare `G-nn` without saying which header it belongs to.

⭐ **The join is machine-checked, in both directions**, by
`tools/lint/guard_tag_lint.ps1`: every tag resolves to an entry here, every live entry is
referenced by exactly one tag, no id is duplicated, and no retired id reappears as a tag. It is a
hard gate. ⚠ It checks that an entry EXISTS. It never checks that this text is TRUE.

⛔ **Nothing in this file is a rationale.** The narrative, the provenance and the derivations
live in `BrawlerInputPackaging-rationale.md`. A guard is a prohibition plus the consequence
of ignoring it, and nothing else.

⚠ **This file is not the source of truth for any VALUE.** The neutral aims live in each
sub-simulation's own `zero()`; the flag constants live in `BrawlerMovementSimulation.h`. Where a
value appears below it is there to make an argument readable.

⭐ **The must-never-move list is re-founded on these ids** (`CommentExtractionRule_v2.md`
§4.1). `impl/task65_mnm.tsv`'s five T1 rows pinned fence TEXT at a LINE, and under v2 the text
leaves the file, so all five go to `hits 0`. Their successors:

| must-never-move row | guard id | state |
|---|---|---|
| **T1-1** | **G-10** | live — tag at the flags write |
| **T1-2** | **G-11** | RETIRED — `static_assert` |
| **T1-3** | **G-15** | live — tag on `makeVisualizationPlayerInput` |
| **T1-4** | **G-07** | RETIRED — `static_assert` |
| **T1-5** | **G-06** | RETIRED — `static_assert` |

---

## G-01 — The projectile slice’s neutral aim is (0,0,0), and "fixing" it is a wire change

**Tag site:** `BrawlerInputPackaging.h`, immediately above `struct ContinuousInputFields`.
**Taxonomy clause:** —.

**The fence, verbatim — these are the bytes it occupied in the header (line 64 of the R0-corrected pre-conversion file):**

```
// ⛔ The projectile slice's neutral aim is (0,0,0), NOT (0,0,1) — "fixing" it is a WIRE CHANGE.
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** A default-constructed `ContinuousInputFields` packs to `getZeroPlayerInput()` in four slices of five — and NOTHING DEPENDS on that (rationale section 2); the two are related by construction, not by an assertion. The projectile slice is the fifth: its neutral aim is the value-initialised `(0,0,0)` the pre-fold `getZeroPlayerInput()` handed that type, not the `(0,0,1)` the other three use. It reads as an oversight at exactly the place someone would "tidy" the defaults into agreement, and that edit moves a shipped wire value.

⚠ **The authority carries the same prohibition.** `BrawlerProjectileSimulation.h:280-282` says it at the type that owns the value — *"Do not \"fix\" it to (0,0,1): that is a wire change."* This entry is the copy that stood beside the DEFAULTS; the edit it guards is typed in that other file.

---

## G-03 — The neutral aim is spelled once, at the radial sub-simulation’s own `zero()`

**Tag site:** `BrawlerInputPackaging.h`, immediately above the `aimDirection` member.
**Taxonomy clause:** —.

**The fence, verbatim — these are the bytes it occupied in the header (line 70 of the R0-corrected pre-conversion file):**

```
    // ⚠ The neutral aim is spelled ONCE, at the radial sub-sim's own zero(). Never re-spell it.
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** The member is initialised by *calling* `dAttackRadialSimulation::PlayerInput::zero()` rather than by writing `glm::vec3(0.f, 0.f, 1.f)`. The literal is shorter, reads identically, and compiles to the same bytes today — so the tidying edit is invisible in review and in test. What it costs is the coupling: after it, the radial sub-simulation can change its neutral and this header will silently keep the old one.

⛔ **Not convertible to a compile-time check, and the reason is worth recording.** An assertion that the two are EQUAL passes in both arms at the moment the edit is made — the re-spelled literal equals `zero()` today, which is the whole hazard. It would fire later, when they diverge, which is a different and weaker check. And it is out of reach anyway: `zero()` is not `constexpr` (probe C7).

---

## G-04 — The single source of truth for the continuous read

**Tag site:** `BrawlerInputPackaging.h`, immediately above `template <typename Src> readContinuousInputFields`.
**Taxonomy clause:** F5a.

**The fence, verbatim — these are the bytes it occupied in the header (lines 82-83 of the R0-corrected pre-conversion file):**

```
// ⛔ THE SINGLE SOURCE OF TRUTH for the continuous read: both UE builders route through it,
// so the two paths cannot drift (why a template and not an interface: §1; §3).
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** Both UE builders route through this one function — `buildPlayerInput` and `buildLatestVisualizationInput`, `OGBrawlerInputCollectionComponent.cpp:362` and `:439`. The alternative is each caller reading the three accessors itself: two copies that agree today and diverge the first time one of them gains a field. The function is three assignments, so "just inline it here" is a cheap-looking edit at the call site.

---

## G-08 — The discrete fields are the caller’s; this header samples none

**Tag site:** `BrawlerInputPackaging.h`, immediately above `inline PlayerInput makeSimPlayerInput`.
**Taxonomy clause:** F3 (absence fence).

**The fence, verbatim — these are the bytes it occupied in the header (line 108 of the R0-corrected pre-conversion file):**

```
// ⛔ Per-tick packer: the discrete fields are the CALLER'S — this header deliberately has none (§1).
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** An absence fence. The per-tick packer takes `leftAttack`, `rightAttack` and `triggeredActionId` as parameters because sampling them requires edge state, a tick, or the motion matcher — none of which may enter an engine-agnostic header that must also compile in a tree that cannot link UE. Nothing in the signature says the omission is deliberate, so the reflex fix for "this function should just read the buttons" is to add the read.

---

## G-09 — The only line in the tree that ORs `kInputFlagHoldGuard` into a flags byte

**Tag site:** `BrawlerInputPackaging.h`, immediately above `uint8_t flags = 0u;` inside `makeSimPlayerInput`.
**Taxonomy clause:** F5c (enumerated site set).

**The fence, verbatim — these are the bytes it occupied in the header (lines 115-116 of the R0-corrected pre-conversion file):**

```
    // ⭐ THE ONE AND ONLY LINE IN THE TREE THAT ORS `kInputFlagHoldGuard` INTO A FLAGS BYTE; its
    // reader came first (§5.1). ⚠ Tests set a flags byte TO the constant; none of them ORs it.
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** An enumerated site set: the value of the claim is that a second OR SITE would be a defect, and the only place that is visible is the first one. Its other half — that the READER shipped a task earlier, deliberately without a writer — is what stops a reader-with-no-writer elsewhere being read as a bug rather than as a staged landing.

⛔ **This fence was FALSE as it shipped and was corrected before it moved (R0).** It read *"THE ONE AND ONLY WRITER of `kInputFlagHoldGuard` in the tree"*. `SimulatableBrawlerTest.cpp:463-464` assigns the constant into a `flags` field and `BrawlerMovementSimulationTest.cpp` hands it to a rig as a flags byte, so the tree has other writers. What is unique is the **OR**: exactly one line in the tree ORs it into an accumulator. The rationale doc already carried the true form (section 5.1); task 66 corrected the doc and left the header standing. The bytes above are the corrected ones.

---

## G-10 — Name the constant, never the bit’s numeric value

**Tag site:** `BrawlerInputPackaging.h`, immediately above the `if (flagFields.holdGuard)` write.
**Taxonomy clause:** F1a + F6 (must-never-move **T1-1**).

**The fence, verbatim — these are the bytes it occupied in the header (lines 118-120 of the R0-corrected pre-conversion file):**

```
    // ⛔ WRITTEN BY NAMING THE CONSTANT, NEVER BY THE BIT'S NUMERIC VALUE. Spelling the set bit
    // positionally compiles and is correct only because holdGuard happens to sit at bit 0 today,
    // and goes silently wrong the moment it moves.
```

**What breaks if it moves.** `flags |= 1` is correct **today**, and only because holdGuard sits at bit 0. It compiles, passes every test, and goes silently wrong the day the bit moves. The prohibition only works on the line where the numeral would be typed.

⛔ **Not convertible to a compile-time check.** `flags | 1` and `flags | kInputFlagHoldGuard` are the same value and the same object code; no expression in C++ distinguishes the two spellings. This is a LOCAL edit the compiler cannot express — see the implementation notes on task 72’s partition.

---

## G-12 — Every OR into the flags byte goes through an explicit `static_cast<uint8_t>`

**Tag site:** `BrawlerInputPackaging.h`, immediately above the `if (flagFields.holdGuard)` write.
**Taxonomy clause:** F1a.

**The fence, verbatim — these are the bytes it occupied in the header (line 136 of the R0-corrected pre-conversion file):**

```
    // ⛔ Every `|=` here goes through an explicit `static_cast<uint8_t>`; never a bare one (§5.3).
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** `flags | k` undergoes integral promotion to `int`, and assigning that back to a `uint8_t` is a narrowing conversion — MSVC reports **C4244**. The bare `flags |= k` is shorter, is what everyone writes, and is not an error in this tree, because no `*.Build.cs` here sets warnings-as-errors. So the wrong form compiles quietly and the rule survives only as long as someone restates it at the line.

⛔ **Not convertible to a compile-time check.** The prohibition is on a SPELLING that produces a warning, not an error, and nothing in the language lets a header assert how the line below it was written.

---

## G-13 — The composite is positional, and this is the one site that assembles one from fields

**Tag site:** `BrawlerInputPackaging.h`, immediately above the `movementInput` argument of the `return`.
**Taxonomy clause:** F5c (enumerated site set).

**The fence, verbatim — these are the bytes it occupied in the header (lines 151-152 of the R0-corrected pre-conversion file):**

```
        // ⛔ The composite is POSITIONAL and this is the ONE site in the tree that assembles
        // one from fields — appending a slice costs one line here and NO UE edit (§7).
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** The `return` builds a five-slice composite by position. The useful half of the claim is the cost estimate it hands the next person: because both UE builders route through this function, a sixth sub-simulation costs one line HERE and no UE edit at all. Someone who does not know that budgets the change as a UE-side job and looks for the assembly sites that do not exist.

---

## G-14 — The render packer pins every discrete field neutral

**Tag site:** `BrawlerInputPackaging.h`, immediately above `inline PlayerInput makeVisualizationPlayerInput`.
**Taxonomy clause:** F5a.

**The fence, verbatim — these are the bytes it occupied in the header (lines 156-157 of the R0-corrected pre-conversion file):**

```
// ⛔ Render-rate packer: continuous fields only, every discrete field pinned neutral here —
// no attack buttons, no holdGuard, triggeredActionId at inputSequence::kNoMatch (§6).
```

**What breaks if it moves.** **Written for this entry (task 66 guard, not on the must-never-move list).** The continuous-vs-discrete split is enforced at the DATA level: the render packer has no parameter through which a discrete value could arrive, so a discrete input edge structurally CANNOT render-echo. The four neutral arguments in the body are what implement that, and each of them reads like a placeholder somebody forgot to wire up.

⚠ **This prohibition was ALSO hoisted to the file header** (pre-conversion lines 38-42, rule 1 of two). Both copies are accounted for by this entry; the hoisted bytes are quoted in [section H](#h-the-two-hoisted-copies).

---

## G-15 — The render packer’s result is cosmetic only

**Tag site:** `BrawlerInputPackaging.h`, immediately above `inline PlayerInput makeVisualizationPlayerInput`.
**Taxonomy clause:** F5a (must-never-move **T1-3**).

**The fence, verbatim — these are the bytes it occupied in the header (lines 159-160 of the R0-corrected pre-conversion file):**

```
// ⛔ The result is COSMETIC ONLY and
// must never be fed to the simulation or the input RPC.
```

**What breaks if it moves.** The render-rate packer returns the **same type** as the sim packer. Nothing in the signature stops a caller feeding a cosmetic `PlayerInput` to the wire; this sentence is the only thing that does, and it must sit on the function that produces it.

⚠ **The caller carries the same fence.** `OGBrawlerInputCollectionComponent.h:81` reads *"Cosmetic only: never feed this to the simulation or to the input RPC."* — and that is the file where the forbidden edit is actually typed.

---

## §R Retired ids

⭐ **Five of these six were fences that became COMPILE ERRORS** and the sixth was already one (v2 §1.4: *"where a fence can be converted into a compile error, do that and write no guard at all"*). The prohibition now lives in a `static_assert` message in the header. That is strictly stronger than the sentence it replaces: it cannot be skimmed past and it cannot go stale.

⛔ **These ids are spent.** They may be named — five of them are named in `static_assert` messages — but they must never again appear as a `⛔G-nn` **tag** in source.

⭐ **Three of the five must-never-move fences are in this section.** That is the headline measurement of this conversion.

---

### G-02 — RETIRED, converted to a `static_assert`

Must-never-move: none. Was the dependency clause of the four-of-five caveat. The CLAIM (four slices of five) is narrative and lives in `BrawlerInputPackaging-rationale.md` section 2. The DEPENDENCY — that the machine slice matches only because `inputSequence::kNoMatch` is `0` — is now a translation failure.

**The text it replaced, verbatim (lines 66-67 of the R0-corrected pre-conversion file):**

```
// ⚠ A default-constructed value packs to getZeroPlayerInput() in four slices of five, and the
// machine slice matches ONLY because inputSequence::kNoMatch is 0 (§2).
```

**Now enforced by**, in the header, immediately below `struct ContinuousInputFields`:

```cpp
static_assert(inputSequence::kNoMatch == 0u, "...");
```

The edit the sentence warned about — giving `kNoMatch` a non-zero value — was previously silent everywhere in the tree. It is now a compile error in every translation unit that includes this header. ⭐ **This is a REMOTE edit (it is typed in `InputSequence/InputSequence.h`) that the compiler CAN express**, which is the first counter-example to task 72’s local-edit/compiler-expressible partition. Poison arm P4 sets `kNoMatch = 0xFFFFFFFFu` on an out-of-tree copy and the assertion fires.

---

### G-05 — RETIRED, converted to a `static_assert`

Must-never-move: none. Was the statement that a flag is set by NAME, so transposing two is a compile error. It described a property the language already enforced for designated initialisers; what it did NOT enforce was the other half — that nothing but an `InputFlagFields` may fill the flags slot. That half is now asserted.

**The text it replaced, verbatim (line 96 of the R0-corrected pre-conversion file):**

```
// ⛔ A field is set BY NAME (`{.holdGuard = true}`), so transposing two is a compile error.
```

**Now enforced by**, in the header, below `makeSimPlayerInput`:

```cpp
static_assert(!detail::kBareBoolInFlagsSlotCompiles<ContinuousInputFields>, "...");
```

Poison arm P2 gives `InputFlagFields` a converting constructor from `bool` on an out-of-tree copy and the assertion fires. ⚠ `MakeSimPlayerInputFlagsTest.cpp`’s `InputWriter.CallShapeRejectsOmittedAndPositionalFlagArguments` already pinned this; the header assertion moves the failure into every TU that includes the header, so it does not wait for the test target to be built.

---

### G-06 — RETIRED, converted to a `static_assert`

Must-never-move **T1-5** (F1d + F5a). The **absence of a default** is the design, and an absence is invisible: a reader sees a required argument and reads it as an oversight. It also carries the only pointer to the compile-time fences that enforce it.

**The text it replaced, verbatim (lines 98-99 of the R0-corrected pre-conversion file):**

```
// ⛔ the parameter has NO DEFAULT, so a caller cannot omit it — see the fences in
//   `MakeSimPlayerInputFlagsTest.cpp`, which pin that call as ILL-FORMED, not merely unwise.
```

**Now enforced by**, in the header, below `makeSimPlayerInput`:

```cpp
static_assert(!detail::kOmittedFlagsArgumentCompiles<ContinuousInputFields>, "...");
```

Poison arm P1 restores the default on an out-of-tree copy and the assertion fires. The message carries the pointer to `MakeSimPlayerInputFlagsTest.cpp` that the sentence carried.

---

### G-07 — RETIRED, converted to a `static_assert`

Must-never-move **T1-4** (F1c). Defends a deliberate *inconvenience* — the mandatory `{}` — against the tidying edit that would restore a default. Without it, "why must I write `{}`?" has no answer at the site.

**The text it replaced, verbatim (lines 101-102 of the R0-corrected pre-conversion file):**

```
// ⚠ The neutral is still spelled explicitly at every call site (`{}`), so "no flags" stays a
// decision somebody wrote down.
```

**Now enforced by**, in the header, below `makeSimPlayerInput`:

The SAME assertion as G-06, and deliberately so: the tidying edit T1-4 defends against **is** restoring the default, and the message names both.

```cpp
static_assert(!detail::kOmittedFlagsArgumentCompiles<ContinuousInputFields>, "...");
```

⚠ The *answer* to "why must I write `{}`?" is narrative and is in `BrawlerInputPackaging-rationale.md` section 4; the assertion message carries the one-line form (*"so \"no flags\" stays a decision somebody wrote down"*).

---

### G-11 — RETIRED, converted to a `static_assert`

Must-never-move **T1-2** (F1a + F2). A trailing defaulted `bool` is the obvious way to add flag #2, and it is what the previous shape did. The alternative and its consequence — a silent-omission trap plus a transposable positional call — are both named here. Moved to a doc, the edit is made by someone who never opened it.

**The text it replaced, verbatim (lines 132-134 of the R0-corrected pre-conversion file):**

```
    // ⛔ What you must NOT do is
    // append another trailing `bool`: that was task 14's shape, correct for exactly one flag,
    // and four more of them would have been four more silent-omission traps.
```

**Now enforced by**, in the header, below `makeSimPlayerInput`:

```cpp
static_assert(!detail::kTrailingBoolCompiles<ContinuousInputFields>, "...");
```

⭐ **This is the only one of the three call-shape assertions that is NEW enforcement.** `MakeSimPlayerInputFlagsTest.cpp` pins the omitted-argument call and the bare-`bool`-in-the-flags-slot call; it does not pin the APPENDED trailing bool, which is the edit T1-2 is about. Poison arm P3 appends a trailing defaulted `bool` on an out-of-tree copy and the assertion fires.

⭐ **And it is silent on the sanctioned path.** Adding a `bool <name> = false;` FIELD to `InputFlagFields` — the recipe this fence exists to steer you to — changes nothing the assertion tests. It fires on the wrong path and only on the wrong path.

⚠ This prohibition was ALSO hoisted to the file header (pre-conversion lines 43-46, rule 2 of two); the hoisted bytes are quoted in [section H](#h-the-two-hoisted-copies).

---

### G-16 — RETIRED, the language already enforced it

Must-never-move: none. Was the note that the `{}` argument at the render-packer call is the defaulted-field neutral rather than an omission.

**The text it replaced, verbatim (line 167 of the R0-corrected pre-conversion file):**

```
                              // ⛔ `{}` is MANDATORY: the DEFAULTED-FIELD neutral, not an omission.
```

⭐ **Retired WITHOUT writing an assertion, because the language already enforces it.** The flags parameter has no default (G-06), so the `{}` cannot be dropped: removing it is a translation failure today, with no help from this file. The only way to make dropping it legal is to restore the default — and that is exactly what the G-06 assertion forbids.

⚠ The fence’s other half — what `{}` MEANS, namely that every field is at its neutral and so a future flag is neutral here for free — is narrative and is in `BrawlerInputPackaging-rationale.md` section 6.

---

## H. The two hoisted copies

⚠ The pre-conversion header carried a block at lines 38-46 titled *"THE TWO RULES THAT REPEAT
AT EVERY SITE BELOW, HOISTED HERE SO THEY ARE NOT RESTATED"*. Both rules are restatements of
fences that also stood at their own sites, so neither gets an id of its own; they are recorded
here so that the census is closed and nobody reads their absence as a deletion.

**Rule 1 — the same prohibition as G-14** (lines 38-42, the block title included):

```
// ⭐ THE TWO RULES THAT REPEAT AT EVERY SITE BELOW, HOISTED HERE SO THEY ARE NOT RESTATED:
//   1. A DISCRETE INPUT EDGE STRUCTURALLY CANNOT RENDER-ECHO. The continuous-vs-discrete
//      split is enforced at the DATA level, not by per-caller judgment: the render packer
//      has no parameter to pass a discrete value through. VISUALIZATION_DISCIPLINE.md
//      states the mesh-only invariant it serves.
```

**Rule 2 — the same prohibition as G-11** (lines 43-46):

```
//   2. A NEW PER-TICK SIGNAL IS A BIT in brawlerMovementSimulation::PlayerInput's flags
//      byte, NEVER a new member, and it reaches this file as a NAMED FIELD on
//      InputFlagFields — never as another trailing `bool` parameter. Every field's
//      neutral is `false`, so `InputFlagFields{}` stays the neutral argument for free.
```

⭐ **What v2 did to the hoist is worth recording.** A hoisted copy is a fence with no site:
it is at the top of the file, where no forbidden edit is typed. Under v1 it cost nine lines and
looked like diligence. Under v2 there is nowhere to put it — a tag has to point at a
declaration — and the duplication became visible the moment the fences were given ids. Rule
2's content is now a `static_assert` message; rule 1's is G-14.

<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: CommentExtractionRule_v2.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: task65_must_never_move.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
# `DAttackMachineSimulationRuntimeTweakables.h` — guards

Every fence that stood in the header. Each entry has an **opaque, stable id**; in the header a
single line `// ⛔G-nn` sits exactly where the fence's text used to sit, on the same
declaration.

**If this file and `DAttackMachineSimulationRuntimeTweakables.h` disagree, the header is
authoritative and this file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check, is
moved to [§R, Retired ids](#r-retired-ids) and its number is spent forever. Reusing one silently
re-points every reference that ever named it. A retired id may be **named** in prose or in a
`static_assert` message; it may never again appear as a `⛔G-nn` **tag**.

⚠ **The id space is PER DOCUMENT, not per tree.** `BrawlerInputPackaging-guards.md` and
`CollisionCategoryConstants-guards.md` also have a `G-01`, and that is not a collision: a tag
carries the path of the document it resolves against, and `tools/lint/guard_tag_lint.ps1`
resolves and counts per document. Never cite a bare `G-nn` without saying which header it
belongs to.

⭐ **The join is machine-checked, in both directions**, by `tools/lint/guard_tag_lint.ps1`:
every tag resolves to an entry here, every live entry is referenced by exactly one tag, no id is
duplicated, and no retired id reappears as a tag. It is a hard gate. ⚠ It checks that an entry
EXISTS. It never checks that this text is TRUE.

⛔ **Nothing in this file is a rationale.** The narrative, the provenance and the derivations
live in `DAttackMachineSimulationRuntimeTweakables-rationale.md`. A guard is a prohibition plus
the consequence of ignoring it, and nothing else.

⚠ **This file is not the source of truth for any VALUE.** The refused names, their reason
strings and the shipped deadzone defaults live in
`DAttackMachineSimulationRuntimeTweakables.cpp`; the console variables and their tombstones live
in `Source/OGBrawlerUnreal/MovementSchemeCVar.cpp`. Where a value appears below it is there to
make an argument readable.

⭐ **The must-never-move list is re-founded on these ids** (`CommentExtractionRule_v2.md` §4.1).
`task65_must_never_move.md`'s four **T2b** rows pinned fence TEXT at a LINE, and under v2 the
text leaves the file, so all four go to `hits 0`. Their successors:

| must-never-move row | clause | guard id | state |
|---|---|---|---|
| **T2b-1** | F4 + F2 | **G-02** | live — tag on `struct RefusedVariable` |
| **T2b-2** | F6 + F5a | **G-06** | live — tag on `refusedVariableReason` |
| **T2b-3** | F5a | **G-04** | live — tag on `RefusedVariable::name` |
| **T2b-4** | F2 | **G-05** | RETIRED — `static_assert` |

⛔ **Two of this file's rows carried text that R0 found FALSE, and one carried a false
CONSEQUENCE on the must-never-move list itself.** The corrections are recorded in the entries
below, at G-01, G-03 and G-04. Nothing false was moved into this document.

---

## G-01 — Every reader of the stick deadzones is enumerated here

**Tag site:** `DAttackMachineSimulationRuntimeTweakables.h`, immediately above
`OGBRAWLER_API extern std::atomic<float> g_moveStickDeadzone;`.
**Taxonomy clause:** F5c (enumerated site set). **Must-never-move:** none.

**The fence, verbatim — these are the bytes it occupied in the header (lines 23-27 of the
R0-corrected pre-conversion file):**

```
// Stick deadzones — magnitudes below these are treated as no input by the UE layer:
// UOGBrawlerInputCollectionComponent's direction builds (buildAimDirection,
// buildMoveDirectionWorld) and its motion-matcher call (resolveTriggeredActionId), plus
// SimmableUpdateComponent's input-history poll, which uses the same number so "no input"
// means one thing across both displays. ⛔ Adding a reader means adding it here.
```

**What breaks if it moves.** Per Taxonomy F5c an enumeration *is* a maintenance contract: a new
site added without updating it silently breaks the invariant the count asserts. The omitted
sites are the ones that matter. `SimmableUpdateComponent`'s input-history poll feeds the same
number into the visualization, so the deadzone is what the input-history display uses to decide
"no input" — change the deadzone's meaning and you silently change what the meter renders as
neutral, in a file the enumeration never named.

⛔ **This fence was FALSE as it shipped and was corrected before it moved (R0), with two
independent defects.** It read (the fence info string `retracted` marks a quote of SUPERSEDED
bytes, so no checker mistakes it for a span that moved):

```retracted
// Stick deadzones — magnitudes below these are treated as no input by the UE
// input-collection layer (Move(), buildMoveDirectionWorld, buildAimDirection).
```

* **(a) `Move()` does not exist.** `grep -n "::Move("` over the whole scan root, excluding the
  engine's own movement-input helper, returns **0 hits**. `OGBrawlerUECharacter.h` and
  `OGBrawlerInputCollectionComponent.cpp` both record its deletion in the past tense. A reader
  who greps the named function to find where a deadzone is applied gets nothing, and cannot tell
  "renamed" from "never existed".
* **(b) The set was INCOMPLETE.** The measured readers of `g_moveStickDeadzone` /
  `g_aimStickDeadzone` are `OGBrawlerInputCollectionComponent`'s `buildAimDirection` and
  `buildMoveDirectionWorld`, its `resolveTriggeredActionId` call, and
  `SimmableUpdateComponent`'s `pollInputHistory` and `captureRowFieldsOf` calls — a second FILE,
  and a motion-matcher call that is not a direction build at all. The bytes above are the
  corrected ones.

⛔ **Not convertible to a compile-time check.** The claim is a statement about *who reads a
global across translation units*, and nothing in a header can count that. Probe arm CAND 5 adds
a sixth reader of `g_moveStickDeadzone` and updates no enumeration anywhere; it compiles clean.

---

## G-02 — The refused-name mechanism is not dead weight, and this is why

**Tag site:** `DAttackMachineSimulationRuntimeTweakables.h`, immediately above
`struct RefusedVariable`.
**Taxonomy clause:** F4 (unreachable-by-design) + F2. **Must-never-move: T2b-1.**

**The fence, verbatim — these are the bytes it occupied in the header (lines 53-57 of the
R0-corrected pre-conversion file):**

```
// ⛔ THE DEFECT THIS EXISTS TO KILL: a stale ini or a stale named-pipe script naming a
// constant that NO LONGER EXISTS used to be indistinguishable from a typo — both fell off the
// end of `SetVariable` and returned `false`, and on the UE side an unregistered cvar name is
// dropped by the console with no diagnostic at all. The operator then believes they tuned
// something. A name that USED to mean something must fail LOUDLY, not silently.
```

**What breaks if it moves.** The whole `RefusedVariable` mechanism *looks* like dead weight: a
table of names nothing uses, three console variables whose value is always empty, and a sweep
that normally reports zero. This is the only text that says why a **silent** `false` was
unacceptable. Coverage tooling and a tidy reader both argue for deleting the table; moved to a
doc, they delete it.

⛔ **Not convertible to a compile-time check, and it is the hardest class there is.** Two
separate reasons, both measured:

1. The accessors that would carry any assertion about the table's existence are ordinary
   functions declared here and defined in another translation unit. Probe arm CAND 3 writes
   `static_assert(refusedVariablesEnd() != refusedVariablesBegin(), …)` and MSVC rejects it:
   *"expression did not evaluate to a constant … failure was caused by call of undefined
   function or one not declared 'constexpr'"*.
2. ⛔ **Even if it were expressible it would still be DOES NOT, because this is a DELETION
   fence.** The edit it forbids is removing the mechanism, and an assertion that reads the
   mechanism is removed along with it. An assertion cannot guard its own subject's existence.

---

## G-03 — A `refusedOnCVarPath` row arms two vectors and obliges you to hand-write the third

**Tag site:** `DAttackMachineSimulationRuntimeTweakables.h`, immediately above
`bool        refusedOnCVarPath;`.
**Taxonomy clause:** F5a (per-field contract) + F5c. **Must-never-move:** none.

**The fence, verbatim — these are the bytes it occupied in the header. Two spans, MERGED into
one entry** (they are the same fact at two granularities, and v2's one-tag-per-site rule forbids
stacking a second tag on `struct RefusedVariable`, which already carries G-02).

Lines 59-65 of the R0-corrected pre-conversion file:

```
// ⭐ ONE TABLE, THREE VECTORS — AND ONLY TWO OF THEM ARE AUTOMATIC. `SetVariable` (the
// named-pipe path, below) consults the table, and the UE layer's `sweepRefusedNames()`
// (`MovementSchemeCVar.cpp`) walks it to scan `[ConsoleVariables]` in the shipped ini for
// `OGBrawler.<name>` at the one-time read. ⛔ The console tombstone is NOT generated from the
// table — a console object needs a name that outlives it, so each is a hand-written
// `FAutoConsoleVariable` in that file. Adding a `refusedOnCVarPath` row here therefore arms
// two vectors and makes a `checkf` fire until you add the third by hand.
```

Lines 79-80 of the same file:

```
    // True ⇒ the UE layer's sweep scans the ini for `OGBrawler.<name>` and requires a
    // hand-written tombstone cvar of that name to exist (`checkf`).
```

**What breaks if it moves.** A reader adds a fifth table row with the flag set, believing all
three vectors are armed, ships, and a development build `checkf`-crashes at the one-time read
in `sweepRefusedNames()`. The sentence invites exactly the edit that breaks it. The `bool` field
is where the decision is made and it is the only place the obligation is visible.

⛔ **This fence was FALSE as it shipped, at THREE sites in this one header, and was corrected
before it moved (R0).** It read *"ONE TABLE, BOTH VECTORS … the UE layer (`MovementSchemeCVar.cpp`)
walks it to (a) **register a tombstone** `OGBrawler.<name>` console variable … Adding a name here
**arms all three**"*, and at the field, *"True ⇒ the UE layer **also tombstones**
`OGBrawler.<name>` and scans the ini for it."*
The walk **checks** the tombstones; it never creates them. `MovementSchemeCVar.cpp` says so in
terms at the statics — *"ONE STATIC PER NAME, WITH A LITERAL NAME … these cannot be generated
from the table in a loop without leaking strings … a `checkf` fires at the one-time read if
somebody adds an entry and forgets its tombstone."* Today: 4 table rows, 3 of them
`refusedOnCVarPath == true`, and exactly 3 hand-written statics — consistent, because the
`checkf` has been enforcing it.
⭐ **The defect survived because nobody read the two files against each other**, which is what an
R0 pass is for and what neither coverage nor any lint would have caught.

⛔ **Not convertible to a compile-time check.** The obligation is discharged by a
`FAutoConsoleVariable` static in a **different module**, and is enforced by a runtime `checkf`
that already exists there. Probe arm CAND 6 writes `constexpr RefusedVariable{ "ProbeOnlyName",
"probe reason", true }` with no tombstone anywhere and compiles clean.

---

## G-04 — The refused table is matched case-insensitively; the live names are not

**Tag site:** `DAttackMachineSimulationRuntimeTweakables.h`, immediately above
`const char* name;`.
**Taxonomy clause:** F5a (per-field contract). **Must-never-move: T2b-3.**

**The fence, verbatim — these are the bytes it occupied in the header (lines 73-74 of the
R0-corrected pre-conversion file):**

```
    // Matched CASE-INSENSITIVELY: a stale script's casing is not something to bet on.
    // ⚠ Only this table is: the LIVE names in `SetVariable` are matched case-SENSITIVELY.
```

**What breaks if it moves.** Binds the `name` field. Nothing about `const char* name;` says the
comparison is case-folded, so an editor adding a row spells it however the constant was spelled,
and a maintainer "tidying" `equalsIgnoringCase` into `strcmp` breaks every stale script this
exists to catch.

⭐ **The second line is new, and it closes a trap R0 flagged as an OPEN QUESTION rather than a
false claim.** Every **live** name in `SetVariable` is matched with `name == "MovementScheme"`
and friends — case-SENSITIVELY. So `setvariable movementscheme AimRelative` falls off the end and
is reported as an unknown name, while `setvariable maxsnapspeed 400` gets a full explanation. A
reader of the first line alone may reasonably generalise the insensitivity to the whole path.
⚠ Whether that asymmetry is a defect rather than a documentation gap is **not settled here and
needs its own backlog entry** — changing the behaviour was out of scope for tasks 65, 67 and 77.

⛔ **The must-never-move row's stated consequence was WRONG, and this entry corrects it.**
`task65_must_never_move.md` T2b-3 ends *"…breaks every stale script this exists to catch — **with
no test name to blame**."* There **is** a test name to blame:
`RuntimeTweakablesRefusedNamesTest.cpp`, case *"A retired tunable name is refused with a reason,
not silently ignored"*, requires `refusedVariableReason("maxsnapspeed")` and
`refusedVariableReason("MAXSNAPSPEED")` to be non-null, and a `strcmp` tidy-up turns both red.
The **prohibition** stands and the fence is unchanged; only the clause about there being no
diagnostic is retracted, and the clause never stood in the header. ⚠ The correction to the
must-never-move row itself is **routed to the lead** — that document is not this task's to edit.

⛔ **Not convertible to a compile-time check.** The comparison lives in a function body in
another translation unit with internal linkage; no expression in this header observes it. Probe
arm CAND 7 defines both a case-folding matcher and the forbidden `strcmp` one over the table's
own field and compiles clean — the header cannot tell them apart.

---

## G-06 — A nullptr reason does not mean the name is live

**Tag site:** `DAttackMachineSimulationRuntimeTweakables.h`, immediately above
`OGBRAWLER_API const char* refusedVariableReason(const std::string& name);`.
**Taxonomy clause:** F6 (the signature misleads) + F5a. **Must-never-move: T2b-2.**

**The fence, verbatim — these are the bytes it occupied in the header (lines 89-92 of the
R0-corrected pre-conversion file):**

```
// The reason `name` is refused, or nullptr when the name is not a refused one. ⛔ A nullptr
// return does NOT mean the name is live — it means it is not KNOWN-dead; an ordinary typo also
// returns nullptr. That difference is the whole point: a refused name is discriminated from a
// typo by a non-null reason, and `SetVariable` reports the two differently.
```

**What breaks if it moves.** `const char* refusedVariableReason(const std::string&)` reads
exactly like a predicate: nullptr = "fine". It means "not KNOWN-dead". A caller who treats
nullptr as "live" re-creates the typo/retired-name conflation the whole file exists to remove —
and the correction only works beside the declaration whose return type is the hazard.

⛔ **Not convertible to a compile-time check.** The fence is about what a *value* MEANS, not
about what type it has. Probe arm CAND 4 writes the exact misreading —
`return refusedVariableReason(name) == nullptr;` used as "not refused, therefore live" — and it
compiles with no diagnostic. The only construction that would express it is a return type that
cannot be implicitly tested for truth, and that is an API change across four other files, not a
conversion.

---

## §R Retired ids

⛔ **These ids are spent.** They may be **named** — G-05 is named in a `static_assert` message —
but they must never again appear as a `⛔G-nn` **tag** in source.

⚠ **A retired-into-an-assertion id is NOT guarded against the assertion being deleted.**
`guard_tag_lint.ps1` CHECK 4 guarantees G-05 is never re-tagged; it does not check that the
`static_assert` still exists. The forward pin is the assertion's own message text, which names
the row and the guard — grep the header for *"Was fence T2b-4, guard G-05"*.

---

### G-05 — RETIRED, converted to a `static_assert`

Must-never-move **T2b-4** (F2). The reason the refused table is exposed as a raw half-open
pointer pair is a **module-boundary** constraint that is invisible from either side of it.

**The text it replaced, verbatim (lines 84-85 of the R0-corrected pre-conversion file):**

```
// Half-open range over the refused table. Arrays rather than a container so the UE layer can
// walk it without agreeing on an allocator across the module boundary.
```

**Now enforced by**, in the header, immediately below the two declarations:

```cpp
static_assert(std::is_same_v<decltype(refusedVariablesBegin()), const RefusedVariable*>
           && std::is_same_v<decltype(refusedVariablesEnd()),   const RefusedVariable*>, "…");
```

⭐ **This is new enforcement, not a restatement.** `const RefusedVariable*` begin/end pairs are
exactly what a modern-C++ reader replaces with `std::span` or a `std::vector` getter; before
this assertion that edit compiled in this header and broke at the UE module boundary, where the
two sides would have had to agree on an allocator. Now it is a translation failure in **every**
translation unit that includes the header — including `MovementSchemeCVar.cpp`, which is the
file that walks the table.

**Poison arms, all four run on an out-of-tree shadow copy of the header
(`impl/task77/probe_ondisk.bat`); the shipped tree was never mutated to take the measurement:**

| arm | edit | result |
|---|---|---|
| `P0_noop` | none — the converted header as it ships | **clean** (the no-op control: the arms are not merely breaking the TU) |
| `P1_begin_span` | `refusedVariablesBegin()` alone returns `std::span<const RefusedVariable>` | **FIRED** |
| `P2_end_span` | `refusedVariablesEnd()` alone returns `std::span<const RefusedVariable>` | **FIRED** |
| `P3_both_vector` | both return `std::vector<RefusedVariable>` by value | **FIRED** |

⭐ **Both halves of the pairing were poisoned separately, and both fire.** An assertion that
caught only one half would be equivalent to one instance of the prohibition while looking
enforced, which is worse than the comment it replaced.

⚠ **What the assertion does NOT catch:** a `std::span` getter ADDED alongside the pair. The
fence is about *replacing* the pair, and deleting either declaration is a hard error at the
assertion rather than a silent pass, so the sanctioned and the forbidden paths stay
distinguishable. **The header gained one line for this:** `#include <type_traits>`.

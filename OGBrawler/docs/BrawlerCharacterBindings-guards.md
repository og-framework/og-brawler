<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerCharacterBindings.h` — guards

Every fence that stood in the header. Each entry has an **opaque, stable id**; in the header a
single line `// ⛔G-nn` sits exactly where the fence's text used to sit, on the same
declaration.

**If this file and `BrawlerCharacterBindings.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check, is
moved to [§R, Retired ids](#r-retired-ids) and its number is spent forever. Reusing one silently
re-points every reference that ever named it. A retired id may be **named** in prose or in a
`static_assert` message; it may never again appear as a `⛔G-nn` **tag**.

⚠ **The id space is PER DOCUMENT, not per tree.** `BrawlerInputPackaging-guards.md` and
`CollisionCategoryConstants-guards.md` also have a `G-01`, and that is not a collision: a tag
carries the path of the document it resolves against, and `tools/lint/guard_tag_lint.ps1` resolves
and counts per document. Never cite a bare `G-nn` without saying which header it belongs to.

⭐ **The join is machine-checked, in both directions**, by `tools/lint/guard_tag_lint.ps1`: every
tag resolves to an entry here, every live entry is referenced by exactly one tag, no id is
duplicated, and no retired id reappears. It is a hard gate. ⚠ It checks that an entry EXISTS. It
never checks that this text is TRUE.

⛔ **Nothing in this file is a rationale.** The narrative, the provenance and the derivations live
in `BrawlerCharacterBindings-rationale.md`. A guard is a prohibition plus the consequence of
ignoring it, and nothing else.

⚠ **This header is not the source of truth for any VALUE.** It holds one `BodyId` and no constant,
no unit and no wire field. Nothing below pins a number, because there is no number here to pin.

⭐ **The must-never-move list has no row here.** This header is on **no tier** — it was created by
task 62 *after* task 65's tiering pass and the census manifest missed it — so the three guards
below were **derived at task 78** from Taxonomy.md's F1–F6 procedure rather than inherited. Their
derivation, and the claims they rest on, are in the R0 audit
impl/r0_audit_BrawlerCharacterBindings.md — **the eighth audit, written at task 78 because this
file had none.**

---

## G-01 — The include list is exactly `OGSimulation/BodyId.h`, and neither half of that is an accident

**Tag site:** `BrawlerCharacterBindings.h`, immediately above `#include "OGSimulation/BodyId.h"`.
**Taxonomy clause:** F1a (the prohibition) + F6 (the misleading sibling name).
**Derived at task 78.** Not on any must-never-move tier.

**The fence, verbatim — these are the bytes it occupied in the header. It stood in TWO places, and
they are one prohibition, so they are one guard and one tag** (v2: *one tag per site; merge where
two guards land on one declaration*).

Part A, the anti-symmetry half (lines 4–7 of the R0-corrected pre-conversion file), directly above
the include:

```
// ⚠ `BodyId` comes from `OGSimulation/BodyId.h`, NOT `OGSimulation/OGTypes.h` — measured at
// task 62. (`BrawlerMovementSimulation.h`, where this struct used to live, carries an
// `OGTypes.h  // for BodyId` comment that has never been true; it got the type transitively.)
// Keep this the ONLY include this header carries — see the leaf rule below.
```

Part B, the hoisted half (lines 39–40), which stood 32 lines lower, above the namespace:

```
// ⛔ KEEP THIS A LEAF. `BodyId` is its only dependency, and that is the whole reason both
// sides can include it. Anything that needs more than `BodyId` does not belong here.
```

**What breaks if it moves.** This header exists **only** so that `DAttackMachineSimulation.h` and
`BrawlerMovementSimulation.h` can both have `CharacterBindings` without either reaching the other.
That works for exactly one reason: the file costs nothing to include. A second `#include` here is
paid by **both** sides of a dependency edge that was split apart, at task 62, precisely to stop
them paying for each other — and the header that pays is not the header where the line is typed,
so the cost is invisible at the edit. The whole-graph reachability scan that proves the split is
still intact (217 nodes from the machine header, `BrawlerMovementSimulation.h` not among them) says
nothing about how expensive the leaf has become.

⚠ **Part A is the sharper half, and it is not hypothetical.** The sibling header carries a live,
false invitation: `BrawlerMovementSimulation.h:4` reads `#include "OGSimulation/OGTypes.h"  // for
BodyId`, which **has never been true** — `git log -S BodyId -- OGSimulation/OGTypes.h` returns zero
commits, and `OGTypes.h` is twelve lines of `int32`/`uint32`/`PI`. An editor tidying this leaf to
match its sibling writes the wrong include.

⭐ **That specific edit is ALREADY A COMPILE ERROR, and this entry records it so nobody writes a
guard for it twice.** Task 78 probe **P3** swaps the include for `OGTypes.h` on an out-of-tree copy:
`error C2065: 'BodyId': undeclared identifier`, plus four more. The language enforces part A's
*consequence* without help. What it does not enforce is part A's *reason*, which is what stops the
editor adding `OGTypes.h` **alongside** `BodyId.h` and concluding both are needed.

⛔ **Part B is NOT convertible to a compile-time check, and this was measured, not reasoned.**
Probe **P4** adds `#include <vector>` and a `std::vector<BodyId>` member to an out-of-tree copy:
**exit 0, clean.** Nothing a header can assert about itself observes its own include list, and an
assertion on the struct's shape (`sizeof`, a member count) would be equivalent to one instance of
"never add a dependency" while looking enforced — v2's narrowness test fails it. **Verdict: DOES
NOT convert.**

---

## G-02 — The namespace is `simulatableBrawler`, and it is not a leftover to be tidied

**Tag site:** `BrawlerCharacterBindings.h`, immediately above `namespace simulatableBrawler`.
**Taxonomy clause:** F1c (a placement defence that names the change it defends against).
**Derived at task 78.** Not on any must-never-move tier.

**The fence, verbatim — these are the bytes it occupied in the header (lines 42–43 of the
R0-corrected pre-conversion file):**

```
// ⭐ [movement-sim task 64] THE NAMESPACE IS `simulatableBrawler`, AND THAT IS THE WHOLE TASK.
// Task 62 left it as `brawlerMovementSimulation` on purpose, so the extraction would churn zero
```

**What breaks if it moves.** Every other type in this directory that belongs to a sub-simulation
lives in that sub-simulation's namespace — `brawlerMovementSimulation`, `dAttackMachineSimulation`,
`dAttackRadialSimulation`, `dAttackGuardSimulation`, `brawlerProjectileSimulation`. This one does
not, and the file it sits next to is the movement header. So `simulatableBrawler` reads as an
oversight at exactly the place someone would "finish" task 62's extraction by renaming it to match
the neighbourhood — which is the name task 64 deliberately removed, because `brawlerMovementSimulation`
was collateral from a file move (OGBrawlerHadouken T35) and **movement is a sub-simulation that
never uses this type at all**. The reason the shared, composite-level namespace is right is in
`BrawlerCharacterBindings-rationale.md` §4; the reason a rename is cheap to make and expensive to
notice is here.

⚠ **A rename is LOUDER THAN NOTHING AND QUIETER THAN IT LOOKS.** Probe **P5**: a TU that names
`simulatableBrawler::CharacterBindings` fails (`C2653`) — but a TU that merely **includes this
header compiles clean, exit 0**. So the rename breaks call sites, not inclusion, and a mechanical
find/replace fixes every call site in the same pass.

⛔ **Not convertible to a compile-time check, and the arm that settles it is P7, not P6.**
A candidate assertion — `static_assert(std::is_same_v<::simulatableBrawler::CharacterBindings,
CharacterBindings>, …)` — looked promising: probe **P6** shows it does reach further than nothing,
failing even the include-only TU that P5 left clean. It still fails, for two measured reasons:

1. ⭐ **Probe P7 defeats it.** A namespace rename is performed as a find/replace, which rewrites the
   assertion's own qualified name along with the namespace. P7 renames both — the way an editor
   actually would — and the TU compiles **clean, exit 0**. The assertion is defeated by the standard
   form of the very edit it forbids.
2. It **hard-errors** (`C3083`, `C2039`, `C2923`) rather than firing as `static_assert failed:`, so
   its message never prints. v2's ruling F-2 requires every assertion's message to name the row and
   guard it replaces; an assertion whose message is unreachable cannot satisfy it, and deleting it
   would leave every gate green.

**Verdict: DOES NOT convert.** This is the first compile-backed statement in the phase of *why* a
SPELLING fence resists conversion: the edit's mechanical form carries the assertion with it.

---

## §R Retired ids

⭐ **One of the three derived guards was a compile-time check waiting to be written** (v2 §1.4:
*"where a fence can be converted into a compile error, do that and write no guard at all"*). The
prohibition now lives in a `static_assert` in the header, beside the struct, and that is strictly
stronger than the sentence it replaces: it cannot be skimmed past and it cannot go stale.

⛔ **This id is spent.** It may be named — it is named in the assertion's message — but it must
never again appear as a `⛔G-nn` **tag** in source.

---

### G-03 — RETIRED, converted to a `static_assert`

Must-never-move: none (this header is on no tier). **Taxonomy clause: F3 — an absence fence.** Its
subject, `SerializableFields<simulatableBrawler::CharacterBindings>`, is a symbol that **is not in
this file**; `grep` outside comments returns zero hits, so a reader cannot arrive at the fence by
looking the name up. They are about to *write* the specialization, not read about it.

**The text it replaced, verbatim (lines 58–60 of the R0-corrected pre-conversion file):**

```
// ⛔ A NAMESPACE NEEDS NO INCLUDE. This header still includes only `OGSimulation/BodyId.h`,
// still compiles standalone, and no wire byte moved: nothing specializes `SerializableFields`
// on `CharacterBindings`, so it is not on the wire at all.
```

⚠ **That span carried two clauses and only the second is this guard.** The first — *a namespace
needs no include* — is an argument about task 64's rename and is now in
`BrawlerCharacterBindings-rationale.md` §4. **The wire clause is what became the assertion.**

**Now enforced by**, in the header, immediately below `struct CharacterBindings`:

```cpp
static_assert(!Serializable<CharacterBindings>, "...");
static_assert(Serializable<BodyId>, "VACUITY CONTROL ...");
```

⭐ **The predicate already shipped.** `OGSimulation/SimulationSerialization.h:38` declares
`template <typename T> struct SerializableFields;` with **no body** — *"must be specialized"* — and
`:45` defines `concept Serializable = requires { { SerializableFields<T>::get() }; }`. The fence's
claim and the tree's own concept are the same predicate, so this conversion adds no machinery.
`BodyId.h` is already on this header's include path, and it brings the concept with it.

**Both arms were run.** Probe **P1** adds exactly the forbidden edit — a
`SerializableFields<simulatableBrawler::CharacterBindings>` specialization — to an out-of-tree copy:
`error C2338: static_assert failed: 'CANDIDATE-G03'`. The no-op arm compiles clean.

⛔ **The vacuity control is mandatory and is not decoration.** `!Serializable<T>` is true of
**every** `SimulationComposite`, the on-wire `simulatableBrawler::State` included, because only
ELEMENT types get a specialization — so the bare assertion is, for a composite, a fence that cannot
fail. `CharacterBindings` is not a composite, and `BodyId` — the type of its one member, visible in
the same translation unit, and genuinely serialized — is the contrast case that proves the predicate
discriminates. Probe **P2** asserts the control over a type with no specialization and it **fires**;
the shipped control passes. Predicate true on one side, false on the other, both measured.

⭐ **And it fires on the wrong path only.** Probe **P4** adds a `std::vector<BodyId>` member to the
struct — a shape change, not a wire change — and both assertions stay clean, **exit 0**.

⚠ **What the assertion does NOT claim.** A `SerializableFields` specialization is *necessary* for
this type to reach the wire, not sufficient: it would also have to enter a State composite. The
assertion is therefore the exact converse of the fence — *"it is not on the wire at all"* holds as
long as the specialization is absent, and the moment one appears the header stops compiling and the
question is asked out loud. That is the equivalence v2's narrowness test requires, and it is why
this one converted while the other two did not.

---

## H. Two fences that got NO TAG here, because the edit they forbid is typed in another file

⛔ v2: *"a fence whose forbidden edit is typed in ANOTHER file gets no tag here — route it, or, if a
working copy already exists at the right site, delete it."* Two of the pre-conversion header's
prohibitions are in that class. **Both working copies were verified verbatim before these were
dropped**, and neither gets an id, so the census is closed and nobody reads their absence as a
deletion.

**H-1 — the acyclicity invariant** (pre-conversion lines 22–27 and the tail of 39–40). The header
stated it as *"nothing reachable from `DAttackMachineSimulation.h` includes
`BrawlerMovementSimulation.h`, directly or through any of its other includes. That is the invariant
to protect."* The edit that violates it is an `#include` typed in one of the **other two** headers,
and **both already carry the prohibition at the line where it would be typed**:

* `BrawlerMovementSimulation.h:43-44` — *"⛔ KEEP THE GRAPH ACYCLIC: nothing reachable from
  `DAttackMachineSimulation.h` may include `BrawlerMovementSimulation.h`."*
* `DAttackMachineSimulation.h:475-477` — *"THIS HEADER MUST NEVER INCLUDE
  `BrawlerMovementSimulation.h`, directly or through any of its other includes."*

The invariant itself, the diagram and the history are in `BrawlerCharacterBindings-rationale.md`
§2–§3. ⚠ Both quoted sites are in files under concurrent edit at the time of writing (tasks 76 and
81); they are quoted **with their date**, not asserted as permanent.

**H-2 — the population contract** (pre-conversion lines 66–68): *"Populated ONCE at registration
time, in `ASimulationManagerUImpl::tryRegister`, … read after the physics-creation fold has run
(before it, that id is still zero)."* F5a/F5c — an enumerated writer set plus an ordering
precondition, and violating it is a runtime defect (a zero body id). But the stamp is typed in
`Source/OGBrawlerUnreal/SimulationManagerUImpl.cpp`, and that file carries the contract in fuller
form at `:1278-1290`, including the `#if DO_CHECK` assertions that catch a zero id. **Nothing about
this struct's declaration is where that edit happens.** The provenance is in the rationale doc §5.

⛔ **The header's copy of H-2 was FALSE, and it is corrected in the rationale rather than moved.**
It read *"from **THIS sub-simulation's** own `PhysicsDeclaration` bindings"*. This file declares no
sub-simulation, and the sub-simulation its previous sentence named — `dAttackMachineSimulation` —
has no `PhysicsDeclaration` at all. The source is the **movement** declaration, and the correct
wording is already at the stamping site (`SimulationManagerUImpl.cpp:1280`). See
impl/r0_audit_BrawlerCharacterBindings.md §2.5.

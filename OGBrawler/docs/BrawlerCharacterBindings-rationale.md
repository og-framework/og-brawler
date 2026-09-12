<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerCharacterBindings.h` — rationale

This is the narrative, the derivations and the provenance for a header that declares **one struct
with one member**: `simulatableBrawler::CharacterBindings`, the per-character handle that carries a
brawler's main physics capsule id. Under `CommentExtractionRule_v2.md` the header keeps **no prose
at all**: a licence line, a two-line pointer, code, and one `⛔G-nn` tag per guard. Every
prohibition is in `BrawlerCharacterBindings-guards.md`, each with a stable id, and **one of the
three is no longer text anywhere** — it became a `static_assert`. This file carries everything else.

**If this file and `BrawlerCharacterBindings.h` disagree, the header is authoritative and this file
is stale.** Fix this file; do not soften the header to match it.

⛔ **Do not put a fence in this file.** A prohibition belongs in
`BrawlerCharacterBindings-guards.md`, with an id and a `⛔G-nn` tag on the declaration where the
wrong line would be typed — or, better, in a `static_assert` so that it cannot be skimmed past at
all. `tools/lint/guard_tag_lint.ps1` is a hard gate on that join in both directions; a fence filed
here has no site and no lint row.

⛔ **This file is not the source of truth for any VALUE.** The header holds one `BodyId` and no
constant, no unit and no wire field.

**Read alongside:** `BrawlerCharacterBindings-guards.md` — the prohibitions, by id.

> ⚠ **Citations into `BrawlerMovementSimulation.h` name a SYMBOL, not a line.** That file is
> under active edit — 1,668 lines at the last commit, 1,889 in the working tree while this
> document was being written — so every line number below roughly line 44 of it would already be
> wrong. Two exceptions are kept as lines because the fence
> text is quoted verbatim beside them and both are in its first forty-four lines: its `OGTypes.h`
> include and its acyclicity fence.

Origin: OGBrawlerHadouken T33 (which created the struct) and T35 (which moved it), and
`brawler-movement-simulation` tasks 11, 13, 17, 62, 64 and 78.

> ⚠ **Every initiative document named below is private working material from an initiative archive
> and is NOT distributed with this submodule.** They are named as provenance, deliberately unlinked;
> every claim this file *asserts* is anchored to a file in this repository and only to those. The
> declarations below tell `tools/lint/doc_anchor_lint.ps1` that these names are intentionally
> unresolvable.

<!-- lint-external-ref: CommentExtractionRule_v2.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->

⚠ **A naming hazard, stated once.** `T34` is **ambiguous in this repository.** Around seventy
in-tree `T34` references mean `og-netcode-v2-input-relay` **T34**, the flush-on-poll rework — an
entirely different task in an entirely different initiative. **Every `T34` in this document means
OGBrawlerHadouken T34**, the bindings migration, and is written with its initiative attached.

---

## 1. What this header is, and why a one-struct file earns its own translation unit

`CharacterBindings` is a per-character handle holding one field: the body id of the character's main
physics capsule. It is **consumed by four places** — `dAttackMachineSimulation::integrate3`
(`DAttackMachineSimulation.h:483`), the `SimulatableBrawler` composite that owns it
(`SimulatableBrawler.h:42-43`, `:62`), the hit-routing system through the composite's accessor
(`BrawlerHitRoutingSystem.h:202`), and the tests — and **written in exactly one**, the registration
path in `SimulationManagerUImpl.cpp` (§5).

A file this small exists for a reason that has nothing to do with the struct: **it is a leaf, and
being a leaf is the whole product.** Two headers that must not see each other both need this type,
and the only arrangement in which both can have it is one where having it costs nothing. That is
`G-01`, and §2 is why.

⚠ **The pre-conversion header stated its own consumer set twice, in two incompatible shapes** — a
one-line label above the struct naming only the machine sub-simulation, and a four-item list thirty
lines higher. The list is the accurate one and it is the one above. This paragraph is the merge.

---

## 2. The include graph, and the TWO cycles — they are not the same cycle

Read `A -> B` as "A includes B". The arrangement this header buys:

```
BrawlerMovementSimulation.h -> DAttackMachineSimulation.h -> BrawlerCharacterBindings.h
BrawlerMovementSimulation.h -------------------------------> BrawlerCharacterBindings.h
```

and nothing points back. Measured at task 78 with a whole-graph reachability scan — `#include "…"`
and `#include <…>` both resolved, from `DAttackMachineSimulation.h`, **217 nodes visited**,
`BrawlerMovementSimulation.h` not among them. **That is the invariant to protect**, and the two
places it is enforced are named in `BrawlerCharacterBindings-guards.md` §H-1, because the edit that
would break it is typed in those files, not in this one.

### 2.1 The first cycle — OGBrawlerHadouken T33/T35, June 2026

OGBrawlerHadouken **T33** (done 2026-06-25) created `CharacterBindings` in
`SimulatableBrawlerTypes.h`, inside `simulatableBrawler`. That header includes
`DAttackMachineSimulation.h`, so the machine header could only *forward-declare* the type it needed
— and a forward declaration is not enough to read a member. T33's workaround was to make
`integrate3`'s bindings parameter a defaulted template type, CharacterBindingsT, so
`characterBindings.capsuleBodyId` became a dependent expression deferred to instantiation.

**T35** (done the same day) cut that knot by moving the struct to a new, then-minimal header,
`BrawlerMovementSimulation.h`, which did not pull in the machine header — eliminating *that* cycle,
allowing a plain `const CharacterBindings&` parameter, and, in the task's own words, *"pre-staking
the file as the eventual home of the planned character-movement sub-sim."* The namespace was renamed
to `brawlerMovementSimulation` to match the file the struct had landed in.

### 2.2 The second cycle — `brawler-movement-simulation` task 62, September 2026

⚠ **This is a different cycle, and it did not exist at T35.** The pre-conversion header said the
file move "existed to dodge an include cycle — the cycle task 62 deleted", which collapses the two
into one; the R0 audit corrected it before this text moved
(impl/r0_audit_BrawlerCharacterBindings.md §2.2).

The T35 arrangement was harmless for as long as movement was a skeleton. It stopped being harmless
when the movement sub-simulation began reading the machine's `State` (the flinch freeze) and its
`PlayerInput` (the move stick is packed onto the machine slice). Now movement needed to include the
machine header, and the machine header already included the movement header for `CharacterBindings`.
The graph became a genuine cycle — and with `#pragma once` **a cycle does not error**: it silently
leaves one side incomplete, depending on which header the translation unit entered from.
`DAttackMachineSimulation.h:471-473` records the moment: *"the 'no include cycle' that made that
safe STOPPED BEING TRUE once the movement sub-simulation began reading this header's `State` and
`PlayerInput` slices."*

The workaround, again, was templates — on **both** movement functions, so the member accesses became
dependent names. **Task 62 deleted both workarounds** and moved the struct to this leaf.

⚠ **What went is both workaround template PARAMETERS, not both templates.**
`machineFreezesMovement` is no longer a template at all — its definition in
`BrawlerMovementSimulation.h` now opens `inline bool machineFreezesMovement(const
dAttackMachineSimulation::State&)`, and `inline` became *required* the moment it stopped being a
template. `integrate` **is still a function template**, on its two adapter types
(`template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>`), which is
unrelated to this history. The
pre-conversion header said "Both templates are gone now", and a reader who checked found a
`template <…>` on `integrate` and had every reason to file the block as stale.

---

## 3. The invariant this header stated about itself — and got wrong for two days

⚠ **History, closed 2026-09-10.** Task 62 wrote this header's summary of the acyclicity invariant as
*"`DAttackMachineSimulation.h` reaches neither of the other two"*, which contradicted the diagram two
lines above it: the machine header includes **this** file, directly, at `DAttackMachineSimulation.h:17`.
Task 62's own review caught it; **task 64** rewrote it to the form in §2, which is the form already
stated in the two headers that enforce it. All three now say the same thing.

⭐ **It is worth recording why the wrong version survived review once.** The false sentence and the
diagram that contradicts it were **four lines apart in the same comment block**, and the block was
fifty-one lines long. Under v2 the diagram is in a document and the prohibition is a tag on a
declaration, so the two can no longer drift inside one wall of prose.

---

## 4. The namespace — why `simulatableBrawler` and not `brawlerMovementSimulation`

This is `G-02`'s argument. The prohibition is in the guards doc; the reasoning is here.

**The old name recorded a file, not a model.** `brawlerMovementSimulation` arrived with
OGBrawlerHadouken T35's file move (§2.1) — the struct was renamed to match the header it had
landed in. That header was chosen to dodge a cycle, so the namespace recorded **where the struct
lived**, not what it is. Task 62 left the name alone on purpose, so that the extraction would churn
zero call sites; **task 64** changed it, and that was the whole of task 64.

**Movement never uses this type.** Every `CharacterBindings` mention left in
`BrawlerMovementSimulation.h` is a comment — two in the include preamble, one on the machine
include, and three in the `` `CharacterBindings` MOVED OUT of this header `` block; the only
non-comment hit is the `#include "OGBrawler/BrawlerCharacterBindings.h"` line itself, which names
this file, not the type. Movement takes its capsule id from its own `RuntimeBindings`, declared
there as `using RuntimeBindings = PhysicsRuntimeBindings;`.

⚠ **But movement is not unique in that, and the pre-conversion header claimed it was.** It read
*"MOVEMENT IS THE ONE SUB-SIM THAT NEVER USES THIS TYPE"*. **Four of the five sub-simulations never
use it**: `CharacterBindings` returns zero hits in `DAttackRadialSimulation.h`,
`DAttackGuardSimulation.h` and `BrawlerProjectileSimulation.h` too — which the same header said
thirty lines lower, in §6's FUTURE note. Only `dAttackMachineSimulation` consumes it. The point was
never exclusivity; it is that the sub-simulation the namespace was **named after** is one of the
four that never touch it.

**And no single sub-simulation's namespace fits the consumers.** The pre-conversion header argued
that the real consumers *"all sit in or under `simulatableBrawler`"*. ⛔ **Not one of them does**,
and the true picture is the stronger argument:

| consumer | namespace |
|---|---|
| `integrate3` | `dAttackMachineSimulation` (`DAttackMachineSimulation.h:63`) |
| the `SimulatableBrawler` composite | **the global namespace** — `SimulatableBrawler.h` declares none, which is why `:62` must spell `simulatableBrawler::CharacterBindings` |
| the hit-routing read | `brawlerHitRouting` (`BrawlerHitRoutingSystem.h:55`, use at `:202`) |
| the tests | global scope |

They are spread across three namespaces and the global scope. **No one sub-simulation's namespace
contains them**, which is exactly why the type belongs in the shared, composite-level
`simulatableBrawler` — where OGBrawlerHadouken T33 originally declared it
(`SimulatableBrawlerTypes.h`, before T35 moved it out) and where the rest of the composite-level
vocabulary lives: `SimulatableBrawlerTypes.h:50`, `BrawlerInputPackaging.h:13`,
`BrawlerMotionMatching.h:82`, `BrawlerVisualizationInputSource.h:65`.

⚠ **A sentence in `BrawlerMovementSimulation.h` still carries the false form of this
argument** (*"every real consumer sits in or under `simulatableBrawler`"*), as does
`SimulatableBrawler.h:41` for the exclusivity claim. Both are **routed, not fixed** — task 78 owns
only this header and these two documents. See impl/r0_audit_BrawlerCharacterBindings.md §4,
findings R-1 and R-2.

**And the rename cost nothing.** A namespace needs no include: this header still carries exactly
one, still compiles standalone (verified at task 78 by compiling a scratch translation unit whose
only include is this header, `/std:c++20`, og-brawler and og-simulation include roots, **no UE
headers** — exit 0; the same unit without the include fails with `C2653`), and **no wire byte
moved** — which is now the `static_assert` retired as `G-03`.

---

## 5. Where the value comes from — registration, and the T13 change of provenance

`capsuleBodyId` is populated **once**, at registration time, in
`ASimulationManagerUImpl::tryRegister` (`SimulationManagerUImpl.cpp:1188`; the single
`setCharacterBindings` call is at `:1309`, on the first-call branch). Its source is the **movement**
sub-simulation's own `PhysicsDeclaration` bindings — `bindings.ownBodyId`, read **after** the
physics-creation fold has run, because before the fold that id is still zero. The contract is stated
in full at the stamping site (`:1278-1290`) and is not tagged here; see
`BrawlerCharacterBindings-guards.md` §H-2 for why.

⛔ **The pre-conversion header said "THIS sub-simulation's own `PhysicsDeclaration`", and that was
false.** This file declares no sub-simulation, and the sub-simulation named in the sentence
immediately before it — `dAttackMachineSimulation` — has **no `PhysicsDeclaration` of any kind**
(the four that do are radial, guard, projectile and movement; `SimulatableBrawler.h:63-69`). The
correct wording has been at the stamping site all along: *"the movement sub-simulation's OWN
PhysicsDeclaration bindings"* (`SimulationManagerUImpl.cpp:1280`). This header inherited the phrase
from `BrawlerMovementSimulation.h`, where "this sub-simulation" **was** correct, and kept it through
the move.

### 5.1 The cutover — `brawler-movement-simulation` task 13, and why the value never changed

⚠ **History, closed at task 13.** The source used to be a UE-side capsule lookup —
ACharacter::GetCapsuleComponent — rather than the movement declaration's own bindings. It is not
any more.

**The VALUE never moved across that cutover**, and that was the design: task 11's descriptor sets
`isRoot` (`BrawlerMovementSimulation.h`, `.isRoot = true,` in `PhysicsSetup`), so the factory **adopts** the
character's existing root capsule instead of creating a body, which makes
`bindings.ownBodyId == bindings.parentBodyId == capsuleBodyId` true **by construction**. It was a
change of PROVENANCE only. The step-by-step design is architecture_movement_sim.md §4.2, *"The
cutover, step by step — designed so the value never changes"* (initiative archive, not in this
repository); the same argument is restated in the tree at `SimulationManagerUImpl.cpp:1286-1290`.

⚠ ACharacter no longer appears in this tree at all — the pawn was later reparented from
ACharacter to `APawn` and the character movement component deleted. The sentence above is closed
history about task 13, not a description of a class you can grep for.

### 5.2 The two-source tripwire — `brawler-movement-simulation` task 17

⚠ **History, closed at task 17.** While two independent sources for the capsule id coexisted, an
assertion watched them agree. Task 17 deleted the tripwire **and the second source with it**:
PendingRegistration::parentBodyId is gone — `SimulationManagerUImpl.h:813-819` now declares four
members and none of them is it, and `:808-812` records the removal.

**The identity is still asserted, one layer down**, by a check neither task touched:
`ChaosPhysicsFactory::createPhysicalObject`'s adopt-root arm ends in
`checkf(bodyId == m_parentBodyId, …)` (`ChaosPhysicsFactory.cpp:195`), and its `m_parentBodyId` is
derived from the same capsule component the registration path passes as the attach parent. So the
removal dropped a duplicate, not the only witness. ⚠ Neither was ever a Shipping-build guarantee —
`checkf` compiles out there.

---

## 6. The unfiled OGBrawlerHadouken T34 migration, and why its case is weaker than it looks

**The fact is still true.** Radial (`DAttackRadialSimulation.h:680`), guard
(`DAttackGuardSimulation.h:257`) and projectile (`BrawlerProjectileSimulation.h:474`) still read a
bare `bindings.parentBodyId`, while the machine sub-simulation takes a `CharacterBindings`. Folding
a bindings field into every sub-simulation's `RuntimeBindings` would remove that duplication.

⚠ **Three raw line numbers in one sentence** — the most decay-prone construction this header
carried. They were re-verified exactly at task 78 (2026-09-11); check them before relying on them.

**But there is no initiative to point at, and the pre-conversion header used to claim there was.**
The only specification is one prose paragraph, in the OGBrawlerHadouken initiative's backlog at
`:965`, literally headed *"T34 (if filed later)"* — and it never was filed. That backlog has **no
`### 34.` entry**; several of its other mentions tag the work *parked* (the section heading at
`:962` reads *"Open architectural note (parked, not in T33 scope) — T34"*); and the host initiative
is itself parked, at *"PHASE 2 COMPLETE — AWAITING USER APPROVAL FOR PHASE 3"* — which is stated in
that initiative's current_state.md, not its backlog. It is an **UNFILED, PARKED** task, not
scheduled work. `SimulatableBrawlerTypes.h:39` says the same thing in the tree.

⚠ The pre-conversion header said *"every other mention tags it 'T34 parked'"*. **Three of eight
do.** The conclusion is unaffected — a section heading that calls the whole note parked is better
evidence than a tally of the word — but the universal was not true.

**And its premise has weakened.** Both of the facts that weaken it are in §5.2 and §5.1 above:
PendingRegistration::parentBodyId is gone, and the two-source tripwire with it; and
`ownBodyId == parentBodyId == capsuleBodyId` now holds **by construction**, asserted one layer down
by `ChaosPhysicsFactory`'s adopt-root `checkf`. OGBrawlerHadouken T34 was written to de-duplicate
two genuinely independent sources. What it would buy today is naming consistency across the **four**
sub-simulations that carry a `using RuntimeBindings = PhysicsRuntimeBindings;` — movement,
projectile, guard and radial — for **~100+ LOC** and no behaviour change. A materially weaker case.

⚠ **The pre-conversion header pointed at these two facts as "the two paragraphs directly above",
and they were the third and fourth.** The two directly above were the FUTURE note and the
unfiled-task note, neither of which states either fact. ⭐ **That defect is the clearest per-file
argument for v2 this conversion produced:** a positional prose pointer cannot survive an edit, cannot
be linted, and was already wrong. As a `§` reference it is both.

---

## 7. What the R0 audit corrected before any of this moved

This header had **no R0 audit** — it was created by task 62 *after* task 65's seven audits were
written, and the census manifest missed it. Task 78 wrote the eighth
(impl/r0_audit_BrawlerCharacterBindings.md) as step 1, because moving a false sentence into a
document **launders** it.

**Seven of thirty-eight claims were false**, and ⭐ **five of the seven were written by this
initiative's own tasks 62 and 64, within the preceding two days.** Recent is not the same as true.

| corrected | was | is |
|---|---|---|
| §2.2 | "Both templates are gone now" | both workaround template **parameters** are; `integrate` is still a template |
| §2.2 | the T35 move dodged "the cycle task 62 deleted" | a **different** cycle, four months earlier |
| §4 | "MOVEMENT IS THE ONE SUB-SIM THAT NEVER USES THIS TYPE" | four of five never use it |
| §4 | the consumers "all sit in or under `simulatableBrawler`" | **none** of them is; they span three namespaces and the global scope |
| §5 | "THIS sub-simulation's own `PhysicsDeclaration`" | the **movement** sub-simulation's |
| §6 | "every other mention tags it 'T34 parked'" | three of eight |
| §6 | "both facts are in the two paragraphs directly above" | the third and fourth — now §5.1 and §5.2 |

⛔ **Two of the seven have twins in files task 78 does not own** and are **routed, not fixed**:
`BrawlerMovementSimulation.h`'s *"every real consumer sits in or under `simulatableBrawler`"* and
`SimulatableBrawler.h:41`. A third routed finding,
`SimulatableBrawler.h:35-36`, still states the **pre-task-13** provenance. See the audit's §4.

---

## 8. The one-line label that is no longer in the header

The member carried a trailing comment: `BodyId capsuleBodyId;  // character's main physics capsule`.
It was true — the adopted root capsule *is* the character's main physics body — and it is removed
under `CommentExtractionRule_v2.md` §1.4 item 5, by user ruling of 2026-09-11: **zero prose**, and
the loss of trailing labels is **accepted, not overlooked**. The content is this sentence.

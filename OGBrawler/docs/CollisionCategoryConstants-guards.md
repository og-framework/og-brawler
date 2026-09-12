<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `CollisionCategoryConstants.h` — guards

Every fence that stood in the header. Each entry has an **opaque, stable id**; in the header a
single line `// ⛔G-nn` sits exactly where the fence's text used to sit, on the same
declaration.

**If this file and `CollisionCategoryConstants.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check,
is moved to [§R, Retired ids](#r-retired-ids) and its number is spent forever. Reusing one
silently re-points every reference that ever named it. A retired id may be **named** in prose or
in a `static_assert` message; it may never again appear as a `⛔G-nn` **tag**.

⭐ **The join is machine-checked, in both directions**, by
`tools/lint/guard_tag_lint.ps1`: every tag resolves to an entry here, every live entry is
referenced by exactly one tag, no id is duplicated, and no retired id reappears as a tag. It is a
hard gate. ⚠ It checks that an entry EXISTS. It never checks that this text is TRUE.

⛔ **Nothing in this file is a rationale.** The narrative, the provenance and the derivations
live in `CollisionCategoryConstants-rationale.md`. A guard is a prohibition plus the consequence
of ignoring it, and nothing else.

⚠ **This file is not the source of truth for any VALUE.** The category ids and the masks live
in the header; the engine channels live in `SimulationManagerUImpl.cpp` and
`Config/DefaultEngine.ini`. Where a value appears below it is there to make an argument readable.

---

## G-01 — Adding a category is an edit at the composition root, not here

**Tag site:** `CollisionCategoryConstants.h`, immediately above `namespace collisionCategory`.
**Taxonomy clause:** —.

**The fence, verbatim — these are the bytes it occupied in the header (lines 12-13 of the pre-conversion file):**

```
// ⛔ ADDING A CATEGORY IS AN EDIT THERE, NOT HERE — and `BeginPlay` builds that table TWICE,
// one branch each; that site's own `⚠ KEEP IN SYNC` note is the one to read before typing.
```

**What breaks if it moves.** **Written for this entry (task 67 guard, not on the must-never-move list).** The category constants are here; the engine mapping is in another module entirely, in a table that exists **twice** (`SimulationManagerUImpl.cpp`, the authority branch and the client branch). A category added here and nowhere else compiles, links, ships, and produces a query that matches nothing — `toEngineChannel` answers `ECollisionChannel(0)` for it, which is a legitimate channel (see G-03). The list of constants is the only place an editor looks.

---

## G-02 — There is no wall probe

**Tag site:** `CollisionCategoryConstants.h`, immediately above `constexpr uint32_t world = 4;`.
**Taxonomy clause:** —.

**The fence, verbatim — these are the bytes it occupied in the header (lines 32-33 of the pre-conversion file):**

```
	// ⚠ THERE IS NO WALL PROBE. Wall blocking is the SOLVER's, through the body shape's
	// `worldAndCharacter` mask — not a query and not a second volume.
```

**What breaks if it moves.** **Written for this entry (task 67 guard, not on the must-never-move list).** An absence fence. `PhysicsSetup::queryVolumes` returns exactly one descriptor — the shrunken ground probe. Nothing in the code says a wall probe was considered and rejected, so a reader debugging wall behaviour looks for the probe that is not there, and the reflex fix is to add a second volume — a second sweep every tick, re-deriving badly what the solver already does through the body shape's blocking mask. The phrase *"wall probe"* is kept in the text on purpose, as a grep handle, and then negated.

---

## G-03 — The adapter's return value cannot tell you `world` is mapped

**Tag site:** `CollisionCategoryConstants.h`, immediately above `constexpr uint32_t world = 4;`.
**Taxonomy clause:** F6 + F5c (must-never-move **T2a-1**).

**The fence, verbatim — these are the bytes it occupied in the header (lines 38-40 of the pre-conversion file):**

```
	// ⚠ ECC_WorldStatic is ECollisionChannel(0), which is ALSO the unmapped fallback,
	// so the return value cannot tell you the mapping is there — only the table's size
	// can. That caveat is spelled out in full at the authority table.
```

**What breaks if it moves.** The adapter's return value **cannot distinguish** "mapped to `WorldStatic`" from "never mapped" — both are `ECollisionChannel(0)`. An editor verifying a new category reads back `ECC_WorldStatic`, concludes it is wired, and ships a category whose bit `toObjectQueryParams` never tests: a well-formed query that matches **nothing**. The only correct check is `m_toEngine.size()`, and nothing at the call site says so.

---

## G-05 — Why the movement capsule is not `body`

**Tag site:** `CollisionCategoryConstants.h`, immediately above `constexpr uint32_t character = 5;`.
**Taxonomy clause:** F2 (must-never-move **T2a-3**).

**The fence, verbatim — these are the bytes it occupied in the header (lines 61-72 of the pre-conversion file):**

```
	// [movement-sim T6] WHY THE CAPSULE IS NOT `body`:
	// `body` is the radial sim's 30 cm hurtbox sphere, searched by the
	// `bodyAndGuard` / `bodyGuardProjectile` masks below. If the movement capsule
	// were registered as `body`, every radial swing and every projectile would
	// additionally overlap the capsule and emit a SECOND hit carrying the same
	// `rootBodyId` — harmless for routing (the flinch flag is idempotent), but it
	// changes the contents of `attackHits[]` and the block-vs-hit classification in the
	// projectile sim, which — after the parent-body filter and the T13 projectile-
	// cancellation branch — tests `guard` and treats everything else as
	// damage. A separate `character` category keeps the capsule invisible to every
	// existing query mask — which is also why neither mask added below is folded
	// into an existing one.
```

**What breaks if it moves.** Names the rejected alternative (register the capsule as `body`) and its consequence (a SECOND hit per swing carrying the same `rootBodyId`, changing `attackHits[]` and the block-vs-hit classification). At the site it stops the merge; in a document it is read *after* the merge.

---

## G-06 — Only ch1 and ch6 are declared in the ini

**Tag site:** `CollisionCategoryConstants.h`, immediately above `constexpr uint32_t character = 5;`.
**Taxonomy clause:** —.

**The fence, verbatim — these are the bytes it occupied in the header (lines 77-78 of the pre-conversion file):**

```
	// ⚠ ONLY ch1 AND ch6 ARE DECLARED IN DefaultEngine.ini. ch2-5 are occupied by the two
	// adapter tables and carry no ini row; an audit that reads only the ini reports them free.
```

**What breaks if it moves.** **Written for this entry (task 67 guard, not on the must-never-move list).** The ini is the file an audit reads to answer "which trace channels are free?", and it lists two rows. Channels 2-5 are occupied by the two adapter tables and leave no trace in it at all, so the honest reading of the ini is off by four. The next person to claim a channel reads the ini, sees ch2 free, and collides with `body`.

---

## §R Retired ids

⭐ **Both of these were fences that became COMPILE ERRORS** (v2 §1.4: *"where a fence can
be converted into a compile error, do that and write no guard at all"*). The prohibition now
lives in a `static_assert` message in the header, on the declaration where the forbidden edit
would be typed. That is strictly stronger than the sentence it replaces: it cannot be skimmed
past and it cannot go stale.

⛔ **These ids are spent.** They may be named — they are named in the two `static_assert`
messages — but they must never again appear as a `⛔G-nn` tag in source.

### G-04 — RETIRED, converted to a `static_assert`

Was must-never-move **T2a-2** (F1c + F6): *"the surviving half of a sentence whose other three
claims were retired. Without it, `character`'s invisibility to the attack masks reads as a
leftover of the old 'deliberately UNMAPPED' story rather than a live property — and the next
editor 'completes' the mapping by folding `character` into `bodyAndGuard`, putting the movement
capsule in every radial swing."*

**The text it replaced, verbatim (lines 57-59 of the pre-conversion file):**

```
	// What has NOT changed is the half that made the old sentence worth writing: no attack
	// or projectile query searches this category, so a `character` shape is still invisible
	// to every mask below. That is now a property of the MASKS, not of a missing mapping.
```

**Now enforced by**, in the header, immediately below `bodyGuardProjectile`:

```cpp
static_assert(!bodyAndGuard.contains(character) && !bodyGuardProjectile.contains(character), ...);
```

The exact edit the sentence forbade — folding `character` into the attack masks — is now a
translation failure in every translation unit that includes this header.

---

### G-07 — RETIRED, converted to a `static_assert`

Was must-never-move **T2a-4** (F1b + F1d): *"two mask constants sitting beside two
near-identical ones is exactly the shape a tidy editor consolidates. The prohibition only works
on the lines where the fold would be typed."*

**The text it replaced, verbatim (lines 90-91 of the pre-conversion file):**

```
	// [movement-sim T6] Movement sub-sim sweep masks. Deliberately NOT folded into
	// `bodyAndGuard` or `bodyGuardProjectile` above — see the `character` note.
```

**Now enforced by**, in the header, immediately below `worldAndCharacter`: an assertion that the
movement sweep masks and the attack masks share **no bit at all**, plus the two subset relations
that argument rests on — `bodyAndGuard` inside `bodyGuardProjectile`, `worldOnly` inside
`worldAndCharacter`. Because the two wide masks are disjoint and each narrow mask is contained in
its wide one, **any** merge across the pair boundary puts an attack bit into a movement mask or a
movement bit into an attack mask, and the assertion fires.

⚠ An earlier draft of this check named six specific clauses instead, and left `projectile` in a
movement mask uncovered. The gap was found by reading this entry back against the code, and
closed by widening the check rather than by narrowing the claim.

⚠ **What the check does NOT cover, stated rather than hidden.** Deleting `worldOnly` and
writing `CollisionCategories::single(world)` inline at the probe is not a fold and does not fire
the assert. That edit is harmless — it changes no mask — which is why the assert is pointed at
the fold and not at the constants' existence.


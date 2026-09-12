<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `CollisionCategoryConstants.h` — rationale

The narrative, the provenance and the derivations for the six DAttack-local collision category
ids and the four pre-built masks. The header carries the code and nothing else; the fences live
in `CollisionCategoryConstants-guards.md`; everything below is the *why*.

**If this file and `CollisionCategoryConstants.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **Do not move a guard into this file.** Every `⛔G-nn` tag in the header points at
`CollisionCategoryConstants-guards.md`, and that separation is checked by
`tools/lint/guard_tag_lint.ps1`. A prohibition filed as narrative is read after the edit, not
before it.

⛔ **This file is not the source of truth for any VALUE.** The ids and masks are in the
header; the engine channel mapping is in `SimulationManagerUImpl.cpp`; the channel declarations
are in `Config/DefaultEngine.ini`. Where a value appears below it is there to make an argument
readable.

**Read alongside:** `QueryGeometry.h` — `CollisionCategories` itself, and the statement that a
category id is not an engine channel number. `ChaosSpatialQueryAdapter.cpp` — the authority on
what an unmapped category does to a query, including the `static_assert` truth table.

Origin: `brawler-movement-simulation` tasks T1, T6, 11, 17, 39, 43, 53, 67 and 72; `hit-resolution` T3, T11 and T13; user rulings #5 (2026-09-03) and #6 (2026-09-04).

> ⚠ **The initiative names in that Origin line are private working material from initiative
> archives and are NOT distributed with this submodule.** They are named as provenance,
> deliberately unlinked; every claim this file *asserts* is anchored to a file in this
> repository and only to those.

---

## 1. What a collision category is, and where the engine mapping lives

Collision category assignments — DAttack-local sequential IDs. These are NOT engine channel
numbers. The QUERY adapter is handed an explicit mapping from these IDs to the engine's native
object type — `ChaosCategoryMapping`, built at the composition root in
`SimulationManagerUImpl.cpp`'s `BeginPlay`. The body adapters (Chaos, Jolt) never see a category
at all.

⭐ The guard that sits on this fact in the header is **G-01**.

## 2. The six categories

| id | constant | what it is |
|---:|---|---|
| 0 | `body` | character hurtbox (only) |
| 1 | `guard` | guard shield |
| 2 | `queryRouting` | trace-channel routing for query volumes |
| 3 | `projectile` | in-flight projectile body |
| 4 | `world` | static level geometry (engine: WorldStatic object type) |
| 5 | `character` | — the header carried no trailing label here; see §5 |

⚠ Those one-line descriptions were trailing comments in the header until task 72, and they are
the clearest single casualty of the zero-prose rule. Measured: five of them, 15 to 58 characters
each, mean 34.0 — and each is now a document away from the constant it named.

## 3. `projectile` (3) — why it is its own category

[hit-resolution T13] In-flight projectile body. Own category so a projectile overlap query can
distinguish "hit another projectile" from "hit a character body/guard".

Projectile-vs-projectile cancels both with no HitFlinch on either owning character; body/guard
takes the existing hit / guard-block behavior.

### 3.1 History

⭐ HISTORY: before this category existed, projectiles registered under `body` and a
projectile-vs-projectile hit routed a HitFlinch to the opposing owner (T3/T11 `rootBodyId`).

## 4. `world` (4) — static level geometry

[movement-sim T6] Static level geometry. No DAttack-authored shape belongs to it, and the only
QUERY that searches it is the ground (attachment) probe (`PhysicsSetup::queryVolumes`).

⭐ The absence fence on this declaration is **G-02**: there is no wall probe, and the header
says so at the site.

### 4.1 The mapping exists

⭐ [movement-sim task 17] THE MAPPING EXISTS: task 39 mapped `world -> ECC_WorldStatic` at BOTH
ChaosCategoryMapping tables in SimulationManagerUImpl.cpp's BeginPlay.

⚠ And it cannot be verified by reading the mapping back — that is **G-03**, the strongest
guard on this header. `ChaosSpatialQueryAdapter.cpp` is the authority: its `ambiguousGapMask`
and its `static_assert` truth table both exist for this one ambiguity.

## 5. `character` (5) — the movement capsule

[movement-sim T1] the character movement sub-sim's body; NOT the hurtbox.

### 5.1 The category is live — all three halves landed

⭐ [movement-sim task 17] THIS CATEGORY IS LIVE, AND THE THREE SENTENCES THAT SAID OTHERWISE ARE
GONE. All three halves of that old claim have landed:

* task 43 MAPPED it — `character -> ECC_GameTraceChannel6` at both ChaosCategoryMapping tables in SimulationManagerUImpl.cpp's BeginPlay;
* task 53 DECLARED the channel — one `+DefaultChannelResponses=(...)` row in Config/DefaultEngine.ini: Channel=ECC_GameTraceChannel6, DefaultResponse=ECR_Block, Name="BrawlerCharacter";
* task 11 took the movement sim LIVE — its PhysicsDeclaration ships in the SimulatableBrawler composite and runs for every character in every session.

### 5.2 What the old text said, and why the quotation is kept

The old text said `character` was "deliberately UNMAPPED in the engine adapter until the movement
sim goes live" and that the channel note below was "documentation only".

⚠ That sentence is retired, and it is quoted here rather than deleted because **G-04's**
replacement `static_assert` message and the header's own history both refer to *"the old
sentence"*. Delete the quotation and those references dangle.

### 5.3 Invisibility to the attack masks is now a property of the MASKS

The half of the old sentence that survived is no longer prose at all. It was fence **T2a-2**,
it became guard **G-04**, and G-04 is now a `static_assert` — see
`CollisionCategoryConstants-guards.md` §R.

### 5.4 Why the capsule is not `body`

⛔ This derivation is **not here**. It is the consequence half of guard **G-05** and it lives
in `CollisionCategoryConstants-guards.md`, because it is what stops the merge rather than what
explains it.

### 5.5 The engine channel

Engine channel — user ruling #6, closed 2026-09-04: `ECC_GameTraceChannel6`, free at the time. ch1
is `Damageable`; ch2-5 are body / guard / queryRouting / projectile.

⚠ The ini tells only half of that story — see guard **G-06**.

## 6. The pre-built masks

Pre-built masks for common query patterns.

**`bodyAndGuard`** — `body` | `guard`. The radial attack query mask. Three non-comment consumer
sites in two files: `DAttackRadialSimulation.h` twice, and `SimmableUpdateComponent.cpp` once on
the UE side.

**`bodyGuardProjectile`** — Projectile-sim query mask: character hurtbox + guard shield + other projectiles (so projectiles
detect each other and both cancel — see T13 branch in the projectile sim's hit loop).

**`worldOnly`** — Ground (attachment) probe: static level geometry only.

**`worldAndCharacter`** — The movement capsule's blocking set (user ruling #5, closed 2026-09-03: brawler-vs-brawler is
BLOCK — the solver separates, mass-ratio 50/50 and the ride-up risk accepted, authored pushbox
parked). ⭐ [movement-sim task 17] THE CONSUMER EXISTS: task 11's
`brawlerMovementSimulation::PhysicsSetup::body` shape descriptor carries this mask.

⛔ The two movement masks are **deliberately not folded** into the two attack masks. That was
fence **T2a-4** and guard **G-07**; it is now a `static_assert` on mask disjointness — see
`CollisionCategoryConstants-guards.md` §R.

## 7. What this header used to look like

Before task 72 this was a 102-line file of which **80 lines were comment-only** and 13 were
code — 78.4 %. Task 67 corrected six false statements in it and the file *grew*, which is the
result that produced the zero-prose rule. Every sentence above stood in the header until
2026-09-11.

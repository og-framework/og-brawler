<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerHitRoutingSystem.h` — rationale

<!-- lint-external-ref: BrawlerHitRouter.h -- RETIRED by the OGSim system-API initiative (T10): the free-function routing pass this system replaced. It must NOT resolve -->
<!-- lint-external-ref: routeInboundHitsAll -- RETIRED with BrawlerHitRouter.h. It must NOT resolve -->
<!-- lint-external-ref: hasHitGuard -- RETIRED by og-netcode-v2-field-defects task 9: the radial State's wire flag branch 5 replaced. It must NOT resolve -->
<!-- lint-external-ref: ASimulationManagerUImpl::m_byRootBodyId -- RETIRED by the OGSim system-API initiative (T8): the adapter-owned map this system's map replaced. It must NOT resolve -->

The law, the five branches, the provenance and the history of the hit-routing system. The
**prohibitions** are in `BrawlerHitRoutingSystem-guards.md`, and nothing here is a fence except §6,
which is observed behaviour recorded at the user's request, not a rule.

**Provenance.** Converted by og-netcode-v2-field-defects task 29 (2026-09-26). The header, as
committed at og-brawler `967a958`, was 442 lines, md5 `db7ce95d…`, and carried 204 comment lines in
25 runs (the SPDX line aside), plus one inline `/* BodyId.value */` label. All of them except the
namespace closer are kept VERBATIM in [§A](#a-the-removed-comments-verbatim), each tagged
`<!-- pristine lines A-B -->` with the section its content now lives in. **Where a section and §A
disagree, the section wins.** §A is the shipped text, including the parts task 29 found false (§8).

⚠ **This file is not the source of truth for any VALUE.** Where it quotes a number, the code and the
tests win.

⚠ **Pending with the user.** `brawler-movement-simulation` has an open question about a melee and a
projectile hit on one target in one pass (§6). Its answer would edit branches 2 and 3.

---

## 1. What the system is

`brawlerHitRouting::System` is the cross-character half of combat: it turns what
`brawlerHitDetection::System` detected into each character's inbound-hit slice
(`brawlerInboundHit::DerivedState`, on the off-wire `DerivedState` composite). The struck character's
machine reads that slice to enter `HitFlinch`, its movement sub-simulation reads it for the
knockback, and an attacker's machine reads it to recoil into `GuardFlinch` from a blocked swing or a
blocked projectile. It began as a free function, `routeInboundHitsAll` in `BrawlerHitRouter.h`
(hit-resolution T16), which the engine adapter called with a map it owned. It became this system
when the OGSim system API landed (og-brawler `361b274`), and the map moved into it (§7).

**When it runs.** In `preIntegrate` of tick T+1, after `integrate(T)` and physics step T and before
`integrate(T+1)`. og-simulation fires `preIntegrate` before `integrateAll` on the prediction,
authority and resim step functions alike (`SimulationManager.h`). The slice is produced and consumed
inside T+1, so a resim replay of T+1 recomputes it from the restored end-of-T state. Until
og-netcode-v2-field-defects task 20 it was a post-integrate pass of T consumed by T+1, which a replay
starting at T+1 never re-ran (`BrawlerHitDetectionSystem-rationale.md` §7).

**Shape.** It satisfies og-simulation's `SimulationSystem<T, StaticDataT>` concept with
`StaticDataT = simulatableBrawler::StaticData`: a `RequiredSimulatables` list (the executor projects
the storage down to it before each hook), a `kRoleAffinity`, and four hooks. `postIntegrate` is
empty and exists because the concept requires all four. `kRoleAffinity` is `AllRoles` for the same
reason as the detector's (`BrawlerHitDetectionSystem-guards.md` G-09): clients predict the reactions
and a resim replay re-routes through this pass. It has no guard of its own because the header never
carried one; an `AuthorityOnly` routing would silently route nothing on a client.

**Layer.** og-brawler core: engine-agnostic, game-specific, no UE or Godot symbols. The OGSim
primitives (`SimulatableList`, `StorageView`, `SimulationTimeStep`) are named unqualified because
the whole OGSim core lives in the global namespace (lead decision D12 of the OGSim system-API
initiative; that design's `ogsim::` prefix was schematic). The header does not include
`OGSimulation/SystemsExecutor.h`, and needs no `OGSIM_OPTIMIZE_OFF` / `OGSIM_OPTIMIZE_ON` pair (§8,
row 2).

---

## 2. Held by the compiler, so neither a comment nor a guard

Each measured by compiling a probe TU against a shadow copy of the header, under the standalone flags
and the `OGBrawlerTests` LLT flags.

| property | what holds it | the edit, and the error |
|---|---|---|
| the four hooks (the pristine "present to satisfy the four-hook `SimulationSystem` concept") | the `SimulationSystemsExecutor` specialization's requires-clause, instantiated by the manager's `BrawlerSystemsExec` and by every rig | delete `postIntegrate` → `C2027`, use of the undefined primary template |
| detection fires BEFORE routing in the same `preIntegrate` (branches 2-5 read what the detector wrote in this pass) | `static_assert(brawlerHitDetection::firesBefore<BrawlerSystemsExec, …>)` in `SimulationManagerUImpl.h`, and `HitDetection.FiresBeforeTraitSeesTheExecutorOrder` | a reversed `BrawlerSystemsExec` fails that assert; seen to fail by task 9 (`BrawlerHitDetectionSystem-rationale.md` §5) |
| the slice stays off the wire | `SimulationDerivedComposite`'s `requires (!(Serializable<Ts> \|\| ...))`; moving it onto the `State` composite fails at the correction gate's `isSimilarTo` | a `SerializableFields` specialization → `C7602`; the slice on `simulatableBrawler::State` → `C7500` (`BrawlerInboundHit-rationale.md` §3) |
| branch 2 iterating the per-swing ledger in place of `hitsThisTick`, literally | the element types: a ledger entry is a `SimCharacterId` | `C2228`, left of `.hitRootBodyId` must have class type. A rewrite would compile, so guards G-04 stands |

⚠ The concept's other requirements (`RequiredSimulatables`, a valid `kRoleAffinity`) are held the same
way; they were never comments here.

---

## 3. The pass: five branches, in this order

`preIntegrate` collects the storage view into `ordered`, sorts it (§5, guards G-02), then runs five
loops over it. **These are the branch numbers every other document cites** ("routing branch 5"):

| branch | what it reads (written by the detector earlier in this pass) | what it writes | lookup | self-hit filter |
|---|---|---|---|---|
| 1 | nothing | every character's slice := `brawlerInboundHit::DerivedState{}` (guards G-03) | — | — |
| 2 | each attacker's radial `DerivedState::hitsThisTick` (guards G-04) | the target's slice via `resolveHitReaction` | `m_byRootBodyId[hit.hitRootBodyId]` | yes |
| 3 | each shooter's projectile `DerivedState::detectedThisTick`, outcome `SlotOutcome::Hit` | the target's slice via `resolveHitReaction` | `m_byRootBodyId[detected.struckRootBodyId]` | yes |
| 4 | the same, outcome `SlotOutcome::BlockedByGuard` | the shooter's own `wasProjectileBlockedThisTick` | none (self) | no |
| 5 | each attacker's radial `DerivedState::guardBlockedThisTick` | the attacker's own `wasGuardBlockedThisTick` (guards G-07) | none (self) | no |

**The self-hit filter** on branches 2 and 3 compares the resolved `SimulatableBrawler*` with the
attacker's (the pristine "D5 … pointer identity, not rootBodyId"). ⚠ R0: comparing root body ids
instead would be equivalent today. The map holds one key per character, its capsule id, so the
resolved pointer is the attacker's exactly when the hit's root id is the attacker's capsule id.

**What each branch's consumer does** (`BrawlerInboundHit-rationale.md` §2 for the full table): the
machine enters `HitFlinch` from any state on `wasHitThisTick`, `GuardFlinch` from any state but
`GuardFlinch` on `wasProjectileBlockedThisTick`, and `GuardFlinch` from `Attacking` only on
`wasGuardBlockedThisTick`. The movement sub-simulation launches on `wasHitThisTick` with
`hitDirectionXY * knockbackSpeed`.

### 3.1 Branch 1 — the whole-slice reset (movement-sim task 27)

Routing owns the slice's reset and set, so every tick's signal is fresh and the machine never sees
a stale flag. Movement-sim task 27 added the resolved reaction (kind, speed, dwell, direction) beside
the bools and made the reset a whole-slice assignment, because a reset that names fields misses the
next field added. ⚠ R0: the pristine says "beside the two bools"; there have been three since
og-netcode-v2-field-defects task 9 added `wasGuardBlockedThisTick`.

### 3.2 Branch 2 — radial hits (T3, movement-sim tasks 83 and 86)

Each entry of an attacker's `hitsThisTick` names the struck character by its root body id (the
capsule, `SpatialQueryHit::rootBodyId`) and carries the swing tangent at the hit. Since
og-netcode-v2-field-defects task 27 it also carries `targetId`, the peer-stable `SimCharacterId`
the detector resolved through its own map; routing does not read it and resolves the root id through
`m_byRootBodyId` (§7). The reaction comes from `m_hitReactions[currenSequenceId]`, guarded by an
`OG_CHECK` whose message is the rule: a hit under the Hadouken sentinel, or a table that stopped
matching the sequences, is a defect.

**Per tick, never per swing** (guards G-04). Movement-sim task 83 found routing iterating the
per-swing ledger. One hit then re-fired on every remaining tick of the swing: the velocity was
re-assigned at full launch speed for the ~0.4 s left (8.0 m), then decayed (5.17 m), 13.2 m against
an authored 5 m. The lockout restarted every tick, the stun re-entered every tick, and the direction
was re-resolved from positions that had moved, so the throw curved. Both containers still exist and
both are still needed. Branch 3 has had the same shape since og-netcode-v2-field-defects task 17: a
per-pass outcome the detector resets, never the slot's persistent `endReason`. (The 8.0 / 5.17 m
figures are task 83's; they were not re-measured here. 5.17 m is the explicit per-tick decay from
2000 cm/s at 4000 cm/s² and 60 Hz.)

⚠ R0: the pristine says each hit "fires exactly once". It fires once per DETECTION. Since task 27 a
mid-swing attacker that a resim does not integrate (NoSlot) re-detects its target on each skipped
pass, and each of those detections is routed (accepted by the user 2026-09-26,
`DAttackRadialSimulation-rationale.md` §4.4).

### 3.3 Branch 3 — projectile hits (T3, movement-sim task 88)

A slot the detector found `Hit` in this pass routes a hit to the struck character. It is one-shot
because the detector resets `detectedThisTick` at the top of every pass
(`BrawlerProjectileHitDetection-guards.md` G-01) and the shooter's own `integrate` ends the slot in
the same step. ⚠ R0: "once" needs the shooter to be integrated. A NoSlot shooter on a resim replay is
detected and routed again on each skipped pass, the accepted residual of
`BrawlerProjectileHitDetection-rationale.md` §4. Until task 17 the branch matched the wire slot's
`endTick` (and `endReason` 2) against the tick integrated last.

**Stun, or launch a target already flinching** (movement-sim task 88; guards G-06). The spec is
`m_projectileHitReaction` (a Stun), or `m_projectileHitOnFlinchReaction` (a Knockback) when the target
is in `HitFlinch`, whatever its reaction kind (user ruling 2026-09-21); the machine's cross-kind
rules do the rest. The direction is the shot's `spawnDir` projected to XY, falling back to +X; a Stun
discards it. The target's machine state is read in THIS pass, before any `integrate`, so a melee hit
routed in the same pass has not reached the target's machine yet and a same-pass projectile sees the
pre-hit state and stays a Stun (pinned by `HitRouting.ProjectileOnFlinchSameTickStaysAStun`). Both
inputs are on the wire, so a resim reproduces the choice. See §6 for what that pass does to the
melee hit's reaction.

### 3.4 Branch 4 — projectile guard-blocks (T15)

A slot the detector found `BlockedByGuard` routes a recoil to its own shooter: the slot's owner is
the character being iterated, so there is no lookup and no self-hit filter (self is the target). The
machine turns it into `GuardFlinch` from any state but `GuardFlinch`, because a projectile can be
blocked long after its shooter's Hadouken commitment has ended (`DAttackMachineSimulation-guards.md`
G-04). The pristine's "T14's blockedByGuard" is `endReason` 4, which `integrate` still writes from
the outcome.

### 3.5 Branch 5 — melee guard-blocks (og-netcode-v2-field-defects task 9)

An attacker whose swing the detector found blocked by a guard (the radial `DerivedState`'s
`guardBlockedThisTick`) gets a recoil routed to itself, exactly as branch 4 does for a projectile.
The flag is derived and recomputed on every replayed tick, including the first one after a restore,
since task 20 put this pass inside the consuming tick. It replaces the radial `State`'s
`hasHitGuard`, which rode the wire. It must be copied here rather than written by the detector:
guards G-07. ⚠ R0: the pristine says blocked "in the produced tick". There is no produced-tick
notion since task 17; the detector reads the state `integrate(T)` left.

---

## 4. The reaction: direction and dwell

### 4.1 `resolveHitReaction`

It writes the whole reaction onto the slice: `wasHitThisTick`, the kind, and for a Knockback the
spec's speed and the direction, with a dwell of `max(lockoutDuration, knockbackSpeed / launchDecel)`;
for a Stun, speed 0, direction 0 and the spec's lockout. Routing resolves the dwell because it is
the one place holding both the attack table and `launchDecel` (the rationale is the header comment
of `BrawlerHitRoutingTest.cpp`, movement-sim task 27). The `OG_CHECK` on `launchDecel > 0` is the
rule: a zero decel is an infinite slide with a zero-length lockout.

### 4.2 Branch 2's direction: the swing tangent, falling back to away-from-attacker (movement-sim task 83)

The direction is the swing tangent: the way the weapon was travelling through the hit, the user's
"orthogonal to the weapon at the moment of the hit". The detector computes it where the projection
onto the swing plane and the authored angular velocity are already in hand, and routing does not
re-derive it. Movement-sim task 27's away-from-attacker rule (target minus attacker position, XY,
falling back to the attacker's initial aim) is the fallback when the tangent's XY projection is
degenerate (guards G-05). Both arguments are always evaluated; that buys a rule that cannot be
reached with a half-initialised fallback.

⚠ R0 (task 29):
* the pristine says the fallback costs "two wire reads". It reads three `State` values: both
  characters' movement `bodyState.position` and the attacker's radial `InitialConditions` (for the
  aim fallback).
* the pristine says the fallback "is still load-bearing" because a horizontal-axis swing has a
  vertical tangent. That mechanism is real but no shipped Knockback row reaches it: rows 0-3 rotate
  about local Z, the aim rotation axis is ±Z (`dAttackMachineSimulation::setRadialSimulationInitialConditions`),
  so their tangent is a horizontal unit vector (the annulus gate keeps the hit's in-plane radius above
  the 90 cm inner radius). Row 4, the only local-Y sequence, is a Stun, and its direction is discarded.
  The fallback is reached today only by a planted vertical tangent
  (`HitRouting.KnockbackDirectionIsTheSwingTangentWithFallback`, section 3, whose own comment says
  the geometry is not reachable through the shipped table) or a NaN tangent (§4.3). A re-tune of row 4
  to a Knockback makes it load-bearing at once, so the guard stays.

### 4.3 `normalisedXY` and a NaN (moved from `BrawlerHitDetectionSystem-guards.md` G-06, task 29)

The detector's G-06 carried this note about this header's code. Task 29 moved it here, verbatim:

> ⚠ **R0, 2026-09-23, read from `BrawlerHitRoutingSystem.h`.** Routing would survive a NaN TODAY,
> by accident: `normalisedXY` returns its fallback unless `lengthSq > 0.f`, and that comparison is
> false for a NaN. The equivalent rewrite `lengthSq <= 0.f ? fallback : ...` would not fall back,
> and the NaN would reach the target's knockback velocity. The fence is what keeps routing's
> comparison direction from mattering.

Re-verified by task 29 against the header. `attackerAimXY` ends in the same `normalisedXY`, so a NaN
aim angle there falls back to +X the same way.

---

## 5. Walk order, and why it is cross-peer since task 25

`StorageView` walks unordered-map order, unspecified and machine-varying (the `SimulationSystemsExecutor`
library contract: a system whose per-character effects do not commute must impose its own order). The
pass sorts `ordered` by the storage key (the pristine's "D4"; guards G-02), and that sort, not the
hash-map iteration, is the per-tick order the outcome depends on (§6).

⚠ R0: the pristine says the sort makes the outcome "byte-identical across machines". That became true
only with og-netcode-v2-field-defects task 25 (2026-09-26), when the storage key became the
peer-stable `SimCharacterId`. Before it, the key was the component's per-process `GetUniqueID()`, so
two peers could sort the same two characters in opposite orders.

---

## 6. Observed: the last writer wins (awaiting a user ruling)

**This section records behaviour. It is not a guard, and it forbids nothing.** The user has not ruled
on it; `brawler-movement-simulation` holds the open question.

`resolveHitReaction` assigns the whole reaction, so when more than one hit reaches the same target in
one pass, the slice holds the one routed last:
* **melee then projectile.** Branch 3 runs after branch 2 for every character, so a projectile hit
  overwrites a same-pass melee hit's reaction: a knockback becomes the projectile's Stun (0.65 s),
  because the projectile reads the pre-hit machine state (§3.3). Measured and pinned as observed in
  `HitRouting.ProjectileOnFlinchSameTickStaysAStun`, section 2 (kind Stun, speed 0, dwell 0.65).
* **two melee attackers.** Within branch 2 the attacker with the higher id is routed last.
* **two projectiles.** Within branch 3, the higher shooter id, then the higher slot index.

`wasHitThisTick` is set either way, so the target flinches once; only which reaction it gets is
decided by order.

---

## 7. The lifecycle hooks and the map

`m_byRootBodyId` maps a character's capsule `BodyId` value (`CharacterBindings::capsuleBodyId`) to
the storage-stable pointer of its `SimulatableBrawler`. The capsule id is what the query adapter
reports as `SpatialQueryHit::rootBodyId` for a hit on any of the character's shapes (hurtbox or
guard), so it is what `hitsThisTick[].hitRootBodyId` and `detectedThisTick[].struckRootBodyId`
carry. The pointer is stable because the storage holds each simulatable in a `unique_ptr`. The map is
per-process and never on the wire: a `BodyId` is an engine handle, which is why it can be
system-owned (the OGSim system-API design's §3.9 and its parent initiative's D8: non-rollback-affecting,
per-machine). It moved here from `ASimulationManagerUImpl::m_byRootBodyId`. The detector keeps a
map with the same key and a `SimCharacterId` value (`BrawlerHitDetectionSystem-guards.md` G-18).

* **`onCharacterRegistered`** fires after the character is inserted into storage (the manager's
  `tryRegister`, `SimulationManagerUImpl-guards.md` G-69), so the view resolves it.
* **`onCharacterUnregistered`** fires before it is erased (`SimulationManagerUImpl-guards.md` G-73,
  which also skips a character that never reached storage). It erases by the stored pointer, not by a
  recomputed key, so no stale entry survives however the key was derived. ⚠ R0: the capsule id does
  not change over a registered lifetime, so a keyed erase would be equivalent today; the pointer walk
  is the defensive choice, not a fix.

---

## 8. R0 — every false claim task 29 found in these two headers, and what is true

| # | where (pristine) | the claim | what is true |
|---|---|---|---|
| 1 | 13-14 | the off-wire discipline is at "current_state.md §D1" | an initiative workspace file outside this repository. Its substance is `SimulationDerivedComposite`'s requires-clause (§2) |
| 2 | 34-42 | "16 of the 45 files in this directory carry the pair (this file is one of the 29 that don't)" | a count that goes stale on the next file. On 2026-09-26: 21 of 46 headers. The only lasting fact is that this header has no pair |
| 3 | 44-46, 144-147 | the sort makes routing "byte-identical across machines" | only since task 25 made the key peer-stable (§5) |
| 4 | 109-110, 161-162 | the slice holds "the two one-shot bools" | three since task 9 (§3.1) |
| 5 | 112-113 | a radial hit "fires exactly once per hit" | once per detection; the NoSlot residual re-detects (§3.2) |
| 6 | 115-116, 247-249 | a projectile hit "fires once" | only when the shooter is integrated on the step (§3.3) |
| 7 | 122-123, 173-174, 318 | "the produced tick" | no such tick since task 17; the detector reads the end state `integrate(T)` left (§3.5) |
| 8 | 172 | hitsThisTick carries "the stable root body id" | a per-process `BodyId`, stable over a registered lifetime only; the peer-stable id is `targetId`, which routing does not read (§3.2) |
| 9 | 232-234 | the fallback "is still load-bearing" for a horizontal swing axis | no shipped Knockback row has one; a re-tune of row 4 would (§4.2) |
| 10 | 235 | the fallback "costs two wire reads" | three (§4.2) |
| 11 | 224 | the self-hit filter must be pointer identity, "not rootBodyId" | the two are equivalent today (§3) |
| 12 | 24-27 | the header "Depends on" four headers and three primitives | also `SystemRoleAffinity.h`, `HitReaction.h`, `OGAssert.h` and glm; the list is not needed (§1) |

The companion header's own findings are in `BrawlerInboundHit-rationale.md` §4.

**Shape finding for the comment rule (guards G-01).** A realization whose first line is forced ONTO
the tagged statement by the compiler (the use of a tick does not compile until `/*step*/` is named)
but whose other lines are typed in the body. Clause C counts only contiguous lines. Scored `yes`, and
recorded here for the rule's next revision.

**Checked and true, among others:** the move history (the adapter's free function, `BrawlerHitRouter.h`
at T16, the system at the OGSim system-API initiative, og-brawler `361b274`); the registration timing
(§7); the storage's `unique_ptr`; the executor's projection; detection before routing; the
task-83 arithmetic (8.0 + 5.17 = 13.17 m); `HitRouting.ProjectileOnFlinchSameTickStaysAStun`; the
Stun discarding the direction; `m_projectileHitReaction` being a Stun; branch 4 needing no lookup;
`hasHitGuard` having ridden the wire.

---

## §A The removed comments, VERBATIM

Each run is the shipped bytes of the header at md5 `db7ce95d…`, with the section its content now
lives in. ⚠ Several are false: read the section, not the archive (§8).

<!-- pristine lines 4-46 --> → §1 and §5 (corrected: §8 rows 1, 2, 3, 12)
```
// brawlerHitRouting::System — the OGSim-system-api "system" form of the
// cross-character combat-event routing pass (formerly a free function in an
// engine-adapter-owned routing wrapper, since removed). A system is a
// cross-simulatable coordinator: it observes
// the whole SimulatableBrawler population once per tick — in preIntegrate of
// T+1, a pre-integrate reduction over the previous tick's end state (T's
// integrate and physics step have run, T+1's integrate has not) — and fans out
// combat signals (target-side HitFlinch, shooter-side GuardFlinch on a blocked
// projectile) via the brawlerInboundHit::DerivedState slice on each
// character's DerivedState composite (see current_state.md §D1 for the
// off-wire discipline). The slice is produced and consumed inside T+1, so a
// resim replay of T+1 recomputes it from the restored end-of-T state
// [og-netcode-v2-field-defects task 20; it was a post-integrate pass of T,
// consumed by T+1, which a replay starting at T+1 never re-ran].
//
// This class satisfies the engine-core `SimulationSystem<T, StaticDataT>`
// concept (Plugins/OGSimulation/.../SystemsExecutor.h) with
// StaticDataT = simulatableBrawler::StaticData. The SimulationSystemsExecutor
// peer fires its hooks; the manager wires the peer around integrateAll.
//
// Layer: OGBrawler core — engine-agnostic, game-specific. Depends on
// SimulatableBrawler, BrawlerProjectileSimulation, brawlerInboundHit, and the
// engine-agnostic OGSim primitives (SimulatableList, StorageView,
// SimulationTimeStep). NO UE or Godot symbols.
//
// NAMESPACE NOTE: OGSim primitives (SimulatableList, StorageView) are named
// UNQUALIFIED here — the entire OGSim core lives in the GLOBAL namespace, and
// this initiative ratified that convention (lead D12, 2026-07-07). The design
// corpus's `ogsim::` qualification is schematic.
//
// PRAGMA NOTE (N-1, corrected by item 77 2026-08-17): the backlog previously
// claimed OGBrawler-core headers don't use the debugger-friendliness pragma;
// that was stale. They do — 16 of the 45 files in this directory carry the
// pair (this file is one of the 29 that don't), same as OGSim-core, and as of
// item 77 all three subtrees (og-simulation, og-brawler, the UE adapters)
// share ONE macro pair, OGSIM_OPTIMIZE_OFF/ON, defined in
// OGSimulation/CompilerControl.h — not a per-subtree convention anymore. This
// file specifically has no pair to convert; it simply doesn't wrap its
// routing pass in one.
//
// Deterministic order (D4): iterates attackers in ascending SimulatableBrawler
// id — sorts the storage-view snapshot internally so the routing outcome is
// byte-identical across machines (StorageView iteration order is unspecified).
```

<!-- pristine lines 54-54 --> → deleted: a trailing include label (§1)
```
#include "OGSimulation/SimulatableList.h"       // SimulatableList
```

<!-- pristine lines 55-55 --> → deleted: a trailing include label (§1)
```
#include "OGSimulation/StorageView.h"           // StorageView
```

<!-- pristine lines 56-56 --> → deleted: a trailing include label (§1)
```
#include "OGSimulation/SimulationTimeContext.h" // SimulationTimeStep
```

<!-- pristine lines 57-57 --> → deleted: a trailing include label (§1)
```
#include "OGSimulation/SystemRoleAffinity.h"    // SystemRoleAffinity
```

<!-- pristine lines 58-58 --> → deleted: a trailing include label (§1)
```
#include "OGBrawler/SimulatableBrawler.h"       // SimulatableBrawler, simulatableBrawler::StaticData
```

<!-- pristine lines 72-78 --> → guards G-01
```
    // [og-netcode-v2-field-defects task 17] No tick arithmetic lives in this system. Task 20
    // Rework (1) kept a per-step-kind tick offset here so branches 3 and 4 could match a
    // projectile slot's endTick against the tick the timeline integrated last; the user ruled
    // that a system must not need to know which tick ran last. Branches 3 and 4 now read the
    // projectile outcome brawlerHitDetection::System produced in this same pass, exactly as
    // branch 5 reads guardBlockedThisTick, so every step kind (Normal, Skip, Stall, HardResync,
    // a resim replay) routes what this pass detected and nothing else.
```

<!-- pristine lines 80-85 --> → §7
```
    // The hit-routing system. Owns the actor-level root-body-id -> registered
    // brawler map (moved here from the engine adapter): the per-character routing
    // table the preIntegrate pass keys inbound hits against. Populate/erase is
    // driven by the onCharacterRegistered / onCharacterUnregistered lifecycle
    // hooks; per-machine-local ids make the map correctly system-owned (§3.9,
    // parent-initiative D8 — non-rollback-affecting, per-machine).
```

<!-- pristine lines 89-91 --> → §1
```
        // The subset of the game's simulatables this system observes. The
        // executor projects the full storage down to exactly this list before
        // calling each hook. UNQUALIFIED SimulatableList — global namespace (D12).
```

<!-- pristine lines 96-98 --> → §1 and §2 (the concept holds it)
```
        // postIntegrate — no work. Routing is a PRE-integrate reduction over the
        // previous tick's end state (see preIntegrate). Present to satisfy the
        // four-hook SimulationSystem concept.
```

<!-- pristine lines 105-139 --> → §3 (corrected: §8 rows 4-7)
```
        // preIntegrate — the per-tick routing pass (T16 logic, relocated; moved from
        // postIntegrate of T to preIntegrate of T+1 by og-netcode-v2-field-defects
        // task 20). It routes what brawlerHitDetection::System detected earlier in THIS
        // pass onto the slice that this step's integrate reads. Five branches:
        //   1. Reset every character's whole inboundHit slice — the two one-shot
        //      bools and the resolved reaction beside them are owned here.
        //   2. Radial swing hits (T3): route HitFlinch to the struck character.
        //      [movement-sim task 83] Fires exactly once per hit — the per-TICK
        //      hitsThisTick[], never the per-SWING hit ledger.
        //   3. Projectile damage hits (T3; SlotOutcome::Hit): route HitFlinch to the
        //      struck character. Fires once: the detector resets the outcome every
        //      pass, and the shooter's integrate ends the slot in the same step.
        //      [movement-sim task 88] The spec is m_projectileHitReaction (Stun), or
        //      m_projectileHitOnFlinchReaction (Knockback) when the target is in HitFlinch.
        //   4. Projectile guard-blocks (T15; SlotOutcome::BlockedByGuard): route
        //      GuardFlinch to the shooter (self-flag; no map lookup).
        //   5. Radial guard-blocks (og-netcode-v2-field-defects task 9): route
        //      GuardFlinch to the attacker whose swing was blocked in the produced tick
        //      (self-flag; no map lookup). The block itself is DETECTED by
        //      brawlerHitDetection::System, which fires before this system.
        //
        // D5 self-hit filter (SimulatableBrawler* pointer identity — NOT
        // rootBodyId) applies on the target-routing branches (2, 3) only; branches 4
        // and 5 are inherently self-directed.
        //
        // [og-netcode-v2-field-defects task 9] Branch 2 reads hitsThisTick, which is
        // written by brawlerHitDetection::System (BrawlerHitDetectionSystem.h) in the SAME
        // pass, before this system — firing order is template order in
        // SimulationSystemsExecutor, and the manager's BrawlerSystemsExec lists detection
        // first. [task 20] Both run in preIntegrate(T+1) over the end state of T, so the
        // machine still reacts on T+1 — no added latency — and the signal never crosses a
        // tick boundary off the wire. [task 17] Branches 3 and 4 read the projectile
        // DerivedState's detectedThisTick, written by the same detector in the same pass at
        // the slot's closed-form position on THIS step's tick, so the reaction lands in this
        // step's integrate: the tick the shot reaches the target.
```

<!-- pristine lines 144-147 --> → guards G-02 and §5 (corrected: §8 row 3)
```
            // Deterministic walk order (D4): StorageView iteration order is
            // unspecified; sort by ascending id for cross-machine reproducibility.
            // This sort — not the hash-map iteration — is the authoritative per-
            // tick ordering the routing contract depends on.
```

<!-- pristine lines 157-163 --> → guards G-03 and §3.1 (corrected: §8 row 4)
```
            // 1. Reset every character's inbound-signal slice before repopulating
            //    this tick. The routing pass owns the reset/set lifecycle so each
            //    tick's signal is fresh (the machine sim never sees a stale flag).
            //    ⚠ [movement-sim task 27] WHOLE-SLICE, not field-by-field: the slice
            //    now carries the resolved reaction (kind, speed, direction, dwell)
            //    beside the two bools, and a reset that names fields is a reset that
            //    the next field added is silently missing from.
```

<!-- pristine lines 171-193 --> → §3.2 and guards G-04 (corrected: §8 rows 7, 8)
```
            // 2. Radial swing hits — each attacker's hitsThisTick[] (written by the
            //    detector earlier in this pass) carries the stable root body id of
            //    every character its weapon registered a hit on in the PRODUCED
            //    tick, and the direction the weapon was
            //    travelling through each of those hits.
            //
            // ⭐⭐ [movement-sim task 83] hitsThisTick, NOT THE PER-SWING LEDGER, AND
            //    THE DIFFERENCE IS THE WHOLE OF THE USER'S 12 METRES. The ledger (a
            //    derived vector then; the synced State::hitTargets of target ids since
            //    og-netcode-v2-field-defects task 27) accumulates for the whole swing
            //    and is cleared only in deactivate(). Iterating it here meant
            //    ONE hit re-fired on EVERY remaining tick of the swing — the target's
            //    velocity was re-assigned at full launch speed with no decay for the
            //    ~0.4 s the swing had left (8.0 m of constant travel, then 5.17 m of
            //    decay = 13.2 m against an authored 5 m), the lockout timer restarted
            //    every tick, a stun re-entered every tick, and the direction was
            //    re-resolved from positions that had MOVED, so the throw curved.
            //    ⛔ The bug was not that the container was wrong; it was that the
            //    per-SWING container was being read as a per-TICK signal. Both still
            //    exist and both are still needed.
            //    ⭐ Branch 3 below has the same shape since og-netcode-v2-field-defects
            //    task 17: a per-pass outcome the detector resets, never the slot's
            //    persistent endReason.
```

<!-- pristine lines 224-224 --> → §3, the self-hit filter (corrected: §8 row 11)
```
                    if (target == attackerPtr)   // D5 self-hit filter (pointer identity, not rootBodyId)
```

<!-- pristine lines 226-236 --> → §4.2 and guards G-05 (corrected: §8 rows 9, 10)
```
                    // ⭐ [movement-sim task 83] THE DIRECTION IS THE SWING TANGENT — the way
                    // the weapon was travelling through the hit, which is the user's "orthogonal
                    // to the weapon at the moment of the hit". It is computed at the push site,
                    // where the projection onto the swing plane and the sequence's authored
                    // angular velocity are both already in hand; nothing here re-derives it.
                    // ⛔ The away-from-attacker rule task 27 shipped is now the FALLBACK, and it
                    // is still load-bearing: a swing whose axis is horizontal has a VERTICAL
                    // tangent whose XY projection is degenerate, and normalisedXY would otherwise
                    // hand a NaN straight into a velocity that never leaves the body. Both
                    // arguments are evaluated, which costs two wire reads and buys a rule that
                    // cannot be reached with a half-initialised fallback.
```

<!-- pristine lines 246-251 --> → §3.3 (corrected: §8 row 6)
```
            // 3. Projectile damage hits — a slot the detector found HIT in this pass
            //    routes a one-shot inbound hit to the struck character. One-shot
            //    because the detector resets detectedThisTick at the top of every pass
            //    and the shooter's integrate ends the slot in this same step.
            //    [og-netcode-v2-field-defects task 17] It used to match the wire slot's
            //    endTick (and endReason 2) against the tick integrated last.
```

<!-- pristine lines 271-280 --> → §3.3 and guards G-06
```
                    // [movement-sim task 88] A target ALREADY in HitFlinch (either reaction
                    // kind, user ruling 2026-09-21) is LAUNCHED rather than re-stunned; the
                    // machine's cross-kind rules (design_hit_reactions.md §3) do the rest.
                    // ⛔ m_currentState ONLY: GuardFlinch is a separate enumerator, so a
                    // guard-flinching target still takes the authored Stun.
                    // ⭐ Read in THIS pass, before any integrate: a melee hit routed in the same
                    // pass has not reached the target's machine yet, so a same-pass projectile
                    // still sees the pre-hit state and stays a Stun (pinned in
                    // HitRouting.ProjectileOnFlinchSameTickStaysAStun). Both inputs are on the
                    // wire, so a resim reproduces the choice.
```

<!-- pristine lines 285-286 --> → §3.3
```
                    // The direction below is the projectile's travel direction; a Stun
                    // discards it, a Knockback launches along it.
```

<!-- pristine lines 297-302 --> → §3.4
```
            // 4. [T15] Shooter-side projectile-blocked routing — any projectile
            //    slot the detector found BLOCKED by a guard in this pass (T14's
            //    blockedByGuard) routes a GuardFlinch trigger to the slot's OWNING
            //    character (the shooter). No map lookup — the shooter IS the
            //    attacker whose sim is being iterated. No self-hit filter: self is
            //    exactly the right target here.
```

<!-- pristine lines 317-327 --> → §3.5 and guards G-07 (corrected: §8 row 7)
```
            // 5. [og-netcode-v2-field-defects task 9] Radial guard-blocks — an attacker whose
            //    swing brawlerHitDetection::System found blocked by a guard in the produced tick (the
            //    radial DerivedState's per-tick guardBlockedThisTick) gets GuardFlinch routed
            //    to itself, exactly as branch 4 does for a blocked projectile. Self-directed,
            //    so no map lookup and no self-hit filter. The flag it copies is derived and
            //    is recomputed on every replayed tick — including the first one after a
            //    restore, since task 20 put this pass inside the consuming tick; the radial
            //    State's hasHitGuard, which this replaces, rode the wire.
            //    ⛔ It must be COPIED here, not written onto the slice by the detector: branch
            //    1 above resets the whole slice every tick, and detection fires before routing,
            //    so a bit the detector set on the slice would be wiped before anyone read it.
```

<!-- pristine lines 339-347 --> → §7
```
        // onCharacterRegistered — index the just-registered character for routing.
        // §3.11 timing: the character IS already in storage when this fires, so the
        // view resolves it. Key = the character's CAPSULE (root) body id — the
        // value the query adapter emits as SpatialQueryHit::rootBodyId for hits on
        // ANY of the character's shapes (hurtbox or guard), hence carried by radial
        // hitsThisTick[].hitRootBodyId / the projectile DerivedState's
        // detectedThisTick[].struckRootBodyId. Value = the
        // storage-stable pointer to the SimulatableBrawler (unique_ptr-backed, so
        // its address is stable for the registered lifetime).
```

<!-- pristine lines 357-361 --> → §7
```
        // onCharacterUnregistered — drop this character's routing entry BEFORE it
        // is erased from storage (§3.11 timing: the character is still in storage
        // here, so the view resolves it). Erase by stored-pointer identity (not by
        // recomputed key) so no stale entry can survive regardless of how the key
        // was derived.
```

<!-- pristine lines 434-439 --> → §7
```
        // Actor-level root-body-id -> registered brawler, for cross-character
        // routing. Key = the character capsule's BodyId.value
        // (CharacterBindings::capsuleBodyId); value = raw pointer into storage's
        // unique_ptr (stable across the character's registered lifetime). Moved
        // out of the engine adapter (ASimulationManagerUImpl::m_byRootBodyId) so
        // the routing system owns its own bookkeeping (§3.9 / D8).
```

<!-- pristine lines 440-440 --> → §7 (an inline `/* */` label, deleted; the code line is otherwise unchanged)
```
        std::unordered_map<uint32_t /* BodyId.value */, SimulatableBrawler*> m_byRootBodyId;
```

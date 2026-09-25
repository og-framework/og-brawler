<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: hasHitGuard -- RETIRED by og-netcode-v2-field-defects task 9: the radial State field the machine's guard-block recoil used to read. It must NOT resolve; the recoil now reads brawlerInboundHit::DerivedState::wasGuardBlockedThisTick -->
<!-- lint-external-ref: glm/vec2.hpp -- the third-party glm header vendored under og-simulation/glm; the lint indexes .h/.cpp, not glm's .hpp -->
<!-- lint-external-ref: glm/common.hpp -- the third-party glm header vendored under og-simulation/glm; the lint indexes .h/.cpp, not glm's .hpp -->
<!-- lint-external-ref: current_state.md -- an initiative workspace file (og-brawler-hit-resolution) outside this repository; quoted from the pre-conversion header, not a join this document relies on -->
# `DAttackMachineSimulation.h` — rationale

The narrative, the derivations and the provenance for the attack state machine: `DAttackState`,
`dAttackMachineSimulation::PlayerInput`, `IntegrationUtils`, `swingTickCount`, `State`,
`Dependencies`, the three `integrate` variants and `dAttackStateName`. The **prohibitions** are in
`DAttackMachineSimulation-guards.md`; nothing here is a fence. Two sections carry a derivation id
(`∴D-nn`) because a tag in the header points at them.

**If this file and `DAttackMachineSimulation.h` disagree, the header is authoritative and this file is
stale.** Fix this file.

⚠ **Not to be confused with `DAttackMachineSimulationRuntimeTweakables-rationale.md`**, which documents
a different header.

⚠ **This file is not the source of truth for any VALUE.** Where it quotes a number, the code and the
tests win.

---

## 0. How to read this file

The header keeps a licence line, a two-path docs pointer, code, and one-line tags. Everything it used
to explain is here or in the guards document. Blocks marked `<!-- header lines A-B -->` are the
pre-conversion header's own comments, **verbatim**. That file is og-brawler `b572456` plus task 9's
four uncommitted hunks, 907 lines. "Above" and "below" inside a quote describe that file. Where a
quoted sentence is false, the **R0** note under it says what is true, and §13 lists every one.

---

## 1. The include graph

The machine header includes the **leaf** `BrawlerCharacterBindings.h`, not
`BrawlerMovementSimulation.h`. The movement sub-simulation reads this header's `State` (the flinch
freeze and the attack end tick) and its `PlayerInput` (the move stick is packed onto the machine
slice), so movement includes this header. The dependency points one way: movement → machine.

<!-- header lines 14-17 -->
> [movement-sim task 62] The LEAF header, NOT `BrawlerMovementSimulation.h`. Everything this
> header wants from the movement sub-sim is `CharacterBindings` (one field, one dependency),
> and it must not reach the movement header — movement includes THIS one, so the dependency
> points one way. See the note above `integrate3`.

<!-- header lines 556-565 -->
> [Task 35, re-pointed at movement-sim task 62] CharacterBindings lives in
> `OGBrawler/BrawlerCharacterBindings.h` — the leaf header included above — so integrate3 takes
> a plain const reference and the T33 templated workaround stays gone.
> ⛔ It used to live in `BrawlerMovementSimulation.h`, and the "no include cycle" that made that
> safe STOPPED BEING TRUE once the movement sub-sim began reading this header's `State` and
> `PlayerInput` slices. Task 62 moved the struct to a leaf both sides can include. The
> dependency now points one way — movement -> machine — and THIS HEADER MUST NEVER INCLUDE
> `BrawlerMovementSimulation.h`, directly or through any of its other includes.
> The Hadouken trigger resolves the parent capsule transform on-demand from the bindings handle
> — matching the bindings-as-integrate-param pattern radial/guard/projectile already use.

**The include ban is held by the compiler (measured by task 18).** Re-adding
`#include "OGBrawler/BrawlerMovementSimulation.h"` to this header fails with `C2653`
(`'dAttackMachineSimulation': is not a class or namespace name`, then a cascade of `C2039`) in every
translation unit that reaches this header before the movement header. That includes one that
includes `SimulatableBrawler.h`, the production entry point. The movement header then compiles
before the names it uses are declared. It compiles clean only in a translation unit that includes
the movement header first. The same holds for an include through any of this header's other
includes, because the failure is in the movement header's own body. So the cycle is not silent: it
breaks the build in the include order the product uses.

`BrawlerInboundHit.h` is included for `brawlerInboundHit::DerivedState`, which `integrate3` takes as
a plain by-reference parameter:

<!-- header lines 19-21 -->
> [hit-resolution T2] brawlerInboundHit::DerivedState — read by integrate3 as a plain
> by-ref param (NOT an ExternalDep, see current_state.md §D7). Zero-dependency header,
> no include cycle.

**R0.** `BrawlerInboundHit.h` is not "zero-dependency". It includes `glm/vec2.hpp` and
`OGBrawler/HitReaction.h`, which includes only `<cstdint>`. What is true is that nothing it includes
reaches this header, so there is no cycle. The reason it is a parameter and not an `ExternalDeps`
entry is a compile error. Measured by task 18: adding it to `Dependencies::External` fails with
`C2338` "UNOWNED EXTERNAL REF", because `ExternalDeps` resolves out of the serialized `State`
composite and this slice lives on the `DerivedState` composite.

`glm/common.hpp` is included for `glm::abs` (§9). Its trailing label read:

<!-- header lines 6 -->
```cpp
#include "glm/common.hpp"	// glm::abs -- see the task-32 note at the abs site below
```

---

## 2. The file-scope constants

<!-- header lines 30-36 -->
> [Task 25] Hadouken commitment duration. When integrate3 fires a Hadouken it transitions
> Idle -> Attacking with the kHadoukenSequenceSentinel active (the sentinel lives at file
> scope in DAttackRadialSimulation.h, included above). Without a commitment window the
> machine exits Attacking -> Idle one tick later (the radial early-returns and leaves
> currenSequenceId == InvalidAttackSequenceId), which lets a still-held attack button chain
> an immediate normal swing. This minimum dwell (0.3 s ≈ 18 ticks at 60 Hz) keeps the
> machine in Attacking for the projectile cast before the normal exit-to-Idle gate fires.

**R0.** The sentinel `kHadoukenSequenceSentinel` is declared in `DAttackSequenceId.h`, which this header
includes directly (and which the radial header includes too). It is not declared at file scope in
`DAttackRadialSimulation.h`. The rest is true, and 0.3 s is exactly 18 ticks at 60 Hz by
`swingTickCount`.

<!-- header lines 39-44 -->
> [hit-resolution T1] Minimum dwell for the target-side HitFlinch state. When an inbound hit
> signal arrives (T2 threads the real External; T1 gates on a false placeholder), the machine
> transitions Idle/Attacking -> HitFlinch and stays there until m_timeInCurrentState exceeds this
> window, then returns to Idle. Mirrors the existing GuardFlinch duration (0.3 s ≈ 18 ticks at
> 60 Hz). File-scope constant matches the kHadoukenCommitmentSeconds precedent above; the eventual
> lift into DAttackMachineSimulationRuntimeTweakables.h is R-P1 cleanup tracked separately.

**R0.** Three corrections:
* `integrate3` reads the real signal, as a plain by-reference `brawlerInboundHit::DerivedState`
  parameter, not an `ExternalDeps` entry. Only the legacy `integrate`/`integrate2` still gate on a
  compile-time `false` (§11).
* The `HitFlinch` case dwells on `State::m_flinchDuration`, not on this constant. The inbound-hit veto
  copies `inboundHit.flinchDuration` (the routed reaction's dwell) into it. `kHitFlinchDuration` is
  only the default member initialiser of `m_flinchDuration`.
* `integrate3`'s veto enters `HitFlinch` from **any** state, not from `Idle`/`Attacking` only. It has no
  origin test. The legacy variants test `!= HitFlinch`.

---

## 3. `PlayerInput`

<!-- header lines 73 -->
> [Task 43] Plain aggregate — const dropped so MemberFieldDesc::write() can assign.

Measured by task 18: re-adding `const` to a field compiles in a header-only translation unit, and
fails (`C2678` for the `glm::vec3`, `C3892` for the `uint32_t`) as soon as `readFromSyncedBuffer`
deserializes the type. `InputRedundancyBundleCodec.h` and `RelayedInputRingCodec.h` both do.

<!-- header lines 79-82 -->
> Set by the input-layer motion matcher (buildPlayerInput) to inputSequence::kHadoukenActionId
> on the tick a Hadouken sequence completes; 0 otherwise. Appended last so the existing
> 5-arg aggregate-init call sites keep compiling (C++20 parenthesized aggregate init
> defaults this to 0). Travels through the PlayerInput RPC like any other input field.

`buildPlayerInput` is the Unreal input component's builder. It calls
`simulatableBrawler::resolveTriggeredActionId` (`BrawlerMotionMatching.h`). Moving the field
anywhere but last breaks `zero()`'s five-argument initializer (`C2440`, measured).

<!-- header lines 85-91 -->
> THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
> SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
> [movement-sim task 22] The value is copied VERBATIM from what that function
> handed this type before the fold; it is a wire value, not something to re-derive.
> ⛔ (0,0,1) forwards, NOT PlayerInput{}: a value-initialised (0,0,0) aim would
> reach normalize(), and the difference is also the TAG the input-resolution and
> net-sync anti-vacuity tests discriminate on. Keep zero() != PlayerInput{}.

<!-- header lines 94-95 -->
> triggeredActionId is left to its default member initialiser (0) by C++20
> parenthesized aggregate init, exactly as the pre-fold call site did.

The prohibition in that block is guard **G-01**, with its R0: `(0,0,1)` is `defaultUp`, and for this
slice the normalize argument does not separate the two values.

---

## 4. `IntegrationUtils` and `swingTickCount`

### 4.1 The accessors

<!-- header lines 119-123 -->
> [movement-sim task 84] Current simulation tick. Needed because State::m_attackEndTick is an
> ABSOLUTE tick, and the machine is the only sub-sim that both owns the Idle->Attacking
> transition and holds the sequence table, so it is the only place the end can be computed.
> Plumbed in from SimulationTimeStep at the SimulatableBrawler::integrate call site, exactly
> as brawlerProjectileSimulation::IntegrationUtils has had it since T15.

<!-- header lines 125-127 -->
> [movement-sim task 84] The authored sequence table this class has always held by reference
> but never exposed. integrate3 reads getDuration() from it at the two write sites that start
> a radial swing; nothing else in this header indexes it.

<!-- header lines 130-134 -->
> Projectile launch parameters — needed by the Hadouken trigger block in integrate3 to
> write the projectile InitialConditions. The parent capsule position is no longer
> pre-resolved here (T33): integrate3 looks it up on-demand via the physics adapter from
> the CharacterBindings handle, matching the bindings-as-integrate-param pattern radial/
> guard/projectile already use.

All three verified 2026-09-23. `SimulatableBrawler::integrate` builds the machine's utils from
`step.getTick()`, the same tick the projectile's utils take. The two `getDuration()` reads are the
`Idle` swing entry and the chain.

### 4.2 `swingTickCount`

<!-- header lines 147-163 -->
> [movement-sim task 84] How many ticks a swing of `duration` occupies, at a fixed `dt`.
>
> This is a LOOP, and it is a loop deliberately: `ceil(duration / dt)` is NOT this number.
> DAttackRadialSimulation::integrate ends a swing on the float predicate
> `state.attackTimer < activeAttackSequence.getDuration()` with `attackTimer` accumulated as
> `attackTimer = attackTimer + deltaSeconds` from `0.f`. Repeated float addition is not
> multiplication: the shipped side swings run 0.7 s and 42 * (1/60) is 0.7 in real arithmetic but
> lands BELOW 0.7f in float, so the radial takes 43 steps where the division says 42. Nothing a
> reader can inspect tells them which side a given (duration, dt) pair falls on, and the helper
> must not guess -- so it performs THE SAME float operations in THE SAME order as the radial and
> returns the count the radial itself will reach. The agreement is pinned by
> DAttack.Integrate3.AttackEndTickMatchesTheFirstIdleTick, which drives the whole
> SimulatableBrawler for every authored sequence and the Hadouken.
>
> It assumes `dt` is the same on every tick of a swing -- true today (fixed 60 Hz step,
> kNominalSimStepSeconds; SimulationManager reads one stepDt per step). A variable step would
> break the radial's own schedule the same way, and guarding that is not this function's job.

**R0.** The conclusion is true and the stated cause is not. Measured on MSVC 14.38 with `/fp:fast`:
42 float additions of `1.f / 60.f` land **exactly on** `0.7f`. The duration `getDuration()` returns for
sequences 0 and 1 is `0.6f + 0.1f` = `0.70000005f`, one ulp higher, because the constructor appends
the zero-velocity point. So the loop takes 43 steps where `ceilf` says 42. The prohibition is guard
**G-02**, and so is the record of why it is not a `static_assert`: MSVC's constant evaluator does not
reproduce the run-time division.

`kNominalSimStepSeconds` is `1.f / 60.f` (`BrawlerMovementSimulation.h`), and `SimulationManager`
reads one `stepDt` per step. The fixed-step assumption holds today.

---

## 5. `State::m_attackEndTick`

<!-- header lines 196-203 -->
> [movement-sim task 84] The absolute sim tick on which this machine will next be Idle --
> one past the LAST Attacking tick. Written ONLY where integrate3 produces a radial EDGE
> (the three write sites below); read by the movement sub-simulation, which runs LAST in
> SimulatableBrawler::integrate and therefore sees Attacking on ticks T..E while this value
> is E + 1. It is ON THE WIRE because a remote proxy that enters a swing by ADOPTION never
> simulated the edge and could not have computed the end -- the same argument
> brawlerRingout::State::respawnAtTick records for its countdown. Appended LAST, so every
> preceding field keeps the byte offset it already had.

**R0.** There are three write sites and two of them are radial edges: the `Idle` swing entry (§8.2) and
the chain (§8.3). The third is the Hadouken entry (§8.1), where the sentinel keeps the radial idle, so
there is no edge. The movement sub-simulation reads the field in its attack-lock branch as
`remaining = (m_attackEndTick > tick) ? m_attackEndTick - tick : 1u`. `brawlerRingout::State::respawnAtTick`
is the precedent the quote names: an absolute tick, on the wire. The field is still the last one in
`State` and the last in `SerializableFields`. That records how it was added; moving it now would be a
wire change like any other.

---

## 6. The Hadouken

### 6.1 The trigger comes first in `Idle`

<!-- header lines 636-640 -->
> Hadouken trigger: the input-layer motion matcher (buildPlayerInput) sets
> triggeredActionId to kHadoukenActionId on the tick a motion completes. Spawn a
> projectile in the aim direction and hand the radial weapon a sentinel sequence so
> it stays in its idle pose. This sits AHEAD of the plain attackLeft/attackRight
> handling so a matched motion takes priority over the idle swing on the same tick.

That is an ordering, and it has no tag: the case `break`s after the trigger, and the button handling
below it never runs on a trigger tick.

<!-- header lines 647-649 -->
> [Task 33] Resolve the parent capsule position on-demand from the CharacterBindings
> handle, instead of receiving it pre-resolved via IntegrationUtils. This matches the
> radial/guard/projectile pattern (bindings passed to integrate, physics queried inside).

⚠ R0, 2026-09-24: "physics queried inside" no longer holds for two of the three. The radial
(og-netcode-v2-field-defects task 9) and the projectile (task 17) issue no spatial queries: their
hit detection is `brawlerHitDetection::System`'s. Both still write their own bodies through the
physics adapter, and the guard still toggles its shapes.

<!-- header lines 654 -->
> XY-projected aim, with a degenerate-aim fallback to avoid a NaN from normalize.

<!-- header lines 660-661 -->
> Closed-form launch parameters (Task 13): write spawnDir, NOT velocity — the
> projectile sim derives velocity = spawnDir * projectileSpeed each tick.

There is no velocity field on `brawlerProjectileSimulation::InitialConditions`: writing one fails with
`C2039` (measured). The projectile's position is closed-form from `spawnPos`, `spawnDir`,
`projectileSpeed` and the spawn tick.

### 6.2 The sentinel

<!-- header lines 668-669 -->
> Hand the radial weapon the sentinel so its integrate early-returns (idle pose)
> instead of indexing attackSequences[] out of bounds.

Verified: `dAttackRadialSimulation::integrate` returns on the sentinel before it would index the
sequence table, after re-parenting the weapon to its owner, and it sets `currenSequenceId` to
`InvalidAttackSequenceId`. It does not call `setIdlePose` on that path; "idle pose" means the weapon
is not driven.

### 6.3 The commitment window

<!-- header lines 793-797 -->
> [Task 25] Hold the Hadouken-Attacking state for a minimum commitment window before
> the normal exit-to-Idle gate may fire. The radial sim early-returns on the sentinel
> (leaving currenSequenceId == InvalidAttackSequenceId from tick T+1), so without this
> guard the machine would drop back to Idle one tick after the trigger and a still-held
> attack button would chain an immediate normal swing.

---

## 7. The inbound signals in `integrate3`

### 7.1 The inbound-hit veto

<!-- header lines 586-596 -->
> [hit-resolution T2] Inbound-hit veto. Real signal read from the plain by-ref parameter
> (T1's compile-time-false placeholder is gone). inboundHit is a per-character
> brawlerInboundHit::DerivedState slice on the composite DerivedState, populated by the
> manager's routing pass (T3) on the prior tick. It is passed as a plain integrate3 param
> (NOT via deps.external) because it lives on the DerivedState composite, not the serialized
> State composite — see current_state.md §D7. Sits AHEAD of the switch — and therefore ahead
> of the Idle case's Hadouken trigger and attack-input handling — so a live signal vetoes both
> attack inputs and the Hadouken trigger on the hit tick. On a live hit we cancel the
> active/queued sequences (mirroring the Attacking -> GuardFlinch cancellation) and drop into
> HitFlinch; the switch below then lands in the HitFlinch case with m_timeInCurrentState
> freshly reset.

**R0.** The slice is written by `brawlerHitRouting::System::postIntegrate` (the systems executor's
routing system, after every character's integrate), not by a manager-owned routing pass. "On the prior
tick" is true. ⚠ **Since og-netcode-v2-field-defects task 20** the pass is
`brawlerHitRouting::System::preIntegrate` of the consuming tick: it reduces the prior tick's end state
and writes the slice inside the tick that reads it, so "set on the prior tick" is no longer literal;
"about the prior tick" is. The reference to `current_state.md` §D7 points at an initiative workspace outside this
repository. Its substance is the compile error in §1. "Ahead of the switch" is an ordering, and it
has no tag.

### 7.2 The two guard-block recoils

The projectile-block recoil is guard **G-04**; its quote is there. The swing's own guard block
arrives the same way:

<!-- header lines 735-738 -->
> [og-netcode-v2-field-defects task 9] The swing's guard block arrives on the inbound
> slice, routed on the prior tick exactly like wasProjectileBlockedThisTick above. It
> used to be the radial State's hasHitGuard, read through deps.external; that field left
> the wire when hit detection moved into brawlerHitDetection::System.

Verified: routing branch 5 copies the radial `DerivedState`'s `guardBlockedThisTick` onto the
attacker's inbound slice as `wasGuardBlockedThisTick`. `hasHitGuard` exists nowhere in the tree.

### 7.3 A hit and a projectile block on the same tick

Found by reading the code, not by running it. Routing sets `wasHitThisTick` (branches 2 and 3) and
`wasProjectileBlockedThisTick` (branch 4) independently. When both are set for one character on one
tick, the veto moves the machine to `HitFlinch` and writes the hit's reaction and dwell. Then the
projectile-block test passes (`HitFlinch` is not `GuardFlinch`) and moves it to `GuardFlinch`. The
switch lands in `GuardFlinch`, not in `HitFlinch` as §7.1's quote says, and the character recoils for
the fixed 0.3 s instead of taking the hit's reaction. Whether that precedence is intended is not recorded anywhere.
It is routed, not changed.

### 7.4 The `HitFlinch` dwell

<!-- header lines 850-852 -->
> [hit-resolution T1] Mirrors GuardFlinch: dwell for kHitFlinchDuration, no attack-input
> reads (gating is automatic — the switch never reaches Idle/Attacking while flinching),
> then return to Idle.

**R0.** The dwell is `State::m_flinchDuration`, the routed reaction's dwell, not `kHitFlinchDuration`
(§2).

---

## 8. The attack end tick, write site by write site

The movement sub-simulation runs **last** in `SimulatableBrawler::integrate`, and the machine runs
**first**, before the radial. Those two orders are what every derivation below rests on.

### 8.1 The Hadouken entry: no `+ 1` ∴D-01

<!-- header lines 674-679 -->
> [movement-sim task 84] WRITE SITE 2 of 3, and it has NO `+ 1`. There is no radial
> here -- the sentinel makes the weapon early-return -- so there is no Invalid arriving
> a tick late. The machine gates ITSELF on `m_timeInCurrentState < kHadoukenCommitment
> Seconds`, accumulating `+= dt` from the 0 assigned on the line above with the same
> `<` predicate swingTickCount replicates, and exits on the first tick the sum reaches
> the window.

Derivation. The machine resets `m_timeInCurrentState` to 0 on the trigger tick T. It adds `dt` at the
top of every later tick, and it holds `Attacking` while the sum is `< kHadoukenCommitmentSeconds`. So
it leaves on the first tick where the sum reaches the window, which is
`T + swingTickCount(kHadoukenCommitmentSeconds, dt)`. The radial set `currenSequenceId` to invalid on
tick T already (§6.2), so nothing arrives a tick late and there is nothing to add.
`DAttack.Integrate3.AttackEndTickMatchesTheFirstIdleTick` pins it (18 ticks at 60 Hz).

### 8.2 The `Idle` swing entry: the radial's deactivate tick plus one ∴D-02

<!-- header lines 708-711 -->
> [movement-sim task 84] WRITE SITE 1 of 3. End tick = the radial's deactivate tick
> + 1: the edge fires THIS tick (the radial resets its timer to 0 and then adds one
> dt), it deactivates on `tick + swingTickCount`, and this machine -- which runs
> BEFORE the radial -- sees that Invalid one tick later and exits then.

Derivation. On tick T the machine writes the new sequence into the radial's `InitialConditions`, and
the radial, running later on the same tick, takes its edge: `setInitialConditions` resets
`attackTimer` to 0 and the swing branch adds one `dt`. After k = `swingTickCount(getDuration(), dt)`
additions, the radial's `<` test fails on tick T + k and it deactivates. The machine runs **before**
the radial, so it first sees the invalid sequence on tick T + k + 1 and goes `Idle` then. That tick is
the field's value.

### 8.3 The chain

Guard **G-07**. Its formula is §8.2's: the block runs on a tick where the radial is already invalid, so
the new sequence edges on the same tick exactly as it does from `Idle`.

### 8.4 The dual-tap branch, and the path its pin does not cover

Guard **G-06** holds the non-write. What the guard's quote does not cover: from a side swing (sequence
0 or 1, for example both buttons with the stick sideways), this branch changes the active sequence to
4. The radial's edge predicate is then true, and the swing restarts as sequence 4 while the end tick
still holds the side swing's prediction. At 60 Hz that prediction is 44 ticks from entry (43 + 1), and
sequence 4 from one tick later ends at 1 + 32 + 1 = 34, so the slide's schedule is 10 ticks long.
Derived from the code and the measured tick counts. Not run. The owner is brawler-movement-simulation
Backlog task 85, whose entry today describes only the 4 → 4 path.

---

## 9. `Dependencies`, `setRadialSimulationInitialConditions` and `glm::abs`

<!-- header lines 207 -->
> [Task 62] Dependencies — OwnedDeps/ExternalDeps layout.

`Dependencies` reads the radial's `State` (for `attackTimer` and `currenSequenceId`) and edits the
radial's and the projectile's `InitialConditions`. The inbound slice is not in it (§1).

<!-- header lines 235-244 -->
> [movement-sim task 32] glm::abs, NOT unqualified abs -- byte-identical to the
> guard site task 29 fixed in DAttackGuardSimulation.h, and fixed for the same
> reason. Under C's `::abs(int)`, which may be the only overload visible at this
> header's point of definition on the Godot/Jolt toolchains, the expression
> collapses to `|aimDot| == 1` EXACTLY: the near-pole epsilon band disappears and
> the normalize(cross(...)) below is handed a near-zero vector.
> Task 32 measured that this was ALREADY binding the float overload on this
> toolchain (MSVC 14.38): PORTABILITY HARDENING, not a behaviour fix. Pinned by
> DAttackAbsQualificationTest.cpp
> "DAttackAbs.MachineNearPoleAimTakesTheEpsilonBandBranch" (axis.z +1 vs -1).

The prohibition is guard **G-03**.

---

## 10. `DAttackState`, its count, and `dAttackStateName`

<!-- header lines 59-60 -->
> The enumerator count, kept ADJACENT so adding a state without bumping it is visible
> in the same few lines. A display that folds over DAttackState sweeps against this.

Converted by task 18. The count is now checked by a `static_assert` beside `dAttackStateName`: every
index below `kDAttackStateCount` has a real name, and the index `kDAttackStateCount` falls through to
`"?"`. On MSVC the name switch is compiled with C4062 as an error, so a new enumerator without a
`case` fails there first. Each wrong edit was made and seen to fail: an enumerator added alone, added
with a name, added with a bump, and inserted mid-enum. The full edit (enumerator, name and bump)
compiles. The consumers that fold over the enum (the input-history lanes and their tests) read the
count and are unaffected by the conversion.

<!-- header lines 540-543 -->
> [og-netcode-v2-field-defects task 7] dAttackStateName is the `%s` of three [Machine.*] lines,
> and the OGBLOG_G clip check charges every `%s` ogblog::kMaxStringArgBytes — a bound no type can
> prove. This is that half: every name, the "?" fallback included (index kDAttackStateCount),
> must fit it, or a [Machine.*] line could clip silently.

Verified: the three `%s` sites are `[Machine.integrate]` and the two `[Machine.transition]` lines that
name the origin state. `ogblog::kMaxStringArgBytes` is 32.

---

## 11. The legacy `integrate` and `integrate2`

Neither is called in production. `SimulatableBrawler::integrate` calls `integrate3`. The one caller of
`integrate` is `DAttackAbsQualificationTest.cpp` (the near-pole rig, §9), and `integrate2` has **no
caller anywhere in the tree**. Both read compile-time `false` placeholders where `integrate3` reads the
inbound slice.

<!-- header lines 269-274 -->
> [hit-resolution T1] Inbound-hit veto. Placeholder signal (always false until T2 threads the
> real brawlerInboundHit::DerivedState External). Read at the top of the integrate body — before
> case dispatch and before any attack-input handling — so a live signal vetoes attack inputs on
> the hit tick. On a live hit we cancel the active/queued sequences (mirroring the GuardFlinch
> cancellation) and drop into HitFlinch; the switch below then lands in the HitFlinch case with
> m_timeInCurrentState freshly reset.

<!-- header lines 405-410 -->
> [hit-resolution T1] Inbound-hit veto. Placeholder signal (always false until T2 threads the
> real brawlerInboundHit::DerivedState External). Read at the top of the integrate body — before
> case dispatch and before any attack-input handling — so a live signal vetoes attack inputs on
> the hit tick. On a live hit we cancel the active/queued sequences (mirroring the GuardFlinch
> cancellation) and drop into HitFlinch; the switch below then lands in the HitFlinch case with
> m_timeInCurrentState freshly reset.

**R0.** In these two variants the placeholder is permanent. T2 gave the real signal to `integrate3`
only, and as a parameter, not an `ExternalDeps` entry.

<!-- header lines 312-314 -->
> [og-netcode-v2-field-defects task 9] Legacy variant. The guard block now arrives on the
> inbound slice (wasGuardBlockedThisTick), which only integrate3 receives -- the same
> shape as the T1 inbound-hit placeholder above. The radial State's hasHitGuard is gone.

<!-- header lines 448-450 -->
> [og-netcode-v2-field-defects task 9] Legacy variant. The guard block now arrives on the
> inbound slice (wasGuardBlockedThisTick), which only integrate3 receives -- the same
> shape as the T1 inbound-hit placeholder above. The radial State's hasHitGuard is gone.

<!-- header lines 378-380 -->
> [hit-resolution T1] Mirrors GuardFlinch: dwell for kHitFlinchDuration, no attack-input
> reads (gating is automatic — the switch never reaches Idle/Attacking while flinching),
> then return to Idle.

<!-- header lines 514-516 -->
> [hit-resolution T1] Mirrors GuardFlinch: dwell for kHitFlinchDuration, no attack-input
> reads (gating is automatic — the switch never reaches Idle/Attacking while flinching),
> then return to Idle.

**R0.** As in §7.4, the case dwells on `State::m_flinchDuration`.

---

## 12. Removed without a home elsewhere

The four section banners, the `[Task 39]` label above the `SerializableFields` specializations, and six
inline `/*!sic*/` markers. The markers sat on the two `attackState.attackTimer > 0.3` reads in each of
`integrate`, `integrate2` and `integrate3`, and not on the `attackTimer < 0.1` reads. They have been there since og-brawler's first public commit, and nothing in
the tree or its history records what they marked. Candidates, none confirmed: that the timer is the
radial's value from the previous tick (the machine runs first), or that `0.3` is a `double` literal.

<!-- header lines 68 -->
```cpp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
```

<!-- header lines 100 -->
```cpp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
```

<!-- header lines 145 -->
```cpp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
```

<!-- header lines 182 -->
```cpp
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
```

<!-- header lines 867 -->
> [Task 39] SerializableFields specializations for dAttackMachineSimulation types.

The six marked lines, pre-conversion:

<!-- header lines 333 -->
```cpp
else if (attackState.attackTimer/*!sic*/ > 0.3)
```

<!-- header lines 339 -->
```cpp
if (attackState.attackTimer/*!sic*/ > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
```

<!-- header lines 469 -->
```cpp
else if (attackState.attackTimer/*!sic*/ > 0.3)
```

<!-- header lines 475 -->
```cpp
if (attackState.attackTimer/*!sic*/ > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
```

<!-- header lines 772 -->
```cpp
else if (attackState.attackTimer/*!sic*/ > 0.3)
```

<!-- header lines 779 -->
```cpp
if (attackState.attackTimer/*!sic*/ > 0.3 && state.m_queuedAttackSequence == InvalidAttackSequenceId)
```

---

## 13. R0 register — every false claim, and what is true

Verified against the tree on 2026-09-23.

| # | pre-conversion lines | the claim | what is true |
|---|---|---|---|
| 1 | 31-32 | the Hadouken sentinel lives at file scope in `DAttackRadialSimulation.h` | it is in `DAttackSequenceId.h` |
| 2 | 40-41 | "T2 threads the real External; T1 gates on a false placeholder" | `integrate3` reads the real signal as a plain parameter; only the legacy variants keep the placeholder |
| 3 | 41-42, 378, 514, 850 | `HitFlinch` dwells for `kHitFlinchDuration` | it dwells for `State::m_flinchDuration`, copied from the routed reaction |
| 4 | 41 | the veto transitions `Idle`/`Attacking` → `HitFlinch` | `integrate3`'s veto fires from any state |
| 5 | 21 | `BrawlerInboundHit.h` is zero-dependency | it includes `glm/vec2.hpp` and `HitReaction.h`; it reaches nothing that includes this header |
| 6 | 89 | `(0,0,1)` is "forwards" | it is `defaultUp`; forward is `(1,0,0)` |
| 7 | 89-90 | a `(0,0,0)` aim would reach `normalize()` (as what separates the two) | every read of this slice's aim XY-projects first, where both are the zero vector |
| 8 | 153-154 | 42 × (1/60) lands BELOW `0.7f` | it lands exactly on `0.7f`; the duration is `0.70000005f` |
| 9 | 197-198 | `m_attackEndTick` is written only on radial edges, at three sites | three sites, two of them radial edges |
| 10 | 589, 611 | the inbound slice is set by the manager's routing pass | by `brawlerHitRouting::System::postIntegrate` (`preIntegrate` of the consuming tick since task 20) |
| 11 | 595-596 | after the veto the switch lands in `HitFlinch` | not when a projectile block arrives on the same tick (§7.3) |
| 12 | 758-760 | a dual-tap write would push the end later while the radial keeps its schedule | true only when the active sequence is already 4 (§8.4) |

Also found: two references (20, 591) to `current_state.md` §D7, a file outside this repository. Their
substance is the compile error in §1.

⚠ **Task 9 left no false claim in this header.** Its four hunks (the two legacy placeholders, the
re-pointed projectile-block paragraph, the `Attacking` case's guard-block read) all describe
`hasHitGuard` as gone, which it is. None of them describes `collisionCheck` or detection inside an
`integrate`. Every row above predates task 9.

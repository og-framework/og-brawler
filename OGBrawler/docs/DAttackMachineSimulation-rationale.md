<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: wasProjectileBlockedThisTick -- RETIRED by og-attackstatetransition-cleanup task 4: the inbound-slice flag that drove the projectile-block recoil (guard G-04 of DAttackMachineSimulation-guards.md). It must NOT resolve -->
<!-- lint-external-ref: hasHitGuard -- RETIRED by og-netcode-v2-field-defects task 9: the radial State field the machine's guard-block recoil used to read. It must NOT resolve; the recoil now reads brawlerInboundHit::DerivedState::wasGuardBlockedThisTick -->
<!-- lint-external-ref: glm/vec2.hpp -- the third-party glm header vendored under og-simulation/glm; the lint indexes .h/.cpp, not glm's .hpp -->
<!-- lint-external-ref: glm/common.hpp -- the third-party glm header vendored under og-simulation/glm; the lint indexes .h/.cpp, not glm's .hpp -->
<!-- lint-external-ref: current_state.md -- an initiative workspace file (og-brawler-hit-resolution) outside this repository; quoted from the pre-conversion header, not a join this document relies on -->
# `DAttackMachineSimulation.h` — rationale

The narrative, the derivations and the provenance for the attack state machine: `DAttackState`,
`dAttackMachineSimulation::PlayerInputView`, `wantsHeldProjectile`, `wantsProjectile`,
`followUpSequenceFor` and its two follow-up ids, `followUpPressSequence`, `IntegrationUtils`,
`swingTickCount`, `State`, `Dependencies`, `launchHadouken`, the three `integrate` variants and
`dAttackStateName`. The **prohibitions** are in
`DAttackMachineSimulation-guards.md`; nothing here is a fence. Three sections carry a derivation id
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
freeze and the attack end tick), so movement includes this header. The dependency points one way:
movement → machine. Until og-syncedInput-rework task 3 movement also read this header's
`PlayerInput`, where the move stick was packed; it now reads the stick from its own `PlayerInputView`.

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

⚠ **Since og-attackstatetransition-cleanup tasks 4 and 6 (2026-10-04)** the value is `0.74167f`, 45
ticks at 60 Hz, and the constant carries `∴D-03` (§15). The purpose the quote states (hold `Attacking`
through the cast) still holds; the length now also sets the blocker's punish window and the
rapid-fire period.

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

⚠ **This section is history since og-syncedInput-rework task 4 (2026-10-03).** Task 3 replaced the
machine's input slice on the wire with `simulatableBrawler::SyncedPlayerInput`, and the machine now
reads its input through its own `PlayerInputView` (six fields, filled by `PlayerInputView::from`). Task 4 deleted
`class PlayerInput`, its `zero()`, its `SerializableFields` specialization and its `SimulationInput`
assertion, and retired guard G-01 (guards §R). What the quotes below say about the type, and the
measurements under them, describe it as it stood. Their live successors:
* the input the codecs deserialize is `SyncedPlayerInput`, whose fields are not `const` either;
* its field order is held by the descriptor-tuple `static_assert` in `BrawlerSyncedPlayerInput.h`,
  not by an initializer's argument count;
* the neutral input's `(0,0,1)` aim, and its difference from the value-initialised aim, are
  `static_assert`s in the same header (`BrawlerSyncedPlayerInput-rationale.md` §5).

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

The prohibition in that block was guard **G-01** (retired by task 4), with its R0: `(0,0,1)` is
`defaultUp`, and for this slice the normalize argument does not separate the two values.

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

⚠ **Since og-attackstatetransition-cleanup task 1 (2026-10-03)** the Hadouken write is one statement
in `launchHadouken`, reached from three routes: the matcher's trigger and the held trigger in `Idle`,
and the repeat in `Attacking` (§6.4, §6.5). Still three statements, still two radial edges.

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

⚠ **Since og-attackstatetransition-cleanup task 1** the condition is `triggeredActionId ==
kHadoukenActionId || wantsHeldProjectile(playerInput)` (§6.4), and the launch itself is
`launchHadouken`. The ordering is unchanged and now covers both: back + a held attack 2 never reaches
the classifier in `Idle`. That is what removes the forward swing the classifier gives a stick inside
its backpedal cone (`dAttackDirection::kBackpedalConeHalfAngle`, 10°).

⚠ **Since og-attackstatetransition-cleanup task 2** the two terms are one function,
`wantsProjectile(playerInput)`, because the follow-up rule (§14) asks the same question: a press that
is a projectile input is never a follow-up press. The `Idle` condition is unchanged in value.

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

⚠ **Since og-attackstatetransition-cleanup tasks 4 and 6** the window is 0.74167 s (45 ticks), long
enough for a blocker within about 2 m to punish the shot (§15).

### 6.4 The held trigger (og-attackstatetransition-cleanup task 1)

**The ruling (user, 2026-10-03).** Holding back + attack 2 fires projectiles continuously, whenever
both are held and the character is free: `Idle`, or the previous projectile's commitment has ended.
Back + a held attack 2 never gives a forward swing. Before this, a held attack 2 fired once (the
matcher reports a rising edge only). The machine left the commitment 18 ticks later (the 0.3 s
commitment of that time), and on the next
tick `Idle` handed the still-held button to the classifier, which calls a stick within 10° of
dead-back a forward swing (sequence 4).
`DAttack.AttackTransitions.HeldBackAndAttack2RefiresProjectile` was seen failing on that code with
exactly that trace: one shot on tick 1, `Idle` on tick 19, sequence 4 from tick 20.

**One definition of "holding back".** `wantsHeldProjectile` is `attackRight` held and
`inputSequence::stickMatchesStep(moveDirectionWorld, aimDirection, kBackShortcutStep, epsilon)`.
`kBackShortcutStep` (`GameMotions.h`) is the only step of motion 3 in `kGameMotions`, the matcher's
back shortcut: within π/8 of dead-back, aim-relative. `inputSequence::stickMatchesStep` is the
per-entry test `inputSequence::matchSequence` applies, extracted so both call one function. So the
first shot (the matcher's edge) and every later one (the machine's hold) test the same cone. The
classifier's 10° backpedal cone answers a different question (which swing) and is not used here. The
prohibitions are guard **G-08**.

**Why the epsilon and not the deadzone.** The matcher runs on the client with
`g_moveStickDeadzone`. The machine runs on every peer and in every resimulation, so it may read only
the wire input and static data; a tweakable can differ between peers. The client already zeroes
`moveDirectionWorld` below its deadzone, so the machine only has to reject that zero, which
`dAttackDirection::kMoveMagnitudeEpsilon` does.

**Both routes, one launch.** On the press tick the matcher's trigger and the held test are usually
both true; either one enters the same `launchHadouken`, once.

**Pins.** `DAttack.AttackTransitions.HeldFromIdleFires` (no matcher action, held input fires on that
tick) and `DAttack.AttackTransitions.BackConeIsTheMatchersCone` (15° off dead-back fires, 30° gives
the classified side swing; both sides).

### 6.5 The repeat (og-attackstatetransition-cleanup task 1)

In `Attacking`, when the active sequence is the sentinel, the commitment has ended (the same
`m_timeInCurrentState < kHadoukenCommitmentSeconds` predicate, false) and `wantsHeldProjectile` is
true, the machine fires again: `Attacking -> Attacking`, logged as
`[Machine.transition] Attacking -> Attacking (Hadouken repeat, projectile spawn requested)`.

* **Before the exit.** The repeat and the "radial finished → `Idle` / queued" exit are one `if` /
  `else if`. A reorder is not a line move; the structure holds it, so it has no tag. When attack 2 is
  released, or the stick leaves the cone, the `else if` runs exactly as before: exit to `Idle` on the
  commitment's last tick, then the classifier on the next.
* **The fire rate is the commitment.** One shot every `swingTickCount(kHadoukenCommitmentSeconds, dt)`
  ticks: 18 at 60 Hz. There is no separate knob.
  ⚠ **Since tasks 4 and 6** that is 45 ticks at 60 Hz, about 1.33 shots per second (§15).
* **No shot is dropped at the shipped tuning.** The pool has 3 slots and a shot lives 0.875 s. A slot
  spawned on tick S ends on tick S + 53 (`52 × dt` is under 0.875 s, `53 × dt` is not), and the
  projectile integrate ends expired slots before it consumes the spawn request on the same tick. At
  the 45-tick period the next shot spawns on S + 45 while the first is still alive, so two shots are
  in flight for 8 ticks; the shot after that, on S + 90, finds the first slot free. The pool never
  holds more than two. Pinned by `DAttack.AttackTransitions.NoShotDroppedAtDefaultPool` (10 shots,
  every one spawned, no `[Projectile.poolFull]`, slots reused). A slower commitment or a shorter
  lifetime only adds slack; a faster commitment or a longer lifetime can drop shots, and that case
  will fail. The period and the lifetime are independent values (§15).
* **The end tick is rewritten on every shot**, by the same D-01 statement (§8.1).
* **The queued sequence is not touched, and is invalid on both routes.** Under the sentinel the
  radial's `attackTimer` is 0: `deactivate` reset it when the last swing ended, and the sentinel path
  returns before advancing it. So neither `attackTimer > 0.3` queue can form. A dual press under the
  sentinel takes the `attackTimer < 0.1` restart branch instead and overwrites the sentinel before the
  repeat test reads it. That path is og-attackstatetransition-cleanup task 2's.
  ⚠ **Since task 2** a dual press is handled on a real swing only (guard **G-10**), so under the
  sentinel neither dual-press branch runs, and the follow-up rule cannot map the sentinel (§14).
  Measured before task 2: the press tick went `Idle` (the exit ran on the same tick, its commitment
  test reading the 4 that had replaced the sentinel), and sequence 4 started on the next tick. With
  the stick back as well, `Idle` fired again on that next tick: a shot every second tick.
* **Priority.** The inbound-hit veto and the projectile-block recoil run before the switch (§7), so a
  hit or a block on the tick the commitment ends wins. The swing's guard-block recoil clears the
  active sequence first, so the sentinel test fails.
  ⚠ **Since task 4** there is no projectile-block recoil (§7.2): a block during the stream leaves it
  on schedule (`DAttack.AttackTransitions.ProjectileBlockDoesNotFlinchShooter`).
* **Movement.** The character stays in `Attacking` for the whole stream, so movement stays locked
  (`machineLocksMovement`), as it is for one projectile.

Pins: `DAttack.AttackTransitions.HeldBackAndAttack2RefiresProjectile` (7 shots, one every period,
never a real sequence, end tick = shot tick + period) and `DAttack.AttackTransitions.ReleaseStopsFire`
(release after a repeat: `Idle` exactly one period after the last shot; stick out of the cone: no shot,
sentinel left on the commitment's last tick).

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

⚠ **Since og-attackstatetransition-cleanup task 4 (2026-10-04) there is one recoil, not two.** The
user ruled (2026-10-03) that a blocked projectile does not recoil its shooter. Task 4 deleted the
transition, its input `wasProjectileBlockedThisTick` and the routing loop that set it. G-04 is
retired with its quote (guards §R). A block now only ends the slot with `endReason` 4; the shooter's
machine is unaffected, and the punish window comes from the commitment instead (§15). The swing's
recoil above is unchanged, 18 ticks at both shipped `dt` values, pinned by
`DAttack.AttackTransitions.MeleeGuardBlockRecoilIsUnchanged`.

### 7.3 A hit and a projectile block on the same tick

Found by reading the code, not by running it. Routing sets `wasHitThisTick` (branches 2 and 3) and
`wasProjectileBlockedThisTick` (branch 4) independently. When both are set for one character on one
tick, the veto moves the machine to `HitFlinch` and writes the hit's reaction and dwell. Then the
projectile-block test passes (`HitFlinch` is not `GuardFlinch`) and moves it to `GuardFlinch`. The
switch lands in `GuardFlinch`, not in `HitFlinch` as §7.1's quote says, and the character recoils for
the fixed 0.3 s instead of taking the hit's reaction. Whether that precedence is intended is not recorded anywhere.
It is routed, not changed.

⚠ **Moot since og-attackstatetransition-cleanup task 4:** there is no projectile-block transition
any more, so a hit on a block tick stands, with its own reaction and dwell.

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
⚠ Since og-attackstatetransition-cleanup tasks 4 and 6 that is 45 ticks (§15); the derivation does
not depend on the length, and the case asserts the relation, not a count.

⚠ **Since og-attackstatetransition-cleanup task 1** the tagged statement is in `launchHadouken`, and
the repeat (§6.5) is the third route to it, after the matcher's trigger and the held trigger. The
derivation holds for the repeat unchanged: the repeat resets `m_timeInCurrentState` to 0 on its own
shot tick T, and the radial was already invalid on T (the sentinel never left). So the next shot, or
the exit, is on T + `swingTickCount(kHadoukenCommitmentSeconds, dt)`: the value written.
`DAttack.AttackTransitions.HeldBackAndAttack2RefiresProjectile` pins the write on every shot, and
`DAttack.AttackTransitions.ReleaseStopsFire` pins the exit after a repeat.

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

⚠ **Since og-attackstatetransition-cleanup task 2** the branch is reached only when the active
sequence is a real swing (guard **G-10**). That removes two paths into it, the projectile sentinel and
a guard-block tick, and leaves the side-swing path above as it was.

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
| 11 | 595-596 | after the veto the switch lands in `HitFlinch` | not when a projectile block arrives on the same tick (§7.3); moot since og-attackstatetransition-cleanup task 4 |
| 12 | 758-760 | a dual-tap write would push the end later while the radial keeps its schedule | true only when the active sequence is already 4 (§8.4) |

Also found: two references (20, 591) to `current_state.md` §D7, a file outside this repository. Their
substance is the compile error in §1.

⚠ **Task 9 left no false claim in this header.** Its four hunks (the two legacy placeholders, the
re-pointed projectile-block paragraph, the `Attacking` case's guard-block read) all describe
`hasHitGuard` as gone, which it is. None of them describes `collisionCheck` or detection inside an
`integrate`. Every row above predates task 9.

---

## 14. The same-side follow-up (og-attackstatetransition-cleanup task 2)

### 14.1 The rule

**The rulings (user, 2026-10-03).** Two swings on the same side in a row make the second one the
fast follow-up, whichever attack button is pressed. A projectile never leads into a follow-up: the
first side swing after one is the normal swing, and a projectile input (back + attack 2) pressed
during a swing is not a follow-up press.

In `integrate3`'s `Attacking` case, when the radial's `attackTimer` is past 0.3 and nothing is
queued yet, the machine queues `followUpSequenceFor(attackIntialConditions.activeAttackSequence,
followUpPressSequence(playerInput))` when that is not `InvalidAttackSequenceId`.

* **`followUpPressSequence`** is the side this tick's press asks for. With no button held it is
  `InvalidAttackSequenceId`. With a button held it is `dAttackDirection::classify`'s answer, the
  same classifier the `Idle` entry uses (guard **G-05** there, guard **G-09** here), except that a
  projectile input (`wantsProjectile`, §6.1) is `InvalidAttackSequenceId`. Which button is held does
  not enter it.
* **`followUpSequenceFor`** maps (active, pressed) to the follow-up. The right swing
  (`dAttackDirection::kRightSequenceId`, 0) and its follow-up (`kRightFollowUpSequenceId`, 2) with a
  right press give 2. The left swing (`kLeftSequenceId`, 1) and its follow-up
  (`kLeftFollowUpSequenceId`, 3) with a left press give 3. Everything else gives
  `InvalidAttackSequenceId`: the opposite side, a forward press, the forward swing (4), no swing, and
  `kHadoukenSequenceSentinel`. Two `static_assert`s beside the function hold the whole table over
  the ids 0–4 and both sentinels.
* **A follow-up repeats.** The follow-up's own id is in the active column, so holding a side keeps
  chaining 2 → 2 or 3 → 3.
* **The sentinel cannot map**, so a side swing that follows a projectile always starts from `Idle`
  through the classifier, as 0 or 1, and the next same-side press chains from there.
* **A projectile input is no press.** During a swing it queues nothing. The swing ends, the machine
  goes `Idle`, and on the next tick `Idle` fires the projectile (§6.4). Between the classifier's 10°
  backpedal cone and the matcher's π/8 cone, `classify` calls the stick a SIDE, so without the
  exclusion that press would chain into the same side's follow-up.
* **Everything that is not a follow-up stays unbuffered**, as before: an opposite-side or forward press
  queues nothing, and a still-held button starts the classified swing from `Idle` once the current
  swing has ended.
* **The dual press keeps its priority.** Its `attackTimer > 0.3` branch, which runs first, queues 4,
  and the follow-up test then sees a queue and does nothing. Both buttons on a real swing past 0.3 s
  still give sequence 4.

The log line keeps its shape, `[Machine.Attacking] queue seq=N (…,timer>0.3,ic=M)`. The button letter
it used to print (`L` for 2, `R` for 3) is now both buttons' levels, `L=l R=r`, because the button no
longer decides the sequence.

**What it replaced.** The test was keyed on the button: `attackLeft` with an active 0 or 2 queued 2,
`attackRight` with an active 1 or 3 queued 3. That is a leftover from when the button chose the side.
Since `dAttackDirection::classify` chooses it from the stick, a left swing (1) chained only on attack 2
and a right swing (0) only on attack 1. Several PIE sessions of 2026-10-01 to 10-03 logged one side's chain
only; the largest, 87 `queued seq=2` lines and no `queued seq=3` (counted 2026-10-03). `DAttack.AttackTransitions.LeftSwingFollowUpWithAttack1` was seen
failing on that code as swings `{ 1, 1, 1, 1, 1 }`: every left swing restarted from `Idle`.

### 14.2 The authored labels disagree — R0 finding, not fixed

The pairing 0 → 2 and 1 → 3 is the pairing the authored data makes. The names given to those ids do
not agree with each other:

| id | `SimulatableBrawlerTypes.h`, sequence list | `SimulatableBrawlerTypes.h`, hit-reaction list | `DAttackDirectionClassifier.h` | the authored angle |
|---|---|---|---|---|
| 0 | "from idle to leftAttack" | "right" | `kRightSequenceId` | rises, −4π/8 → 3π/8 |
| 1 | "from idle to rightAttack" | "left" | `kLeftSequenceId` | falls, 4π/8 → −3π/8 |
| 2 | "from leftAttack to leftAttack" | "left -> left" | (none) | rises, 4π/8 → 17π/8 |
| 3 | "from rightAttack to rightAttack" | "right -> right" | (none) | falls, −4π/8 → −17π/8 |
| 4 | "from idle to rightAttack (vertical)" | "forward/overhead" | `kForwardSequenceId` | about Y, not Z |

Read from the tree on 2026-10-03. The sequence list names 0 and 2 the same side ("leftAttack"), and
the angle of 0 and of 2 rises while that of 1 and of 3 falls. So 2 continues 0's sweep and 3
continues 1's, and the button-keyed rule paired them the same way. The hit-reaction list calls 0
"right" and 2 "left -> left", and the classifier calls 0 right. Nothing was renamed. The lead raises
the labels with the user.

### 14.3 The dual press under the sentinel, and on a guard-block tick

Guard **G-10**. Before task 2 a dual press during the projectile commitment took the restart branch
(the radial's `attackTimer` is 0 under the sentinel), replaced the sentinel with 4 and ended the
commitment on that tick; §6.5 has the measured trace. The same gate also stops a dual press on a
guard-block tick from writing 4 into a character already in `GuardFlinch`. Past 0.3 s that 4 was
queued, survived the recoil and `Idle`, and the next swing chained into 4 instead of its own
follow-up. Both were measured by deleting the gate (the guard's R0).

### 14.4 Pins

All in `AttackStateTransitionTest.cpp`, tags `[DAttack][AttackTransitions]`:
`LeftSwingFollowUpWithAttack1`, `RightSwingFollowUpWithAttack2`, `PreviouslyWorkingFollowUpsStillChain`,
`OppositeOrForwardPressIsNotAFollowUp`, `FirstSwingAfterProjectileIsNormal`,
`ProjectileInputMidSwingIsNotAFollowUp`, `DualPressCannotHijackProjectile`,
`FollowUpPressIsTheClassifiedSideWithoutProjectileIntent` and
`DualPressOnAGuardBlockTickQueuesNothing`. Each one's pre-change result is in
`AttackStateTransitionTest-rationale.md` §4.

---

## 15. The Hadouken commitment is the punish window (og-attackstatetransition-cleanup tasks 4 and 6) ∴D-03

**The rulings.** A blocked projectile must not recoil its shooter (§7.2; guards §R, G-04). Instead
every shot locks its shooter long enough for a blocker close by to punish it (user, 2026-10-03,
task 4). Task 4 set `kHadoukenCommitmentSeconds` to 0.875 s (53 ticks), sized for a block at the 3 m edge
of swing reach. The user then shortened it to `0.74167f`, 45 ticks (user, 2026-10-04, task 6), so the
guaranteed punish now reaches a block within about 2 m. The cost: the rapid-fire stream (§6.5) fires
one shot per 45 ticks, about 1.33 per second (it was one per 18 ticks before task 4).

**Derivation.** The lock must outlast, from the shot tick, the projectile's flight to the blocker, plus
the blocker's reaction, its input delay and its swing up to the hit:

`commitment ticks ≥ k + D_in + r + f` = 24 + 6 + 10 + 5 = 45

* **k = 24**, swing start to first hit on a target the blocker aims at. Hit detection registers a
  target only while the weapon is in the authored segment that holds the target's direction, so k is
  the start time of the segment holding the aim (local angle 0), counted with `swingTickCount`. For
  the side swings (sequences 0 and 1) that segment starts at keyframe time 0.4 s: k = 24. For the
  forward swing (sequence 4) it starts at 0.3 s: k = 18. The slower side swing binds. Distance
  decides whether the swing reaches at all (90 to 300 cm), never when it hits.
* **D_in = 6**, the input delay. An input captured on tick c is applied on c plus the larger of the
  session floor and the connection tier's delay (`ConnectionTierTable.h`, `ServerInputDelayQueue.h`).
  The shipped floor is `RelayDelayFloorTicks=6` in `Config/DefaultEngine.ini`, so every player's
  press lands 6 ticks late. It is a config value, not a sim constant: the pin names it as an
  assumption and does not read the config.
* **r = 10**, the reaction budget: 167 ms from seeing the block to pressing.
* **f = 5**, the flight to the farthest block the lock guarantees. The shot spawns 60 cm ahead of its
  shooter and moves 800 cm/s, 13.33 cm per tick. Its sphere (radius 40) meets the blocker's guard
  sphere (radius 40, 30 cm above the capsule centre) roughly 64 to 77.5 cm before the blocker's
  centre. So a block f ticks after the shot happens with the blocker at about
  60 + 13.33·f + (64 to 77.5) cm: for f = 5, 1.9 to 2.0 m.

The numbers k, D_in and the flight formula come from og-attackstatetransition-cleanup task 3's
research (its §5 and §6). r is the user's choice (task 3's option B: 40 ticks from the block). The
total, 45, is the user's (task 6). f = 5 is the lead's split of it, keeping k, D_in and r; the user
may revise it.

**Beyond 2 m the reaction budget shrinks.** Each tick of flight past 5 comes out of r: a side-swing
punish needs r ≤ 15 − f ticks, a forward-swing punish r ≤ 21 − f. At the 3 m edge of swing reach
(f = 13) that leaves 2 ticks for a side swing and 8 for a forward swing: possible only when the
blocker presses before it sees the block. Beyond about 3 m a blocker standing still cannot reach the
shooter, whatever the lock.

**The literal is a half tick.** 45 ticks exactly would be 45/60 = 0.75 s. A literal on a tick
boundary can come out one tick off under float accumulation: 0.3 = 18/60 gives 18 only because the
float sum overshoots. `0.74167f` is 44.5/60 to within a few microseconds and sits half a tick from
both neighbours, so `swingTickCount` returns 45 at `dt` = `1.f / 60.f` and at the engine's fixed step
0.016667. `DAttack.AttackTransitions.CommitmentIs45Ticks` pins it: one shot holds `Attacking` for
exactly 45 ticks at both `dt` values, its end tick is shot tick + 45, and the rapid-fire period is 45.

**The pin.** `DAttack.AttackTransitions.CommitmentLeavesThePunishWindow` computes k from the shipped
`StaticData` with no physics: for sequences 0, 1 and 4, the segment that holds local angle 0
(`DAttackRadialSequence::getAttackSegment`), its start keyframe's time
(`DAttackRadialSequence::getAttackPointTime`), counted with `swingTickCount`. It takes the slowest k
and checks the commitment against k + 6 + 10 + 5 at both `dt` values. It measures k = 24, 24 and 18.
The sum is met exactly (45 ≥ 45), with no slack: a commitment one tick shorter (`0.725f`, 44 ticks)
fails it, and so does a side-swing wind-up one tick longer. Retuning a swing's keyframes, or this
constant, re-runs the sum.

**What else the length drives.** The same constant is the D-01 end tick (§8.1), so the movement lock
of a shot lasts 45 ticks; the rapid-fire period (§6.5); and the exit to `Idle` after a single shot.

**Independent of the projectile lifetime.** `brawlerProjectileSimulation::StaticData::maxLifetime` is
0.875 s (a slot lives 53 ticks). One is how long the shooter is locked, the other how far the shot
flies. They are different values, and neither may be derived from the other. With the lock shorter
than the lifetime, two of a stream's shots are in flight at once for 8 ticks (§6.5).

**Range.** Only blocks within about 2 m are guaranteed punishable ("Beyond 2 m" above). Task 3's
research found blocks happening out to about 8 m, and put the eight blocks in its PIE logs at an
estimated 3.1 to 4.6 m. A blocker beyond swing reach cannot close the gap in time, and no reach fix
is planned (user, 2026-10-03).

**No combo after a projectile hit (user, 2026-10-04).** A point-blank projectile HIT does not leave
its shooter time to land a side swing before the target's 0.65 s stun (`m_projectileHitReaction`,
hand-authored) ends. Brawler-movement-simulation task 87 chose the 0.65 s for that combo, when the
lock was 0.3 s. The swing connects 19 ticks after the shooter goes `Idle`: with the shot on tick 1
and a lock of L ticks, on tick L + 20. The stun is over on tick 41, so the combo needs L ≤ 20. With
the 45-tick lock the shooter is `Idle` on tick 46 and its swing connects on tick 65, 24 ticks late
(measured by task 6 with the retired rig; with 53 ticks it was tick 73, 32 late). The user dropped
the combo under task 4 and kept the 0.65 s stun. The case that pinned the combo,
HitRouting.ProjectilePointBlankFollowUpWindow, lost its follow-up half; its shot-only half is now
`HitRouting.ProjectilePointBlankHitStunsOnce`. No test asserts that the combo is impossible: the two
numbers that rule it out are pinned absolutely instead
(`DAttack.AttackTransitions.CommitmentIs45Ticks`, and the 0.65 line in
`HitRouting.ProjectileHitStunsInPlace`). Bringing the combo back means re-choosing one of them.

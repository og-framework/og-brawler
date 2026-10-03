<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerSyncedPlayerInput.h` — guards

Prohibitions with a tagged site in `BrawlerSyncedPlayerInput.h`. Each entry has an **opaque, stable
id**. In the header, a single line `// ⛔G-nn` sits exactly where the wrong edit would be typed.

**Since og-syncedInput-rework task 4 (2026-10-03) this header has no live guard.** Both entries
became `static_assert`s at the foot of the header and are in [§R](#r-retired-ids).

**If this file and `BrawlerSyncedPlayerInput.h` disagree, the header is authoritative and this file
is stale.** Fix this file. Do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or run-time
check, moves to [§R, Retired ids](#r-retired-ids) and its number is spent forever.

⛔ **Nothing in this file is a rationale.** The byte map, the field order and the recipe for adding
a view live in `BrawlerSyncedPlayerInput-rationale.md`.

**Not here, because the compiler holds them:** the field order and count (the descriptor-tuple
`static_assert`), the 39-byte size, `SimulationInput` and trivial copyability, the view contract
`simulatableBrawler::BrawlerInputView`, and, since task 4, the `(0,0,1)` aim of `zero()` and its
difference from the value-initialised aim (retired G-02 and G-01).

**Provenance.** Written new by og-syncedInput-rework task 2 (2026-10-03). The two entries carry the
prohibitions of three predecessor entries on the per-sub-simulation input types that this struct
replaces on the wire: `DAttackRadialSimulation-guards.md` G-01 and G-02, and
`DAttackMachineSimulation-guards.md` G-01, including those entries' R0 corrections. Task 4 deleted
those types and retired the three predecessor ids, and retired both entries here in the same change.

---

## §R Retired ids

⛔ **Spent forever.** Neither number may appear as a `⛔G-nn` tag again. Each may be named in prose or
in a `static_assert` message, which is what the two assertions that replaced them do.

---

### G-01 — RETIRED (og-syncedInput-rework task 4): converted to a compile-time check

**Was:** *`aimDirection` keeps a `(0,0,0)` default*. The entry below is the text as task 2 and
task 3 left it; the tag it describes was removed with this retirement.

**Tag site:** `BrawlerSyncedPlayerInput.h`, on `glm::vec3 aimDirection{};` in `SyncedPlayerInput`.

**The prohibition.** Do not give `aimDirection` a default member initialiser of `(0,0,1)`, or of any
value equal to the aim `SyncedPlayerInput::zero()` returns. That would make `SyncedPlayerInput{}`
equal to `zero()`.

**What breaks if the edit is made.** The neutral input and a value-initialised input become the
same value, so every assertion that tells "the game's zero" apart from a default-constructed input
loses the field it discriminates on.
* Today, `SimulatableBrawler.SyncedPlayerInput.ZeroIsNotValueInitialised` in
  `SyncedPlayerInputTest.cpp` goes red. Witnessed 2026-10-03: with the default set to
  `glm::vec3(0.f, 0.f, 1.f)` the build succeeds and that case fails on
  `SyncedPlayerInput{}.aimDirection == glm::vec3(0.f, 0.f, 0.f)`.
* Once `simulatableBrawler::PlayerInput` is this struct, the resolution and net-sync suites'
  `isGameZeroInput` checks compare the whole aim against `getZeroPlayerInput()`'s. Their
  `REQUIRE_FALSE(isGameZeroInput(simulatableBrawler::PlayerInput{}))` controls then fail, and the
  positive checks keep passing while testing nothing.

**Predecessor.** `DAttackRadialSimulation-guards.md` G-02, the same prohibition on the radial
slice's aim.

**Score (§9.1): substitution on the tagged token → `yes`.**

**Now enforced by**, at the foot of `BrawlerSyncedPlayerInput.h`,
`static_assert(simulatableBrawler::SyncedPlayerInput{}.aimDirection != simulatableBrawler::SyncedPlayerInput::zero().aimDirection, …)`.
Its message ends *"Was guard G-01 of BrawlerSyncedPlayerInput-guards.md."* The forbidden edit now
fails the build instead of turning a test red. Witnessed 2026-10-03: with the default set to
`glm::vec3(0.f, 0.f, 1.f)`, OGBrawlerTests stops with `error C2338` on that assertion and on no
other line.

⛔ **Why it could retire.** Task 2 kept this a guard because the three vector defaults were spelled
`{}`, and a value-initialised `glm` member read in a constant expression is `C2131`. Task 4 respelled
them `glm::vec3(0.f)` / `glm::vec2(0.f)` (the same values) and made `zero()` `constexpr`. Measured on
the OGBrawlerTests build: respelling `aimDirection` back to `{}` is `error C2131` on this assertion,
so that edit is loud too. Respelling `moveStick` alone back to `{}` compiles, because neither
assertion reads it.

⚠ **R0 on the text above (2026-10-03).** "Today, … goes red" and "the positive checks keep passing"
describe a build that compiles. With the assertion in place the forbidden edit no longer compiles,
so the case and the controls never run. The cases are still the run-time pins for everything else
they check.

---

### G-02 — RETIRED (og-syncedInput-rework task 4): converted to a compile-time check

**Was:** *`zero()` carries `(0,0,1)`, never `SyncedPlayerInput{}`*. The entry below is the text as
task 2 and task 3 left it; the tag it describes was removed with this retirement.

**Tag site:** `BrawlerSyncedPlayerInput.h`, inside `SyncedPlayerInput::zero()`, on its `return`
statement.

**The prohibition.** Do not make `zero()` return `SyncedPlayerInput{}`, and do not change the aim it
returns from `glm::vec3(0.f, 0.f, 1.f)`. The value is a wire value, not something to re-derive: it is
the neutral input the legacy composite serialized. The machine slice's `zero()` aim is
`(0,0,1)` and the movement slice's `flags` is 0. Until og-syncedInput-rework task 3, task 2's case
ZeroBytesEqualTheLegacyMachineSliceAndMovementByte checked that the 39 bytes of `zero()` equal those
slices' bytes in the composite's `getZeroPlayerInput()`. Task 3 deleted that case with the composite
(2026-10-03). `simulatableBrawler::PlayerInput` is now this type, which the `is_same` `static_assert`
in `SyncedPlayerInputTest.cpp` holds, so `getZeroPlayerInput()` returns this `zero()`.

**What breaks if the edit is made.**
* The bytes every peer fills a tick with when it has no input change. Peers on different builds then
  disagree about the neutral input.
* `SimulatableBrawler.SyncedPlayerInput.ZeroSerializesToTheCapturedBytes` goes red: it pins the 39
  bytes of `zero()` exactly.
* Returning `SyncedPlayerInput{}` also removes the anti-vacuity gap described in G-01.

**R0 corrections carried from the predecessors (re-verified 2026-10-03).**
* `(0,0,1)` is **+Z**, not "forwards". `DAttackRadialSequence::defaultForward()` is `(1,0,0)`.
* Neither `(0,0,1)` nor `(0,0,0)` is the safer aim for a reader, so that is not why the prohibition
  exists. No reader uses the raw aim: the machine (`setRadialSimulationInitialConditions`, the
  Hadouken spawn and `dAttackDirection::classify`) and the guard (`dAttackGuardSimulation::integrate`)
  all project it onto XY first, where both values are the same zero vector. The guard substitutes its
  default forward below a length of 1e-4. The prohibition stands on the other two reasons: the wire
  value and the test tag.
* ⚠ **Corrected 2026-10-03:** `DAttackMachineSimulation-guards.md` G-01 says the neutral input
  "presses no attack button, so it reaches none of those reads". That is false for the queued chain.
  The chain branch of `integrate3`, which starts `m_queuedAttackSequence` when the current
  sequence ends, calls `setRadialSimulationInitialConditions` whatever this tick's buttons are, and
  that function normalizes the aim's XY projection with no length check. A neutral-filled tick at
  the end of a swing with a queued chain therefore reaches it. Both candidate aims project to the
  same zero vector there, so the choice between them still changes nothing for that reader.
* The predecessors say a `static_assert` cannot be written because `glm::vec3` has no `constexpr`
  constructor. That is not quite what was measured on 2026-10-03, under the OGBrawlerTests compile
  flags: `glm::vec3(0.f, 0.f, 1.f)` is a constant expression. A **value-initialised** `glm::vec3{}`
  or `glm::vec2{}` member is not (`C2131`). This struct spells its three vector defaults `{}`, so
  neither `zero()` nor `SyncedPlayerInput{}` is constant-evaluable as written, and both prohibitions
  are guards rather than assertions. See rationale §5.

**Predecessors.** `DAttackRadialSimulation-guards.md` G-01 and `DAttackMachineSimulation-guards.md`
G-01, the same prohibition on those slices' `zero()`.

**Score (§9.1): substitution on the tagged statement → `yes`.**

**Now enforced by**, at the foot of `BrawlerSyncedPlayerInput.h`,
`static_assert(simulatableBrawler::SyncedPlayerInput::zero().aimDirection == glm::vec3(0.f, 0.f, 1.f), …)`.
Its message ends *"Was guard G-02 of BrawlerSyncedPlayerInput-guards.md."* It refuses both edits the
entry names: a `zero()` that returns `SyncedPlayerInput{}` has the `(0,0,0)` aim, and any other aim
fails the comparison. Witnessed 2026-10-03: with `zero()`'s aim set to `glm::vec3(0.f, 1.f, 0.f)`,
OGBrawlerTests stops with `error C2338` on that assertion and on no other line.

⛔ **Why it could retire.** The same respelling as G-01: `zero()` is `constexpr` and every member of
`SyncedPlayerInput` has a constant initialiser, so `zero()` is a constant expression.

⚠ **R0 on the text above (2026-10-03).** Its last paragraph said both prohibitions are guards
because the vector defaults are spelled `{}`. That stopped being true with this retirement. The
other corrections it carries (+Z, the projection to XY, the queued-chain read) are facts that forbid
no edit, and are kept in `BrawlerSyncedPlayerInput-rationale.md` §5.

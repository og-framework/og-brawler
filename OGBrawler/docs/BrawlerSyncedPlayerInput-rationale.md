<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerSyncedPlayerInput.h` — rationale

Companion to `OGBrawler/BrawlerSyncedPlayerInput.h`. Its guards doc,
`BrawlerSyncedPlayerInput-guards.md`, holds no live entry since og-syncedInput-rework task 4: both
prohibitions are `static_assert`s (§5).

## 1. What the struct is

`simulatableBrawler::SyncedPlayerInput` is the per-tick input of one brawler character: every input
field once, in one flat struct. It is built to be the value the input codecs carry (the client's
redundancy bundle to the server, and the server's relayed input ring to the other clients), the
value the input resolution fills a missing tick with (`zero()`), and the value each sub-simulation
reads its own fields from. The status line below says how much of that is true yet.

The sub-simulations are not meant to read the struct directly. Each one declares a small `PlayerInputView` holding
only the fields it needs, and fills it with a `static PlayerInputView from(const SyncedPlayerInput&)` in its
own header (§6). `simulatableBrawler::BrawlerInputView` is the contract such a view must meet: it
has that `from()`, it is **not** `Serializable` (a view is never put on the wire), and it is trivially
copyable.

**Status, 2026-10-03.** Since og-syncedInput-rework task 3, `simulatableBrawler::PlayerInput` is
this type (`using PlayerInput = SyncedPlayerInput;` in `SimulatableBrawlerTypes.h`): the input codecs
carry it, the input resolution neutral-fills with `zero()`, and each of the six sub-simulations reads
it through its own `PlayerInputView`. Task 2 had added the struct with no production user, while the wire
still carried the six-slice composite (one slice per sub-simulation) that task 3 replaced.

This header includes no sub-simulation header. It is the list of available inputs, and every
sub-simulation header can include it without a cycle.

## 2. The byte map

The serializer writes the fields in declaration order with no padding (`syncSize` sums the field
sizes):

| bytes | field | type |
|---|---|---|
| 0–11 | `aimDirection` | `glm::vec3` |
| 12 | `attackLeft` | `bool` |
| 13 | `attackRight` | `bool` |
| 14–21 | `moveStick` | `glm::vec2` |
| 22–33 | `moveDirectionWorld` | `glm::vec3` |
| 34–37 | `triggeredActionId` | `uint32_t` |
| 38 | `flags` | `uint8_t`, bit 0 = `brawlerMovementSimulation::kInputFlagHoldGuard` |

39 bytes in all. The descriptor-tuple `static_assert` holds the order and the size `static_assert`
holds the total. The flags byte is the last one, and a holdGuard input differs from `zero()` at index
38 and nowhere else (`SimulatableBrawler.SyncedPlayerInput.FlagsByteDiscriminatesAtIndex38`).

**Changing the layout is a wire format change.** Reordering, inserting or removing a field moves
bytes that a peer on the previous build reads positionally, even when the total does not change.
Both input codecs carry a version byte that a receiver checks:
`relayedInputRing::kWireFormatVersion` and `inputRedundancyBundle::kWireFormatVersion`. A layout
change bumps both.

## 3. Why this order

The order is the legacy machine slice (the machine sub-simulation's `PlayerInput`, deleted by task 4:
aim, the two attack buttons, the move stick, the world move direction and the action id) followed by
the legacy movement slice's flags byte. Those two slices already held every field some sub-simulation reads, so the new
struct's bytes for a given input are exactly those two slices' bytes from the composite.

That was checked in-tree while the composite existed: task 2's case
"ZeroBytesEqualTheLegacyMachineSliceAndMovementByte" serialized `getZeroPlayerInput()` through the
composite and required bytes 14 to 51 (the machine slice) followed by byte 76 (the movement slice)
to equal the 39 bytes of `SyncedPlayerInput::zero()`. It passed. Task 3 removed the composite and
deleted that case; in its place `SyncedPlayerInputTest.cpp` asserts at compile time that
`simulatableBrawler::PlayerInput` is this type.

The machine slice calls the stick `moveDirection`. This struct calls it `moveStick`, the name the
input packer uses. The machine's own `PlayerInputView` keeps `moveDirection`, so the machine's read
expressions do not change.

## 4. Why flat, and not a composite of per-sim slices

* **Every field once.** The composite carried the aim four times and the two attack buttons twice.
  For a given input those copies were always equal, because the one packer wrote the same value into
  each.
* **Trivially copyable.** A `SimulationComposite` fails `std::is_trivially_copyable` through its
  `std::tuple` alone. A flat struct of plain members passes, which matters to a host that reads an
  input capture slot on one thread while another thread writes it. A `static_assert` holds it.
* **It nests by field.** A `Serializable` aggregate used as a member of another serialized type is
  written field by field (`MemberFieldDesc` sizes such a member with `syncSize`, not `sizeof`).
  The composite is not `Serializable`, so it does not get that treatment.

### F1 — the bytes nobody read

In the legacy composite the radial slice (14 B) and the projectile slice (12 B) were serialized,
relayed and resimulated, and nothing read them: neither `DAttackRadialSimulation.h` nor
`BrawlerProjectileSimulation.h` calls `getPlayerInput()`. The projectile's spawn direction comes from
the machine, which reads its own aim on the Hadouken trigger and writes the projectile's initial
conditions. In this struct those 26 bytes are simply absent, and the radial and projectile views
are empty.

## 5. The neutral input's two values are assertions

Two `static_assert`s at the foot of the header hold the values that tell the neutral input apart:

* `zero()`'s aim is `(0,0,1)`. It is the wire value every peer fills a tick with when it has no
  input, so changing it, or making `zero()` return `SyncedPlayerInput{}`, makes peers on different
  builds disagree about the neutral input. The message names the guard it replaced, G-02.
* The value-initialised aim differs from `zero()`'s. The resolution and net-sync tests tell the
  neutral input from a default-constructed one by this field (`isGameZeroInput`), so equal values
  would let those controls pass while testing nothing. The message names G-01.

Until og-syncedInput-rework task 4 both were guards (`BrawlerSyncedPlayerInput-guards.md` §R), because
neither value was a constant expression. Measured on 2026-10-03, under both a standalone
`/std:c++20 /permissive-` compile and the OGBrawlerTests target's own compile flags:

* A `constexpr` variable initialised with `glm::vec3(0.f, 0.f, 1.f)` compiles, and so does a
  `static_assert` on its components or on `==`.
* A `glm::vec3` member initialised with `{}`, read inside a constant expression, is error `C2131`
  ("expression did not evaluate to a constant").
* The same member initialised with `= glm::vec3(0.f)` is a constant expression.

Task 4 spelled the three vector defaults `glm::vec3(0.f)` and `glm::vec2(0.f)` (the same values as
`{}`) and made `zero()` `constexpr`. Measured on the OGBrawlerTests build after that change:

* a default aim equal to `zero()`'s fails the inequality assertion (`C2338`), and nothing else;
* a `zero()` aim of `(0,1,0)` fails the `(0,0,1)` assertion (`C2338`), and nothing else;
* respelling the aim's default back to `{}` is `C2131` on the inequality assertion, so that edit is
  loud as well;
* respelling `moveStick` alone back to `{}` compiles, because neither assertion reads it.

The run-time pins are `SimulatableBrawler.SyncedPlayerInput.ZeroSerializesToTheCapturedBytes` (the
39 bytes of `zero()`) and `SimulatableBrawler.SyncedPlayerInput.ZeroIsNotValueInitialised` (the two
aims differ, field-wise and on the wire).

### 5.1 What the `(0,0,1)` aim is not

These corrections came with the retired guards and their predecessors on the per-sub-simulation
input types (re-verified 2026-10-03). They forbid no edit.

* `(0,0,1)` is **+Z**, not "forwards". `DAttackRadialSequence::defaultForward()` is `(1,0,0)`.
* Neither `(0,0,1)` nor `(0,0,0)` is the safer aim for a reader. No reader uses the raw aim: the
  machine (`setRadialSimulationInitialConditions`, the Hadouken spawn and `dAttackDirection::classify`)
  and the guard (`dAttackGuardSimulation::integrate`) all project it onto XY first, where both values
  are the same zero vector. The guard substitutes its default forward below a length of 1e-4.
* The neutral input does reach the machine's normalize. The chain branch of
  `dAttackMachineSimulation::integrate3` starts `m_queuedAttackSequence` when the current sequence
  ends, whatever this tick's buttons are, and calls `setRadialSimulationInitialConditions`, which
  normalizes the aim's XY projection with no length check. A neutral-filled tick at the end of a
  swing with a queued chain therefore normalizes a zero vector. Both candidate aims project to that
  same zero vector, so the choice of neutral changes nothing there. The missing length check is a
  pre-existing defect outside this struct.

## 6. Adding to the input — the recipe

**A sub-simulation that needs input:**

1. Read `SyncedPlayerInput`. It is the list of everything a player sends.
2. In your own sub-simulation header, include `OGBrawler/BrawlerSyncedPlayerInput.h` and declare
   `struct PlayerInputView` with only the fields your `integrate` reads. Give each the default it should
   have, and nothing more: no `zero()`, no `SerializableFields`.
3. Next to your `integrate`, write `static PlayerInputView from(const simulatableBrawler::SyncedPlayerInput& in)`,
   returning the view by designated initializers from the fields of `in`. A sub-simulation that reads
   no input still declares an empty `PlayerInputView` whose `from()` returns `{}`.
4. Name the view as your `Dependencies::InputType` and as the first argument of your `AllInput`
   alias. Two sub-simulations must not share one view type: the dependency validator treats each
   `InputType` as owned by its sub-simulation. `SimulatableBrawlerTypes.h` asserts the
   `BrawlerInputView` contract over every `Dependencies` entry of `ExecutionOrder`, so a view
   without `from()`, or one made `Serializable`, fails the build there.
5. `SimulatableBrawler::integrate` (`SimulatableBrawler.h`) builds your view once per tick with
   `from()` and hands it to your `integrate` through `AllInput`.

Nothing in a second file is yours to edit.

**A field that is not captured yet** (a new button or signal) means editing three places together:
this struct and its descriptor list (append it, and bump both codec versions, §2), the input packer
that fills the struct, and the engine-side builder that captures the raw value. A new on/off signal
should be a bit in `flags` rather than a new member: bit 0 is holdGuard, and bits 1 to 7 are free.

## 7. What an input byte costs

Every byte of this struct is paid once per relayed ring entry, for every remote character, in every
round the server sends. That is why input bytes are dearer than state bytes, and why §6 prefers a
flag bit to a new member. `RoundVsPacketBudgetTest.cpp` in og-brawler-tests prices the round from
`relayedInputRing::detail::entryStride<simulatableBrawler::PlayerInput>()` and pins the character
count that fits one packet. Read the current numbers there.

## 8. The pins

`SyncedPlayerInputTest.cpp` in og-brawler-tests:

| case | pins |
|---|---|
| `SimulatableBrawler.SyncedPlayerInput.ZeroSerializesToTheCapturedBytes` | the 39 bytes of `zero()` |
| `SimulatableBrawler.SyncedPlayerInput.ZeroIsNotValueInitialised` | the anti-vacuity gap between `zero()` and `{}` at run time; the two §5 assertions hold it at compile time |
| `SimulatableBrawler.SyncedPlayerInput.FlagsByteDiscriminatesAtIndex38` | the flags byte is byte 38 |
| `SimulatableBrawler.SyncedPlayerInput.RingAndBundleRoundTripEveryField` | ring and bundle round trips of a value that differs from both `zero()` and `{}` in every field |
| `SimulatableBrawler.SyncedPlayerInput.CodecStrideAndSlotSize` | the ring entry stride and the bundle slot input size, at compile time |
| `SimulatableBrawler.SyncedPlayerInput.BrawlerInputViewDiscriminates` | the view contract accepts a view with `from()` and rejects one without it, one that is `Serializable`, and one that owns memory |
| `SimulatableBrawler.SyncedPlayerInput.TestHelperMakeCarriesFlags` | the test helper `brawlerTestInputs::make` copies `flags` (an omitted designator compiles silently) |

The same file asserts, at namespace scope, that `simulatableBrawler::PlayerInput` is this type (§3).
`InputViewSpecTest.cpp` pins every sub-simulation's `from()` against the packer's arguments.

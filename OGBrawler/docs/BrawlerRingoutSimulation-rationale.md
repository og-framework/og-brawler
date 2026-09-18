<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerRingoutSimulation.h` — rationale

<!-- lint-external-ref: `Ringout.SpawnSlotOutOfRange` -- R0-8: this is the name the header CITED and that no test case has ever had. It is quoted as the defect, so it must stay unresolvable. -->
<!-- lint-external-ref: `FName::FastLess` -- engine symbol, Runtime/Core/Public/UObject/NameTypes.h, outside every scan root. -->
<!-- lint-external-ref: `FNameFastLess` -- engine symbol, same header, outside every scan root. -->
<!-- lint-external-ref: `FNameEntryId` -- engine symbol, same header, outside every scan root. -->
<!-- lint-external-ref: `FMinimalName` -- engine symbol, same header, outside every scan root. -->


Death by kill plane, and respawn after a fixed tick countdown. Prohibitions with a tagged site
are in `BrawlerRingoutSimulation-guards.md`; everything else is here.

**R0 notice.** Every factual claim below was re-measured against the tree on 2026-09-13 before it
was moved out of the header. Nine were wrong. Each one is carried **in the words it was written
in**, with the correction attached immediately after it, so the false sentence can never be read
without its correction. They are marked **⛔ R0-n**.

---

## 1. What the compiler and the suite already enforce — read this before adding a comment

This header used to carry 366 comment lines for 340 lines of code. Most of them restated a
property that was *already* machine-checked. Before anything moved, every prohibition in the file
was put to a compiler or to a red-probe run, and the result is the table below. Nothing in it was
decided by reasoning.

⭐ **The point of the table is what it says about future edits:** if you are about to add a
comment to this header, the odds are that the property is already checked and what you want is a
better assertion message.

### 1a. Already enforced by an assertion IN this header

| property | the check |
|---|---|
| `State::flags` bit 0 is `kFlagDead`, and the assignment is on the wire | `static_assert(kFlagDead == (1u << 0))` |
| `State` and `InitialConditions` are APPEND ONLY — the wire layout is positional | two `std::is_same_v` assertions on the `SerializableFields` tuples |
| the two wire slices are 4 B and 5 B | two `syncSize` assertions |
| `PlayerInput` costs nothing | `syncSize<PlayerInput>() == 0u` |
| ring-out declares valid, owned dependencies | `ValidDependencies<Dependencies>` |

### 1b. NEWLY enforced by this conversion — four prohibitions the compiler took

| prohibition | the check that replaced it | the poison that was seen to fire |
|---|---|---|
| `DerivedState` is off the wire | `!Serializable<brawlerRingout::DerivedState>` | a `SerializableFields<DerivedState>` specialization in a probe TU ⇒ `C2338` |
| the slot table is authority-side bookkeeping, not a sub-simulation type | `!Serializable<brawlerRingout::SpawnSlotAllocator>` (**G-01**) | a specialization for the allocator ⇒ `C2338` |
| ticks, never float seconds | `!RingoutStepExposesDeltaTime<IntegrationUtils>` **and** `sizeof(IntegrationUtils) == sizeof(uint32_t)` (**G-02**) | four shadow-copy arms; see the table in the guards entry |
| `kNoFreeSlot` IS `kMaxSpawnPoints` | `kNoFreeSlot == kMaxSpawnPoints` at the declaration | a shadow giving it the value `99u` ⇒ `C2338` |

### 1c. Already enforced by `BrawlerRingoutSimulationTest.cpp` — measured by injection

Each row is an edit made to this header, built, and run. The count is failing cases out of 39.

| the edit | result |
|---|---|
| `else if` in the death arm demoted to `if` | **1 red** — `Ringout.SpawnSlotOutOfRangeWritesNoTeleportSeed` |
| delete the unconditional `derived.diedThisTick = false` at the top of the step | **3 red** |
| delete the `std::sort` in `spawnPointsFromLevelPlacements` | **6 red** |
| start `merged` default-constructed instead of copying `authoredFallback` down | **2 red** |
| delete the `movementIc.teleportPending == 0u` term from the death arm | **1 red** — `Ringout.APendingTeleportSuppressesDeathDetection` |
| replace the bit-pattern tie-break with a float compare | **1 red** — `Ringout.SpawnPoints.DuplicateNamesStillOrderDeterministically` |
| remove the out-of-range branch on `spawnSlot` | **1 red** |
| `std::sort` → `std::stable_sort` | ✅ **green — inert, exactly as the header claimed** |

The execution-order edge is enforced the same way, one level up: two `static_assert`s in
`BrawlerRingoutSimulationTest.cpp` pin that ringout-before-movement validates and that
movement-before-ringout does **not**, and a runnable case re-quotes both.

### 1d. Measured and REJECTED — the negatives are half the result

* **`kMaxSpawnPoints` cannot be pinned to the packet-budget cap from here.** The derivation lives
  in a file-local `constexpr` function in `RoundVsPacketBudgetTest.cpp` and the runtime constant is
  a member of a `UCLASS`. An engine-free header can reach neither, and the low-level-test target
  cannot reach the UE module file either. The mirror stays a documented mirror — §2.
* **The comparator's TOTALITY does not convert.** `spawnOrderBits` is a `std::memcpy`, so it is not
  usable in a constant expression, and the position type is not constexpr-readable either. A
  runtime check that no two adjacent elements compare equivalent would false-fire on a level that
  legitimately places two identical actors. What covers it instead is the red probe in §1c.
* **"Owns no physics body" needs no assertion, because the mistake is already unrepresentable.**
  `Dependencies` declares no adapter and `integrate` takes none, so there is nothing to add a check
  to — §4.
* **`spawnPointsFromLevelPlacements` taking its list BY VALUE is already enforced by the
  compiler.** Changing the parameter to `const std::vector<LevelSpawnPoint>&` fails to build at the
  `std::sort` call (`C2678`, measured). The header used to explain the choice; it needs no comment,
  no entry and no check.
* **`!Serializable<brawlerRingout::StaticData>` compiles and its poison fires, and it was NOT
  shipped.** `BrawlerRingoutSimulationTest.cpp` already asserts it and no prose in this header ever
  claimed it, so adding it would have been a line with nothing to replace.

### 1e. The R0 index, and the procedure that produced it

⛔ **A stopping point is not completeness**, so the search is published rather than only its
result. Every sentence in the header was read in order and sorted into one of: a quantified claim
(a number, a count, a size, a byte figure), a named identifier (a symbol, a file, a test case), a
quotation of another file's words, or an unquantified argument. The first three classes were
re-measured against the tree one by one; the fourth was read for internal contradiction against the
rest of the same file. **Nine claims came back wrong.** Unquantified arguments that could not be
falsified this way are carried unmarked, and a reviewer attacking this audit should start there.

| | the claim | where the correction is |
|---|---|---|
| **R0-1** | "more would be entries nothing can ever index" | §2 |
| **R0-2** | "a table with fewer entries ... would hand two characters the same spawn point" | §2 |
| **R0-3** | "exactly as `brawlerProjectileSimulation::StaticData::projectilePoolSize` is" | §3 |
| **R0-4** | "placeholder authoring, sized to the level this mode ships on ... change them here and nowhere else" | §3 |
| **R0-5** | "Sorting FNames with `<`" | §5a |
| **R0-6** | "a float-comparing version could not be red-probed on this build anyway" | §5c |
| **R0-7** | the `teleportPending == 0` term is "belt-and-braces" | §8 |
| **R0-8** | the out-of-range case cited as `Ringout.SpawnSlotOutOfRange` | §9 |
| **R0-9** | "the absence of a specialization is what enforces that" and "its absence is the enforcement of ticks, never float seconds" | the guards file, **G-01** and **G-02** |

⭐ **R0-9 is a family, not an incident.** Twice, in two unrelated places, the header claimed that an
ABSENCE enforced something. An absence enforces nothing — it is the state to be protected, not the
protection. Both are now assertions. If a third such sentence is ever written in this file, it is
the same defect.

---

## 2. `kMaxSpawnPoints` — why the authored table has the size it has

> Compile-time CAPACITY of the authored spawn table, and the reason it is 4:
> `ASimulationManagerUImpl::kPreDietCharacterCap == 4` is the largest character count whose
> join-alone round still fits one packet (`RoundVsPacketBudgetTest.cpp`,
> `largestFittingCharacterCountUnderJoin()` — the runtime fence MIRRORS that derivation, it
> does not define it). A table with fewer entries than the cap would hand two characters the
> same spawn point at a legal player count; more would be entries nothing can ever index.

✅ The first sentence is verified: the constant is 4, the derivation is
`largestFittingCharacterCountUnderJoin()`, and that file states in its own words that the runtime
fence mirrors it rather than defining it.

⛔ **R0-1 — "more would be entries nothing can ever index" is FALSE.** `kPreDietCharacterCap` is
**ADVISORY**. Its runtime fence logs a `Warning` and lets the session continue — its own comment
reads *"WARNING, not Log, and not an assert: an over-cap session still RUNS - report, do not
crash"*. So a fifth character registers today, and with a five-entry table `acquire` would hand it
index 4 and the fighter would spawn there. Nothing prevents it.

⛔ **R0-2 — "a table with fewer entries ... would hand two characters the same spawn point" is
FALSE for this design.** `acquire` returns the lowest index no live character holds, or
`kNoFreeSlot`; it never returns an index twice. A short table means the surplus characters get
**no** point — the out-of-range branch and a `Warning` — not a shared one. Handing two characters
the same point is the failure mode of the *rejected* per-wire derivation (§7), and the sentence
imported that symptom onto a design that cannot produce it.

⚠ **The count of mirrors is deliberately not restated here.** The header said *"THIS IS THE THIRD
MIRROR OF THAT 4"* and that was true when it was written, but a number that has to be incremented
every time someone adds a mirror is a sentence scheduled to become wrong. What is load-bearing is
the shape: this constant is a **mirror**, the thing it mirrors lives in files an engine-free
header cannot reach (a `UCLASS` member and a test), and when the wire diet deletes
`kPreDietCharacterCap` this constant does **not** die with it — it becomes sized by the supported
player count directly, and this section is what has to be rewritten rather than the number
silently kept.

---

## 3. `StaticData` — the three authored constants

> Every constant is DEFAULTED to its authored value, exactly as
> `brawlerProjectileSimulation::StaticData::projectilePoolSize` is: this is the one place
> each literal is declared, so a default-constructed StaticData — which is what the LLT
> rigs and `simulatableBrawler::StaticData`'s member both get — is the shipped object
> rather than a second set of numbers to keep in step.

⛔ **R0-3 — "exactly as ... `projectilePoolSize` is" compares two different mechanisms.**
Ring-out's constants are defaulted **constructor parameters**; `projectilePoolSize` is a **default
member initializer** and is not a constructor parameter at all. The consequence claimed — one
declaration per literal, and a default-constructed object that is the shipped object — holds for
both, so only the word "exactly" is wrong.

**`killPlaneZ`.** Ruling 6: the kill plane is a SINGLE authored world-Z plane. Strictly below it
you are dead. Not a volume, not a per-level shape — one float, compared once per character per
tick. The strictness of that comparison is pinned by a case in the suite.

**`respawnDelayTicks`.** 120 ticks == 2.0 s at 60 Hz. The countdown is an ABSOLUTE tick, not a
remaining duration — see guard **G-02** for why, and for the two assertions that now enforce it.

**`spawnPoints`.** Ruling 7: an AUTHORED spawn table with a deterministic per-character index.
There is no penalty for dying — a respawn goes to this character's own slot, every time.

> The positions are placeholder authoring, sized to the level this mode ships on. They
> are tuning values, not derived ones — change them here and nowhere else.

⛔ **R0-4 — both halves of that are FALSE, and the file contradicted itself.** The user ruling of
2026-09-13 that produced §5 says the opposite in its own words: the platform this mode ships on is
somewhere else entirely, and a respawn dropped fighters off the map. And since that ruling the
table is **overwritten** at level load by `spawnPointsFromLevelPlacements`, so changing these
literals changes only the fallback for a level that has not been dressed. The authored values are
a FALLBACK; the level is the source of truth.

---

## 4. No physics body, no adapters — and why that needs no check

> ⛔ THIS SUB-SIM OWNS NO PHYSICS BODY. There is no `PhysicsSetup`, no `PhysicsDeclaration`
> and no `RuntimeBindings`, and that is deliberate: ring-out READS the movement sub-sim's
> solved body position and WRITES the movement sub-sim's teleport seed. It never touches the
> physics adapter, so it is handed neither adapter and cannot.

✅ Verified. It is also the reason `IntegrationUtils` here is a plain class where every other
sub-simulation's is a class template on the two adapter types — and the reason the vacuity control
beside guard **G-02** uses a local probe type: no other `IntegrationUtils` in this directory can be
named without instantiating it against adapters ring-out is deliberately not handed.

The shape of the file is borrowed, not invented: `StaticData` / `InitialConditions` / `State` /
`DerivedState` / `PlayerInput` / `Dependencies` / `integrate`, then the `SerializableFields`
specializations and the role assertions at the bottom, exactly as
`BrawlerProjectileSimulation.h` lays them out.

---

## 5. Where the spawn points actually come from: the level

**User ruling, 2026-09-13.** The authored table is placeholder authoring at (±200, ±200, Z=200)
and the platform this mode ships on is somewhere else entirely, so a respawn dropped fighters off
the map. Respawn slot N is now PLAYER START N — placement becomes level design instead of a code
edit, and the authored table degrades into the FALLBACK for a level that has not been dressed yet.

**Why the merge lives in this engine-free header and not in the UE layer that calls it.**
`SimulationManagerUImpl.cpp` is a UE module file and the low-level-test target cannot reach it, so
any claim made there is verified by review and by a PIE run and by nothing else. The three
properties that actually matter — that the result does NOT depend on the order the engine handed
the actors over, that zero placements leave the authored table intact, and that a short list fills
what it can and leaves the rest authored — are therefore assertions in
`BrawlerRingoutSimulationTest.cpp`. The UE layer holds the actor walk and no policy.

### 5a. Trap 1 — the engine's order is not an order

`UGameplayStatics::GetAllActorsOfClass` and `TActorIterator` both hand back actors in UNSPECIFIED
order — it is the level's actor array, whose order depends on load order, on streaming, and (under
One File Per Actor, which this project uses) on the order the external-actor packages resolved. Two
peers running the same map can see different orders.

This table is authored data every peer must agree on ENTRY FOR ENTRY: `integrate` indexes it with
the wire-carried `spawnSlot`, so if the server's slot 0 and a client's slot 0 are different points,
the client predicts a respawn to the wrong place, the server corrects it, and the fighter visibly
snaps on every single respawn. Nothing fails to compile, no wire fence moves — the wire carries the
INDEX, not the POINT — and no existing test notices.

⛔ **So the arrival order is discarded and the list is sorted by a key every peer computes
identically: the level-placed actor's name.** It is saved in the map package, so every peer that
loaded the map holds the same string; it is the one property of a placed actor that is neither
derived from load order nor recomputed at runtime.

⛔ **And the obvious UE spelling of that sort is the bug wearing the fix's costume.**
`FName::FastLess`, `FNameFastLess` and `FNameEntryId::operator<` all compare the NAME TABLE INDEX,
and the engine states the consequence at the declaration itself: *"Fast non-alphabetical order that
is only stable during this process' lifetime"*. That index is the order in which the string was
first interned in THIS process, and a server and a client never intern in the same order. Ordering
`FName`s that way reintroduces precisely the disagreement this sort exists to remove. ⇒ the UE
caller hands this function the name as BYTES and the comparator is a plain byte-lexicographic
`std::string` compare, which no process state can reach.

⛔ **R0-5 — the header said "Sorting FNames with `<`", and that expression does not compile.**
`FName` has no `operator<`; the engine declares one on `FNameEntryId` and on `FMinimalName` only.
The hazard is real and arrives through the three routes named above — which the same paragraph
already listed correctly — so only the summary spelling was wrong. It is corrected above.

⚠ **Byte-lexicographic, not numeric:** `PlayerStart_10` sorts BEFORE `PlayerStart_2`. That is
deterministic and identical on every peer, which is the property that matters — but a level
carrying more than `kMaxSpawnPoints` player starts keeps the first four IN THAT ORDER, so name them
such that the order you want is also the order you read.

⚠ **Place them in the persistent level.** A player start inside a STREAMED sublevel may not be
loaded at the moment the manager seeds this table, and may be loaded at different moments on
different peers — which is the same disagreement again, arriving through a different door.

### 5b. Trap 2 — not enough player starts

ZERO placements returns `authoredFallback` UNCHANGED — the level simply has not been dressed and
the placeholder table is the best answer available. FEWER than `kMaxSpawnPoints` fills the low
slots from the level and leaves the remainder at their authored values.

⛔ **No slot is ever left at the origin, and the shape of the function is what guarantees it.** The
result STARTS as a copy of the authored table and is overwritten downwards; there is no path
through it that default-constructs an entry. A fighter respawning at (0,0,0) is the same class of
bug as a fighter respawning off the platform, and a `std::array` built up rather than copied down is
exactly how that bug gets written. Machine-checked — §1c, 2 red.

### 5c. The ordering key, and why the tie-break is a bit pattern

`LevelSpawnPoint::name` is a `std::string` of bytes, not an `FName` and not a wide string, and that
is the whole point of §5a. The conversion happens once, in the UE layer, at the only place an
`FName` is in scope at all.

**Name first** — that is the peer-identical part, and in a UE world actor names are unique within a
level, so in practice the name alone decides every comparison.

⛔ **The position tie-break is a TOTALITY guard, not a second key.** `std::sort` over a key with
ties leaves the tied elements in an ARRIVAL-DEPENDENT order, which is the exact property this
function exists to destroy — a comparator that can answer "equivalent" for two distinct actors has
a hole in it the size of the original bug. Two placed actors CAN share a name across two different
sublevel outers, so the possibility is not zero, and "actor names are unique" written in a comment
is documentation, not enforcement.

⛔ **The tie-break compares bit patterns, not floats, and that is a correctness choice.** `a < b`
is FALSE in both directions for a NaN, which makes a float-comparing tie-break not a strict weak
ordering and makes `std::sort` UNDEFINED BEHAVIOUR rather than merely wrong. A bit pattern is a
total order on every input there is. It is not a NUMERIC order (−0.f sorts away from +0.f,
negatives sort above positives) and it does not need to be: it is a tie-break, its only job is to
be TOTAL and to be identical on every peer, and the values it orders come from the same asset on
every peer.

> ⚠ AND A FLOAT-COMPARING VERSION COULD NOT BE RED-PROBED ON THIS BUILD ANYWAY. The
> low-level-test target compiles `/fp:fast` ..., under which MSVC may assume no NaN exists and
> rewrites the comparison; the defect would be real and the probe would stay green.

⛔ **R0-6 — FALSE, and measured false today.** The float-comparing version **was** red-probed: it
turns `Ringout.SpawnPoints.DuplicateNamesStillOrderDeterministically` red, 1 failing case of 39.
The `/fp:fast` argument is sound about the **NaN** discriminator only. The line that actually bites
is the **sign bit** — that case pins `spawnOrderBits(-700.f) > spawnOrderBits(700.f)`, an ordering
a float compare reverses, and no floating-point contraction flag touches it. The prohibition was
already enforced; the sentence explaining why it could not be was the only thing wrong.

⭐ **`std::sort` and not `std::stable_sort`, and the choice is INERT rather than lucky.**
`levelSpawnPointOrderBefore` is a TOTAL order, so no two distinct elements are ever equivalent and
there is no tied run for stability to preserve. (It would also be inert for a second reason — this
toolchain runs insertion sort at N ≤ 32 and this list is at most a handful — but that reason is a
toolchain accident and this one is a property of the comparator, so this is the one written down.)
Measured: swapping one for the other leaves all 39 cases green.

**The merge takes the placements BY VALUE** because it sorts them: the caller's arrival order is
scratch by construction, and nothing upstream wants it back. `authoredFallback` is the table to
keep wherever the level says nothing; in production it is `StaticData::spawnPoints` exactly as the
defaults left it, so "leave the rest authored" is literally "leave the rest alone". ⭐ This needs no
fence: taking the list by const reference does not compile — see §1d.

---

## 6. The wire slices

**`InitialConditions` — 4 B, seeded ONCE, at registration, by the authority.** It is on the wire
for the same reason the movement sub-sim's teleport seed is: so the AUTHORITY'S number is the one
every peer uses, and a correction RESTORES it instead of a client RECOMPUTING it. A recomputed slot
is a slot that can differ between peers, and two peers disagreeing about a spawn point is a desync
you only see on the respawn tick.

⛔ **Unlike the movement teleport seed, this is NOT a counter-free edge** — nothing consumes and
clears it. It is written once and read on every respawn for the life of the character. The UE
caller says the same thing from its own side.

**`State` — 5 B: one flags byte plus the absolute respawn tick.** `respawnAtTick` is meaningful
only while `kFlagDead` is set; it is left at its last value once cleared, deliberately — an extra
"reset to 0 on respawn" write would be a second thing the wire has to agree on for nothing, and
nothing reads it while the dead bit is clear.

**`kFlagDead` is bit 0, and it is READ by both arms of the law in `integrate`** — the respawn arm
requires it set, the death arm requires it clear. That is the point: a neighbouring sub-simulation
once shipped a flag bit that was written and never read, and this tree has already paid for that.

⛔ **These are this sub-sim's own flags, and that is the whole point of the type.** Two neighbouring
bytes were considered and are both wrong:

* `brawlerMovementSimulation::kFlagFrozen` is NOT a "disable the character" flag. It is a per-tick
  DERIVED output of the support probe, cleared and re-set inside every single movement step.
  Anything a ring-out task writes there is gone on the next tick.
* `brawlerMovementSimulation::State::flags` bits 4-7 ARE free, but they are inside the one header
  another live initiative holds exclusively, and putting ring-out semantics there would couple two
  initiatives through a byte neither of them owns.

A separate slice costs 9 B of the correction buffer's headroom and zero conflict.

**`DerivedState` is off the wire.** Recomputed from scratch at the top of every step, including
every replayed step of a resim, which is exactly why the scoring system may read it: a derived edge
can never arrive on a correction and can never be stale. `diedThisTick` is TRUE on the single tick
the character crossed the plane and FALSE on every other tick — including the ticks it spends dead
afterwards. This is the EDGE the authority-side score system consumes (ruling 5: the award fires on
the authority tick that detects the death, because the authority never rewinds). The prohibition
half of this is now an assertion — §1b and retired id **G-03**.

**`PlayerInput` is empty — zero serialized fields, zero wire bytes**, mirroring
`dAttackGuardSimulation::InitialConditions`'s empty specialization. It exists because
`ValidDependencies` requires `Dependencies::InputType` and the ownership validator treats that type
as OWNED by this sub-sim: naming any other sub-sim's input here would report an ownership overlap.
It does NOT exist because ring-out wants a per-tick signal — death is positional and respawn is a
tick countdown, so there is nothing for a player to press. The arithmetic for why an input byte is
expensive is in that line's own assertion message.

---

## 7. The execution-order edge — and why it is not written down twice

⭐ **This property is machine-checked, so this section states only what the assertions do not.**
`BrawlerRingoutSimulationTest.cpp` asserts that ringout-before-movement validates AND that
movement-before-ringout does not, and a runnable case re-quotes both with the offending pair named.
That is a stronger fence than any comment, and it lives in a file an edit to this header cannot
reach.

**What it buys, concretely.** On the respawn tick T, ring-out clears the dead bit and writes the
teleport seed; movement consumes that seed LATER IN THE SAME TICK T and puts the body on the spawn
point. By tick T+1 the position ring-out reads is already above the plane. Integrated the other way
round, the seed would sit unconsumed until T+1, ring-out would read the still-below-plane position
at T+1 with the dead bit clear, and the character would die again immediately — an infinite respawn
loop with nothing failing to compile.

**What it costs, and this is the half no assertion states.** Ring-out reads the body position as
the movement sub-sim left it at the END of tick T-1. A death is therefore detected one tick after
the crossing. That is a fixed, deterministic one-tick latency on a 60 Hz step, identical on every
peer and through every replay, and it is the cheaper half of the trade.

**Why the dependency validator agrees.** Both external edges point at the movement sub-sim, so the
validator resolves them the way it resolves every read/write pair: the HARD edge (the non-const
`InitialConditions&` write) wins and the SOFT edge (the const `State&` read) is dropped —
`validateOneExternalRef` drops a soft edge whenever `hasHardEdge` holds between the same two sims.
The declared order that survives is ringout → movement.

---

## 8. The death arm's condition — the three terms are NOT independent ∴D-01

**This is the one expression in the file a reader cannot change correctly from the identifiers in
front of them, and the header's own explanation of it was wrong.**

```cpp
else if (!isDead(state)
         && movementIc.teleportPending == 0u
         && movementState.bodyState.position.z < sd.killPlaneZ)
```

The header said the `else` was load-bearing twice over — against the respawn arm, because *"a plain
`if` would re-arm the countdown on the very tick it expired"*, and against itself, because
`!isDead` is what makes a death happen exactly once. It then called the `teleportPending == 0` term
*"belt-and-braces"*.

⛔ **R0-7 — "belt-and-braces" is wrong, and the two fences overlap almost completely.** Measured:
demoting `else if` to `if` leaves **38 of 39 cases green**. The reason is that the respawn arm sets
`teleportPending = 1u` before the death arm is evaluated, so on the ordinary respawn path the
`teleportPending == 0` term — not the `else` — is what suppresses the immediate re-death. The one
case that does go red is `Ringout.SpawnSlotOutOfRangeWritesNoTeleportSeed`, which is precisely the
path where **no seed is written**, so `teleportPending` stays 0 and the `else` is the only thing
left holding.

⇒ **What each term actually buys, after measurement:**

| term | what it alone covers |
|---|---|
| `else` | the respawn tick **when no teleport seed was written** — i.e. the out-of-range slot. Nothing else. |
| `movementIc.teleportPending == 0u` | the respawn tick on every normal path, **and** the registration seed: the UE layer seeds a teleport at spawn before the movement sub-sim has integrated once, so the position in `movementState` is about to be REPLACED and is not where the character is going to be. |
| `!isDead(state)` | a character that keeps falling below the plane while dead. Without it the respawn never arrives and the score system counts the same death on every tick of the fall. |

⛔ **And the `teleportPending` term is still not a substitute for the execution order.** It stops
the immediate re-death under a ringout-after-movement order; it does NOT stop the respawn itself
arriving a tick late there. §7 is still the thing that has to hold.

⚠ **The coverage is real but INCIDENTAL, which is why this section exists.** The single case that
catches a deleted `else` was written for a different purpose. Make the out-of-range branch write a
seed and that coverage disappears silently, leaving the `else` unguarded again.

---

## 9. The respawn arm's defensive index — a runtime branch, deliberately not an assert

> ⛔ DEFENSIVE INDEX, AND IT IS A REAL RUNTIME BRANCH — not an `OG_CHECK`.
> `OG_CHECK` forwards to `checkf`, which COMPILES OUT IN SHIPPING: a guard spelled
> that way would leave the Shipping build reading past the end of the array, which is
> the one build where nobody is watching.
>
> ⛔ AND THERE IS NO `OG_CHECK` RIDING ALONGSIDE IT EITHER, which is the deliberate
> half. An assert here fires in Development and takes the process with it, so the
> recovery branch below would be code no test could ever execute — the defensive arm
> would be as unread as the flag bit once was. The `[Warning]` log in the
> else arm is the loudness, and the out-of-range case is what proves the arm runs. A bad
> slot is an authority assignment bug; it must not also be a crash.

⛔ **R0-8 — the header named the proving case as `Ringout.SpawnSlotOutOfRange`, and no case has
that name.** It is `Ringout.SpawnSlotOutOfRangeWritesNoTeleportSeed`, which the same file cited
correctly ninety lines further down. The citation as written landed on nothing.

✅ Verified: `OG_CHECK` does forward to `checkf` on the UE build arm, and deleting the branch turns
that case red (§1c).

⚠ **This prohibition has NO machine backstop in the direction that matters.** Deleting the branch
is caught. *Replacing* it with an `OG_CHECK` is not caught by anything — the suite runs a build
where `checkf` is live, so the check would fire and the case would go red for the wrong reason,
while Shipping quietly read past the end of the array. It gets no tag because the forbidden edit is
several lines of replacement rather than one token at a site, but it is the one fence in this file
that a reviewer should read with their own eyes.

---

## 10. The teleport seed hand-off

⛔ **Reuse the movement teleport seed. Do not invent a second teleport path.** Its own contract says
it is on the wire *"because a respawn must replay identically"*, and its consumer is the ONE body
write in the movement sub-sim that ignores `drivesBody`: it assigns the position, zeroes the
velocity, calls `setBodyTransform` and `setBodyLinearVelocity` with zero on the capsule, and clears
`teleportPending` in the same tick. A respawn wants every one of those.

✅ Verified against the movement header: that block sits outside the `drivesBody` branch and does
all four things, plus clearing the movement sub-sim's own command flag and step state.

---

## 11. The authority's spawn-slot table

⛔ **This is not a sub-simulation type.** It is in no composite, it has no `SerializableFields`
specialization, and it costs ZERO wire bytes — `simulatableBrawler::State` is 335 B with it and
335 B without it. The value it produces rides the wire; the table that produced it does not. It is
authority-side bookkeeping that lives BESIDE the simulation, the same shape the score system uses,
and for the same reason: a rollback must not be able to reach it. Now enforced — guard **G-01**.

⭐ **Why it is in this engine-free header rather than in the UE layer that calls it.**
`SimulationManagerUImpl.cpp` is a UE module file and the low-level-test target cannot reach it. The
two properties that actually matter here — that two separate remote clients get DIFFERENT slots,
and that a released slot is reused by a later join — are therefore assertions in
`BrawlerRingoutSimulationTest.cpp` instead of prose in a review note. The UE layer holds the two
calls and no logic at all.

### 11a. The rule

The AUTHORITY assigns each character the LOWEST index no live character currently holds, once, at
registration. Arrival order decides who gets which number.

⛔ **Arrival order is acceptable here. Do not "fix" this into a per-peer derivation.** The
determinism rule this looks like it bends exists to stop exactly two things: peers DISAGREEING
about a spawn point, and a resim producing a DIFFERENT answer than the original run. Neither can
happen to a value the authority assigns once and the wire carries:

* a client never calls this. It READS `spawnSlot` out of an ordinary correction; it never derives
  it, so there is no second derivation to disagree with the first.
* a resim RESTORES `InitialConditions` by whole-struct assignment before replaying, so a replay
  cannot recompute it differently. It is restored, not re-derived — exactly like the movement
  sub-sim's teleport position, which is arrival-ordered in the same way and for the same reason.

The arrival-order dependence is confined to ONE authority-side assignment that is never
resimulated. That is the same argument that licenses the score itself.

✅ Verified: ring-out's `InitialConditions` is a member of the composite `State` the reconciliation
path restores by whole-struct assignment.

⛔ **And the thing a "deterministic" replacement would break is not hypothetical.** The obvious
per-peer derivation is the engine's per-wire player slot, and it is WRONG: it returns 0 for the
primary pawn of EVERY remote client and 1..N only for couch-co-op siblings on one machine. Two
remote clients would both index spawn point 0 and respawn on top of each other. That is the
collision this design exists to prevent, and a derivation reintroduces it. It is machine-checked by
`Ringout.SpawnSlots.TwoRemoteClientsDoNotBothGetZero`, which writes the wrong answer down as a
constant and asserts against it.

### 11b. When the table is full

`acquire` returns `kNoFreeSlot`, whose VALUE is `kMaxSpawnPoints` — deliberately the exact value
`integrate`'s defensive index branch already treats as out of range. So an over-capacity join
respawns to NO teleport seed and logs a warning, rather than silently taking slot 0 and landing on
whoever legitimately holds it. LOUD AND PLACELESS BEATS QUIET AND COLLIDING: the character still
lives, still dies, and still clears its dead bit. Now asserted at the declaration — retired id
**G-04**.

### 11c. The contract of the three operations

⛔ **`acquire` is IDEMPOTENT BY CONTRACT.** A character that already holds a slot gets the SAME one
back and nothing is consumed. The UE caller's own first-call guard means this is reached once per
character, but registration is RETRIED — `tryRegister` returns `Pending` until the bodies resolve —
so an allocator that handed out a second entry on a second call would leak the first one with no
symptom until the table ran dry. Machine-checked by
`Ringout.SpawnSlots.AcquireIsIdempotentAndDoesNotConsumeASecondEntry`.

⛔ **`release` is the other half of the contract, and it is not optional.** Without it a session
that churns characters exhausts a four-entry table and every later join respawns nowhere. It is a
NO-OP for an id holding nothing, and that is load-bearing: the UE unregister path runs on BOTH
roles, and on a client nothing ever acquired. That is the same shape as the ungated erase it sits
beside — an ungated call whose emptiness on the non-authority role is a property of the inserts,
not of a role test. Machine-checked by
`Ringout.SpawnSlots.ReleaseFreesTheIndexAndALaterJoinReusesIt`.

⛔ **A `held` flag, not a sentinel character id.** Character ids arrive from the engine as an
object's unique id; reserving one of their values to mean "free" would be a guess about a range
this engine-free header cannot see.

---

## 12. What this conversion broke elsewhere, and what was done about it

Two sentences in `BrawlerScoreboardVisualization.h` pointed at comments in this header that no
longer exist. ⚠ Both now live in `docs/BrawlerScoreboardVisualization-rationale.md` (sections
5.3 and 8), where that header's prose was moved on 2026-09-16; they are no longer in the header. Both were written by this same initiative, so the falsity is this lane's to fix and
both were repaired in place:

1. a sentence quoting `brawlerRingout::State::respawnAtTick`'s comment verbatim as if it stood at
   that declaration — now attributed to this document;
2. a sentence saying *"that constant's own comment already calls 'THE THIRD MIRROR OF THAT 4'"* —
   now written so that it does not depend on a comment existing at all, which is also what keeps it
   from going stale on the next mirror.

Nothing else in the tree quotes this header's prose. Its declarations are cited widely and all of
them survive, because the declarations did not move.

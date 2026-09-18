<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerRingoutSimulation.h` — guards

Prohibitions with a tagged site in `BrawlerRingoutSimulation.h`. Ids are opaque, stable, and
retired rather than reused.

⚠ **This file is SHORT ON PURPOSE, and the reason is a measurement rather than a preference.**
Every prohibition this header used to state in prose was put to the compiler or to the suite
before it was written down, and all but two came back already enforced — by a `static_assert` in
this header, or by a `TEST_CASE` in `BrawlerRingoutSimulationTest.cpp` that was seen to turn RED
when the prohibited edit was actually made. A property that is already enforced needs no entry
here; it needs the assertion that enforces it, and that assertion's message is where its
reasoning lives. The full table of what was measured, and what fired, is in
`BrawlerRingoutSimulation-rationale.md` §1.

---

## G-01 — `SpawnSlotAllocator` must never acquire a `SerializableFields` specialization

**Site:** the `static_assert(!Serializable<brawlerRingout::SpawnSlotAllocator>)` beside the
`SerializableFields` specializations at the foot of the header.

**The prohibition, as it stood in the header:**

> ⛔ THIS IS NOT A SUB-SIMULATION TYPE. It is in no composite, it has no `SerializableFields`
> specialization, and it costs ZERO wire bytes — `simulatableBrawler::State` is 335 B with it
> and 335 B without it. The value it produces rides the wire; the table that produced it does
> not. It is authority-side bookkeeping that lives BESIDE the simulation, the same shape the
> score system uses, and for the same reason: a rollback must not be able to reach it.

**The consequence of getting it wrong.** Putting the allocator on the wire makes the whole
authority-side slot table a thing a correction can overwrite and a resim can replay. Slot
assignment is arrival-ordered by design; it is safe *only* because it is assigned once by the
authority and never re-derived. A rollback that can reach the table can reassign a slot mid-life,
and the character respawns somewhere else from then on.

**What breaks if the tag moves.** The tag is positioned for the ORPHAN ARM, not for the reader —
it sits with the assertion so that an edit which deletes the assertion takes the tag with it and
orphans this entry, failing the build. Moved away from the assertion, that protection is gone and
the entry is ordinary prose with an id.

**Why this one keeps a live id when the neighbouring assertion does not.** The
`!Serializable<brawlerRingout::DerivedState>` assertion directly above carries no tag, because
`BrawlerRingoutSimulationTest.cpp` already asserts the same property with a
`STATIC_REQUIRE_FALSE` in a file this header's editor cannot reach: deleting the header assertion
alone leaves the property checked. Nothing anywhere asserts the allocator's, so deleting this one
would leave nothing — which is exactly the gap this id exists to close.

---

## G-02 — `IntegrationUtils` must never be handed a delta time

**Site:** the `static_assert(!RingoutStepExposesDeltaTime<IntegrationUtils>)` directly beneath the
`IntegrationUtils` class.

**The prohibition, as it stood in the header:**

> ⛔ THERE IS NO `getDeltaTime()`, and its absence is the enforcement of "ticks, never float
> seconds". A countdown in seconds is the reflex implementation of a respawn delay, and a
> sub-sim that is never handed a delta cannot write one.

> ⛔ TICKS, NEVER FLOAT SECONDS. The sim is a fixed 60 Hz step
> (`AsyncFixedTimeStepSize = 0.016667`), and the countdown is stored as the ABSOLUTE tick
> to respawn at (`State::respawnAtTick`), not as a remaining duration. A seconds
> accumulator would land on a different tick after a resim replayed the same interval
> with a different number of steps; `tick >= respawnAtTick` cannot.

⛔ **R0 — the first sentence was FALSE as written, and this entry exists because of it.** *An
absence enforces nothing.* Nothing stopped anyone adding the accessor; the header simply asserted
that nobody had. The two `static_assert`s at the site are what make the sentence true, and the
prohibition is written down here only because the §2.1 suppression does not cover a check whose
deletion is co-located with the edit it forbids.

**The consequence of getting it wrong.** A respawn countdown accumulated in seconds lands on a
different tick when a resim replays the same wall-clock interval with a different number of steps.
The client then clears its dead bit on a tick the server did not, and the correction that follows
snaps a fighter who is mid-respawn.

**⚠ The measured limit of the check, so the entry is not read as stronger than it is.** Two
assertions sit at the site and they cover different halves:

| the edit | `!RingoutStepExposesDeltaTime` | `sizeof(IntegrationUtils) == sizeof(uint32_t)` |
|---|---|---|
| add `getDeltaTime()` — the spelling every other `IntegrationUtils` in this directory uses | **fires** | silent |
| add a differently-named accessor returning a hard-coded constant | silent | silent |
| add a private delta member with no accessor at all | silent | **fires** |
| add a stored delta plus an accessor under any other name | silent | **fires** |

A delta that actually varies has to be stored, and storage fires the second line whatever the
accessor is called. The one uncovered case is an accessor returning a constant, which is not a
delta the engine handed in. Both lines were measured against a shadow copy of this header; the
clean control stayed silent in the same runs.

**What breaks if the tag moves.** Same as G-01: it sits with the assertion so that deleting the
assertion orphans this entry. The two lines are kept **adjacent, directly under the tag**, so
either deletion is at the tag.

⚠ **The residual limit, stated rather than hidden.** One tag cannot protect two assertions against
*independent* deletion: remove only the `sizeof` line and the tag survives on its neighbour, so no
orphan fires. That is a property of the one-tag-per-site rule, not of this entry, and it is why
both messages say the two lines are one fence.

---

## Retired ids — never reuse

| id | what it was | where it went |
|---|---|---|
| **G-03** | `DerivedState` is off the wire and must stay there — an edge that can arrive on a correction is an edge the score system can count twice. | `static_assert(!Serializable<brawlerRingout::DerivedState>)` in this header, plus the `STATIC_REQUIRE_FALSE` that `BrawlerRingoutSimulationTest.cpp` already carried. No live tag: a separate file's assertion survives the joint deletion. |
| **G-04** | `SpawnSlotAllocator::kNoFreeSlot` has the VALUE `kMaxSpawnPoints` on purpose, so that writing it straight into `spawnSlot` lands on `integrate`'s out-of-range branch instead of needing a second guard at the UE call site. | `static_assert(kNoFreeSlot == kMaxSpawnPoints)` at the declaration, plus the `STATIC_REQUIRE` already in `BrawlerRingoutSimulationTest.cpp`. No live tag, same reason. |

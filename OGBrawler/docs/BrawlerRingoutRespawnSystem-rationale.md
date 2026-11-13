<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerRingoutRespawnSystem.h` — rationale

`brawlerRingout::RespawnSlotSystem`, og-input-latency task 2d, 2026-11-11. It moves a respawn off a spawn point another
character occupies. The rule itself (`chooseRespawnSlot`, `spawnPointIsOccupied`, `kSpawnPointOccupiedRadius`) lives in
`BrawlerRingoutSimulation.h`, and its reasoning in `BrawlerRingoutSimulation-rationale.md` §11d. This file covers the
system that applies it.

The header states no prohibition in prose. Its two rules are code: a `static_assert` that the system is `AuthorityOnly`,
and an `OG_CHECK` that the rows are in id order. So it carries a one-path docs pointer and there is no guards doc.

## 1. What it does, on which tick

| hook | selects | why then |
|---|---|---|
| `postIntegrate` | every character whose `DerivedState::diedThisTick` is set: it died this tick | the earliest point the new slot can be known. It reaches each client with the first correction for this tick or a later one, about `respawnDelayTicks` (2 s) before the client predicts the respawn, so a moved slot causes no visible snap |
| `preIntegrate` | every character for which `respawnIsDue(state, tick)` holds: `integrate`'s respawn arm will teleport it later in this step | the occupancy may have changed during the countdown. If the slot chosen at death is still free this keeps it, so the slot changes only when it must |

For each selected character, in id order, it builds the occupants, calls `chooseRespawnSlot(spawnSlot, spawnPoints,
occupants)` and writes the result back into `InitialConditions::spawnSlot`. It logs one line per choice,
`[Ringout.respawnSlot] tick=… id=… at=death|respawn slot=<before>-><after> nearestOccupant=<m> radius=<m> occupied=0|1`
through `OGBLOG_G` (`LogOGBrawler`). With `occupied=1` the line starts `[Warning]`: every slot was occupied and the
character kept its own. It writes no line for an out-of-range slot, which the respawn arm already warns about.

## 2. The occupants

Every character except the one choosing:
- **alive** (`!isDead`): its body position, `brawlerMovementSimulation::State::bodyState.position`, or its teleport target
  `teleportPos` while `teleportPending` is set. A registration or a respawn seeds a teleport that the movement sub-sim has
  not consumed yet, so the position in the state is about to be replaced;
- **dead**: the point of its own `spawnSlot`, if that is in range. It will respawn there, so the point is reserved. Without
  this, a character that died at tick *t* could pick a dead character's point, and that one's respawn check would then
  move it a second time, on its respawn tick, which the clients would see as a snap.

Characters choosing in the same pass see the earlier ones' new slots, because the reservation reads the slot as it is
now. `RingoutRespawnSlot.TwoSimultaneousRespawnsDoNotShareAFreeSlot` pins it.

## 3. Why the authority, and why it may write a wire field

**The brief's rule (og-input-latency task 2d):** the authority chooses the slot, every peer applies the same one, and the
slot rides today's path, `InitialConditions::spawnSlot`. That field is already serialized and already authority-written
(the registration seed, `BrawlerRosterUImpl-rationale.md` §2). This system adds a second authority write of it.

**Why `AuthorityOnly` (the `static_assert`).** The choice reads the *other* characters' positions. On a client those are
predictions, and on a replayed tick they are a rewind's, so an `AllRoles` system would let each peer write its own slot and
the authority's correction would fight it on every respawn. On the authority the hook fires exactly once per tick and never
on a replay (the executor's `checkAuthorityOnlyStep` tripwire).

⚠ **This stretches `SystemRoleAffinity.h`'s description of an `AuthorityOnly` system** as one whose outputs live "beside
the simulation — no composite, no wire, never corrected". This system's output is in the composite and on the wire. What
the affinity guarantees, exactly-once on a role that never rewinds, is the property this write needs. The mismatch is in
og-simulation's prose, which this task does not own; it is reported in the task's impl notes.

**What a client sees.** A client predicts with the slot it last received. A slot moved at a death reaches it with the
first correction for that tick or later: its predicted `InitialConditions` differs, so `isSimilarTo` (which walks every
serialized field, `spawnSlot` included) reports a misprediction and the client resimulates with the new slot. That is long before the respawn tick. A slot moved at the respawn tick itself reaches the
client after it has already predicted the respawn to the old point, and the correction moves the character: one snap, only
when the occupancy changed during the countdown.

## 4. Determinism

`StorageView::forEachSimulatable` walks an unordered map. Two characters choosing in one pass can compete for one free
slot, so the walk order would decide the winner. The system sorts its rows by id first, and an `OG_CHECK(std::is_sorted…)`
fails if the sort is deleted. The occupancy test is an "any" over the occupant list, so the list's own order does not
matter. The slot order is the table's index order.

## 5. Where it is wired

- The game: `BrawlerCompositionUImpl::Systems`, last, and its construction in `BrawlerSessionGameUImpl::emplaceSystems`
  (`BrawlerCompositionUImpl-rationale.md` §4).
- The golden harness: `goldenTraceScene::SystemsExec` and its constructor (`GoldenTraceScene.h`, og-brawler-tests), in the
  same order. Both goldens stayed exact with it, so no respawn in either scenario moved.

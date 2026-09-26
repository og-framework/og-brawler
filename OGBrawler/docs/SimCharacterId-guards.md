<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `SimCharacterId.h` — guards

Prohibitions with a tagged site in `SimCharacterId.h`. Ids are opaque, stable, and retired rather
than reused.

The header's other properties are enforced by the compiler and need no entry: the one-byte,
trivially copyable type, the ceiling equal to the underlying type's maximum, and an allocator that
cannot be copied, moved or assigned. The allocator test in og-brawler-tests pins them as well.

---

## G-01 — a `SimCharacterId` is never reused: the allocator counts up, refuses at the ceiling, never wraps and never releases

**Site:** `SimCharacterIdAllocator::allocate()`.

**The prohibition** (user ruling R2, 2026-09-26): the authority hands out 1, 2, 3 … 255, in that
order. Id 0 (`SimCharacterId::None`) means "no character" and is never handed out. After 255,
`allocate()` returns `SimCharacterId::None` and the host refuses the registration. ⛔ Do not wrap
back to 1. Do not add a `release` / `free` / `reset` that returns an id to the pool. Do not
"recycle" the id of an unregistered character. Whether to widen the type or to start reusing ids
when the ceiling is reached is **the user's decision**. It is not an edit to make here.

**The consequence of getting it wrong.** Every consumer below keys state by the id and assumes
that an id names one character for the whole life of the simulation that issued it. A reused id
silently joins a new character to an old one's leftovers:

* **the simulation object storage, the correction caches and the input-resolution containers.**
  The host passes the id to its registration door as the storage key.
* **the host's pending-registration record**, which is keyed by id and only erased on the Ready
  path. A reused id would inherit a half-built record.
* **`brawlerRingout::SpawnSlotAllocator::acquire`**, which returns the slot an id already holds.
  A reused id would inherit a slot, or be refused one.
* **`brawlerRingout::ScoreSystem`'s score table.** A late award for the old character would be
  credited to the new one.
* **every `id=%u` log line.** Log joins across peers are a plain equality since task 25, and a
  reused id makes one number name two fighters in the same log.
* **the attacker-side hit ledger (netcode task 27, shipped 2026-09-26: the radial
  `dAttackRadialSimulation::State::hitTargets`).** It stores target ids on the wire
  across ticks, so a reused id would make a new character un-hittable by a swing that hit the old
  one.

⚠ The lint allows ONE tag per id, so these consumers cannot each carry this tag. The list above
is the index a reuse task starts from. A consumer that starts to rely on permanence must be added
here.

**What breaks if the tag moves.** The tag sits on `allocate()` because that is where a wrap, a
fallback or a free-list would be typed. Deleting `allocate()` takes the tag with it and orphans
this entry (CHECK 2). Resetting the counter by assigning a fresh allocator is a compile error (the
`static_assert` below the class), so it needs no tag of its own.

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `SimCharacterId.h` — rationale

Companion to `OGBrawler/SimCharacterId.h`. Its prohibitions are in `SimCharacterId-guards.md`.

## 1. What the id is, and the contract with the host

`SimCharacterId` is the number a character is known by inside the simulation, and it is **the
same on every peer**. The engine-free core never interprets it. It is a key for storage, caches,
rosters, the ring-out spawn-slot table and every `id=%u` log line.

**The host contract** is one sentence, and any host (the Unreal module today, a Godot port later)
must honour it:

> The authority assigns each character a `SimCharacterId` from one `SimCharacterIdAllocator` per
> authority simulation, every peer learns that same value from the authority, and a peer
> registers the character only once it holds a non-zero id.

Only the *source* of the number is host-specific. In the Unreal host the authority's simulation
manager owns the allocator, the pawn replicates the value, and a client's registration waits
(`Pending`) until the value arrives. That reasoning lives in the host's own documents, next to
the host code.

## 2. Why a distinct type and not `uint8_t`

Before task 25 the id was an `unsigned int` taken from an engine object's per-process unique id,
so a server and a client used different numbers for the same character. A join on the wrong
object's id (the pawn instead of its component) compiled and silently matched nothing, because
both returned `unsigned int`.

`enum class SimCharacterId : std::uint8_t` is one byte, like a plain `uint8_t`, but:

* it does not convert implicitly to or from `unsigned int`, so an engine object id handed to a
  function that takes `SimCharacterId` is a compile error, and
* the only way to a storage key is the named `toStorageKey`, which is greppable.

The storage and cache keys stay `unsigned int`. Only the value is small and peer-stable, so no
og-simulation or og-brawler container changed type.

## 3. Why `uint8_t`, and why the ceiling is refused rather than wrapped (R2)

The id is meant to ride the correction wire inside the hit ledger (netcode task 27), where every
byte counts: at a cap of 3 ledger entries, `uint8_t` costs 3 B against 12 B for `uint32_t`. The
user ruled (R2, 2026-09-26): `uint8`, count-up, ceiling 255, never reused. At the ceiling the
registration is refused and the user decides between widening and reuse (guard G-01).

255 characters is far above any session this game runs (the pre-diet cap warns at 4). The
ceiling is reached only by churn: a server hosting one simulation long enough to register 255
characters, rejoins included, because a rejoin is a new pawn and gets a new id.

## 4. Why the allocator is non-copyable and non-assignable

"Never reused" has two routes that do not go through `allocate()`:

* **a reset by assignment.** assigning a fresh `SimCharacterIdAllocator{}` to the member restarts the count at 1 while
  characters still hold low ids.
* **a copy.** Two allocators that both issue the same next id.

Deleting all four copy and move members makes both of these compile errors. The `static_assert`
below the class states the reason in its message, and the test repeats the traits.

## 5. Where the counter lives (user ruling, 2026-09-26)

There is one allocator per **authority simulation**: a member of the authority's simulation
manager. It is not a process static. The ids must be unique for as long as the storage and the
wire that use them live, and that lifetime is exactly the manager's. A process static would run on
across every PIE session hosted by the editor process and exhaust 255 after roughly 60-120 runs.
A new manager (a new PIE run or a map load) starts again at 1. By then every character, storage
entry and composite of the previous session is gone.

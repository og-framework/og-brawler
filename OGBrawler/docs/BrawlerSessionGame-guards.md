<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerSessionGame.h` — guards

Prohibitions with a tagged site in og-brawler's `OGBrawler/BrawlerSessionGame.h`. Ids are opaque, stable, and retired rather than
reused. The reasoning lives in `BrawlerSessionGame-rationale.md`.

**If this file and the source disagree, the source is authoritative and this file is stale.**

---

## G-01 — the neutral input is `getZeroPlayerInput()`, not `PlayerInput{}`

**Tag site:** `BrawlerSessionGame.h`, directly above `return simulatableBrawler::getZeroPlayerInput();` in the inline body of
`neutralInput()`, which an engine adapter's session passes to the input resolution's `setNeutralInput` on both roles.

**The prohibition.** ⛔ Do not return a value-initialised input (`return PlayerInput{};`) or any other input. The neutral input
fills the `[0, effectiveDelay)` window and every tick an input is missing for.

**The consequence.** A value-initialised input carries a zero aim vector where the game expects `(0,0,1)`
(`SyncedPlayerInput::zero()`, `simFrame::kUp`).

**Why this is still a tag, measured.** Making it a `static_assert` does not compile: `simulatableBrawler::PlayerInput` has no
`operator==`, and `getZeroPlayerInput()` is not `constexpr`. `BrawlerSyncedPlayerInput.h` asserts that `SyncedPlayerInput::zero()`
aims at `(0,0,1)`, which does not see which value this function returns. The test
`BrawlerSessionGame.TheNeutralInputIsTheZeroInputNotAValueInitialisedOne` (og-brawler-tests, `[SimulatableBrawler]`) compares every
field with `getZeroPlayerInput()` and checks that the aim differs from a value-initialised input's.

*Moved from `BrawlerSessionGameUImpl-guards.md` G-01 (root `Source/OGBrawlerUnreal/docs/`, retired there) by og-session-runtime
task 26, with the function's body; that entry was `OGBrawlerSimulationManager-guards.md` G-60 before
og-ue-simulationcomponentandmanagersplit task 27b. R0 on the tree: `simulatableBrawler::PlayerInput` is `SyncedPlayerInput` (`SimulatableBrawlerTypes.h`);
`getZeroPlayerInput()` is a plain `inline` function returning `PlayerInput::zero()`; `SyncedPlayerInput` declares no
`operator==`; the Unreal session calls `Game::neutralInput()` before `setNeutralInput` on both role branches, and the brawler's
`BrawlerSessionGameUImpl::neutralInput()` returns this function's value.*

---

## §R — Retired ids

None.

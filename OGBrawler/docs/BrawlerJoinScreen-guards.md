<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: EKeys::Tab -- the engine's own key constant; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: EKeys::Insert -- the engine's own key constant; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: FKey::IsValid -- the engine's own key validity test (InputCore); owned by the host engine, not by this repository -->
# `BrawlerJoinScreen.h` — guards

Prohibitions with a tagged site in `BrawlerJoinScreen.h`. Ids are opaque, stable, and retired rather
than reused.

The header's other rules are held by the compiler and need no entry: a typed character is a wide
character (rationale §2.2), the recent-address separator can never occur inside an address
(rationale §4.2), every failure reason is counted (rationale §8) and an ink has no alpha channel
(rationale §12). The Catch2 suite in og-brawler-tests pins the behaviour.

---

## G-01 — the local co-op key names are the keys the player controller binds, not wording

**Site:** `kLocalCoopKeyNames`, the one constant the local co-op hint is built from.

**The prohibition.** `kLocalCoopKeyNames.addPlayer` and `.removePlayer` are the engine key names of
the two keys that `AOGBrawlerPlayerController::SetupInputComponent` binds to
`JoinLocalPlayer` and `LeaveLocalPlayer`: `EKeys::Tab` (engine name `"Tab"`) and `EKeys::Insert`
(engine name `"Insert"`). ⛔ Do not edit either string to improve the wording ("Ins", "the Tab key",
a translation), and do not change them without changing the two bindings in the same change. ⛔ Do
not change a binding without changing this constant.

**The consequence of getting it wrong.** The join screen tells a player which key adds a local
player. If the text and the binding disagree, the hint names a key that does nothing, and a local
playtest (one PC, several players) cannot be started from what the screen says. Nothing in the
pure tree can see the binding, so no test here fails.

**Why these are engine key names.** The strings are the names of the engine's own key constants, so
the UE layer can bind from them. Since task 12 it does: the player controller binds
`JoinLocalPlayer`/`LeaveLocalPlayer` to an `FKey` built from these two names (the UE-tier
`JoinScreen-rationale.md` §7). The hint and the binding therefore always name the same key, and
changing this constant changes the binding with it. A name that is not an engine key fails a
`checkf` (`FKey::IsValid`) at the first local player controller's input setup in every Development
and Editor run. The prohibition stays: a wording edit no longer drifts silently, but it still breaks
local co-op until someone reverts it.

**What breaks if the tag moves.** The tag sits on the constant because that is where the wrong
edit is typed. Deleting the constant takes the tag with it and orphans this entry (CHECK 2). The
binding no longer has a key of its own to keep in step; it reads this constant.

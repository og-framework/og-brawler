<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `KeyGlyphs.h` — rationale

Companion to `OGBrawler/InputMapping/KeyGlyphs.h`, which turns a `dInput::KeyId` into the short
text the controls HUD prints for it. **The header carries the code; this file carries the
reasoning.** It has **no guards file**: the one prohibition it has (never leave a key without a
glyph or a gamepad classification) is held by the compiler (§3, §4).

Written by og-brawler-controls-hud task 2 (2026-10-31), as a new file in the new comment convention.

---

## 1. What it is, and why it is pure code

`KeyGlyphs.h` :: `keyGlyph`, `isGamepadKey`, `kLastKeyIdValueAsShipped`

The controls HUD lists, for each row, every key bound to that row's action (user ruling R2 of
og-brawler-controls-hud: keyboard and mouse first, then gamepad, e.g. `5 / Pad Y`). The key names
therefore have to come from the binding itself, never from hand-typed text, and the code that names
a key has to be reachable from the brawler's Catch2 suite. `Source/OGBrawlerTests` links og-brawler
and og-simulation but not the game module, so the spelling lives here, engine-free (STL only), and
the UE layer only draws what it is given.

`keyGlyph` is `constexpr` and returns a `std::string_view` onto a string literal, so a glyph is
valid for the whole program and costs nothing to look up. `isGamepadKey` lets the HUD put the
keyboard-and-mouse keys before the gamepad keys without a second key list.

## 2. The spellings

Every glyph is 7-bit ASCII. The HUD draws with the engine's small fixed-size font, and an arrow or a
controller-button symbol outside ASCII is not guaranteed to have a glyph in it. That is why the D-pad
directions are spelled out (`D-pad Up`, not an arrow).

* Letters and the number row are the key's own character. The number row is the ONLY way a digit
  reaches the panel: the control-mode text table may hold no digit at all (see
  `ControlModeTexts-rationale.md` §4), so a hotkey's name always comes through a binding.
* The face buttons use the Xbox letters (`Pad A` for `KeyId::Gamepad_FaceBottom`, and so on) and the
  View button is `View` (`KeyId::Gamepad_Special_Left`, also called Back). The brawler names no other
  pad family, so a PlayStation player sees the Xbox names.
* The face buttons carry the `Pad` prefix so that no two glyphs are equal: without it the keyboard
  `A` and the pad's `A` would print the same text on a row that lists both.
* Triggers have two forms each in `dInput::KeyId`, a digital one and an analog one. `LT` and `RT` are
  the digital keys, `LT axis` and `RT axis` the analog ones, so a row bound to both stays readable.
* `Mouse` is the two-axis mouse (`KeyId::Mouse_XY`); the single axes are `Mouse X` and `Mouse Y`.
  `LS` and `RS` are the two-axis sticks.

The full list is pinned, glyph by glyph, by the brawler test case
"ControlsHud: keyGlyph spells each KeyId as the backlog lists it" in
`Visualization/BrawlerControlsHudTextsTest.cpp`.

## 3. Why there is no default case, and the tripwire

`keyGlyph` is one `switch` with a case per enumerator and no `default`; after the switch it returns
an empty view. On MSVC the switch sits between a `#pragma warning(push)` that makes C4062
("enumerator not handled") an error and the matching pop, so a new `KeyId` without a glyph does not
compile. That is the same device `dAttackStateName` in `OGBrawler/DAttackMachineSimulation.h` uses.
On another compiler the empty return is what the test sees: every enumerator from `KeyId::Key_A`
through the pinned value `kLastKeyIdValueAsShipped` must have a non-empty glyph.

The `static_assert` below the functions pins three facts at once:

1. `KeyId::Gamepad_Special_Left` has the value `kLastKeyIdValueAsShipped` (62: 26 letters, 4
   modifiers, 10 digits, 6 mouse keys and 17 gamepad keys, counting from 0);
2. it has a glyph;
3. the value after it has none.

What each kind of enum edit then meets:

* **an enumerator appended without a glyph**: C4062, at the `keyGlyph` switch;
* **an enumerator appended with a glyph but no gamepad classification**: C4062, at the
  `isGamepadKey` switch (§4);
* **an enumerator appended with both**: the `static_assert` (fact 3);
* **an enumerator inserted anywhere before the last one**: C4062 at both switches, and the
  `static_assert` (fact 1).

All of them force a visit to this header, and the assertion's message names the other places a new
key needs: its row in the brawler test's `kExpectedGlyphs` (and in `kGamepadKeys` for a pad key), and
`InputMappingUETranslator::toUnrealKey`, whose `default` case returns an invalid key and so drops a
binding without any error. The first three were seen firing when task 2 and its rework appended a
dummy enumerator under a build lock.

⚠ The pin is a value, not a "last enumerator" query: `dInput::KeyId` has no count enumerator, and
adding one would change og-simulation, which this header must not do for a HUD.

## 4. `isGamepadKey` is a second exhaustive switch

`isGamepadKey` is a `switch` with a case for every enumerator and no `default`: the 46 keyboard and
mouse keys fall through to `return false`, the 17 `Gamepad_*` keys to `return true`. It sits inside the
same C4062-as-error pragma block as `keyGlyph`, so on MSVC a new `KeyId` does not compile until it is
classified, whichever block it joins and wherever it is added. A range test (`Gamepad_LeftStick_XY`
through `Gamepad_Special_Left`) would silently class an appended gamepad key as keyboard and mouse,
which is why it is not one. On another compiler the `return false` after the switch is what a
missing case gives, and the test below catches it for a gamepad key.

The brawler test checks the classification against an explicit list of the 17 gamepad keys
(`kGamepadKeys`), not against the switch. Its loops over `KeyId` end at `kLastKeyIdValueAsShipped`, so
moving the pin of §3 extends every loop: the new key's glyph is then checked for emptiness, ASCII and
uniqueness, its classification against the list, and the `kExpectedGlyphs` size check fails until
that list gains the new key's row.

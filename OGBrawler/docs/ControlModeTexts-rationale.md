<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `ControlModeTexts.h` / `ControlModeTexts.cpp` — rationale

Companion to `OGBrawler/InputMapping/ControlModeTexts.h` and `OGBrawler/InputMapping/ControlModeTexts.cpp`:
the one data table that says, for each camera and control-mode hotkey, what the controls HUD calls
it and what its help legend says about it. **The code carries the table; this file carries the
reasoning.** There is **no guards file**: the rules the table must keep are held by the brawler test
suite (§6), and no prohibition here is one a tag at a line would stop.

Written by og-brawler-controls-hud task 2 (2026-10-31), as a new file pair in the new comment
convention. The legend texts are the initiative's first draft; the user tunes them at its gate G1.

---

## 1. One table, three readers

`ControlModeTexts.h` :: `ControlModeEntry`, `ControlModeKind`, `controlModeEntries`, `findControlModeEntry`

The camera-look toggle, the four movement-scheme shortcuts and the legend toggle each appear in
three places once the controls HUD is built: an indicator row, a legend section, and (for the four
schemes) the UE input handler that sets the scheme when the key is pressed. If each place kept its
own list, the HUD could light one row while the key selected another scheme. The table is built so
that the three read the same entry: the HUD by walking `controlModeEntries`, the handler by
`findControlModeEntry` on its own action. The entry's `scheme` is then both what the key selects and
what the HUD highlights.

The order of the entries is the indicator's row order: camera look, then the scheme shortcuts in key
order (Camera-relative, Aim-relative, Aim follows move, Aim-relative with swapped sticks), then the
legend toggle.

The table holds no key names. A row's keys come from `dInput::gameMapping::buildDefaultContext`
through `dInput::keyGlyphs::keyGlyph` (see `KeyGlyphs-rationale.md`), so a rebinding changes the HUD
with no edit here.

## 2. Why the table is behind a function, in the `.cpp`

The action descriptors in `GameInputMapping.h` (`dInput::gameMapping::BlockLook` and its siblings)
are `extern OGBRAWLER_API` objects, so that one instance, and so one address, exists across the
module boundary. Their addresses are not constant expressions in a module that imports them, so a
`constexpr` table in a header could not hold them. The table is therefore built in the `.cpp`, in the
same module as the descriptors, as a function-local `static const` vector, and handed out by const
reference.

The descriptors are constant-initialized (`const` aggregates with literal initializers), so their
addresses are valid before any code runs, whichever caller builds the table first. The function-local
static is built once, thread-safely, on the first call.

`findControlModeEntry` is a linear search over six entries; it returns a null pointer for an action
the table does not list (and for a null action).

## 3. What each field means

* `action` is the identity of the entry. No two entries share one.
* `kind` decides which other fields mean anything:
  * `ControlModeKind::CameraLook` (one entry, `BlockLook`): `lines` is the body while look mode is
    OFF and `linesWhenOn` the body while it is ON.
  * `ControlModeKind::MovementScheme` (one per scheme): `scheme` is the scheme the key selects;
    `lines` is the body; `linesWhenOn` is empty.
  * `ControlModeKind::LegendToggle` (one entry, `ToggleControlsLegend`): no body at all.
* ⚠ `scheme` is meaningful only for the MovementScheme kind. The other two entries carry a
  value-initialized scheme, which is `MovementScheme::CameraRelative`; reading it without checking
  `kind` first gives a plausible wrong answer.
* `shortLabel` is the indicator row's text and `title` the legend heading. For the schemes the two
  are the same today; they are separate so the user can tune one without the other.

## 4. The string rules

Every string is a `std::string_view` onto a string literal, so the table owns no memory and a view
never dangles. Text built at run time must not be stored in an entry.

The brawler test suite requires of every string in the table:

* **7-bit ASCII**: the HUD draws with the engine's small fixed-size font, as `keyGlyph` does;
* **at most 64 characters**: the small font and the legend panel's width;
* **no digit**: a hotkey's name reaches the screen only through the row's key list, built from the
  bindings. A digit typed into the text ("press 8") would go stale on the first rebinding, which is
  exactly what one table was meant to prevent. The legend therefore names the other schemes by their
  labels, never by their keys.

Lines that continue the previous line start with two spaces, meant to be drawn as they are.

## 5. What each sentence describes, and the code that makes it true

The legend sentences describe the shipped defaults: `g_movementScheme` AimRelative,
`g_gamepadMoveStickFeedsAim` true, `g_swapMoveAndAimSticks` false. They were checked against the code
when task 2 wrote them; when that code changes, this is the list of sentences to re-read.

* **Camera look, OFF** ("the camera holds still and the mouse cursor aims"): with look mode off,
  `dAttackCameraBehaviour::integrate` returns before any yaw or pitch change, so the orbit camera
  does not turn. It still follows the character; "holds still" means it does not rotate. The cursor
  is shown and the mouse aim is resolved (`UOGBrawlerInputCollectionComponent::updateGameThreadCache`).
  The right stick still aims ahead of the mouse when it is pushed.
* **Camera look, ON** ("the cursor hides; mouse and aim stick turn the camera"): the cursor is
  hidden and the mouse aim cleared in the same function; the character's tick feeds the mouse look
  delta and the routed aim stick into `dAttackCameraBehaviour::integrate`, which turns the camera by
  their sum.
* **Camera look, ON, "In Aim-relative you aim where the camera faces"**: `dInput::stickRouting::cameraLookAim`
  answers only for `MovementScheme::AimRelative` with look mode on, and
  `UOGBrawlerInputCollectionComponent::buildAimDirection` takes it ahead of every other aim source.
  It does not answer for the swapped variant; with keyboard and mouse only, the swapped variant ends
  on the same aim anyway (no stick, no mouse aim, so the flattened camera forward).
* **Move lines**: `UOGBrawlerInputCollectionComponent::buildMoveDirectionWorld` turns the routed move
  stick (movement keys plus left stick, `dInput::stickRouting::routeSticks`) relative to the camera,
  except in the Aim-relative family, where it turns it relative to the aim. "Up walks toward it, left
  and right strafe around it": up is the aim direction and left and right are perpendicular to it.
  With the mouse the aim is recomputed toward the cursor's point every frame, so a strafe circles that
  point; with the right stick the aim is a fixed direction and a strafe is a straight sideways walk.
* **Aim lines**: `UOGBrawlerInputCollectionComponent::buildAimDirection` takes, in order, the routed
  aim stick past its deadzone (relative to the camera, or in Aim follows move relative to the move
  direction when there is one, else the camera: "camera-relative when still"), then the gamepad
  fallback below, then the mouse aim, then the flattened camera forward.
* **"Gamepad: right stick released, ..."**: the gamepad fallback. With the aim stick released, the
  last input from the gamepad and the move stick past its deadzone, the aim is the move stick turned
  relative to the camera. In Camera-relative and Aim follows move the move is turned relative to the
  camera too, so you aim where you walk. In the Aim-relative family `buildMoveDirectionWorld` drops to
  camera-relative in exactly that case, so the left stick both moves and aims.
* **Swapped sticks**: `dInput::stickRouting::routeSticks` gives the move to the movement keys plus the
  right stick and the aim to the left stick, except while the right stick is released (with the
  fallback on), when the left stick is the move again and the fallback above makes it aim as well.

## 6. What the tests pin

`Visualization/BrawlerControlsHudTextsTest.cpp` in og-brawler-tests, tag `[CharacterViz][ControlsHud]`:
the row order by action address; that every entry's action is bound in `buildDefaultContext` and no
two entries share one, and that `findControlModeEntry` finds each entry and nothing else; exactly one
scheme entry per `MovementScheme` enumerator, each paired with its shortcut action as shipped; one
camera-look entry with both bodies and one legend-toggle entry with none; and the string rules of §4.
Each case was seen failing against a poisoned table when task 2 added it.

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerControlsHud.h` — rationale

Companion to `OGBrawler/BrawlerControlsHud.h`, which holds the whole model of the controls HUD: the
input it is drawn from, the key text of a row, the indicator rows, the legend lines, the layout, the
two console clamps and the inks. **The header carries the code; this file carries the reasoning.**
There is **no guards file**: every rule the header must keep is either a compile-time check in the
header itself or a brawler test (§9), and no prohibition here is one a tag at a line would stop.

Written by og-brawler-controls-hud task 3 (2026-10-31), as a new header in the new comment
convention. It is modelled on `BrawlerScoreboardVisualization.h` and its rationale: the same split
between a pure, tested model here and a UE layer that only gathers the input and makes the canvas
calls.

---

## 1. Why the model is pure code

The brawler test target links og-brawler but not the UE game module, so anything written against a
canvas cannot be tested there. Everything that decides whether the HUD is *right* — which rows show,
which one is lit, which keys a row names, what the legend says, where each line goes and what a bad
console value turns into — is therefore decided here, where a Catch2 case reaches it. The UE layer is
left with the gather (filling `ControlsHudInput`), measuring text and drawing.

⛔ **Every string, ink and length the UE layer draws comes from this header** (or, through it, from
the control-mode table and the mapping context). The UE layer decides no geometry beyond measuring
text width, and types no text of its own. A second copy of any of it in the UE layer would let the
two drift apart unseen, because only this side is tested.

## 2. The input

`BrawlerControlsHud.h` :: `ControlsHudInput`, `CameraView`

`ControlsHudInput` is everything the model reads besides the mapping context: the active movement
scheme, which camera is the view, whether camera look mode is on, whether the legend is open and
whether the legacy stick swap console variable is on. It is a plain value, so a test builds any
combination directly, and the UE layer fills it once per draw.

`legendOpen` is the help state (user ruling R6, task 6). Closed is the default: the indicator is
**collapsed** to its keys column and no legend is drawn. Open **expands** it: every row shows its label
and state too, and the legend opens below. One field drives both, so the rows and the legend cannot
disagree about whether help is open.

`CameraView::Solo` means the third-person camera of the one local player is the view;
`CameraView::Shared` means the shared camera of two or more local players is. The user ruled (R1) that
the camera-look control applies only to the solo camera, so in the shared view the camera-look row and
the legend's camera-look section are not drawn.

The defaults (Aim-relative, solo, look off, legend closed, no swap) are the shipped game defaults. They
only matter to a caller that forgets a field; the UE gather sets all five.

## 3. The key text of a row

`BrawlerControlsHud.h` :: `bindingText`, `kBindingSeparator`

A row names its keys from the mapping context, never from a list of its own, so a rebinding changes the
HUD with no edit here (the test rebinds two actions to prove it). User ruling R2: every key bound to the
action is listed, keyboard and mouse keys first, then gamepad keys (`dInput::keyGlyphs::isGamepadKey`),
each group in binding order, joined by `" / "`. There is no device detection.

* An action may appear in more than one mapping; all of them are read, in context order.
* A key bound twice (for example once plain and once with a modifier) is listed once. The modifier is
  not shown: it changes the value a key feeds, not which key to press.
* An action with no binding gives an empty string. The legend heading of such an action then starts
  with the separating space. No action the HUD shows is unbound in the default context (the control-mode
  table test pins that), so this is only reachable with a custom context.

## 4. The indicator rows

`BrawlerControlsHud.h` :: `indicatorRows`, `IndicatorRow`, `RowInk`, `kCameraLookOnState`, `kCameraLookOffState`, `kActiveSchemeState`

One row per control-mode table entry, in table order, which is the order the table rationale fixes
(camera look, the four schemes in key order, the legend toggle). Each row has three columns: keys,
label (the entry's short label) and state, plus an ink.

* **Camera look**: drawn only in the solo view (R1). State `ON` or `OFF`; ink Active when on, Normal
  when off. ON is the state that changes what the mouse does, so it is the one that stands out.
* **A scheme**: the row whose scheme equals the active one has ink Active and state `<`; every other
  scheme row is Dim with no state. Exactly one scheme row is lit for each scheme.
* **The legend toggle**: ink Normal, no state.

The switch over the entry kind has no `default` and sits under MSVC's C4062 raised to an error, so a new
`ControlModeKind` fails the build here until it is given a row rule.

### 4.1 Collapsed and expanded

The rules above are the **expanded** rows (help open). **Collapsed** (help closed, the default), every
row keeps its keys and its ink exactly as above and has an empty label and an empty state (user ruling
R6, task 6). So the collapsed indicator is the keys column alone: the active scheme's key in the Active
ink, the other scheme keys Dim, `5 / Pad Y` Active while look mode is on and Normal while it is off
(solo view only, as above), and `H / View` Normal. The ink alone carries what the label and state
said, which is why the inks do not change between the two modes.

The collapse is one step after the switch, applied to every row alike, not a second switch. The row
rules are therefore written once, and a new `ControlModeKind` needs a rule only in the switch. The
row count does not change either, so the layout helpers and the legend position are the same in both
modes.

## 5. The legend

`BrawlerControlsHud.h` :: `legendLines`, `keysThenTitle`, `schemeEntry`, `firstEntryOfKind`

Empty while the legend is closed. Open, in this order:

1. the active scheme's keys and title, then its body lines;
2. a blank line;
3. in the solo view, the camera-look entry's keys and title, then its ON or OFF body (by
   `cameraLookOn`); in the shared view, the single line `kSharedCameraLine` instead;
4. `kLegacyStickSwapNote`, only when the legacy swap is on and the scheme is not Aim-relative with
   swapped sticks;
5. the legend toggle's keys followed by `kCloseHelpSuffix`, so the last line always says how to close it.

The texts come from the control-mode table (`ControlModeTexts-rationale.md`) or from the named
constants in this header. The same string rules hold for the constants as for the table: 7-bit ASCII,
at most 64 characters, and no digit, because a key name reaches the screen only through
`bindingText`. The test also checks that every legend line built from the default context, in every
combination of the input, is ASCII and at most 64 characters.

### 5.1 What the two fixed lines describe

Both are the Backlog's text after the lead ruling of 2026-10-31 (no digit, at most 64 characters);
the user tunes them at gate G1. Checked against the code by task 3:

* **The shared-camera line** ("it follows every player and does not turn"). The shared camera frames
  the positions of every *locally controlled* brawler (`ASharedIsometricCameraActor`'s per-frame
  update collects only the characters that are locally controlled), and its rotation is the
  actor's fixed pitch and yaw unless the iso camera console variables are changed; no player input
  turns it. ⚠ "Every player" means every local player: in an online session the remote players are
  not framed. The view itself is chosen by `AOGBrawlerPlayerController::computeDesiredViewTarget`:
  two or more local brawlers select the shared camera.
* **The legacy swap note** ("swaps the sticks"). With the swap on and any scheme but the swapped one,
  `dInput::stickRouting::routeSticks` makes the right stick the move and the movement keys plus the
  left stick the aim. ⚠ So the movement keys move with the left stick to the aim; the note says only
  the sticks. In the swapped scheme `routeSticks` returns before it reads the swap, so the swap has no
  effect there, and that is why the note is left out for that scheme.

## 6. The layout

`BrawlerControlsHud.h` :: `ControlsHudLayout`, `scaledControlsHudLayout`, `indicatorPanelWidth`, `indicatorRowTopY`, `indicatorHeight`, `legendTopY`, `legendHeight`, `legendLineTopY`, `legendTextX`, `indicatorKeysX`, `indicatorLabelX`, `indicatorStateX`

The panel is top-left (user ruling R4) and the legend opens below it. All values are in canvas
pixels at scale 1:

```
originY ┬──────────────────────────────── panelWidth ─┐
        │ padding                                    │
        │ keysX   labelX               stateX         │  rowHeight each
        │ 5 / Pad Y  Camera look          OFF         │
        │ ...                                         │
        │ padding                                    │
        ┴─────────────────────────────────────────────┘
          legendGapY
        ┌──────────────────────────────── legendWidth ──────────┐
        │ padding, then legendLineHeight per line, padding       │
        └────────────────────────────────────────────────────────┘
```

* The x offsets (`keysX`, `labelX`, `stateX`) are measured from `originX`, the panel's left edge. The
  legend's text starts at the same x as the keys column (`legendTextX`).
* `padding` is inside each backdrop, above the first row or line and below the last.
  So `indicatorRowTopY(layout, i)` is `originY + padding + i * rowHeight`. `indicatorHeight` is
  `2 * padding + rowCount * rowHeight`.
* The indicator backdrop's width is `indicatorPanelWidth(layout, expanded)`: `panelWidth` (330) when
  help is open, `compactPanelWidth` (80) when the indicator is collapsed to its keys column (§4.1). The
  80 is the keys column's offset (6), plus the longest default keys text, `5 / Pad Y` (the Backlog's
  estimate is about 63 px at the small font; not measured here), plus the padding (4), rounded up. A
  longer keys text (after a rebind) does not overflow: the UE layer widens a backdrop to the measured
  text. The legend's width is unchanged.
* The legend's backdrop starts `legendGapY` below the indicator's backdrop
  (`legendTopY = originY + indicatorHeight + legendGapY`), which puts it below the last row's bottom
  plus the gap. Its lines start at `legendLineTopY`.
* The helpers take the row count rather than assuming six, because the shared view draws five rows.

`scaledControlsHudLayout` multiplies every length and `textScale` by the scale and leaves the origin
where it is, so the panel grows from its top-left corner and stays in the corner at every scale.

⚠ **Not measured here: whether the text fits the columns.** The model has no font. The widths are a
first draft that the user tunes at G1; the UE layer measures the text and sizes the backdrops from it.
By character count at scale 1, the keys column (86 px) holds the default "5 / Pad Y" (9 characters)
but a third key on one action, such as "5 / MMB / Pad Y" (15), is likely wider than the column. The
label column (208 px) holds the longest label (28 characters), and the legend (470 px) the 64-character
cap.

## 7. The console clamps

`BrawlerControlsHud.h` :: `clampControlsHudScale`, `clampControlsHudBackgroundAlpha`, `isNotANumberByBits`, `kControlsHudDefaultScale`, `kControlsHudDefaultBackgroundAlpha`

The scale's default is 1 and its range 0.25 to 4, as in the scoreboard; the backdrop alpha's default is
0.4 (the scoreboard's is 0, no backdrop) and its range 0 to 1. The clamps live here so that the UE
layer's console accessors call them instead of re-implementing the range, which keeps the range tested. A value below the range or a NaN gives the minimum, above
the range gives the maximum. Plus infinity gives the maximum and minus infinity the minimum.

The NaN case is decided on the bits, not by a comparison. The scoreboard's own test records that on
this build (fast floating-point mode) the compiler may assume no NaN exists, and it evaluated a NaN
less-than 0.25 as true, so
which branch a NaN takes in a comparison chain is up to the optimiser. `isNotANumberByBits` (exponent
all ones, mantissa non-zero) does not depend on that. A `static_assert` pins it to true for every kind
of NaN and false for infinities, zeros, the largest float and the smallest denormal. A second
`static_assert` pins each default inside its own range.

## 8. The inks

`BrawlerControlsHud.h` :: `ControlsHudInk`, `controlsHudRowInk`, `kControlsHudActiveInk`, `kControlsHudNormalInk`, `kControlsHudDimInk`, `kControlsHudBackdropInk`

Linear 0..1 RGB triples, turned into engine colours only at the draw site, so this header gains no
engine type. Active is a warm yellow (1, 0.85, 0.2), Normal a light grey (0.9, 0.9, 0.9), Dim a mid
grey (0.55, 0.55, 0.55) and the backdrop black. The text inks are opaque. The backdrop's transparency is
the separate background alpha, so it is the one value the console varies. A `static_assert` keeps the
ink at exactly three floats. The row-ink switch is exhaustive under C4062, like the row switch.

## 9. What the tests pin

`Visualization/BrawlerControlsHudTest.cpp` in og-brawler-tests, tag `[CharacterViz][ControlsHud]`:

* the six default rows exactly (keys, label, state, ink), expanded;
* the six default rows collapsed (keys and inks kept, every label and state empty), and, in every
  combination of the input, each collapsed row equal to its expanded row in keys and ink;
* the panel width, compact when collapsed and full when expanded, at scale 1 and 2;
* `bindingText`'s order, merging and de-duplication on a hand-built context;
* the shared view without the camera-look row and section, with the shared-camera line;
* each scheme lighting exactly its own row and heading the legend;
* camera look flipping the row's state and ink and the legend body;
* a rebinding reaching the rows and the legend;
* the closed legend empty in every combination, the open one ending with the close line, and the
  whole default legend;
* the legacy swap note's condition;
* scaling (every length doubled, the origin kept);
* both clamps, including NaN and the infinities;
* the legend below the indicator at four scales, both clamp ends included;
* the inks and the string rules.

The shared-view, scheme, look and rebinding cases check both modes. Each case was seen failing
against a poisoned header when task 3 added it, and each case task 6 added or changed was seen
failing again then. The compile-time checks were
seen firing too: an out-of-range default, a NaN test that counts infinity, and a new `RowInk` or
`ControlModeKind` enumerator.

<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: CommentExtractionRule_v2.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl_notes_seam_16.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
# `DAttackMachineSimulationRuntimeTweakables.h` — rationale

This is the narrative, the derivations and the provenance for the runtime tweakables and the
refused-name table: `MovementScheme`, the five `g_*` globals, `RefusedVariable`,
`refusedVariablesBegin`/`refusedVariablesEnd`, `refusedVariableReason` and `SetVariable`.
Under `CommentExtractionRule_v2.md` the header keeps **no prose at all**: a licence line, a
two-line pointer, code, and one `⛔G-nn` tag per guard. Every fence is in
`DAttackMachineSimulationRuntimeTweakables-guards.md`, each with a stable id, and **one of the
six is no longer text anywhere** — it became a `static_assert` in the header. This file carries
everything else.

**If this file and `DAttackMachineSimulationRuntimeTweakables.h` disagree, the header is
authoritative and this file is stale.** Fix this file; do not soften the header to match it.

⛔ **Do not put a fence in this file.** A prohibition belongs in
`DAttackMachineSimulationRuntimeTweakables-guards.md`, with an id and a `⛔G-nn` tag on the
declaration where the wrong line would be typed — or, better, in a `static_assert` so that it
cannot be skimmed past at all. `tools/lint/guard_tag_lint.ps1` is a hard gate on that join in
both directions; a fence filed here has no site and no lint row.

⛔ **This file is not the source of truth for any VALUE.** The refused names, their reason
strings and the shipped defaults of the five globals live in
`DAttackMachineSimulationRuntimeTweakables.cpp`; the console variables, their tombstones and the
stale-ini sweep live in `Source/OGBrawlerUnreal/MovementSchemeCVar.cpp`. Where a value appears
below it is there to make an argument readable.

**Read alongside:** `DAttackMachineSimulationRuntimeTweakables-guards.md` — the prohibitions, by
id. `Source/OGBrawlerUnreal/MovementSchemeCVar.cpp` — the UE half of the refused-name path, and
the file this header's claims are mostly claims *about*.
`RuntimeTweakablesRefusedNamesTest.cpp` (under `Source/OGBrawlerTests`) — what the table is
actually pinned to.

Origin: `brawler-movement-simulation` tasks 16 (the refused-name path), 56 and user ruling #28
(the obligation it discharges), 57 and ruling #29, 65 (the R0 audit and the must-never-move
list), and 77 (this conversion). The UE-side half of the path is demonstrated by the headless
run recorded in `impl_notes_seam_16.md`.

> ⚠ **Every task and ruling number in that Origin line is private working material from the
> initiative archive and is NOT distributed with this submodule.** They are named as provenance,
> deliberately unlinked; every claim this file *asserts* is anchored to a file in this
> repository and only to those.

---

## 1. What this header is

Five process-global tweakables and a table of names that are **refused**. The globals are
`std::atomic` because they are written from the UE game thread (a console-variable sink, or the
named-pipe `SetVariable` handler) and read from wherever the input collection runs; the
`OGBRAWLER_API` export is what lets the UE module see them at all.

Nothing here is simulation state. Every one of the five is read by the UE layer only, and the
sub-simulations take the values they need as **parameters** rather than reading a global —
`BrawlerInputHistoryVisualization.h` states that discipline at its own seam.

## 2. The movement schemes

`MovementScheme` is set from `OGBrawler.MovementScheme` (a console variable, in
`MovementSchemeCVar.cpp`), from the named-pipe `SetVariable` path, and from the debug keybinds
whose input actions are declared in `GameInputMapping.cpp` and handled in
`UOGBrawlerInputCollectionComponent`. The third enumerator is the one that needed explaining;
this is the text that stood on it:

> Movement direction is camera-relative (like CameraRelative). Aim direction is
> rotated relative to the current movement direction — aim-stick-up makes the aim
> direction equal the movement direction. With no movement input, aim falls back to
> camera-relative interpretation of the aim stick. UE-side only; sim doesn't read it.

The branch that implements it is in `UOGBrawlerInputCollectionComponent::buildAimDirection`: the
aim reference starts at the cached camera forward and is replaced by the move direction only
when there is one, which is the "falls back to camera-relative" half. Every reader of
`g_movementScheme` is in the UE layer, and so is every writer except `SetVariable` (this
header's named-pipe path); the simulation sees only the resulting directions.

The fourth enumerator, `AimRelativeSwapped`, is AimRelative with the two gamepad sticks
exchanged: the right stick is the move stick and the left stick is the aim stick. What each raw
source feeds is decided in one place, `dInput::stickRouting::routeSticks` in
`InputMapping/StickRouting.h`. It takes the scheme, the two stick toggles and the move deadzone as parameters
and reads no global. With the right stick at or inside the move deadzone and
`g_gamepadMoveStickFeedsAim` on, that function routes this scheme exactly as it routes
AimRelative with `g_swapMoveAndAimSticks` off, so the single-stick fallback is shared rather than reimplemented.
`isAimRelativeFamily` is the test for "AimRelative or AimRelativeSwapped", for the call sites
that treat the two alike.

On the named-pipe path `SetVariable` takes the scheme by **name**. The legacy numerals it still
accepts are not the enum values: `"3"` means CameraRelative there, and `AimRelativeSwapped` is
reached only by its name. `StickRoutingTest.cpp` in og-brawler-tests pins both.

## 3. The stick deadzones

The prohibition — that every reader of the two deadzones is enumerated at the declaration — is
**G-01** in the guards doc. What is not a prohibition is the ownership note that stood with it:

> Sim layer does not read these; they are tweakables-by-convention living next to
> g_movementScheme so the named-pipe SetVariable path can tune them at runtime.

"Tweakables-by-convention" is the whole of their claim to live in this header: they are not part
of the movement scheme, they are simply tunable by the same two paths, and `SetVariable` clamps
both to `[0, 1]` through `parseClampedFloat`.

## 4. The two UE-side input toggles

Both are flags the simulation never sees. This is the text that stood on each.

**`g_swapMoveAndAimSticks`:**

> When true, the physical stick assignments are swapped: the right (aim) stick feeds
> the move direction and the left (move) stick feeds the aim direction. UE-side only;
> the sim sees the resulting (moveStick, aimDirection, moveDirectionWorld) tuple either
> way and does not read this flag.

It is implemented in `dInput::stickRouting::routeSticks` (rows 1 and 2 of its table), which
`UOGBrawlerInputCollectionComponent::getMoveStick` and `::getAimStick` call, with no sign flips.
The "left (move) stick" of the quote is the summed WASD + D-pad + left-stick input, so the swap
moves the keys onto aim too. `AimRelativeSwapped` ignores this flag.

**`g_gamepadMoveStickFeedsAim`:**

> Gamepad-only feature toggle: when true (default), if the aim stick is below its
> deadzone and the most recent move input came from the gamepad left stick, the move
> stick feeds BOTH the movement direction and the aim direction (both resolve to the
> same camera-relative move-stick vector). When false, the gamepad case falls through
> to mouse aim / camera forward like any other input source. Keyboard+mouse play is
> unaffected either way (the latch m_lastMoveInputWasGamepad gates this).

The latch is a member of `UOGBrawlerInputCollectionComponent`. It starts false. A gamepad stick past its
deadzone or a D-pad move sets it; a WASD move with no D-pad direction held, or the primary player's
mouse cursor travelling a few pixels, clears it. That is what makes the "keyboard+mouse is unaffected"
half true rather than merely intended. Until og-attackstatetransition-cleanup task 13 any non-zero
stick event set it, and a resting pad's drift broke that half (§10).

Since og-brawler-3rdControllerMode task 5, both toggles also shape one bit the simulation does
see, still without the simulation reading either of them. `UOGBrawlerInputCollectionComponent::buildPlayerInput`
passes them, with `g_movementScheme` and `g_moveStickDeadzone`, to
`dInput::stickRouting::guardFreezeRequested`. That function decides whether a held guard raises
`brawlerMovementSimulation::kInputFlagHoldGuard`, and it raises the bit only when the scheme's
own move (routed with the fallback off, so `g_swapMoveAndAimSticks` picks which stick it is) or
the actual routed move (with the real `g_gamepadMoveStickFeedsAim`) is below the move deadzone.

## 5. The refused-name path — where the obligation came from

This is the provenance banner that opened the second half of the header:

> ⭐⭐ NAMES THIS PATH REFUSES, AND WHY — movement-sim TASK 16, 2026-09-08. The
> obligation was ROUTED HERE FROM TASK 56: ruling #28 removed `maxSnapSpeed`, and task 56 had
> no cvar/ini path to reject it against, so it left the breadcrumb in the hover-gains group
> banner in BrawlerMovementSimulation.h (immediately above `hoverFrequency`) and handed the
> work to this task.

⛔ **The location in that paragraph was FALSE as it shipped and was corrected before it moved
(R0).** It said the breadcrumb sat *"at `hoverMaxAccel`'s declaration"*. It does not: the
breadcrumb is in the group banner that opens with *"THE HOVER SERVO'S GAINS, IN FEEL UNITS"*,
whose first annotated declaration is `float hoverFrequency;`. `float hoverMaxAccel;` is declared
some thirty-five lines further down and its own comment is about the acceleration ceiling and
says nothing about `maxSnapSpeed`. Correct name, wrong owner.

⚠ **Two things outside this file are routed rather than fixed here.** The same false location is
repeated in `RuntimeTweakablesRefusedNamesTest.cpp`'s file banner; and the breadcrumb itself, in
`BrawlerMovementSimulation.h`, still reads *"there is no cvar/ini path in the tree yet (task 16
builds it)"* — task 16 has landed and this header is its output, so the breadcrumb should now
point at the refused table instead of promising a future path. Neither file is this task's to
edit.

**What the table actually drives** is in the guards doc under **G-03**, because it is a
prohibition: adding a `refusedOnCVarPath` row arms the named-pipe refusal and the raw-ini sweep
automatically, and obliges you to hand-write the console tombstone yourself.

## 6. `refusedOnCVarPath`, and the one row where the two paths disagree

> ⚠ `refusedOnCVarPath` is FALSE for a name that is dead on THIS path but alive as a cvar.
> `MoveSpeed` is exactly that: task 16 deleted the sim-side walk-speed global and made it a
> ONE-TIME read of the live `OGBrawler.MoveSpeed` cvar into the movement sub-simulation's
> StaticData, so the named-pipe name is dead while the cvar name must keep working.

It is the only `false` in the table, and it is deliberate: getting it backwards would tombstone
`OGBrawler.MoveSpeed` and break the very read task 16 added.
`RuntimeTweakablesRefusedNamesTest.cpp` gives that one row a case of its own for exactly that
reason.

## 7. The `reason` string

> Printed verbatim in the rejection. Says what happened to the constant and where the
> value lives now, because "unknown variable" is what made the old behaviour useless.

Both halves of the path print it unmodified — the named-pipe handler with `%s`, the console sink
with `%hs` — so the string is the operator-facing diagnostic and not an internal label. The
tests assert on its *content*, not merely that it is non-null: an empty or generic string would
pass a `!= nullptr` check while leaving the defect exactly where it was.

## 8. What this conversion did, and what it measured

The header went from 97 lines (the R0-corrected pre-conversion state) to a file whose only
English is the licence line, the two-line pointer, six one-line tags and one `static_assert`
message. Six fences were identified; **one** of them — the module-boundary constraint on the
half-open pointer pair — became a compile-time check and took no guard entry, and the other five
became tags. The R0 pass corrected **five false claims carrying six defects** before anything
moved, which on 88 lines is the worst defect density measured in this phase and is the reason
this file was singled out for conversion at all.

The honest reader test — *"if I were about to make the edit `G-nn` forbids, would this tag stop
me?"* — is answered per guard in the task's implementation notes, not here, because it is a
measurement of the method rather than a fact about this code.

## 9. The camera-look aim in AimRelative

User decision (og-attackstatetransition-cleanup task 11): in AimRelative, while BlockLook is held, the
look stick keeps turning the solo orbit camera, and the aim is the camera's forward, flattened.
`dInput::stickRouting::cameraLookAim` in `InputMapping/StickRouting.h` is that decision as a pure
function. It takes the scheme, the BlockLook button and a camera forward, and reads no global. It returns
the unit horizontal part of the forward when the scheme is AimRelative and BlockLook is held, and
`std::nullopt` otherwise. `UOGBrawlerInputCollectionComponent::buildAimDirection` calls it first, ahead
of the aim stick, the move-stick fallback and the mouse; on `std::nullopt` the scheme's usual aim applies.

**Since task 12, BlockLook is a toggle.** The 5 key or gamepad Y flips look mode once per press, and it
starts off for a new pawn. "Held" in this section therefore means "look mode is on", and the parameter
keeps its name `blockLookHeld`. The rule did not change: it reads the state, not the button.

**AimRelative only.** `isAimRelativeFamily` is deliberately not the test: AimRelativeSwapped is unchanged
until the user decides otherwise. There the look stick is `getAimStick`, which is the left stick while the
right stick is in use. With both sticks in use, the left stick turns the camera AND aims relative to the
camera forward, so the aim turns with the camera. With the left stick alone (the single-stick fallback)
the left stick is routed to move and to aim, and the camera does not turn.

**Which camera forward.** The caller passes `m_camForwardCache`, the camera manager's rotation that
`updateGameThreadCache` reads at the start of the character's `Tick`. That is the view on screen, one
frame old, not the boom this `Tick` is about to write. The reasons:

* It is "where the camera looks" as the player sees it, also during a view-target blend and under the
  shared iso camera, where the boom is not the view.
* It is the basis every other branch of `buildAimDirection` and `buildMoveDirectionWorld` already uses,
  so the movement while looking (below) is exactly the CameraRelative movement.
* It needs no new value handed from the game thread to the physics thread.
* The cost is one frame: at most one frame of the look stick's yaw rate.

**A camera with no horizontal forward.** Below `kMinCameraLookAimHorizontalLength` (1e-3) the function
returns `std::nullopt`. The orbit camera's steepest pitch, `dAttackCameraBehaviour::kHardPitchMaxDeg`
(89°), still leaves a horizontal part of about 0.017, and a test pins that it aims. Only a view looking
straight down has none.

**What else changes while AimRelative + BlockLook is held.**

* **Movement** rotates around the aim in AimRelative (`buildMoveDirectionWorld`), so it is
  camera-relative while looking. The gamepad branch that skips the aim (the move stick feeding the aim)
  is camera-relative through the same cache, so the two branches give the same direction.
* **The move stick no longer turns the aim.** With the aim stick at rest, the gamepad fallback that
  aimed along the move is skipped; the character faces the camera forward and strafes.
* **The projectile's "back"** (`kBackShortcutStep`, checked by `inputSequence::stickMatchesStep`
  against the aim) becomes a move opposite to the camera forward: the move stick pulled down.
* **The wire** is unchanged: the same unit `aimDirection` in `SyncedPlayerInput`.
* **The visualization** (the aim arcs and the mesh facing) reads `buildAimDirection`, so it shows the
  camera forward.
* **Under the shared iso camera** (two or more local players) the camera manager's view is the iso
  camera, so the aim is the iso camera's forward, the top of the screen, and the look stick does not aim
  while BlockLook is held. The keyboard-and-mouse path already behaved this way: BlockLook clears the
  mouse aim, and the last fallback of `buildAimDirection` is the flattened camera forward.
* **Threads.** `buildAimDirection` also runs on the physics thread (the sim input), where it now reads
  `m_blockLook`, which the input handler writes on the game thread. That is the same unsynchronized
  member read the sticks and the attack buttons already have.

`StickRoutingTest.cpp` in og-brawler-tests pins the function: the four cases tagged [CameraLookAim].

## 10. The gamepad latch

**The bug (user PIE, og-attackstatetransition-cleanup task 13).** After using a pad, keyboard and mouse
misbehaved: WASD set both the movement and the aim, WASD movement was not relative to where the mouse
pointed, and the aim followed the mouse only while the brawler stood still.

**The cause, confirmed in the code.** The latch gates the gamepad fallback (the move stick feeds the
aim) in `UOGBrawlerInputCollectionComponent::buildAimDirection` and the matching camera-relative branch
of `buildMoveDirectionWorld`. Before task 13, `onMoveStick` and `onAim` set it on any non-zero value.
Three engine facts make that value non-zero every frame while a pad rests with slight drift:

* the XInput device sends a stick value whenever it changes, not only outside its own deadzone
  (XInputInterface.cpp, the analog send around line 294), so the drift reaches the key state raw;
* the two 2D stick keys get no deadzone in Enhanced Input (§1.1 of the initiative's tpcamera research),
  and an action with no triggers is Triggered on any non-zero value (EnhancedPlayerInput.cpp:518);
* the bindings fire in the order they were bound (EnhancedPlayerInput.cpp:571), and `setupBindings`
  binds Move before MoveStick and Aim.

So every frame `onMove` cleared the latch for WASD, and the drift set it again straight after. While
moving, the fallback aimed along the move and, in AimRelative, the move was camera-relative instead of
relative to the aim. While standing still the move stick was below its deadzone, the fallback did not
apply, and the mouse aimed. That is exactly the reported symptom.

**The rule.** Three pure functions in `InputMapping/StickRouting.h` decide every write; the handlers
only call them. None reads a global.

* `lastInputWasGamepadAfterStick`: a stick sets the latch only past its deadzone, compared with `>`
  like the fallback's own gate. The caller passes `g_moveStickDeadzone` for the left stick and
  `g_aimStickDeadzone` for the right stick, per physical stick. A stick at or below its deadzone leaves
  the latch alone; it never clears it.
* `lastInputWasGamepadAfterMoveKeys`: a non-zero Move event (`moveKeysPressed`, the same 1e-4 per-axis
  tolerance as the engine's near-zero test the handler used before) sets the latch to "a D-pad
  direction is held". So WASD clears it and the D-pad sets it, as before. A release leaves it.
* `lastInputWasGamepadAfterCursor`: mouse movement clears it (below).

**The mouse signal: the cursor position, not the Look action.** The task offered both. The Look delta
reaches the game only while the viewport holds mouse capture (SceneViewport.cpp, the delta
accumulation around line 768). With the cursor shown, which is exactly when the mouse aims, the viewport
releases capture on every mouse-up (SceneViewport.cpp, around lines 669-714), so after the first click
the Look delta stops. The cursor position is updated on every mouse move whether or not the viewport has
capture, and it is the very value the mouse aim deprojects. The aim direction itself would be a bad
signal: it changes when the brawler or the camera moves under a still cursor.

**The threshold and the anchor.** `kCursorLatchMinPixels` is 8 viewport pixels. It is measured from an
anchor, not per frame, so a slow deliberate move still counts at any frame rate (1 pixel per frame
crosses it on the 8th frame), and a jitter of a few pixels never does. While the latch says keyboard and
mouse, the anchor follows the cursor; while the latch says gamepad, it stays put. The cursor must
therefore travel 8 pixels from where it was when the gamepad took over. The first sample only sets the
anchor. 8 pixels is far below any deliberate re-aim and above a few pixels of jitter; it is a
judgement, not a measurement.

**Who samples the cursor.** `UOGBrawlerInputCollectionComponent::updateGameThreadCache`, once per
character `Tick`, on the game thread like the handlers, and only:

* for the primary local player. Keyboard and mouse input go to that player only (the viewport sends a
  key event to one local player, chosen by input device), so a couch player on a pad is never cleared
  by another player's mouse;
* while look mode is off. In look mode the cursor is hidden and the mouse does not aim.

**Residual edges.** A frame that has both a stick past its deadzone and a mouse move ends with the
mouse, because the pawn's `Tick` runs after the controller's input. That is one frame, and only when
both devices are really in use. A cursor jump of 8 pixels or more with no hand on the mouse (a resize
or a cursor warp) clears the latch once; the next stick push past its deadzone sets it again.

**What it does not change.** The fallback and its two gates are unchanged, and so is the routing. A pad
whose drift is above the deadzone still steals the aim, as it would without this latch. The wire is
unchanged: the latch is UE-side only, and the simulation sees the resulting directions.

`StickRoutingTest.cpp` in og-brawler-tests pins the three functions: the cases tagged [GamepadLatch],
including the bug's frame order (WASD, then a drifting left stick, then a drifting right stick, every
frame) and a mouse move beside a drifting pad.

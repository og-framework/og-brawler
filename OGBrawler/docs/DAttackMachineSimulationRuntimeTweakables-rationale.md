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
when there is one, which is the "falls back to camera-relative" half. Every reader and every
writer of `g_movementScheme` is in the UE layer; the simulation sees only the resulting
directions.

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

It is implemented as a straight pointer swap in
`UOGBrawlerInputCollectionComponent::getMoveStick` and `::getAimStick`, with no sign flips.

**`g_gamepadMoveStickFeedsAim`:**

> Gamepad-only feature toggle: when true (default), if the aim stick is below its
> deadzone and the most recent move input came from the gamepad left stick, the move
> stick feeds BOTH the movement direction and the aim direction (both resolve to the
> same camera-relative move-stick vector). When false, the gamepad case falls through
> to mouse aim / camera forward like any other input source. Keyboard+mouse play is
> unaffected either way (the latch m_lastMoveInputWasGamepad gates this).

The latch is a member of `UOGBrawlerInputCollectionComponent`, false until a gamepad-driven
move, which is what makes the "keyboard+mouse is unaffected" half true rather than merely
intended.

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

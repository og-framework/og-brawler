<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: UInputModifierNegate -- the engine's own Enhanced Input negate modifier; owned by the host engine, not by this repository, and must not resolve here -->
# `DAttackCamera.h` / `DAttackCamera.cpp` — guards

Prohibitions with a tagged site in `DAttackCamera.cpp` or `DAttackCamera.h`. Ids are opaque, stable, and retired rather
than reused.

The header's range rules are held by the compiler and need no entry: the hard pitch limits stay inside
(0°, 90°), the default limits sit inside the hard ones, the stick deadzone is below 1, the pull's
e-fold (and the minimum a host value is raised to) and cap are positive, and the default hold-off and
dominance ratio are not negative, the default fields of view (at the target pitch and at the horizon) are
angles inside (0°, 180°), and the
field-of-view axes are the console values 0 and 1, the default look-mode-off return rate is positive, the
followed ground's cut-off (30° + 0.1°) stays below 90°, the default slope pitch limits sit inside the hard ones, in
order, the look-mode-off turn up a slope starts inside the followed range, its threshold's tolerance is positive and
below the cut-off's, and its default rate is positive, its default dwell is positive, the dwell's heading tolerance lies
inside (0°, 180°), and the dwell's time tolerance is positive and below 1 ms, the turn's default ease and acceleration
are positive, and its arrival snap is positive and below one frame's step of the default rate at 1000 fps, and an
input that does not set the ground has no supported ground, so it never turns to the iso yaw (task 21f)
(`static_assert`s in `DAttackCamera.h`;
rationale §6, §13, §14,
§18, §19 and §20). The Catch2 suite in og-brawler-tests (`DAttackOrbitCameraTest.cpp`,
`DAttackCameraIsoSeedTest.cpp`, `DAttackCameraFramingTest.cpp`, `DAttackCameraFollowTest.cpp`,
`DAttackCameraMatchSoloTest.cpp`, `DAttackCameraSlopeFrameTest.cpp`, `DAttackCameraSlopeYawTest.cpp`) pins the behaviour.

---

## G-01 — the mouse term is a signed displacement: no deadzone, no normalization, and its sign pairs with the Look mapping

**Site:** the `return` in `mouseLook`, the one statement that turns the Look action's value into degrees.

**The prohibition.** ⛔ Do not put a threshold or deadzone in front of the mouse value, and do not
normalize it. Every unit the host reports must turn the camera by `mouseDegreesPerUnit × mouseSens`
degrees, however small. ⛔ Do not drop or flip the minus sign on `mouseUnits.x`, and do not flip the
sign on `mouseUnits.y`, without changing the Look action's `Mouse_XY` binding in
`InputMapping/GameInputMapping.cpp` in the same change. That binding carries `KeyModifier::Negate`,
which the Unreal host turns into a `UInputModifierNegate` that negates BOTH axes, so the value that
arrives here is (−right, −up) in mouse units. The minus on x makes mouse-right turn the view right; the
plain y makes mouse-up look up (the pitch is measured below the horizon, so looking up lowers it).

**The consequence of getting it wrong.** A threshold or a normalization is exactly the defect this
file replaced. The old camera dropped any mouse step under 0.1 units (about 1.4 mouse counts per frame,
because the host had already scaled the counts by its 0.07 mouse sensitivity) and normalized the
rest, so a flick and a slow drag of the same length turned the camera by different angles and slow,
precise pans did nothing. A sign change turns mouse look into "drag the world": mouse-right turns the
view left, or mouse-up looks down.

**What breaks if the tag moves.** The tag sits on the `return` because the sign, a normalization and
a threshold are all typed on that statement. Deleting the function takes the tag with it and orphans
this entry (CHECK 2). `DAttackOrbitCamera.MouseIsADisplacement` (one 0.07 unit turns 0.175°, and twice the
units turn twice the angle) and `DAttackOrbitCamera.SignPins` (which reads the binding's modifier from
`buildDefaultContext`) fail on either edit.

---

## G-03 — on a slope the clamp's bounds include the player's pitch: never clamp to the slope limits alone

**Site:** the `const float pitchDeg = ...` statement in `worldView` (`DAttackCamera.h`), the one statement that clamps
the world pitch on a slope.

New guard (og-ground-aligned-attacks task 21b, 2026-11-03). It replaced no comment.

**The prohibition.** ⛔ Do not reduce the bounds `std::min(playerPitchDeg, limits.minDeg)` and
`std::max(playerPitchDeg, limits.maxDeg)` to `limits.minDeg` and `limits.maxDeg`, and do not drop either one. A pitch the
player set outside the slope limits (og.cam.slopePitchMin / og.cam.slopePitchMax, 35° and 75° by default, inside the
player's own 10° to 80°) must come out of the slope branch unchanged while the ground's rise along the view is zero. So
the slope moves the pitch only by the rise, and the clamp only stops that move, never makes one.

**The consequence of getting it wrong.** `worldView` returns the state verbatim while the frame up is flat, and its slope
branch takes over on the first frame the frame up leaves world up (`isCameraUpFlat`, 1e-6; the first look-mode pull on a
followed slope, rationale §18). With the plain clamp a camera at pitch 20° jumps to 35° on that frame at every heading,
one at 80° jumps to 75°, and one at 10° to 35°. No error and no log: a camera that pops when the ground under it starts
to count.

**What breaks if the tag moves.** `DAttackCamera.SlopeFramePitchDoesNotPopWhenTheFrameLeavesWorldUp` (the frame eased
from world up to the 30° face in 100 steps at pitch 20° and 80°, no step above 1°) fails with a 15° step, the
just-tilted sweep in `DAttackCamera.SlopeFrameWorldUpIsTodaysView` with a 25° gap, and
`DAttackCamera.SlopeFramePitchLimitsAreOrdered` with 40° for 60° (task 21b's second poison).

---

## §R — Retired ids

### G-02 — RETIRED by og-ground-aligned-attacks task 21b (2026-11-03)

**Was:** "a vertical view takes its yaw from the tilted heading, never from the view's own rounding noise", on the
statement in `worldView` that picked the world yaw (tag site: immediately above it). Task 21's `worldView` tilted the
local view into the frame and read the yaw from the tilted view, falling back to the tilted heading's yaw when the
view's horizontal length was within 1e-6 of vertical. The prohibition: never reduce that statement to the plain
two-argument arctangent of the tilted view, and never lower the threshold to 0. On the exact 30° face at the 60° target
the down-slope view was straight down, and the yaw of its rounding noise would have spun the camera, and the
camera-relative move basis with it.

**Why retired.** Its subject is gone. Task 21b (the user's G7 ruling, "Full tilt, clamped") made the world yaw the
player's yaw, `state.yawDeg`, at every heading. `worldView` no longer tilts the view, so there is no tilted view, no yaw
to pick, and no straight-down view (the world pitch stays inside [1°, 89°]). The tilt helper and the threshold
constant were deleted with the guard; nothing else called them. ⛔ The number 2 is spent.

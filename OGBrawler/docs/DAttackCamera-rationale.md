<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: UInputModifierNegate -- the engine's own Enhanced Input negate modifier; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: AddControllerYawInput -- the engine's own control-rotation input; owned by the host engine, not by this repository, and must not resolve here -->
# The solo orbit camera — rationale

Companion to `OGBrawler/DAttackCamera.h` and `OGBrawler/DAttackCamera.cpp`.
**The source carries the code; this file carries the reasoning.** Its prohibitions are in
`DAttackCamera-guards.md`.

---

## 1. What this is

The look-mode third-person camera a single local player gets (the shared isometric camera takes over
when two or more local brawlers play). It is pure: `dAttackCameraBehaviour::integrate` takes the frame's
look input, the settings and the previous state, and returns the next state. The host owns no camera
logic. It feeds the input, writes the boom rotation and length from the state, and seeds the state.

It replaced a PID camera in og-attackstatetransition-cleanup task 8. The old camera:

* turned at a fixed speed whenever the mouse moved at all, because it normalized the mouse delta, so a
  flick and a slow drag of the same length gave different angles;
* dropped mouse steps under 0.1 units, about 1.4 mouse counts per frame;
* fed a pitch PID whose gains were applied per frame, with a derivative term divided by dt. Its closed
  loop is unstable above about 98.5 frames per second. In the engine-free sweep the task recorded, a
  camera seeded at the iso pitch swung between −89.75° and +89.9° at 144 fps;
* read its pitch back through a quaternion → Euler round trip every frame, and had no pitch clamp.

Nothing here is simulation state. The camera never reaches the wire; it is local presentation.

## 2. State, input and output

* **State** (`OrbitCameraState`): `yawDeg` is the yaw in the host's yaw convention (the Unreal host
  uses it as the rotator yaw, so a larger yaw looks further right). `pitchDeg` is degrees **below** the
  horizon, so the iso camera's Unreal pitch of −60° is 60 here. `holdoffSeconds` is the vertical hold-off
  timer (§4 rule 4). It stays 0 unless the host switches the hold-off on (`verticalHoldoffSeconds` > 0, §13). Three explicit scalars: there is no quaternion and no Euler decomposition, so the
  gimbal flip at ±90° cannot happen and the sign of the pitch is a plain negation. `frameUp` (og-ground-aligned-attacks
  task 21) is the up of the ground the camera follows, a unit vector in the host's Z-up camera space (`kCameraUp`,
  world up, by default). While it is world up, the yaw is the world yaw and the pitch is below the world horizon, as
  before task 21. On a followed slope the yaw is still the world yaw, and the pitch is measured below the ground's line
  along the view (§19, task 21b; task 21 orbited the camera about `frameUp`, §18). `slopeDwellSeconds` and
  `slopeDwellYawDeg` (og-ground-aligned-attacks task 21d) are how long, with look mode off, the up-slope heading has been
  steady and the heading that dwell started on (§20). `slopeYawRateDegPerSec` (task 21e) is the speed of that turn up
  the slope, 0 or above, and 0 whenever the turn is not running (§20). They are display state only, like the rest.
* **Input** (`OrbitCameraInput`): `lookHeld` (look mode is on; the Unreal host passes its BlockLook state, a toggle on 5 / gamepad Y since
  og-attackstatetransition-cleanup task 12, a held button before), `mouseUnits` (the Look action's
  value summed over the frame, §3), `stick` (the look stick, x right and y up, as the host routes it) and
  `targetPitchDeg` (the pitch the pull aims for; the Unreal host passes the iso pitch) and `targetUp` (the frame up
  the pull aims for, §18; the Unreal host passes `cameraTargetUp` of the ground under its character; with look mode
  off it also decides whether the camera turns to face up a slope, §20).
* **Output:** the state. The host writes the boom as `FRotator(-view.pitchDeg, view.yawDeg, 0)` with
  `view = worldView(state, settings)`, which is `{yawDeg, pitchDeg}` unchanged while `frameUp` is world up and on a
  slope keeps the yaw and moves the pitch by the ground's rise along the view, clamped (§19), its length as
  `boomLength(pitchDeg, targetPitchDeg, settings)` and the camera's field of view as
  `horizontalFovDeg(fovAtPitch(pitchDeg, targetPitchDeg, settings), settings.fovAxis, aspect)` (§14, §17).
* **Seeding:** `seedFromUERotation(uePitchDegrees, ueYawDegrees, settings)` gives yaw = the Unreal yaw (wrapped) and
  pitch = −the Unreal pitch, clamped into the limits, with no hold-off. `pitchDegFromUEPitchDegrees`
  is the same conversion for the target. Both clamp, so the state's pitch is always inside the limits.
  The seed's `frameUp` is world up, so a seed (and the host's re-seed when it returns from the iso camera) starts
  level. A default-constructed state is (0, 0, 0, world up, 0, 0, 0) and is meant to be overwritten by a seed; a seed's
  dwell and turn speed are 0, so a re-seeded camera waits for a steady slope again and its turn starts from rest.

## 3. Signs

* **Mouse.** The host does not pass raw counts. The Look action is bound to `Mouse_XY` with
  `KeyModifier::Negate` (`InputMapping/GameInputMapping.cpp`), and the Unreal host turns that into a
  `UInputModifierNegate`, which by default negates every axis. The host also scales mouse keys by its
  0.07 mouse sensitivity before any modifier runs. So `mouseUnits = −0.07 × (counts right, counts up)`.
  `mouseLook` therefore turns yaw by `−mouseUnits.x` and pitch-below-horizon by `+mouseUnits.y`: mouse
  right looks right, mouse up looks up. `invertMouseY` flips the pitch only. Guard G-01.
* **Stick.** x right, y up: stick right looks right, stick up looks up. `invertStickY` flips the pitch
  only.
* **Changed from the old camera: only mouse X.** The old camera fed both through the same law (`+x` =
  yaw right, `+y` = pitch down). With the negated mouse, mouse right gave `−x` and turned the view
  LEFT; mouse up gave `−y` and looked up. The stick reached that law as the Unreal host's
  `getAimStick()`, which stores stick up as −Y, so stick up also looked up. The new camera follows the
  traditional third-person convention (the Unreal template: mouse right turns right, nothing inverted
  by default), so mouse X is reversed and stick Y and mouse Y are unchanged. The stick Y result does
  not depend on which sign the engine reports for up: the old and new cameras read the same stored
  value, and both raise the pitch-below-horizon by `+stored y`. `invertStickY` does not
  restore anything old: it gives an inverted stick. The Unreal host's `og.cam.invertMouseX` restores
  the old mouse X.

## 4. The rules, in the order `integrate` applies them

1. **No look input unless `lookHeld`.** With look mode off the mouse and the stick are ignored and the yaw does not
   change, except on a followed slope of 15° or more that has held steady for the dwell, where it turns, eased, to
   face up the slope as it was when the dwell started (§20, tasks 21c, 21d and 21e). Since og-ground-aligned-attacks
   task 21 the camera then returns, over time, to world up and the target pitch (§18); before it, nothing moved at all
   (`returnDegPerSec` 0 still gives that camera). The hold-off timer still counts down while look mode is off (it
   measures time since the last deliberate vertical look, not time spent looking), so a pause longer than the hold-off
   never delays the next pull.
2. **Player look.** Mouse: `Δ = units × mouseDegreesPerUnit × mouseSens`, no dt, no deadzone, no
   normalization. `mouseLook` takes no frame time on purpose: a mouse report is already a distance.
   Stick: radial deadzone, rescaled response curve, then `× rate × stickSens × dt` (§5). If both are
   present they are summed (the old camera used the mouse and ignored the stick on frames where both
   moved).
3. **Apply the player's yaw (wrapped to [−180°, 180°]) and pitch, then clamp the pitch** to the limits (§6).
   That is the frame's result when nothing is pulled. On a frame with a pull, the pitch is solved again
   from the frame's start together with the pull (§7.1) and clamped once at the end.
4. **Vertical hold-off (OFF by default, §13).** Only when `verticalHoldoffSeconds` is above 0: if the
   player's pitch step is faster than `verticalIntentDegPerSec`, the hold-off restarts at
   `verticalHoldoffSeconds` and nothing is pulled this frame. A speed, not "any vertical input", so a mouse
   swipe's hand tremor does not reset it. At 0 (the default) the whole rule is skipped: there is no pause
   after vertical look and none during it either, so the pull also acts while the player tilts. A tiny
   positive value does pause during vertical look, so 0 is a switch, not the end of a continuous dial.
5. **Horizontal dominance (OFF by default, §13).** `weight = clamp(1 − dominanceRatio × |Δpitch| / |Δyaw|, 0, 1)`.
   At the default ratio of 0 the weight is always 1. At a ratio of 2 it reaches 0 at a slope of 1:2, about
   27° off horizontal. The weight scales the pull's exponent (rule 6), so a weight of 0 means no pull and
   a weight of 1 the full pull.
6. **The pull, per degree of yaw.** With no player pitch step, `fraction = 1 − exp(−weight × |Δyaw| / pullYawDegreesPerEFold)`
   and the step is `(target − pitch) × fraction`, capped at `pullMaxDegPerSec × the pull window` (§7).
   Because the fraction depends on how far the camera turned, not on time, a slow pan pulls slowly, a 45°
   turn at the default 20° e-fold closes 1 − e^−2.25 ≈ 89 % of the error, and successive frames compose
   exactly: the product of the per-frame `exp` terms is `exp(−total yaw / e-fold)` at any frame rate. The
   step is always a fraction of the error, so it cannot overshoot; there is no integrator, so nothing winds
   up. When the player also tilts in the same frame, the player's step and the pull are solved together
   (§7.1), so the camera settles where the player's tilt and the pull balance, at any frame rate.
7. **No pull without look input.** The pull scales with the yaw step, so with no yaw there is no pull.
   There is no idle spring-back in look mode: the camera never moves on its own while look mode is on (a user decision,
   2026-10-04). The pull also moves `frameUp` toward `targetUp`, by the same law (§18). Turning look mode off is the one
   time the camera moves without look input: it returns to world up and the target pitch (rule 1, §18), and on a
   followed slope of 15° or more, once it has held steady for the dwell, it turns, eased, to face up the slope (§20).

The target is clamped into the limits, so the pull can never push the pitch out of them.

## 5. The stick curve

`stickCurve(magnitude)`: the magnitude is capped at 1, anything at or below the deadzone gives 0, and
above it the response is `((m − deadzone) / (1 − deadzone)) ^ exponent`. The rescale makes the response
start at 0 at the deadzone edge, so there is no jump when the stick leaves it. The deadzone is RADIAL (on
the stick's length), so a small diagonal that is outside the deadzone moves the camera even though each
axis on its own is inside it. The direction is the stick's own direction, so a diagonal stick turns
diagonally.

## 6. Pitch limits

* The defaults are 10° to 80° below the horizon. Above 0° keeps the boom-length curve and the horizon
  sane; below 90° keeps a camera-relative move basis (the camera's forward flattened to the ground) away
  from the zero vector it would be when looking straight down.
* `pitchLimits` sanitizes host-provided limits: the minimum is clamped into the hard limits [1°, 89°], and
  the maximum into [minimum, 89°]. So a console variable typo (a minimum above the maximum, or a value
  outside 0–90) can never reach `std::clamp` with its bounds reversed, and `boomLength` never divides by
  zero. The hard limits are the task-7 target clamp, kept, and a `static_assert` holds them inside
  (0°, 90°).

## 7. The pull window: why the hold-off can expire part-way through a frame ∴D-01

This section only matters when the host switches the hold-off on (`verticalHoldoffSeconds` > 0; it is 0
by default since task 10, §13). With the hold-off off, the timer stays 0 and `pullSeconds` is always the
whole frame.

`pullSeconds = max(0, deltaSeconds − the hold-off left at the start of the frame)`. The hold-off counts
down by whole frames, but it does not end on a frame boundary: a 0.6 s hold-off is 18 frames at 30 fps but
86.4 frames at 144 fps. If the pull only started on the first frame that begins with the hold-off at 0, the
144 fps camera would pull a fraction of a frame later than the 30 fps one, and with the pull running at
its cap that is a difference of up to one frame's cap (8° at 30 fps with the 240°/s default). So the frame
on which the hold-off runs out pulls for the part of the frame after it ran out: its yaw step is scaled by
`pullSeconds / deltaSeconds` (the yaw is assumed uniform across the frame), the player's pitch step is
split the same way (§7.1), and its cap is `pullMaxDegPerSec × pullSeconds`. Every other frame has
`pullSeconds` equal to the whole frame or to 0. With this, a "tilt, then pan" path
(`DAttackOrbitCamera.SamePathSameAnglesAtEveryFrameRate`, run with the hold-off on) ends at the same pitch
at every frame rate from 30 to 240 fps: within 0.00001° in a double-precision model of this function
(task 10). With a whole-frame expiry instead, the same path at 100 fps ended 0.19° away from the 60 fps run
(task 8's poison arm, with that task's 90° e-fold and 60°/s cap).

### 7.1 The player's step and the pull are solved together ∴D-02

Inside the pull window the frame is treated as continuous, with the player's pitch rate `v` and yaw rate
uniform across it. The pull then obeys `dp/dt = v + k × (target − p)`, with `k = weight × |yaw rate| / e-fold`.
Over the window, with `a = k × pullSeconds` (the `exponent`: weight × the window's share of the yaw step ÷
e-fold), `fraction = 1 − e^−a` and the player's window step `s = v × pullSeconds` (`playerInWindow`), the
exact solution is

`p_end = p_start + (target − p_start) × fraction + s × fraction / a`

`fraction / a` (`playerKeptShare`, 1 when a is 0) is the share of the player's own step that survives the
pull acting during it. The code writes the pull as the difference from the plain player step,
`(target − p_start) × fraction + s × (playerKeptShare − 1)`, so the cap limits only the pull and never the
player's own step. The start pitch (`beforeWindow`) is the pitch after the part of the frame before the window
(non-zero only on the frame a hold-off runs out), which carries its share of the player's step with no pull.
The result is clamped into the limits once, at the end. While the rates stay constant, the clamped exact
solution is also the solution of the clamped motion, because the pitch moves monotonically toward
`target + v / k`.

**Why not "apply the player's step, then pull on the result".** That ordering, the plain reading of the
task-10 brief, lets the pull act on the whole player step every frame, so where the player tilts and the
pull balance depends on the frame step: the balance point is off by about `v × dt / 2`. On the existing
sweep path "stick 30° off horizontal for 1 s" it ended up to 0.41° from the 60 fps run, four times the
0.1° budget, and a 45° diagonal stick up to 0.59° (double-precision model). Task 8 had only a small O(dt)
residue from the same ordering, because its hold-off and dominance weight switched the pull off during
most vertical input; with both off by default, the pull meets the player's step on every diagonal. With the
joint solution the steady state of a 45° full-stick diagonal is `60 − 20 × 110 / 180 ≈ 47.78°` at every
frame rate, and `DAttackOrbitCamera.PullFollowsTheClosedFormBelowTheCap` holds three paths to the closed
form `p(t) = rest + (p0 − rest) × e^−kt` within 0.01° at every frame step. The sequential ordering, as a
poison arm, failed that case and the frame-rate sweep.

## 8. The boom length

`boomLength` keeps the old camera's curve: full length at or above the target pitch, falling linearly to
the horizon length at the horizon. Both lengths are settings (`boomLengthAtTargetPitch`,
`boomLengthAtHorizon`), by default `kBoomLengthAtTargetPitch` = 2500 and `kBoomLengthAtHorizon` = 550
(og-ground-aligned-attacks task 18, 2026-10-30: 2500 is the value the user tuned in play, the horizon
length is still the user's to decide; task 15 had set 1200 and 550, scaling the 900 and 400 before by
about 1.33). With the default limits and the 60° iso target the shortest boom is
about 875 at the 10° minimum. The blend factor, `towardTargetPitch` (the pitch clamped into [0, target]
over the target, with the target clamped into the pitch limits), is shared with the field of view (§17).
A host that sets the boom length itself when it seeds the camera (before the
first `integrate`) must use the full length of the settings it seeds with (`kBoomLengthAtTargetPitch` for
the defaults), because a seed at the target pitch is a boom of that length. With the old 900 and 400 as
settings, `boomLength` gives the old function's bits (`DAttackCameraFramingTest.cpp`).

## 9. Defaults

| setting | value | why |
|---|---|---|
| `kMouseDegreesPerUnit` | 2.5 | with the host's 0.07 mouse sensitivity, 0.175° per count: the Unreal template's mouse feel (0.07 × its legacy 2.5 yaw scale). 140° per inch at 800 dpi |
| `kStickDeadzone` | 0.15 | the same value as the move-stick deadzone |
| `kStickExponent` | 1.5 | fine control at small tilt |
| `kStickYawDegPerSec` / `kStickPitchDegPerSec` | 180 / 110 | a 180° turn in 1 s; about twice the old fixed speeds |
| `kPitchMinDeg` / `kPitchMaxDeg` | 10 / 80 | §6 |
| `kPullYawDegreesPerEFold` | 20 | strong (task 10, lead default, was 90): a 45° turn closes about 89 % of the error, a 90° turn about 99 % |
| `kMinPullYawDegreesPerEFold` | 1 | a host e-fold of 0 or below is raised to this, so the pull never divides by zero or reverses (§13) |
| `kPullMaxDegPerSec` | 240 | task 10 lead default (was 60): a full-stick turn (180°/s) is capped only for the first few degrees of a 30° gap; a 90° mouse flick in 0.1 s moves the pitch at most 24° |
| `kDominanceRatio` | 0 | off (user ruling, task 10; was 2, which stopped the pull past about 27° off horizontal) |
| `kVerticalIntentDegPerSec` | 20 | above a mouse swipe's tremor, below a deliberate tilt; only read while the hold-off is on |
| `kVerticalHoldoffSeconds` | 0 | off (user ruling, task 10; was 0.6 s, which kept the pull from undoing a pitch the player just chose) |
| `invertMouseY` / `invertStickY` | off / off | |
| `mouseSens` / `stickSens` | 1 / 1 | multipliers for the host's settings |
| `kBoomLengthAtTargetPitch` / `kBoomLengthAtHorizon` | 2500 / 550 | §8 (task 18, the user's tuned boom; task 15 had 1200 / 550, before it 900 / 400) |
| `kFovDeg` / `kFovAxis` | 40 / shorter axis | §17 (task 18, the user's tuned value, at the target pitch); task 15 had 58.72, the old 90° horizontal on 16:9 (§14) |
| `kFovDegAtHorizon` | 58 | §17 (task 18, the user's request): the field of view at the horizon |
| `kReturnDegPerSec` | 90 | §18 (task 21): with look mode off, the camera returns to world up and the target pitch at this angular rate |
| `kCameraFollowMaxSlopeDeg` / `kCameraFollowSlopeEpsilonDeg` | 30 / 0.1 | §18 (task 21, the user's ruling): the camera follows ground up to 30° + 0.1° from world up |
| `kSlopePitchMinDeg` / `kSlopePitchMaxDeg` | 35 / 75 | §19 (task 21b, the user's G7 ruling "Full tilt, clamped"): on a followed slope the ground's rise along the view moves the world pitch no further than these |
| `kSlopeYawMinSlopeDeg` / `kSlopeYawSlopeEpsilonDeg` / `kSlopeYawDegPerSec` | 15 / 0.001 / 90 | §20 (task 21c, the user's request and rulings): with look mode off, a followed slope of 15° or more turns the camera to face up it, at this rate |
| `kSlopeYawDwellSeconds` / `kSlopeYawDwellToleranceDeg` / `kSlopeYawDwellEpsilonSeconds` | 0.2 / 10 / 1e-4 | §20 (task 21d, the user's G7 twitch report): the turn starts only after the up-slope heading has held within 10° for 0.2 s; the 1e-4 s absorbs the float sum's error |
| `kSlopeYawEaseSeconds` / `kSlopeYawAccelDegPerSec2` / `kSlopeYawArriveDeg` | 0.3 / 360 / 0.01 | §20 (task 21e, the user's G7 re-check): the turn speeds up from rest at 360°/s² to at most `kSlopeYawDegPerSec`, slows over the last of the gap with a 0.3 s time constant, and takes the goal exactly within 0.01° |

The look values are the camera research of og-attackstatetransition-cleanup (2026-10-04). The pull values
are task 10's (2026-10-05, §13). The Unreal host exposes the four pull values as console variables, so they
can be tuned live.

## 10. Why the camera does not use the engine's control rotation

The traditional engine route (`AddControllerYawInput` and friends, a spring arm that follows the control
rotation) was not taken. In the Unreal host the legacy input scales are on, which would multiply yaw by
2.5 and pitch by −2.5 on that path. The engine also applies the control rotation in the controller's
tick, before the pawn's, so the pull and the clamp would have to live in a camera modifier: engine
code with no engine-free tests, and the rules split over two places. What that route would give (a pitch
clamp) is a few lines here.

## 11. Portability: every absolute value is qualified

All float absolute values are `glm::abs` (or `std::` functions on floats), never an unqualified `abs`. On
toolchains where the only `abs` in scope is C's `abs(int)`, an unqualified call would truncate a float
to an integer: the per-frame yaw and pitch steps are often below 1°, so the yaw step would read 0 (no pull at
all), and the hold-off test and the dominance weight would misfire. The behaviour tests drive leftward yaw (negative steps) and sub-degree
steps at 144 and 240 fps. Task 8 truncated the hold-off test's absolute value to an integer as a poison
arm: the hold-off stopped starting at high frame rates and two test cases failed.

## 12. What was removed with the old camera

The DAttackCameraInput and DAttackCameraState classes (the state also held an unused camera transform), the PID use (the PID itself
stays in og-simulation), the quaternion round trip, the radian targetPitchFromUEPitchDegrees (replaced
by `pitchDegFromUEPitchDegrees`, in degrees), an unused DAttackCameraBehaviour class with a placeholder
constructor, and the file-wide optimization-off pragmas.

## 13. Task 10: a strong pull that is never switched off by default (2026-10-05)

**The user decisions (from PIE).** The pull toward the target pitch must be STRONG during horizontal
camera movement, and must never be disabled: task 8's vertical hold-off (0.6 s) and its horizontal-dominance
weight (ratio 2, no pull past about 27° off horizontal) switched it off on most diagonals and for 0.6 s
after any deliberate tilt. The pull stays tied to horizontal movement (a fraction per degree of yaw turned)
and there is still no pull without look input. A later ruling the same day kept both mechanisms in the code,
with defaults that switch them off (hold-off 0 s, ratio 0), so the user can try them again; the Unreal host
exposes them as console variables (`og.cam.pullHoldoffSec`, `og.cam.pullDominanceRatio`).

**The strength** (lead defaults, tunable live as `og.cam.pullEFoldDeg` and `og.cam.pullMaxDegPerSec`): an
e-fold of 20° of yaw (was 90°) and a cap of 240°/s (was 60°/s). `DAttackOrbitCamera.AFortyFiveDegreeTurnClosesMostOfTheGap`
pins the strength: a 45° turn from 30° toward 60° closes at least 85 % of the gap (89.5 % measured; task 8's
values closed 39 %).

**What "off" means for each mechanism.**

* Hold-off 0: rule 4 is skipped entirely, so the pull acts during vertical look too, not only after it.
  `holdoffSeconds` then stays 0 and §7's window is always the whole frame.
* Dominance ratio 0: the weight is always 1.

**Host values are sanitized,** as the pitch limits are (§6): an e-fold of 0 or below is raised to
`kMinPullYawDegreesPerEFold` (1°), so the exponent never divides by zero and a negative e-fold can never
turn the pull away from the target; a negative cap counts as 0, which means no pull, and never reaches
`std::clamp` with its bounds reversed; a negative hold-off counts as off, and a negative ratio gives the
weight 1. Before task 10 nothing exposed these values to a host. `DAttackOrbitCamera.HostPullSettingsAreSanitized`
covers each case; against task 8's code, a negative e-fold (−5) drove the pitch below the 10° limit (8° at 30 fps), a
negative cap moved it 10° away from the target, and a negative hold-off was stored as the state's timer.

**The tests.** Task 8's hold-off and dominance case still runs, with both switched on (0.6 s, ratio 2):
`DAttackOrbitCamera.NoPullDuringVerticalInputOrTheHoldoffWhenSwitchedOn`. The frame-rate sweep runs every
path with the defaults and with both switched on. `DAttackOrbitCamera.PullActsDuringDiagonalsAndRightAfterVerticalInput`
pins the defaults (both 0) and the behaviour they give.

## 14. Fields of view and the iso framing distance (og-ground-aligned-attacks task 15, 2026-10-29)

**The user request.** The solo camera further back and the shared iso camera closer, with live console
variables to tune them, separate horizontal and vertical margins for the iso framing, separate field-of-view
settings for the two cameras, and framing that also looks right on screens that are not 16:9 (Android
portrait among them).

**The field-of-view axis** (`FovAxis`). The Unreal host renders both cameras' field of view as the
HORIZONTAL angle at every aspect: each camera sets its component's aspect-ratio axis override to
MaintainXFOV (the iso actor's constructor and the character's), and the iso camera does not constrain its
aspect ratio. Without the override the local player's default applies, which in this engine is
MaintainYFOV (Engine/Config/BaseEngine.ini, section /Script/Engine.LocalPlayer). The engine's projection
(CalculateProjectionMatrixGivenViewRectangle in Engine/Source/Runtime/Engine/Private/Camera/CameraStackTypes.cpp)
then keeps the VERTICAL angle that the field of view gives at the component's reference aspect of
1.777778, and the window's aspect only widens or narrows the horizontal one, so the conversion and the
framing below would hold only on 16:9 (off it the rendered extent is aspect / 1.777778 of the framed one:
0.75 on 4:3, 1.31 on 21:9, 0.32 on 9:16; `DAttackCameraFramingTest.cpp` models both constraints). Before
task 15 neither camera had the override, so the game rendered a fixed vertical angle at every aspect, 20°
and 90° horizontal only on 16:9. A fixed horizontal angle shows less and less height as the screen gets
wider, and a portrait screen gets a very narrow view. So a field of view can also be authored on the SHORTER screen axis
(`FovAxis::Shorter`, the default for both cameras): a wider screen then shows more to the sides at the same
height, and a portrait screen shows more above and below at the same width. `horizontalFovDeg` turns an
authored angle into the horizontal one the host renders, `2 × atan(tan(fov / 2) × aspect)` on a landscape
screen (aspect = width / height above 1). On a portrait or square screen, and for `FovAxis::Horizontal`,
the shorter axis is the horizontal one or the authored angle already is, and it returns its input unchanged.
The defaults (task 15's; task 18 retuned the solo camera's to 40°, §17) are re-expressed on the shorter axis so that 16:9 looks as before within 0.006°: 58.72° for
the solo camera (`kFovDeg`; 90.0053° horizontal on 16:9, the engine's default was 90°) and 11.33° for the
iso camera (the host's own default; 20.0025° horizontal on 16:9, it was 20°). The host converts every frame
with the viewport's current aspect, so a resized window re-converts at once. The aspect is the whole game
viewport's, so split-screen local players get the window's aspect, not their own view's.
`fovAxisFromConsoleValue` maps a console value to an axis: 0 and below horizontal, 1 and above the shorter
axis. Console value 0 now means a horizontal angle at EVERY aspect, which changes the framing off 16:9
against the build before task 15 (that one rendered a fixed vertical angle); 16:9 is unchanged.

**The iso framing distance** (`isoFramingDistance`). The camera sits on its optical axis through the
centre of the brawlers' box, at the distance where the box's on-screen half-extents, each padded by its
own margin (horizontal `marginH`, vertical `marginV`, world centimetres at the camera plane), fit the
rendered field of view, clamped into `[minDist, maxDist]`. With `t = tan(horizontal fov / 2)` the
horizontal axis needs `paddedH / t` and the vertical one `paddedV × aspect / t` (the vertical half-angle's
tangent is `t / aspect`), and the larger wins. It is the Unreal actor's former formula, moved here so it can
be tested: the same operations in the same order, the 0.001 floor on the tangent
(`kMinFramingTanHalfFov`, which keeps a near-zero field of view from dividing by zero) and the engine's
max and clamp written out as the comparisons the engine uses. With `FovAxis::Horizontal` and equal margins
it returns the former formula's bits (`DAttackCameraFramingTest.cpp`, 5250 inputs). For
`FovAxis::Shorter` it converts first and then applies the same formula, so the framing uses exactly the
angle the host renders.

**Defaults (the host's).** The iso margins are 400 cm horizontal and 300 cm vertical (one 800 cm margin
padded both before). Two brawlers 2 m apart side by side on 16:9 are framed from about 30 m instead of
about 81 m (the vertical margin binds in both). The solo
boom lengths are in §8.

## 15. The iso camera's follow step (og-ground-aligned-attacks task 16, 2026-10-29)

**The user report.** The shared iso camera moved "very jaggedly" when the two local brawlers separated
along the camera's right vector, and smoothly when they also moved forward; setting the start distance to
1 cm (`og.iso.posDeadzoneCm 1`) made it smooth. The Unreal actor moved the camera only on ticks where it
was more than one deadzone (100 cm) from its framing target, by the engine's VInterpTo with speed 6. At a
steady follow of a target moving at `v`, the error before each step settles at `v / speed` (the step closes
the fraction `k = dt × speed` of the error, and the target adds `v × dt` per tick, so the error before a
step is `v × dt / k`), whatever the frame rate. A sideways separation moves only the fitted distance
(§14), at about 5.7 m/s once task 15's margins let the horizontal term bind, so the error settled at about
95 cm, just inside the deadzone: the camera stopped, fell behind past 100 cm, moved, and stopped again.

**The hysteresis** (`followStep`). The camera starts following when its distance to the target is above
`startDistance` and keeps following until it is at or below `stopDistance`, then holds. The caller carries
the `tracking` flag from tick to tick (`FollowStep::tracking`). Each call compares the current error with
`startDistance` when not tracking and with the smaller of `stopDistance` and `startDistance` when tracking:
above it, it returns `interpTo`'s step and `tracking` true; otherwise it returns `current` unchanged and
`tracking` false. The error is the engine's FVector::Dist in double precision: the squared differences
`target - current`, summed x, y, z in that order, then the square root. A stop distance at or above the
start distance therefore gives exactly the actor's former rule, one deadzone; `DAttackCameraFollowTest.cpp`
checks that bit for bit against the former actor statements (stop = start and stop = 5000) and checks that
this rule still holds the camera on some ticks of a 5.7 m/s target at 30, 60, 120 and 144 Hz, while the
10 cm stop never holds it after the first start.

With the 10 cm stop the camera holds only when the target is still or slower than `stopDistance × speed`
(60 cm/s with the defaults): then the error before a step settles below 10 cm. A target that keeps creeping
below that speed holds the camera until the error passes the start distance again, after which it is chased
back to within 10 cm (about 0.5 s from 200 cm at 60 Hz). That is a pause and a short chase, not the
tick-level stop-and-go above. A target that stays within the start distance of a held camera (jitter under
1 m with the defaults) never starts it.

**The step** (`interpTo`). The engine's VInterpTo (Engine/Source/Runtime/Core/Private/Math/UnrealMath.cpp),
transcribed so the step can be tested here: a speed at or below 0 returns the target; a squared distance
below `kInterpSnapDistanceSquared` (the engine's 1e-4 small number, in the caller's squared length unit)
returns the target; otherwise it returns `current + toTarget × fraction`, with `fraction = dt × speed`
clamped into [0, 1] in float and widened to double. The clamp is written out as the engine's max of a
min, the engine's comparisons in the engine's order. Positions are double because the host's are, and the
host's widening into `glm::dvec3` is exact. The step is a fixed fraction of the remaining error per tick,
not a continuous-time exponential, so its time constant depends on the frame rate, as it did before.
`DAttackCameraFollowTest.cpp` compares it with the engine function's bits over 540 inputs where the
engine is on the include path.

**Units and defaults (the host's).** The functions take any length unit; the Unreal host passes world
centimetres. The start distance (100 cm, `og.iso.posDeadzoneCm`), the stop distance (10 cm,
`og.iso.posStopCm`, new) and the speed (6, `og.iso.posLerp`) are the actor's properties, each overridden by
its console variable when above 0.

## 16. The iso camera matches the solo camera (og-ground-aligned-attacks task 17, 2026-10-30)

**The user request.** Tune only the solo camera (its boom and field of view) and have the shared iso
camera follow, so that a group of local players sees around its outermost players what one player sees
around themselves: "if a single character can see 10 m to the north, south, west and east ... a group ...
should be able to see 10 m in all directions from the outermost positions". The level puts the fight
platforms face-on to the camera and a mountainside that falls away along the camera's forward, which must
read with depth, so the iso camera takes the solo camera's perspective (the user's option A) and not a
narrow telephoto angle.

**What the solo camera sees** (`sightReach`, `soloSightReach`, `SightReach`). At the target pitch the solo
camera looks at its player from the full boom, `boomLengthAtTargetPitch`. On the plane through the player
that faces the camera, it sees `halfWidth = boom × tan(h / 2)` to each side and `halfHeight = boom ×
tan(v / 2)` above and below. `h` is the horizontal field of view the host renders (`horizontalFovDeg`, §14;
both cameras render the horizontal angle at every aspect) and `v` is the vertical one at the same aspect,
`verticalFovDeg(h, aspect) = 2 × atan(tan(h / 2) / aspect)`. With task 17's defaults (1200 cm, 58.72° on the
shorter axis) that is 9.00 × 6.75 m on 4:3, 12.00 × 6.75 m on 16:9, 15.75 × 6.75 m on 21:9 and 6.75 ×
12.00 m on 9:16 (half-width × half-height): the shorter axis always reaches 6.75 m and the longer one more.

**The matched margins** (`matchedIsoMargins`, `IsoMargins`). The iso framing (§14) pads the group's
on-screen half-extents by a horizontal and a vertical margin. Matching the solo camera sets the margins to
that reach, `marginH = halfWidth` and `marginV = halfHeight`, at the live aspect. Then:

* one player (half-extents 0): the horizontal term needs `marginH / tan(h / 2) = boom` and the vertical one
  `marginV × aspect / tan(h / 2)`, which is the boom too, since `tan(v / 2) × aspect = tan(h / 2)`. So the
  iso camera frames one player from exactly the solo boom, with the solo field of view: the same view.
* a group: the camera backs off until the binding axis shows the group's half-extent plus the margin, so the
  outermost players on that axis keep the solo reach beyond them, and the other axis keeps at least as
  much. `DAttackCameraMatchSoloTest.cpp` checks both at four aspects, booms from 800 to 2000 cm and 2500 cm, and four
  fields of view.

The vertical margin must come from the vertical field of view at the live aspect. Taken from the
horizontal one, the vertical term is `aspect` times too large on a landscape screen, and one player is
framed from `aspect × boom` (2133 cm instead of 1200 on 16:9). Taken at a fixed 16:9 aspect, only 16:9
is right. Both mistakes fail the test (its rationale §4 quotes them).

**The resolution** (`resolveIsoFraming`, `IsoFramingSettings`). The host resolves the iso camera's own
settings (field of view, axis, both margins, the minimum and maximum distance) and passes them with the
solo settings and the aspect. With matching off they are returned unchanged, bit for bit, so the framing
is the one from tasks 15 and 16. With matching on, the field of view and its axis are the solo camera's,
the margins are the matched ones, the minimum distance is 0, and the maximum distance stays the iso
camera's own. The iso camera's minimum distance (2000 cm by default) would otherwise keep one player
farther away than any solo boom below it (task 17's default 1200 cm; task 18's 2500 cm is above it, but og.cam.boomTarget can go lower) and break the "one player looks like the solo view" property.
The matched framing never comes closer than the boom on its own (up to float rounding), so the minimum is not needed for it.
`isoFramingDistance` also takes an `IsoFramingSettings` and passes its fields to the nine-argument form.

**The readout** (`cameraSight`, `CameraSight`). Everything the host prints in its sight readout comes from
one call: the solo boom, the solo horizontal and vertical fields of view, the solo reach, whether matching
is on, the resolved iso settings, and the iso distance for a given group. The Unreal host prints it as a
log line, only when an input changes and on a console command (the host's rationale has the details).

**Defaults (the host's).** Matching is on by default (`og.iso.matchSolo` 1). The iso camera's own field of
view (11.33°), margins (400 / 300 cm) and minimum distance are used only with `og.iso.matchSolo` 0. So by
default the shared camera renders the solo camera's field of view (90° on 16:9 with task 17's defaults,
65.8° with task 18's) instead of 20°, and it is much closer for a small group.

## 17. The solo field of view follows the pitch (og-ground-aligned-attacks task 18, 2026-10-30)

**The user request.** After tuning the solo camera in play to a 2500 cm boom and a 40° field of view, the
user asked that pitching toward the horizon also widen the view, "from my new 40, to 58 degrees when at
the horizon", as the boom already shortens toward the horizon.

**The blend** (`fovAtPitch`, `fovDegAtHorizon`, `kFovDegAtHorizon`). `fovAtPitch(pitch, target, settings)`
is `fovDeg − (fovDeg − fovDegAtHorizon) × (1 − towardTarget)`, the boom's formula (§8) with the two fields
of view in place of the two lengths, and the same factor from the same helper, `towardTargetPitch`. So at
or past the target pitch it is `fovDeg`, at the horizon (and below it) `fovDegAtHorizon`, and linear in the
pitch between them. Both angles are on the `fovAxis` axis: the host converts the blended angle with
`horizontalFovDeg` (§14), so on a landscape screen the blend is linear in the shorter-axis angle, not in
the rendered horizontal one. With `fovDegAtHorizon` equal to `fovDeg` the product is zero and the field of
view is constant at every pitch, the camera before task 18 (`DAttackOrbitCameraTest.cpp` pins the
endpoints, the midpoint, the shared factor and the constant case).

**The iso camera is unaffected.** The iso camera looks at the target pitch, so with `og.iso.matchSolo` 1
it keeps the solo camera's values at the target pitch: `fovDeg` and `boomLengthAtTargetPitch`
(`resolveIsoFraming` and `soloSightReach` never read the horizon field of view). One player in the shared
view is framed from 2500 cm at 40° by default (`DAttackCameraMatchSoloTest.cpp`). The readout's
`CameraSight` adds `soloHorizontalFovAtHorizonDeg`, the horizon angle converted at the live aspect.

**Defaults.** `kBoomLengthAtTargetPitch` 2500 (was 1200), `kFovDeg` 40 (was 58.72), `kFovDegAtHorizon` 58,
`kBoomLengthAtHorizon` unchanged at 550 (the user has not decided it yet). On 16:9 the shorter-axis 40°
renders 65.8° horizontal and 58° renders 89.2°. With the 60° iso target and the default 10° minimum pitch
the solo camera is at least 1/6 of the way to the target, so its field of view there is 55.0°.

## 18. Look mode orbits in the ground's frame (og-ground-aligned-attacks task 21, 2026-11-03)

**The user request** (2026-11-01): "the mode where the camera is rotated corrects it's 60 degree angle against the
slope normal ... up to a 30 degree slope, if the character falls onto the 60 degree slope it should not correct towards
the slope normal, instead keep correcting towards the world up vector. If the camera mode is disabled the camera should
be corrected back to a 60 degree angle." The mode is look mode (`lookHeld`). The rulings (AskUserQuestion, 2026-11-01;
the backlog task accepted as written 2026-11-03): orbit about the normal, a cut-off at 30° + 0.1° as compile-time
constants, only the yaw-driven pull corrects in look mode, world up while airborne, and a time-based return when look
mode is turned off. Display only: nothing here reaches the simulation or the wire.

**The frame** (`OrbitCameraState::frameUp`). `frameUp` is the up of the ground the camera follows. It moves only by the
look-mode pull and the look-mode-off return, below. What the host renders from it is `worldView`, task 21b's rule since
2026-11-03 (§19). `boomLength` and `fovAtPitch` keep their code and read the state's pitch.

**Task 21's world view, replaced by task 21b (§19).** Task 21 orbited the camera about `frameUp`. Its `worldView` built
the local view direction from the yaw and the pitch about Z-up, tilted it by the minimal rotation from Z-up to `frameUp`
(og-simulation's trig-free Rodrigues form, `surfaceFrame::tiltToSurface`, re-typed for the camera's Z-up space), and read
a roll-0 yaw and pitch back from the tilted view, the pitch clamped into [1°, 89°]. On the 30° face at the 60° target that
gave world pitch 30° up-slope, 48.59° along the contour (with the world yaw bent 40.9° toward up-slope), and down-slope a
straight-down view drawn at 89°, whose yaw needed a fallback (guard G-02, retired). Turning through the down-slope
heading passed the camera over the top of the character, and its yaw swung through 180° there. The user's G7 report
(2026-11-03): "looking up the slope the camera seems to correct to 30 degrees instead of 60 … when I turn the camera to
look down the slope it quickly rotates over to the other side"; "it felt like the camera almost hit 90 degrees as I
looked along the slope, when I expected it to have the same angle as when standing on flat ground".

**The target up** (`cameraTargetUp(groundUp, supported)`). The host passes the ground's up under its character
(Unreal space) and whether the movement is supported. The answer is:

* world up (`kCameraUp`) when unsupported (airborne): the user's "keep correcting towards the world up vector";
* world up when the ground's up is not a usable direction (zero length, not finite) or is flat (`isCameraUpFlat`
  after normalizing). The flat case returns world up exactly, so a 1-ULP-short face normal on flat ground (the G3
  capture's `up=(0,0.99999994,0)`) leaves the camera on today's branch;
* world up when the ground tilts more than `kCameraFollowMaxSlopeDeg + kCameraFollowSlopeEpsilonDeg` (30° + 0.1°)
  from world up: `!(up.z >= cos(30.1°))`, so a non-finite z falls to world up too. The cosine is computed at run time
  (`cameraFollowMinUpDot`: `std::cos` is not `constexpr` on this toolchain); the two degrees are `constexpr`s, as ruled;
* otherwise the normalized ground up.

**Why 0.1°.** The user: "Aim for an epsilon that always covers our exact 30 degree slope, all geometry in the game should
follow this limitation and use slopes between 0 and 30 degrees". The G3 capture (2026-11-02, 158 tilted swing starts on
the level's 30° face) measured the face at 30° + [−1.32e-5°, +6.4e-6°], so 30.1° is 0.0999936° above the largest
measured angle: about 7,600 times the largest measured deviation. A 30.09° normal follows and a 30.11° one does not
(`DAttackCameraSlopeFrameTest.cpp`).

**The correction in look mode: only the yaw-driven pull.** On a frame with a pull (§4 rules 5 and 6, §7), `frameUp`
turns toward `targetUp` along the great circle by `min(angle × fraction, cap)`: the pitch's own `fraction` (the same
exponent, with the same weight and pull window) and its own cap, `pullMaxDegPerSec × the window`, as an angle in
degrees (`rotateUpToward`, `angleBetweenDeg`). So the frame up and the pitch close the same share of their gaps on
every frame: after 60° of yaw both have 0.0498 of their gap left (`exp(−3)`), within 1e-4 of each other at 30 to
240 fps. With the mouse still, with a pitch-only input, during a hold-off and at a dominance weight of 0 nothing
re-tilts. Under the airborne rule this means a camera whose character leaves the ground turns toward world up only
while the player turns it, as the lead told the user before dispatch.

**Look mode off: the return** (`returnDegPerSec`, `kReturnDegPerSec` 90°/s; the Unreal host's og.cam.returnDegPerSec).
`integrate` no longer returns at once when look mode is off. It ignores the look input, keeps the yaw (since task 21c
it turns the yaw up a followed slope of 15° or more, §20), and moves `frameUp` toward world up (`rotateUpToward`) and
the pitch toward the target pitch (`stepToward`), each by at most `returnDegPerSec × dt` per frame, each snapping to
its goal when it is within one step. Both are reached after `ceil(angle / (returnDegPerSec × dt))` frames, at 30 to
240 fps, and then they are exactly world up and the target, so `worldView` is back on its flat branch: the frozen
camera before task 21, at world up and 60°. A rate of 0 or below gives that frozen camera from the start.

## 19. On a slope the pitch follows the ground's rise along the view, clamped; the yaw is the player's (og-ground-aligned-attacks task 21b, 2026-11-03)

**The ruling.** At G7 the user approved "Full tilt, clamped" (and "Before the commit"), after the report quoted in §18.
Display only: nothing reaches the simulation or the wire, and the shared iso camera is untouched. The target up, the
30.1° cut-off, the look-mode pull of `frameUp` and the look-mode-off return are task 21's, unchanged (§18).

**The rule** (`worldView(state, settings)`, its slope branch). With `p` the state's pitch, `ψ` its yaw,
`h = (cos ψ, sin ψ, 0)` the view heading and `n = frameUp`:

* the world yaw is `ψ`, the player's heading, at every heading. The slope never turns the camera, so nothing swings or
  flips;
* `rise = atan(−dot(n, h) / n.z)`, in degrees, is the ground's rise along the view heading: the angle of the ground's
  line in the vertical plane through the view. It is +30° looking up a 30° face, 0 along the contour and −30° looking
  down it. `n.z` is positive for every followed frame (the cut-off's `static_assert`);
* the world pitch is `clamp(p − rise, min(p, lo), max(p, hi))`, then the hard clamp [1°, 89°], with `lo` and `hi` from
  `slopePitchLimits` (the Unreal host's og.cam.slopePitchMin and og.cam.slopePitchMax; defaults `kSlopePitchMinDeg` 35°
  and `kSlopePitchMaxDeg` 75°).

So `p` is the pitch below the ground's line along the view: the camera keeps its angle to the ground it looks along, as
far as the limits allow. On the 30° face at p = 60° the world pitch is 35° up-slope (30° unclamped), 60° along the
contour (the flat angle, the user's expectation) and 75° down-slope (90° unclamped, drawn at the hard 89°). On a 12°
face it is 48°, 60° and 72°, inside the limits. A slope rising along Unreal Y gives the same numbers
(`DAttackCameraSlopeFrameTest.cpp`).

**Why `min(p, lo)` and `max(p, hi)`** (guard G-03). A pitch the player set outside [lo, hi], such as 20° or 80° inside
the player's own 10° to 80° limits, is itself a bound of the clamp, so where the rise is zero the slope branch returns
`p`, as the flat branch does. The clamp only limits how far the rise moves the pitch: at 20° the pitch stays 20°
looking up-slope (the rise would lower it, and it is already below 35°) and rises to 50° looking down-slope. Easing
`frameUp` from world up to the 30° face in 100 steps moves the pitch monotonically from `p`, at most 0.3° a step. A
clamp to [lo, hi] alone would pop a 20° camera to 35° on the first tilted frame.

**The limits are ordered** (`slopePitchLimits`). The maximum is clamped into the hard limits [1°, 89°] and the minimum
into [1°, maximum], so a minimum above the maximum takes the maximum (50 over 40 gives 40 to 40). The slope clamp's own
bounds are ordered whatever the limits are, because `min(p, lo) ≤ p ≤ max(p, hi)`. A `static_assert` holds the
defaults inside the hard limits, in order.

**No swing.** Turning through 360° in 0.5° steps at p = 60° on the 30° face, every world yaw step equals the input
step, bit for bit, and the pitch changes by at most `0.5° × tan 30°` (0.2887°) a step, with the defaults and unclamped:
the rise changes fastest along the contour, at `tan` of the face's angle per unit of yaw. Task 21's tilted yaw fails
this sweep: all 720 steps differ, the largest by 90.2°.

**Flat is unchanged.** While `frameUp` is world up (`isCameraUpFlat`), `worldView` returns the state verbatim: the
camera before task 21, bit for bit. At an exact world up the formula would also give `p` and `ψ` (the rise is 0). Just
past the flat tolerance (a 2e-6 rad tilt) the slope branch is within 1.1e-4° of the flat one.

**The host's other readers.** The camera-relative move basis and the aim fallbacks read the rendered camera's forward
flattened to the ground, which is the world yaw. On a slope in look mode they now follow the player's heading, without
task 21's bend toward the fall line. The mouse aim deprojects the cursor through the rendered camera and is only read
with look mode off, so on a slope it sees this rule only while the camera returns to world up (or for as long as it
holds still at a return rate of 0), and it follows whatever is drawn. Boom length and field of view still read `p`.

**Removed with task 21's view:** the tilt helper and the near-vertical threshold with its `static_assert` (nothing else
called them), and guard G-02 (retired).

## 20. Look mode off on a slope: the camera turns to face up the slope (og-ground-aligned-attacks task 21c, 2026-11-04)

**The request and the rulings.** The user at G7 (2026-11-04): "Could we make it so that the locked camera also aligns
with the slope normal? When the character is standing on a slope that is between 15 and 30 + epsilon it rotates to the
slope normal so that it always looks exactly up the slope". The rulings (AskUserQuestion): look mode off ("when look
mode is off, the camera should correct itself to a 60 degree angle, and when it is standing on a slope it should align
itself along the slope normal, on a 30 degree slope the camera should be perfectly aligned with the slope normal"); the
pitch stays 60° everywhere ("Always 60°, face up"); on leaving the slope the heading stays where it is; the turn rate is
90°/s, tunable; the task lands before the commit. Display only: nothing reaches the simulation or the wire, and the
shared iso camera is untouched.

**The rule** (task 21c's constant-rate step, `slopeYawStep`; since task 21d it is gated by the dwell, and since task
21e `integrate` turns through `slopeYawEase` toward the dwell's heading, eased, both below. At an ease of 0 and an
acceleration of 0 `slopeYawEase` takes this step's yaw bit for bit, and the tests keep `slopeYawStep` as its
reference). With
`u = input.targetUp`, which is
`cameraTargetUp`'s answer (§18: world up, or the normalized ground up of a supported slope up to 30° + 0.1°):

* the camera turns when `turnsUpSlope(u)`: the slope angle `acos(u.z)` in degrees, plus `kSlopeYawSlopeEpsilonDeg`
  (0.001°), is at least `kSlopeYawMinSlopeDeg` (15°). World up has angle 0, so flat ground, the air and ground past the
  cut-off never turn it. A non-finite `z` fails the comparison and holds the yaw;
* the goal is `upSlopeYawDeg(u)`, `atan2(−u.y, −u.x)` in degrees and wrapped: the heading opposite the normal's
  horizontal part, which points down the slope, so the goal looks up it;
* the yaw moves toward the goal along the shorter arc (the gap wrapped into [−180°, 180°]) by at most
  `max(0, slopeYawDegPerSec) × dt` per frame, and the result is wrapped as the player's yaw is (`wrapYawDeg`). Within
  one step it takes the goal, so it arrives after `ceil(gap / (rate × dt))` frames and then holds the goal bit for bit;
* otherwise the yaw is returned unchanged, bit for bit: below 15°, on flat ground, in the air, past 30.1°, and at a rate
  of 0 or below. Nothing turns it back, so when the character leaves the slope the heading stays (the ruling).

The pitch and `frameUp` keep their look-mode-off return (§18): they reach the target pitch and world up, so `worldView`
is on its flat branch and the camera is drawn at the goal yaw and the 60° target. Look mode on is untouched: the turn
sits only in the look-mode-off branch, and in look mode the yaw moves only with the player's input (§19).

**The dwell: the turn starts only on a steady slope (task 21d, 2026-11-04).** The user's G7 report: "a small camera
twitch when there is only one local brawler and mouse look is off, when I cross a flat to slope crease and when I walk
off the slope edge". With og.cam.slopeYawDegPerSec 0, "The twitch is gone", so the turn above caused it. The target up
is read from the guard body's up, which is posed to each tick's ground face, so at a crease or a rim the target can
change from tick to tick: the face, flat ground, the air, or an edge contact whose up-slope heading points elsewhere.
Without the dwell every slope tick turned the yaw by rate × dt (1.5° at 60 fps) toward that tick's goal. The state
carries `slopeDwellSeconds` and `slopeDwellYawDeg`, and each look-mode-off frame `slopeDwellStep` updates them:

* on a turning slope (`turnsUpSlope(u)`), with a dwell running (`slopeDwellSeconds` > 0) and this frame's goal within
  `kSlopeYawDwellToleranceDeg` (10°) of `slopeDwellYawDeg` (the gap wrapped into [−180°, 180°]), the dwell adds `dt`;
* on any other turning-slope frame it restarts: `slopeDwellSeconds = dt` and `slopeDwellYawDeg` = this frame's goal;
* off a turning slope `slopeDwellSeconds` returns to 0. `slopeDwellYawDeg` is kept, and is read only while a dwell runs.

The yaw turns only on a turning slope where `slopeDwellElapsed` holds: `slopeDwellSeconds + kSlopeYawDwellEpsilonSeconds` (1e-4 s) is at
least `max(0, slopeYawDwellSeconds)`, which is `kSlopeYawDwellSeconds` (0.2 s) by default and the console variable
og.cam.slopeYawDwellSec in the host. The pitch and `frameUp` return as before, every frame. Look mode on sets
`slopeDwellSeconds` to 0, so after look mode the dwell starts again; the look-mode-on yaw, pitch and frame up do not
read the dwell.

* **The first turning frame.** On a steady slope, frame `f` (counted from 1) holds the float sum of `f` frame times, and
  the first turning frame is the first `f` whose sum plus 1e-4 s is at least 0.2 s. That is frame `ceil(0.2 / dt)`:
  6, 12, 29 and 48 at 30, 60, 144 and 240 fps. The yaw keeps its bits for the `ceil(0.2 / dt) − 1` frames before it and
  then turns. With the easing off (task 21e, below) it takes 21c's steps, so it arrives `ceil(0.2 / dt) − 1` frames
  after 21c's `ceil(gap / (rate × dt))`; with the default easing it arrives after the turning times measured below.
* **Why a tolerance on the time.** At 240 fps the float sum of 48 frame times is 0.19999997, below the float 0.2, so a
  plain `>=` would start the turn on frame 49. The 1e-4 s is far above that error and below one frame at 1000 fps. At
  the tested rates it never starts a turn early: the frame before the turning one is at least 0.0042 s short (240 fps,
  whose 47 frames sum to 0.1958 s).
* **What a flicker does.** A target that alternates slope and flat ground (every tick, or in 2-tick runs) never keeps a
  dwell past 2 frames (0.067 s at 30 fps), so the yaw never moves. A goal that alternates with one 40° off, as an edge
  contact's can, restarts the dwell every tick, so from the first such tick the yaw holds; it turns again on the
  `ceil(0.2 / dt)`-th frame of a steady target. A goal alternating with one 9.9° off keeps the dwell running and turns
  on schedule; 10.1° off restarts it every tick. A target that stays on each face for longer than the dwell still turns
  toward each in turn: the dwell removes tick-to-tick flicker, not a slow alternation.
* **A dwell of 0 is task 21c** (with an ease of 0 and an acceleration of 0, task 21e). At 0 or below the comparison
  holds on every frame. Off a turning slope the yaw keeps its bits, as `slopeYawStep` keeps them. On one, the speed is
  the rate and the step is `slopeYawStep`'s, so the yaw is 21c's bit for bit wherever each turning frame's goal is the
  dwell's heading: on a steady slope, and wherever consecutive turning goals differ by more than 10° or have a
  non-turning frame between them (each restarts the dwell). A goal within 10° of a running dwell's heading is where
  task 21e differs on purpose (below). The cost of the default dwell is that the camera waits 0.2 s on a new slope
  before it starts to turn.

**The anchored goal and the easing (task 21e, 2026-11-04).** The user's G7 re-check, look mode off, one local brawler,
with og.cam.slopeYawDwellSec 2: "crossing the crease from flat to slope it twitches to a slightly new angle after 2
seconds, crossing the same crease from slope to flat it twitches instantly as I cross the crease"; with
og.cam.slopeYawDegPerSec 0, "the slope > flat twitch is gone". The crease is between the sloped box and the flat arc
platforms, whose collision is a hand-fitted convex hull. Two causes:

* **slope to flat:** once the dwell had elapsed, a tick whose goal lay within 10° of the dwell's heading kept the turn
  running, and 21c's step aimed at that tick's goal. The hull's edge facets give a few ticks of a slope target whose
  up-slope heading is a few degrees off, so the yaw stepped 1.5° (at 60 fps) toward each and back;
* **flat to slope:** after the dwell, a small leftover offset (from earlier look-mode orbiting) was closed at a constant
  90°/s, which reads as a snap.

**The goal is the dwell's heading.** The turn aims at `slopeDwellYawDeg`, the heading the dwell started on, never at
the tick's. A tick within 10° of it continues the dwell and leaves the goal where it is; one more than 10° off restarts
the dwell, which stops the turn (task 21d). So a goal flickering within the tolerance changes nothing the turn does:
the yaw and the speed are bit-identical to the steady slope's.

**The easing.** `slopeYawEase(yaw, goal, rate, dt, settings)` reads the state's speed `slopeYawRateDegPerSec`. With
`e` the goal − yaw gap wrapped into [−180°, 180°] and `maxRateDegPerSec` = `max(0, slopeYawDegPerSec)`:

* at a `maxRateDegPerSec` of 0 the yaw keeps its bits and the speed is 0;
* within `kSlopeYawArriveDeg` (0.01°) of the goal the yaw takes the goal exactly and the speed is 0, so the arrival is
  bit-stable;
* otherwise the target speed is `min(maxRateDegPerSec, |e| × (1 − exp(−dt / τ)) / dt)`, with τ =
  `max(0, slopeYawEaseSeconds)` (`kSlopeYawEaseSeconds`, 0.3 s; the host's og.cam.slopeYawEaseSec), or
  `maxRateDegPerSec` at τ = 0. The speed is
  `min(target, rate + accel × dt)`, with accel = `max(0, slopeYawAccelDegPerSec2)` (`kSlopeYawAccelDegPerSec2`,
  360°/s²; og.cam.slopeYawAccelDegPerSec2), or the target at an accel of 0. `std::max(0, NaN)` is 0, so a NaN setting
  counts as 0;
* the yaw moves `speed × dt` toward the goal by the shorter arc and is wrapped. A step that reaches the goal takes it
  exactly, with a speed of 0.

Whenever the turn does not run (look mode on, off a turning slope, or before the dwell has elapsed) the speed is set to
0 and the yaw keeps its bits, so every turn starts from rest.

* **Why `1 − exp(−dt / τ)` and not `|e| / τ`.** Both are the same law: a speed proportional to the gap, with time
  constant τ. `|e| / τ × dt` is its explicit Euler step. At 30 fps that closes 11.1 % of the gap per frame against the
  law's 10.5 %, so a 30 fps turn arrives early. From 91.9° off it turned in 3.100 s at 30 fps and 3.204 s at 240 fps,
  6.75 ms over the "one frame plus 2 %" budget. A sweep of start gaps from 0.5° to 180° in 0.1° steps fails 412 of
  them (3.2° to 121.9°). The factor `−expm1(−dt / τ) / dt` closes exactly the law's share of the gap each frame and
  passes the whole sweep (worst margin 3.8 ms). It is below `1 / τ`, so the speed stays below `|e| / τ` and the step
  below the gap (no overshoot), and it tends to `1 / τ` as dt goes to 0.
* **What it does.** From rest the speed rises by at most 360 × dt per frame, so from 5° off the first step moves at
  360 × dt degrees per second (12 at 30 fps, 1.5 at 240 fps), not 90. The speed caps at 90°/s, and over roughly the last
  27° (90 × 0.3) it falls with the gap. Measured turning times, from the first turning frame to the arrival, at 30, 60,
  144 and 240 fps: from 5° off 1.933, 1.900, 1.896 and 1.892 s; from 30° 2.567, 2.533, 2.528 and 2.525 s; from 91.9°
  3.267, 3.233, 3.222 and 3.221 s; from 180° 4.233, 4.217, 4.201 and 4.200 s. The yaw approaches the goal
  monotonically and arrives exactly.
* **Why the snap.** The eased speed falls with the gap, so without the snap the yaw would never reach the goal. 0.01°
  is invisible, and it is below one frame's step of the default rate at 1000 fps (0.09°), so with the easing off the
  turn takes the goal on 21c's frame (a `static_assert` holds it).
* **The cost.** A long turn takes longer to settle: 4.2 s from 180° off against 21c's 2.0 s, in exchange for no visible
  start or stop. og.cam.slopeYawEaseSec 0 with og.cam.slopeYawAccelDegPerSec2 0 gives task 21d's constant-rate turn
  back, still aimed at the dwell's heading.

**The sign, on the level's face.** The G3 capture's measured swing ups (og-simulation axes) are Unreal
(−0.49999997, 0, 0.866025448) and (−0.5, −1.49e-8, 0.866025388): the face rises along Unreal +X. The goal is
`atan2(−u.y, −u.x)` = 0° (−0° for the first, whose y is 0; 1.7e-6° for the second), heading +X, up the face, where
task 21b's rise along the view is +30°. At the 60° pitch the view direction is (cos 60° cos ψ, cos 60° sin ψ, −sin 60°)
= (0.5, 0, −0.866): −n, the camera looking straight down the normal, as ruled. The test measures the view against −n
within 1e-4 on five faces. A reversed sign turns the camera to face down the slope: view (−0.5, 0, −0.866), rise −30°,
1.0 off −n.

**Why the target up, not `frameUp`.** With look mode off `frameUp` returns to world up within a third of a second on
the 30° face, while a 180° turn at 90°/s takes two seconds (4.2 s with task 21e's easing). A turn read from `frameUp` would stop once the frame was
below 15°. The target up is the ground under the character, so it holds for as long as the character stands there.

**Why a tolerance on the threshold.** A face built at exactly 15° measures 14.9999762° in float after normalizing,
2.4e-5° short, so a plain `>=` would leave a 15° face out against the ruling that exactly 15° counts. The 0.001°
tolerance is 40 times that and far below the cut-off's 0.1°: a 14.9995° face turns and a 14.998° one does not. The
level's slopes are 0°, 30° and 60°, so the threshold needs no hysteresis.

**The host's readers.** Camera-relative movement and the aim fallbacks read the rendered camera's yaw (§19), so while
the camera turns, a held move direction turns with it, for up to about 4.4 s (the 0.2 s dwell, then up to 4.2 s of
eased turn). With look mode off that is the movement
keys and left stick in Camera-relative (7) and Aim follows move (9), the gamepad move-stick fallback in Aim-relative (8)
and Aim-relative, swapped sticks (0), and the aim stick in all four (camera-relative, or in Aim follows move relative to
the camera-relative walk). The mouse aim deprojects the cursor through
the rendered camera, so the world point under a still cursor moves while the camera turns.

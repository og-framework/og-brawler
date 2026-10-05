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

* **State** (`OrbitCameraState`): `yawDeg` is the world yaw in the host's yaw convention (the Unreal host
  uses it as the rotator yaw, so a larger yaw looks further right). `pitchDeg` is degrees **below** the
  horizon, so the iso camera's Unreal pitch of −60° is 60 here. `holdoffSeconds` is the vertical hold-off
  timer (§4 rule 4). It stays 0 unless the host switches the hold-off on (`verticalHoldoffSeconds` > 0, §13). Three explicit scalars: there is no quaternion and no Euler decomposition, so the
  gimbal flip at ±90° cannot happen and the sign of the pitch is a plain negation.
* **Input** (`OrbitCameraInput`): `lookHeld` (look mode is on; the Unreal host passes its BlockLook state, a toggle on 5 / gamepad Y since
  og-attackstatetransition-cleanup task 12, a held button before), `mouseUnits` (the Look action's
  value summed over the frame, §3), `stick` (the look stick, x right and y up, as the host routes it) and
  `targetPitchDeg` (the pitch the pull aims for; the Unreal host passes the iso pitch).
* **Output:** the state. The host writes the boom as `FRotator(-pitchDeg, yawDeg, 0)` and its length as
  `boomLength(pitchDeg, targetPitchDeg, settings)`.
* **Seeding:** `seedFromUERotation(uePitchDegrees, ueYawDegrees, settings)` gives yaw = the Unreal yaw (wrapped) and
  pitch = −the Unreal pitch, clamped into the limits, with no hold-off. `pitchDegFromUEPitchDegrees`
  is the same conversion for the target. Both clamp, so the state's pitch is always inside the limits.
  A default-constructed state is (0, 0, 0) and is meant to be overwritten by a seed.

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

1. **Nothing moves unless `lookHeld`.** The hold-off timer still counts down while look mode is
   off (it measures time since the last deliberate vertical look, not time spent looking), so a
   pause longer than the hold-off never delays the next pull.
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
   There is no idle spring-back: the camera never moves on its own (a user decision, 2026-10-04).

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

`boomLength` keeps the old camera's curve: 900 at or above the target pitch, falling linearly to 400 at
the horizon (`kBoomLengthAtTargetPitch`, `kBoomLengthAtHorizon`). With the default limits the shortest boom
is about 483 at the 10° minimum. A host that sets the boom length itself when it seeds the camera (before
the first `integrate`) must use `kBoomLengthAtTargetPitch`, because a seed at the target pitch is a
boom of that length.

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

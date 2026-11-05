<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `AimPlane.h` / `AimPlane.cpp` — rationale

Companion to `OGBrawler/InputMapping/AimPlane.h` and `OGBrawler/InputMapping/AimPlane.cpp`: the engine-free mouse-aim
plane, which turns the cursor's world ray into the unit aim the input component feeds the simulation. **The code
carries the arithmetic; this file carries the reasoning.** There is **no guards file**: the rules the code must keep
are held by the brawler test suite (og-brawler-tests `AimPlaneTest.cpp`, §6), and no prohibition here is one a tag at
a line would stop better than those tests.

Written by og-ground-aligned-attacks task 12 (2026-11-03), as a new file pair in the new comment convention.

---

## 1. What it is for

`AimPlane.h` :: `tiltedPlaneUp`, `hitOffset`, `unitAim`, `AimAxes`

The user at gate G3 (2026-11-02): "check 11 worked fine, but we should still align the mouse target direction with the
slope", ruled as OQ8 (a) the same day. Before this task the input component
(`UOGBrawlerInputCollectionComponent::updateGameThreadCache`) intersected the cursor's ray with the **horizontal** plane
through the capsule centre, and the attack machine then took that horizontal aim into the ground frame and flattened
it (`simFrame::flatten` of `surfaceFrame::tiltFromSurface`, og-ground-aligned-attacks task 5). Along the iso view the
two agree; from a third-person camera at a non-zero yaw on the 30° slope they do not, because the horizontal plane's
hit is not the point on the slope under the cursor. Measured by `AimPlane.TheSwingHeadingPassesUnderTheCursorOnTheSlope`
over its grid (§6): up to 24.8° between the swing's heading and the direction to the cursor's slope point from a
camera 2500 cm away at 60° pitch, and rays that hit the horizontal plane behind the camera (a reversed aim).

The task moves the intersection into this engine-free pair, so it is testable, and gives it a plane normal: the
character's ground up.

## 2. The plane

**The normal is the guard body's up.** The caller reads it with `UOGBrawlerSimulatableComponent::vizGround`
(og-ground-aligned-attacks task 21; the guard body's up from the visualization snapshot, in og-simulation axes,
`dAttackVisualizationUtils::guardBodyUp`). The guard is posed in the attack ground sample's frame every tick (task 6):
the exact face normal on walkable ground, otherwise world up (`brawlerMovementSimulation::attackGroundNormal`). That is
the normal the swing frame and the guard use, so an aim in that plane is one the machine's flatten leaves alone (§5).
It is read whether or not the character is supported: the guard is posed from the same ground sample the swing starts
from in either case.

**The plane point stays the capsule centre**, as before.

**`tiltedPlaneUp` decides whether the plane tilts.** It returns the ground up unchanged when the plane should tilt,
and nothing when it should not:

* a flat up (`surfaceFrame::isFlat`, the 1e-6 tolerance every flat test in this initiative uses) returns nothing.
  The level's flat faces can carry a normal 1 ULP short of world up (`(0, 0.99999994, 0)`, seen in the G3 capture), and
  this keeps them on the exact world-up literal the caller wrote before the task, so flat ground runs today's
  numbers (§4);
* an up that does not point up (`dot(kUp, n) <= 0`: zero, horizontal or downward) returns nothing. The guard pose
  cannot produce one; the case is closed so that a broken snapshot falls back to world up rather than to a plane facing
  down.

Nothing means world up. The caller also takes world up when `vizGround` is empty (no manager yet, the character not
registered) and when the pawn has no simulatable component.

## 3. The intersection

`AimPlane.h` :: `hitOffset`, `kParallelTolerance`, `kZeroAimTolerance`

`hitOffset` is the input component's former statements, with the plane normal as a parameter, in the same operations
and order:

1. the plane point to the ray origin, in double;
2. the normal's dot with the ray direction, in double, **narrowed to float** (the old `const float dotProduct`);
3. reject if its magnitude is below `kParallelTolerance` (a float compare): the ray is near-parallel to the plane;
4. the normal's dot with step 1, divided by step 2 widened back, **narrowed to float** (the old `const float distance`);
5. the hit point: the ray origin minus the direction times that distance, in double;
6. the offset from the plane point, in double;
7. reject if every component's magnitude is at most `kZeroAimTolerance` widened to double: the engine's
   `IsNearlyZero` with its default tolerance, a cursor on the capsule centre.

The two narrowings are not cleanups to make: they set the bits of every aim, and they bound its accuracy (§5). Both
tolerances are 1e-4, the engine's KINDA_SMALL_NUMBER; og-brawler cannot include the engine, so the brawler test
asserts the equality at compile time where the engine is on its include path.

The function is frame-agnostic: the caller passes Unreal axes in centimetres (the doubles the deprojection returns,
copied component by component), and the zero tolerance is therefore in centimetres, as before.

## 4. Why the axis conversion is a parameter of `unitAim`

`AimPlane.h` :: `unitAim`, `AimAxes`

The former last statement was `glm::normalize(ueFrame::directionToSim(aimVec))`: narrow to float in og-simulation axes,
**then** normalize. glm's normalize sums the squares in component order, so normalizing in Unreal axes and permuting
afterwards can change the last bit. `unitAim` therefore takes the conversion from the caller (`AimAxes`, a plain
function pointer), applies it to the offset and normalizes the result: the old order, with the conversion still done by
`ueFrame` in the game module (its out-of-line narrowing included). The brawler test passes its own permutation of the
same axes.

The bodies are in the `.cpp`, so the game and the test target run one compiled body each, not one inlined copy per
caller.

**World up is today's aim.** With the normal `(0, 0, 1)` the helper computes every value the old statements did, and
`AimPlane.WorldUpIsTodaysMouseAimBitForBit` compares the two over 138,247 rays: 0 mismatches. One difference is
visible in the disassembly and not in the numbers: the old code had the constant normal folded in, so the compiler
reduced both dots to the direction's (or the offset's) height, while the helper multiplies out a runtime normal
(`0·x + 0·y + 1·z`). The products by zero are zeros and the sum is the height exactly; the two can differ only in the
sign of an exact zero, where a zero along-normal component is a reject anyway and a zero distance moves no non-zero
coordinate.

## 5. On a slope

The aim then lies in the slope plane, so the machine's `simFrame::flatten` of `surfaceFrame::tiltFromSurface` is the
identity on it to rounding, and the swing's heading, taken back to the world, points at the cursor's hit point.
Measured over the test grid (two 30° slopes, four capsule positions, 240 camera poses, 48 targets per pose):
|dot(aim, n)| at most 6.95e-7, the angle to the hit at most 8.2e-7 rad, the in-frame lift at most 7.1e-7 and the swing
heading at most 7.1e-7 rad from the cursor.

That residue is the float `distance` of §3, step 4: a relative error of up to 2⁻²⁴ on a distance of a few thousand
centimetres moves the hit by up to about 2e-4 cm along the ray. On flat ground the same rounding leaves the old aim up to 6.97e-7 out of
the horizontal (the same test's `worst |aim.y|`), so the slope aim is as flat in its plane as the old aim was in its
own.

## 6. Where it is pinned

og-brawler-tests `AimPlaneTest.cpp` (`[DAttack][AimPlane]`; its rationale is that file's `AimPlaneTest-rationale.md`):
world up against the old statements bit for bit, the 30° aim in the plane and at the hit, the swing heading under the
cursor, both rejects for world up and both slopes, and `tiltedPlaneUp` on flat, nearly flat, downward and tilted ups.
Seen failing: a helper that ignores its normal and uses `(0, 0, 1)` fails the 30° case (|dot(aim, n)| 0.4999969).

## 7. What it does not change

* A ray that meets the plane behind the camera still gives an aim away from the cursor, as the horizontal plane did;
  the task keeps today's two rejects and adds none.
* The reject paths still write a zero aim (the caller's `value_or`), which `UOGBrawlerInputCollectionComponent::buildAimDirection`
  still treats as no mouse aim (its 0.2 length test) and replaces with the camera forward.
* The aim is an input value only: the wire carries the same 12-byte aim field, so there is no wire or version change.

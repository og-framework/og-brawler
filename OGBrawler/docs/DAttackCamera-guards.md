<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: UInputModifierNegate -- the engine's own Enhanced Input negate modifier; owned by the host engine, not by this repository, and must not resolve here -->
# `DAttackCamera.h` / `DAttackCamera.cpp` — guards

Prohibitions with a tagged site in `DAttackCamera.cpp`. Ids are opaque, stable, and retired rather
than reused.

The header's range rules are held by the compiler and need no entry: the hard pitch limits stay inside
(0°, 90°), the default limits sit inside the hard ones, the stick deadzone is below 1, the pull's
e-fold (and the minimum a host value is raised to) and cap are positive, and the default hold-off and
dominance ratio are not negative (`static_assert`s in `DAttackCamera.h`; rationale §6 and §13). The
Catch2 suite in og-brawler-tests (`DAttackOrbitCameraTest.cpp`, `DAttackCameraIsoSeedTest.cpp`) pins
the behaviour.

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

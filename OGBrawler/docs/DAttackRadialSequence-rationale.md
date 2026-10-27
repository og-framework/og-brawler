<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `DAttackRadialSequence.h` / `DAttackRadialSequence.cpp` — rationale

What an authored swing table is, how its derived values are built, what its lookups return, and the two
measure-zero ties of its direction lookup. The **prohibition** is in `DAttackRadialSequence-guards.md`.

**If this file and the source disagree, the source is authoritative and this file is stale.** Fix this
file.

**Provenance.** Converted by og-sim-metres-yup task 26 (2026-10-26) from og-brawler `fcef52f`: the header
was 96 lines (md5 `1345ab80…`) with 21 comment lines in two runs besides the licence, the source 298
lines (md5 `0c3ee44c…`) with 38 (a commented-out initializer, one label, a five-line explanation, a
commented-out lookup of 31 lines over pristine lines 180-214) and three inline remnants. Every removed comment is kept verbatim in
[§A](#a-the-removed-comments-verbatim). Where a section and §A disagree, the section wins.

---

## 1. What a sequence is

A `DAttackRadialSequence` is one swing: a table of `DAttackRadialSequencePoint`s (time, angle, state)
that turns the weapon about a rotation axis, plus a `timeToReachZeroVelocity`. The shipped five live in
`simulatableBrawler::StaticData` (`SimulatableBrawlerTypes.h`): sequences 0 to 3 turn about
`simFrame::kTurnAxis`, sequence 4, the overhead, about `simFrame::kVerticalTurnAxis`.

The constructor derives a piecewise-constant angular acceleration and the velocity at each point, from rest
at the first point. Then it **appends** one more point at `m_attackPoints.back().time +
timeToReachZeroVelocity`, with the acceleration that brings the velocity back to zero over that span and
the angle that acceleration reaches ("reach zero velocity at the end", §A). So `m_attackPoints` holds one
more point than the table.

The angles are planar-chart angles about the sequence's rotation axis. Since og-sim-metres-yup task 26 the
chart is the native one (`SimulationFrame-rationale.md` in og-simulation, §3): a positive angle turns
counter-clockwise about `kTurnAxis`, `+Y`, and every shipped angle is the negation of the value it had
under chart P, when angles turned about `−Y`. Negation is exact, so every derived acceleration, velocity
and appended point negated exactly too, and the world motion is unchanged.

---

## 2. `getDuration` is not the authored window

`getDuration()` returns `m_attackPoints.back().time` after the append of §1: the last authored keyframe
time **plus** `timeToReachZeroVelocity`. Every shipped sequence has a `timeToReachZeroVelocity` of 0.1 s,
so it is 0.1 s longer than the table: sequences 0 and 1 0.6 → 0.7 s, 2 and 3 0.4 → 0.5 s, 4 0.42 →
0.52 s.

That value is what the attack machine sizes a swing with: `dAttackMachineSimulation::swingTickCount` is
called with `getDuration()` at its two end-tick writes in `DAttackMachineSimulation.h`, and that
function's `OG_CHECK` text says so ("plus the appended zero-velocity point"). The radial sub-simulation's
own active window (`state.attackTimer < activeAttackSequence.getDuration()` in `DAttackRadialSimulation.h`)
reads it too. A caller that wants the **authored** window reads the table instead (the last point of the
authored table, or `getAttackPointTime` at `getAttackPointCount() - 2`).

History, kept as provenance: the name misled twice in movement-sim tasks 84 and 86 (a hand derivation of
0.7000000476837158, and a six-tick swing that nearly shipped a stun one tick short), which is why task 87
wrote the warning that §A keeps. Changing the value is guard G-01.

---

## 3. `getAngle` and `getAngularVelocity` walk the same segments

`getAngle(time)` finds the segment `[t0, t1)` that contains `time` and returns
`o0 + w0*dt + a*dt*dt/2`. `getAngularVelocity(time)` is the same walk differentiated, `w0 + a*dt`.
Outside the span both clamp: before it to the front point's angle and velocity (velocity 0, the swing
starts at rest), after it to the back point's.

The angular velocity is **table data**: the authored derivative, not the body's captured
`angularVelocity`. The hit detector reads its sign to turn the swing plane's tangent into the weapon's
direction of travel (`BrawlerHitDetectionSystem-rationale.md`), which is the direction a hit throws its
target. The captured velocity would answer the same question from the physics engine's integration and so
could differ between peers; the table value cannot.

The two walks are written in parallel on purpose, so that a change to one that is not made to the other is
visible side by side. `DAttackRadial.GetAngularVelocityIsTheDerivativeOfGetAngle` (og-brawler-tests,
`DAttackRadialFirstTickCollisionTest.cpp`) checks the derivative numerically for every shipped sequence and
the two clamps.

---

## 4. The lookups

* `getDirection(index)` is `(cos θ, sin θ)` of point `index`: a planar-chart direction.
* `getVectorInRotationPlane(v)` maps a world vector into the chart of the sequence's rotation plane: it is
  `simFrame::planar(v)` for a sequence that turns about `turnAxis()` (that is `simFrame::kTurnAxis`), and
  otherwise `planar` of `v` rotated by `getFromRotationAxisToTurnAxis()`, the rotation that takes the
  sequence's axis onto the turn axis (`dMathUtil::getRotationMatrix`). For the overhead, task 26 negated
  both the axis and the turn axis, and the rotation between them, built from their cross product and dot
  product, is the same matrix.
* `getAttackSegment(index)`, `getAttackSegment(float angle)` and `getDAttackRadialSequenceState(float)`
  index the table. The float overloads have no caller in og-brawler.
* `getAttackSegment(const glm::vec2& localDirection)` is the one the hit detector and the radial
  visualization use, through `dAttackRadialSimulation::getAttackSegment`: §5.

---

## 5. `getAttackSegment` of a direction: the wrap and its two ties

The lookup takes `glm::orientedAngle` from the sequence's initial direction (`getDirection(0)` computed
inline) to `localDirection`, wraps it into the sweep, and finds the segment whose span, measured from the
initial angle, contains it. The wrap depends on the sign of the velocity at point 1:
* positive (the sequence turns towards positive angles): a negative angle gets `2π` added;
* negative: an angle `<= 0` becomes its absolute value, a positive one `2π −` itself.

If no segment contains the angle, the "no segment" value is returned: index `getAttackPointCount()`,
state `Idle`, both angles 0.

glm's 2-D `orientedAngle` returns the unsigned angle when the 2-D cross product is strictly positive and
its negation otherwise (`glm/gtx/vector_angle.inl`), so at an **exactly zero** cross it returns the
negated angle whatever the side. That is not odd under a mirror of the chart, and task 26 mirrors the
chart, which swaps the two branches of the wrap for every sequence. Two inputs meet it:

* **The exact parallel tie (Q13, task 26a).** A direction whose dot product with the initial direction
  rounds to at least 1 has an unsigned angle of exactly 0, so the oriented angle is `−0`. The positive
  branch keeps it (segment 0). The negative branch used `angle < 0.f`, false for `−0`, and wrapped it to
  `2π` (no segment). Task 26a made the test `<=`, so both branches give segment 0, and the mirror cannot
  move it. `DAttackRadialSegmentTieTest.cpp` (og-brawler-tests) pins it.
* **F1: the unit-length copies (user ruling 2026-10-26: accepted).** `getDirection(0)` is not exactly unit
  length (for ±π/2 the sine is `0x3f7fffff`), so for the initial direction itself the dot product rounds to
  `0x3f7ffffe`, the unsigned angle is 2⁻¹¹ rad (`0x3a000000`) and the cross is exactly 0: the oriented
  angle is −2⁻¹¹. The positive branch wraps it to 2π − 2⁻¹¹ (no segment), the negative branch to +2⁻¹¹
  (segment 0). The antiparallel copy (`-getDirection(0)`) has an unsigned angle near π, also with a zero
  cross, and the two branches wrap it 2π apart in the same way. Task 26 swaps the branches, so these
  inputs move between segments. Measured with task 26a's probe on the shipped table, P against N:

  | sequence | `getDirection(0)` under P → N | `-getDirection(0)` under P → N |
  |---|---|---|
  | 0 | no segment (6) → 0 | 6 → 6 (unchanged) |
  | 1 | 0 → no segment (6) | 6 → 6 (unchanged) |
  | 2 | no segment (8) → 0 | 4 → 3 |
  | 3 | 0 → no segment (8) | 3 → 4 |
  | 4 | no segment (5) → 0 | 1 → 1 (unchanged) |

  The same probe showed the twice-scaled direction (the 26a tie) at segment 0 in both charts for every
  sequence. The user accepted F1 as a further measure-zero exception beside Q13, with no code change: a
  hit direction is a normalised world vector projected through `getVectorInRotationPlane`, and it equals a
  sequence's own unit initial direction, bit for bit, only on a set of measure zero.

Every other input wraps to the same value in both charts: `std::abs(x)` and `−x` agree for negative `x`,
and the antiparallel exact case (`−π`) already mirrored.

---

## §A. The removed comments, verbatim

`DAttackRadialSequence.h`, above `getAngularVelocity` (pristine lines 40-44) → §3:
```cpp
	// [movement-sim task 83] The AUTHORED angular velocity at `time`, i.e. the exact derivative
	// of getAngle within a segment. Its SIGN is what turns the swing plane's tangent into the
	// weapon's direction of TRAVEL, which is the direction a hit throws its target.
	// The captured body angularVelocity would answer the same question from the engine's
	// integration and therefore differ between peers; this one is table data and does not.
```

`DAttackRadialSequence.h`, above `getDuration` (pristine lines 59-74) → §2 and guard G-01:
```cpp
	// ⛔⛔ [movement-sim task 87] THIS IS NOT THE AUTHORED WINDOW ITS NAME PROMISES.
	// The constructor APPENDS one more point at `m_attackPoints.back().time +
	// timeToReachZeroVelocity` (the "reach zero velocity at the end" block), and this returns
	// `m_attackPoints.back().time` AFTER that append. So it is the LAST AUTHORED KEYFRAME TIME
	// PLUS `timeToReachZeroVelocity` -- 0.1 s longer than the table for every shipped sequence
	// (seq 0/1 0.6 -> 0.7 s, seq 2/3 0.4 -> 0.5 s, seq 4 0.42 -> 0.52 s).
	// ⛔ IT HAS COST TWO HAND DERIVATIONS IN THIS INITIATIVE -- task 84 (a wrong
	// 0.7000000476837158) and task 86 (SIX ticks, which nearly shipped a stun one tick short).
	// The truth was already written down in `dAttackMachineSimulation::swingTickCount`'s
	// OG_CHECK ("plus the appended zero-velocity point") and BOTH readers still missed it, which
	// is why it now sits HERE, on the declaration nobody can call without reading.
	// ⭐ This is what the MACHINE reads when it sizes a swing
	// (`DAttackMachineSimulation.h`, the two `swingTickCount(... .getDuration(), ...)` sites);
	// read `m_attackPoints` / the authored table directly if the AUTHORED window is what you
	// want. ⚠ Behaviour unchanged by task 87 -- the name and the value are load-bearing for
	// every caller and this edit is legibility only.
```
Corrected reading: `m_attackPoints` is private, so a caller reads the authored table through
`getAttackPointTime` (§2). The radial's own active window reads `getDuration` too, besides the machine's
two sites.

`DAttackRadialSequence.cpp`, in the constructor's initializer list (pristine line 18), a commented-out
alternative with the two directions swapped. Removed as dead code; the shipped rotation is from the
sequence's axis to the turn axis:
```cpp
	//, m_fromRotationAxisToTurnAxis(dMathUtil::getRotationMatrix(turnAxis(), m_rotationAxis))
```

`DAttackRadialSequence.cpp`, the constructor (pristine line 61) → §1:
```cpp
	//reach zero velocity at the end
```

`DAttackRadialSequence.cpp`, above `getAngularVelocity` (pristine lines 110-114) → §3:
```cpp
// [movement-sim task 83] The SAME SEGMENT WALK as getAngle above, differentiated: inside the
// segment containing `time` the angle is o0 + w0*dt + a*dt*dt/2, so the velocity is w0 + a*dt.
// Outside the authored span it is the front / back table velocity, matching getAngle's clamp to
// the front / back angle. Keeping the two walks textually parallel is deliberate: an edit to one
// that is not mirrored in the other is visible here rather than only in a hit direction.
```

`DAttackRadialSequence.cpp`, the start of `getAttackSegment(const glm::vec2&)` (pristine lines 180-214), an
earlier cross-product formulation of the lookup, commented out. Removed as dead code:
```cpp
	//auto isBetween = [](const glm::vec2& target, const glm::vec2& vec1, const glm::vec2& vec2, bool positiveRotationDirection) {
	//	float crossProduct = vec1.x * target.y - vec1.y * target.x;
	//	float crossProduct2 = vec2.x * target.y - vec2.y * target.x;
	//	float crossProduct3 = vec1.x * vec2.y - vec1.y * vec2.x;

	//	if (positiveRotationDirection)
	//	{
	//		if (crossProduct3 > 0)
	//			return crossProduct >= 0 && crossProduct2 < 0;
	//		else
	//			return !(crossProduct < 0 && crossProduct2 >= 0);
	//	}
	//	else
	//	{
	//		if (crossProduct3 < 0)
	//			return crossProduct <= 0 && crossProduct2 > 0;
	//		else
	//			return !(crossProduct >= 0 && crossProduct2 > 0);
	//	}
	//};

	//for (auto attackPointIt = m_attackPoints.begin(); attackPointIt != m_attackPoints.end(); ++attackPointIt)
	//{
	//	auto attackPointNextIt = attackPointIt + 1;

	//	glm::vec2 current(std::cos(attackPointIt->angle), std::sin(attackPointIt->angle));
	//	glm::vec2 next(std::cos(attackPointNextIt->angle), std::sin(attackPointNextIt->angle));
	//	if (isBetween(localDirection, current, next, attackPointNextIt->angle > attackPointIt->angle))
	//	{
	//		const unsigned int index = (attackPointIt - m_attackPoints.begin());
	//		return DAttackSegment{ attackPointIt->angle, attackPointNextIt->angle, index, getDAttackRadialSequenceState(index) };
	//	}
	//}

	//return DAttackSegment{ 0, 0, static_cast<unsigned int>(m_attackPoints.size()), DAttackRadialSequenceState::Idle };
```

`DAttackRadialSequence.cpp`, inline remnants in the same function, removed without a change to the code
they sat in: `/*2.f * glm::pi<float>() -*/` before the `glm::orientedAngle` call (pristine line 220),
`/*std::abs*/` twice in the segment test (line 241: the two angles already pass through `std::abs` where
they are computed), and a bare `// TODO` after the "no segment" return (line 247), which said nothing more.

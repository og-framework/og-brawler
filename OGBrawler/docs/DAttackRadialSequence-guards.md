<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `DAttackRadialSequence.h` / `DAttackRadialSequence.cpp` — guards

Every prohibition for the swing table that the compiler does not already hold. Each entry has an
**opaque, stable id**. In the source a single line `// ⛔G-nn` sits exactly where the wrong edit would be
typed.

**If this file and the source disagree, the source is authoritative and this file is stale.** Fix this
file. Do not soften the source to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or run-time check,
moves to [§R, Retired ids](#r-retired-ids) and its number is spent forever.

⛔ **Nothing in this file is a rationale.** What a sequence is, its derived values and its lookups live in
`DAttackRadialSequence-rationale.md`.

**Provenance.** Converted by og-sim-metres-yup task 26 (2026-10-26) from og-brawler `fcef52f`. The one
entry opens with the comment it replaces, verbatim; rationale §A keeps every removed comment.

---

## G-01 — `getDuration` returns the appended end, not the authored window

**Tag site:** `DAttackRadialSequence.cpp`, `getDuration`, on `return m_attackPoints.back().time;`.

<!-- header lines 59-74 (pristine DAttackRadialSequence.h) -->
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

**The prohibition.** Do not change the value `getDuration` returns: in particular, do not make it return
the last **authored** keyframe time (the table's end, one point before `m_attackPoints.back()`), however
much the name suggests it.

**What breaks if the edit is made.** Every caller is sized by the appended end. The machine's two
end-tick writes (`swingTickCount(… .getDuration(), deltaTime)` in `DAttackMachineSimulation.h`) and the
radial's active window (`state.attackTimer < activeAttackSequence.getDuration()` in
`DAttackRadialSimulation.h`) would all end every swing 0.1 s early, before the weapon has come back to
rest, and the machine's `Attacking` window and the radial's swing would shrink together. A caller that
wants the authored window reads the table (rationale §2) instead.

**Placement.** A substitution on the tagged statement. The edit the pristine comment warned callers about
(reading this value as the authored window) is typed at a caller, so it has no tag: rationale §2.

---

## §R — Retired ids

*(None.)*

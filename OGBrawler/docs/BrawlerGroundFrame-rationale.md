<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerGroundFrame.h` — rationale

The narrative behind `BrawlerGroundFrame.h`: the brawler's per-tick ground sample, the one normal every
attack consumer is handed. The header keeps its licence, its code and a `static_assert` (its one guard,
G-01, retired with the knob in task 14, §3); everything that explains it lives here.

**If this file and `BrawlerGroundFrame.h` disagree, the header is authoritative and this file is stale.**
Fix this file; do not soften the header to match it.

⛔ **Do not move a prohibition into this file.** Prohibitions with a site live in
`BrawlerGroundFrame-guards.md`, reachable from the `⛔G-nn` tag at that site.

---

## §1 What the header is for

Ground-aligned attacks build every attack quantity (the swing plane, the aim, the guard pose, the
projectile launch) in a ground-local frame whose up is the brawler's ground contact normal, then tilt it
into the world with `surfaceFrame::tiltToSurface`. The normal comes from one place:
`brawlerMovementSimulation::sampleAttackGround`, which `SimulatableBrawler::integrate` calls once per tick,
before the attack machine, and hands by reference to the machine and the guard. `GroundSample` is the type
of that hand-off.

The header is a **leaf**: it includes glm, `OGSimulation/SimulationFrame.h` and
`OGSimulation/SurfaceFrame.h` and nothing from og-brawler. `BrawlerMovementSimulation.h` includes
`DAttackMachineSimulation.h`, so the machine cannot include the movement header back (that would be the
include cycle `DAttackMachineSimulation-rationale.md` §1 records); a leaf both of them include is the only
place the shared type can live.

Everything is in `namespace brawlerGround`.

---

## §2 `GroundSample`

One field, `normal`, a unit world vector: the walkable ground's face normal under the brawler, or
`simFrame::kUp` when there is none (airborne, a miss, or a face steeper than the movement's walkable
cap). Its default is `simFrame::kUp`, and a `static_assert` holds the default to `kUp`'s exact bits, `+0`
zeros included: the default is what every consumer receives off walkable ground, and each consumer's flat
path (behind `surfaceFrame::isFlat`) was written against exactly that up.

The sample is **not state**. It is re-taken every tick from the wire-restored movement `State` (the body
position) and static geometry, so a replayed tick sees the same sample as the authority's tick; the
consumers that need it beyond one tick freeze it into wire fields they already have (the radial
initial conditions, the projectile slot). It costs 0 wire bytes.

⚠ **Not the movement's `DerivedState::surfaceNormal`.** That field is the movement's own probe normal
(the sweep's `normal`, `kWorldUp` on unwalkable ground), written at the end of the movement step and **not
restored on rollback**, so on the first replayed tick it holds the pre-rollback frontier's value. The
sample reads `SweepHit::surfaceNormal` (the exact face normal an adapter may provide) with a fallback to
`SweepHit::normal`, through `probeGround`; it never reads the `DerivedState` field. The two names are
alike and the quantities are not.

---

## §3 The retired knob (tasks 4 to 14)

From task 4 to task 14 the header also held a compile-time `bool`, the one switch for the whole feature,
applied at the single normal source: `sampleAttackGround` took a `bool` template parameter that defaulted
to it (the movement's `detachesFromSupport`, `writesKnockbackVelocity` and `integrate` did too). Off, the
sample was `kUp` and issued no query, so every consumer ran its flat path everywhere: that was the state
while the consumers were built and tested (tasks 4 to 9b), when the slope and ramp ticks kept their
values bit for bit. Task 10 turned it on, with the correction wire format version 9 -> 10 and both golden
traces re-recorded. Per-consumer switches and a runtime console variable were considered and rejected
(peers must agree on the value; a runtime value would need the wire token to carry it).

Task 14 deleted it (OQ4 (a), ruled by the user after G3), with the template parameter and every off
branch, keeping the on arms verbatim: the shipped build computed the on arms already, so the deletion
changed no float (both goldens exact). `sampleAttackGround` is now a template on its query type only,
and `detachesFromSupport` and `writesKnockbackVelocity` are plain `inline` functions. A test that wants a
slope passes a tilted `GroundSample` directly or scripts the sweep the sample reads.

---

## Provenance

Written with the header on 2026-10-28 (og-ground-aligned-attacks task 4); §3 updated for the flip
(task 10) and rewritten as history when task 14 retired the knob. Nothing in this file depends
on any document outside this submodule.

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerGroundFrame.h` — rationale

The narrative behind `BrawlerGroundFrame.h`: the brawler's per-tick ground sample, the one normal every
attack consumer is handed, and the compile-time knob that decides whether it follows the ground at all.
The header keeps its licence, its code, a `static_assert` and one `⛔G-nn` tag; everything that explains it
lives here.

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
`simFrame::kUp` when there is none (airborne, a miss, a face steeper than the movement's walkable cap, or
the knob off). Its default is `simFrame::kUp`, and a `static_assert` holds the default to `kUp`'s exact
bits, `+0` zeros included: the default is what every consumer receives while the knob is off, and each
consumer's flat path (behind `surfaceFrame::isFlat`) was written against exactly that up.

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

## §3 `kGroundAlignedAttacks`

A `constexpr bool`, `false`. It is the one switch for the whole feature, applied at the single normal
source: `sampleAttackGround`'s default template argument is this constant, and with it `false` the
sample is `kUp` and issues no query. Every consumer then runs its flat path, so the slope and ramp ticks
keep today's values bit for bit while the consumers are built and tested; unit tests reach the slope
paths by calling `sampleAttackGround<true>` or by passing a tilted `GroundSample` directly.

Turning it on is one edit with consequences outside this header: on a slope the radial initial
conditions a peer predicts change, so the correction wire format version moves with it, and the golden
traces are re-recorded. Per-consumer switches and a runtime console variable were considered and rejected
(peers must agree on the value; a runtime value would need the wire token to carry it).

---

## Provenance

Written with the header on 2026-10-28 (og-ground-aligned-attacks task 4). Nothing in this file depends
on any document outside this submodule.

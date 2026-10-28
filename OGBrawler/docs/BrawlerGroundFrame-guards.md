<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerGroundFrame.h` — guards

Every prohibition that governs a line of `BrawlerGroundFrame.h`. Each entry has an **opaque, stable id**.
In the header a single line `// ⛔G-nn` sits exactly where the forbidden edit would be typed.

**If this file and `BrawlerGroundFrame.h` disagree, the header is authoritative and this file is stale.**
Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check, is moved to
§R and its number is spent forever. A retired id may be **named** in prose or in a `static_assert`
message; it may never again appear as a `⛔G-nn` **tag**.

⛔ **Nothing in this file is a rationale.** Why the sample and the knob are shaped as they are lives in
`BrawlerGroundFrame-rationale.md`. A guard is a prohibition plus the consequence of ignoring it.

---

## G-01 — `kGroundAlignedAttacks` is flipped only together with the correction wire version

**Tag site:** `BrawlerGroundFrame.h`, immediately above `inline constexpr bool kGroundAlignedAttacks = false;`.

**The prohibition.** Do not change the value of `kGroundAlignedAttacks` on its own. Changing it changes
what a peer predicts on sloped ground, so the same change must raise the correction wire format version
(`kWireFormatVersion` in og-simulation's `CorrectionStateBufferCodec.h`) and re-record the golden
traces. Do not turn it into a runtime value (a console variable, a `StaticData` field) either: the wire
version cannot see a runtime value.

**The consequence.** Two builds that differ only in this constant carry the same wire version, so they
connect, and on any sloped ground they compute different radial initial conditions (the swing plane) for
the same input. Every attack started on a slope then mispredicts and is corrected, with no refusal at
login and no error anywhere; on flat ground both builds agree, so a flat test arena shows nothing.

**What breaks if it moves.** The constant is read in exactly one place, as the default template argument
of `brawlerMovementSimulation::sampleAttackGround`; every consumer receives the sample from there. A
second switch elsewhere would let a consumer follow the ground while the sample stays `kUp`, or the
reverse.

---

## §R Retired ids

**None.**

The default normal's bits are a `static_assert` in the header, not a guard (rationale §2).

---

## Provenance

G-01 was written with the header on 2026-10-28 (og-ground-aligned-attacks task 4). Nothing in this file
depends on any document outside this submodule.

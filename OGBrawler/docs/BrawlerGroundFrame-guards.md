<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerGroundFrame.h` — guards

Every prohibition that governs a line of `BrawlerGroundFrame.h`. Each entry has an **opaque, stable id**.
In the header a single line `// ⛔G-nn` sits exactly where the forbidden edit would be typed.

**If this file and `BrawlerGroundFrame.h` disagree, the header is authoritative and this file is stale.**
Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check, is moved to
§R and its number is spent forever. A retired id may be **named** in prose or in a `static_assert`
message; it may never again appear as a `⛔G-nn` **tag**.

⛔ **Nothing in this file is a rationale.** Why the sample is shaped as it is lives in
`BrawlerGroundFrame-rationale.md`. A guard is a prohibition plus the consequence of ignoring it.

The header has **no live guard** since og-ground-aligned-attacks task 14.

---

## §R Retired ids

### G-01 — RETIRED by og-ground-aligned-attacks task 14 (2026-11-02)

**Was:** "the knob is flipped only together with the correction wire version", on the header's
compile-time `bool` that switched ground-aligned attacks on (tag site: immediately above that
constant). The prohibition: never change the constant's value without raising the correction wire
format version (`kWireFormatVersion` in og-simulation's `CorrectionStateBufferCodec.h`) and
re-recording the golden traces, and never turn it into a runtime value; two builds that differed only
in it connected and mispredicted every attack started on a slope.

**Why retired.** Its subject is gone. Task 14 deleted the constant (OQ4 (a), ruled by the user
2026-11-02 after G3) together with the `bool` template parameter that `sampleAttackGround`,
`detachesFromSupport`, `writesKnockbackVelocity` and the movement `integrate` defaulted to it; their
ground-aligned arms are the only code left. There is no value left to flip, so no build can differ from
another in it. ⛔ The number 1 is spent.

The default normal's bits are a `static_assert` in the header, not a guard (rationale §2).

---

## Provenance

G-01 was written with the header on 2026-10-28 (og-ground-aligned-attacks task 4); task 10 flipped the
constant to `true`, with the correction wire version 9 -> 10; task 14 deleted it and retired G-01.
Nothing in this file depends on any document outside this submodule.

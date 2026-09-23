<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `OGBrawlerLog.h` — guards

Every prohibition that stood in the header. Each entry has an **opaque, stable id**; in the header
a single line `// ⛔G-nn` sits where the wrong edit would be typed.

**If this file and `OGBrawlerLog.h` disagree, the header is authoritative and this file is stale.**
Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time check, moves to
[§R, Retired ids](#r-retired-ids) and its number is spent forever. A retired id may be **named** in
prose or in a `static_assert` message; it may never again appear as a `⛔G-nn` **tag**.

⚠ **The id space is PER DOCUMENT.** Other headers' guards documents also have a `G-01`; a tag
carries the path of the document it resolves against. Never cite a bare `G-nn` without the header.

⛔ **Nothing in this file is a rationale.** The narrative, the derivations and the provenance live in
`OGBrawlerLog-rationale.md`.

Converted by og-netcode-v2-field-defects task 16 (2026-09-23), in the same change that added the
integrate-scope prefix. Three of the header's comments were prohibitions; the rest were rationale, or
are already enforced by the header's own `static_assert`s and need no text at all (rationale §6).

---

## G-01 — The clip check in `OGBLOG_G` is the guarantee; raise the buffer, never weaken the check

**Tag site:** `OGBrawlerLog.h`, immediately above `#define OGBLOG_G`.

**The fence, verbatim.** It stood in two places, and they are one prohibition, so they are one guard.
Above `kLineBufferBytes` (og-netcode-v2-field-defects task 7):

```
// [og-netcode-v2-field-defects task 7] ⛔ A LINE THAT DOES NOT FIT IS CLIPPED SILENTLY.
// vsnprintf truncates and returns the would-be length; nothing surfaces the loss. So the
// size of this buffer is not a margin anyone re-derives by hand: EVERY OGBLOG_G call site
// static_asserts (inside the macro, below) that the worst case its format and argument
// types can print fits here. Adding a field that could overflow fails the BUILD at that
// call site. Raise this number, never delete the assertion.
```

And above the macro:

```
// ⛔ The static_assert is the clip check: it runs at EVERY call site, against that site's
// format and argument types. If it fires, the line could be truncated — raise
// ogblog::kLineBufferBytes (or teach the checker a new conversion); never shorten the proof.
```

**What breaks if it moves.** `std::vsnprintf` truncates a line that does not fit and reports nothing.
A clipped `[Machine.*]` or `[Radial.*]` line loses its trailing fields (`endTick=`, `aimAngle=`), which
are the fields the knockback and swing analyses read, and nothing in the log shows the loss. The
`static_assert` in the macro is the only thing that makes a too-long format a **build** error at the
call site that introduced it. The tempting edit when it fires is to delete it, or to charge a
conversion less than it can print so the site compiles. Either one brings back silent clipping at every
call site at once. The correct responses are to raise `kLineBufferBytes`, or to teach
`worstCaseFormattedLengthOf` a new conversion **with its true worst case**.

⚠ **The check is only as sound as its fail-closed default.** `kUnbounded` is what the checker returns
for anything it cannot bound. The self-tests at the end of the `detail` namespace assert that result
for six shapes (`%llu`, a 64-bit argument, `%e`, a `%s` given an `int`, and too few or too many
arguments). A "fix" that returns a guess instead of `kUnbounded` fails the build there. That half
needs no guard (rationale §6).

⚠ **Since task 16 the check charges every literal `kIntegrateScopePrefixMaxBytes` (30) on top of its
own worst case**, because `emitFormatted` may insert `id=4294967295 tick=4294967295 ` into any line.
Removing that term from `fitsLineBuffer` re-admits literals whose prefixed line clips. The probe at
task 16 showed a 994-character literal firing and a 993-character literal compiling. Before task 16,
literals up to 1023 characters were admitted.

---

## G-02 — The `id= tick=` prefix is INSERTED here, after the tag and past the verbosity marker, and only inside an integrate scope. It is never authored in a format string.

**Tag site:** `OGBrawlerLog.h`, `emitFormatted`, immediately above the `if` that reads
`simulationLog::currentIntegrateScope`.

**Written, not moved.** Task 16 added this rule; no comment held it before. What it replaces is
og-netcode-v2-field-defects task 7's plumbing: a character id carried through `SimulatableBrawler`
and both sub-simulations' integration utils, with `id=%u tick=%u` typed into 23 format literals.
Task 16 removed all of that. The id and tick are now ambient log context:
`SimulationIntegrationExecutor::integrateAll` opens a `simulationLog::IntegrateScope` around each
integrate, and this block is the **one** place that reads it.

**The prohibition has four parts. Each names an edit that looks harmless.**

1. **Never author `id=%u tick=%u` (or `tick=%u`) in a format string whose line runs inside
   integrate.** The prefix is already inserted there, so an authored copy prints twice. A future
   "task 7, again" is the likely edit: someone reading a `[Machine.*]` literal, seeing no id, and
   adding one. ⚠ **This part is already violated by seven lines that predate task 16, in other
   headers:** `[Movement.cadence]`, `[Movement.teleport]`, `[Movement.hover]` and `[Movement.surface]`
   in `BrawlerMovementSimulation.h`, and `[Ringout.respawn]`, `[Ringout.spawnSlot]` and
   `[Ringout.death]` in `BrawlerRingoutSimulation.h`. Each authors its own `tick=%u`, and each
   runs inside `SimulatableBrawler::integrate`. They now print `id=<id> tick=<t> tick=<t> …`.
   Those headers belong to another initiative, so task 16 recorded the duplication and did not edit them.
   Lines that run **outside** a scope (the ring-out score system, and `OGBrawlerUECharacter.cpp`'s
   `[Ringout.score.client]`) must keep authoring their own `id=`/`tick=`, because nothing inserts one.
2. **Never move the insertion ahead of a leading `[Verbose]` or `[Warning]`.** Both UE sinks
   (`ogblogServer` and `ogblogClient` in `SimulationManagerUImpl.cpp`) choose the verbosity by
   `StartsWith(TEXT("[Verbose]"))` on the emitted string. A prefix at column 0 of
   `[Verbose][Radial.branch] …` silently promotes that line to `Log`. Those are the six
   `[Radial.branch]` lines, the highest-volume line in the tree (~267/s), which task 15 made
   Verbose-only. The `static_assert`s on `integrateScopePrefixOffset` fail the build on this edit, so
   they are the enforcement. This entry records why they exist.
3. **Never insert outside a scope, or from any other source of id.** Outside every
   `IntegrateScope` the emitted line must be byte-identical to the format's output. That covers
   `firstResimStep`, body-state capture, systems, game-thread code and a test that calls `integrate`
   directly. A line with no scope has no character, and a guessed id is worse than none.
   `OGBrawlerLog.OutsideAnyScopeTheLineIsByteIdentical` pins it.
4. **Never open a scope from og-brawler code.** The storage id belongs to og-simulation's executor.
   A scope opened inside an integrate call is nested, and it trips the no-nesting `OG_CHECK` in
   `simulationLog::IntegrateScope`. A scope opened anywhere else is worse, because nothing trips:
   around the systems executor, for example, whose lines already print their own `id=`, every
   line would silently gain a second one.

**What breaks if it moves.** Parts 1 and 3 produce lines that attribute wrongly or twice. The
knockback-revert analyses join `[Machine.transition] id=` to `[CollectInput] id=` by that token,
and a doubled or guessed id breaks the join silently. Part 2 puts task 15's reference volume back
into default-verbosity and shipped logs: 9,357 `[Radial.branch]` lines per 35 s. Nothing warns
when it happens.

---

## G-03 — One line buffer per emitted line, in `emitFormatted`, never one per call site in the macro

**Tag site:** `OGBrawlerLog.h`, immediately above `emitFormatted` (above its `format` attribute).

**The fence, verbatim** (og-netcode-v2-field-defects task 7):

```
// ONE buffer per emitted line, whatever the number of call sites in the caller. A
// per-site array in the macro would put one kLineBufferBytes array PER SITE in the
// frame of a function built with optimisation off (integrate3 has fourteen).
```

**What breaks if it moves.** The obvious simplification is to format inside `OGBLOG_G`, into a local
`char` array of `kLineBufferBytes`. That is how the macro read before task 7, with a 256-byte array.
A block-scoped array inside a `do { } while (0)` looks as if it lives for one statement. But
`DAttackMachineSimulation.h` and `DAttackRadialSimulation.h` are compiled under
`OGSIM_OPTIMIZE_OFF`, and `integrate3` has fourteen `OGBLOG_G` sites (R0 re-count at task 16: 14
sites, all inside `integrate3`). Task 7 reasoned that an unoptimised build gives each site its own
array. ⚠ **That stack cost was reasoned, not measured.** No frame size has been read off a build.
The second reason was measured: since task 16 the buffer must also hold the inserted prefix, and
`emitFormatted` is the only function that knows whether one is inserted.

---

## H. A convention that has no tag here, because the edit it forbids is typed in another file

**H-1 — a `%s` argument must be a short, fixed name.** The fence, verbatim, from above
`kMaxStringArgBytes`:

```
// ⚠ The ONE bound the check cannot derive from a type. A `%s` argument is a `const char*`,
// and its length is a runtime value; the check charges every `%s` this many bytes. That
// is a CONVENTION: pass only short fixed names (dAttackStateName, which asserts it,
// and the movement sub-sim's support names). A `%.Ns` precision is honoured if smaller.
```

The wrong edit is a new `OGBLOG_G(… %s …)` in some other header that passes a runtime string longer
than 32 bytes. That is typed at the call site, not here, so a tag on `kMaxStringArgBytes` would sit
where nobody makes that edit. Where the rule is enforced today: `DAttackMachineSimulation.h`
`static_assert`s that every `dAttackStateName` fits. The movement sub-simulation's `supportName`
lambda returns at most 14 characters, and nothing asserts that. Rationale §3 has the bound.

---

## §R Retired ids

None. This header was converted at task 16, and no id has been retired yet.

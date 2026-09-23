<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `OGBrawlerLog.h` — rationale

The narrative, the derivations and the provenance for `OGBLOG_G`, og-brawler's game-rule log macro,
and its line buffer and clip check. The header keeps no prose. The prohibitions are in
`OGBrawlerLog-guards.md`, each with a stable id; this file carries everything else.

**If this file and `OGBrawlerLog.h` disagree, the header is authoritative and this file is stale.**

⛔ **Do not put a fence in this file.** A prohibition belongs in `OGBrawlerLog-guards.md`, with an id and
a `⛔G-nn` tag at the site, or better, in a `static_assert`.

Origin: og-netcode-v2-field-defects task 7 (the 1024-byte buffer, the compile-time clip check,
`emitFormatted`), task 15 (the `[Verbose]` marker on `[Radial.branch]`), and task 16 (the
integrate-scope prefix, and this conversion).

---

## 1. What this header is

`OGBLOG_G(fmt, …)` formats one line and hands it to `ogblog::g_sink`, a process-global
`std::function` that is empty unless a host sets it. When it is empty the macro evaluates no argument
and prints nothing. The only production host is the UE adapter: `ASimulationManagerUImpl::BeginPlay`
installs `ogblogServer` or `ogblogClient` (`SimulationManagerUImpl.cpp:913` and `:972`). Both map a
leading `[Verbose]` or `[Warning]` in the emitted string to that verbosity of
`UE_LOG(LogOGBrawler, …)`, and log everything else at `Log`. They log the string **unstripped**, so the
marker is visible in the log. The tests install their own capturing sinks.

It is the sink for og-brawler's sub-simulations and systems, and it is distinct from og-simulation's
`SIMLOG_G`, whose sink routes framework messages to per-subsystem categories.

**The header comment this section replaces, verbatim, and what R0 found wrong with it:**

```
// Game-rule logging sink for the OGBrawler plugin (DAttackMachine, DAttackRadial,
// future DAttackGuard). Distinct from OGSimulation/SimulationLog.h's SIMLOG_G,
// which is reserved for the framework's tick/sync/reconciliation messages.
//
// At the UE instantiation site (ASimulationManagerUImpl::BeginPlay), the logger
// is set to route messages through UE_LOG(LogOGBrawler, ...). Toggle verbosity
// via DefaultEngine.ini under [Core.Log]:
//
//     LogOGBrawler=Warning   ; default — silence per-tick spam
//     LogOGBrawler=Verbose   ; enable while debugging attack/machine logic
//
// Prefix a message with "[Verbose]" or "[Warning]" to pick a non-default
// verbosity for that single line (mirrors SIMLOG_G's convention).
```

* ⛔ **`LogOGBrawler=Warning ; default` was false.** The category is declared
  `DECLARE_LOG_CATEGORY_EXTERN(LogOGBrawler, Log, All)` (`SimulationManagerUImpl.h:67`), so the code
  default is `Log`. `Config/DefaultEngine.ini:362` ships `LogOGBrawler=Verbose`.
* ⚠ **The user list was stale.** "Future DAttackGuard" never logged: `DAttackGuardSimulation.h` has zero
  `OGBLOG_G` sites. The callers at task 16 are the machine (14 sites), the radial (9), the projectile
  (6), movement (4), the ring-out simulation (3) and score system (1), plus `OGBrawlerUECharacter.cpp` (1).
* True as written: the `BeginPlay` install site, and the `[Verbose]` / `[Warning]` marker convention.
  `RouteOGMessage`, the `SIMLOG_G` sink, parses the same two markers.

## 2. The buffer size: 1024

`kLineBufferBytes` is 1024. Task 7 sized it from the widest worst case in the tree,
`[Warning][Movement.hover]` at 551 characters, and raised it from 256. Task 7's measurement found that the
old 256 could already clip three lines at their worst case (`[Movement.hover]` 551, `[Movement.surface]`
299, `[Radial.setInitialConditions]` 266). Since task 16 every line is also charged the 30-character
prefix (§5), so the widest charged line is 581, still under 1024.

The comment this replaces, verbatim (the prohibition in the same block is guards G-01):

```
// 1024 is sized by the widest site in the tree, `[Movement.hover]` (ten float conversions);
// the widest `[Machine.*]` / `[Radial.*]` sites are well under half of it.
```

⚠ R0: `[Movement.hover]` has **nine** float conversions and one `%u` — ten conversions, not ten
floats. The second clause still holds with the prefix: the widest in-scope sub-simulation line,
`[Radial.setInitialConditions]`, is 266 + 30 = 296.

## 3. The one bound a type cannot prove: `%s`

A `%s` argument's length is a runtime value, so the checker charges every `%s` a fixed
`kMaxStringArgBytes` (32), or a smaller `%.Ns` precision. That is a convention, not a proof; guards
§H-1 has the text and where it is enforced. `dAttackStateName` is `constexpr` so that
`DAttackMachineSimulation.h` can assert every name, the `"?"` fallback included, fits.

## 4. The per-conversion worst cases ∴D-01

The `switch` in `worstCaseFormattedLengthOf` charges each conversion the widest output its
argument **type** can produce. Each number comes from this derivation:

| conversion | argument | bound | derivation |
|---|---|---|---|
| `%d` `%i` | ≤ 32-bit integer or enum | 11 | `"-2147483648"` |
| `%u` | ≤ 32-bit | 10 | `"4294967295"` |
| `%x` `%X` | ≤ 32-bit | 8, 10 with `#` | eight hex digits, plus `0x` |
| `%c` | ≤ 32-bit | 1 | one character |
| `%f` `%F` | `float` | 1 + 39 + point + fraction | sign, every integer digit of `FLT_MAX` (39), the point, the fraction (default 6) |
| `%f` `%F` | `double` | 1 + 309 + point + fraction | as above with `DBL_MAX` (309 integer digits) |
| `%s` | `const char*` | `kMaxStringArgBytes` or a smaller precision | §3 |
| anything else | — | `kUnbounded` | length modifiers `h l ll z j t L`, `*`, unknown conversions, the end of the string |

`inf` and `nan` print shorter than any finite bound. A field width counts when it exceeds the bound.

**Why the type and not the format.** `"%.4f"` says nothing about whether it gets a `float` or a
`double`. Task 7 measured the same four-conversion format compiling with floats and firing with doubles.

The comments this section replaces, verbatim: the two trailing labels `// "-2147483648"` and
`// "4294967295"`; `// incl. h/l/ll/z/j/t/L, '*', '\0'` on the `default:`; and above the float case:

```
// Sign + every integer digit of the largest finite value (FLT_MAX has 39,
// DBL_MAX 309) + the point and the fraction. inf/nan print shorter.
```

The self-tests at the end of the `detail` namespace pin 10, 11, 45 (`%.4f` float), 40 (`%.0f`),
315 (`%.4f` double) and 32, every value hand-derived. An edit to a bound that does not also edit its
self-test fails the build.

## 5. The integrate-scope prefix — og-netcode-v2-field-defects task 16

Every `OGBLOG_G` line emitted while a character is being integrated carries that character's
storage id and the integrated tick, `id=<id> tick=<tick> `, directly after the tag:

```
[Machine.transition] id=7 tick=9 Idle -> Attacking seq=2 endTick=30
[Verbose][Radial.branch] id=7 tick=9 idle (state.curSeq invalid)
```

**Where the values come from.** `SimulationIntegrationExecutor::integrateAll` holds `(id, step)` at the
point it calls each simulatable's `integrate`. It opens a `simulationLog::IntegrateScope(id,
step.getTick())` around that call and nowhere else. `emitFormatted` reads
`simulationLog::currentIntegrateScope` and, when a scope is open, inserts the prefix. The two
lines are the whole mechanism. No sub-simulation, composite or integration utils type names the id.

**Why this replaced task 7's plumbing.** Task 7 carried the id as an off-wire member of
`SimulatableBrawler`, stamped from the UE registration seam, passed as a constructor parameter through
the machine's and the radial's integration utils, and typed into 23 format literals. It worked, and it
taught three simulation types a value the simulation must never read. The user approved moving it to
ambient log context on 2026-09-23. The 23 literals are back to their pre-task-7 text; the six
`[Radial.branch]` literals keep task 15's `[Verbose]`.

**The insertion point, `integrateScopePrefixOffset`.**

1. Skip one leading `[Verbose]` or `[Warning]`. The UE sinks read that marker at column 0 (§1), so the
   prefix must never precede it (guards G-02 part 2).
2. If what follows starts with `[`, the prefix goes after the first `]` and the single space after it.
3. Otherwise, a line with no tag, the prefix goes at that point: column 0, or right after the marker.
   A `[` whose `]` never comes is treated as no tag.

The `static_assert`s beside the function pin all of these shapes, and `OGBrawlerLog.IntegrateScopePrefixLandsAfterTheTag`
pins them at runtime through the real macro.

**The buffer reserves the widest prefix.** `kIntegrateScopePrefixMaxBytes` is the checker's own worst
case for `"id=%u tick=%u "` with an `unsigned int` and a `uint32_t`: `id=4294967295 tick=4294967295 `,
30 characters, asserted. `fitsLineBuffer` charges it to every literal, so any literal that compiles can
take the widest prefix without clipping. `OGBrawlerLog.TheWidestPrefixOnTheLongestAdmittedLiteralDoesNotClip`
emits a 993-character literal inside the widest scope and reads back 1023 characters, intact. At
runtime `emitFormatted` inserts only if the line still fits. That branch cannot fail for a literal the
check admitted; it exists so that the insertion never writes past the buffer.

**Outside every scope the line is byte-identical** to what the format prints. That is
`firstResimStep`, body-state capture, systems, game-thread code, and a test that calls `integrate`
directly (`OGBrawlerLog.OutsideAnyScopeTheLineIsByteIdentical`).

**What the scope covers that task 7 did not.** The scope wraps the whole `SimulatableBrawler::integrate`,
so every sub-simulation's lines gain the prefix: projectile (6 sites), movement (4) and ring-out
simulation (3), which task 7 had listed as out of scope. Seven of those already author their own
`tick=%u` and now print it twice (guards G-02 part 1). The ring-out score system runs outside any scope
and is unchanged.

**Thread.** The scope is `inline thread_local`. Integrate runs on the physics thread today, and a future
off-thread integrate must never read another thread's scope.

## 6. What needs no text

These comments were deleted, because code or a compile-time check already says what they said:

* `kUnbounded`'s fail-closed rule is enforced by the six fail-closed self-tests (`%llu`, a 64-bit
  argument, `%e`, a `%s` given an `int`, too few and too many arguments) and the final
  `!fitsLineBuffer("%u", TypeList<>{})`. The deleted comment, verbatim:
  ```
  // Returned for anything the check cannot bound: an unknown conversion, a length
  // modifier, a `*` width/precision, a 64-bit integer, an unsupported argument type, or
  // an argument count that does not match the conversions. FAIL CLOSED: the assertion
  // fires and the next person teaches the checker, rather than the line clipping.
  ```
* `typesOf` is declared and never defined, so a call outside `decltype` fails to link. Deleted,
  verbatim: `// Declared, never defined: used only inside decltype to capture the call site's` /
  `// argument types after the default argument promotions' decay.`
* `worstCaseFormattedLength` counts characters **excluding** the terminating NUL, which is why
  `fitsLineBuffer` uses `<`. The `"a%%b"` self-test (3) pins the count. Deleted, verbatim:
  `// The worst-case number of characters (excluding the terminating NUL) `fmt` can print` /
  `// with arguments of the listed types.`
* Labels deleted: `// Self-tests, every value hand-derived. They pin the arithmetic the macro trusts.`,
  `// Fail-closed arms: each of these must NOT be boundable.`, and task 16's own
  `// The widest integrate-scope prefix emitFormatted can insert: …` (now §5, and asserted `== 30`).

## 7. Where every comment went

| pre-conversion comment | now |
|---|---|
| orientation block (sinks, ini, markers) | §1, R0-corrected |
| buffer clip prohibition (task 7) | guards G-01 |
| "1024 is sized by `[Movement.hover]`…" | §2 |
| `%s` convention | guards §H-1; bound in §3 |
| `kUnbounded` fail-closed | §6 (enforced by self-tests) |
| `typesOf` declared-not-defined | §6 |
| per-conversion trailing labels and the float derivation | §4 ∴D-01 |
| worst-case contract (excludes NUL) | §6 |
| prefix constant label | §5 |
| "ONE buffer per emitted line…" | guards G-03 |
| self-test labels | §6 |
| "the static_assert is the clip check…" above the macro | guards G-01 |
| (new, task 16) prefix inserted, never authored | guards G-02 |

The `static_assert` message in `OGBLOG_G` used to end "See OGBrawlerLog.h."; it now names guards G-01.

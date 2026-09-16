<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerRingoutScoreSystem.h` — guards

Every prohibition that stood in the header. Each entry has an **opaque, stable id**; in
the header a single line `// ⛔G-nn` sits exactly where the fence's text used to sit, on
the statement or declaration where the wrong edit would be typed.

**If this file and `BrawlerRingoutScoreSystem.h` disagree, the header is authoritative**
**and this file is stale.** Fix this file; do not soften the header to match it.

⛔ **An id is never reused.** A guard that is deleted, or that becomes a compile-time or
run-time check, is moved to [§R, Retired ids](#r-retired-ids) and its number is spent
forever. A retired id may be **named** in prose or in an assertion message; it may never
again appear as a `⛔G-nn` **tag**.

⭐ **The join is machine-checked, in both directions**, by `tools/lint/guard_tag_lint.ps1`:
every tag resolves to an entry here, every live entry is referenced by exactly one tag, no
id is duplicated, and no retired id reappears as a tag. It is a hard gate. ⚠ It checks that
an entry EXISTS. It never checks that this text is TRUE.

⛔ **Nothing in this file is a rationale.** The law, the provenance and the derivations live
in `BrawlerRingoutScoreSystem-rationale.md`. A guard is a prohibition plus the consequence
of ignoring it, and nothing else.

⭐ **Every consequence below was MEASURED by injection on 2026-09-13**, not reasoned. The
arm, the binary it ran against and the resulting failure list are recorded in the
ring-out initiative workspace, in task 14's implementation notes.

---

## G-01 — The award is gated to the authority role, and the step cannot supply that gate

**Tag site:** `BrawlerRingoutScoreSystem.h`, on the `if (!this->m_isAuthority) return;`
that opens `postIntegrate`.

**The fence, verbatim — these are the bytes it occupied in the pre-conversion header:**

<!-- header lines 32-36 -->
```
// ══ ⛔ THE ROLE GATE, AND WHY THE STEP CANNOT SUPPLY IT ════════════════════════════════════
// `SimulationSystemsExecutor` fires these hooks on ALL THREE roles — the authority tick, a
// client's forward prediction tick, and EVERY resim replay tick (`SimulationManager.h`, grep
// `or rollback routing stops being deterministic`). An ungated award double-counts on every
// client, once per replayed tick, forever.
```

<!-- header lines 149-150 -->
```
        // ⛔ THE GATE. See the banner: a client reaches this on its forward prediction tick
        // AND on every replayed tick of every resim.
```

**What breaks if it moves.** `SimulationSystemsExecutor` fires this hook on all three
roles. Deleting the early return leaves every client awarding points on its own forward
prediction tick **and once more per replayed tick of every resim**, forever, against a
replicated value the server keeps overwriting.

**Measured:** deleting the two-line early return that the tag sits on turns exactly
one case red —
`RingoutScore.ANonAuthorityRoleAwardsNothing` (39 cases, 1 failed).

⚠ The gate must stay on `m_isAuthority` and must not be re-derived from `step`: that the
step cannot answer the question is now a `static_assert` — see **G-09** in §R below.

---

## G-02 — The role gate is defaulted CLOSED

**Tag site:** `BrawlerRingoutScoreSystem.h`, on `bool m_isAuthority = false;`.

**The fence, verbatim:**

<!-- header lines 44-47 -->
```
// ⭐ IT DEFAULTS TO FALSE, AND THAT IS THE DESIGN. A `ScoreSystem` nobody configured awards
// NOTHING. The failure mode of a forgotten wiring call is a scoreboard stuck at zero — loud,
// harmless, and repairable — rather than a client quietly inventing points that the server
// never granted and the replicated value keeps overwriting.
```

<!-- header lines 283-283 -->
```
    // ⛔ THE ROLE GATE, DEFAULTED CLOSED. See `setIsAuthority`.
```

**What breaks if it moves.** Changing the initialiser to `true` — the natural "make it
work out of the box" edit — inverts the failure mode of a forgotten wiring call from a
scoreboard stuck at zero into every client inventing points the server never granted.

⛔ **The compiler CANNOT take this one, and that was measured rather than assumed.**
`ScoreSystem` holds a `std::unordered_map`, so it is not usable in a constant expression;
the probe `constexpr ScoreSystem s; return s.getIsAuthority();` fails to compile with
MSVC `C3615: constexpr function cannot result in a constant expression`. The default is
pinned at run time instead, by `RingoutScore.SatisfiesTheSimulationSystemConcept`.

---

## G-03 — Collect the whole population BEFORE awarding a single point

**Tag site:** `BrawlerRingoutScoreSystem.h`, on the `std::vector<Row> rows;` declaration
that the one walk fills.

**The fence, verbatim:**

<!-- header lines 154-162 -->
```
        // ---- ONE WALK, AND IT IS ALSO THE SNAPSHOT ------------------------------------
        //
        // ⛔ COLLECT BEFORE AWARDING. Ruling 3 says two simultaneous deaths award two points
        // to each SURVIVOR, and that both diers are absent from each other's award. The
        // implementation that walks and awards in the same pass cannot express that: having
        // retired the first dier, it either awards the second dier a point for the first (if
        // it tests liveness before its own bookkeeping) or not (if after), and WHICH of those
        // two wrong answers it gives depends on unordered-map order. `rows` is read once,
        // before a single point is added, so there is no such dependence to have.
```

**What breaks if it moves.** An implementation that awards inside the walk has, by the
time it meets the second dier, already changed the world the first award read. Which of
two wrong answers it gives then depends on unordered-map order, which is unspecified and
machine-varying — so the same two deaths score differently on the server and on a client
that registered its characters in a different order.

**Measured:** replacing the collect-then-apply body with a single pass that awards one
point per death to everyone already seen alive turns **9 of 39 cases red**, including
`RingoutScore.TwoSimultaneousDeathsAwardTwoToEachSurvivor` and
`RingoutScore.TwoIterationOrdersProduceIdenticalScores`. This guard has the strongest
machine backstop of the seven.

---

## G-04 — The award inserts, deliberately; it does not `find`-and-skip

**Tag site:** `BrawlerRingoutScoreSystem.h`, on `this->m_scores[row.id] += deathCount;`.

**The fence, verbatim:**

<!-- header lines 225-229 -->
```
            // `operator[]`, deliberately: it inserts at 0 and then adds. The roster is seeded
            // at registration below, so in a legal session the entry already exists — but an
            // award to an id the roster has not got is a real award, not a dropped one, and a
            // `find`-and-skip here would hide a registration defect AND make the gate's own
            // RED PROBE come back green on an empty table.
```

**What breaks if it moves.** A `find`-and-skip would turn an award to an unseeded id into
a silently dropped point, hiding a registration defect — and it would make the role
gate's own red probe come back green, because an ungated client awarding into an empty
table would then change nothing observable.

⚠ **This is a SPELLING fence** — `operator[]` and `find` produce the same answer whenever
the roster is seeded — so no expression distinguishes them and no assertion can be
written. It is also the one line whose insert behaviour a UE-layer caller depends on:
`SimulationManagerUImpl.cpp`'s `pushRingoutScoresToCharacters` carries a `checkf` over
`hasScoreEntry` precisely because an insert here would rehash on the physics thread under
its game-thread walk. Changing this line means re-reading that argument.

---

## G-05 — Seeding the roster is authority-only

**Tag site:** `BrawlerRingoutScoreSystem.h`, on the `if (!this->m_isAuthority) return;`
in `onCharacterRegistered`.

**The fence, verbatim:**

<!-- header lines 245-249 -->
```
    // ⛔ AUTHORITY ONLY, like the award itself. On a client the table stays empty for the life
    // of the session — the same property `ASimulationManagerUImpl::m_authorityRegisteredIds`
    // and task 3's `m_spawnSlots` both rely on, and for the same reason: nothing on that role
    // ever writes it, so nothing on that role can read a wrong answer out of it. A client
    // learns scores from the replicated `UPROPERTY` task 5 adds, never from here.
```

**What breaks if it moves.** Ungating it gives a client a roster it has no business
holding, which is the precondition for the client reading a wrong answer out of a table
nothing on that role ever writes correctly. The property the code rests on is that on a
client the table is empty for the life of the session.

⚠ **Measured, and this is the honest half:** deleting this gate leaves all 39 cases green.
It is a prohibition with no machine backstop; the suite drives registration only through
authority rigs.

---

## G-06 — Seed with `emplace`, never with `operator[]`

**Tag site:** `BrawlerRingoutScoreSystem.h`, on `this->m_scores.emplace(id, 0u);`.

**The fence, verbatim:**

<!-- header lines 261-261 -->
```
        this->m_scores.emplace(id, 0u);   // emplace, not [] — a re-registration keeps the score
```

**What breaks if it moves.** `m_scores[id] = 0u` would reset a re-registering character's
score to zero. `emplace` leaves an existing entry alone, which is what makes a
re-registration different from a rejoin (a rejoin arrives after
`onCharacterUnregistered` has erased the entry, so it legitimately starts at zero —
`RingoutScore.UnregisterDropsTheEntryAndARejoinStartsAtZero` pins that half).

⚠ A SPELLING fence again: both forms compile, both leave a zero behind on a fresh id, and
they differ only on a path no case in the suite drives.

---

## G-07 — `onCharacterUnregistered` is UNGATED, and that is deliberate

**Tag site:** `BrawlerRingoutScoreSystem.h`, immediately above `onCharacterUnregistered`.

**The fence, verbatim:**

<!-- header lines 267-270 -->
```
    // ⛔ UNGATED, and that is deliberate rather than an oversight — the same call task 3 made
    // for `SpawnSlotAllocator::release`. `notifyCharacterUnregistered` fires on BOTH roles;
    // nothing on the client role ever inserted, so the erase is a no-op there BY CONSTRUCTION.
    // A role gate here would be a branch no test could ever make fail.
```

**What breaks if it moves.** Nothing observable — and that is exactly why the prohibition
is worth an entry. `notifyCharacterUnregistered` fires on both roles; nothing on the
client role ever inserted, so the erase is a no-op there BY CONSTRUCTION. A reviewer who
notices the asymmetry with G-05 and "fixes" it by adding a gate here writes a branch that
is correct, unnecessary, and permanently untested.

**Measured, and it is the claim the fence rests on:** injecting a role gate into
`onCharacterUnregistered` leaves **all 39 cases green** (372 assertions). The fence's own
sentence — *"A role gate here would be a branch no test could ever make fail"* — is
therefore true as written. ⛔ **This is the one guard of the seven with no machine
backstop in either direction**, and the only protection it has is that deleting the
declaration takes its tag with it and orphans this entry.

---

## §R Retired ids

Spent forever. `guard_tag_lint.ps1` CHECK 4 rejects any tag naming one.

⚠ **CHECK 4 does not check that the assertion which replaced a fence still exists.** Each
entry below therefore names its id inside the assertion's own message text, so deleting
the assertion deletes the only surviving mention of the id.

### G-08 — RETIRED, converted to a `static_assert`

**The fence, verbatim, as it stood in the header:**

<!-- header lines 19-24 -->
```
// ══ ⛔ SCORES ARE NOT SIMULATION STATE, AND THAT IS THE ENTIRE POINT OF RULING 1'S SPLIT ═══
// `m_scores` is authority-side bookkeeping that lives BESIDE the simulation. It is in no
// composite, it has no `SerializableFields` specialization, it is never serialized, never
// corrected and never rewound — `simulatableBrawler::State` measures 335 B with this system
// compiled in and 335 B without it, which `RingoutScore.TheAwardCostsTheCompositeNothing`
// re-quotes from task 2 unchanged.
```

<!-- header lines 286-289 -->
```
    // character id -> points. ⛔ NOT SIM STATE. In no composite, on no wire, never corrected,
    // never rewound — see the banner. `uint32_t` at one point per death is unreachable
    // overflow; it is sized to match the replicated property task 5 adds rather than to a
    // bound anyone has to defend.
```

**Replaced by**, at the bottom of the header:

```cpp
static_assert(!Serializable<brawlerRingout::ScoreSystem>,
    "brawlerRingout::ScoreSystem must never gain a SerializableFields specialization. Was guard "
    "G-08 (SCORES ARE NOT SIMULATION STATE), retired into this assertion by ringout task 14. A "
    ...);
```

**The poison that proves it is not vacuous.** Adding a `SerializableFields` specialization
for `ScoreSystem` in a probe TU fires the assertion (`C2338`), and the same TU without the
specialization compiles clean. The two probe translation units live in the ring-out
initiative workspace, not in this repository.

⚠ **R0 — one number in the retired text was re-measured and one was corrected.** The 335 B
figure still stands: `RingoutScore.TheAwardCostsTheCompositeNothing` passed on
2026-09-13 against the shipped tree, and it carries the number as a `static_assert`. The
sizing sentence in the second block is corrected in
`BrawlerRingoutScoreSystem-rationale.md` §9 — see **R0-2** there.

### G-09 — RETIRED, converted to a `static_assert`

**The fence, verbatim, as it stood in the header:**

<!-- header lines 38-42 -->
```
// ⛔ AND `SimulationTimeStep` CANNOT TELL YOU THE ROLE. `StepKind` is
// `{Normal, Stall, Skip, HardResync}` — CLOCK behaviour, not role — and `getIsResimulating()`
// is FALSE on BOTH the authority tick and a client's forward prediction tick, so it separates
// replay from live, never server from client. The flag has to be plumbed in explicitly, which
// is what `setIsAuthority` exists for.
```

**Replaced by**, at the bottom of the header:

```cpp
static_assert(!brawlerRingout::detail::StepExposesARole<SimulationTimeStep>, ...);
static_assert(brawlerRingout::detail::StepExposesARole<brawlerRingout::detail::RoleBearingStep>, ...);   // vacuity control
```

**Why this one converts at all.** The prohibition is an **absence-of-member** fence about
a type in *another* repository, and those are the cheapest conversions available: one
assertion here turns a silent staleness into a build break for the person editing
`SimulationTimeContext.h`. If the step ever *can* answer the question, the sentence this
fence was protecting has become false and someone must be told.

**The poison that proves it is not vacuous.** A shadow copy of the whole `OGSimulation`
directory, placed first on the include path with `bool getIsAuthority() const` added to
`SimulationTimeStep`, fires the assertion (`C2338`), while the no-op control TU compiles
clean against that same poisoned shadow. The shadow harness lives in the ring-out
initiative workspace, not in this repository.

⛔ **What it does NOT cover, measured.** The other half of the retired sentence —
*"`StepKind` is `{Normal, Stall, Skip, HardResync}` — CLOCK behaviour, not role"* — has no
honest assertion. Pinning the four enumerator values fires when an enumerator is
**inserted** and stays silent when one is **appended**, which is the likelier edit; an
assertion equivalent to one half of its own fence is worse than none, so it was rejected
rather than shipped. The four enumerators were re-read from
`SimulationTimeContext.h` on 2026-09-13 and are unchanged.

### G-10 — RETIRED, converted to an `OG_CHECK`

**The fence, verbatim, as it stood in the header:**

<!-- header lines 199-217 -->
```
        // ---- THE ORDERED APPLY -----------------------------------------------------------
        //
        // ⛔ SORT BY ID. `SystemsExecutor.h` item 81 states this as a LIBRARY CONTRACT, not a
        // style note: character order within a sweep is unordered-map order — unspecified, and
        // machine-varying with registration history — so a system whose per-character effects
        // are non-commutative "MUST impose its own order over the view (e.g. sort by id)" or it
        // "manufactures a permanent, input-independent server/client divergence that no amount
        // of gate or verdict work downstream can repair".
        //
        // ⚠ BE HONEST ABOUT WHAT THIS BUYS TODAY: the effect below is `+=` on a per-id counter,
        // which is COMMUTATIVE, so the walk order cannot change the resulting scores while the
        // law stays this shape. The sort is discharging the contract IN ADVANCE, and the
        // things that would make it load-bearing are all one edit away — a per-tick cap on how
        // many points exist, a first-blood bonus to whoever sorts first, any tie-break, or an
        // award whose amount READS the score table while the table is being written.
        // ⛔ DO NOT DELETE IT AS DEAD WEIGHT. It is deleted correctly only together with a
        // proof that the award is still commutative, and that proof is not in the type system.
        // `RingoutScore.TwoIterationOrdersProduceIdenticalScores` pins the composition of this
        // sort and the snapshot above; the snapshot is the half that bites today.
```

**Replaced by**, immediately below the `std::sort` in `postIntegrate`:

```cpp
OG_CHECK(std::is_sorted(rows.begin(), rows.end(),
             [](const Row& a, const Row& b) { return a.id < b.id; }),
    "... the std::sort above has been deleted or moved. Was guard G-10 (SORT BY ID) ...");
```

⭐⭐ **This is the one conversion in the file that is strictly stronger than any tag or
comment could be, and the numbers say so.**

| arm | `[BrawlerRingout]` result |
|---|---|
| shipped (sort + `OG_CHECK`) | **39 cases, 372 assertions, all passed** |
| `std::sort` deleted, no check (task 4's red probe, re-run 2026-09-13) | ⛔ **all 39 still green** |
| `std::sort` deleted, `OG_CHECK` present | ⭐ **1 failed** — `RingoutScore.TwoIterationOrdersProduceIdenticalScores` |
| `OG_CHECK(false)` vacuity control | 9 failed — the macro is live in this target, not compiled out |

The case that fires is the one whose acceptance criterion always claimed to buy the sort
and which task 4 proved could not: its `INFO` lines print the two walk orders
(`0, 1, 2, 3` and `3, 2, 1, 0`) and its `REQUIRE(orderA != orderB)` vacuity guard is what
keeps the check from going quiet. Both orders are permutations of the same four ids and
the guard forces them to differ, so at least one rig must present a non-sorted walk —
which is the property the `OG_CHECK` needs and the reason it cannot silently stop biting.

⛔ **What it still does not cover.** Deleting the sort **together with** this check
restores the green suite. Nothing in the rule catches that: §2.1 suppresses the guard
entry and the tag once a check exists, so there is no live id left to orphan. See the
verdict section of task 14's implementation notes — this is the pilot's one concrete
recommendation for a rule amendment.

⚠ **The retired text's honesty note is retained above and is still true:** the award is
`+=` on a per-id counter, which is commutative, so the sort changes no score *today*. The
check does not make the sort load-bearing; it makes its **deletion** loud.

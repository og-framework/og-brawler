<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerRingoutScoreSystem.h` — rationale

The law, the provenance and the derivations. The **prohibitions** are in
`BrawlerRingoutScoreSystem-guards.md`; nothing here is a fence.

⭐ **Extraction is a MOVE.** Every fenced block below is the pre-conversion header's own
bytes, sliced by line range (the `<!-- header lines A-B -->` marker above each block says
which), carried across unchanged. **R0 corrections are never applied to the carried text**
— each one sits in an `R0-nn` note immediately BEFORE the block it corrects, so a false
sentence cannot be read without its correction attached.

⚠ **This file is not the source of truth for any VALUE.** Every number below was
re-measured on 2026-09-13; where it lives in code, the code wins.

---

## 1. What this file is, and the template it follows

<!-- header lines 4-11 -->
```
// brawlerRingout::ScoreSystem — the ring-out point award [ringout task 4].
//
// THE SHAPE IS BORROWED, NOT INVENTED. This file is modelled on
// `BrawlerHitRoutingSystem.h`, the shipped `SimulationSystem` in this tree: the four hooks,
// the `RequiredSimulatables` alias, and the collect-into-a-vector / sort-by-id / then-apply
// body of `postIntegrate`. Everything that differs from that template is called out where it
// differs, and there are exactly two differences that matter — the ROLE GATE and the
// SNAPSHOT.
```

✅ **R0 — verified 2026-09-13.** `brawlerHitRouting::System` does have an empty
`preIntegrate` and a `postIntegrate` that collects into a vector and sorts by id
(`BrawlerHitRoutingSystem.h`, the `std::sort` on `ordered`).

<!-- header lines 58-58 -->
```
// Layer: OGBrawler core — engine-agnostic, game-specific. NO UE or Godot symbols.
```

<!-- header lines 60-61 -->
```
// NAMESPACE NOTE: OGSim primitives (`SimulatableList`, `StorageView`, `SimulationTimeStep`)
// are named UNQUALIFIED — the whole OGSim core lives in the GLOBAL namespace (lead D12).
```

## 2. The law

<!-- header lines 13-17 -->
```
// ══ THE LAW ════════════════════════════════════════════════════════════════════════════════
// On every tick, one point to every fighter that is ALIVE, per fighter that DIED this tick.
// Two simultaneous deaths therefore award TWO points to each survivor (user ruling 3), and
// each dier is absent from the other's award. The dier gains nothing and loses nothing
// (rulings 2 and 7: there is no penalty for dying).
```

## 3. Why the score lives outside simulation state

The prohibition half of this is now a `static_assert` — see **G-08** in the guards doc.
What remains is the argument for the split, which no assertion carries.

<!-- header lines 26-30 -->
```
// ⛔ A SCORE IS A MONOTONIC SIDE EFFECT AND A ROLLBACK IS NOT. Every other quantity in this
// project is RECOMPUTED on a resim; a point is not. Put it in the composite and a correction
// either double-counts it or silently un-awards it, and nothing in the wire fence or the test
// suite would notice. That is why the DETECTION (`brawlerRingout::DerivedState::diedThisTick`)
// is in the sim and replays identically, and the AWARD is here, outside it, on one role.
```

## 4. The role gate, and why the derivation is in the UE layer

⭐ **R0-5 — this fence is not orphaned here; a WORKING COPY already stands at the site**
**where the edit it forbids is actually typed.** `SimulationManagerUImpl.cpp`, directly
above `m_systemsExec.get<brawlerRingout::ScoreSystem>().setIsAuthority(worldIsAuthority);`,
carries the same two prohibitions in its own words: *"⛔ worldIsAuthority, NOT
HasAuthority(). bReplicates = false makes Role always authority on this actor"* and
*"⛔ ABOVE THE ROLE BRANCH, AND THAT PLACEMENT IS THE PROOF THE WINDOW IS EMPTY"*. Because
the forbidden edit is typed **there**, `⛔G-nn` at the declaration here could not reach the
editor, so this block is rationale and carries no tag. Verified verbatim on 2026-09-13.

<!-- header lines 49-56 -->
```
// ⭐ WHY THE GATE MECHANISM IS HERE AND THE ROLE DERIVATION IS IN THE UE LAYER. This is the
// same split task 3 made for `SpawnSlotAllocator`, and for the same reason:
// `SimulationManagerUImpl.cpp` is a UE module file the low-level-test target CANNOT REACH, so
// a gate implemented there is a gate no test can drive. The bool is DERIVED in the UE layer
// (`worldNetMode != NM_Client`, the world-level test — ⛔ NEVER `HasAuthority()`, which
// `bReplicates = false` makes permanently true on that actor) and APPLIED here, where
// `RingoutScore.ANonAuthorityRoleAwardsNothing` can drive both a prediction tick and a resim
// tick against it.
```

✅ **R0 — the three load-bearing facts in the "why a setter" argument below were each
re-verified 2026-09-13:** `m_systemsExec` is a plain value member (`BrawlerSystemsExec`
`m_systemsExec;`, no `std::optional`); `SimulationSystemsExecutor() = default;` does still
exist beside the `std::piecewise_construct` constructor, so that constructor closes no
gap; and the `setIsAuthority` call site does sit above the role branch that emplaces
`m_manager`, with nothing between them.

⚠ **Three inbound citations point a reader at `setIsAuthority` for this argument** —
`SimulationManagerUImpl.h` (*"See brawlerRingout::ScoreSystem::setIsAuthority for why a
setter and not the executor's piecewise_construct ctor"*),
`BrawlerRingoutScoreSystemTest.cpp` and `BrawlerRingoutScorePushContractTest.cpp`. After
this conversion those citations land on a bare declaration, and the reader's next hop is
the two-line docs pointer at the top of the header, then this section. **None of the three
was edited** — the header is this task's only source-file scope.

<!-- header lines 91-110 -->
```
    // ---- THE ROLE GATE ---------------------------------------------------------------
    //
    // Supplied by the composition root once, before the manager that fires these hooks
    // exists. ⛔ THE ARGUMENT MUST BE THE WORLD-LEVEL TEST (`GetNetMode() != NM_Client`),
    // never `HasAuthority()`.
    //
    // ⭐ WHY A SETTER RATHER THAN THE EXECUTOR'S `std::piecewise_construct` CTOR, which
    // exists for exactly this (`SystemsExecutor.h`). Three reasons, in order of weight:
    //   1. `ASimulationManagerUImpl::m_systemsExec` is a plain VALUE member, constructed with
    //      the actor. The piecewise ctor would force it to `std::optional` and make the
    //      fourth peer NULLABLE for the first time — `m_manager`'s `Params` take it by
    //      reference, so every fire path would gain a dereference that can be wrong.
    //   2. The piecewise ctor does not actually close the gap it looks like it closes: the
    //      executor's DEFAULT ctor still exists and still compiles, so forgetting to pass the
    //      flag stays a silent runtime fact either way. What makes it safe is the default
    //      being `false`, and that is a property of this class, not of the ctor used.
    //   3. The window in which the flag is unset is PROVABLY EMPTY, not merely short: these
    //      hooks are reached only through `SimulationManager`, `m_manager` is emplaced in
    //      `BeginPlay`, and the call site sits ABOVE the role branch that emplaces it. There
    //      is no tick, and no registration, between construction and the set.
```

## 5. `RequiredSimulatables`

<!-- header lines 86-88 -->
```
    // The subset of the game's simulatables this system observes. The executor projects the
    // full storage down to exactly this list before calling each hook. UNQUALIFIED
    // `SimulatableList` — global namespace (D12).
```

## 6. The readouts

<!-- header lines 114-117 -->
```
    // ---- READOUT ---------------------------------------------------------------------
    //
    // `scoreOf` answers 0 for an id with no entry, which is the same answer it gives for a
    // registered id that has not scored. Task 5 replicates this value onto the character.
```

⚠ **R0-2 (the second half of this claim is corrected in §9).**

<!-- header lines 124-125 -->
```
    // Whether this id has a ROSTER ENTRY at all — distinct from "scores zero". The
    // unregister contract is about this, not about the value.
```

⭐ **`hasScoreEntry`'s most important consumer is not named in the carried text.**
`SimulationManagerUImpl.cpp`'s `pushRingoutScoresToCharacters` wraps every game-thread
read of the table in `checkf(scoreSystem.hasScoreEntry(id), ...)`, because the award's
`operator[]` (see **G-04**) could otherwise insert — and rehash — on the physics thread
under that walk. The unregister contract is the *other* consumer, not the only one.

## 7. `preIntegrate` does nothing

<!-- header lines 135-137 -->
```
    // preIntegrate — no work. The award reads `diedThisTick`, which the ring-out sub-sim
    // produces DURING integrate, so there is nothing to do before it. Present to satisfy the
    // four-hook `SimulationSystem` concept, exactly as `brawlerHitRouting::System`'s is.
```

## 8. The walk, the early-out, and the population

⛔ **R0-1 — THE POPULATION SENTENCE BELOW IS WRONG AS WRITTEN, AND IT IS THE SAME CLAIM**
**THIS INITIATIVE HAS ALREADY HAD TO CORRECT ONCE (finding F22).**
`ASimulationManagerUImpl::kPreDietCharacterCap` is `4`, but it is **advisory, not**
**enforced**: the only code that reads it logs `UE_LOG(LogOGNet, Warning, ...)` at
`[PreDietCap]` and **lets the session continue** (`SimulationManagerUImpl.cpp`,
`if (registered > kPreDietCharacterCap)`). Read the carried sentence as *"the population
this is INTENDED to run against is 4"*; a session with more characters is over-cap,
noisy, and still running, and this walk still has to be correct for it. The absence of a
`reserve` is unaffected either way — `StorageView` genuinely exposes no count (verified
2026-09-13).

<!-- header lines 164-165 -->
```
        // No `reserve`: neither the view nor the storage exposes a cheap count, and the
        // populations this runs against are `kPreDietCharacterCap` = 4 characters.
```

<!-- header lines 194-195 -->
```
        // ⭐ THE EARLY-OUT, AND IT IS THE COMMON CASE. Nobody dies on the overwhelming
        // majority of ticks; that tick costs one vector walk and no sort.
```

## 9. The score table, its key and its width ∴D-02

⛔ **R0-2 — THE SIZING CLAIM BELOW IS FALSE.** The replicated property task 5 added is
`int32 RingoutScore` — **signed** (`OGBrawlerUECharacter.h`, `UPROPERTY(ReplicatedUsing =
OnRep_RingoutScore)`), because a `UPROPERTY` cannot be `uint32_t`. The **width** matches
at 32 bits; the **signedness** does not, and the push converts with an explicit
`static_cast<int32>`. `OGBrawlerUECharacter.h` already carries that correction next to the
property, under its own `⚠ SIGNEDNESS` note; this is the other copy of the pair.

<!-- header lines 286-289 -->
```
    // character id -> points. ⛔ NOT SIM STATE. In no composite, on no wire, never corrected,
    // never rewound — see the banner. `uint32_t` at one point per death is unreachable
    // overflow; it is sized to match the replicated property task 5 adds rather than to a
    // bound anyone has to defend.
```

## 10. Why the roster is seeded at registration

<!-- header lines 243-243 -->
```
    // onCharacterRegistered — seed this character's roster entry at zero.
```

⛔ **R0-3 — THE MECHANISM NAMED BELOW IS THE WRONG ONE.** The scoreboard does draw a row
of `0` rather than a missing row (`BrawlerScoreboardVisualization.h`, verified
2026-09-13) — but it gets that zero from the replicated `GetRingoutScore()` on the
character, which is `0` by default whether or not the roster was ever seeded
(`ScoreboardVisualizationUImpl.cpp` never touches `ScoreSystem`). **Seeding is not what
makes the zero row appear.** What the seed actually buys is named in the same paragraph
and is real: the unregister contract needs an entry to drop for a character that died to
nobody, and `SimulationManagerUImpl.cpp`'s `checkf` over `hasScoreEntry` needs every
authority-registered id present before the push walks.

<!-- header lines 251-254 -->
```
    // WHY SEED AT ALL rather than let the first award insert. A fighter who has never scored
    // must be DISTINGUISHABLE from a fighter who is not in the match — the scoreboard (task 6)
    // draws a row of `0`, not a missing row — and the unregister contract below needs
    // something to drop for a character that died to nobody.
```

## 11. Unregister, and the score dying with the character

<!-- header lines 264-265 -->
```
    // onCharacterUnregistered — forget the id, or the table grows for the life of the session
    // as players join and leave.
```

<!-- header lines 272-274 -->
```
    // ⚠ THE SCORE DIES WITH THE CHARACTER. There is no match-level ledger keyed by player —
    // ruling 4 puts no match end in this slice, and inventing a persistent one here would be
    // inventing a scope the user has not asked for.
```

## 12. Why the concept is asserted at the definition

<!-- header lines 295-299 -->
```
// The concept, checked at the definition rather than only where the executor instantiates it:
// `SimulationSystemsExecutor`'s requires-clause would catch a non-conforming system too, but it
// would catch it in `SimulationManagerUImpl.h` — a UE module file the low-level-test target
// cannot compile — so the diagnostic would appear only in an Editor build. Here it appears in
// every translation unit that includes this header, including the LLT ones.
```

That argument is now inside the assertion's own message, where it cannot go stale
separately from the check it explains.

## 13. The award log line

<!-- header lines 233-235 -->
```
        // ⚠ `deaths` and `pointsEach` are the SAME NUMBER and are printed separately on
        // purpose: they are the same only because the award is one point per death, and the
        // day a bonus or a cap changes that, the line that has to show it already exists.
```

## 14. `alive` is a LEVEL, not an edge — ruling 2 ∴D-01

⭐ This section is the target of the `∴D-01` tag on the `alive` expression in
`postIntegrate`. It is the one derivation in this file that a reader must have in front of
them to change that line correctly: the identifiers alone (`isDead`, `getState`) do not
say why the edge (`diedThisTick`) is *not* also tested, nor why being airborne is
irrelevant.

✅ **R0 — verified 2026-09-13.** `brawlerRingout::integrate` does set `kFlagDead` and raise
`diedThisTick` on the same tick (`BrawlerRingoutSimulation.h`, the kill-plane branch sets
`state.flags | kFlagDead` and `derived.diedThisTick = true` in the same block), which is
what makes one test exclude both a fresh dier and a fighter still waiting out an earlier
death.

<!-- header lines 180-186 -->
```
                // ⭐ ONE TEST RETIRES BOTH EXCLUSIONS. `integrate` sets `kFlagDead` on the
                // same tick it raises `diedThisTick`, so a dier reads NOT ALIVE here and is
                // excluded from its own award and from its twin's by the same expression that
                // excludes a fighter still waiting out an EARLIER death. "Alive at the death
                // tick" (ruling 2) is a LEVEL, and this is it; being airborne is not part of
                // it, which is the whole reason ruling 2 needs nothing from the movement sim's
                // support state.
```

## 15. What this header used to look like, and what was dropped

The pre-conversion file was **305 lines, 177 of them comment (58 %)** — 176 once the
SPDX line is set aside. The converted file is **185 lines** and, measured rather than
claimed, it contains **zero explanatory comment lines**: 1 SPDX line, 1 docs pointer,
**9 tags** (7 `⛔G` + 2 `∴D`), one `} // namespace brawlerRingout`, and nothing else that
is a comment. **28 of its lines are assertion or log message text** — English that is now
code, cannot be skimmed past, and cannot go stale separately from the check it explains.
⚠ The comment-only measure does not see those 28 lines; report them explicitly or the
reduction looks larger than it is.

**Deleted from the header and carried nowhere**, as the rule requires in its §6: the
`---- HOOKS ----` section banner; the `// The award system.` and
`// postIntegrate — the award, authority only.` orientation one-liners; the
`// brawlerRingout::DerivedState::diedThisTick — the EDGE` and
`// !isDead(State) — the LEVEL` trailing labels on the `Row` fields; the trailing
`// SimulatableList` / `// StorageView` labels on the include lines; and 14 bare `//`
separator lines.

⚠ **Four other section banners — `---- THE ROLE GATE ----`, `---- READOUT ----`,
`---- ONE WALK, AND IT IS ALSO THE SNAPSHOT ----` and `---- THE ORDERED APPLY ----` —
are gone from the header but DO survive inside the verbatim quotations here and in the
guards doc**, because they are part of the bytes the fences occupied and extraction is a
move, not a rewrite. They are quoted text, not structure: nothing in either document is
organised by them.

**Carried-verbatim coverage, measured by task 14's generator script against the frozen
pre-conversion copy:** 176 comment lines in the header (SPDX set aside), **159 carried
byte-for-byte into one of the two docs**, 17 not carried — and 14 of those 17 are bare
`//` separators. The three substantive deletions are the `---- HOOKS ----` banner and the
two orientation one-liners named above.

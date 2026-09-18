<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: UEngine::GetSmallFont -- the engine's own fixed-size debug font; owned by the host engine, not by this repository, and must not resolve here -->

# The ring-out scoreboard's layout — rationale

Companion to `OGBrawler/BrawlerScoreboardVisualization.h`, which holds the board's whole
model: a row type, an ordering, a layout, two console clamps, three inks and the countdown
arithmetic. **The header carries the code; this file carries the reasoning.**

## ⚠ Sibling document, and which one wins

`Source/OGBrawlerUnreal/docs/ScoreboardDisplay-rationale.md` covers the **UE layer** of the
same board — the three console variables, the gather, and the draw. The two documents
overlap on purpose and the overlap cannot be removed: this tier travels with the
`og-brawler` submodule, which ships standalone, so a reader who has checked out only
`og-brawler` must be able to reach the pure header's reasoning without a path that leaves
this repository.

⛔ **Where the two overlap, THIS file is authoritative for decisions the pure header makes**
(the row model, the ordering, the clamps, the layout, the inks, the countdown), and the
sibling is authoritative for decisions the UE layer makes (the console variables, the join,
the canvas calls). ⛔ **A correction to a shared claim is owed to both.** Nothing checks
this; it is a maintenance instrument and it only works if it is used.

---

## 1. Why the presentation is pure code and not UE code

`Source/OGBrawlerTests` links `{ Core, OGSimulation, OGBrawler }` and **not**
`OGBrawlerUnreal`, so anything written against a canvas is untestable by construction.

The claims that decide whether this panel is *useful* rather than merely correct — that two
peers see the same player in the same row, that the board sits flush against the right edge
at every scale, and that a console value outside its range is pulled to the nearer end
rather than multiplying the whole layout into a non-number — are therefore decided **here**,
where a Catch2 case can reach them, and the UE layer is left holding only the gather and the
draw calls.

⛔ **The clamps live here, not at the console.** The UE accessor calls
`clampScoreboardScale` and `clampScoreboardBackgroundAlpha`; it does not re-implement
either. That is the existing discipline in `InputHistoryVisualizationUImpl.h`, and it is the
only reason the clamps are testable at all.

### 1.1 The shape is borrowed, not invented

This file is the mirror of `BrawlerInputHistoryVisualizationPanel.h` — the user's ruling of
2026-09-11 is that the scoreboard uses the same tech and the same approach as the
input-history pane. The constant block, the `clamp*` functions, the `*Layout` POD, the
`scaled*Layout` and `placed*Layout` builders and the row-geometry helpers all answer to that
file, name for name.

⛔ **The number of differences is deliberately not stated.** A sentence naming a total is
falsified by the next difference somebody adds, and `ScoreboardLayout::maxRows` — not
console-driven, while the panel's `visibleRows` **is** clamped by `clampPanelVisibleRows` —
is one such difference already. Section 3 below states the ones that have a reason worth
writing down; it does not claim to be exhaustive.

---

## 2. The row, left to right

```
| swatch |  score | status |
```

The swatch says **who**, the score says **how many**, and the status says whether this
fighter is currently out and for how much longer. A field that changed a row's meaning
without being drawn — a dead flag with no column, say — would make a frozen score
unexplainable to the person watching.

⛔ **Three columns is also the row's whole identity, and the two may not come apart.**
`Scoreboard.TheRowIsThreeOrderedColumnsAndNoneOfThemLeavesIt` pins it.

### 2.1 ⭐⭐ Column one was the raw character id until task 10, and the swap is narrower than it looks

`characterId` did not leave the row and did not stop being load-bearing: it is still the
**join key** the UE gather assembles each row on, and it is still the **sort key**
`scoreboardRowPrecedes` orders by, so two peers continue to show the same player in the same
row.

⛔ **Only the displayed column changed.** Nothing on this board is ordered by, keyed on, or
compared by colour, and a future edit that sorted by the swatch would put the two peers back
out of step the moment one of them assigned tints in a different order.

The reason for the swap is that `42353` is a number a player has no way to connect to a body
on the screen, and the tint is one they already read at a glance, because it is the colour of
the fighter they are watching.

### 2.2 ⛔ The swatch is three plain floats here and an engine colour only at the draw site

This file has no engine type in it and gains none. `ScoreboardInk` is the linear-`0..1` RGB
triple the row ink already used, the row carries one of them, and the UE layer builds its
`FLinearColor` from the three floats where it is about to make a canvas call. That is the
same boundary the rest of the header keeps.

---

## 3. Where this board deliberately differs from the input-history pane

Each of these is a place where copying the precedent would have been a defect. ⚠ This list
is not claimed to be complete — see 1.1.

### 3.1 `viewportWidth`, which the input pane never took

The input-history pane is flush **left** (`kPanelLeftEdgeX` is `0.f`) and x = 0 needs no
measurement, so `placedPanelLayout` never asked what the viewport was wide. A right-flush
panel's origin is `viewportWidth - width`, so it cannot be a constant and it cannot be
guessed: hardcoding 1920 puts the board **640 px inboard of the right edge** on a 2560-wide
ultrawide, and **640 px off-screen** at 1280.

> ⚠ **Measured, not reasoned** — through the shipped `placedScoreboardLayout`: at 2560 the
> hardcoded board's right edge lands at 1920 against a correct 2560, and at 1280 it lands at
> 1920 against a correct 1280. ⛔ **The direction is the point:** a hardcoded width LARGER
> than the viewport is what pushes the board off-screen, so the off-screen case is the
> SMALLER viewport.

⛔ **Do not drop the parameter and do not hardcode a width.**

**Why right**, measured rather than chosen (`current_state.md`, "Screen real estate" <!-- lint-anchor-ignore: initiative workspace archive path, carried verbatim; it lives outside every scan root by design -->): the
input-history pane is flush left and vertically centred, and the frame meter is horizontally
centred near the bottom. Right-flush collides with neither at the ruled defaults, and reads
as the input pane's mirror.

`Scoreboard.ItClearsBothShippedSurfacesAtTheRuledDefaults` states the margins as numbers and
pins the scales at which each clearance is lost.

#### ⚠ One configuration does collide, and it is not hypothetical

A **full eight-row** board above scale ≈ 3.67 at 720p overlaps the frame meter in both axes.
Five, six and seven rows never collide anywhere in the clamp range.

> **Re-measured 2026-09-16** against the two meter edges that case asserts (right edge 1123,
> top edge 624.2 at 720p), sweeping every row count 0..8 against every 0.01 step of the
> 0.25..4 clamp range through the shipped `placedScoreboardLayout` and `scoreboardHeight`:
> only an eight-row board collides at all, and only from scale 3.67 upward. The corner the
> suite asserts — eight rows at scale 4 — puts the board's bottom edge at 648 against a meter
> top of 624.2.

That case is **reported by the test and accepted rather than clamped**, and the reason is
**not** that the configuration is unreachable — see 8.1. A session with more than four
characters is already degraded by the netcode's own warning on the same registration path, so
a viz overlap in that configuration is consistent with it rather than a new defect, and a
clamp bought here would hide the overlap without making the session playable.

### 3.2 The rows are ordered here, and it is not cosmetic

`SystemsExecutor.h` item 81 states it as a **library contract**: character order within a
sweep is unordered-map order — unspecified, and machine-varying with registration history.
The UE layer gathers one row per character by walking exactly such a sweep, so an unsorted
scoreboard shows two players the same scores in **different rows**, and which player is on
top depends on who joined first on that machine.

⛔ **The ordering is folded in here, in pure code, where it is testable** — never at the draw
site. This is the same move `brawlerRingout::ScoreSystem`'s `postIntegrate` makes with the
same justification, and unlike that one it is **load-bearing today** rather than discharged
in advance: the award is `+=` on a per-id counter and commutes, but "which row is drawn at
the top" does not commute with anything.

### 3.3 The centred height is the drawn one, not the reserved one

`placedPanelLayout` centres `panelWindowHeight` — the whole 24-row window, full or not —
because the input pane's rows *arrive while you watch*, and a top edge that crept upward with
each new row would be unreadable.

A scoreboard's row count is the player count: it changes at a join or a leave and is
otherwise constant for the match, so there is nothing to creep, and reserving eight rows'
height for a two-player match would draw the board visibly above centre for the whole
session.

⛔ **That is why `placedScoreboardLayout` takes `rowCount` at all** — an **occupancy**, where
the panel's third parameter is a **window size** it reserves whole.

⚠ `placedPanelLayout` takes `(base, scale, visibleRows, viewportHeight)`, so it *does* take a
row count in the same position — the difference is what the number means, not that it is
there. The parameter the panel genuinely has no use for is `viewportWidth`, which is 3.1.

`scoreboardWindowHeight` still exists and still means what the panel's does — the height a
**full** board reserves — and its job is the worst-case footprint question: does the board
still fit, and still clear the frame meter, when the last player joins.

### 3.4 `maxRows` is not console-driven

Unlike the input pane's `visibleRows` it has no clamp. It lives on the layout rather than
being read as a constant so that `scoreboardDrawnRowCount` and `scoreboardWindowHeight` take
it from the same place a future console variable would write it.

---

## 4. `ScoreboardInk` — the colour triple, and the fence that is a compiler error

Linear `0..1` RGB, the same convention the frame meter's styles use. Alpha is not here: the
row ink is opaque, the swatch is opaque, and the only alpha on this panel is the backdrop's,
which is a console value with its own clamp.

⭐ **It is declared above `ScoreboardRow` because task 10 gave the row one.** It used to sit
with the three ink constants near the foot of the header; the constants and the selector are
still there and still say what they always did. Only the type moved, and only because a
`ScoreboardRow` member cannot name a type declared after it.

⛔ **One triple type, not two.** A second three-float struct for the brawler tint would be
the same three floats under a second name, and every conversion between them would be a place
to transpose a channel.

### 4.1 ⭐ The "no alpha" fence is promoted from prose to the compiler

The `static_assert` on `sizeof(ScoreboardInk)` at the declaration is the machine backstop.
Measured 2026-09-14: adding a fourth float to the struct left **every**
`[BrawlerRingoutScoreboard]` case then in the suite green — 18 of them — so until that line
the prohibition had no machine backstop of any kind.

It fires on a fourth float **whatever it is named**, and stays silent on a rename of an
existing channel, which is not this prohibition. The reasoning it enforces is in 2.2 and 5.4.

---

## 5. `ScoreboardRow` — a plain value, and deliberately not a sim type

⛔ **This header includes no ring-out header, and that is the point rather than an
oversight.** `brawlerRingout::State` carries a flags **byte** and an **absolute** respawn
tick; what a scoreboard row needs is a bool and a **countdown**, and the subtraction that
turns one into the other needs the current tick — a thing the simulation knows and this
header does not.

The UE gather does that subtraction once, at the gather, and hands the plain fields down.
`Scoreboard.ARowIsFivePlainFieldsAndZeroIsARealScore` pins the row's shape and its defaults,
so the field list is machine-checked rather than described by a count in a sentence.

The cost of the alternative is a layout header that cannot be unit-tested without standing up
a simulation.

⭐ **Amended by task 6b:** the subtraction's *arithmetic* (and its zero clamp) moved down to
`scoreboardTicksUntilRespawn` at the foot of the header, so that a Catch2 case can reach it.
The two ticks it takes are still the simulation's to supply and the call is still made
exactly once, at the gather — nothing about the paragraph above changed except where the
three characters of arithmetic are typed.

### 5.1 `characterId`

It matches its source exactly: it is the `unsigned int` every `StorageView` sweep hands out,
and the same `unsigned int` `AOGBrawlerUECharacter::GetSimCharacterId` returns at the gather.

### 5.2 `score` — and the mismatch the gather absorbs

⚠ `score` does **not** match its source exactly. It is the `uint32_t`
`brawlerRingout::ScoreSystem`'s `scoreOf` returns — but what task 5 replicates, and what the
board actually reads, is `AOGBrawlerUECharacter::GetRingoutScore`, which returns `int32`;
`gatherScoreboardRows` narrows it and floors it at zero. Only the id matches its source
exactly.

⚠ **Zero is a real score, not an absence.** `brawlerRingout::ScoreSystem`'s
`onCharacterRegistered` seeds every fighter's entry at zero precisely so a fighter who has
never scored is *distinguishable* from a fighter who is not in the match: this draws a row of
`0`.

### 5.3 `isDead` and `ticksUntilRespawn`

`isDead` is `brawlerRingout::kFlagDead` — out, awaiting respawn. **The level, not the edge.**

`ticksUntilRespawn` is the ticks remaining until the respawn, and **meaningful only while
`isDead`**. It mirrors `brawlerRingout::State`'s `respawnAtTick`, which is likewise
meaningful only while the dead bit is set and is left at its last value once cleared (see
`docs/BrawlerRingoutSimulation-rationale.md`) — so a renderer that drew this for a living
fighter would draw a stale countdown. That is why the status column is gated on `isDead` and
not on this being non-zero.

### 5.4 `swatch` — the fighter's own tint

⭐⭐ It is what column one **draws**, while `characterId` stays what the row is **joined and
sorted** on.

It is the same colour that fighter's mesh is wearing, not a colour this board picks: the UE
gather copies `AOGBrawlerUECharacter::BrawlerColor`'s three channels in.

⛔ **That source is already on every peer.** `BrawlerColor` is assigned server-side from the
character file's `kBrawlerPalette` and replicated with a plain `DOREPLIFETIME`, deliberately
**not** `COND_OwnerOnly` — the comment at its registration gives the reason in one line,
*"every client must see every brawler's colour"* — so a client's board draws real tints
rather than a column of white. It travels exactly like the score does, which is why this
column works on a client for the same reason column two does.

⚠ The couch-co-op argument for declining `COND_OwnerOnly` sits at `RingoutScore`'s
registration immediately below `BrawlerColor`'s, and a separate couch-co-op argument — for
keying the palette per brawler rather than per connection — sits at `kBrawlerPalette`.
Neither is the one-line comment quoted above.

⚠ **The default is white, and it is the right default rather than an arbitrary one.**
`BrawlerColor` itself defaults to white, so a brawler that has not been possessed yet, or
whose colour has not replicated in yet, draws the swatch its **mesh** is currently wearing.
The board and the world agree even in that window.

⛔ **No alpha.** The swatch is opaque; see 4 and 4.1. `BrawlerColor` carries an alpha channel
that the material uses and this column deliberately drops, because a half-transparent swatch
over a bright scene is a different colour to the eye than the same swatch over a dark one —
and "which fighter is this" must not depend on what happens to be behind the board.

---

## 6. `scoreboardRowSwatch` — the mapping, and why it ignores `isDead`

This is the whole of the swatch's policy, and it is one line so that the policy is stated in
a place a Catch2 case can reach rather than being an emergent property of the draw site. The
draw method calls this and names no colour of its own.

⛔⛔ **What the swatch does while a fighter is counting down: nothing.** It is the fighter's
full tint on a dead row and on a live row alike, byte for byte. That is a **choice**, and
here is the argument for it.

* The swatch's only job is **identity**, and a dead row is exactly when identity is most
  needed: a player who has just been rung out is looking at this board to find their own row
  and read how long they are out for. A column that changed appearance at the moment it is
  being read hardest is the wrong column to carry status in.
* **Status is already carried twice**, by two channels that are not the identity channel: the
  row's *text* ink switches to `kScoreboardDeadRowInk` (dimmer and warmer), and column three
  appears and reads a countdown. A third cue bought nothing that those two do not already
  say, and it would have been the only one to cost identity.
* ⛔ **Any "dim while dead" rule is a many-to-one map on the one axis that must stay
  one-to-one.** `kBrawlerPalette` is hand-picked so that neighbouring tints stay
  distinguishable for a colour-blind reader and against the level's grey; pulling every dead
  swatch toward a common colour compresses exactly that separation, and two fighters can be
  dead at once. Dimming would trade the property this column exists for against a cue the row
  already has.

  This is measured rather than asserted:
  `Scoreboard.TheSwatchIsBoundToItsOwnRowAndSurvivesTheOrderingFold` builds a full eight-row
  board on which every fighter is dead, and shows that the mildest imaginable dim — a
  half-blend toward the dead ink — costs exactly half the minimum pairwise separation, while
  the shipped rule preserves it exactly, because it is the identity.
* It also keeps the board honest about what it is showing: the fighter's **body** does not
  change colour when it dies — it respawns wearing the same tint — so a swatch that did would
  be showing something the world does not.

⛔ **So this function must not grow a `row.isDead` branch.** If a future task wants a
dead-state cue in column one, the honest form is a **second mark** (an outline, a strike)
drawn beside the tint, never a transform applied to it.

---

## 7. The ordering — and why it is `stable_sort`

`scoreboardRowPrecedes` is a strict weak ordering on the id alone. It is named rather than
written as a lambda at the sort so a test can assert the ordering **relation** directly, not
only its effect on one vector.

⭐ **`stable_sort`, not `sort`.** Registration makes ids unique, so on any legal input the two
agree exactly. But `scoreboardRowPrecedes` orders on the id **alone**, so two rows carrying
the same id are **tied**, and `std::sort` leaves tied elements in an **unspecified** relative
order — the very machine-varying nondeterminism this call exists to remove. An ordering that
is deterministic only while its input happens to be duplicate-free is not an ordering; it is
a coincidence. The populations here are `kScoreboardMaxRows` rows, so the two algorithms cost
the same.

### 7.1 ⚠ Be honest about what this suite can and cannot show

Measured on this tree on 2026-09-13: MSVC's `std::sort` falls back to **insertion sort** at
or below 32 elements, and insertion sort is stable, so the two produce byte-identical output
at N = 4, 8, 16 and 32 and first diverge at **N = 33**. This board's cap is 8.

⛔ **So at every size the board can reach, nothing distinguishes them** — and task 6's red
probe (`stable_sort` swapped for `sort`) accordingly stayed **green** against the thirteen
cases that existed when it first ran, every one of them at N ≤ 8.

⭐ **The finding then produced the test.**
`Scoreboard.TheStableSortIsAGuardThisSuiteCanOnlyDiscriminateAboveItsOwnCap` drives the
shipped ordering at N = 33 as well, purely to pin the divergence point, and on the re-run the
probe bit there — one failed assertion, and only that one.

⛔ **Read the verdict precisely:** the guard is now covered **above** the cap and is still
undiscriminated **at** it, which is the honest description of what it buys. It earns its
place against a cap that rises past 32, a library that moves its threshold, or a different
toolchain — all edits nowhere near this file.

### 7.2 The value-returning form

`orderedScoreboardRows` is what the UE gather calls: it takes the unordered vector **by
value** and hands back the ordered one, so there is no way to reach a draw site holding rows
that were never sorted.

---

## 8. `kScoreboardMaxRows` — a screen bound, not a population bound

⛔ **It is deliberately not 4.**

`ASimulationManagerUImpl::kPreDietCharacterCap` is **4**, and it is an **advisory** cap, not
an enforced one. Its own fence (grep `PreDietCap`) reads *"WARNING, not Log, and not an
assert: an over-cap session still RUNS - report, do not crash"*, so a fifth character
registers today, and `brawlerRingout::ScoreSystem`'s `onCharacterRegistered` gives it a
roster row without consulting the spawn table.

That 4 is **already** mirrored in this directory as `brawlerRingout::kMaxSpawnPoints`, and in
the packet-budget test besides. Another mirror here would be a draw cap that silently
**hides** a fighter — not on the day the wire diet lifts the cap, but in any over-cap
session, which can happen **now** — and a hidden fighter on a scoreboard is worse than a
board that runs off the screen, because nothing about it looks wrong.

So it is set from what the **screen** can hold instead: 8 rows at 18 px, at the maximum scale
of 4, is 576 px — still inside a 720p viewport's height, which is the shortest this project
draws to. It is also twice the advisory cap, so lifting that cap to 8 needs no edit here at
all. `Scoreboard.TheRowCapIsAScreenBoundAndAFullBoardFitsThe720pViewport` asserts that
arithmetic rather than leaving it in this paragraph.

### 8.1 ⛔ "Over-cap is unreachable" is a claim, and it is false

Stated separately because the claim is easy to assume and nothing in the tree enforces it:
nothing in `Source/OGBrawlerUnreal` rejects a
join (`ApproveLogin`, `PreLogin`, `MaxPlayers`, `GameSession` → zero hits) <!-- lint-anchor-ignore: these four names are asserted ABSENT - the sentence exists to report zero hits, so a resolving name would falsify it -->, the cap fence
logs and proceeds, and the score roster seeds an entry for every authority-registered
character. **Eight rows can therefore exist**, which is what makes 3.1's accepted overlap an
accepted overlap rather than a dead branch.

---

## 9. The scale and alpha ranges, and the clamps

The board is sized by **one** multiplier so that a tweak is a console line, not a rebuild.
The range is the input pane's, unchanged: the two panels are tuned side by side and a
different span for each would make "scale 2" mean two different things on one screen.

The rows carry their own contrast, so the shipped board adds no backdrop at all. ⛔ **At zero
the rectangle is skipped**, not drawn invisibly — an invisible rectangle is still a canvas
call on every frame.

A request outside either range is **clamped to the nearer end, never rejected** — the same
contract `clampPanelScale` offers, and for the same reason: a rejected value would leave the
console echoing a number that nothing uses.

### 9.1 ⛔ The first test is negated on purpose

A console float can arrive as a **non-number**, and under IEEE-754 NaN fails both `>= min`
and `> max`, so the negated form is what lands it on the minimum instead of letting it
through to multiply every geometric field in the layout into NaN — a board that draws
nowhere, from a value the console accepted.

### 9.2 ⚠⚠ And here is the measured fact the shipped precedent does not record

Taken on this tree on 2026-09-13, and re-confirmed 2026-09-16. **This target compiles with
`/fp:fast`** — 52 of the test target's compiler response files carry it — under which the
compiler is licensed to assume no NaN exists and rewrites `x < c` into the negated form.
Measured directly: for a `volatile`-sourced NaN, `n < 0.25f` evaluates **true** here, which
IEEE-754 says it must not.

⛔ **So on this build both spellings land NaN on the minimum, and a red probe against the
negation stays green.** Task 6 ran that probe and reported it as a finding rather than
claiming a bite it did not get.

The negation is kept anyway, and is not decoration: it is a **portability** guard. `/fp:fast`
is a per-module setting, so the day a module that includes this header is compiled
`/fp:precise` — or a non-MSVC toolchain builds it — the non-negated spelling silently starts
returning NaN and the negated one does not.
`Scoreboard.TheNaNClampIsCorrectHereForAReasonTheToolchainSupplies` pins the toolchain fact
itself, so it goes red the day that flag changes and the negation becomes load-bearing again.

⛔ **Do not "simplify" either clamp to the non-negated form.** The reason it currently looks
equivalent is a compiler flag, not the language.

---

## 10. `ScoreboardLayout` — the fields, and why each one is shaped as it is

Offsets are from the board origin. A text column's `...RightX` is its **right** edge, because
a right-aligned number is the only way a column of scores stays a column — and on this board
that matters twice over, since the whole panel is right-aligned too.

| field | why |
|---|---|
| `originX`, `originY` | Written by `placedScoreboardLayout`: `originX` depends on the viewport width and the scale, `originY` on the viewport height, the scale and the row count. ⛔ Neither origin is a chosen number, and unlike the input pane neither is a constant either — see 3.1 and 3.3. |
| `swatchX` | Column one's **left** edge, for the reason the id text had one: it puts every row's swatch against the same vertical rule the backdrop's left edge draws, so the column reads as a column. <!-- lint-anchor-ignore: `idX` is a RETIRED field name - this row exists to record the rename, so it must not resolve -->⚠ Renamed from `idX` — column one no longer draws an id, and a field still called `idX` would be a name a reader would have to disbelieve. |
| `swatchWidth` | Wide enough to read as a colour rather than as a dot at the minimum scale (0.25 takes it to 7.5 px), and narrow enough that the three columns keep their ordering: `swatchX` plus `swatchWidth` is 36, well left of `scoreRightX` at 78. |
| `swatchInsetY` | Inset from the **top and bottom** of the row, so the swatch is a band inside its row rather than a block that touches its neighbours. Two adjacent swatches with no gap read as one two-tone rectangle, which is precisely the confusion this column exists to remove. |
| `scoreRightX` | Column two, a **right** edge. Scores reach two and three digits mid-match, and a left-aligned number column jitters horizontally every time one does. |
| `statusRightX` | Column three — the respawn countdown while `isDead`, and nothing at all while alive. A **right** edge, for the same reason as the score. |
| `textOffsetY` | A row's text, measured down from the row's own top edge. |
| `textScale` | See 10.1. |
| `maxRows` | See 3.4. |

⛔ **The swatch's height is not a field** — it is `rowHeight` minus twice `swatchInsetY`,
derived by `scoreboardSwatchRect`. Storing it would let a scale move the row height and the
swatch height by different factors.

### 10.1 ⛔ `textScale` is how the one factor reaches the glyphs

The board draws with a fixed-size font (`UEngine::GetSmallFont`), so a scale reaching only
the geometry would give a bigger box holding the same tiny glyphs. It is set in one place.

⛔ **And the measure must take it too.** A right-aligned column's left edge is its right edge
minus the measured text width, so a text-measure call that omits this scale measures the
unscaled glyphs and the column drifts further out of alignment the larger the board gets —
invisible at the shipped default, which is the only scale most runs use.

### 10.2 `scaledScoreboardLayout` — scaling decides size only

Every positional number is multiplied by **one** factor, with the text scale set from that
same factor in the same statement list, so the two cannot be given different values. The
origins are deliberately left alone: where the board sits is *placement*, and that is decided
after this, from the width and the height this produced.

⚠ **`maxRows` is not scaled.** It is a count of rows, not a length; scaling it would change
how many fighters the board can show when the user changes how big it is.

---

## 11. `scoreboardSwatchRect` — column one's rectangle, in absolute screen pixels

**Why it is a function in this header and not four expressions at the draw site.** Columns
two and three are single text calls that need one x and one y; column one needs **four**
numbers, one of which is derived rather than stored, and every one of them has to come out of
the same layout. Written at the draw site it would be the only geometry in a
`Source/OGBrawlerUnreal` file — the exact thing finding F26 says to move, since no mechanical
gate in this tree reaches that layer. Here a Catch2 case reads it directly.

The rectangle's y is inset from the row's **own** top edge, so the band moves with its row and
with nothing else; `scoreboardRowTopY` is the single place a row's y is decided.

### 11.1 ⚠ No clamp on the height, and that is a decision of the same kind as 12.1

`rowHeight` and `swatchInsetY` are **both** multiplied by the same factor in
`scaledScoreboardLayout`, so their difference scales linearly and its **sign** is
scale-invariant: **no console value can flip it.**

A non-positive height is therefore reachable only by **authoring** one — either by editing
the two base constants into disagreement, or by handing this function a layout a caller
assembled by hand, which the suite itself does. ⚠ **The second path needs no constant
touched.** Measured: `ScoreboardLayout` is a mutable aggregate, so setting `swatchInsetY` to
12 on an otherwise default layout yields a swatch height of **-6**.

⭐ **The load-bearing half holds, and is measured** (2026-09-16): the sign is
scale-invariant, so no console value can flip it — that -6 becomes -24 at scale 4 and never
crosses zero.

And a zero-floor there would turn that authoring error into a swatch of **zero height** — a
column that silently disappears, which is the failure mode this whole column exists to
remove. `Scoreboard.TheSwatchIsABandInsideItsOwnRowAtEveryScale` asserts the base relation
and the scale-invariance instead, so an edit that broke it goes red rather than drawing
nothing.

---

## 12. Placement — right-flush, centred, and the one factor that must already be applied

`scoreboardHeight` is the height the board actually **draws**. Reading `rowHeight` off the
layout is what makes this the *scaled* height whenever the layout is a scaled one — which is
the property the centring depends on.

`scoreboardWindowHeight` is the height a **full** board reserves — every row the cap allows,
occupied or not. Nothing in the shipped placement centres this (see 3.3); it answers the
worst-case footprint question instead.

⛔ **The width passed to `scoreboardRightFlushOriginX` must already be scaled**, for exactly
the reason the centring spells out: a right edge computed from an unscaled width is exact at
scale 1 and wrong at every other, and it is wrong by the amount the board overhangs the
screen.

⛔ **The height passed to `scoreboardCenteredOriginY` must already be scaled.** Centring an
unscaled height is exact at scale 1 and off-centre at every other, which is invisible if only
the default is run.

⭐ **`placedScoreboardLayout` is the one way to get a drawable layout**: scale, then place what
the scale produced. The order is not the caller's to get wrong, because the caller never sees
the two steps.

### 12.1 ⚠ No clamp on `originX`, and that is a decision rather than an omission

At a large scale on a narrow viewport the board is wider than the screen and this returns a
**negative** origin, so the board runs off the **left** while its right edge stays exactly on
the right edge of the screen.

Pulling it back to 0 would silently break the one property the panel is defined by — *"the
right edge sits at `viewportWidth`"* — and would hide an unusable scale behind a board that
merely looked cramped.
`Scoreboard.TheBoardOverhangsRatherThanUnflushingWhenItCannotFit` pins the scale at which
that starts, at each probed viewport, rather than leaving it to be found in play.

---

## 13. The ink — three colours, and not one of them keyed on an enumerator

⛔ **That is a constraint, not an accident.** `palette_legend_lint.ps1` is scoped to the frame
meter's three enum-keyed palettes — `provenanceCellStyleOf`, `machineCellStyleOf` and
`delayVerdictStyleOf` in `BrawlerInputHistoryVisualizationBars.h` — each of which must keep a
legend table in its rationale doc in step with its switch arms. A fourth such palette would
need its own legend table **and** its own lint arm, in the same change, or it becomes exactly
the stale table that lint exists to prevent.

The board has no enumerated state to colour. It has **one boolean** — out, or not — so
`scoreboardRowInk` is a two-armed function of a `bool`, there is no switch, there are no
enumerator names for a table to list, and the lint's scope does not reach it.

⛔ **If a future task gives this board an enum-keyed colour, it owes the table and the lint
arm.**

### 13.1 ⭐⭐ And task 10's swatch did not become one

Stated explicitly rather than left to be inferred, because it is the first colour on this
board that is not a constant.

`scoreboardRowSwatch` is a per-character **runtime tint**: it returns a value that was carried
in on the row, assigned at possession time from `kBrawlerPalette` over in the UE character
file, and this header neither names an entry of that palette nor selects between entries.

The lint's subject is *a function that switches on an enumerator*, because that is the shape
whose arms can silently fall out of step with a legend table listing the enumerator names. A
function with no arms at all has nothing to list: a legend for this column would have to
enumerate the palette, which lives in a different module, is indexed by a possession counter
rather than by any named state, and is therefore exactly the table that would go stale the day
someone appends another colour.

⛔ **The rule above is unchanged and still binds**: the day this board keys a colour on an
**enumerator**, it owes the table and the lint arm in the same change.

### 13.2 The three constants

`kScoreboardLiveRowInk` is a fighter who is in play. `kScoreboardDeadRowInk` is a fighter who
is out, waiting on the countdown in column three — dimmer and warmer, so the row is still
readable (a score you cannot read is worse than a score that looks the same as everyone
else's) while being obviously not one of the live ones. `kScoreboardBackdropInk` is the
backdrop, drawn only when the alpha is non-zero.

⛔ **`scoreboardRowInk` is keyed on a `bool`.** That is what keeps the board outside
`palette_legend_lint.ps1`'s scope, and it is a property of the **shape** of the function, not
of its current contents.

---

## 14. The countdown

`scoreboardRowDrawsCountdown` answers whether column three has anything to say. Gated on the
**level**, never on the countdown being non-zero: `ticksUntilRespawn` is meaningful only while
`isDead`, so a living fighter's copy is stale by construction (5.3).

`scoreboardTicksUntilRespawn` is the countdown, from two absolute ticks — added by task 6b.

**Why it is here and not at the gather, where it was first written.** The gather is a
`Source/OGBrawlerUnreal` translation unit and this suite does not link that module, so the one
piece of real arithmetic in the whole UE layer sat where nothing could probe it. It is three
characters wide and carries a live hazard, which is a bad combination to leave untested.

⛔ **Clamped at zero, not allowed to wrap, and the hazard is not hypothetical.** Both ticks are
`uint32_t` and **the display tick moves backwards**: a hard resync rewinds the client
prediction clock, so the tick a board is drawing at can legitimately be later than a
`respawnAtTick` it has not reached yet. Unguarded, the difference then reads about four
billion ticks — a **ten-digit** number in a two-digit column, on exactly the frames a desync
investigation is looking at.

⚠ **It also covers an entirely ordinary frame**, which is the easier one to forget: the
respawn tick has arrived but the sub-simulation has not yet run the step that clears
`kFlagDead`, so the flag is still set and the difference is zero or negative. The board reads
0 for that frame rather than a wrapped number.

⛔ **Not gated on `isDead` here.** This answers an arithmetic question; whether the answer is
*meaningful* is `scoreboardRowDrawsCountdown`'s question, asked against the flag, and folding
the two together would give a caller one predicate that quietly means two things.

`Scoreboard.TheRespawnCountdownClampsAtZeroRatherThanWrapping` pins both halves.

---

## 15. Reading order

1. This document, for anything that reads as a choice rather than a consequence.
2. `OGBrawler/BrawlerScoreboardVisualization.h` — the code.
3. `Source/OGBrawlerUnreal/docs/ScoreboardDisplay-rationale.md` — the UE layer: the three
   console variables, the join, and the draw. ⚠ Not present in a standalone `og-brawler`
   checkout, which is why this document does not depend on it.

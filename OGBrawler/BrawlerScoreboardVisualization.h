#pragma once
// SPDX-License-Identifier: BUSL-1.1

// How the RING-OUT SCOREBOARD is laid out. Everything a renderer needs to decide before
// it knows what a canvas is [ringout task 6].
//
//   * Pure. A row model and arithmetic. No engine type, no canvas, no font -- a position
//     is a float and a colour is three of them.
//   * ⭐ THE SHAPE IS BORROWED, NOT INVENTED. This file is the MIRROR of
//     `BrawlerInputHistoryVisualizationPanel.h` -- the user's ruling of 2026-09-11 is that
//     the scoreboard uses the same tech and the same approach as the input-history pane.
//     Constant block, `clamp*`, `*Layout` POD, `scaled*Layout`, `placed*Layout`, and the
//     row-geometry helpers all answer to that file, name for name. Everything that differs
//     is called out where it differs, and there are exactly THREE differences that matter:
//     the VIEWPORT WIDTH parameter, the ORDERING, and the height that is CENTRED.
//
// ---------------------------------------------------------------------------
// WHY PRESENTATION IS PURE CODE AND NOT UE CODE.
//
// Source/OGBrawlerTests links { Core, OGSimulation, OGBrawler } and NOT OGBrawlerUnreal,
// so anything written against a canvas is untestable by construction. The claims that
// decide whether this panel is USEFUL rather than merely correct -- that two peers see the
// same player in the same row, that the board sits flush against the right edge at every
// scale, and that a console value outside its range is pulled to the nearer end rather
// than multiplying the whole layout into a non-number -- are therefore decided HERE, where
// a Catch2 case can reach them, and the UE layer (task 6b) is left holding only the gather
// and the draw calls.
//
// ⛔ THE CLAMPS LIVE HERE, NOT AT THE CONSOLE. The UE accessor calls `clampScoreboardScale`
// and `clampScoreboardBackgroundAlpha`; it does not re-implement either. That is the
// existing discipline in `InputHistoryVisualizationUImpl.h`, and it is the only reason the
// clamps are testable at all.
//
// ---------------------------------------------------------------------------
// THE ROW, LEFT TO RIGHT -- EXACTLY THREE COLUMNS.
//
//   | swatch |  score | status |
//
// The swatch says WHO, the score says HOW MANY, and the status says whether this fighter
// is currently out and for how much longer. A field that changed a row's meaning without
// being drawn -- a dead flag with no column, say -- would make a frozen score
// unexplainable to the person watching.
// ⛔ THREE COLUMNS IS ALSO THE ROW'S WHOLE IDENTITY, and the two may not come apart.
//
// ⭐⭐ COLUMN ONE WAS THE RAW CHARACTER ID UNTIL TASK 10, AND THE SWAP IS NARROWER THAN IT
// LOOKS. `characterId` did not leave the row and did not stop being load-bearing: it is
// still the JOIN KEY the UE gather assembles each row on, and it is still the SORT KEY
// `scoreboardRowPrecedes` orders by, so two peers continue to show the same player in the
// same row. ⛔ ONLY THE DISPLAYED COLUMN CHANGED. Nothing on this board is ordered by,
// keyed on, or compared by colour, and a future edit that sorted by the swatch would put
// the two peers back out of step the moment one of them assigned tints in a different
// order. The reason for the swap is that `42353` is a number a player has no way to
// connect to a body on the screen, and the tint is one they already read at a glance
// because it is the colour of the fighter they are watching.
//
// ⛔ THE SWATCH IS THREE PLAIN FLOATS HERE AND AN ENGINE COLOUR ONLY AT THE DRAW SITE.
// This file has no engine type in it and gains none: `ScoreboardInk` is the linear-0..1
// RGB triple the row ink already used, the row carries one of them, and the UE layer
// builds its `FLinearColor` from the three floats where it is about to make a canvas call.
// That is the same boundary the rest of this header keeps.
//
// ---------------------------------------------------------------------------
// ⭐⭐ DIFFERENCE 1 OF 3 -- `viewportWidth`, WHICH THE INPUT PANEL NEVER TOOK.
//
// The input-history pane is FLUSH LEFT (`kPanelLeftEdgeX = 0.f`) and x = 0 needs no
// measurement, so `placedPanelLayout` never asked what the viewport was wide. A right-flush
// panel's origin is `viewportWidth - width`, so it cannot be a constant and it cannot be
// guessed: hardcoding 1920 puts the board 640 px off-screen on an ultrawide and 640 px
// inboard at 1280. ⛔ DO NOT DROP THE PARAMETER AND DO NOT HARDCODE A WIDTH.
//
// WHY RIGHT, measured rather than chosen (`current_state.md`, "Screen real estate"):
//   * input-history pane -- flush LEFT, vertically centred.
//   * frame meter        -- horizontally CENTRED, near the BOTTOM.
// Right-flush collides with neither at the ruled defaults, and reads as the input pane's
// mirror. `Scoreboard.ItClearsBothShippedSurfacesAtTheRuledDefaults` states the margins as
// numbers and pins the scales at which each clearance is lost.
// ⚠ ONE CONFIGURATION DOES COLLIDE AND IT IS NOT HYPOTHETICAL: a FULL eight-row board
// above scale ~3.67 at 720p overlaps the frame meter in both axes. Five, six and seven
// rows never collide anywhere in the clamp range -- measured, not assumed. That case is
// reported by the same test and ACCEPTED rather than clamped; the reason is written out
// at the site, and it is NOT that the configuration is unreachable.
//
// ---------------------------------------------------------------------------
// ⭐⭐ DIFFERENCE 2 OF 3 -- THE ROWS ARE ORDERED HERE, AND IT IS NOT COSMETIC.
//
// `SystemsExecutor.h` item 81 states it as a LIBRARY CONTRACT: character order within a
// sweep is unordered-map order -- unspecified, and machine-varying with registration
// history. The UE layer gathers one row per character by walking exactly such a sweep, so
// an unsorted scoreboard shows two players the same scores in DIFFERENT ROWS, and which
// player is on top depends on who joined first on that machine.
//
// ⛔ THE ORDERING IS FOLDED IN HERE, IN PURE CODE, WHERE IT IS TESTABLE -- never at the
// draw site. This is the same move `brawlerRingout::ScoreSystem::postIntegrate` makes with
// the same justification, and unlike that one it is LOAD-BEARING TODAY rather than
// discharged in advance: the award is `+=` on a per-id counter and commutes, but "which
// row is drawn at the top" does not commute with anything.
//
// ---------------------------------------------------------------------------
// ⭐⭐ DIFFERENCE 3 OF 3 -- THE CENTRED HEIGHT IS THE DRAWN ONE, NOT THE RESERVED ONE.
//
// `placedPanelLayout` centres `panelWindowHeight` -- the whole 24-row window, full or not --
// because the input pane's rows ARRIVE while you watch, and a top edge that crept upward
// with each new row would be unreadable. A scoreboard's row count is the player count: it
// changes at a join or a leave and is otherwise constant for the match, so there is nothing
// to creep, and reserving eight rows' height for a two-player match would draw the board
// visibly above centre for the whole session.
// ⛔ THAT IS WHY `placedScoreboardLayout` TAKES `rowCount` AT ALL. It is the only parameter
// in the signature that the input panel's equivalent has no use for.
// `scoreboardWindowHeight` still exists and still means what the panel's does -- the height
// a FULL board reserves -- and its job is the worst-case footprint question: does the board
// still fit, and still clear the frame meter, when the last player joins.

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace brawlerScoreboardVisualization
{

// ---------------------------------------------------------------------------
// THE COLOUR TRIPLE. Linear 0..1 RGB, the same convention the frame meter's styles use.
// Alpha is not here: the row ink is opaque, the swatch is opaque, and the only alpha on
// this panel is the backdrop's, which is a console value with its own clamp further down.
//
// ⭐ DECLARED HERE, ABOVE THE ROW, BECAUSE TASK 10 GAVE THE ROW ONE. It used to sit with
// the three ink constants near the foot of this file; the constants and the selector are
// still there and still say what they always did. Only the type moved, and only because a
// `ScoreboardRow` member cannot name a type declared after it.
// ⛔ ONE TRIPLE TYPE, NOT TWO. A second three-float struct for the brawler tint would be
// the same three floats under a second name, and every conversion between them would be a
// place to transpose a channel.
// ---------------------------------------------------------------------------
struct ScoreboardInk
{
    float r = 1.f;
    float g = 1.f;
    float b = 1.f;
};

// ---------------------------------------------------------------------------
// ONE ROW OF THE BOARD -- A PLAIN VALUE, AND DELIBERATELY NOT A SIM TYPE.
//
// ⛔ THIS HEADER INCLUDES NO RING-OUT HEADER, and that is the point rather than an
// oversight. `brawlerRingout::State` carries a flags BYTE and an ABSOLUTE respawn tick;
// what a scoreboard row needs is a bool and a COUNTDOWN, and the subtraction that turns one
// into the other needs the current tick, which is a thing the simulation knows and this
// header does not. Task 6b does that subtraction once, at the gather, and hands these four
// plain fields down. The cost of the alternative is a layout header that cannot be
// unit-tested without standing up a simulation.
// ⭐ AMENDED BY TASK 6b: the subtraction's ARITHMETIC (and its zero clamp) moved down to
// `scoreboardTicksUntilRespawn` at the foot of this file, so that a Catch2 case can reach
// it. The two TICKS it takes are still the simulation's to supply and the CALL is still
// made exactly once, at the gather -- nothing about the paragraph above changed except
// where the three characters of arithmetic are typed.
//
// `characterId` and `score` are typed to match their sources exactly: the id is the
// `unsigned int` every `StorageView` sweep hands out, and the score is the `uint32_t`
// `brawlerRingout::ScoreSystem::scoreOf` returns and task 5 replicates.
// ---------------------------------------------------------------------------
struct ScoreboardRow
{
    unsigned int characterId = 0u;

    // ⚠ ZERO IS A REAL SCORE, NOT AN ABSENCE. `ScoreSystem::onCharacterRegistered` seeds
    // every fighter's entry at zero precisely so a fighter who has never scored is
    // DISTINGUISHABLE from a fighter who is not in the match: this draws a row of `0`.
    uint32_t score = 0u;

    // `brawlerRingout::kFlagDead` -- out, awaiting respawn. The LEVEL, not the edge.
    bool isDead = false;

    // Ticks remaining until the respawn, and MEANINGFUL ONLY WHILE `isDead`. It mirrors
    // `brawlerRingout::State::respawnAtTick`, which is likewise "meaningful only while the
    // dead bit is set, left at its last value once cleared" -- so a renderer that drew this
    // for a living fighter would draw a stale countdown, which is why the status column is
    // gated on `isDead` and not on this being non-zero.
    uint32_t ticksUntilRespawn = 0u;

    // ⭐⭐ THE FIGHTER'S OWN TINT [ringout task 10] -- what column one DRAWS, while
    // `characterId` above stays what the row is JOINED and SORTED on.
    //
    // It is the same colour that fighter's mesh is wearing, not a colour this board picks:
    // the UE gather copies `AOGBrawlerUECharacter::BrawlerColor`'s three channels in.
    // ⛔ THAT SOURCE IS ALREADY ON EVERY PEER. `BrawlerColor` is assigned server-side from
    // the character file's `kBrawlerPalette` and replicated with a PLAIN `DOREPLIFETIME`,
    // deliberately NOT `COND_OwnerOnly` -- the comment at its registration explains the
    // couch-co-op reasoning -- so a client's board draws real tints rather than a column of
    // white. It travels exactly like the score does, which is why this column works on a
    // client for the same reason column two does.
    //
    // ⚠ THE DEFAULT IS WHITE, AND IT IS THE RIGHT DEFAULT RATHER THAN AN ARBITRARY ONE:
    // `BrawlerColor` itself defaults to white, so a brawler that has not been possessed
    // yet, or whose colour has not replicated in yet, draws the swatch its MESH is
    // currently wearing. The board and the world agree even in that window.
    //
    // ⛔ NO ALPHA. The swatch is opaque; see `ScoreboardInk`. `BrawlerColor` carries an
    // alpha channel that the material uses and this column deliberately drops, because a
    // half-transparent swatch over a bright scene is a different colour to the eye than
    // the same swatch over a dark one -- and "which fighter is this" must not depend on
    // what happens to be behind the board.
    ScoreboardInk swatch{};
};

// ⭐⭐ THE ROW-TO-COLOUR MAPPING, AND IT IS DELIBERATELY INDEPENDENT OF `isDead`.
//
// This is the whole of the swatch's policy, and it is one line so that the policy is
// stated in a place a Catch2 case can reach rather than being an emergent property of the
// draw site. `drawScoreboard` calls this and names no colour of its own.
//
// ⛔⛔ WHAT THE SWATCH DOES WHILE A FIGHTER IS COUNTING DOWN: NOTHING. It is the fighter's
// full tint on a dead row and on a live row alike, byte for byte. That is a CHOICE, and
// here is the argument for it.
//
//   * The swatch's only job is IDENTITY, and a dead row is exactly when identity is most
//     needed: a player who has just been rung out is looking at this board to find their
//     own row and read how long they are out for. A column that changed appearance at the
//     moment it is being read hardest is the wrong column to carry status in.
//   * STATUS IS ALREADY CARRIED TWICE, by two channels that are not the identity channel:
//     the row's TEXT ink switches to `kScoreboardDeadRowInk` (dimmer and warmer), and
//     column three appears and reads `OUT nnn`. A third cue bought nothing that those two
//     do not already say, and it would have been the only one to cost identity.
//   * ⛔ ANY "DIM WHILE DEAD" RULE IS A MANY-TO-ONE MAP ON THE ONE AXIS THAT MUST STAY
//     ONE-TO-ONE. `kBrawlerPalette` is hand-picked so that neighbouring tints stay
//     distinguishable for a colour-blind reader and against the level's grey; pulling every
//     dead swatch toward a common colour compresses exactly that separation, and two
//     fighters can be dead at once. Dimming would trade the property this column exists for
//     against a cue the row already has.
//   * It also keeps the board honest about what it is showing: the fighter's BODY does not
//     change colour when it dies -- it respawns wearing the same tint -- so a swatch that
//     did would be showing something the world does not.
//
// ⛔ SO THIS FUNCTION MUST NOT GROW A `row.isDead` BRANCH. If a future task wants a
// dead-state cue in column one, the honest form is a SECOND mark (an outline, a strike)
// drawn beside the tint, never a transform applied to it.
inline constexpr ScoreboardInk scoreboardRowSwatch(const ScoreboardRow& row)
{
    return row.swatch;
}

// ---------------------------------------------------------------------------
// THE ORDERING. See DIFFERENCE 2 in the banner.
// ---------------------------------------------------------------------------

// Strict weak ordering on the id alone. Named rather than written as a lambda at the sort
// so a test can assert the ordering RELATION directly, not only its effect on one vector.
inline bool scoreboardRowPrecedes(const ScoreboardRow& left, const ScoreboardRow& right)
{
    return left.characterId < right.characterId;
}

// Sorts in place, ascending by character id.
//
// ⭐ `stable_sort`, NOT `sort`. Registration makes ids unique, so on any legal input the
// two agree exactly. But `scoreboardRowPrecedes` orders on the id ALONE, so two rows
// carrying the same id are TIED, and `std::sort` leaves tied elements in an UNSPECIFIED
// relative order -- the very machine-varying nondeterminism this call exists to remove. An
// ordering that is deterministic only while its input happens to be duplicate-free is not
// an ordering; it is a coincidence. The populations here are `kScoreboardMaxRows` = 8 rows,
// so the two algorithms cost the same.
//
// ⚠ BE HONEST ABOUT WHAT THIS SUITE CAN AND CANNOT SHOW, BECAUSE IT CHANGED MID-TASK.
// Measured on this tree on 2026-09-13: MSVC's `std::sort` falls back to INSERTION SORT at
// or below 32 elements, and insertion sort is stable, so the two produce byte-identical
// output at N = 4, 8, 16 and 32 and first diverge at N = 33. This board's cap is 8.
// ⛔ SO AT EVERY SIZE THE BOARD CAN REACH, NOTHING DISTINGUISHES THEM -- and task 6's red
// probe P2 (`stable_sort` swapped for `sort`) accordingly stayed GREEN against the thirteen
// cases that existed when it first ran, every one of them at N <= 8.
// ⭐ THE FINDING THEN PRODUCED THE TEST. `Scoreboard.TheStableSortIsAGuardThisSuiteCanOnly
// DiscriminateAboveItsOwnCap` drives the SHIPPED ordering at N = 33 as well, purely to pin
// the divergence point, and on the re-run P2 bit there -- one failed assertion, and only
// that one. ⛔ READ THE VERDICT PRECISELY: the guard is now covered ABOVE the cap and is
// still undiscriminated AT it, which is the honest description of what it buys. It earns
// its place against a cap that rises past 32, a library that moves its threshold, or a
// different toolchain -- all edits nowhere near this file.
inline void sortScoreboardRowsById(std::vector<ScoreboardRow>& rows)
{
    std::stable_sort(rows.begin(), rows.end(), scoreboardRowPrecedes);
}

// The value-returning form, which is what the gather in task 6b calls: it takes the
// unordered vector BY VALUE and hands back the ordered one, so there is no way to reach a
// draw site holding rows that were never sorted.
inline std::vector<ScoreboardRow> orderedScoreboardRows(std::vector<ScoreboardRow> rows)
{
    sortScoreboardRowsById(rows);
    return rows;
}

// ---------------------------------------------------------------------------
// THE BOARD'S GEOMETRY. Screen space, pixels, y GROWING DOWNWARD as every 2D canvas
// counts it -- so row 0 is the SMALLEST y, and x grows toward the right edge the board
// is flush against.
// ---------------------------------------------------------------------------

// How many rows the board will ever draw.
//
// ⛔ THIS IS A SCREEN BOUND, NOT A POPULATION BOUND, AND IT IS DELIBERATELY NOT 4.
// `ASimulationManagerUImpl::kPreDietCharacterCap == 4` is an ADVISORY cap, not an enforced
// one: its own fence (grep `PreDietCap`) reads "WARNING, not Log, and not an assert: an
// over-cap session still RUNS - report, do not crash", so a 5th character registers today,
// and `ScoreSystem::onCharacterRegistered` gives it a roster row without consulting the
// spawn table. That 4 is mirrored in this directory as `brawlerRingout::kMaxSpawnPoints`,
// which that constant's own comment already calls "THE THIRD MIRROR OF THAT 4". A fourth
// mirror here would be a draw cap that silently HIDES a fighter -- not on the day item
// 40's wire diet lifts the cap, but in any over-cap session, which can happen NOW -- and a
// hidden fighter on a scoreboard is worse than a board that runs off the screen, because
// nothing about it looks wrong.
//
// So it is set from what the SCREEN can hold instead: 8 rows at 18 px, at the maximum
// scale of 4, is 576 px -- still inside a 720p viewport's height, which is the shortest
// this project draws to. It is also twice the advisory cap, so lifting that cap to 8 needs
// no edit here at all. `Scoreboard.TheRowCapIsAScreenBoundAndAFullBoardFitsThe720pViewport`
// asserts that arithmetic rather than leaving it in this paragraph.
inline constexpr std::size_t kScoreboardMaxRows = 8u;

// The board is sized by ONE multiplier so that a tweak is a console line, not a rebuild.
// The range is the input pane's, unchanged: the two panels are tuned side by side and a
// different span for each would make "scale 2" mean two different things on one screen.
inline constexpr float kScoreboardDefaultScale = 1.0f;
inline constexpr float kScoreboardMinScale     = 0.25f;
inline constexpr float kScoreboardMaxScale     = 4.f;

// The rows carry their own contrast, so the shipped board adds no backdrop at all.
// ⛔ AT ZERO THE RECTANGLE IS SKIPPED, not drawn invisibly -- an invisible rectangle is
// still a canvas call on every frame.
inline constexpr float kScoreboardDefaultBackgroundAlpha = 0.0f;
inline constexpr float kScoreboardMinBackgroundAlpha     = 0.f;
inline constexpr float kScoreboardMaxBackgroundAlpha     = 1.f;

// A request outside the range is CLAMPED to the nearer end, never rejected -- the same
// contract `clampPanelScale` offers, and for the same reason: a rejected value would leave
// the console echoing a number that nothing uses.
//
// ⛔ THE FIRST TEST IS NEGATED ON PURPOSE: a console float can arrive as a NON-NUMBER, and
// under IEEE-754 NaN fails BOTH `>= min` and `> max`, so `!(x >= min)` is what lands it on
// the minimum instead of letting it through to multiply every geometric field in the
// layout into NaN -- a board that draws nowhere, from a value the console accepted.
//
// ⚠⚠ AND HERE IS THE MEASURED FACT THAT THE SHIPPED PRECEDENT DOES NOT RECORD, taken on
// this tree on 2026-09-13. THIS TARGET COMPILES WITH `/fp:fast`
// (`Intermediate/Build/Win64/x64/OGBrawlerTests/Development/OGBrawlerTests/*.rsp`), under
// which the compiler is licensed to assume no NaN exists and rewrites `x < c` into
// `!(x >= c)`. Measured directly: for a `volatile`-sourced NaN, `n < 0.25f` evaluates
// **TRUE** here, which IEEE-754 says it must not.
//
// ⛔ SO ON THIS BUILD BOTH SPELLINGS LAND NaN ON THE MINIMUM, AND A RED PROBE AGAINST THE
// NEGATION STAYS GREEN. Task 6 ran that probe (P7/P8) and reported it as a finding rather
// than claiming a bite it did not get. The negation is kept anyway, and is not decoration:
// it is a PORTABILITY guard. `/fp:fast` is a per-module setting, so the day a module that
// includes this header is compiled `/fp:precise` -- or a non-MSVC toolchain builds it --
// the non-negated spelling silently starts returning NaN and the negated one does not.
// `Scoreboard.TheNaNClampIsCorrectHereForAReasonTheToolchainSupplies` pins the toolchain
// fact itself, so it goes RED the day that flag changes and the negation becomes
// load-bearing again.
// ⛔ DO NOT "SIMPLIFY" THIS TO `if (x < min)`. The reason it currently looks equivalent is
// a compiler flag, not the language.
constexpr float clampScoreboardScale(float requestedScale)
{
    if (!(requestedScale >= kScoreboardMinScale))
        return kScoreboardMinScale;

    if (requestedScale > kScoreboardMaxScale)
        return kScoreboardMaxScale;

    return requestedScale;
}

constexpr float clampScoreboardBackgroundAlpha(float requestedAlpha)
{
    if (!(requestedAlpha >= kScoreboardMinBackgroundAlpha))
        return kScoreboardMinBackgroundAlpha;

    if (requestedAlpha > kScoreboardMaxBackgroundAlpha)
        return kScoreboardMaxBackgroundAlpha;

    return requestedAlpha;
}

// Offsets are from the board origin. A text column's `...RightX` is its RIGHT edge,
// because a right-aligned number is the only way a column of scores stays a column -- and
// on this board that matters twice over, since the whole panel is right-aligned too.
struct ScoreboardLayout
{
    // Both are written by placedScoreboardLayout: originX depends on the viewport WIDTH and
    // the scale, originY on the viewport HEIGHT, the scale and the row count.
    // ⛔ NEITHER ORIGIN IS A CHOSEN NUMBER, and unlike the input pane neither of them is a
    // constant either -- see DIFFERENCE 1 in the banner.
    float originX = 0.f;
    float originY = 0.f;

    float rowHeight = 18.f;
    float rowWidth  = 132.f;

    // Column 1: the fighter's COLOUR SWATCH [ringout task 10] -- a filled rectangle, not
    // text. A LEFT edge, for the reason the id text had one: it puts every row's swatch
    // against the same vertical rule the backdrop's left edge draws, so the column reads as
    // a column.
    // ⚠ RENAMED FROM `idX`, WHICH IS WHY THIS IS A RENAME AND NOT AN ADDITION. Column one
    // no longer draws an id, and a field still called `idX` would be a name a reader would
    // have to disbelieve.
    float swatchX = 6.f;

    // How wide the swatch is. Wide enough to read as a colour rather than as a dot at the
    // minimum scale (0.25 takes this to 7.5 px), and narrow enough that the three columns
    // keep their ordering: `swatchX + swatchWidth` is 36, well left of `scoreRightX`.
    float swatchWidth = 30.f;

    // Inset from the TOP and the BOTTOM of the row, so the swatch is a band inside its row
    // rather than a block that touches its neighbours. Two adjacent swatches with no gap
    // read as one two-tone rectangle, which is precisely the confusion this column exists
    // to remove.
    // ⛔ THE SWATCH'S HEIGHT IS NOT A FIELD -- it is `rowHeight - 2 * swatchInsetY`, derived
    // by `scoreboardSwatchRect` below. Storing it would let a scale move the row height and
    // the swatch height by different factors.
    float swatchInsetY = 3.f;

    // Column 2: the score. A RIGHT edge. Scores reach two and three digits mid-match, and a
    // left-aligned number column jitters horizontally every time one does.
    float scoreRightX = 78.f;

    // Column 3: the respawn countdown while `isDead`, and nothing at all while alive.
    // A RIGHT edge, for the same reason as the score.
    float statusRightX = 126.f;

    // A row's text, measured down from the row's own top edge.
    float textOffsetY = 2.f;

    // The board draws with a FIXED-SIZE font (`GEngine->GetSmallFont()`), so a scale
    // reaching only the geometry would give a bigger box holding the same tiny glyphs.
    // ⛔ THIS IS HOW THE ONE FACTOR REACHES THE GLYPHS, and it is set in one place.
    // ⛔ AND THE MEASURE MUST TAKE IT TOO. A right-aligned column's left edge is
    // `rightX - textWidth`, so a `GetTextSize` call that omits this scale measures the
    // unscaled glyphs and the column drifts further out of alignment the larger the board
    // gets -- invisible at the shipped default, which is the only scale most runs use.
    float textScale = 1.f;

    // ⚠ NOT CONSOLE-DRIVEN, so unlike the input pane's `visibleRows` it has no clamp.
    // It lives on the layout rather than being read as a constant so that
    // `scoreboardDrawnRowCount` and `scoreboardWindowHeight` take it from the same place a
    // future cvar would write it.
    std::size_t maxRows = kScoreboardMaxRows;
};

// Every positional number multiplied by ONE factor, with the text scale set from that same
// factor in the same statement list, so the two cannot be given different values. The
// origins are deliberately left alone: where the board sits is placement, and that is
// decided after this, from the width and the height this produced.
// ⛔ SCALING DECIDES SIZE ONLY.
// ⚠ `maxRows` IS NOT SCALED. It is a count of rows, not a length; scaling it would change
// how many fighters the board can show when the user changes how big it is.
inline ScoreboardLayout scaledScoreboardLayout(const ScoreboardLayout& base, float scale)
{
    ScoreboardLayout scaled = base;

    scaled.rowHeight    = base.rowHeight * scale;
    scaled.rowWidth     = base.rowWidth * scale;
    scaled.swatchX      = base.swatchX * scale;
    scaled.swatchWidth  = base.swatchWidth * scale;
    scaled.swatchInsetY = base.swatchInsetY * scale;
    scaled.scoreRightX  = base.scoreRightX * scale;
    scaled.statusRightX = base.statusRightX * scale;
    scaled.textOffsetY  = base.textOffsetY * scale;
    scaled.textScale    = base.textScale * scale;

    return scaled;
}

// How many of a roster's rows are drawn: all of them, capped at the layout's own bound.
inline std::size_t scoreboardDrawnRowCount(const ScoreboardLayout& layout,
                                           std::size_t             rowCount)
{
    return (rowCount < layout.maxRows) ? rowCount : layout.maxRows;
}

// Top edge of a row. Strictly increasing in `slotFromTop`, so slot 0 is the topmost row --
// and because `orderedScoreboardRows` has already sorted, slot 0 is the LOWEST character id
// on every peer.
inline float scoreboardRowTopY(const ScoreboardLayout& layout, std::size_t slotFromTop)
{
    return layout.originY + static_cast<float>(slotFromTop) * layout.rowHeight;
}

// ---------------------------------------------------------------------------
// ⭐ COLUMN ONE'S RECTANGLE [ringout task 10], IN ABSOLUTE SCREEN PIXELS.
//
// WHY IT IS A FUNCTION IN THIS HEADER AND NOT FOUR EXPRESSIONS AT THE DRAW SITE. Columns
// two and three are single `DrawText` calls that need one x and one y; column one needs
// FOUR numbers, one of which is derived rather than stored, and every one of them has to
// come out of the same layout. Written at the draw site it would be the only geometry in a
// `Source/OGBrawlerUnreal` file -- the exact thing finding F26 says to move, since no
// mechanical gate in this tree reaches that layer. Here a Catch2 case reads it directly.
// ---------------------------------------------------------------------------
struct ScoreboardSwatchRect
{
    float x      = 0.f;
    float y      = 0.f;
    float width  = 0.f;
    float height = 0.f;
};

// ⚠ NO CLAMP ON THE HEIGHT, and that is a decision of the same kind as
// `placedScoreboardLayout`'s missing clamp on `originX` -- stated here so it is not read as
// an omission.
//
// `rowHeight` and `swatchInsetY` are BOTH multiplied by the same factor in
// `scaledScoreboardLayout`, so `rowHeight - 2 * swatchInsetY` scales linearly and its SIGN
// is scale-invariant: no console value can flip it. A non-positive height is therefore
// reachable only by editing the two base constants above into disagreement, and a
// `max(0, ...)` there would turn that authoring error into a swatch of zero height -- a
// column that silently disappears, which is the failure mode this whole task exists to
// remove. `Scoreboard.TheSwatchIsABandInsideItsOwnRowAtEveryScale` asserts the base
// relation and the scale-invariance instead, so an edit that broke it goes red rather than
// drawing nothing.
inline ScoreboardSwatchRect scoreboardSwatchRect(const ScoreboardLayout& layout,
                                                 std::size_t             slotFromTop)
{
    ScoreboardSwatchRect rect;

    rect.x = layout.originX + layout.swatchX;

    // Inset from the row's OWN top edge, so the band moves with its row and with nothing
    // else. `scoreboardRowTopY` is the single place a row's y is decided.
    rect.y = scoreboardRowTopY(layout, slotFromTop) + layout.swatchInsetY;

    rect.width  = layout.swatchWidth;
    rect.height = layout.rowHeight - 2.f * layout.swatchInsetY;

    return rect;
}

// The height the board actually DRAWS. Reading rowHeight off the layout is what makes this
// the SCALED height whenever the layout is a scaled one -- which is the property the
// centring below depends on.
inline float scoreboardHeight(const ScoreboardLayout& layout, std::size_t drawnRowCount)
{
    return static_cast<float>(drawnRowCount) * layout.rowHeight;
}

// The height a FULL board reserves -- every row the cap allows, occupied or not. Nothing in
// the shipped placement centres this (see DIFFERENCE 3); it answers the worst-case
// footprint question instead: does the board still fit, and still clear the frame meter,
// on the tick the last player joins.
inline float scoreboardWindowHeight(const ScoreboardLayout& layout)
{
    return static_cast<float>(layout.maxRows) * layout.rowHeight;
}

// ⛔ THE WIDTH PASSED IN MUST ALREADY BE SCALED, for exactly the reason the centring below
// spells out: a right edge computed from an unscaled width is exact at scale 1 and wrong at
// every other, and it is wrong by the amount the board overhangs the screen.
inline float scoreboardRightFlushOriginX(float viewportWidth, float scaledBoardWidth)
{
    return viewportWidth - scaledBoardWidth;
}

// ⛔ THE HEIGHT PASSED IN MUST ALREADY BE SCALED. Centring an unscaled height is exact at
// scale 1 and off-centre at every other, which is invisible if only the default is run.
inline float scoreboardCenteredOriginY(float viewportHeight, float scaledBoardHeight)
{
    return (viewportHeight - scaledBoardHeight) * 0.5f;
}

// ⭐ THE ONE WAY TO GET A DRAWABLE LAYOUT: scale, then place what the scale produced. The
// order is not the caller's to get wrong, because the caller never sees the two steps.
//
// ⚠ NO CLAMP ON `originX`, and that is a decision rather than an omission. At a large scale
// on a narrow viewport the board is wider than the screen and this returns a NEGATIVE
// origin, so the board runs off the LEFT while its right edge stays exactly on the right
// edge of the screen. Pulling it back to 0 would silently break the one property the panel
// is defined by -- "the right edge sits at viewportWidth" -- and would hide an unusable
// scale behind a board that merely looked cramped.
// `Scoreboard.TheBoardOverhangsRatherThanUnflushingWhenItCannotFit` pins the scale at which
// that starts, at each probed viewport, rather than leaving it to be found in play.
inline ScoreboardLayout placedScoreboardLayout(const ScoreboardLayout& base,
                                               float                   scale,
                                               std::size_t             rowCount,
                                               float                   viewportWidth,
                                               float                   viewportHeight)
{
    ScoreboardLayout placed = scaledScoreboardLayout(base, scale);

    const std::size_t drawnRows = scoreboardDrawnRowCount(placed, rowCount);

    placed.originX = scoreboardRightFlushOriginX(viewportWidth, placed.rowWidth);
    placed.originY =
        scoreboardCenteredOriginY(viewportHeight, scoreboardHeight(placed, drawnRows));

    return placed;
}

// ---------------------------------------------------------------------------
// THE INK -- THREE COLOURS, AND NOT ONE OF THEM IS KEYED ON AN ENUMERATOR.
//
// ⛔ THAT IS A CONSTRAINT, NOT AN ACCIDENT. `palette_legend_lint.ps1` is scoped to the
// frame meter's three enum-keyed palettes (`provenanceCellStyleOf`, `machineCellStyleOf`,
// `delayVerdictStyleOf` in `BrawlerInputHistoryVisualizationBars.h`), each of which must
// keep a legend table in the rationale doc in step with its switch arms. A FOURTH such
// palette would need its own legend table AND its own lint arm, in the same change, or it
// becomes exactly the stale table that lint exists to prevent.
//
// The board has no enumerated state to colour. It has ONE BOOLEAN -- out, or not -- so the
// selector below is a two-armed function of a bool, there is no switch, there are no
// enumerator names for a table to list, and the lint's scope does not reach it. ⛔ IF A
// FUTURE TASK GIVES THIS BOARD AN ENUM-KEYED COLOUR, IT OWES THE TABLE AND THE LINT ARM.
//
// ⭐⭐ AND TASK 10'S SWATCH DID NOT BECOME ONE -- STATED EXPLICITLY RATHER THAN LEFT TO BE
// INFERRED, BECAUSE IT IS THE FIRST COLOUR ON THIS BOARD THAT IS NOT A CONSTANT.
// `scoreboardRowSwatch` is a per-character RUNTIME TINT: it returns a value that was
// carried in on the row, assigned at possession time from `kBrawlerPalette` over in
// `OGBrawlerUECharacter.cpp`, and this header neither names an entry of that palette nor
// selects between entries. The lint's subject is a FUNCTION THAT SWITCHES ON AN ENUMERATOR
// -- `provenanceCellStyleOf` and its two siblings -- because that is the shape whose arms
// can silently fall out of step with a legend table listing the enumerator names. A
// function with no arms at all has nothing to list: a legend for this column would have to
// enumerate the palette, which lives in a different module, is indexed by a possession
// counter rather than by any named state, and is therefore exactly the table that would go
// stale the day someone appends an eleventh colour. ⛔ THE RULE ABOVE IS UNCHANGED AND
// STILL BINDS: the day this board keys a colour on an ENUMERATOR, it owes the table and
// the lint arm in the same change.
// ---------------------------------------------------------------------------

// ⭐ `ScoreboardInk` ITSELF IS DECLARED ABOVE `ScoreboardRow`, not here -- task 10 gave the
// row a member of this type and a member cannot name a type declared after it. The three
// constants and the selector stayed put.

// A fighter who is in play.
inline constexpr ScoreboardInk kScoreboardLiveRowInk{ 1.f, 1.f, 1.f };

// A fighter who is out, waiting on the countdown in column three. Dimmer and warmer, so the
// row is still readable -- a score you cannot read is worse than a score that looks the
// same as everyone else's -- while being obviously not one of the live ones.
inline constexpr ScoreboardInk kScoreboardDeadRowInk{ 0.85f, 0.45f, 0.35f };

// The backdrop, drawn only when the alpha above is non-zero.
inline constexpr ScoreboardInk kScoreboardBackdropInk{ 0.f, 0.f, 0.f };

// ⛔ KEYED ON A BOOL. See the banner: this is what keeps the board outside
// `palette_legend_lint.ps1`'s scope, and it is a property of the SHAPE of this function,
// not of its current contents.
inline constexpr ScoreboardInk scoreboardRowInk(bool isDead)
{
    return isDead ? kScoreboardDeadRowInk : kScoreboardLiveRowInk;
}

// Whether column three has anything to say. Gated on the LEVEL, never on the countdown
// being non-zero: `ticksUntilRespawn` is meaningful only while `isDead`, so a living
// fighter's copy is stale by construction. See `ScoreboardRow::ticksUntilRespawn`.
inline constexpr bool scoreboardRowDrawsCountdown(const ScoreboardRow& row)
{
    return row.isDead;
}

// ⭐ THE COUNTDOWN, FROM TWO ABSOLUTE TICKS -- added by task 6b.
//
// WHY IT IS HERE AND NOT AT THE GATHER, WHERE IT WAS FIRST WRITTEN. The gather is a
// `Source/OGBrawlerUnreal` translation unit and this suite links { Core, OGSimulation,
// OGBrawler } and not that module, so the one piece of real arithmetic in the whole UE
// layer sat where nothing could probe it. It is three characters wide and carries a live
// hazard, which is a bad combination to leave untested.
//
// ⛔ CLAMPED AT ZERO, NOT ALLOWED TO WRAP, AND THE HAZARD IS NOT HYPOTHETICAL. Both ticks
// are `uint32_t` and THE DISPLAY TICK MOVES BACKWARDS: a hard resync rewinds the client
// prediction clock, so the tick a board is drawing at can legitimately be LATER than a
// `respawnAtTick` it has not reached yet. Unguarded, `respawnAtTick - displayTick` then
// reads about four billion ticks -- a ten-digit number in a two-digit column, on exactly
// the frames a desync investigation is looking at.
// ⚠ IT ALSO COVERS AN ENTIRELY ORDINARY FRAME, which is the easier one to forget: the
// respawn tick has arrived but the sub-simulation has not yet run the step that clears
// `kFlagDead`, so the flag is still set and the difference is zero or negative. The board
// reads 0 for that frame rather than a wrapped number.
//
// ⛔ NOT GATED ON `isDead` HERE. This answers an arithmetic question; whether the answer is
// MEANINGFUL is `scoreboardRowDrawsCountdown`'s question, asked against the flag, and
// folding the two together would give a caller one predicate that quietly means two things.
inline constexpr uint32_t scoreboardTicksUntilRespawn(uint32_t respawnAtTick,
                                                      uint32_t displayTick)
{
    return (respawnAtTick > displayTick) ? (respawnAtTick - displayTick) : 0u;
}

} // namespace brawlerScoreboardVisualization

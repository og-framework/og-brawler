#pragma once
// SPDX-License-Identifier: BUSL-1.1

// The frame meter: TWO STACKED BARS of one cell per tick, read off the per-tick lanes.
//
//   * Pure. Fixed arrays and floats only -- no engine type, no canvas, no font.
//   * A SIBLING of BrawlerInputHistoryVisualizationLanes.h, in the same spirit as the
//     panel header is to the row model: that one stores, this one presents.
//
// ---------------------------------------------------------------------------
// THE REFERENCE IS A FIGHTING GAME'S FRAME METER, and three of its properties are the
// whole design: one cell per tick, newest at the right, and the run length printed on
// the run's LAST cell. Runs are detected HERE, at draw time, by comparing neighbours in
// the retained window. Nothing upstream folds them.
//
// ---------------------------------------------------------------------------
// ONE GEOMETRY DRIVES BOTH BARS, and that is what tick alignment IS.
//
// Both bars take their x from frameMeterCellX(geometry, offset) and differ only in the
// bar index they pass to frameMeterBarTopY. A vertical slice through the two is then the
// same capture tick by construction rather than by two derivations agreeing.
// ⛔ NEVER GIVE A BAR ITS OWN ORIGIN OR ITS OWN STRIDE.
//
// ---------------------------------------------------------------------------
// A HOLE IS NOT A STATE.
//
// The machine lane has gaps by design -- there is no per-tick machine-state history to
// back-fill from -- and the provenance lane has none for a tick no observation named. In
// a netcode diagnostic a coloured cell reads as evidence, so a gap must read as absence.
// ⛔ A HOLE DRAWS NOTHING AND JOINS NO RUN.
// ---------------------------------------------------------------------------

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "OGBrawler/BrawlerInputHistoryVisualizationLanes.h"
#include "OGBrawler/BrawlerInputHistoryVisualizationPoll.h"

namespace brawlerInputHistoryVisualization
{

// ---------------------------------------------------------------------------
// COLOUR. Linear RGB in [0, 1], the same numbers the canvas takes, so the renderer
// converts nothing and no second table exists to drift.
// ---------------------------------------------------------------------------
struct LaneCellColor
{
	float r = 0.f;
	float g = 0.f;
	float b = 0.f;
};

// Channel-sum distance, the same measure the panel's own palette was tuned against.
// Cheap, monotone in every channel, and it needs no colour space to be argued about.
constexpr float laneColorGap(LaneCellColor left, LaneCellColor right)
{
	const float dr = (left.r > right.r) ? (left.r - right.r) : (right.r - left.r);
	const float dg = (left.g > right.g) ? (left.g - right.g) : (right.g - left.g);
	const float db = (left.b > right.b) ? (left.b - right.b) : (right.b - left.b);

	return dr + dg + db;
}

// Two states inside one palette must differ by at least this. Calibrated ON the existing
// provenance palette, whose own closest pair (Unknown against NoStateWritten) sits at 0.32.
inline constexpr float kLanePaletteMinPairGap = 0.30f;

// Across the two palettes the bar is stricter, because a machine cell that read as a
// provenance state would make the two bars one encoding stacked twice.
inline constexpr float kLanePaletteMinCrossGap = 0.45f;

// What a cell does when it is drawn. Three answers, not two: an absent cell and an
// UNMAPPED enumerator are opposite failures and must not share a look.
enum class LaneCellFill : uint8_t
{
	Hole,
	State,
	Unnamed,
};

struct LaneCellStyle
{
	LaneCellFill  fill = LaneCellFill::Hole;
	LaneCellColor color{};
};

// A value outside its enumeration is a table that stopped covering its own enum.
// ⛔ LOUD ON PURPOSE, and further from every palette entry than any two of them are.
inline constexpr LaneCellColor kUnnamedLaneColor{ 1.00f, 1.00f, 0.35f };

// A collapsed idle span is not a state, it is missing time, so its marker belongs to
// neither palette and clears the cross-palette floor against every entry in both.
inline constexpr LaneCellColor kLaneElisionColor{ 0.62f, 0.32f, 0.00f };

// The axis vocabulary's second word: a resync is time the client simulated TWICE or never,
// which is the opposite claim to the elision's, so it takes the opposite half of the wheel.
// ⛔ NEVER THE ELISION'S COLOUR READ BY ITS SIGN -- colour is how this meter states a claim.
inline constexpr LaneCellColor kLaneResyncColor{ 0.05f, 0.00f, 1.00f };

// ---------------------------------------------------------------------------
// THE PROVENANCE PALETTE -- NINE COLOURS, AND THEY ARE NOT NEW.
//
// These are the input panel's own nine, kept value for value so the two displays speak
// one colour language about one datum. Corrected against Resimulated is the pair this
// whole display exists to separate, so they sit at opposite ends of the hue circle.
// ---------------------------------------------------------------------------
constexpr LaneCellStyle provenanceCellStyleOf(RowProvenanceSummary summary)
{
	switch (summary)
	{
	case RowProvenanceSummary::Unknown:
		return { LaneCellFill::State, { 0.42f, 0.42f, 0.42f } };
	case RowProvenanceSummary::Pending:
		return { LaneCellFill::State, { 0.20f, 0.45f, 0.95f } };
	case RowProvenanceSummary::NoStateWritten:
		return { LaneCellFill::State, { 0.35f, 0.30f, 0.55f } };
	case RowProvenanceSummary::RanUnconfirmed:
		return { LaneCellFill::State, { 0.88f, 0.88f, 0.88f } };
	case RowProvenanceSummary::LineageUnavailable:
		return { LaneCellFill::State, { 0.70f, 0.60f, 0.18f } };
	case RowProvenanceSummary::Confirmed:
		return { LaneCellFill::State, { 0.15f, 0.80f, 0.30f } };
	case RowProvenanceSummary::Corrected:
		return { LaneCellFill::State, { 1.00f, 0.55f, 0.05f } };
	case RowProvenanceSummary::Resimulated:
		return { LaneCellFill::State, { 0.10f, 0.82f, 0.95f } };
	case RowProvenanceSummary::ProvenanceLie:
		return { LaneCellFill::State, { 1.00f, 0.05f, 0.75f } };
	}

	return { LaneCellFill::Unnamed, kUnnamedLaneColor };
}

// ---------------------------------------------------------------------------
// THE MACHINE-STATE PALETTE -- FOUR COLOURS PLUS A HOLE, IN HUES THE NINE LEFT FREE.
//
// Deep red, deep teal, chartreuse and violet sit in gaps the provenance ladder does not
// occupy, so the nearest cross-palette pair is further apart than the closest pair inside
// the provenance palette itself. The sweep in the suite is what keeps that true.
// ---------------------------------------------------------------------------
constexpr LaneCellStyle machineCellStyleOf(MachineStateCell cell)
{
	switch (cell)
	{
	// ⛔ THE HOLE CARRIES NO COLOUR AT ALL: a colour here would be read as a state.
	case MachineStateCell::NotSampled:
		return { LaneCellFill::Hole, {} };
	case MachineStateCell::Attacking:
		return { LaneCellFill::State, { 0.85f, 0.10f, 0.10f } };
	case MachineStateCell::Idle:
		return { LaneCellFill::State, { 0.05f, 0.30f, 0.25f } };
	case MachineStateCell::GuardFlinch:
		return { LaneCellFill::State, { 0.50f, 1.00f, 0.05f } };
	case MachineStateCell::HitFlinch:
		return { LaneCellFill::State, { 0.55f, 0.00f, 0.85f } };
	}

	return { LaneCellFill::Unnamed, kUnnamedLaneColor };
}

// ---------------------------------------------------------------------------
// THE SAME TWO TABLES, ASKED BY ORDINAL. A bar stores the enumerator's value, and a
// sweep needs to walk PAST the last one to prove the table still covers its own enum.
// ⛔ ONE TABLE EACH -- these forward, they do not restate a colour.
// ---------------------------------------------------------------------------
using LaneCellStyleOfOrdinal = LaneCellStyle (*)(uint8_t);

constexpr LaneCellStyle provenanceCellStyleOfOrdinal(uint8_t ordinal)
{
	return provenanceCellStyleOf(static_cast<RowProvenanceSummary>(ordinal));
}

constexpr LaneCellStyle machineCellStyleOfOrdinal(uint8_t ordinal)
{
	return machineCellStyleOf(static_cast<MachineStateCell>(ordinal));
}

// ---------------------------------------------------------------------------
// THE DELAY-VERDICT PALETTE -- SIX COLOURS PLUS A HOLE, ON THE SAME LADDER.
//
// NoVerdict is the hole. ServerEarlier means the server's own number came in SMALLER
// than the client's, which lateness alone cannot cause, so it is the one state that
// always names a real divergence -- it earns the most visual room of the six, measured
// as the largest minimum gap to the others rather than picked by eye.
// ---------------------------------------------------------------------------
constexpr LaneCellStyle delayVerdictStyleOf(InputDelayVerdict verdict)
{
	switch (verdict)
	{
	case InputDelayVerdict::NoVerdict:
		return { LaneCellFill::Hole, {} };
	case InputDelayVerdict::Agree:
		return { LaneCellFill::State, { 0.00f, 0.84f, 0.00f } };
	case InputDelayVerdict::LagShortByOne:
		return { LaneCellFill::State, { 0.00f, 0.14f, 0.52f } };
	case InputDelayVerdict::ServerLater:
		return { LaneCellFill::State, { 0.76f, 0.50f, 0.50f } };
	case InputDelayVerdict::ServerEarlier:
		return { LaneCellFill::State, { 0.36f, 1.00f, 1.00f } };
	case InputDelayVerdict::LagUnverified:
		return { LaneCellFill::State, { 0.80f, 0.08f, 1.00f } };
	case InputDelayVerdict::NoCaptureNamed:
		return { LaneCellFill::State, { 0.47f, 0.02f, 0.12f } };
	}

	return { LaneCellFill::Unnamed, kUnnamedLaneColor };
}

// Asked by ordinal, matching the two tables above -- one table each, forwarded here.
constexpr LaneCellStyle delayVerdictStyleOfOrdinal(uint8_t ordinal)
{
	return delayVerdictStyleOf(static_cast<InputDelayVerdict>(ordinal));
}

// Light cells need dark ink and dark cells need light ink, or the run length vanishes
// into the cell it belongs to. Green-weighted, which is where perceived brightness is.
constexpr bool laneLabelPrefersDarkInk(LaneCellColor color)
{
	return (0.30f * color.r + 0.59f * color.g + 0.11f * color.b) > 0.55f;
}

// ---------------------------------------------------------------------------
// THE CELLS OF ONE BAR. `value` is the enumerator ordinal, which is all run detection
// needs; a hole carries none and joins nothing.
// ---------------------------------------------------------------------------
struct FrameMeterCell
{
	bool    filled = false;
	uint8_t value  = 0u;
};

struct FrameMeterBarCells
{
	std::array<FrameMeterCell, kTickLaneCapacity> cells{};
	uint32_t                                      count = 0u;
};

// ---------------------------------------------------------------------------
// BAR IDENTITY. Declaration order top-to-bottom IS the display order -- the ONLY
// place the reorder the user asked for lives.
// ---------------------------------------------------------------------------
enum class FrameMeterBarKind : uint8_t
{
	Provenance,
	InputDelay,
	CharacterState,
};

inline constexpr uint8_t kFrameMeterBarKindCount = 3u;

// Every bar defaults ON; the UE-side gates flip these before geometry is asked for
// anything.
struct FrameMeterBarSelection
{
	bool provenance     = true;
	bool inputDelay     = true;
	bool characterState = true;
};

// ⛔ THE ONLY PLACE A KIND MAPS TO ITS OWN FLAG.
constexpr bool frameMeterBarKindEnabled(const FrameMeterBarSelection& selection,
                                        FrameMeterBarKind             kind)
{
	switch (kind)
	{
	case FrameMeterBarKind::Provenance:
		return selection.provenance;
	case FrameMeterBarKind::InputDelay:
		return selection.inputDelay;
	case FrameMeterBarKind::CharacterState:
		return selection.characterState;
	}

	return false;
}

// How many bars are switched on, 0..3.
constexpr uint32_t frameMeterEnabledBarCount(const FrameMeterBarSelection& selection)
{
	return static_cast<uint32_t>(selection.provenance)
	     + static_cast<uint32_t>(selection.inputDelay)
	     + static_cast<uint32_t>(selection.characterState);
}

// `kind`'s slot AMONG THE ENABLED BARS, in declaration order; nullopt when it is off.
// ⛔ THE ONLY PLACE COMPACTION HAPPENS.
constexpr std::optional<uint32_t> frameMeterBarSlotOf(const FrameMeterBarSelection& selection,
                                                       FrameMeterBarKind             kind)
{
	if (!frameMeterBarKindEnabled(selection, kind))
		return std::nullopt;

	uint32_t slot = 0u;
	for (uint8_t ordinal = 0u; ordinal < static_cast<uint8_t>(kind); ++ordinal)
	{
		if (frameMeterBarKindEnabled(selection, static_cast<FrameMeterBarKind>(ordinal)))
			++slot;
	}
	return slot;
}

// The meter WITHOUT the delay bar -- stays 2 so the four-argument geometry overload
// below keeps forwarding to today's shape.
inline constexpr uint32_t kFrameMeterBarCount   = 2u;

// R3: no run labels on the input-delay bar -- its runs are short and sparse, and the
// numbers would read as noise.
constexpr bool frameMeterBarDrawsRunLabels(FrameMeterBarKind kind)
{
	return kind != FrameMeterBarKind::InputDelay;
}

// How many cells a window fills, never more than the lanes physically hold.
inline uint32_t frameMeterCellCount(const PollWindow& window)
{
	return (window.tickCount() < static_cast<uint32_t>(kTickLaneCapacity))
	           ? window.tickCount()
	           : static_cast<uint32_t>(kTickLaneCapacity);
}

// The provenance bar over `window`. A tick no observation named has no cell, which is a
// hole here rather than a defaulted Unknown -- nothing was learned, so nothing is claimed.
inline void readProvenanceBar(const InputHistoryTickLanes& lanes,
                              const PollWindow&            window,
                              FrameMeterBarCells&          bar)
{
	bar.count = frameMeterCellCount(window);

	for (uint32_t offset = 0u; offset < bar.count; ++offset)
	{
		const RowProvenanceSummary* summary = lanes.provenanceAt(window.oldestTick + offset);

		bar.cells[offset].filled = (summary != nullptr);
		bar.cells[offset].value  = (summary != nullptr) ? static_cast<uint8_t>(*summary) : 0u;
	}
}

// The machine-state bar over the SAME window. NotSampled is the lane's own total answer,
// so it becomes a hole here without any second opinion being formed about it.
inline void readMachineStateBar(const InputHistoryTickLanes& lanes,
                                const PollWindow&            window,
                                FrameMeterBarCells&          bar)
{
	bar.count = frameMeterCellCount(window);

	for (uint32_t offset = 0u; offset < bar.count; ++offset)
	{
		const MachineStateCell cell = lanes.machineCellAt(window.oldestTick + offset);

		bar.cells[offset].filled = (cell != MachineStateCell::NotSampled);
		bar.cells[offset].value  = static_cast<uint8_t>(cell);
	}
}

// The delay-verdict bar over the SAME window. A cell nothing has written, or one whose
// verdict comes out NoVerdict, is a hole -- exactly like the two lanes above.
inline void readDelayBar(const InputHistoryTickLanes& lanes,
                         const PollWindow&            window,
                         FrameMeterBarCells&          bar)
{
	bar.count = frameMeterCellCount(window);

	for (uint32_t offset = 0u; offset < bar.count; ++offset)
	{
		const InputDelayCell*   cell    = lanes.delayCellAt(window.oldestTick + offset);
		const InputDelayVerdict verdict = (cell != nullptr) ? delayVerdictOf(*cell)
		                                                    : InputDelayVerdict::NoVerdict;

		bar.cells[offset].filled = (verdict != InputDelayVerdict::NoVerdict);
		bar.cells[offset].value  = static_cast<uint8_t>(verdict);
	}
}

// ---------------------------------------------------------------------------
// RUNS, DETECTED AT DRAW TIME. A run is a maximal stretch of neighbouring cells holding
// one value; the number goes on `lastOffset`, which is the run's RIGHT-hand cell.
// ---------------------------------------------------------------------------
struct LaneRun
{
	uint32_t firstOffset = 0u;
	uint32_t lastOffset  = 0u;
	uint32_t length      = 0u;
	uint8_t  value       = 0u;
};

struct LaneRunList
{
	std::array<LaneRun, kTickLaneCapacity> runs{};
	uint32_t                               count = 0u;
};

// ⛔ A HOLE ENDS THE RUN IT INTERRUPTS AND STARTS NONE OF ITS OWN.
inline void collectLaneRuns(const FrameMeterBarCells& bar, LaneRunList& out)
{
	out.count = 0u;

	for (uint32_t offset = 0u; offset < bar.count; ++offset)
	{
		const FrameMeterCell& cell = bar.cells[offset];

		if (!cell.filled)
			continue;

		const bool extendsPrevious = out.count != 0u
			&& out.runs[out.count - 1u].lastOffset + 1u == offset
			&& out.runs[out.count - 1u].value == cell.value;

		if (extendsPrevious)
		{
			LaneRun& run   = out.runs[out.count - 1u];
			run.lastOffset = offset;
			++run.length;
			continue;
		}

		out.runs[out.count] = LaneRun{ offset, offset, 1u, cell.value };
		++out.count;
	}
}

// ---------------------------------------------------------------------------
// GEOMETRY. Screen space, pixels, y GROWING DOWNWARD -- so the bottom of the screen is
// the LARGEST y, and "bottom middle" is a subtraction from the viewport height.
//
// Cell sizes are absolute because legibility is absolute; the margin off the bottom edge
// is a FRACTION because where a widget sits in the frame is relative to the frame.
// ---------------------------------------------------------------------------
struct FrameMeterLayout
{
	float preferredCellStride = 8.f;
	float cellGap             = 1.f;
	float minCellStride       = 3.f;

	float barHeight = 14.f;
	float barGap    = 3.f;

	float bottomMarginFraction = 0.09f;

	// The bar spans no more of the viewport than this before its cells start shrinking.
	float maxWidthFraction = 0.90f;

	float backdropPadding = 3.f;
};

// Everything both bars are drawn from. There is exactly one of these per frame.
struct FrameMeterGeometry
{
	float    originX    = 0.f;
	float    originY    = 0.f;
	float    cellStride = 0.f;
	float    cellWidth  = 0.f;
	float    barHeight  = 0.f;
	float    barGap     = 0.f;
	uint32_t cellCount  = 0u;
	uint32_t barCount   = kFrameMeterBarCount;
};

inline float frameMeterWidth(const FrameMeterGeometry& geometry)
{
	return static_cast<float>(geometry.cellCount) * geometry.cellStride;
}

inline float frameMeterHeight(const FrameMeterGeometry& geometry)
{
	// ⛔ barCount 0 is reachable now that bars compact -- unsigned barCount - 1u would underflow.
	if (geometry.barCount == 0u)
		return 0.f;

	return static_cast<float>(geometry.barCount) * geometry.barHeight
	       + static_cast<float>(geometry.barCount - 1u) * geometry.barGap;
}

// The bar, centred horizontally and anchored above the bottom edge. A wide retained
// window on a narrow viewport shrinks the cells rather than running off either side.
inline FrameMeterGeometry frameMeterGeometryFor(const FrameMeterLayout& layout,
                                                float                   viewportWidth,
                                                float                   viewportHeight,
                                                uint32_t                cellCount,
                                                uint32_t                barCount)
{
	FrameMeterGeometry geometry;
	geometry.cellCount = cellCount;
	geometry.barHeight = layout.barHeight;
	geometry.barGap    = layout.barGap;
	geometry.barCount  = barCount;

	geometry.cellStride = layout.preferredCellStride;
	if (cellCount != 0u)
	{
		const float affordable =
			(viewportWidth * layout.maxWidthFraction) / static_cast<float>(cellCount);

		if (affordable < geometry.cellStride)
			geometry.cellStride = affordable;
	}

	if (geometry.cellStride < layout.minCellStride)
		geometry.cellStride = layout.minCellStride;

	geometry.cellWidth = geometry.cellStride - layout.cellGap;
	if (geometry.cellWidth < 1.f)
		geometry.cellWidth = geometry.cellStride;

	geometry.originX = (viewportWidth - frameMeterWidth(geometry)) * 0.5f;
	geometry.originY = viewportHeight
	                   - viewportHeight * layout.bottomMarginFraction
	                   - frameMeterHeight(geometry);

	return geometry;
}

// ⛔ MUST RETURN TODAY'S GEOMETRY BYTE-FOR-BYTE -- the compatibility pin.
inline FrameMeterGeometry frameMeterGeometryFor(const FrameMeterLayout& layout,
                                                float                   viewportWidth,
                                                float                   viewportHeight,
                                                uint32_t                cellCount)
{
	return frameMeterGeometryFor(
		layout, viewportWidth, viewportHeight, cellCount, kFrameMeterBarCount);
}

// ⛔ THE ONLY SOURCE OF A COLUMN'S X, and both bars ask it -- that IS the tick alignment.
inline float frameMeterCellX(const FrameMeterGeometry& geometry, uint32_t offset)
{
	return geometry.originX + static_cast<float>(offset) * geometry.cellStride;
}

// Top edge of one bar. Bar 0 is the provenance lane, bar 1 the machine-state lane.
inline float frameMeterBarTopY(const FrameMeterGeometry& geometry, uint32_t barIndex)
{
	return geometry.originY
	       + static_cast<float>(barIndex) * (geometry.barHeight + geometry.barGap);
}

// ---------------------------------------------------------------------------
// TWO VERTICAL MARKERS CROSS THESE BARS, AND THEY MEAN OPPOSITE THINGS.
//
// The frozen horizon says "the correction cache can no longer answer for anything left
// of here". The authority marker says "the server is here, and everything right of it
// is prediction nobody has confirmed". A reader who took one for the other would reach
// the opposite conclusion about a desync, so every field that could make the two look
// alike is named in ONE struct and swept in the suite.
// ⛔ NEITHER MARKER IS STYLED AT THE DRAW SITE.
// ---------------------------------------------------------------------------
enum class FrameMeterMarkerShape : uint8_t
{
	PlainRule,      // a rule, with nothing attached to it
	LabelledRule,   // a rule carrying a number of its own
};

struct FrameMeterMarkerStyle
{
	LaneCellColor         color{};
	float                 alpha     = 1.f;
	float                 thickness = 1.f;
	FrameMeterMarkerShape shape     = FrameMeterMarkerShape::PlainRule;
};

// ---------------------------------------------------------------------------
// THE FROZEN HORIZON.
//
// The provenance lane is only WRITTEN across the correction cache's resident window, so
// cells older than that can no longer change -- truthfully, since neither seam can answer
// for them any more. A reader watching the left half never update deserves to be told why.
// ⛔ THE FROZEN CELLS ARE CORRECT OBSERVATIONS AND ARE NEVER DIMMED OR HATCHED.
//
// The writable window is 60 SIM ticks, so the edge is a RESIDENCY READING converted through
// the same placement helper as the authority marker, not a lane-cell count -- see
// `placeFrameMeterSimTick` and `frameMeterHorizonOf` below the authority marker section.
// ---------------------------------------------------------------------------

// Quiet, thin and unlabelled. A loud rule here would report the cells left of it as
// suspect when they are finished observations.
inline constexpr FrameMeterMarkerStyle kFrameMeterHorizonStyle{
	{ 0.55f, 0.55f, 0.58f }, 0.45f, 1.f, FrameMeterMarkerShape::PlainRule };

// ---------------------------------------------------------------------------
// THE MARKER CELLS -- AN ELISION AND A RESYNC, ONE LEDGER WALK.
//
// A paused span leaves ONE empty lane tick behind, and drawing nothing there would let
// the bar imply that the runs either side of it were neighbours. The marker says they
// were not, and carries the number of ticks that were removed between them.
//
// The gap is one cell wide whether it swallowed sixteen ticks or sixteen hundred.
// ⛔ THE COUNT COMES OFF THE GATE THAT MADE THE CUT, never off the gap in the bar.
// ---------------------------------------------------------------------------
// One marker cell of EITHER kind, with the label that kind prints. An elision counts the
// ticks it removed; a resync states how far the axis jumped, and the SIGN is the direction.
// ⛔ THE SIGN IS NEVER DROPPED: a backward resync re-ran time, a forward one skipped it.
struct FrameMeterAxisEvent
{
	LaneAxisEventKind kind         = LaneAxisEventKind::Elision;
	uint32_t          offset       = 0u;
	uint32_t          skippedTicks = 0u;
	int32_t           deltaTicks   = 0;
};

struct FrameMeterAxisEventList
{
	std::array<FrameMeterAxisEvent, kLaneElisionLedgerCapacity> marks{};
	uint32_t                                                    count = 0u;
};

// The markers whose own lane tick falls inside `window`, as bar offsets. Both bars take
// these SAME offsets, so one marker cuts the pair at one column.
inline void collectFrameMeterAxisEvents(const InputHistoryTickLanes& lanes,
                                        const PollWindow&            window,
                                        FrameMeterAxisEventList&     out)
{
	out.count = 0u;

	const uint32_t cellCount = frameMeterCellCount(window);

	for (std::size_t index = 0u; index < lanes.gate().axisEventCount(); ++index)
	{
		const LaneAxisEvent& event = lanes.gate().axisEventAt(index);

		if (event.laneTick < window.oldestTick)
			continue;

		const uint32_t offset = event.laneTick - window.oldestTick;
		if (offset >= cellCount)
			continue;

		// ⛔ THE OPERANDS STAY int64_t UNTIL AFTER THIS SUBTRACTION -- two unsigned ticks
		//   subtracted first would wrap a backward jump into a vast positive number.
		const int64_t delta = (event.kind == LaneAxisEventKind::Resync)
		                          ? static_cast<int64_t>(event.simTick)
		                                - static_cast<int64_t>(event.fromSimTick)
		                          : 0;

		out.marks[out.count] = FrameMeterAxisEvent{ event.kind, offset, event.skippedTicks,
			static_cast<int32_t>(delta) };
		++out.count;
	}
}

// The marker is drawn across BOTH bars, so it reads as time removed rather than as a
// state in one of them; its count sits clear above the backdrop, over no cell at all.
inline float frameMeterElisionLabelTopY(const FrameMeterGeometry& geometry,
                                        const FrameMeterLayout&   layout,
                                        float                     labelHeight)
{
	return geometry.originY - layout.backdropPadding - labelHeight;
}

// ---------------------------------------------------------------------------
// THE AUTHORITY MARKER -- WHERE THE SERVER ACTUALLY IS.
//
// The client runs ahead of authority by the estimator's own prediction offset, and the
// tick it steers to IS authorityTick plus that offset. Subtracting the offset back off
// the prediction tick therefore names the authority tick, and every column right of it
// is prediction nobody has confirmed yet.
// ⛔ THE OFFSET IS READ, NEVER RE-DERIVED: it moves with RTT and jitter.
//
// A target with no cell of its own is the hard part, and there are three such cases: a
// tick inside a span the gate has collapsed, a tick inside the span it is collapsing
// right now, and a tick older than its ledger still reaches.
// ⛔ NEVER CLAMPED TO THE NEAREST RECORDED TICK -- that names a tick authority is not
//   at, in the one display built to say where authority is.
// ⛔ NEVER HIDDEN: silence reads as "no data" when the truth is "inside that span".
// ---------------------------------------------------------------------------

// What the authority tick turned out to be on the lane axis. The four answers that are
// not a plain cell are four different truths, and collapsing any of them would be a lie.
enum class AuthorityMarkerKind : uint8_t
{
	NoEstimate,      // this role does not predict, so there is no offset to point with
	OnCell,          // the tick has a lane cell of its own
	OnElidedSpan,    // inside a closed span the axis has no cells for, which owns one
	InsideOpenSpan,  // inside the span being collapsed right now, which owns none yet
	TooOldToPlace,   // the gate's elision ledger no longer reaches back that far
};

// Where the rule is anchored. ⛔ A TARGET WITH NO COLUMN IS NEVER MOVED ONTO ONE -- it
// flags the edge it fell off, which cannot be counted as a tick.
enum class AuthorityMarkerAnchor : uint8_t
{
	None,
	Column,
	LeftEdge,
	RightEdge,
};

struct FrameMeterAuthorityMarker
{
	AuthorityMarkerKind   kind          = AuthorityMarkerKind::NoEstimate;
	AuthorityMarkerAnchor anchor        = AuthorityMarkerAnchor::None;
	uint32_t              barOffset     = 0u;
	uint32_t              authorityTick = 0u;
	uint32_t              offsetTicks   = 0u;
};

// Opaque, white and three times the horizon's width: this is the one rule a reader counts
// cells from, and it clears the palette's own pair floor against every colour it covers.
inline constexpr FrameMeterMarkerStyle kFrameMeterAuthorityStyle{
	{ 1.00f, 1.00f, 1.00f }, 1.00f, 3.f, FrameMeterMarkerShape::LabelledRule };

// The same marker with its target off the bar. ⚠ A TARGET AT COLUMN 0 AND A TARGET
// OLDER THAN COLUMN 0 SHARE AN X, so half-lit and thinner is what keeps them apart.
inline constexpr FrameMeterMarkerStyle kFrameMeterAuthorityOffBarStyle{
	{ 1.00f, 1.00f, 1.00f }, 0.55f, 2.f, FrameMeterMarkerShape::LabelledRule };

// ⛔ ONE STYLE PER ANCHOR, decided here rather than at the canvas.
constexpr FrameMeterMarkerStyle authorityMarkerStyleOf(AuthorityMarkerAnchor anchor)
{
	return (anchor == AuthorityMarkerAnchor::Column) ? kFrameMeterAuthorityStyle
	                                                 : kFrameMeterAuthorityOffBarStyle;
}

// The column a lane tick occupies, or false when this window does not reach it.
inline bool frameMeterColumnOfLaneTick(uint32_t          laneTick,
                                       const PollWindow& window,
                                       uint32_t          cellCount,
                                       uint32_t&         outOffset)
{
	if (laneTick < window.oldestTick)
		return false;

	const uint32_t offset = laneTick - window.oldestTick;
	if (offset >= cellCount)
		return false;

	outOffset = offset;
	return true;
}

// A placed lane tick, or the edge it fell off. Both cases that own a cell share this.
inline void anchorFrameMeterMarker(uint32_t                laneTick,
                                   const PollWindow&        window,
                                   uint32_t                 cellCount,
                                   AuthorityMarkerAnchor&   anchor,
                                   uint32_t&                barOffset)
{
	if (frameMeterColumnOfLaneTick(laneTick, window, cellCount, barOffset))
	{
		anchor = AuthorityMarkerAnchor::Column;
		return;
	}

	anchor = (laneTick < window.oldestTick) ? AuthorityMarkerAnchor::LeftEdge
	                                        : AuthorityMarkerAnchor::RightEdge;
}

// ---------------------------------------------------------------------------
// THE PLACEMENT HELPER -- ONE SIM TICK TO LANE COLUMN, SHARED BY BOTH MARKERS.
//
// The authority marker and the frozen horizon both turn a sim tick into a column against
// the same gate and the same window; a second derivation here is the one thing this task
// exists to rule out. `AuthorityMarkerKind` is reused rather than cloned -- the caller
// decides what each answer MEANS, this decides only where it lands.
// ---------------------------------------------------------------------------
struct FrameMeterSimTickPlacement
{
	AuthorityMarkerKind   kind      = AuthorityMarkerKind::TooOldToPlace;
	AuthorityMarkerAnchor anchor    = AuthorityMarkerAnchor::LeftEdge;
	uint32_t              barOffset = 0u;
};

// Where `simTick` lands on the lane axis this window reads: a column of its own, a closed
// span's marker cell, the span still being collapsed, or too old for the ledger to place.
inline FrameMeterSimTickPlacement placeFrameMeterSimTick(const InputHistoryTickLanes& lanes,
                                                         const PollWindow&            window,
                                                         uint32_t                     simTick)
{
	FrameMeterSimTickPlacement placement;

	const uint32_t      cellCount = frameMeterCellCount(window);
	const LaneIdleGate& gate      = lanes.gate();

	const std::optional<uint32_t> laneTick = gate.laneTickOf(simTick);
	if (laneTick.has_value())
	{
		placement.kind = AuthorityMarkerKind::OnCell;
		anchorFrameMeterMarker(*laneTick, window, cellCount, placement.anchor, placement.barOffset);
		return placement;
	}

	// A closed span already occupies one cell, and that cell stands for exactly the
	// stretch of time this tick fell somewhere inside.
	bool pastEveryClosedSpan = true;
	for (std::size_t index = 0u; index < gate.axisEventCount(); ++index)
	{
		const LaneAxisEvent& span    = gate.axisEventAt(index);
		const uint32_t       spanEnd = span.simTick + span.skippedTicks;

		// A FORWARD resync's dead range -- the ticks the axis jumped over -- is a stretch the
		// axis has no cells for, exactly like an elided one, so its marker cell answers for it.
		// ⚠ A BACKWARD RESYNC LEAVES THIS RANGE EMPTY (begin runs past end) and is skipped.
		const uint32_t spanBegin = (span.kind == LaneAxisEventKind::Resync)
		                               ? span.fromSimTick + 1u
		                               : span.simTick;

		if (simTick < spanEnd)
			pastEveryClosedSpan = false;

		if (simTick < spanBegin || simTick >= spanEnd)
			continue;

		placement.kind = AuthorityMarkerKind::OnElidedSpan;
		anchorFrameMeterMarker(span.laneTick, window, cellCount, placement.anchor, placement.barOffset);
		return placement;
	}

	// The span still open has no ledger entry and no cell yet; its column will open to the
	// right of the newest one. ⛔ IT ONLY REACHES TICKS PAST EVERY SPAN ALREADY FILED --
	//   an older tick the ledger has dropped is unplaceable, not idle time in progress.
	if (gate.paused() && pastEveryClosedSpan)
	{
		placement.kind   = AuthorityMarkerKind::InsideOpenSpan;
		placement.anchor = AuthorityMarkerAnchor::RightEdge;
		return placement;
	}

	placement.kind   = AuthorityMarkerKind::TooOldToPlace;
	placement.anchor = AuthorityMarkerAnchor::LeftEdge;
	return placement;
}

// The marker for the lanes' OWN reading, resolved against the gate that owns their axis.
// An absent reading is a role that does not predict, and it draws nothing at all.
// ⛔ THE READING IS NOT A PARAMETER: it is the one the poll filed beside the axis tick.
inline FrameMeterAuthorityMarker frameMeterAuthorityMarkerOf(
	const InputHistoryTickLanes& lanes,
	const PollWindow&            window)
{
	FrameMeterAuthorityMarker marker;

	const std::optional<PredictionOffsetReading>& reading = lanes.authorityReading();
	if (!reading.has_value())
		return marker;

	marker.authorityTick = authorityTickOf(*reading);
	marker.offsetTicks   = reading->offsetTicks;

	const FrameMeterSimTickPlacement placement =
		placeFrameMeterSimTick(lanes, window, marker.authorityTick);

	marker.kind      = placement.kind;
	marker.anchor    = placement.anchor;
	marker.barOffset = placement.barOffset;
	return marker;
}

// Where a rule lands on x, from an anchor and (when it matters) a column: the one mapping
// both vertical markers share, so neither can quietly acquire its own.
inline float authorityMarkerXFor(const FrameMeterGeometry& geometry,
                                 AuthorityMarkerAnchor      anchor,
                                 uint32_t                   barOffset)
{
	switch (anchor)
	{
	case AuthorityMarkerAnchor::Column:
		return frameMeterCellX(geometry, barOffset);
	case AuthorityMarkerAnchor::RightEdge:
		return geometry.originX + frameMeterWidth(geometry);
	case AuthorityMarkerAnchor::LeftEdge:
	case AuthorityMarkerAnchor::None:
		break;
	}

	return geometry.originX;
}

// Where the rule is drawn. An edge anchor sits ON the bar's own edge, so it leaves the
// cell grid rather than claiming a column inside it.
inline float authorityMarkerX(const FrameMeterGeometry&        geometry,
                              const FrameMeterAuthorityMarker& marker)
{
	return authorityMarkerXFor(geometry, marker.anchor, marker.barOffset);
}

// The offset value goes BELOW the bars, where an elision count goes above them: the two
// numbers on this display mean different things and must never share a line.
inline float frameMeterAuthorityLabelTopY(const FrameMeterGeometry& geometry,
                                          const FrameMeterLayout&   layout)
{
	return geometry.originY + frameMeterHeight(geometry) + layout.backdropPadding;
}

// ---------------------------------------------------------------------------
// THE HORIZON, AS A RESIDENCY READING.
//
// The rule now names WHY it sits where it does rather than just where: the same
// classification the provenance lane's own write rule uses, mapped onto the placement
// helper above instead of a lane-cell count.
// ⛔ THE EDGE IS A SIM TICK, CONVERTED ONCE, HERE, THROUGH THE GATE -- never a lane tick minus 59.
// ---------------------------------------------------------------------------
enum class FrameMeterHorizonKind : uint8_t
{
	NoReading,        // nothing polled yet -- draw nothing
	NoCache,          // this role holds no correction cache -- RightEdge; the readout says why
	WholeBarLive,     // the edge is older than the oldest cell (TooOldToPlace) -- draw nothing
	OnCell,           // the edge has a column
	OnElidedSpan,     // the edge is inside a closed span -- that span's marker cell
	InsideOpenSpan,   // the edge is inside the span being collapsed now -- RightEdge: frozen
	WholeBarFrozen,   // nothing resident in the window (Unclassifiable) -- RightEdge
};

// For the sweep in the suite. A kind added without extending that sweep fails there.
inline constexpr uint8_t kFrameMeterHorizonKindCount = 7u;

struct FrameMeterHorizon
{
	FrameMeterHorizonKind kind        = FrameMeterHorizonKind::NoReading;
	AuthorityMarkerAnchor anchor      = AuthorityMarkerAnchor::None;   // REUSED, not a second enum
	uint32_t              barOffset   = 0u;
	uint32_t              edgeSimTick = 0u;
};

// The rules are applied IN THIS ORDER, exactly like classifyNoSlot's: an unset residency
// bound compared against would misclassify, so the guard causes are checked before anything
// derived from oldestResident is trusted.
inline FrameMeterHorizon frameMeterHorizonOf(const InputHistoryTickLanes& lanes,
                                             const PollWindow&            window)
{
	FrameMeterHorizon horizon;

	const std::optional<ResidencyReading>& reading = lanes.residencyReading();
	if (!reading.has_value())
		return horizon;

	const WindowResidency& residency = reading->residency;

	if (!residency.hasCache)
	{
		horizon.kind   = FrameMeterHorizonKind::NoCache;
		horizon.anchor = AuthorityMarkerAnchor::RightEdge;
		return horizon;
	}

	if (!residency.anyResident)
	{
		horizon.kind   = FrameMeterHorizonKind::WholeBarFrozen;
		horizon.anchor = AuthorityMarkerAnchor::RightEdge;
		return horizon;
	}

	horizon.edgeSimTick = residency.oldestResident;

	const FrameMeterSimTickPlacement placement =
		placeFrameMeterSimTick(lanes, window, residency.oldestResident);

	switch (placement.kind)
	{
	case AuthorityMarkerKind::OnCell:
		horizon.kind      = FrameMeterHorizonKind::OnCell;
		horizon.anchor    = placement.anchor;
		horizon.barOffset = placement.barOffset;
		break;
	case AuthorityMarkerKind::OnElidedSpan:
		horizon.kind      = FrameMeterHorizonKind::OnElidedSpan;
		horizon.anchor    = placement.anchor;
		horizon.barOffset = placement.barOffset;
		break;
	case AuthorityMarkerKind::InsideOpenSpan:
		horizon.kind   = FrameMeterHorizonKind::InsideOpenSpan;
		horizon.anchor = AuthorityMarkerAnchor::RightEdge;
		break;
	case AuthorityMarkerKind::TooOldToPlace:
	case AuthorityMarkerKind::NoEstimate:
		// NoEstimate is unreachable here -- placeFrameMeterSimTick never returns it, since it
		// takes a sim tick directly rather than an optional reading.
		horizon.kind   = FrameMeterHorizonKind::WholeBarLive;
		horizon.anchor = AuthorityMarkerAnchor::None;
		break;
	}

	return horizon;
}

// The horizon shares the authority marker's x mapping -- a RightEdge horizon and a
// RightEdge authority marker coincide in x by construction, which is why their four style
// fields, not position, are what keeps them apart.
inline float authorityMarkerX(const FrameMeterGeometry& geometry, const FrameMeterHorizon& horizon)
{
	return authorityMarkerXFor(geometry, horizon.anchor, horizon.barOffset);
}

// ---------------------------------------------------------------------------
// THE DELAY READOUT -- FACTS ONLY. The pure header decides what is true; the string
// that says so is built UE-side from these fields, the same split every other reading
// on this bar keeps.
// ---------------------------------------------------------------------------
struct InputDelayReadout
{
	bool                             present = false;
	InputDelayDecomposition          decomposition{};
	std::optional<InputDelayVerdict> newestVerdict;
	uint32_t                         divergedInWindow = 0u;
	bool                             formulaMismatch  = false;
	bool                             publishMismatch  = false;
};

// The newest FILLED cell's verdict, and how many cells in the window read as a real
// divergence. No reading at all -- the delay display is not being fed -- answers
// `present == false` and touches no other field.
inline InputDelayReadout buildInputDelayReadout(const InputHistoryTickLanes& lanes,
                                                const PollWindow&            window)
{
	InputDelayReadout readout;

	const std::optional<InputDelayReading>& reading = lanes.delayReading();
	if (!reading.has_value())
		return readout;

	readout.present         = true;
	readout.decomposition   = reading->decomposition;
	readout.formulaMismatch =
		reading->decomposition.effectiveTicks != reading->decomposition.formulaTicks;
	readout.publishMismatch =
		reading->decomposition.formulaTicks != reading->decomposition.publishedTicks;

	const uint32_t cellCount = frameMeterCellCount(window);
	for (uint32_t offset = 0u; offset < cellCount; ++offset)
	{
		const InputDelayCell* cell = lanes.delayCellAt(window.oldestTick + offset);
		if (cell == nullptr)
			continue;

		const InputDelayVerdict verdict = delayVerdictOf(*cell);
		if (verdict == InputDelayVerdict::NoVerdict)
			continue;

		// Offsets walk ascending, so the last one written is the newest filled cell.
		readout.newestVerdict = verdict;

		if (verdict == InputDelayVerdict::ServerLater
			|| verdict == InputDelayVerdict::ServerEarlier)
		{
			++readout.divergedInWindow;
		}
	}

	return readout;
}

// ONE FORMULA PLACES EVERY READOUT LINE under the bars, indexed from the offset label:
// line 0 sits one label's height and one padding below it, and each further line repeats
// that same gap -- so a second reading never has to invent its own offset from the first.
inline float frameMeterReadoutLineTopY(const FrameMeterGeometry& geometry,
                                       const FrameMeterLayout&   layout,
                                       float                     labelHeight,
                                       uint32_t                  lineIndex)
{
	return frameMeterAuthorityLabelTopY(geometry, layout)
	       + static_cast<float>(lineIndex + 1u) * (labelHeight + layout.backdropPadding);
}

// ⛔ KEPT BYTE-IDENTICAL TO ITS OLD SELF -- line 0, re-expressed through the formula above.
inline float frameMeterDelayReadoutTopY(const FrameMeterGeometry& geometry,
                                        const FrameMeterLayout&   layout,
                                        float                     labelHeight)
{
	return frameMeterReadoutLineTopY(geometry, layout, labelHeight, 0u);
}

// ---------------------------------------------------------------------------
// THE PROVENANCE RESIDENCY READOUT -- FACTS ONLY, the same split every other reading on
// this bar keeps: this header decides what is true, the string is built UE-side.
//
// Eviction and missing-in-window counts are NOT stored here -- a count that needs a home
// later gets one then; this reads residency facts only.
// ---------------------------------------------------------------------------
struct ProvenanceResidencyReadout
{
	bool     present        = false;   // no reading -- draw nothing
	bool     hasCache       = false;
	bool     anyResident    = false;
	uint32_t oldestResident = 0u;      // SIM ticks -- the text must say "sim"
	uint32_t newestResident = 0u;
	uint32_t residentCount  = 0u;      // newest - oldest + 1 when anyResident, else 0
};

// The lanes' own residency reading, restated as what a reader needs to print. No reading
// at all -- the provenance bar is not being fed -- answers `present == false` and touches
// no other field.
inline ProvenanceResidencyReadout buildProvenanceResidencyReadout(const InputHistoryTickLanes& lanes)
{
	ProvenanceResidencyReadout readout;

	const std::optional<ResidencyReading>& reading = lanes.residencyReading();
	if (!reading.has_value())
		return readout;

	readout.present     = true;
	readout.hasCache    = reading->residency.hasCache;
	readout.anyResident = reading->residency.anyResident;

	if (readout.anyResident)
	{
		readout.oldestResident = reading->residency.oldestResident;
		readout.newestResident = reading->residency.newestResident;
		readout.residentCount  = readout.newestResident - readout.oldestResident + 1u;
	}

	return readout;
}

// ---------------------------------------------------------------------------
// THE CLOCK READOUT -- FACTS ONLY, the same split every other reading on this bar keeps.
//
// A skip and a stall are RATES on a continuous axis, not events: both ticks of a skip get
// cells and a stall repeats a frame, so neither can be drawn as a marker. What CAN be
// drawn is the drift state that decides them, plus the one fact a single reading cannot
// carry -- how long the authority tick has stood still, which is what a resync storm
// looks like from the client.
// ⛔ authorityStaticTicks IS IN SIM TICKS, NOT POLLS: polls are render frames.
// ---------------------------------------------------------------------------
struct ClockDriftReadout
{
	bool              present = false;   // no reading -- draw nothing
	ClockDriftReading reading{};
	uint32_t          authorityStaticTicks = 0u;   // ticks SIMULATED since authority moved
};

// The lanes' own clock reading, restated as what a reader needs to print. No reading at
// all -- the authority role, or nothing polled yet -- answers `present == false` and
// touches no other field, exactly as the delay and residency readouts do.
inline ClockDriftReadout buildClockDriftReadout(const InputHistoryTickLanes& lanes)
{
	ClockDriftReadout readout;

	const std::optional<ClockDriftReading>& reading = lanes.clockDriftReading();
	if (!reading.has_value())
		return readout;

	readout.present              = true;
	readout.reading              = *reading;
	readout.authorityStaticTicks = lanes.authorityStaticSimTicks();

	return readout;
}

// ---------------------------------------------------------------------------
// THE RUN-LENGTH LABEL. It goes on the run's LAST cell and is allowed to overlap its
// neighbours -- the reference does exactly that, and shrinking cells to fit a number
// would cost the one-cell-per-tick property the whole display is for.
// ---------------------------------------------------------------------------
inline constexpr float kLaneLabelDigitWidth = 6.f;

constexpr uint32_t decimalDigitCount(uint32_t value)
{
	uint32_t digits = 1u;
	while (value >= 10u)
	{
		value /= 10u;
		++digits;
	}

	return digits;
}

// A label is suppressed only when its own run is narrower than the number itself -- at
// the default stride that never happens, and on a shrunken bar it hits the shortest runs.
inline bool runLabelFits(const FrameMeterGeometry& geometry, const LaneRun& run)
{
	const float labelWidth =
		static_cast<float>(decimalDigitCount(run.length)) * kLaneLabelDigitWidth;

	return static_cast<float>(run.length) * geometry.cellStride >= labelWidth;
}

// Centre of the run's last cell. The renderer subtracts half the measured text width,
// which is the one number a font knows and this header does not.
inline float runLabelCenterX(const FrameMeterGeometry& geometry, const LaneRun& run)
{
	return frameMeterCellX(geometry, run.lastOffset) + geometry.cellWidth * 0.5f;
}

} // namespace brawlerInputHistoryVisualization

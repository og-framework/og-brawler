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
#include <cmath>
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

// ---------------------------------------------------------------------------
// THE RELAY-HEALTH PALETTE -- SEVEN COLOURS PLUS A HOLE.
//
// "I found the input I was looking for" is the same claim on both bars, and two greens a
// shade apart would be read as two different claims. It is the one deliberate collision
// between two of this meter's palettes, and a case pins the identity rather than the gap.
// ⭐ `Hit` IS THE DELAY BAR'S `Agree` GREEN, CHANNEL FOR CHANNEL AND ON PURPOSE.
//
// Every other colour here clears the IN-PALETTE floor against the DELAY bar directly
// above it as well, so the one collision stays the one that was chosen.
//
// The design asks for dark grey twice and red once, and none of the three is reachable:
// Unknown (0.42), RanUnconfirmed (0.88) and the horizon rule (0.55) hold the grey axis
// while the machine bar's Idle teal holds its dark end, and the dark-red axis is bracketed
// by Attacking and the delay bar's NoCaptureNamed. They are desaturated and re-hued
// instead; the two inert states never share a bar, since `LocalNoRelay` fills the whole
// window or none of it.
// ⚠ NEITHER INERT STATE IS GREY, AND `FallbackNeverArrived` IS NOT RED.
// ---------------------------------------------------------------------------
constexpr LaneCellStyle relayReadVerdictStyleOf(RelayReadVerdict verdict)
{
	switch (verdict)
	{
	case RelayReadVerdict::NoVerdict:
		return { LaneCellFill::Hole, {} };
	case RelayReadVerdict::Hit:
		return { LaneCellFill::State, { 0.00f, 0.84f, 0.00f } };
	case RelayReadVerdict::Neutral:
		return { LaneCellFill::State, { 0.30f, 0.08f, 0.30f } };
	case RelayReadVerdict::LocalNoRelay:
		return { LaneCellFill::State, { 0.22f, 0.15f, 0.04f } };
	case RelayReadVerdict::FallbackPending:
		return { LaneCellFill::State, { 0.82f, 0.84f, 0.00f } };
	case RelayReadVerdict::FallbackArrivedReplayable:
		return { LaneCellFill::State, { 0.00f, 0.29f, 0.79f } };
	case RelayReadVerdict::FallbackArrivedTooLate:
		return { LaneCellFill::State, { 0.93f, 0.40f, 0.35f } };
	case RelayReadVerdict::FallbackNeverArrived:
		return { LaneCellFill::State, { 0.60f, 0.00f, 0.35f } };
	}

	return { LaneCellFill::Unnamed, kUnnamedLaneColor };
}

// Asked by ordinal, matching the three tables above -- one table each, forwarded here.
constexpr LaneCellStyle relayReadVerdictStyleOfOrdinal(uint8_t ordinal)
{
	return relayReadVerdictStyleOf(static_cast<RelayReadVerdict>(ordinal));
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

	// A second ordinal a bar may put on its cells, joined into run identity below. Every
	// bar that has nothing to say here leaves it 0 and its runs are exactly what they were.
	// ⛔ IT IS NOT A COLOUR: `styleOf` never sees it.
	uint8_t label  = 0u;
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
	RelayHealth,
};

inline constexpr uint8_t kFrameMeterBarKindCount = 4u;

// Every bar defaults ON; the UE-side gates flip these before geometry is asked for
// anything.
// The other three bars describe the character the stack is about whoever it is; this one
// describes a relay, which only a stack following someone else's character has, so the
// draw site turns it on for that stack alone.
// ⚠ `relayHealth` DEFAULTS OFF.
struct FrameMeterBarSelection
{
	bool provenance     = true;
	bool inputDelay     = true;
	bool characterState = true;
	bool relayHealth    = false;
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
	case FrameMeterBarKind::RelayHealth:
		return selection.relayHealth;
	}

	return false;
}

// How many bars are switched on, 0..4.
constexpr uint32_t frameMeterEnabledBarCount(const FrameMeterBarSelection& selection)
{
	return static_cast<uint32_t>(selection.provenance)
	     + static_cast<uint32_t>(selection.inputDelay)
	     + static_cast<uint32_t>(selection.characterState)
	     + static_cast<uint32_t>(selection.relayHealth);
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

// What a labelled run says. Two bars say HOW LONG the run is; the relay bar says WHY it
// fell back, because a fallback's cause is the thing a reader cannot get from the colour.
// R3: no run labels on the input-delay bar -- its runs are short and sparse, and the
// numbers would read as noise.
enum class FrameMeterRunLabel : uint8_t
{
	None,
	RunLength,
	CauseLetter,
};

// ⛔ THE ONLY PLACE A BAR'S LABEL VOCABULARY IS DECIDED.
constexpr FrameMeterRunLabel frameMeterRunLabelOf(FrameMeterBarKind kind)
{
	switch (kind)
	{
	case FrameMeterBarKind::Provenance:     return FrameMeterRunLabel::RunLength;
	case FrameMeterBarKind::InputDelay:     return FrameMeterRunLabel::None;
	case FrameMeterBarKind::CharacterState: return FrameMeterRunLabel::RunLength;
	case FrameMeterBarKind::RelayHealth:    return FrameMeterRunLabel::CauseLetter;
	}

	return FrameMeterRunLabel::None;
}

// ⛔ RE-EXPRESSED THROUGH THE TABLE ABOVE, never a second answer to the same question.
constexpr bool frameMeterBarDrawsRunLabels(FrameMeterBarKind kind)
{
	return frameMeterRunLabelOf(kind) != FrameMeterRunLabel::None;
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

// The relay-health bar over the SAME window. `isLocallyControlled` is the ONE locality
// test this display makes, taken at the call site and passed in: a character this client
// controls resolves no relayed read at all, and every tick of its window says so rather
// than reading as a window nothing has polled.
// ⛔ A LOCAL CHARACTER'S BAR IS FULL, NOT EMPTY -- the stack's height never moves.
inline void readRelayHealthBar(const InputHistoryTickLanes& lanes,
                               const PollWindow&            window,
                               bool                         isLocallyControlled,
                               FrameMeterBarCells&          bar)
{
	bar.count = frameMeterCellCount(window);

	for (uint32_t offset = 0u; offset < bar.count; ++offset)
	{
		const RelayHealthCell* cell = lanes.relayHealthCellAt(window.oldestTick + offset);

		const RelayReadVerdict verdict =
			(cell != nullptr)     ? cell->verdict
			: isLocallyControlled ? RelayReadVerdict::LocalNoRelay
			                      : RelayReadVerdict::NoVerdict;

		bar.cells[offset].filled = (verdict != RelayReadVerdict::NoVerdict);
		bar.cells[offset].value  = static_cast<uint8_t>(verdict);
		bar.cells[offset].label  = static_cast<uint8_t>(
			(cell != nullptr) ? cell->missLabel : RelayMissLabel::None);
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

	// The cells' own `label`, which is part of what made them one run. ⛔ LAST FIELD, so
	//   every existing brace-initialised run keeps meaning what it meant.
	uint8_t  label       = 0u;
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

		// ⛔ THE LABEL IS PART OF RUN IDENTITY. A run is what one label can truthfully be
		//   printed on, so two neighbours of one colour with different labels are two runs.
		const bool extendsPrevious = out.count != 0u
			&& out.runs[out.count - 1u].lastOffset + 1u == offset
			&& out.runs[out.count - 1u].value == cell.value
			&& out.runs[out.count - 1u].label == cell.label;

		if (extendsPrevious)
		{
			LaneRun& run   = out.runs[out.count - 1u];
			run.lastOffset = offset;
			++run.length;
			continue;
		}

		out.runs[out.count] = LaneRun{ offset, offset, 1u, cell.value, cell.label };
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

// ---------------------------------------------------------------------------
// TWO STACKS, ONE ANCHOR -- AND THE ANCHORED ONE IS NOT THE PRIMARY.
//
// The meter is anchored to the BOTTOM and its own label band plus readout rows already
// spend the bottom margin, so there is no room underneath it for a second stack: about
// 38 px at 1080p and 6 px at 720p. The second stack therefore takes TODAY'S ANCHOR and
// the primary is LIFTED off it by one whole stack plus a gap -- which is also why the
// eye keeps the bottom position for the character being watched.
//
// The lift is a separate step applied to that function's answer, so "one stack draws
// exactly what it drew before" is a property of the SHAPE, not of an argument being 0.
// ⛔ `frameMeterGeometryFor` IS NOT TOUCHED BY ANY OF THIS.
// ---------------------------------------------------------------------------

// The clear space between the lifted stack's lowest readout and the anchored stack's
// own label band.
inline constexpr float kFrameMeterStackGap = 8.f;

// The same geometry, raised by `liftPixels`. Nothing but the origin moves: both stacks
// keep one stride, one cell width and one bar height, so a column means the same width
// of screen on either of them.
inline FrameMeterGeometry frameMeterLiftedBy(FrameMeterGeometry geometry, float liftPixels)
{
	geometry.originY -= liftPixels;
	return geometry;
}

// How tall one whole stack draws, from its elision label band down to the bottom of its
// last readout line. DERIVED FROM THE PLACEMENT HELPERS THEMSELVES rather than restated,
// so a band that moves moves this with it:
//
//   `frameMeterElisionLabelTopY`  puts the top one padding and one label above originY
//   `frameMeterHeight`            is the bars
//   `frameMeterAuthorityLabelTopY` puts the offset label one padding below them
//   `frameMeterReadoutLineTopY`   repeats a label plus a padding per readout line
//
// `labelHeight` is the font's measured line height, which only the draw site knows.
// ⛔ MEASURED ONCE AND PASSED IN: two measures can differ by a pixel, and overlap by it.
inline float frameMeterStackHeight(const FrameMeterLayout& layout,
                                   uint32_t                barCount,
                                   uint32_t                readoutLines,
                                   float                   labelHeight)
{
	FrameMeterGeometry bars;
	bars.barHeight = layout.barHeight;
	bars.barGap    = layout.barGap;
	bars.barCount  = barCount;

	return layout.backdropPadding + labelHeight
	     + frameMeterHeight(bars)
	     + layout.backdropPadding + labelHeight
	     + static_cast<float>(readoutLines) * (labelHeight + layout.backdropPadding);
}

// How far the PRIMARY stack is lifted so the nearest one can take its anchor.
//
// The lift moves this stack's entire extent -- the readouts hanging below its origin as
// well as the label band above it -- so it must clear the ANCHORED stack's TOP EDGE, and
// the two stacks reach that edge from different places:
//
//   * the lifted stack contributes its OWN whole height, because that is what hangs below
//     the point the lift moves. It is ITS OWN HEIGHT AND NEVER THE OTHER STACK'S, since
//     the two do not draw the same number of readout lines -- the clock is on the primary.
//   * the anchored stack contributes the difference between the two BAR BANDS, because
//     `frameMeterGeometryFor` anchors an origin to the viewport's bottom margin and then
//     puts the bars ABOVE it. A taller bar band therefore raises the anchored stack's top
//     without moving its bottom, and a lift blind to that leaves the two overlapping by
//     exactly that difference.
//
// ⚠ THE TWO STACKS DO NOT DRAW THE SAME NUMBER OF BARS EITHER: the relay-health bar is
//   on the stack that is following someone else's character, and on no other.
inline float frameMeterPrimaryLift(const FrameMeterLayout& layout,
                                   uint32_t                liftedBarCount,
                                   uint32_t                liftedReadoutLines,
                                   float                   labelHeight,
                                   uint32_t                anchoredBarCount)
{
	FrameMeterGeometry liftedBars;
	liftedBars.barHeight = layout.barHeight;
	liftedBars.barGap    = layout.barGap;
	liftedBars.barCount  = liftedBarCount;

	FrameMeterGeometry anchoredBars = liftedBars;
	anchoredBars.barCount = anchoredBarCount;

	return frameMeterStackHeight(layout, liftedBarCount, liftedReadoutLines, labelHeight)
	     + (frameMeterHeight(anchoredBars) - frameMeterHeight(liftedBars))
	     + kFrameMeterStackGap;
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
	SignedTickMark, // a short mark above the bars, crossing no cell, with a signed glyph
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

// The two whole-stack extents, stated HERE because the top edge is this label band --
// the highest thing a stack puts on screen -- and a second expression for it could
// drift from the band itself.
// The top edge of a stack drawn at `geometry` -- its elision label band, which is the
// highest thing it puts on screen. The one number a clipping check needs.
inline float frameMeterStackTopY(const FrameMeterGeometry& geometry,
                                 const FrameMeterLayout&   layout,
                                 float                     labelHeight)
{
	return frameMeterElisionLabelTopY(geometry, layout, labelHeight);
}

// The bottom edge of a stack drawn at `geometry` with `readoutLines` readouts -- the
// baseline-plus-height of its last readout line, which is the lowest thing it draws.
inline float frameMeterStackBottomY(const FrameMeterGeometry& geometry,
                                    const FrameMeterLayout&   layout,
                                    uint32_t                  readoutLines,
                                    float                     labelHeight)
{
	return frameMeterStackTopY(geometry, layout, labelHeight)
	     + frameMeterStackHeight(layout, geometry.barCount, readoutLines, labelHeight);
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
// THE RATE MARKS -- WHERE THE CLOCK INSERTED A TICK OR WITHHELD ONE.
//
// A skip and a stall correct the RATE time arrives at rather than the contents of a tick,
// so what either leaves behind is a BOUNDARY between two columns and not a cell. The mark
// sits on a column edge and consumes no lane tick.
//
// The two kinds sit on OPPOSITE EDGES of their column, and the reason is physical:
//   * a skip at P arrived together with P-1, which is a backfilled copy, so the jump is on
//     the LEFT edge of P-1's column -- between P-2 and P-1, where two ticks landed at once;
//   * a stall at T is a step that passed with no tick at all, so it is on the RIGHT edge of
//     T's column -- between T and the tick after it, which did not arrive.
//
// The glyph is the sign of the tick DISPLACEMENT -- inserted is plus, withheld is minus --
// which is the resync marker's own convention, so one sign reads across every correction
// this axis can take.
// The gate's four answers map onto the bar as: a cell of its own takes that cell's edge; a
// closed span takes the span's own cell, on its left edge, whichever kind landed there; and
// the span still being collapsed, or a tick older than the ledger reaches, is not placed.
// ⛔ THE COLUMN COMES OFF placeFrameMeterSimTick, NEVER OFF A LANE TICK MINUS ONE.
// ⚠ A mark inside the span being collapsed now is dropped: only the readout counts it.
// ---------------------------------------------------------------------------

// A short mark in the label band above the backdrop, beside its glyph, crossing no cell --
// which is why it needs no palette clearance of its own against the colours below it.
// ⛔ THE COLOUR IS NAMED, NEVER COPIED: a rate mark and a resync are one clock's two grades.
inline constexpr FrameMeterMarkerStyle kFrameMeterRateMarkStyle{
	kLaneResyncColor, 0.80f, 4.f, FrameMeterMarkerShape::SignedTickMark };

// One placed boundary: the column it borders, WHICH edge of that column, and how many
// corrections of this kind the poll that filed it saw.
struct FrameMeterRateMark
{
	RateMarkKind kind      = RateMarkKind::Skip;
	uint32_t     offset    = 0u;
	bool         rightEdge = false;
	uint32_t     count     = 1u;
};

struct FrameMeterRateMarkList
{
	std::array<FrameMeterRateMark, kRateMarkLedgerCapacity> marks{};
	uint32_t                                                count = 0u;
};

// The ledger's marks this window can place, as column edges. A mark whose tick has no
// column here is left out entirely rather than moved onto one that is not its own.
inline void collectFrameMeterRateMarks(const InputHistoryTickLanes& lanes,
                                       const PollWindow&            window,
                                       FrameMeterRateMarkList&      out)
{
	out.count = 0u;

	for (std::size_t index = 0u; index < lanes.rateMarkCount(); ++index)
	{
		const RateMark& mark = lanes.rateMarkAt(index);

		const bool stall = (mark.kind == RateMarkKind::Stall);

		// ⛔ THE FIRST TICK OF ALL HAS NO PREDECESSOR to hang a skip's left edge on.
		if (!stall && mark.simTick == 0u)
			continue;

		const uint32_t placedTick = stall ? mark.simTick : (mark.simTick - 1u);

		const FrameMeterSimTickPlacement placement =
			placeFrameMeterSimTick(lanes, window, placedTick);

		// ⛔ ONLY A COLUMN IS PLACED: the span still open, a tick past the ledger and a
		//   tick this window has scrolled past all anchor on an EDGE, and none is moved on.
		if (placement.anchor != AuthorityMarkerAnchor::Column)
			continue;

		const bool onSpan = (placement.kind == AuthorityMarkerKind::OnElidedSpan);

		// A closed span's one cell stands for the whole stretch that fell inside it, so a
		// mark placed there takes that cell's left edge rather than a boundary of its own.
		out.marks[out.count] = FrameMeterRateMark{ mark.kind, placement.barOffset,
			stall && !onSpan, mark.count };
		++out.count;
	}
}

// Where the mark is drawn. The two edges of one column are exactly one stride apart, which
// is what "opposite edges" means in x and the whole reason the kinds cannot be confused.
inline float rateMarkX(const FrameMeterGeometry& geometry, const FrameMeterRateMark& mark)
{
	return frameMeterCellX(geometry, mark.offset)
	       + (mark.rightEdge ? geometry.cellStride : 0.f);
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
// A skip and a stall own no cell: both ticks of a skip get one and a stall repeats a
// frame, so what either leaves is a boundary between two columns rather than a column.
// The line therefore carries the drift state that decides them, the one fact a single
// reading cannot carry -- how long the authority tick has stood still, which is what a
// resync storm looks like from the client -- and the counts of the boundaries themselves,
// which outlive the marks that scroll off the bar.
// ⛔ authorityStaticTicks IS IN SIM TICKS, NOT POLLS: polls are render frames.
// ---------------------------------------------------------------------------
struct ClockDriftReadout
{
	bool              present = false;   // no reading -- draw nothing
	ClockDriftReading reading{};
	uint32_t          authorityStaticTicks = 0u;   // ticks SIMULATED since authority moved

	// Clock events since this display's FIRST reading, never the clock's own totals: it
	// was counting before the display existed. A mark that scrolled off survives here.
	uint32_t skips   = 0u;
	uint32_t stalls  = 0u;
	uint32_t resyncs = 0u;
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
	readout.skips                = lanes.clockEventCounts().skips;
	readout.stalls               = lanes.clockEventCounts().stalls;
	readout.resyncs              = lanes.clockEventCounts().resyncs;

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

// A label is suppressed only when its own run is narrower than the label itself -- at
// the default stride that never happens, and on a shrunken bar it hits the shortest runs.
// ⛔ IN CHARACTERS, because not every bar labels a run with a number.
inline bool runLabelFits(const FrameMeterGeometry& geometry, const LaneRun& run,
                         uint32_t labelCharacters)
{
	const float labelWidth = static_cast<float>(labelCharacters) * kLaneLabelDigitWidth;

	return static_cast<float>(run.length) * geometry.cellStride >= labelWidth;
}

// ⛔ THE RUN-LENGTH LABEL'S OWN WIDTH, re-expressed through the test above.
inline bool runLabelFits(const FrameMeterGeometry& geometry, const LaneRun& run)
{
	return runLabelFits(geometry, run, decimalDigitCount(run.length));
}

// Centre of the run's last cell. The renderer subtracts half the measured text width,
// which is the one number a font knows and this header does not.
inline float runLabelCenterX(const FrameMeterGeometry& geometry, const LaneRun& run)
{
	return frameMeterCellX(geometry, run.lastOffset) + geometry.cellWidth * 0.5f;
}

// ---------------------------------------------------------------------------
// WHO THE SECOND STACK DRAWS -- THE NEAREST BRAWLER TO THE FIRST LOCAL ONE.
//
// Nearest by XY distance between capsule positions, because a brawler directly above
// another is not the one you are fighting. The choice is made every frame from
// positions that move every frame, so two candidates at nearly equal distance would
// swap the whole stack back and forth: the previous choice is KEPT unless another is
// closer by a clear margin.
//
// A client can run several brawlers of its own, and "the one nearest me" is still the
// right answer for each; locality decides where the DELAY bar's client half comes from.
// ⛔ GEOMETRIC ONLY -- it does not skip a locally controlled candidate.
//
// ⛔ THE ORDER IS TOTAL: equal distances are broken by the lower character id, so the
//   answer does not depend on the order the candidates were gathered in.
// ---------------------------------------------------------------------------

// How much closer a rival must be before the stack switches to it.
inline constexpr float kNearestHysteresisCm = 50.f;

// How many characters one selection considers. A match this display is used on is a
// handful of brawlers; beyond this the gather simply stops adding.
inline constexpr std::size_t kNearestCandidateCapacity = 16u;

struct NearestCharacterCandidate
{
	unsigned int id = 0u;
	float        x  = 0.f;
	float        y  = 0.f;
};

struct NearestCharacterCandidateList
{
	std::array<NearestCharacterCandidate, kNearestCandidateCapacity> candidates{};
	std::size_t                                                      count = 0u;

	// ⛔ FULL IS A REFUSAL, NEVER AN OVERWRITE: dropping the newest candidate loses one
	//   brawler from the selection, dropping an old one loses whichever it landed on.
	void add(const NearestCharacterCandidate& candidate)
	{
		if (count < kNearestCandidateCapacity)
		{
			candidates[count] = candidate;
			++count;
		}
	}
};

// Squared XY distance. Squared, because the SELECTION only ever orders distances and a
// square root would buy nothing but a rounding difference; the hysteresis below is the
// one place a real distance is needed, and it takes the root there.
constexpr float nearestPlanarDistanceSquared(const NearestCharacterCandidate& left,
                                             const NearestCharacterCandidate& right)
{
	const float dx = left.x - right.x;
	const float dy = left.y - right.y;
	return dx * dx + dy * dy;
}

// The brawler nearest `localId`, or nullopt when `localId` is not among the candidates
// or is the only one there.
//
// `previousChoice` is last frame's answer. It is kept whenever it is still a candidate,
// unless the closest rival is nearer by at least `kNearestHysteresisCm`.
// ⛔ THE MARGIN IS ON THE DISTANCE, NOT ITS SQUARE: a squared one varies with range.
inline std::optional<unsigned int> nearestCharacterIdTo(
	const NearestCharacterCandidateList& candidates,
	unsigned int                         localId,
	std::optional<unsigned int>          previousChoice)
{
	const NearestCharacterCandidate* local = nullptr;
	for (std::size_t index = 0u; index < candidates.count; ++index)
	{
		if (candidates.candidates[index].id == localId)
		{
			local = &candidates.candidates[index];
			break;
		}
	}

	if (local == nullptr)
		return std::nullopt;

	std::optional<unsigned int> best;
	float                       bestDistanceSquared     = 0.f;
	bool                        previousIsCandidate     = false;
	float                       previousDistanceSquared = 0.f;

	for (std::size_t index = 0u; index < candidates.count; ++index)
	{
		const NearestCharacterCandidate& candidate = candidates.candidates[index];

		// ⛔ THE LOCAL CHARACTER IS NOT ITS OWN NEIGHBOUR: at distance 0 it wins every time.
		if (candidate.id == localId)
			continue;

		const float distanceSquared = nearestPlanarDistanceSquared(*local, candidate);

		if (previousChoice.has_value() && candidate.id == *previousChoice)
		{
			previousIsCandidate     = true;
			previousDistanceSquared = distanceSquared;
		}

		// Strictly closer, or the same distance with the lower id: one total order, so
		// the gather's own order cannot change the answer.
		if (!best.has_value() || distanceSquared < bestDistanceSquared
			|| (distanceSquared == bestDistanceSquared && candidate.id < *best))
		{
			best                = candidate.id;
			bestDistanceSquared = distanceSquared;
		}
	}

	if (!best.has_value())
		return std::nullopt;

	if (previousIsCandidate)
	{
		const float previousDistance = std::sqrt(previousDistanceSquared);
		const float bestDistance     = std::sqrt(bestDistanceSquared);

		if (previousDistance - bestDistance < kNearestHysteresisCm)
			return previousChoice;
	}

	return best;
}

// The XY distance between two candidates, or nullopt when either is missing from the
// list. ⛔ THE ONE PLACE A REAL DISTANCE IS TAKEN besides the hysteresis margin, and it
//   is taken for the READER, never for the ordering above.
inline std::optional<float> nearestCharacterDistanceCm(
	const NearestCharacterCandidateList& candidates, unsigned int fromId, unsigned int toId)
{
	const NearestCharacterCandidate* from = nullptr;
	const NearestCharacterCandidate* to   = nullptr;

	for (std::size_t index = 0u; index < candidates.count; ++index)
	{
		if (candidates.candidates[index].id == fromId)
			from = &candidates.candidates[index];
		if (candidates.candidates[index].id == toId)
			to = &candidates.candidates[index];
	}

	if (from == nullptr || to == nullptr)
		return std::nullopt;

	return std::sqrt(nearestPlanarDistanceSquared(*from, *to));
}

// ---------------------------------------------------------------------------
// THE TWO STACK HEADERS -- WHICH CHARACTER EACH STACK IS ABOUT.
//
// The primary is always the first local character; the second stack's id CHANGES as the
// fight moves, and a bar whose subject is unnamed is a bar whose readings cannot be
// attributed. Both stacks are therefore labelled, and the second carries the distance
// it was chosen on and whether it is one this client controls -- which is what says
// where its delay bar's client half came from.
// ⛔ FACTS ONLY. The word, the format and the ink are the renderer's.
// ---------------------------------------------------------------------------
struct FrameMeterStackHeader
{
	bool         isNearestStack = false;
	unsigned int characterId    = 0u;

	// Metres, and only meaningful on the nearest stack.
	float        distanceMeters = 0.f;

	// Whether this client holds a capture line for the character -- the same test the
	// row poll makes. ⛔ NOT A ROLE TEST: a client can control several brawlers.
	bool         isLocallyControlled = false;
};

// ---------------------------------------------------------------------------
// THE RELAY READOUT -- what the second stack shows where the primary shows its tier
// and effective-delay decomposition.
//
// A tier is a property of the LOCAL connection and says nothing about a remote proxy.
// What does say something is the relay this client is predicting that proxy from: the
// schedule stamp its newest arrival carried, and how the scheduled read has been going.
// ⛔ FACTS ONLY, the same split every other reading on this bar keeps.
// ⛔ THE TALLY IS OVER THE OBSERVATIONS THE RING STILL HOLDS, never a session total:
//   a run of misses that has scrolled out is not a run of misses that is happening.
// ---------------------------------------------------------------------------
struct RelayReadReadout
{
	bool     present = false;   // no reading -- draw nothing

	// The schedule stamp of the newest observation, and whether there was one at all.
	bool     dLatestKnown = false;
	uint32_t dLatest      = 0u;

	uint32_t hits        = 0u;
	uint32_t misses      = 0u;
	uint32_t verifyFails = 0u;

	// Reads that formed no probe at all: nothing had ever arrived for this character.
	uint32_t noProbes = 0u;
};

// ---------------------------------------------------------------------------
// THE RELAY-HEALTH READOUT -- the four rungs of the bar above it, counted, and how late
// the arrivals were. FACTS ONLY; the string is built UE-side, as every reading here is.
//
// A stack that grew a readout row where another stack has none would stop being the same
// shape, and the pair only just fits at 720p as it is.
// ⛔ IT SHARES THE RELAY READING'S ONE LINE AND NEVER CLAIMS A SECOND.
// ⛔ A TALLY OVER THE DISPLAYED WINDOW, the same window the bar draws -- so the numbers
//   under the bar and the cells in it are one reading.
// ---------------------------------------------------------------------------
struct RelayHealthReadout
{
	bool present = false;   // no cell in the window -- draw nothing
	// This client controls the character, so there is no relay to be healthy or not.
	bool local   = false;

	uint32_t hits              = 0u;
	uint32_t neutral           = 0u;
	uint32_t fallbackPending   = 0u;
	uint32_t arrivedReplayable = 0u;
	uint32_t arrivedTooLate    = 0u;
	uint32_t neverArrived      = 0u;

	// The four causes, spelled as the bar's own run labels spell them.
	uint32_t loss    = 0u;
	uint32_t starved = 0u;
	uint32_t evicted = 0u;
	uint32_t verify  = 0u;

	// The MEDIAN of the arrived cells' lateness. ⛔ ABSENT WHEN NOTHING ARRIVED, never 0:
	//   zero ticks late is a real and common answer.
	bool    medianLatenessKnown = false;
	uint8_t medianLatenessTicks = 0u;
};

// Lateness is one byte, so 256 buckets answer exactly and the walk is bounded by the
// alphabet rather than by the window.
// ⭐ A MEDIAN FROM A HISTOGRAM, NOT A SORT.
inline RelayHealthReadout buildRelayHealthReadout(const InputHistoryTickLanes& lanes,
                                                  const PollWindow&            window,
                                                  bool                         isLocallyControlled)
{
	RelayHealthReadout readout;

	if (isLocallyControlled)
	{
		readout.present = true;
		readout.local   = true;
		return readout;
	}

	std::array<uint16_t, 256> latenessCounts{};
	uint32_t                  arrivedCells = 0u;

	const uint32_t cellCount = frameMeterCellCount(window);
	for (uint32_t offset = 0u; offset < cellCount; ++offset)
	{
		const RelayHealthCell* cell = lanes.relayHealthCellAt(window.oldestTick + offset);
		if (cell == nullptr)
			continue;

		readout.present = true;

		switch (cell->verdict)
		{
		case RelayReadVerdict::Hit:                       ++readout.hits;              break;
		case RelayReadVerdict::Neutral:                   ++readout.neutral;           break;
		case RelayReadVerdict::FallbackPending:           ++readout.fallbackPending;   break;
		case RelayReadVerdict::FallbackArrivedReplayable: ++readout.arrivedReplayable; break;
		case RelayReadVerdict::FallbackArrivedTooLate:    ++readout.arrivedTooLate;    break;
		case RelayReadVerdict::FallbackNeverArrived:      ++readout.neverArrived;      break;
		case RelayReadVerdict::NoVerdict:
		case RelayReadVerdict::LocalNoRelay:                                           break;
		}

		switch (cell->missLabel)
		{
		case RelayMissLabel::Loss:    ++readout.loss;    break;
		case RelayMissLabel::Starved: ++readout.starved; break;
		case RelayMissLabel::Evicted: ++readout.evicted; break;
		case RelayMissLabel::Verify:  ++readout.verify;  break;
		case RelayMissLabel::None:                       break;
		}

		if (cell->verdict == RelayReadVerdict::FallbackArrivedReplayable
			|| cell->verdict == RelayReadVerdict::FallbackArrivedTooLate)
		{
			++latenessCounts[cell->latenessTicks];
			++arrivedCells;
		}
	}

	if (arrivedCells != 0u)
	{
		const uint32_t half = arrivedCells / 2u;
		uint32_t       seen = 0u;
		for (uint32_t bucket = 0u; bucket < 256u; ++bucket)
		{
			seen += latenessCounts[bucket];
			if (seen > half)
			{
				readout.medianLatenessKnown = true;
				readout.medianLatenessTicks = static_cast<uint8_t>(bucket);
				break;
			}
		}
	}

	return readout;
}

} // namespace brawlerInputHistoryVisualization

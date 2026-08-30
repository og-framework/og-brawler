#pragma once
// SPDX-License-Identifier: BUSL-1.1

// How the input-history rows are LAID OUT. Everything a renderer needs to decide
// before it knows what a canvas is.
//
//   * Pure. glm + the row model only. No engine type, no canvas, no font -- a
//     position is two floats and a line is two of those.
//   * A SIBLING of BrawlerInputHistoryVisualization.h and ...Poll.h, in the same
//     spirit: that one is the data, the poll fills it, this one presents it.
//
// ---------------------------------------------------------------------------
// WHY PRESENTATION IS PURE CODE AND NOT UE CODE.
//
// Source/OGBrawlerTests links { Core, OGSimulation, OGBrawler } and NOT
// OGBrawlerUnreal, so anything written against a canvas is untestable by
// construction. The two claims that matter most on this panel -- that the newest
// row draws at the TOP, and that an "up" input draws an arrow pointing UP -- are
// therefore decided here, where a Catch2 case can reach them, and the UE layer is
// left holding only the draw calls.
// ---------------------------------------------------------------------------
// THE ROW, LEFT TO RIGHT -- EXACTLY THREE COLUMNS.
//
//   |  count | dir | buttons |
//
// A field that split a row without being drawn makes its tick count unexplainable.
// ⛔ THREE COLUMNS IS ALSO THE ROW'S WHOLE IDENTITY, and the two may not come apart.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>

#include "glm/vec2.hpp"

#include "OGBrawler/BrawlerInputHistoryVisualization.h"

namespace brawlerInputHistoryVisualization
{

// The two button slots, in the existing motionButtonMask order: bit 0 = attackLeft,
// bit 1 = attackRight. Read from the recorded mask, never re-derived from an input.
inline const char* buttonMaskGlyph(uint8_t buttonMask)
{
	const bool left  = (buttonMask & 0b01u) != 0u;
	const bool right = (buttonMask & 0b10u) != 0u;

	return left ? (right ? "LR" : "L-") : (right ? "-R" : "--");
}

// ---------------------------------------------------------------------------
// THE PANEL'S GEOMETRY. Screen space, pixels, y GROWING DOWNWARD as every 2D canvas
// counts it -- which is what makes "newest at the top" the SMALLEST y.
// ---------------------------------------------------------------------------

// 64 rows at a readable height overflows any screen, and the reference display shows
// nineteen. Rows beyond the window scroll off the BOTTOM; the newest are always drawn.
inline constexpr std::size_t kPanelVisibleRows = 24u;

// One row is the smallest window that still says anything, and the ring holds 64, so a
// larger one would reserve height for rows that cannot exist.
inline constexpr std::size_t kPanelMinVisibleRows = 1u;
inline constexpr std::size_t kPanelMaxVisibleRows = kInputHistoryRowCapacity;

// The panel is sized by ONE multiplier so that a tweak is a console line, not a rebuild.
inline constexpr float kPanelDefaultScale = 1.0f;
inline constexpr float kPanelMinScale     = 0.25f;
inline constexpr float kPanelMaxScale     = 4.f;

// The rows carry their own contrast, so the shipped panel adds no backdrop at all.
// ⛔ AT ZERO THE RECTANGLE IS SKIPPED, not drawn invisibly.
inline constexpr float kPanelDefaultBackgroundAlpha = 0.0f;
inline constexpr float kPanelMinBackgroundAlpha     = 0.f;
inline constexpr float kPanelMaxBackgroundAlpha     = 1.f;

// The panel's left edge sits ON the left edge of the screen, so its rows have the
// viewport's own border to align against rather than a margin nobody chose.
inline constexpr float kPanelLeftEdgeX = 0.f;

// A request outside the range is CLAMPED to the nearer end, never rejected -- the same
// contract clampRetainedLaneTicks offers. NaN fails both comparisons, and the negated
// first test is what lands it on the minimum instead of into every multiply below.
constexpr float clampPanelScale(float requestedScale)
{
	if (!(requestedScale >= kPanelMinScale))
		return kPanelMinScale;

	if (requestedScale > kPanelMaxScale)
		return kPanelMaxScale;

	return requestedScale;
}

constexpr float clampPanelBackgroundAlpha(float requestedAlpha)
{
	if (!(requestedAlpha >= kPanelMinBackgroundAlpha))
		return kPanelMinBackgroundAlpha;

	if (requestedAlpha > kPanelMaxBackgroundAlpha)
		return kPanelMaxBackgroundAlpha;

	return requestedAlpha;
}

// Taken as int64 so a negative setting clamps rather than wrapping through unsigned.
constexpr std::size_t clampPanelVisibleRows(int64_t requestedRows)
{
	if (requestedRows < static_cast<int64_t>(kPanelMinVisibleRows))
		return kPanelMinVisibleRows;

	if (requestedRows > static_cast<int64_t>(kPanelMaxVisibleRows))
		return kPanelMaxVisibleRows;

	return static_cast<std::size_t>(requestedRows);
}

// Offsets are from the panel origin. A text column's `...RightX` is its right edge,
// because a right-aligned number is the only way a column of counts stays a column.
struct PanelLayout
{
	// Both are written by placedPanelLayout: originY depends on the viewport and the
	// scale, so it cannot be a constant, and originX is the screen's own left edge.
	// ⛔ NEITHER ORIGIN IS A CHOSEN NUMBER ANY MORE.
	float originX = kPanelLeftEdgeX;
	float originY = 0.f;

	float rowHeight = 18.f;
	float rowWidth  = 84.f;

	float tickCountRightX = 34.f;

	// A centre and a half-length, not a left edge: an arrow is drawn ABOUT a point,
	// and centring it is what keeps eight rotations inside one cell.
	float directionCenterX = 48.f;
	float directionExtent  = 6.f;

	float buttonsX = 62.f;

	float arrowThickness = 1.6f;
	float neutralDotSize = 5.f;

	// A row's text, measured down from the row's own top edge.
	float textOffsetY = 2.f;

	// The panel draws with a FIXED-SIZE font, so a scale reaching only the geometry
	// would give a bigger box holding the same tiny glyphs.
	// ⛔ THIS IS HOW THE ONE FACTOR REACHES THE GLYPHS, and it is set in one place.
	float textScale = 1.f;

	std::size_t visibleRows = kPanelVisibleRows;
};

// Every positional number multiplied by ONE factor, with the text scale set from that
// same factor in the same statement list, so the two cannot be given different values.
// The origins are deliberately left alone: where the panel sits is placement, and that
// is decided after this, from the height this produced.
// ⛔ SCALING DECIDES SIZE ONLY.
inline PanelLayout scaledPanelLayout(const PanelLayout& base, float scale)
{
	PanelLayout scaled = base;

	scaled.rowHeight        = base.rowHeight * scale;
	scaled.rowWidth         = base.rowWidth * scale;
	scaled.tickCountRightX  = base.tickCountRightX * scale;
	scaled.directionCenterX = base.directionCenterX * scale;
	scaled.directionExtent  = base.directionExtent * scale;
	scaled.buttonsX         = base.buttonsX * scale;
	scaled.arrowThickness   = base.arrowThickness * scale;
	scaled.neutralDotSize   = base.neutralDotSize * scale;
	scaled.textOffsetY      = base.textOffsetY * scale;
	scaled.textScale        = base.textScale * scale;

	return scaled;
}

// How many of a ring's rows are drawn: the newest, capped at the layout's own window.
inline std::size_t panelDrawnRowCount(const PanelLayout& layout, std::size_t ringSize)
{
	return (ringSize < layout.visibleRows) ? ringSize : layout.visibleRows;
}

// ⛔ SLOT 0 IS THE NEWEST ROW. The ring indexes OLDEST-first, so this inverts it.
inline std::size_t panelRingIndexForSlot(std::size_t ringSize, std::size_t slotFromTop)
{
	return ringSize - 1u - slotFromTop;
}

// Top edge of a slot. Strictly increasing in `slotFromTop`, which IS newest-at-top.
inline float panelRowTopY(const PanelLayout& layout, std::size_t slotFromTop)
{
	return layout.originY + static_cast<float>(slotFromTop) * layout.rowHeight;
}

inline float panelHeight(const PanelLayout& layout, std::size_t drawnRowCount)
{
	return static_cast<float>(drawnRowCount) * layout.rowHeight;
}

// The height the panel RESERVES -- the whole window, full or not. Reading rowHeight off
// the layout is what makes this the SCALED height whenever the layout is a scaled one.
inline float panelWindowHeight(const PanelLayout& layout)
{
	return static_cast<float>(layout.visibleRows) * layout.rowHeight;
}

// ⛔ THE HEIGHT PASSED IN MUST ALREADY BE SCALED. Centring an unscaled height is exact
// at scale 1 and off-centre at every other, which is invisible if only the default is run.
inline float panelCenteredOriginY(float viewportHeight, float scaledPanelHeight)
{
	return (viewportHeight - scaledPanelHeight) * 0.5f;
}

// The ONE way to get a drawable layout: scale, then centre what the scale produced. The
// order is not the caller's to get wrong, because the caller never sees the two steps.
inline PanelLayout placedPanelLayout(const PanelLayout& base,
                                     float              scale,
                                     std::size_t        visibleRows,
                                     float              viewportHeight)
{
	PanelLayout placed = scaledPanelLayout(base, scale);
	placed.visibleRows = visibleRows;

	placed.originX = kPanelLeftEdgeX;
	placed.originY = panelCenteredOriginY(viewportHeight, panelWindowHeight(placed));

	return placed;
}

// ---------------------------------------------------------------------------
// THE DIRECTION GLYPH -- A DRAWN VECTOR, NOT A TYPED CHARACTER.
//
// A Unicode arrow would be a font dependency, and a missing glyph renders as a tofu
// box: a runtime failure no test in this tree can see. Lines have no such failure mode.
//
// A renderer receives endpoints and rotates nothing, so the screen-space y sign is
// decided in one place, under test, rather than at each draw site.
// ⛔ THE AXIS IS DERIVED HERE AND NOWHERE ELSE.
//
// The compass on DirectionBucket IS the screen picture, so a bucket's ordinal indexes
// its own arrow directly and there is no second ordering here to drift out of step.
// ---------------------------------------------------------------------------

// Unit screen-space direction for a bucket, or the zero vector for Neutral.
//
// UP MEANS "MOVING WHERE I AM AIMING": Forward is aim-relative angle 0 and draws
// straight up, and on a canvas whose y grows DOWNWARD up is toward the SMALLER y.
// ⛔ FORWARD IS THEREFORE (0, -1), AND THE OTHER SEVEN ROTATE CLOCKWISE FROM IT.
inline glm::vec2 directionArrowAxis(DirectionBucket bucket)
{
	// sqrt(2)/2 -- a diagonal's two components, so all eight entries are unit length.
	constexpr float diagonal = 0.70710678f;

	// Clockwise from Forward, in the enum's own ordinal order. Neutral leads at 0 and
	// has no direction, so it falls out as the zero vector without a branch of its own.
	constexpr glm::vec2 kScreenAxes[kDirectionBucketCount] = {
		glm::vec2(     0.f,       0.f),   // Neutral
		glm::vec2(     0.f,      -1.f),   // Forward
		glm::vec2(diagonal, -diagonal),   // ForwardRight
		glm::vec2(     1.f,       0.f),   // Right
		glm::vec2(diagonal,  diagonal),   // BackRight
		glm::vec2(     0.f,       1.f),   // Back
		glm::vec2(-diagonal, diagonal),   // BackLeft
		glm::vec2(    -1.f,       0.f),   // Left
		glm::vec2(-diagonal, -diagonal),  // ForwardLeft
	};

	const std::size_t ordinal = static_cast<std::size_t>(bucket);
	if (ordinal >= kDirectionBucketCount)
		return glm::vec2(0.f, 0.f);

	return kScreenAxes[ordinal];
}

// One drawn line, in absolute panel pixels.
struct ArrowSegment
{
	glm::vec2 from{};
	glm::vec2 to{};
};

// What one row's direction cell draws: a shaft and two barbs, or a dot for Neutral.
struct DirectionGlyph
{
	bool         isNeutralDot = false;
	glm::vec2    dotOrigin{};
	float        dotSize = 0.f;
	ArrowSegment shaft{};
	ArrowSegment leftBarb{};
	ArrowSegment rightBarb{};
};

// The barbs sweep 35 degrees back off the shaft, at three quarters of its half-length.
inline constexpr float kArrowBarbCos      = 0.819152f;
inline constexpr float kArrowBarbSin      = 0.573576f;
inline constexpr float kArrowBarbFraction = 0.75f;

// Neutral has no direction, so any arrow drawn for it points somewhere unpressed.
// ⛔ NEUTRAL IS A DOT, NEVER AN ARROW.
inline DirectionGlyph directionGlyphOf(const PanelLayout& layout,
                                       float              rowTopY,
                                       DirectionBucket    bucket)
{
	const glm::vec2 center(layout.originX + layout.directionCenterX,
	                       rowTopY + layout.rowHeight * 0.5f);
	const glm::vec2 axis = directionArrowAxis(bucket);

	DirectionGlyph glyph;

	if (axis.x == 0.f && axis.y == 0.f)
	{
		glyph.isNeutralDot = true;
		glyph.dotSize      = layout.neutralDotSize;
		glyph.dotOrigin    = glm::vec2(center.x - layout.neutralDotSize * 0.5f,
		                               center.y - layout.neutralDotSize * 0.5f);
		return glyph;
	}

	const glm::vec2 tip(center.x + axis.x * layout.directionExtent,
	                    center.y + axis.y * layout.directionExtent);
	const glm::vec2 tail(center.x - axis.x * layout.directionExtent,
	                     center.y - axis.y * layout.directionExtent);

	// Back along the shaft, and across it. The barbs are these two mixed by the sweep.
	const glm::vec2 back(-axis.x, -axis.y);
	const glm::vec2 perpendicular(-axis.y, axis.x);
	const float     barb = layout.directionExtent * kArrowBarbFraction;

	glyph.shaft.from = tail;
	glyph.shaft.to   = tip;

	glyph.leftBarb.from = tip;
	glyph.leftBarb.to   = glm::vec2(
		tip.x + barb * (back.x * kArrowBarbCos + perpendicular.x * kArrowBarbSin),
		tip.y + barb * (back.y * kArrowBarbCos + perpendicular.y * kArrowBarbSin));

	glyph.rightBarb.from = tip;
	glyph.rightBarb.to   = glm::vec2(
		tip.x + barb * (back.x * kArrowBarbCos - perpendicular.x * kArrowBarbSin),
		tip.y + barb * (back.y * kArrowBarbCos - perpendicular.y * kArrowBarbSin));

	return glyph;
}

} // namespace brawlerInputHistoryVisualization

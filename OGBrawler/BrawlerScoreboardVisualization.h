#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerScoreboardVisualization-rationale.md

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace brawlerScoreboardVisualization
{

struct ScoreboardInk
{
    float r = 1.f;
    float g = 1.f;
    float b = 1.f;
};

static_assert(sizeof(ScoreboardInk) == 3u * sizeof(float),
              "NO ALPHA: this triple is exactly three linear-0..1 floats. A fourth channel "
              "would let a half-transparent swatch make 'which fighter is this' depend on "
              "what happens to be drawn behind the board, and the row ink and backdrop ink "
              "are opaque for the same reason. Was the prose fences at ScoreboardInk and at "
              "ScoreboardRow::swatch; they now live in "
              "docs/BrawlerScoreboardVisualization-rationale.md sections 4.1, 2.2 and 5.4, "
              "and this line is the only machine-checked part of them.");

struct ScoreboardRow
{
    unsigned int characterId = 0u;

    uint32_t score = 0u;

    bool isDead = false;

    uint32_t ticksUntilRespawn = 0u;

    ScoreboardInk swatch{};
};

inline constexpr ScoreboardInk scoreboardRowSwatch(const ScoreboardRow& row)
{
    return row.swatch;
}

inline bool scoreboardRowPrecedes(const ScoreboardRow& left, const ScoreboardRow& right)
{
    return left.characterId < right.characterId;
}

inline void sortScoreboardRowsById(std::vector<ScoreboardRow>& rows)
{
    std::stable_sort(rows.begin(), rows.end(), scoreboardRowPrecedes);
}

inline std::vector<ScoreboardRow> orderedScoreboardRows(std::vector<ScoreboardRow> rows)
{
    sortScoreboardRowsById(rows);
    return rows;
}

inline constexpr std::size_t kScoreboardMaxRows = 8u;

inline constexpr float kScoreboardDefaultScale = 1.0f;
inline constexpr float kScoreboardMinScale     = 0.25f;
inline constexpr float kScoreboardMaxScale     = 4.f;

inline constexpr float kScoreboardDefaultBackgroundAlpha = 0.0f;
inline constexpr float kScoreboardMinBackgroundAlpha     = 0.f;
inline constexpr float kScoreboardMaxBackgroundAlpha     = 1.f;

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

struct ScoreboardLayout
{
    float originX = 0.f;
    float originY = 0.f;

    float rowHeight = 18.f;
    float rowWidth  = 132.f;

    float swatchX = 6.f;

    float swatchWidth = 30.f;

    float swatchInsetY = 3.f;

    float scoreRightX = 78.f;

    float statusRightX = 126.f;

    float textOffsetY = 2.f;

    float textScale = 1.f;

    std::size_t maxRows = kScoreboardMaxRows;
};

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

inline std::size_t scoreboardDrawnRowCount(const ScoreboardLayout& layout,
                                           std::size_t             rowCount)
{
    return (rowCount < layout.maxRows) ? rowCount : layout.maxRows;
}

inline float scoreboardRowTopY(const ScoreboardLayout& layout, std::size_t slotFromTop)
{
    return layout.originY + static_cast<float>(slotFromTop) * layout.rowHeight;
}

struct ScoreboardSwatchRect
{
    float x      = 0.f;
    float y      = 0.f;
    float width  = 0.f;
    float height = 0.f;
};

inline ScoreboardSwatchRect scoreboardSwatchRect(const ScoreboardLayout& layout,
                                                 std::size_t             slotFromTop)
{
    ScoreboardSwatchRect rect;

    rect.x = layout.originX + layout.swatchX;

    rect.y = scoreboardRowTopY(layout, slotFromTop) + layout.swatchInsetY;

    rect.width  = layout.swatchWidth;
    rect.height = layout.rowHeight - 2.f * layout.swatchInsetY;

    return rect;
}

inline float scoreboardHeight(const ScoreboardLayout& layout, std::size_t drawnRowCount)
{
    return static_cast<float>(drawnRowCount) * layout.rowHeight;
}

inline float scoreboardWindowHeight(const ScoreboardLayout& layout)
{
    return static_cast<float>(layout.maxRows) * layout.rowHeight;
}

inline float scoreboardRightFlushOriginX(float viewportWidth, float scaledBoardWidth)
{
    return viewportWidth - scaledBoardWidth;
}

inline float scoreboardCenteredOriginY(float viewportHeight, float scaledBoardHeight)
{
    return (viewportHeight - scaledBoardHeight) * 0.5f;
}

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

inline constexpr ScoreboardInk kScoreboardLiveRowInk{ 1.f, 1.f, 1.f };

inline constexpr ScoreboardInk kScoreboardDeadRowInk{ 0.85f, 0.45f, 0.35f };

inline constexpr ScoreboardInk kScoreboardBackdropInk{ 0.f, 0.f, 0.f };

inline constexpr ScoreboardInk scoreboardRowInk(bool isDead)
{
    return isDead ? kScoreboardDeadRowInk : kScoreboardLiveRowInk;
}

inline constexpr bool scoreboardRowDrawsCountdown(const ScoreboardRow& row)
{
    return row.isDead;
}

inline constexpr uint32_t scoreboardTicksUntilRespawn(uint32_t respawnAtTick,
                                                      uint32_t displayTick)
{
    return (respawnAtTick > displayTick) ? (respawnAtTick - displayTick) : 0u;
}

} // namespace brawlerScoreboardVisualization

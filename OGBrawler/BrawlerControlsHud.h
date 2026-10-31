#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerControlsHud-rationale.md

#include "OGBrawler/DAttackMachineSimulationRuntimeTweakables.h"
#include "OGBrawler/InputMapping/ControlModeTexts.h"
#include "OGBrawler/InputMapping/KeyGlyphs.h"
#include "OGSimulation/InputMapping/InputActionDescriptor.h"

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace brawlerControlsHud
{

using MovementScheme = dAttackMachineSimulation::MovementScheme;

enum class CameraView : uint8_t
{
	Solo,
	Shared
};

struct ControlsHudInput
{
	MovementScheme scheme = MovementScheme::AimRelative;
	CameraView view = CameraView::Solo;
	bool cameraLookOn = false;
	bool legendOpen = false;
	bool legacyStickSwap = false;
};

inline constexpr std::string_view kBindingSeparator = " / ";
inline constexpr std::string_view kKeysTitleSeparator = " ";
inline constexpr std::string_view kCameraLookOnState = "ON";
inline constexpr std::string_view kCameraLookOffState = "OFF";
inline constexpr std::string_view kActiveSchemeState = "<";
inline constexpr std::string_view kSharedCameraLine = "Shared camera: it follows every player and does not turn.";
inline constexpr std::string_view kLegacyStickSwapNote = "Note: OGBrawler.SwapMoveAndAimSticks swaps the sticks.";
inline constexpr std::string_view kCloseHelpSuffix = ": close this help";

inline std::string bindingText(const dInput::MappingContext& context, const dInput::ActionDescriptor& action)
{
	std::vector<dInput::KeyId> keyboardAndMouse;
	std::vector<dInput::KeyId> gamepad;
	for (const dInput::ActionMapping& mapping : context.actionMappings)
	{
		if (mapping.action != &action)
			continue;
		for (const dInput::KeyBinding& binding : mapping.bindings)
		{
			std::vector<dInput::KeyId>& group =
				dInput::keyGlyphs::isGamepadKey(binding.key) ? gamepad : keyboardAndMouse;
			if (std::find(group.begin(), group.end(), binding.key) == group.end())
				group.push_back(binding.key);
		}
	}

	std::string text;
	for (const std::vector<dInput::KeyId>* group : { &keyboardAndMouse, &gamepad })
		for (const dInput::KeyId key : *group)
		{
			if (!text.empty())
				text += kBindingSeparator;
			text += dInput::keyGlyphs::keyGlyph(key);
		}
	return text;
}

enum class RowInk : uint8_t
{
	Active,
	Normal,
	Dim
};

struct IndicatorRow
{
	std::string keys;
	std::string label;
	std::string state;
	RowInk ink = RowInk::Normal;
};

inline const dInput::controlModeTexts::ControlModeEntry* firstEntryOfKind(
	dInput::controlModeTexts::ControlModeKind kind)
{
	for (const dInput::controlModeTexts::ControlModeEntry& entry : dInput::controlModeTexts::controlModeEntries())
		if (entry.kind == kind)
			return &entry;
	return nullptr;
}

inline const dInput::controlModeTexts::ControlModeEntry* schemeEntry(MovementScheme scheme)
{
	for (const dInput::controlModeTexts::ControlModeEntry& entry : dInput::controlModeTexts::controlModeEntries())
		if (entry.kind == dInput::controlModeTexts::ControlModeKind::MovementScheme && entry.scheme == scheme)
			return &entry;
	return nullptr;
}

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(error: 4062)
#endif
inline std::vector<IndicatorRow> indicatorRows(const dInput::MappingContext& context, const ControlsHudInput& input)
{
	using dInput::controlModeTexts::ControlModeEntry;
	using dInput::controlModeTexts::ControlModeKind;

	std::vector<IndicatorRow> rows;
	for (const ControlModeEntry& entry : dInput::controlModeTexts::controlModeEntries())
	{
		IndicatorRow row;
		switch (entry.kind)
		{
		case ControlModeKind::CameraLook:
			if (input.view != CameraView::Solo)
				continue;
			row.state = std::string(input.cameraLookOn ? kCameraLookOnState : kCameraLookOffState);
			row.ink = input.cameraLookOn ? RowInk::Active : RowInk::Normal;
			break;
		case ControlModeKind::MovementScheme:
		{
			const bool active = entry.scheme == input.scheme;
			row.state = active ? std::string(kActiveSchemeState) : std::string();
			row.ink = active ? RowInk::Active : RowInk::Dim;
			break;
		}
		case ControlModeKind::LegendToggle:
			row.ink = RowInk::Normal;
			break;
		}
		row.keys = bindingText(context, *entry.action);
		row.label = std::string(entry.shortLabel);
		if (!input.legendOpen)
		{
			row.label.clear();
			row.state.clear();
		}
		rows.push_back(std::move(row));
	}
	return rows;
}
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

inline std::string keysThenTitle(const dInput::MappingContext& context,
	const dInput::controlModeTexts::ControlModeEntry& entry)
{
	return bindingText(context, *entry.action) + std::string(kKeysTitleSeparator) + std::string(entry.title);
}

inline std::vector<std::string> legendLines(const dInput::MappingContext& context, const ControlsHudInput& input)
{
	using dInput::controlModeTexts::ControlModeEntry;
	using dInput::controlModeTexts::ControlModeKind;

	std::vector<std::string> lines;
	if (!input.legendOpen)
		return lines;

	if (const ControlModeEntry* scheme = schemeEntry(input.scheme))
	{
		lines.push_back(keysThenTitle(context, *scheme));
		for (const std::string_view line : scheme->lines)
			lines.emplace_back(line);
	}

	lines.emplace_back();

	if (input.view == CameraView::Solo)
	{
		if (const ControlModeEntry* cameraLook = firstEntryOfKind(ControlModeKind::CameraLook))
		{
			lines.push_back(keysThenTitle(context, *cameraLook));
			for (const std::string_view line : input.cameraLookOn ? cameraLook->linesWhenOn : cameraLook->lines)
				lines.emplace_back(line);
		}
	}
	else
	{
		lines.emplace_back(kSharedCameraLine);
	}

	if (input.legacyStickSwap && input.scheme != MovementScheme::AimRelativeSwapped)
		lines.emplace_back(kLegacyStickSwapNote);

	if (const ControlModeEntry* legendToggle = firstEntryOfKind(ControlModeKind::LegendToggle))
		lines.push_back(bindingText(context, *legendToggle->action) + std::string(kCloseHelpSuffix));

	return lines;
}

struct ControlsHudLayout
{
	float originX = 12.f;
	float originY = 12.f;
	float rowHeight = 16.f;
	float keysX = 6.f;
	float labelX = 92.f;
	float stateX = 300.f;
	float panelWidth = 330.f;
	float compactPanelWidth = 80.f;
	float legendGapY = 8.f;
	float legendLineHeight = 15.f;
	float legendWidth = 470.f;
	float padding = 4.f;
	float textScale = 1.f;
};

inline ControlsHudLayout scaledControlsHudLayout(const ControlsHudLayout& base, float scale)
{
	ControlsHudLayout scaled = base;

	scaled.rowHeight = base.rowHeight * scale;
	scaled.keysX = base.keysX * scale;
	scaled.labelX = base.labelX * scale;
	scaled.stateX = base.stateX * scale;
	scaled.panelWidth = base.panelWidth * scale;
	scaled.compactPanelWidth = base.compactPanelWidth * scale;
	scaled.legendGapY = base.legendGapY * scale;
	scaled.legendLineHeight = base.legendLineHeight * scale;
	scaled.legendWidth = base.legendWidth * scale;
	scaled.padding = base.padding * scale;
	scaled.textScale = base.textScale * scale;

	return scaled;
}

inline float indicatorKeysX(const ControlsHudLayout& layout)
{
	return layout.originX + layout.keysX;
}

inline float indicatorLabelX(const ControlsHudLayout& layout)
{
	return layout.originX + layout.labelX;
}

inline float indicatorStateX(const ControlsHudLayout& layout)
{
	return layout.originX + layout.stateX;
}

inline float indicatorPanelWidth(const ControlsHudLayout& layout, bool expanded)
{
	return expanded ? layout.panelWidth : layout.compactPanelWidth;
}

inline float indicatorRowTopY(const ControlsHudLayout& layout, std::size_t rowIndex)
{
	return layout.originY + layout.padding + static_cast<float>(rowIndex) * layout.rowHeight;
}

inline float indicatorHeight(const ControlsHudLayout& layout, std::size_t rowCount)
{
	return 2.f * layout.padding + static_cast<float>(rowCount) * layout.rowHeight;
}

inline float legendTopY(const ControlsHudLayout& layout, std::size_t rowCount)
{
	return layout.originY + indicatorHeight(layout, rowCount) + layout.legendGapY;
}

inline float legendHeight(const ControlsHudLayout& layout, std::size_t lineCount)
{
	return 2.f * layout.padding + static_cast<float>(lineCount) * layout.legendLineHeight;
}

inline float legendTextX(const ControlsHudLayout& layout)
{
	return layout.originX + layout.keysX;
}

inline float legendLineTopY(const ControlsHudLayout& layout, std::size_t rowCount, std::size_t lineIndex)
{
	return legendTopY(layout, rowCount) + layout.padding + static_cast<float>(lineIndex) * layout.legendLineHeight;
}

inline constexpr float kControlsHudDefaultScale = 1.f;
inline constexpr float kControlsHudMinScale = 0.25f;
inline constexpr float kControlsHudMaxScale = 4.f;

inline constexpr float kControlsHudDefaultBackgroundAlpha = 0.4f;
inline constexpr float kControlsHudMinBackgroundAlpha = 0.f;
inline constexpr float kControlsHudMaxBackgroundAlpha = 1.f;

constexpr bool isNotANumberByBits(float value)
{
	return (std::bit_cast<std::uint32_t>(value) & 0x7fffffffu) > 0x7f800000u;
}

constexpr float clampControlsHudScale(float requestedScale)
{
	if (isNotANumberByBits(requestedScale) || !(requestedScale >= kControlsHudMinScale))
		return kControlsHudMinScale;
	if (requestedScale > kControlsHudMaxScale)
		return kControlsHudMaxScale;
	return requestedScale;
}

constexpr float clampControlsHudBackgroundAlpha(float requestedAlpha)
{
	if (isNotANumberByBits(requestedAlpha) || !(requestedAlpha >= kControlsHudMinBackgroundAlpha))
		return kControlsHudMinBackgroundAlpha;
	if (requestedAlpha > kControlsHudMaxBackgroundAlpha)
		return kControlsHudMaxBackgroundAlpha;
	return requestedAlpha;
}

static_assert(clampControlsHudScale(kControlsHudDefaultScale) == kControlsHudDefaultScale
		&& clampControlsHudBackgroundAlpha(kControlsHudDefaultBackgroundAlpha) == kControlsHudDefaultBackgroundAlpha,
	"brawlerControlsHud: each console default must lie inside its own clamp range, or the shipped "
	"default is not the value the HUD draws with.");

static_assert(isNotANumberByBits(std::numeric_limits<float>::quiet_NaN())
		&& isNotANumberByBits(-std::numeric_limits<float>::quiet_NaN())
		&& isNotANumberByBits(std::numeric_limits<float>::signaling_NaN())
		&& !isNotANumberByBits(std::numeric_limits<float>::infinity())
		&& !isNotANumberByBits(-std::numeric_limits<float>::infinity())
		&& !isNotANumberByBits(std::numeric_limits<float>::max())
		&& !isNotANumberByBits(0.f) && !isNotANumberByBits(-0.f)
		&& !isNotANumberByBits(std::numeric_limits<float>::denorm_min()),
	"brawlerControlsHud: isNotANumberByBits must be true for every NaN and false for every other float; "
	"the clamps rely on it because a NaN comparison is not reliable under /fp:fast.");

struct ControlsHudInk
{
	float r = 1.f;
	float g = 1.f;
	float b = 1.f;
};

static_assert(sizeof(ControlsHudInk) == 3u * sizeof(float),
	"brawlerControlsHud: an ink is exactly three linear 0..1 floats; the backdrop's transparency is "
	"the separate background alpha, and the text inks are opaque.");

inline constexpr ControlsHudInk kControlsHudActiveInk{ 1.0f, 0.85f, 0.2f };
inline constexpr ControlsHudInk kControlsHudNormalInk{ 0.9f, 0.9f, 0.9f };
inline constexpr ControlsHudInk kControlsHudDimInk{ 0.55f, 0.55f, 0.55f };
inline constexpr ControlsHudInk kControlsHudBackdropInk{ 0.f, 0.f, 0.f };

#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(push)
#pragma warning(error: 4062)
#endif
constexpr ControlsHudInk controlsHudRowInk(RowInk ink)
{
	switch (ink)
	{
	case RowInk::Active: return kControlsHudActiveInk;
	case RowInk::Normal: return kControlsHudNormalInk;
	case RowInk::Dim: return kControlsHudDimInk;
	}
	return kControlsHudNormalInk;
}
#if defined(_MSC_VER) && !defined(__clang__)
#pragma warning(pop)
#endif

} // namespace brawlerControlsHud

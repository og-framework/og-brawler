#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/ControlModeTexts-rationale.md

#include "OGSimulation/OGExport.h"
#include "OGSimulation/InputMapping/InputActionDescriptor.h"
#include "OGBrawler/DAttackMachineSimulationRuntimeTweakables.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace dInput
{
namespace controlModeTexts
{

enum class ControlModeKind : uint8_t
{
	CameraLook,
	MovementScheme,
	LegendToggle
};

struct ControlModeEntry
{
	const ActionDescriptor* action;
	ControlModeKind kind;
	dAttackMachineSimulation::MovementScheme scheme;
	std::string_view shortLabel;
	std::string_view title;
	std::vector<std::string_view> lines;
	std::vector<std::string_view> linesWhenOn;
};

OGBRAWLER_API const std::vector<ControlModeEntry>& controlModeEntries();

OGBRAWLER_API const ControlModeEntry* findControlModeEntry(const ActionDescriptor* action);

} // namespace controlModeTexts
} // namespace dInput

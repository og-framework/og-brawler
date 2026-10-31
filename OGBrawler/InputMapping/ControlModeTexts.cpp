// SPDX-License-Identifier: BUSL-1.1
// docs/ControlModeTexts-rationale.md
#include "OGBrawler/InputMapping/ControlModeTexts.h"

#include "OGBrawler/InputMapping/GameInputMapping.h"

namespace dInput
{
namespace controlModeTexts
{

const std::vector<ControlModeEntry>& controlModeEntries()
{
	using dAttackMachineSimulation::MovementScheme;
	constexpr MovementScheme kSchemeUnusedByThisKind{};

	static const std::vector<ControlModeEntry> entries = {
		{ &gameMapping::BlockLook, ControlModeKind::CameraLook, kSchemeUnusedByThisKind,
			"Camera look", "Camera look (solo camera)",
			{
				"OFF: the camera holds still and the mouse cursor aims.",
			},
			{
				"ON: the cursor hides; mouse and aim stick turn the camera.",
				"In Aim-relative you aim where the camera faces.",
			} },
		{ &gameMapping::SetSchemeCameraRelative, ControlModeKind::MovementScheme, MovementScheme::CameraRelative,
			"Camera-relative", "Camera-relative",
			{
				"Move (movement keys / left stick): relative to the camera.",
				"Aim: the mouse cursor, or the right stick (camera-relative).",
				"Gamepad: right stick released, you aim where you walk.",
			},
			{} },
		{ &gameMapping::SetSchemeAimRelative, ControlModeKind::MovementScheme, MovementScheme::AimRelative,
			"Aim-relative", "Aim-relative",
			{
				"Aim: the mouse cursor, or the right stick (camera-relative).",
				"Move (movement keys / left stick): relative to your aim;",
				"  up walks toward it, left and right strafe around it.",
				"Gamepad: right stick released, the left stick moves and aims.",
			},
			{} },
		{ &gameMapping::SetSchemeMoveRelativeAim, ControlModeKind::MovementScheme, MovementScheme::MoveRelativeAim,
			"Aim follows move", "Aim follows move",
			{
				"Move (movement keys / left stick): relative to the camera.",
				"Aim (right stick): relative to where you walk;",
				"  stick up aims where you walk (camera-relative when still).",
				"The mouse cursor aims directly.",
				"Gamepad: right stick released, you aim where you walk.",
			},
			{} },
		{ &gameMapping::SetSchemeAimRelativeSwapped, ControlModeKind::MovementScheme, MovementScheme::AimRelativeSwapped,
			"Aim-relative, swapped sticks", "Aim-relative, swapped sticks",
			{
				"As Aim-relative, with the gamepad sticks swapped:",
				"  the right stick moves, the left stick aims.",
				"Keyboard and mouse work as in Aim-relative.",
				"Gamepad: right stick released, the left stick moves and aims.",
			},
			{} },
		{ &gameMapping::ToggleControlsLegend, ControlModeKind::LegendToggle, kSchemeUnusedByThisKind,
			"Controls help", "Controls help",
			{},
			{} },
	};
	return entries;
}

const ControlModeEntry* findControlModeEntry(const ActionDescriptor* action)
{
	for (const ControlModeEntry& entry : controlModeEntries())
		if (entry.action == action)
			return &entry;
	return nullptr;
}

} // namespace controlModeTexts
} // namespace dInput

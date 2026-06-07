// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "OGSimulation/OGExport.h"
#include <atomic>
#include <string>

namespace dAttackMachineSimulation
{

enum class MovementScheme : uint32_t
{
	CameraRelative = 0,
	AimRelative    = 1,
	// Movement direction is camera-relative (like CameraRelative). Aim direction is
	// rotated relative to the current movement direction — aim-stick-up makes the aim
	// direction equal the movement direction. With no movement input, aim falls back to
	// camera-relative interpretation of the aim stick. UE-side only; sim doesn't read it.
	MoveRelativeAim = 2,
};

OGBRAWLER_API extern std::atomic<MovementScheme> g_movementScheme;

// Stick deadzones — magnitudes below these are treated as no input by the UE
// input-collection layer (Move(), buildMoveDirectionWorld, buildAimDirection).
// Sim layer does not read these; they are tweakables-by-convention living next to
// g_movementScheme so the named-pipe SetVariable path can tune them at runtime.
OGBRAWLER_API extern std::atomic<float> g_moveStickDeadzone;
OGBRAWLER_API extern std::atomic<float> g_aimStickDeadzone;

// When true, the physical stick assignments are swapped: the right (aim) stick feeds
// the move direction and the left (move) stick feeds the aim direction. UE-side only;
// the sim sees the resulting (moveStick, aimDirection, moveDirectionWorld) tuple either
// way and does not read this flag.
OGBRAWLER_API extern std::atomic<bool> g_swapMoveAndAimSticks;

// Character walk speed in UE units (cm) per second. Drives CharacterMovementComponent's
// MaxWalkSpeed each Tick. UE-side only; sim doesn't read it. Default 100 (half of the
// pre-tweakable 200 the constructor set).
OGBRAWLER_API extern std::atomic<float> g_moveSpeed;

// Gamepad-only feature toggle: when true (default), if the aim stick is below its
// deadzone and the most recent move input came from the gamepad left stick, the move
// stick feeds BOTH the movement direction and the aim direction (both resolve to the
// same camera-relative move-stick vector). When false, the gamepad case falls through
// to mouse aim / camera forward like any other input source. Keyboard+mouse play is
// unaffected either way (the latch m_lastMoveInputWasGamepad gates this).
OGBRAWLER_API extern std::atomic<bool> g_gamepadMoveStickFeedsAim;

OGBRAWLER_API bool SetVariable(const std::string& name, const std::string& value);

}

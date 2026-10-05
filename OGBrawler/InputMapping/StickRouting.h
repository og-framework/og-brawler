#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGBrawler/DAttackMachineSimulationRuntimeTweakables.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"

#include <cmath>
#include <optional>

namespace dInput
{
namespace stickRouting
{

struct StickSources
{
	glm::vec2 moveKeys;
	glm::vec2 leftStick;
	glm::vec2 rightStick;
};

struct LogicalSticks
{
	glm::vec2 move;
	glm::vec2 aim;
};

inline glm::vec2 clampUnit(const glm::vec2& v)
{
	const float len = glm::length(v);
	return len > 1.f ? v / len : v;
}

inline LogicalSticks routeSticks(
	const StickSources& sources,
	dAttackMachineSimulation::MovementScheme scheme,
	bool legacySwap,
	bool feedsAim,
	float moveDeadzone)
{
	if (scheme == dAttackMachineSimulation::MovementScheme::AimRelativeSwapped)
	{
		const bool rightStickNeutral =
			glm::dot(sources.rightStick, sources.rightStick) <= moveDeadzone * moveDeadzone;
		if (!(rightStickNeutral && feedsAim))
			return { clampUnit(sources.moveKeys + sources.rightStick), sources.leftStick };
		return { clampUnit(sources.moveKeys + sources.leftStick), sources.rightStick };
	}

	const glm::vec2 keysAndLeft = clampUnit(sources.moveKeys + sources.leftStick);
	if (legacySwap)
		return { sources.rightStick, keysAndLeft };
	return { keysAndLeft, sources.rightStick };
}

inline bool guardFreezeRequested(
	bool holdGuard,
	const StickSources& sources,
	dAttackMachineSimulation::MovementScheme scheme,
	bool legacySwap,
	bool feedsAim,
	float moveDeadzone)
{
	if (!holdGuard)
		return false;
	const bool ownMoveIgnoresFallback = false;
	const glm::vec2 ownMove =
		routeSticks(sources, scheme, legacySwap, ownMoveIgnoresFallback, moveDeadzone).move;
	const glm::vec2 actualMove = routeSticks(sources, scheme, legacySwap, feedsAim, moveDeadzone).move;
	return glm::length(ownMove) < moveDeadzone || glm::length(actualMove) < moveDeadzone;
}

inline constexpr float kMinCameraLookAimHorizontalLength = 1e-3f;

inline std::optional<glm::vec3> cameraLookAim(
	dAttackMachineSimulation::MovementScheme scheme,
	bool blockLookHeld,
	const glm::vec3& cameraForward)
{
	if (!blockLookHeld || scheme != dAttackMachineSimulation::MovementScheme::AimRelative)
		return std::nullopt;
	const glm::vec3 horizontal(cameraForward.x, cameraForward.y, 0.f);
	const float horizontalLength = glm::length(horizontal);
	if (!(horizontalLength > kMinCameraLookAimHorizontalLength))
		return std::nullopt;
	return horizontal / horizontalLength;
}

inline constexpr float kMoveKeysZeroTolerance = 1e-4f;
inline constexpr float kCursorLatchMinPixels = 8.f;

inline bool moveKeysPressed(const glm::vec2& moveKeys)
{
	return std::abs(moveKeys.x) > kMoveKeysZeroTolerance || std::abs(moveKeys.y) > kMoveKeysZeroTolerance;
}

inline bool lastInputWasGamepadAfterStick(bool lastInputWasGamepad, const glm::vec2& stick, float stickDeadzone)
{
	return lastInputWasGamepad || glm::length(stick) > stickDeadzone;
}

inline bool lastInputWasGamepadAfterMoveKeys(bool lastInputWasGamepad, const glm::vec2& moveKeys, bool dpadHeld)
{
	return moveKeysPressed(moveKeys) ? dpadHeld : lastInputWasGamepad;
}

struct CursorLatch
{
	bool lastInputWasGamepad;
	std::optional<glm::vec2> anchor;
};

inline CursorLatch lastInputWasGamepadAfterCursor(
	bool lastInputWasGamepad, const std::optional<glm::vec2>& anchor, const glm::vec2& cursor)
{
	if (!anchor.has_value() || !lastInputWasGamepad)
		return { lastInputWasGamepad, cursor };
	if (glm::length(cursor - *anchor) < kCursorLatchMinPixels)
		return { true, anchor };
	return { false, cursor };
}

} // namespace stickRouting
} // namespace dInput

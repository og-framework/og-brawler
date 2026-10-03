#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGBrawler/DAttackMachineSimulationRuntimeTweakables.h"
#include "glm/vec2.hpp"
#include "glm/geometric.hpp"

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

} // namespace stickRouting
} // namespace dInput

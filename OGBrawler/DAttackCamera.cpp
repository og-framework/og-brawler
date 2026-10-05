// SPDX-License-Identifier: BUSL-1.1
// docs/DAttackCamera-rationale.md · docs/DAttackCamera-guards.md
#include "DAttackCamera.h"
#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include <algorithm>
#include <cmath>

namespace dAttackCameraBehaviour
{
namespace
{
struct LookDelta
{
	float yawDeg;
	float pitchDeg;
};

LookDelta mouseLook(const glm::vec2& mouseUnits, const OrbitCameraSettings& settings)
{
	const float degreesPerUnit = settings.mouseDegreesPerUnit * settings.mouseSens;
	const float pitchSign = settings.invertMouseY ? -1.f : 1.f;
	// ⛔G-01  docs/DAttackCamera-guards.md
	return LookDelta{ -mouseUnits.x * degreesPerUnit, pitchSign * mouseUnits.y * degreesPerUnit };
}

LookDelta stickLook(const glm::vec2& stick, float deltaSeconds, const OrbitCameraSettings& settings)
{
	const float magnitude = glm::length(stick);
	const float response = stickCurve(magnitude, settings) * settings.stickSens * deltaSeconds;
	if (response == 0.f)
		return LookDelta{ 0.f, 0.f };
	const glm::vec2 direction = stick / magnitude;
	const float pitchSign = settings.invertStickY ? 1.f : -1.f;
	return LookDelta{ direction.x * response * settings.stickYawDegPerSec,
		pitchSign * direction.y * response * settings.stickPitchDegPerSec };
}
} // namespace

OrbitCameraState integrate(float deltaSeconds, const OrbitCameraInput& input,
	const OrbitCameraSettings& settings, const OrbitCameraState& state)
{
	if (!(deltaSeconds > 0.f))
		return state;

	OrbitCameraState next = state;
	next.holdoffSeconds = std::max(0.f, state.holdoffSeconds - deltaSeconds);
	if (!input.lookHeld)
		return next;

	const LookDelta mouse = mouseLook(input.mouseUnits, settings);
	const LookDelta stick = stickLook(input.stick, deltaSeconds, settings);
	const float yawDelta = mouse.yawDeg + stick.yawDeg;
	const float pitchDelta = mouse.pitchDeg + stick.pitchDeg;

	next.yawDeg = wrapYawDeg(state.yawDeg + yawDelta);
	next.pitchDeg = clampPitchDeg(state.pitchDeg + pitchDelta, settings);

	if (settings.verticalHoldoffSeconds > 0.f && glm::abs(pitchDelta) > settings.verticalIntentDegPerSec * deltaSeconds)
	{
		next.holdoffSeconds = settings.verticalHoldoffSeconds;
		return next;
	}

	// ∴D-01  docs/DAttackCamera-rationale.md
	const float pullSeconds = std::max(0.f, deltaSeconds - state.holdoffSeconds);
	const float yawMagnitude = glm::abs(yawDelta);
	if (pullSeconds == 0.f || yawMagnitude == 0.f)
		return next;

	const float pullShare = pullSeconds / deltaSeconds;
	const float weight = std::clamp(1.f - settings.dominanceRatio * glm::abs(pitchDelta) / yawMagnitude, 0.f, 1.f);
	const float eFold = std::max(settings.pullYawDegreesPerEFold, kMinPullYawDegreesPerEFold);
	const float exponent = weight * yawMagnitude * pullShare / eFold;
	const float fraction = -std::expm1(-exponent);
	const float playerKeptShare = exponent > 0.f ? fraction / exponent : 1.f;
	const float beforeWindow = clampPitchDeg(state.pitchDeg + pitchDelta * (1.f - pullShare), settings);
	const float playerInWindow = pitchDelta * pullShare;
	const float target = clampPitchDeg(input.targetPitchDeg, settings);
	const float cap = std::max(0.f, settings.pullMaxDegPerSec) * pullSeconds;
	// ∴D-02  docs/DAttackCamera-rationale.md
	const float pull = (target - beforeWindow) * fraction + playerInWindow * (playerKeptShare - 1.f);
	next.pitchDeg = clampPitchDeg(beforeWindow + playerInWindow + std::clamp(pull, -cap, cap), settings);
	return next;
}
} // namespace dAttackCameraBehaviour

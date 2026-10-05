#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/DAttackCamera-rationale.md · docs/DAttackCamera-guards.md

#include "OGSimulation/OGExport.h"
#include "glm/vec2.hpp"
#include <algorithm>
#include <cmath>

namespace dAttackCameraBehaviour
{
inline constexpr float kMouseDegreesPerUnit = 2.5f;
inline constexpr float kStickDeadzone = 0.15f;
inline constexpr float kStickExponent = 1.5f;
inline constexpr float kStickYawDegPerSec = 180.f;
inline constexpr float kStickPitchDegPerSec = 110.f;
inline constexpr float kPitchMinDeg = 10.f;
inline constexpr float kPitchMaxDeg = 80.f;
inline constexpr float kPullYawDegreesPerEFold = 20.f;
inline constexpr float kPullMaxDegPerSec = 240.f;
inline constexpr float kDominanceRatio = 0.f;
inline constexpr float kVerticalIntentDegPerSec = 20.f;
inline constexpr float kVerticalHoldoffSeconds = 0.f;
inline constexpr float kMinPullYawDegreesPerEFold = 1.f;

inline constexpr float kHardPitchMinDeg = 1.f;
inline constexpr float kHardPitchMaxDeg = 89.f;

inline constexpr float kBoomLengthAtTargetPitch = 900.f;
inline constexpr float kBoomLengthAtHorizon = 400.f;

static_assert(0.f < kHardPitchMinDeg && kHardPitchMinDeg < kHardPitchMaxDeg && kHardPitchMaxDeg < 90.f,
	"The pitch must stay inside (0, 90) degrees below the horizon: boomLength divides by the target, and at 90 the "
	"camera-relative move basis has no horizontal forward. Was the task-7 [1, 89] target clamp.");
static_assert(kHardPitchMinDeg <= kPitchMinDeg && kPitchMinDeg < kPitchMaxDeg && kPitchMaxDeg <= kHardPitchMaxDeg,
	"The default pitch limits must sit inside the hard limits.");
static_assert(0.f <= kStickDeadzone && kStickDeadzone < 1.f, "stickCurve divides by (1 - deadzone).");
static_assert(0.f < kMinPullYawDegreesPerEFold && kMinPullYawDegreesPerEFold <= kPullYawDegreesPerEFold && kPullMaxDegPerSec > 0.f,
	"The pull divides by its e-fold (never below the minimum a host value is raised to), and the cap is positive.");
static_assert(kVerticalHoldoffSeconds >= 0.f && kDominanceRatio >= 0.f,
	"A hold-off of 0 and a dominance ratio of 0 switch those mechanisms off (task 10 defaults); negative values mean nothing.");
static_assert(kBoomLengthAtHorizon < kBoomLengthAtTargetPitch, "The boom is longest at the target pitch.");

struct OrbitCameraSettings
{
	float mouseDegreesPerUnit = kMouseDegreesPerUnit;
	float stickDeadzone = kStickDeadzone;
	float stickExponent = kStickExponent;
	float stickYawDegPerSec = kStickYawDegPerSec;
	float stickPitchDegPerSec = kStickPitchDegPerSec;
	float pitchMinDeg = kPitchMinDeg;
	float pitchMaxDeg = kPitchMaxDeg;
	float pullYawDegreesPerEFold = kPullYawDegreesPerEFold;
	float pullMaxDegPerSec = kPullMaxDegPerSec;
	float dominanceRatio = kDominanceRatio;
	float verticalIntentDegPerSec = kVerticalIntentDegPerSec;
	float verticalHoldoffSeconds = kVerticalHoldoffSeconds;
	bool invertMouseY = false;
	bool invertStickY = false;
	float mouseSens = 1.f;
	float stickSens = 1.f;
};

struct OrbitCameraInput
{
	bool lookHeld = false;
	glm::vec2 mouseUnits = glm::vec2(0.f);
	glm::vec2 stick = glm::vec2(0.f);
	float targetPitchDeg = 0.f;
};

struct OrbitCameraState
{
	float yawDeg = 0.f;
	float pitchDeg = 0.f;
	float holdoffSeconds = 0.f;
};

struct PitchLimits
{
	float minDeg;
	float maxDeg;
};

inline PitchLimits pitchLimits(const OrbitCameraSettings& settings)
{
	const float minDeg = std::clamp(settings.pitchMinDeg, kHardPitchMinDeg, kHardPitchMaxDeg);
	return PitchLimits{ minDeg, std::clamp(settings.pitchMaxDeg, minDeg, kHardPitchMaxDeg) };
}

inline float clampPitchDeg(float pitchDeg, const OrbitCameraSettings& settings)
{
	const PitchLimits limits = pitchLimits(settings);
	return std::clamp(pitchDeg, limits.minDeg, limits.maxDeg);
}

inline float pitchDegFromUEPitchDegrees(float uePitchDegrees, const OrbitCameraSettings& settings)
{
	return clampPitchDeg(-uePitchDegrees, settings);
}

inline float wrapYawDeg(float yawDeg)
{
	return std::remainder(yawDeg, 360.f);
}

inline OrbitCameraState seedFromUERotation(float uePitchDegrees, float ueYawDegrees, const OrbitCameraSettings& settings)
{
	return OrbitCameraState{ wrapYawDeg(ueYawDegrees), pitchDegFromUEPitchDegrees(uePitchDegrees, settings), 0.f };
}

inline float stickCurve(float magnitude, const OrbitCameraSettings& settings)
{
	const float tilt = std::min(magnitude, 1.f);
	if (!(tilt > settings.stickDeadzone) || !(settings.stickDeadzone < 1.f))
		return 0.f;
	return std::pow((tilt - settings.stickDeadzone) / (1.f - settings.stickDeadzone), settings.stickExponent);
}

inline float boomLength(float pitchDeg, float targetPitchDeg, const OrbitCameraSettings& settings)
{
	const float target = clampPitchDeg(targetPitchDeg, settings);
	const float towardTarget = std::clamp(pitchDeg, 0.f, target) / target;
	return kBoomLengthAtTargetPitch - (kBoomLengthAtTargetPitch - kBoomLengthAtHorizon) * (1.f - towardTarget);
}

OGBRAWLER_API OrbitCameraState integrate(float deltaSeconds, const OrbitCameraInput& input,
	const OrbitCameraSettings& settings, const OrbitCameraState& state);
} // namespace dAttackCameraBehaviour

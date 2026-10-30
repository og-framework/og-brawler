#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/DAttackCamera-rationale.md · docs/DAttackCamera-guards.md

#include "OGSimulation/OGExport.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
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

inline constexpr float kBoomLengthAtTargetPitch = 2500.f;
inline constexpr float kBoomLengthAtHorizon = 550.f;

enum class FovAxis : int
{
	Horizontal = 0,
	Shorter = 1,
};

inline constexpr FovAxis kFovAxis = FovAxis::Shorter;
inline constexpr float kFovDeg = 40.f;
inline constexpr float kFovDegAtHorizon = 58.f;
inline constexpr float kMinFramingTanHalfFov = 0.001f;

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
static_assert(0.f < kFovDeg && kFovDeg < 180.f && 0.f < kFovDegAtHorizon && kFovDegAtHorizon < 180.f,
	"A field of view is an angle inside (0, 180) degrees.");
static_assert(static_cast<int>(FovAxis::Horizontal) == 0 && static_cast<int>(FovAxis::Shorter) == 1,
	"The og.cam.fovAxis / og.iso.fovAxis console values are these numbers: 0 = horizontal, 1 = the shorter screen axis.");

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
	float boomLengthAtTargetPitch = kBoomLengthAtTargetPitch;
	float boomLengthAtHorizon = kBoomLengthAtHorizon;
	float fovDeg = kFovDeg;
	FovAxis fovAxis = kFovAxis;
	float fovDegAtHorizon = kFovDegAtHorizon;
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

inline float towardTargetPitch(float pitchDeg, float targetPitchDeg, const OrbitCameraSettings& settings)
{
	const float target = clampPitchDeg(targetPitchDeg, settings);
	return std::clamp(pitchDeg, 0.f, target) / target;
}

inline float boomLength(float pitchDeg, float targetPitchDeg, const OrbitCameraSettings& settings)
{
	const float towardTarget = towardTargetPitch(pitchDeg, targetPitchDeg, settings);
	return settings.boomLengthAtTargetPitch
		- (settings.boomLengthAtTargetPitch - settings.boomLengthAtHorizon) * (1.f - towardTarget);
}

inline float fovAtPitch(float pitchDeg, float targetPitchDeg, const OrbitCameraSettings& settings)
{
	const float towardTarget = towardTargetPitch(pitchDeg, targetPitchDeg, settings);
	return settings.fovDeg - (settings.fovDeg - settings.fovDegAtHorizon) * (1.f - towardTarget);
}

inline FovAxis fovAxisFromConsoleValue(int value)
{
	return value <= static_cast<int>(FovAxis::Horizontal) ? FovAxis::Horizontal : FovAxis::Shorter;
}

inline float horizontalFovDeg(float fovDeg, FovAxis fovAxis, float aspect)
{
	if (fovAxis == FovAxis::Horizontal || !(aspect > 1.f))
		return fovDeg;
	constexpr float kDegToRad = 3.1415926535897932f / 180.f;
	constexpr float kRadToDeg = 180.f / 3.1415926535897932f;
	return 2.f * std::atan(std::tan(fovDeg * 0.5f * kDegToRad) * aspect) * kRadToDeg;
}

inline float isoFramingDistance(float halfH, float halfV, float aspect, float fovDeg, FovAxis fovAxis,
	float marginH, float marginV, float minDist, float maxDist)
{
	const float horizontalDeg = horizontalFovDeg(fovDeg, fovAxis, aspect);
	const float paddedHalfH = halfH + marginH;
	const float paddedHalfV = halfV + marginV;
	const float tanHalfFov = std::tan(horizontalDeg * 0.5f * (3.1415926535897932f / 180.f));
	const float safeTan = (kMinFramingTanHalfFov < tanHalfFov) ? tanHalfFov : kMinFramingTanHalfFov;
	const float verticalAtAspect = paddedHalfV * aspect;
	const float requiredDistance = ((verticalAtAspect < paddedHalfH) ? paddedHalfH : verticalAtAspect) / safeTan;
	const float atMost = (requiredDistance < maxDist) ? requiredDistance : maxDist;
	return (minDist < atMost) ? atMost : minDist;
}

inline constexpr float kSightDegreesToRadians = 3.1415926535897932f / 180.f;
inline constexpr float kSightRadiansToDegrees = 180.f / 3.1415926535897932f;

inline float verticalFovDeg(float horizontalDeg, float aspect)
{
	return 2.f * std::atan(std::tan(horizontalDeg * 0.5f * kSightDegreesToRadians) / aspect) * kSightRadiansToDegrees;
}

struct SightReach
{
	float halfWidth;
	float halfHeight;
};

inline SightReach sightReach(float distance, float fovDeg, FovAxis fovAxis, float aspect)
{
	const float horizontalDeg = horizontalFovDeg(fovDeg, fovAxis, aspect);
	const float verticalDeg = verticalFovDeg(horizontalDeg, aspect);
	return SightReach{ distance * std::tan(horizontalDeg * 0.5f * kSightDegreesToRadians),
		distance * std::tan(verticalDeg * 0.5f * kSightDegreesToRadians) };
}

inline SightReach soloSightReach(const OrbitCameraSettings& solo, float aspect)
{
	return sightReach(solo.boomLengthAtTargetPitch, solo.fovDeg, solo.fovAxis, aspect);
}

struct IsoMargins
{
	float horizontal;
	float vertical;
};

inline IsoMargins matchedIsoMargins(float boom, float fovDeg, FovAxis fovAxis, float aspect)
{
	const SightReach reach = sightReach(boom, fovDeg, fovAxis, aspect);
	return IsoMargins{ reach.halfWidth, reach.halfHeight };
}

struct IsoFramingSettings
{
	float fovDeg;
	FovAxis fovAxis;
	float marginH;
	float marginV;
	float minDist;
	float maxDist;
};

inline IsoFramingSettings resolveIsoFraming(
	bool matchSolo, const IsoFramingSettings& iso, const OrbitCameraSettings& solo, float aspect)
{
	if (!matchSolo)
		return iso;
	const IsoMargins margins = matchedIsoMargins(solo.boomLengthAtTargetPitch, solo.fovDeg, solo.fovAxis, aspect);
	return IsoFramingSettings{ solo.fovDeg, solo.fovAxis, margins.horizontal, margins.vertical, 0.f, iso.maxDist };
}

inline float isoFramingDistance(float halfH, float halfV, float aspect, const IsoFramingSettings& framing)
{
	return isoFramingDistance(halfH, halfV, aspect, framing.fovDeg, framing.fovAxis, framing.marginH, framing.marginV,
		framing.minDist, framing.maxDist);
}

struct CameraSight
{
	float soloBoom;
	float soloHorizontalFovDeg;
	float soloVerticalFovDeg;
	SightReach soloReach;
	bool matchSolo;
	IsoFramingSettings iso;
	float isoDistance;
	float soloHorizontalFovAtHorizonDeg;
};

inline CameraSight cameraSight(const OrbitCameraSettings& solo, bool matchSolo, const IsoFramingSettings& iso,
	float aspect, float groupHalfH, float groupHalfV)
{
	const float horizontalDeg = horizontalFovDeg(solo.fovDeg, solo.fovAxis, aspect);
	const IsoFramingSettings framing = resolveIsoFraming(matchSolo, iso, solo, aspect);
	return CameraSight{ solo.boomLengthAtTargetPitch, horizontalDeg, verticalFovDeg(horizontalDeg, aspect),
		soloSightReach(solo, aspect), matchSolo, framing, isoFramingDistance(groupHalfH, groupHalfV, aspect, framing),
		horizontalFovDeg(solo.fovDegAtHorizon, solo.fovAxis, aspect) };
}

inline constexpr float kInterpSnapDistanceSquared = 1.e-4f;

inline glm::dvec3 interpTo(const glm::dvec3& current, const glm::dvec3& target, float deltaSeconds, float interpSpeed)
{
	if (interpSpeed <= 0.f)
		return target;
	const glm::dvec3 toTarget = target - current;
	if (toTarget.x * toTarget.x + toTarget.y * toTarget.y + toTarget.z * toTarget.z < kInterpSnapDistanceSquared)
		return target;
	const float scaled = deltaSeconds * interpSpeed;
	const float atMostOne = (scaled < 1.f) ? scaled : 1.f;
	const float fraction = (0.f < atMostOne) ? atMostOne : 0.f;
	const double step = static_cast<double>(fraction);
	return current + glm::dvec3(toTarget.x * step, toTarget.y * step, toTarget.z * step);
}

struct FollowStep
{
	glm::dvec3 position;
	bool tracking;
};

inline FollowStep followStep(const glm::dvec3& current, const glm::dvec3& target, bool tracking, float deltaSeconds,
	float interpSpeed, float startDistance, float stopDistance)
{
	const double dx = target.x - current.x;
	const double dy = target.y - current.y;
	const double dz = target.z - current.z;
	const double error = std::sqrt(dx * dx + dy * dy + dz * dz);
	const float threshold = !tracking ? startDistance : ((stopDistance < startDistance) ? stopDistance : startDistance);
	if (!(error > threshold))
		return FollowStep{ current, false };
	return FollowStep{ interpTo(current, target, deltaSeconds, interpSpeed), true };
}

OGBRAWLER_API OrbitCameraState integrate(float deltaSeconds, const OrbitCameraInput& input,
	const OrbitCameraSettings& settings, const OrbitCameraState& state);
} // namespace dAttackCameraBehaviour

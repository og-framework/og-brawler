#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/DAttackCamera-rationale.md · docs/DAttackCamera-guards.md

#include "OGSimulation/OGExport.h"
#include "OGSimulation/SurfaceFrame.h"
#include "glm/geometric.hpp"
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

inline constexpr float kReturnDegPerSec = 90.f;
inline constexpr float kCameraFollowMaxSlopeDeg = 30.f;
inline constexpr float kCameraFollowSlopeEpsilonDeg = 0.1f;
inline constexpr glm::vec3 kCameraUp(0.f, 0.f, 1.f);
inline constexpr float kSlopePitchMinDeg = 35.f;
inline constexpr float kSlopePitchMaxDeg = 75.f;
inline constexpr float kSlopeYawMinSlopeDeg = 15.f;
inline constexpr float kSlopeYawSlopeEpsilonDeg = 0.001f;
inline constexpr float kSlopeYawDegPerSec = 90.f;
inline constexpr float kSlopeYawDwellSeconds = 0.2f;
inline constexpr float kSlopeYawDwellToleranceDeg = 10.f;
inline constexpr float kSlopeYawDwellEpsilonSeconds = 1e-4f;
inline constexpr float kSlopeYawEaseSeconds = 0.3f;
inline constexpr float kSlopeYawAccelDegPerSec2 = 360.f;
inline constexpr float kSlopeYawArriveDeg = 0.01f;

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
static_assert(kReturnDegPerSec > 0.f, "The look-mode-off return moves the camera back; 0 would be the frozen camera before task 21.");
static_assert(0.f < kCameraFollowMaxSlopeDeg && 0.f < kCameraFollowSlopeEpsilonDeg
		&& kCameraFollowMaxSlopeDeg + kCameraFollowSlopeEpsilonDeg < 90.f,
	"The followed ground tilts less than 90 degrees from world up, so a frame up keeps z > 0: worldView's rise along the "
	"view divides by z, and the cut-off's cosine is positive.");
static_assert(kHardPitchMinDeg <= kSlopePitchMinDeg && kSlopePitchMinDeg <= kSlopePitchMaxDeg
		&& kSlopePitchMaxDeg <= kHardPitchMaxDeg,
	"The default slope pitch limits sit inside the hard limits, in order.");
static_assert(0.f < kSlopeYawMinSlopeDeg && kSlopeYawMinSlopeDeg <= kCameraFollowMaxSlopeDeg,
	"With look mode off the camera turns up a slope only where the target up follows the ground, so the turn's "
	"threshold lies inside the followed range (0, 30] degrees; at 0 a flat ground would turn it.");
static_assert(0.f < kSlopeYawSlopeEpsilonDeg && kSlopeYawSlopeEpsilonDeg < kCameraFollowSlopeEpsilonDeg,
	"A face built at exactly the turn's threshold measures 2.4e-5 degrees less in float (14.9999762), so the threshold "
	"takes a tolerance far above that and far below the followed cut-off's.");
static_assert(kSlopeYawDegPerSec > 0.f,
	"By default the look-mode-off camera turns up a slope; only a console value of 0 or below holds its yaw.");
static_assert(kSlopeYawDwellSeconds > 0.f,
	"By default the look-mode-off turn starts only on a slope target held steady this long (task 21d); only a console "
	"value of 0 or below starts it on the first slope frame, as task 21c did.");
static_assert(0.f < kSlopeYawDwellToleranceDeg && kSlopeYawDwellToleranceDeg < 180.f,
	"The dwell continues only while the up-slope heading stays this close to the one it started on: at 180 any heading "
	"would continue it, and at 0 a 1-ULP change of the guard's up would restart it.");
static_assert(0.f < kSlopeYawDwellEpsilonSeconds && kSlopeYawDwellEpsilonSeconds < 1e-3f,
	"The dwell sums float frame times (48 frames of 1/240 s sum to 0.19999997, below 0.2), so its comparison takes a "
	"tolerance far above that sum's error and below one frame at 1000 fps.");
static_assert(kSlopeYawEaseSeconds > 0.f && kSlopeYawAccelDegPerSec2 > 0.f,
	"By default the look-mode-off turn eases in and out (task 21e); only console values of 0 or below for both give "
	"task 21d's constant-rate turn.");
static_assert(0.f < kSlopeYawArriveDeg && kSlopeYawArriveDeg < kSlopeYawDegPerSec / 1000.f,
	"The turn's arrival snap is positive and below one frame's step of the default rate at 1000 fps, so with the easing "
	"off the turn arrives on task 21d's frame.");

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
	float returnDegPerSec = kReturnDegPerSec;
	float slopePitchMinDeg = kSlopePitchMinDeg;
	float slopePitchMaxDeg = kSlopePitchMaxDeg;
	float slopeYawDegPerSec = kSlopeYawDegPerSec;
	float slopeYawDwellSeconds = kSlopeYawDwellSeconds;
	float slopeYawEaseSeconds = kSlopeYawEaseSeconds;
	float slopeYawAccelDegPerSec2 = kSlopeYawAccelDegPerSec2;
};

struct OrbitCameraInput
{
	bool lookHeld = false;
	glm::vec2 mouseUnits = glm::vec2(0.f);
	glm::vec2 stick = glm::vec2(0.f);
	float targetPitchDeg = 0.f;
	glm::vec3 targetUp = kCameraUp;
	glm::vec3 groundUp = kCameraUp;
	bool groundSupported = false;
	float isoYawDeg = 0.f;
};

static_assert(!OrbitCameraInput{}.groundSupported,
	"An input that does not set the ground (task 21f) has no supported ground, so with look mode off it never turns to "
	"the iso yaw and the camera is task 21e's.");

struct OrbitCameraState
{
	float yawDeg = 0.f;
	float pitchDeg = 0.f;
	float holdoffSeconds = 0.f;
	glm::vec3 frameUp = kCameraUp;
	float slopeDwellSeconds = 0.f;
	float slopeDwellYawDeg = 0.f;
	float slopeYawRateDegPerSec = 0.f;
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

inline PitchLimits slopePitchLimits(const OrbitCameraSettings& settings)
{
	const float maxDeg = std::clamp(settings.slopePitchMaxDeg, kHardPitchMinDeg, kHardPitchMaxDeg);
	return PitchLimits{ std::clamp(settings.slopePitchMinDeg, kHardPitchMinDeg, maxDeg), maxDeg };
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

inline float yawDegFromUEYawDegrees(float ueYawDegrees)
{
	return wrapYawDeg(ueYawDegrees);
}

inline OrbitCameraState seedFromUERotation(float uePitchDegrees, float ueYawDegrees, const OrbitCameraSettings& settings)
{
	return OrbitCameraState{ yawDegFromUEYawDegrees(ueYawDegrees), pitchDegFromUEPitchDegrees(uePitchDegrees, settings),
		0.f };
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

inline bool isCameraUpFlat(const glm::vec3& up)
{
	return up.z > 0.f && up.x * up.x + up.y * up.y <= surfaceFrame::kFlatTolerance * surfaceFrame::kFlatTolerance;
}

inline float cameraFollowMinUpDot()
{
	return std::cos((kCameraFollowMaxSlopeDeg + kCameraFollowSlopeEpsilonDeg) * kSightDegreesToRadians);
}

inline glm::vec3 cameraTargetUp(const glm::vec3& groundUp, bool supported)
{
	if (!supported)
		return kCameraUp;
	const float length = glm::length(groundUp);
	if (!(length > 0.f))
		return kCameraUp;
	const glm::vec3 up = groundUp / length;
	if (isCameraUpFlat(up) || !(up.z >= cameraFollowMinUpDot()))
		return kCameraUp;
	return up;
}

inline float angleBetweenDeg(const glm::vec3& a, const glm::vec3& b)
{
	return std::atan2(glm::length(glm::cross(a, b)), glm::dot(a, b)) * kSightRadiansToDegrees;
}

inline glm::vec3 rotateUpToward(const glm::vec3& from, const glm::vec3& to, float maxStepDeg)
{
	if (!(maxStepDeg > 0.f))
		return from;
	if (!(angleBetweenDeg(from, to) > maxStepDeg))
		return to;
	const glm::vec3 axis = glm::cross(from, to);
	const float sine = glm::length(axis);
	if (!(sine > 0.f))
		return from;
	const float stepRad = maxStepDeg * kSightDegreesToRadians;
	return glm::normalize(from * std::cos(stepRad) + glm::cross(axis / sine, from) * std::sin(stepRad));
}

inline float stepToward(float value, float target, float maxStep)
{
	const float gap = target - value;
	if (!(glm::abs(gap) > maxStep))
		return target;
	return value + ((gap > 0.f) ? maxStep : -maxStep);
}

inline bool turnsUpSlope(const glm::vec3& targetUp)
{
	return std::acos(targetUp.z) * kSightRadiansToDegrees + kSlopeYawSlopeEpsilonDeg >= kSlopeYawMinSlopeDeg;
}

inline float upSlopeYawDeg(const glm::vec3& targetUp)
{
	return std::atan2(-targetUp.y, -targetUp.x) * kSightRadiansToDegrees;
}

inline float slopeYawStep(float yawDeg, const glm::vec3& targetUp, float maxStepDeg)
{
	if (!(maxStepDeg > 0.f) || !turnsUpSlope(targetUp))
		return yawDeg;
	const float targetDeg = wrapYawDeg(upSlopeYawDeg(targetUp));
	const float gapDeg = wrapYawDeg(targetDeg - yawDeg);
	if (!(glm::abs(gapDeg) > maxStepDeg))
		return targetDeg;
	return wrapYawDeg(yawDeg + ((gapDeg > 0.f) ? maxStepDeg : -maxStepDeg));
}

inline bool turnsToIsoYaw(const glm::vec3& groundUp, bool groundSupported)
{
	if (!groundSupported)
		return false;
	const float length = glm::length(groundUp);
	if (!(length > 0.f))
		return false;
	return std::acos(std::clamp(groundUp.z / length, -1.f, 1.f)) * kSightRadiansToDegrees + kSlopeYawSlopeEpsilonDeg
		< kSlopeYawMinSlopeDeg;
}

struct LookOffYawGoal
{
	bool turns;
	float yawDeg;
};

inline LookOffYawGoal lookOffYawGoal(const OrbitCameraInput& input)
{
	if (turnsUpSlope(input.targetUp))
		return LookOffYawGoal{ true, wrapYawDeg(upSlopeYawDeg(input.targetUp)) };
	if (turnsToIsoYaw(input.groundUp, input.groundSupported))
		return LookOffYawGoal{ true, wrapYawDeg(input.isoYawDeg) };
	return LookOffYawGoal{ false, 0.f };
}

struct SlopeDwell
{
	float seconds;
	float yawDeg;
};

inline SlopeDwell slopeDwellStep(const SlopeDwell& dwell, const LookOffYawGoal& goal, float deltaSeconds)
{
	if (!goal.turns)
		return SlopeDwell{ 0.f, dwell.yawDeg };
	if (dwell.seconds > 0.f && glm::abs(wrapYawDeg(goal.yawDeg - dwell.yawDeg)) <= kSlopeYawDwellToleranceDeg)
		return SlopeDwell{ dwell.seconds + deltaSeconds, dwell.yawDeg };
	return SlopeDwell{ deltaSeconds, goal.yawDeg };
}

inline bool slopeDwellElapsed(float dwellSeconds, float requiredSeconds)
{
	return dwellSeconds + kSlopeYawDwellEpsilonSeconds >= std::max(0.f, requiredSeconds);
}

struct SlopeYawTurn
{
	float yawDeg;
	float rateDegPerSec;
};

inline SlopeYawTurn slopeYawEase(float yawDeg, float goalDeg, float rateDegPerSec, float deltaSeconds,
	const OrbitCameraSettings& settings)
{
	const float maxRateDegPerSec = std::max(0.f, settings.slopeYawDegPerSec);
	if (!(maxRateDegPerSec > 0.f))
		return SlopeYawTurn{ yawDeg, 0.f };
	const float gapDeg = wrapYawDeg(goalDeg - yawDeg);
	const float distanceDeg = glm::abs(gapDeg);
	if (!(distanceDeg > kSlopeYawArriveDeg))
		return SlopeYawTurn{ goalDeg, 0.f };
	const float easeSeconds = std::max(0.f, settings.slopeYawEaseSeconds);
	const float accelDegPerSec2 = std::max(0.f, settings.slopeYawAccelDegPerSec2);
	const float targetRateDegPerSec = (easeSeconds > 0.f)
		? std::min(maxRateDegPerSec, distanceDeg * -std::expm1(-deltaSeconds / easeSeconds) / deltaSeconds)
		: maxRateDegPerSec;
	const float nextRateDegPerSec = (accelDegPerSec2 > 0.f)
		? std::min(targetRateDegPerSec, rateDegPerSec + accelDegPerSec2 * deltaSeconds)
		: targetRateDegPerSec;
	const float stepDeg = nextRateDegPerSec * deltaSeconds;
	if (!(distanceDeg > stepDeg))
		return SlopeYawTurn{ goalDeg, 0.f };
	return SlopeYawTurn{ wrapYawDeg(yawDeg + ((gapDeg > 0.f) ? stepDeg : -stepDeg)), nextRateDegPerSec };
}

struct WorldView
{
	float yawDeg;
	float pitchDeg;
};

inline WorldView worldView(const OrbitCameraState& state, const OrbitCameraSettings& settings)
{
	if (isCameraUpFlat(state.frameUp))
		return WorldView{ state.yawDeg, state.pitchDeg };
	const float yawRad = state.yawDeg * kSightDegreesToRadians;
	const glm::vec3 heading(std::cos(yawRad), std::sin(yawRad), 0.f);
	const float riseDeg = std::atan(-glm::dot(state.frameUp, heading) / state.frameUp.z) * kSightRadiansToDegrees;
	const PitchLimits limits = slopePitchLimits(settings);
	const float playerPitchDeg = state.pitchDeg;
	// ⛔G-03  docs/DAttackCamera-guards.md
	const float pitchDeg = std::clamp(playerPitchDeg - riseDeg, std::min(playerPitchDeg, limits.minDeg),
		std::max(playerPitchDeg, limits.maxDeg));
	return WorldView{ state.yawDeg, std::clamp(pitchDeg, kHardPitchMinDeg, kHardPitchMaxDeg) };
}

OGBRAWLER_API OrbitCameraState integrate(float deltaSeconds, const OrbitCameraInput& input,
	const OrbitCameraSettings& settings, const OrbitCameraState& state);
} // namespace dAttackCameraBehaviour

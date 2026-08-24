#pragma once
// SPDX-License-Identifier: BUSL-1.1

// THE ONE DEFINITION of "which way does an attack go, given aim and movement".
//
//   * Pure. glm only. No simulation state, no rendering, no engine types.
//   * ⛔ SIMULATION IS AUTHORITATIVE AND DOES NOT KNOW VISUALIZATION EXISTS.
//     This header sits on the simulation side; the visualization path includes it
//     and consumes the same decision. The dependency runs viz -> sim, never back.
//   * Both callers take the decision from here rather than re-deriving it, so the
//     indicator cannot disagree with the attack it is drawing.
//
// Callers: dAttackMachineSimulation::integrate3 (authoritative) and
// dAttackVisualizationUtils::computeAttackIndicatorGeometry (presentation).
//
// ---------------------------------------------------------------------------
// THE MODEL — three regions of ONE angle.
//
// Measure the UNSIGNED angle between where you aim and where you move, in [0, pi].
// It falls in exactly one of three regions:
//
//   0                      kForwardConeHalfAngle         pi - kBackpedalConeHalfAngle      pi
//   |<------ FORWARD ------>|<--------- SIDE (left or right) --------->|<-- BACKPEDAL -->|
//     moving with your aim        moving across your aim                 moving away from it
//
//   FORWARD   -> forward attack
//   BACKPEDAL -> forward attack TOO. Backing straight off is a forward swing, not a
//                side swing; it is a separate region only because it is a separate
//                interval of the angle, not because it produces a different attack.
//   SIDE      -> left or right, and this is the ONLY region where the sign of the
//                angle is consulted at all.
//
// The two cone half-angles are independent knobs and must stay that way: one sets
// how far off-aim you may drift before a swing goes to a side, the other sets how
// near dead-behind you must be before it stops going to a side. Folding them into
// one constant would make tuning either silently move the other.
// ---------------------------------------------------------------------------

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include "glm/common.hpp"
#include "glm/trigonometric.hpp"
#include "glm/exponential.hpp"
#include "glm/ext/scalar_constants.hpp"

#include "OGBrawler/DAttackSequenceId.h"

namespace dAttackDirection
{

// Half-width of the FORWARD region, measured from the aim direction.
inline constexpr float kForwardConeHalfAngle = glm::pi<float>() / 6.f;    // 30 degrees

// Half-width of the BACKPEDAL region, measured from dead-behind (pi).
inline constexpr float kBackpedalConeHalfAngle = glm::pi<float>() / 18.f; // 10 degrees

// Movement magnitude below which there is no meaningful direction to classify.
inline constexpr float kMoveMagnitudeEpsilon = 0.00001f;

// Attack sequence ids. The left/right labelling follows the existing convention in
// dAttackVisualizationUtils and dAttackAimVisualization: a positive signed angle
// (movement counter-clockwise from aim about +Z) is the LEFT swing.
inline constexpr unsigned int kForwardSequenceId = 4u;
inline constexpr unsigned int kLeftSequenceId    = 1u;
inline constexpr unsigned int kRightSequenceId   = 0u;

// Returns the attack sequence id. Callers ask which region they landed in by
// comparing against the constants above — there is deliberately no second,
// redundant way to ask the same question.
//
// aimDirection / moveDirectionWorld are 3D world vectors; moveDirection is the 2D
// stick, used only for the magnitude gate.
inline unsigned int classify(const glm::vec3& aimDirection,
                             const glm::vec3& moveDirectionWorld,
                             const glm::vec2& moveDirection)
{
	// Standing still: there is no movement direction, so swing forward.
	if (glm::length(moveDirection) < kMoveMagnitudeEpsilon)
		return kForwardSequenceId;

	const glm::vec3 moveN  = glm::normalize(moveDirectionWorld);
	const glm::vec3 moveXY = glm::normalize(glm::vec3(moveN.x, moveN.y, 0.f));
	const glm::vec3 aimXY  = glm::normalize(glm::vec3(aimDirection.x, aimDirection.y, 0.f));

	// Both terms XY-projected. Using the 3D aim here inflates the angle by aim's
	// downward z (mouse-on-floor projection from a capsule above z=0) and trips the
	// threshold even when the XY directions align. Clamp so FP rounding cannot push
	// the dot past 1.0 and turn acos into NaN.
	const float dotXY = glm::clamp(glm::dot(aimXY, moveXY), -1.f, 1.f);
	const float angle = glm::acos(dotXY);   // UNSIGNED, [0, pi]

	// --- FORWARD region: moving roughly with your aim. -----------------------
	if (angle < kForwardConeHalfAngle)
		return kForwardSequenceId;

	// --- BACKPEDAL region: moving roughly opposite your aim. -----------------
	// ⛔⛔ THIS TEST MUST STAY AHEAD OF THE SIGN, AND ON THE UNSIGNED ANGLE.
	// Dead-behind is a sign singularity: moveXY ~= -aimXY makes
	// cross(aimXY, moveXY).z collapse to ~0, so its sign is floating-point noise.
	// Consult it here and the swing flickers left/right every frame — which it did,
	// for the ordinary input of backing straight away from your aim. Classifying the
	// region before the sign exists removes the unstable band instead of damping it.
	if (angle > glm::pi<float>() - kBackpedalConeHalfAngle)
		return kForwardSequenceId;

	// --- SIDE region: the only place the sign is meaningful. ------------------
	const float signedAngle = glm::sign(glm::cross(aimXY, moveXY).z) * angle;

	// Strict `>`: an angle of exactly +kForwardConeHalfAngle lands on the right
	// swing. Pre-existing boundary behaviour, preserved deliberately.
	return signedAngle > kForwardConeHalfAngle ? kLeftSequenceId : kRightSequenceId;
}

} // namespace dAttackDirection

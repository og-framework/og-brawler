// SPDX-License-Identifier: BUSL-1.1
#include "DAttackCamera.h"
#include <algorithm>
#include <stdexcept>
#include "glm/geometric.hpp"
#include "glm/common.hpp"	// glm::abs -- see the task-32 note in integrate()

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

DAttackCameraBehaviour::DAttackCameraBehaviour(float test)
{

}

namespace dAttackCameraBehaviour
{
	void integrate(float deltaSeconds, const DAttackCameraInput& input, DAttackCameraState& state)
	{
		if (!input.getBlockLook())
			return;

		const glm::vec2 cameraAxis = [&input]() {
			if (glm::length(input.getMouseAxis()) > 0.1f)
				return glm::normalize(input.getMouseAxis());
			else if (glm::length(input.getRStickAxis()) > 0.1f)
				return glm::vec2(glm::normalize(input.getRStickAxis()));
			else
				return glm::vec2(0.f, 0.f);
		}();

		if (glm::length(cameraAxis) < 0.1f)
			return;

		const glm::mat4& cameraBoomTransform = state.getCameraBoomTransform();
		const glm::quat cameraBoomRotation = glm::quat_cast(cameraBoomTransform);
		const glm::vec3 cameraBoomEulerAngles = glm::eulerAngles(cameraBoomRotation);
		const float currentPitch = cameraBoomEulerAngles.y;

		dPID::update(input.getTargetPitch(), currentPitch, deltaSeconds, input.getPitchPIDSettings(), state.editPitchPIDState());

		const glm::vec2 normalizedCameraAxis = glm::normalize(cameraAxis);
		// [movement-sim task 32] glm::abs on all THREE sites in this function, NOT
		// unqualified abs. Every operand is a float, and every magnitude here is in
		// [0,1] or a sub-radian pitch -- so C's `::abs(int)`, potentially the only
		// overload in scope on the Godot/Jolt toolchains, would truncate them ALL to 0:
		//   * `abs(x) < abs(y)` becomes `0 < 0`, always false -- the suppression branch
		//     goes unreachable and a vertical gesture scales instead of zeroing.
		//   * `abs(nx) - abs(ny)` on a NORMALIZED vec2 becomes 0 - 0, so the pitch PID
		//     adjustment is multiplied to nothing every frame -- pitch control dies
		//     silently, with no NaN and no discontinuity to notice.
		//   * `abs(clampedPitch)` on a sub-1-radian pitch becomes 0, so distanceFactor
		//     pins at 1 and the boom snaps to its minimum length.
		// Task 32 measured that all three were ALREADY binding the float overload on this
		// toolchain (MSVC 14.38): PORTABILITY HARDENING, not a behaviour fix.
		// ⚠ Pinned by three SEPARATE cases in DAttackAbsQualificationTest.cpp
		// (DAttackAbs.Camera*), and a probe must poison ONE site at a time: poisoning :42
		// and :45 together sends both routes to a zero adjustment and the :42 case goes
		// green again.
		if (glm::abs(cameraAxis.x) < glm::abs(cameraAxis.y))
			state.editPitchPIDState().setAdjustment(0.f);
		else
			state.editPitchPIDState().setAdjustment(state.getPitchPIDState().getAdjustment() * (glm::abs(normalizedCameraAxis.x) - glm::abs(normalizedCameraAxis.y)));

		const float horizontalSensitivity = 1.5f;
		const glm::vec3 eulerAngleToAdd = glm::vec3(0.f, state.editPitchPIDState().getAdjustment() + cameraAxis.y * deltaSeconds, cameraAxis.x * horizontalSensitivity * deltaSeconds);
		const glm::vec3 newBoomEulerAngles = cameraBoomEulerAngles + eulerAngleToAdd;
		const glm::quat newBoomRotation = glm::quat(newBoomEulerAngles);
		state.setCameraBoomTransform(glm::mat4_cast(newBoomRotation));

		const float minPitch = 0.f;
		const float clampedPitch = std::clamp(currentPitch, minPitch, input.getTargetPitch());
		const float distanceFactor = 1.f - (glm::abs(clampedPitch) / (input.getTargetPitch()));
		state.setCameraBoomLength(900.f - 500.f * distanceFactor);
	}
}
OGSIM_OPTIMIZE_ON

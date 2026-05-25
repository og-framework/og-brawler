// SPDX-License-Identifier: BUSL-1.1
#include "DAttackCamera.h"
#include <algorithm>
#include <stdexcept>
#include "glm/geometric.hpp"

#pragma optimize( "", off )

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
		if (abs(cameraAxis.x) < abs(cameraAxis.y))
			state.editPitchPIDState().setAdjustment(0.f);
		else
			state.editPitchPIDState().setAdjustment(state.getPitchPIDState().getAdjustment() * (abs(normalizedCameraAxis.x) - abs(normalizedCameraAxis.y)));

		const float horizontalSensitivity = 1.5f;
		const glm::vec3 eulerAngleToAdd = glm::vec3(0.f, state.editPitchPIDState().getAdjustment() + cameraAxis.y * deltaSeconds, cameraAxis.x * horizontalSensitivity * deltaSeconds);
		const glm::vec3 newBoomEulerAngles = cameraBoomEulerAngles + eulerAngleToAdd;
		const glm::quat newBoomRotation = glm::quat(newBoomEulerAngles);
		state.setCameraBoomTransform(glm::mat4_cast(newBoomRotation));

		const float minPitch = 0.f;
		const float clampedPitch = std::clamp(currentPitch, minPitch, input.getTargetPitch());
		const float distanceFactor = 1.f - (abs(clampedPitch) / (input.getTargetPitch()));
		state.setCameraBoomLength(900.f - 500.f * distanceFactor);
	}
}
#pragma optimize( "", on )

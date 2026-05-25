#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include <vector>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "OGSimulation/DPID.h"
#include <glm/gtc/quaternion.hpp>

class DAttackCameraInput
{
public:

	DAttackCameraInput(glm::vec3 rStickAxis, glm::vec2 mouseAxis, bool blockLook, float targetPitch, const DPIDSettings& pitchPIDSettings)
		: m_rStickAxis(rStickAxis)
		, m_mouseAxis(mouseAxis)
		, m_blockLook(blockLook)
		, m_targetPitch(targetPitch)
		, m_pitchPIDSettings(pitchPIDSettings)
	{
	}

	const glm::vec3& getRStickAxis() const { return m_rStickAxis; }
	const glm::vec2& getMouseAxis() const { return m_mouseAxis; }
	bool getBlockLook() const { return m_blockLook; }
	float getTargetPitch() const { return m_targetPitch; }
	const DPIDSettings& getPitchPIDSettings() const { return m_pitchPIDSettings; }

private:
	glm::vec3 m_rStickAxis;
	glm::vec2 m_mouseAxis;
	bool m_blockLook;
	float m_targetPitch;

	DPIDSettings m_pitchPIDSettings;
};

class DAttackCameraState
{
public:
	DAttackCameraState()
		: m_pitchPIDState()
		, m_cameraTransform(glm::identity<glm::mat4>())
		, m_cameraBoomTransform(glm::identity<glm::mat4>())
		, m_cameraBoomLength(0.f)
	{
	}

	const DPIDState& getPitchPIDState() const { return m_pitchPIDState; }
	const glm::mat4& getCameraTransform() const { return m_cameraTransform; }
	const glm::mat4& getCameraBoomTransform() const { return m_cameraBoomTransform; }
	float getCameraBoomLength() const { return m_cameraBoomLength; }

	void setPitchPIDState(const DPIDState& pitchPIDState) { m_pitchPIDState = pitchPIDState; }
	void setCameraTransform(const glm::mat4& cameraTransform) { m_cameraTransform = cameraTransform; }
	void setCameraBoomTransform(const glm::mat4& cameraBoomTransform) { m_cameraBoomTransform = cameraBoomTransform; }
	void setCameraBoomLength(float cameraBoomLength) { m_cameraBoomLength = cameraBoomLength; }

	DPIDState& editPitchPIDState() { return m_pitchPIDState; }
private:

	DPIDState m_pitchPIDState;

	glm::mat4 m_cameraTransform;
	glm::mat4 m_cameraBoomTransform;
	float m_cameraBoomLength;
};

#pragma optimize( "", off )

namespace dAttackCameraBehaviour
{
	OGBRAWLER_API void integrate(float deltaSeconds, const DAttackCameraInput& input, DAttackCameraState& state);
}
#pragma optimize( "", on )


class DAttackCameraBehaviour
{
public:
 	OGBRAWLER_API DAttackCameraBehaviour(float test);
private:
	DAttackCameraBehaviour() = default;
};


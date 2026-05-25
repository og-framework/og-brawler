// SPDX-License-Identifier: BUSL-1.1
#include "DAttackRadialSimulation.h"

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace dAttackRadialSimulation
{

DAttackSegment getAttackSegment(const InitialConditions& initialConditions, const StaticData& staticData, const glm::vec3& directionInRotationPlane)
{
	const DAttackRadialSequence& sequence = staticData.getAttackSequences()[initialConditions.activeAttackSequence];
	const DAttackCircle& circle = staticData.getAttackCircle();

	const glm::mat4 initialRotation = glm::rotate(glm::mat4(1.f), initialConditions.initialAimAngle, initialConditions.initialAimRotationAxis);
	const glm::mat4 initialRotationInverse = glm::inverse(initialRotation);
	const glm::vec3 localCurrentDirection = initialRotationInverse * glm::vec4(directionInRotationPlane, 0.f);
	const glm::vec2 localCurrentDirection2 = sequence.getVectorInRotationPlane(localCurrentDirection);
	return sequence.getAttackSegment(localCurrentDirection2);
}

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

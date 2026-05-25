#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include <vector>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

enum class DAttackRadialSequenceState
{
	WindUp,
	Damaging,
	WindDown,
	Idle,
};

struct DAttackRadialSequencePoint
{
	float time;
	float angle;
	DAttackRadialSequenceState state;
};

struct DAttackSegment
{
	float startAngle;
	float endAngle;
	unsigned int index;
	DAttackRadialSequenceState state;
};

class DAttackRadialSequence
{
public:
 	OGBRAWLER_API DAttackRadialSequence(const std::vector<DAttackRadialSequencePoint>& attackPoints, float timeToReachZeroVelocity, glm::vec3 rotationAxis);
	OGBRAWLER_API float getAngularAcceleration(float time) const;
	OGBRAWLER_API float getAngle(float time) const;
	
	OGBRAWLER_API float getInitialAngle() const;
	OGBRAWLER_API float getInitialVelocity() const;

	OGBRAWLER_API glm::vec2 getDirection(unsigned int index) const;
	OGBRAWLER_API glm::vec3 getRotationAxis() const;

	OGBRAWLER_API DAttackSegment getAttackSegment(unsigned int index) const;
	OGBRAWLER_API DAttackSegment getAttackSegment(float angle) const;
	OGBRAWLER_API DAttackSegment getAttackSegment(const glm::vec2& localDirection) const;
	OGBRAWLER_API DAttackRadialSequenceState getDAttackRadialSequenceState(float angle) const;
	OGBRAWLER_API DAttackRadialSequenceState getDAttackRadialSequenceState(unsigned int index) const;

	OGBRAWLER_API float getDuration() const;
	OGBRAWLER_API unsigned int getAttackPointCount() const;
	OGBRAWLER_API unsigned int getAttackSegmentCount() const;

	OGBRAWLER_API static glm::vec3 defaultUp() { return glm::vec3(0.0f, 0.0f, 1.0f);}
	OGBRAWLER_API static glm::vec3 defaultForward() { return glm::vec3(1.0f, 0.0f, 0.0f);}
	OGBRAWLER_API const glm::mat4& getFromRotationAxisToDefaultUp() const { return m_fromRotationAxisToDefaultUp; }
	OGBRAWLER_API glm::vec2 getVectorInRotationPlane(glm::vec3 vectorInWorld) const;

private:
	DAttackRadialSequence() = delete;

	std::vector<DAttackRadialSequencePoint> m_attackPoints;
	std::vector<float> m_accelerations;
	std::vector<float> m_velocityAtPoint;

	const float m_timeToReachZeroVelocity;
	const glm::vec3 m_rotationAxis;
	glm::mat4 m_fromRotationAxisToDefaultUp;
};


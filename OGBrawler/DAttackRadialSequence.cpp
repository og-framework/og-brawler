// SPDX-License-Identifier: BUSL-1.1
#include "DAttackRadialSequence.h"
#include "OGSimulation/OGAssert.h"
#include "OGSimulation/DMathUtil.h"
#include <algorithm>
#include "glm/geometric.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/vector_angle.hpp>
#include <glm/gtc/constants.hpp>

DAttackRadialSequence::DAttackRadialSequence(const std::vector<DAttackRadialSequencePoint>& attackPoints, float timeToReachZeroVelocity, glm::vec3 rotationAxis)
	: m_attackPoints(attackPoints)
	, m_accelerations()
	, m_velocityAtPoint()
	, m_timeToReachZeroVelocity(timeToReachZeroVelocity)
	, m_rotationAxis(rotationAxis)
	, m_fromRotationAxisToDefaultUp(dMathUtil::getRotationMatrix(m_rotationAxis, defaultUp()))
	//, m_fromRotationAxisToDefaultUp(dMathUtil::getRotationMatrix(defaultUp(), m_rotationAxis))
{
	OG_CHECK(attackPoints.size() >= 1, "attackPoints must have at least one point");
	OG_CHECK(attackPoints[0].time == 0.f, "First attack point must have time 0");

	auto getAccelerationAngleKnown = [](float t1, float t0, float w0, float o1, float o0) {
		const float dt = t1 - t0;
		return (2 * ((o1 - o0) - w0 * dt)) / (dt * dt);
	};

	auto getAngleAccelerationKnown = [](float t1, float t0, float w0, float a) {
		const float dt = t1 - t0;
		return w0*dt + a * 0.5f * (dt * dt);
	};

	auto getVelocity = [](float t1, float t0, float w0, float a) {
		return w0 + a * (t1 - t0);
	};

	auto getAccelerationVelocityKnown = [](float t1, float t0, float w1, float w0) {
		return (w1 - w0) / (t1 - t0);
	};

	m_velocityAtPoint.push_back(0.f);
	for (auto attackPointIt = m_attackPoints.begin(); attackPointIt != m_attackPoints.end(); ++attackPointIt)
	{
		auto attackPointNextIt = attackPointIt + 1;
		if (attackPointNextIt == m_attackPoints.end())
			break;

		const float t1 = attackPointNextIt->time;
		const float t0 = attackPointIt->time;
		OG_CHECK(t1 != t0, "Attack points must have different times");

		const float o1 = attackPointNextIt->angle;
		const float o0 = attackPointIt->angle;
		const float w0 = m_velocityAtPoint.back();
		const float a0 = getAccelerationAngleKnown(t1, t0, w0, o1, o0);
		const float w1 = getVelocity(t1, t0, w0, a0);
		m_accelerations.push_back(a0);
		m_velocityAtPoint.push_back(w1);
	}

	//reach zero velocity at the end
	const float t1 = m_attackPoints.back().time + m_timeToReachZeroVelocity;
	const float t0 = m_attackPoints.back().time;
	const float w1 = 0.f;
	const float w0 = m_velocityAtPoint.back();
	const float a0 = getAccelerationVelocityKnown(t1, t0, w1, w0);
	m_accelerations.push_back(a0);
	const float w1Validation = getVelocity(t1, t0, w0, a0);
	m_velocityAtPoint.push_back(w1Validation);
	m_attackPoints.push_back({ t1, m_attackPoints.back().angle + getAngleAccelerationKnown(t1, t0, w0, a0), DAttackRadialSequenceState::WindDown});
}

float DAttackRadialSequence::getAngularAcceleration(float time) const
{
	for (auto attackPointIt = m_attackPoints.begin(); attackPointIt != m_attackPoints.end(); ++attackPointIt)
	{
		auto attackPointNextIt = attackPointIt + 1;
		if (time >= attackPointIt->time && time < attackPointNextIt->time)
			return m_accelerations[attackPointIt - m_attackPoints.begin()];

		if (attackPointNextIt == m_attackPoints.end())
			break;
	}

	return 0.f;
}

float DAttackRadialSequence::getAngle(float time) const
{
	if (time <= m_attackPoints.front().time)
		return m_attackPoints.front().angle;

	for (size_t i = 0; i + 1 < m_attackPoints.size(); ++i)
	{
		const float t0 = m_attackPoints[i].time;
		const float t1 = m_attackPoints[i + 1].time;
		if (time >= t0 && time < t1)
		{
			const float dt = time - t0;
			const float o0 = m_attackPoints[i].angle;
			const float w0 = m_velocityAtPoint[i];
			const float a  = m_accelerations[i];
			return o0 + w0 * dt + 0.5f * a * dt * dt;
		}
	}

	return m_attackPoints.back().angle;
}

float DAttackRadialSequence::getInitialAngle() const
{
	return m_attackPoints.front().angle;
}

float DAttackRadialSequence::getInitialVelocity() const
{
	return m_velocityAtPoint.front();
}

glm::vec2 DAttackRadialSequence::getDirection(unsigned int index) const
{
	const float angle = m_attackPoints[index].angle;
	return glm::vec2(std::cos(angle), std::sin(angle));
}

glm::vec3 DAttackRadialSequence::getRotationAxis() const
{
	return m_rotationAxis;
}

DAttackSegment DAttackRadialSequence::getAttackSegment(unsigned int index) const
{
	return DAttackSegment{ m_attackPoints[index].angle, m_attackPoints[index + 1].angle, index, getDAttackRadialSequenceState(index)};
}

DAttackSegment DAttackRadialSequence::getAttackSegment(float angle) const
{
	for (auto attackPointIt = m_attackPoints.begin(); attackPointIt != m_attackPoints.end(); ++attackPointIt)
	{
		auto attackPointNextIt = attackPointIt + 1;
		if (angle >= attackPointIt->angle && angle < attackPointNextIt->angle ||
			angle < attackPointIt->angle && angle >= attackPointNextIt->angle)
		{
			const unsigned int index = (attackPointIt - m_attackPoints.begin());
			return DAttackSegment{ attackPointIt->angle, attackPointNextIt->angle, index, getDAttackRadialSequenceState(index) };
		}
	}

	return DAttackSegment{ 0, 0, static_cast<unsigned int>(m_attackPoints.size()), DAttackRadialSequenceState ::Idle};
}

DAttackSegment DAttackRadialSequence::getAttackSegment(const glm::vec2& localDirection) const
{
	//auto isBetween = [](const glm::vec2& target, const glm::vec2& vec1, const glm::vec2& vec2, bool positiveRotationDirection) {
	//	float crossProduct = vec1.x * target.y - vec1.y * target.x;
	//	float crossProduct2 = vec2.x * target.y - vec2.y * target.x;
	//	float crossProduct3 = vec1.x * vec2.y - vec1.y * vec2.x;

	//	if (positiveRotationDirection)
	//	{
	//		if (crossProduct3 > 0)
	//			return crossProduct >= 0 && crossProduct2 < 0;
	//		else
	//			return !(crossProduct < 0 && crossProduct2 >= 0);
	//	}
	//	else
	//	{
	//		if (crossProduct3 < 0)
	//			return crossProduct <= 0 && crossProduct2 > 0;
	//		else
	//			return !(crossProduct >= 0 && crossProduct2 > 0);
	//	}
	//};

	//for (auto attackPointIt = m_attackPoints.begin(); attackPointIt != m_attackPoints.end(); ++attackPointIt)
	//{
	//	auto attackPointNextIt = attackPointIt + 1;

	//	glm::vec2 current(std::cos(attackPointIt->angle), std::sin(attackPointIt->angle));
	//	glm::vec2 next(std::cos(attackPointNextIt->angle), std::sin(attackPointNextIt->angle));
	//	if (isBetween(localDirection, current, next, attackPointNextIt->angle > attackPointIt->angle))
	//	{
	//		const unsigned int index = (attackPointIt - m_attackPoints.begin());
	//		return DAttackSegment{ attackPointIt->angle, attackPointNextIt->angle, index, getDAttackRadialSequenceState(index) };
	//	}
	//}

	//return DAttackSegment{ 0, 0, static_cast<unsigned int>(m_attackPoints.size()), DAttackRadialSequenceState::Idle };

	bool velocityIsPositive = m_velocityAtPoint[1] > 0.f;
	auto& firstPoint = m_attackPoints.front();
	glm::vec2 initialDirection(std::cos(firstPoint.angle), std::sin(firstPoint.angle));
	float initialAngle = firstPoint.angle;
	float angle = /*2.f * glm::pi<float>() -*/ glm::orientedAngle(initialDirection, localDirection);

	if (velocityIsPositive)
	{ 
		if (angle < 0.f)
			angle += 2.f * glm::pi<float>();
	}
	else
	{
		if (angle < 0.f)
			angle = std::abs(angle);
		else
			angle = 2.f * glm::pi<float>() - angle;
	}


	for (auto attackPointIt = m_attackPoints.begin(); attackPointIt != (m_attackPoints.end() - 1); ++attackPointIt)
	{
		auto attackPointNextIt = attackPointIt + 1;
		const float thisAngle = std::abs(attackPointIt->angle - initialAngle);
		const float nextAngle = std::abs(attackPointNextIt->angle - initialAngle);
		if (angle >= /*std::abs*/(thisAngle) && angle < /*std::abs*/(nextAngle))
		{
			const unsigned int index = (attackPointIt - m_attackPoints.begin());
			return DAttackSegment{ attackPointIt->angle, attackPointNextIt->angle, index, getDAttackRadialSequenceState(index) };
		}
	}
	return DAttackSegment{ 0, 0, static_cast<unsigned int>(m_attackPoints.size()), DAttackRadialSequenceState::Idle }; // TODO
}

DAttackRadialSequenceState DAttackRadialSequence::getDAttackRadialSequenceState(float angle) const
{
	for (auto attackPointIt = m_attackPoints.begin(); attackPointIt != m_attackPoints.end(); ++attackPointIt)
	{
		auto attackPointNextIt = attackPointIt + 1;
		if (angle >= attackPointIt->angle && angle < attackPointNextIt->angle ||
			angle < attackPointIt->angle && angle >= attackPointNextIt->angle)
		{
			const unsigned int index = (attackPointIt - m_attackPoints.begin());
			return getDAttackRadialSequenceState(index);
		}
	}

	return DAttackRadialSequenceState::Idle;
}

DAttackRadialSequenceState DAttackRadialSequence::getDAttackRadialSequenceState(unsigned int index) const
{
	return m_attackPoints[index].state;
}

float DAttackRadialSequence::getDuration() const
{
	return m_attackPoints.back().time;
}

unsigned int DAttackRadialSequence::getAttackPointCount() const
{
	return static_cast<unsigned int>(m_attackPoints.size());
}

unsigned int DAttackRadialSequence::getAttackSegmentCount() const
{
	return m_attackPoints.size() - 1;
}

glm::vec2 DAttackRadialSequence::getVectorInRotationPlane(glm::vec3 vectorInWorld) const
{
	if (glm::length(m_rotationAxis - defaultUp()) > 0.0001f)
		return glm::vec2(m_fromRotationAxisToDefaultUp * glm::vec4(vectorInWorld, 0.f));
	else
		return glm::vec2(vectorInWorld);
}


#pragma optimize( "", on )

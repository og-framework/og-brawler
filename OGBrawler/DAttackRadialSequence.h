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
	// [movement-sim task 83] The AUTHORED angular velocity at `time`, i.e. the exact derivative
	// of getAngle within a segment. Its SIGN is what turns the swing plane's tangent into the
	// weapon's direction of TRAVEL, which is the direction a hit throws its target.
	// The captured body angularVelocity would answer the same question from the engine's
	// integration and therefore differ between peers; this one is table data and does not.
	OGBRAWLER_API float getAngularVelocity(float time) const;
	
	OGBRAWLER_API float getInitialAngle() const;
	OGBRAWLER_API float getInitialVelocity() const;

	OGBRAWLER_API glm::vec2 getDirection(unsigned int index) const;
	OGBRAWLER_API glm::vec3 getRotationAxis() const;

	OGBRAWLER_API DAttackSegment getAttackSegment(unsigned int index) const;
	OGBRAWLER_API DAttackSegment getAttackSegment(float angle) const;
	OGBRAWLER_API DAttackSegment getAttackSegment(const glm::vec2& localDirection) const;
	OGBRAWLER_API DAttackRadialSequenceState getDAttackRadialSequenceState(float angle) const;
	OGBRAWLER_API DAttackRadialSequenceState getDAttackRadialSequenceState(unsigned int index) const;

	// ⛔⛔ [movement-sim task 87] THIS IS NOT THE AUTHORED WINDOW ITS NAME PROMISES.
	// The constructor APPENDS one more point at `m_attackPoints.back().time +
	// timeToReachZeroVelocity` (the "reach zero velocity at the end" block), and this returns
	// `m_attackPoints.back().time` AFTER that append. So it is the LAST AUTHORED KEYFRAME TIME
	// PLUS `timeToReachZeroVelocity` -- 0.1 s longer than the table for every shipped sequence
	// (seq 0/1 0.6 -> 0.7 s, seq 2/3 0.4 -> 0.5 s, seq 4 0.42 -> 0.52 s).
	// ⛔ IT HAS COST TWO HAND DERIVATIONS IN THIS INITIATIVE -- task 84 (a wrong
	// 0.7000000476837158) and task 86 (SIX ticks, which nearly shipped a stun one tick short).
	// The truth was already written down in `dAttackMachineSimulation::swingTickCount`'s
	// OG_CHECK ("plus the appended zero-velocity point") and BOTH readers still missed it, which
	// is why it now sits HERE, on the declaration nobody can call without reading.
	// ⭐ This is what the MACHINE reads when it sizes a swing
	// (`DAttackMachineSimulation.h`, the two `swingTickCount(... .getDuration(), ...)` sites);
	// read `m_attackPoints` / the authored table directly if the AUTHORED window is what you
	// want. ⚠ Behaviour unchanged by task 87 -- the name and the value are load-bearing for
	// every caller and this edit is legibility only.
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


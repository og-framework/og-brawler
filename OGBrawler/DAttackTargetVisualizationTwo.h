#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext/scalar_constants.hpp"
#include "DAttackRadialSimulation.h"
#include "DAttackGuardSimulation.h"
#include "DAttackMachineSimulation.h"
#include "DAttackCircle.h"
#include "DAttackVisualizationUtils.h"
#include "OGSimulation/SpatialQueryAdapter.h"
#include "OGSimulation/SpatialQueryResult.h"
#include "OGSimulation/QueryGeometry.h"

#pragma optimize( "", off )

class DAttackRadialSequence;

namespace dAttackTargetVisualizationTwo
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class State
{
public:
	State(const std::vector<QueryVolumeId>& volumeIds)
		: m_volumeIds(volumeIds)
	{}

	State(const State& other)
		: m_volumeIds(other.m_volumeIds)
	{}

	const std::vector<QueryVolumeId>& getVolumeIds() const { return m_volumeIds; }

private:
	State() = default;

	std::vector<QueryVolumeId> m_volumeIds;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SpatialQueryAdapterType, typename RendererFunctorType>
class Input
{
public:
	Input(float deltaTime,
		const glm::vec3& aimDirection,
		SpatialQueryAdapterType& queryAdapter,
		RendererFunctorType rendererFunctorImpl)
		: m_deltaTime(deltaTime)
		, aimDirection(aimDirection)
		, m_queryAdapter(queryAdapter)
		, rendererFunctorImpl(rendererFunctorImpl)
	{}

	const glm::vec3& getAimDirection() const { return aimDirection; }
	float getDeltaTime() const { return m_deltaTime; }
	SpatialQueryAdapterType& getQueryAdapter() const { return m_queryAdapter; }
	RendererFunctorType getRendererFunctorImpl() const { return rendererFunctorImpl; }

private:
	float m_deltaTime;
	const glm::vec3 aimDirection;
	SpatialQueryAdapterType& m_queryAdapter;
	RendererFunctorType rendererFunctorImpl;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename SpatialQueryAdapterType, typename RendererFunctorType>
void visualize(const Input<SpatialQueryAdapterType, RendererFunctorType>& input,
	State& state,
	const dAttackRadialSimulation::StaticData& staticData,
	const dAttackMachineSimulation::State& machineSimulationState,
	const dAttackRadialSimulation::State& radialState,
	const dAttackGuardSimulation::State& guardState)
{
	auto rendererFunctor = input.getRendererFunctorImpl();
	const bool isAttacking = machineSimulationState.m_currentState == DAttackState::Attacking;

	const glm::mat4 rootTransform = glm::translate(glm::mat4(1.f), radialState.bodyState.position)
	                               * glm::mat4_cast(radialState.bodyState.rotation);
	const glm::vec3 rootTranslation = glm::vec3(rootTransform[3]);

	const glm::mat4 guardTransform = glm::translate(glm::mat4(1.f), guardState.bodyState.position)
	                                * glm::mat4_cast(guardState.bodyState.rotation);
	const glm::vec3 guardTranslation = glm::vec3(guardTransform[3]);
	const glm::mat3x3 guardRotation = glm::mat3x3(guardTransform);
	glm::vec3 guardForward = glm::vec3(guardTransform[0]);
	//rendererFunctor.drawSolidBox(guardTranslation, guardRotation, glm::vec3(30.f, 30.f, 5.f), 2);
	//rendererFunctor.drawLine(guardTranslation, guardTranslation + guardForward * 100.f, 2, 1.f);

	for (const auto& volumeId : state.getVolumeIds())
		input.getQueryAdapter().setVolumeParentTransform(volumeId, rootTransform);

	auto queryReport = input.getQueryAdapter().overlap(state.getVolumeIds());

	if (queryReport.empty())
		return;
	
	const float shieldAngle = 0.25f;
	const float outerShieldAngle = glm::pi<float>()*0.5;
	const float shieldAngleDiff = outerShieldAngle - shieldAngle;
	const float middlePointShieldAngle = shieldAngle + (shieldAngleDiff*0.5f);

	for (const auto& result : queryReport)
	{
		glm::vec3 collidingPosition = result.objectPosition;
		collidingPosition.z = rootTranslation.z;
		const glm::vec3 collisionDirection = collidingPosition - rootTranslation;
		const glm::vec3 normalizedCollisionDirection = glm::normalize(collisionDirection);
		glm::vec4 normalizedCollisionDirection4(normalizedCollisionDirection, 0.f);
		const glm::vec3 ortogonalCollisionDirection = glm::cross(normalizedCollisionDirection, glm::vec3(0.f, 0.f, 1.f));
		//rendererFunctor.drawLine(collidingPosition + ortogonalCollisionDirection* staticData.getAttackCircle().getOuterRadius(), collidingPosition - ortogonalCollisionDirection * staticData.getAttackCircle().getOuterRadius(), 2, 1.f);


		const float arcRadius = std::min(staticData.getAttackCircle().getOuterRadius(), glm::length(collisionDirection));
		const float targetAngleDifference = std::acos(glm::dot(normalizedCollisionDirection, input.getAimDirection()));
		if (targetAngleDifference > outerShieldAngle)
			continue;

		glm::vec3 closeGuardPoint = rootTranslation + normalizedCollisionDirection * staticData.getAttackCircle().getInnerRadius();
		rendererFunctor.drawLine(closeGuardPoint, collidingPosition, 2, 1.f);

		const glm::vec3 guardAxis = glm::cross(normalizedCollisionDirection, input.getAimDirection());
		dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, rootTranslation, normalizedCollisionDirection, guardAxis, shieldAngle * 2.f, staticData.getAttackCircle().getInnerRadius(), 0.f, 2, 1.f);
		glm::mat4 leftMiddlePointShieldRotation = glm::rotate(glm::mat4(1.f), middlePointShieldAngle, guardAxis);
		glm::vec3 leftMiddlePointDirection = glm::vec3(leftMiddlePointShieldRotation * normalizedCollisionDirection4);		
		glm::mat4 rightMiddlePointShieldRotation = glm::rotate(glm::mat4(1.f), middlePointShieldAngle, -guardAxis);
		glm::vec3 rightMiddlePointDirection = glm::vec3(rightMiddlePointShieldRotation * normalizedCollisionDirection4);
	/*	dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, rootTranslation, leftMiddlePointDirection, guardAxis, shieldAngleDiff, staticData.getAttackCircle().getInnerRadius(), 1.f, 2, 1.f);
		dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, rootTranslation, rightMiddlePointDirection, -guardAxis, shieldAngleDiff, staticData.getAttackCircle().getInnerRadius(), 1.f, 2, 1.f);*/

		if (!isAttacking)
		{
			if (targetAngleDifference < shieldAngle)
			{
				//dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, rootTranslation, normalizedCollisionDirection, guardAxis, shieldAngle*2.f, arcRadius, 0.f, 2, 1.f);

				dAttackVisualizationUtils::drawSegmentSolid(rendererFunctor, rootTranslation, normalizedCollisionDirection, guardAxis, shieldAngle * 2.f, staticData.getAttackCircle().getInnerRadius(), 1.0f, 2, 1.f);
				const float segmentLength = shieldAngle * 2.f * staticData.getAttackCircle().getInnerRadius();
				//dAttackVisualizationUtils::drawArcTriangle(rendererFunctor, rootTranslation, normalizedCollisionDirection, guardAxis, shieldAngle * 2.f, staticData.getAttackCircle().getInnerRadius(), 2);
			
				const glm::vec3 guardDirection = rootTranslation + normalizedCollisionDirection * staticData.getAttackCircle().getInnerRadius() - collidingPosition;
				//rendererFunctor.drawLine(rootTranslation + normalizedCollisionDirection * staticData.getAttackCircle().getInnerRadius(), collidingPosition, 0, 2.f);
				//rendererFunctor.drawCircleArc(collidingPosition, -normalizedCollisionDirection, glm::length(guardDirection), shieldAngle * 2.f, 0, 1.f);
				const float predictedAttackArcAngle = segmentLength / glm::length(guardDirection);
				//dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, collidingPosition, -normalizedCollisionDirection, guardAxis, predictedAttackArcAngle, length(guardDirection), 0.f, 0, 1.f);
			}
			else
			{
				glm::mat4 middlePointShieldRotation = glm::rotate(glm::mat4(1.f), middlePointShieldAngle, guardAxis);
				glm::vec3 middlePointDirection = glm::vec3(middlePointShieldRotation * normalizedCollisionDirection4);
				//dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, rootTranslation, middlePointDirection, guardAxis, shieldAngleDiff, arcRadius, 0.f, 2, 1.f);

				dAttackVisualizationUtils::drawSegmentSolid(rendererFunctor, rootTranslation, middlePointDirection, guardAxis, shieldAngleDiff, staticData.getAttackCircle().getInnerRadius(), 1.f, 2, 1.f);

				glm::vec4 middlePointDirection4(middlePointDirection, 0.f);

				glm::mat4 innerRotaition = glm::rotate(glm::mat4(1.f), shieldAngleDiff * 0.5f, guardAxis);
				glm::vec3 innerCollisionDirection = glm::vec3(innerRotaition * middlePointDirection4);
				const glm::vec3 innerGuardPoint = rootTranslation + innerCollisionDirection * staticData.getAttackCircle().getInnerRadius();
				//rendererFunctor.drawLine(innerGuardPoint, collidingPosition, 0, 1.f);
				const glm::vec3 targetAttackDirection = innerGuardPoint - collidingPosition;
				const float targetAttackDirectionLength = glm::length(targetAttackDirection);
				const glm::vec3 normalizedTargetAttackDirection = glm::normalize(targetAttackDirection);
				const float predictedAttackArcAngle = glm::pi<float>() * 0.5f * 0.5;
				glm::mat4 predictedDirectionRotation  = glm::rotate(glm::mat4(1.f), predictedAttackArcAngle, -guardAxis);
				//rendererFunctor.drawCircleArc(collidingPosition, (predictedDirectionRotation * glm::vec4(normalizedTargetAttackDirection, 0.f)) * targetAttackDirectionLength, targetAttackDirectionLength, predictedAttackArcAngle, 0, 1.f);


				//dAttackVisualizationUtils::drawArcTriangle(rendererFunctor, rootTranslation, middlePointDirection, guardAxis, shieldAngleDiff, staticData.getAttackCircle().getInnerRadius(), 2);
			}		
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}

#pragma optimize( "", on )



 
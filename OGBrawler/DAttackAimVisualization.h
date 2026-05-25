#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DAttackRadialSimulation.h"
#include "DAttackCircle.h"
#include "OGSimulation/DMathUtil.h"
#include "DAttackVisualizationUtils.h"

#pragma optimize( "", off )


class DAttackRadialSequence;

namespace dAttackAimVisualization
{

class State
{
public:
	State()
	{}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename RendererFunctorType, typename LoggingFunctorType>
class Input
{
public:
	Input(float deltaTime,
		const glm::vec3& aimDirection,
		RendererFunctorType rendererFunctorImpl,
		LoggingFunctorType loggingFunctor,
		const glm::vec2& moveDirection,
		const glm::vec3& moveDirectionWorld)
		: m_deltaTime(deltaTime)
		, aimDirection(aimDirection)
		, rendererFunctorImpl(rendererFunctorImpl)
		, loggingFunctor(loggingFunctor)
		, moveDirection(moveDirection)
		, moveDirectionWorld(moveDirectionWorld)
	{}

	const glm::vec3& getAimDirection() const { return aimDirection; }
	float getDeltaTime() const { return m_deltaTime; }
	RendererFunctorType getRendererFunctorImpl() const { return rendererFunctorImpl; }
	LoggingFunctorType getLoggingFunctor() const { return loggingFunctor; }
	const glm::vec2& getMoveDirection() const { return moveDirection; }
	const glm::vec3& getMoveDirectionWorld() const { return moveDirectionWorld; }

private:
	Input() = default;

	float m_deltaTime;
	const glm::vec3 aimDirection;
	RendererFunctorType rendererFunctorImpl;
	LoggingFunctorType loggingFunctor;
	glm::vec2 moveDirection;
	glm::vec3 moveDirectionWorld;
};


template <typename RendererFunctorType, typename LoggingFunctorType>
void visualize(const Input<RendererFunctorType, LoggingFunctorType>& input,
	const dAttackRadialSimulation::State& simulationState,
	const dAttackRadialSimulation::InitialConditions& simulationInitialConditions,
	const dAttackRadialSimulation::DerivedState& radialSimulationDerivedState,
	const dAttackRadialSimulation::StaticData& radialSimulationStaticData,
	State& state)
{
	const DAttackRadialSequence& sequence = radialSimulationStaticData.getAttackSequences()[simulationInitialConditions.activeAttackSequence];
	const DAttackCircle& circle = radialSimulationStaticData.getAttackCircle();
	auto rendererFunctor = input.getRendererFunctorImpl();

	const glm::mat4 rootTransform = glm::translate(glm::mat4(1.f), simulationState.bodyState.position)
	                               * glm::mat4_cast(simulationState.bodyState.rotation);
	const glm::vec3 rootTranslation = glm::vec3(rootTransform[3]);
	glm::mat3 rootRotation = glm::mat3(rootTransform);


	const float attackDirectionAngleIncrement = glm::pi<float>() / 6.f;
	const float sideAngle = (glm::pi<float>() / 2.f) - (attackDirectionAngleIncrement);

	const float aimArchAngle = 0.1f;
	rendererFunctor.drawLine(rootTranslation - input.getAimDirection() * 30.f, rootTranslation + input.getAimDirection() * radialSimulationStaticData.getAttackCircle().getOuterRadius(), 1, 1.f);
	rendererFunctor.drawCircleArc(rootTranslation, input.getAimDirection(), radialSimulationStaticData.getAttackCircle().getOuterRadius(), attackDirectionAngleIncrement, 1, 1.f);
	rendererFunctor.drawCircleArc(rootTranslation, input.getAimDirection(), radialSimulationStaticData.getAttackCircle().getInnerRadius(), aimArchAngle, 1, 1.f);


	const glm::vec4 defaultForward4(DAttackRadialSequence::defaultForward(), 0.f);
	const glm::vec3 currentDirection = glm::vec3(rootTransform * defaultForward4);
	input.getLoggingFunctor().logVec3("dAttackRadialVisualization currentDirection: ", currentDirection);

	const glm::vec3 moveDirection = glm::normalize(glm::vec3(input.getMoveDirectionWorld()));
	const glm::vec3 moveDirectionXY = glm::normalize(glm::vec3(moveDirection.x, moveDirection.y, 0.f));

	const glm::vec3 aimDirection = glm::normalize(input.getAimDirection());
	const glm::vec3 aimDirectionXY = glm::normalize(glm::vec3(input.getAimDirection().x, input.getAimDirection().y, 0.f));
	const glm::vec4 aimDirectionXY4 = glm::vec4(aimDirectionXY, 0.f);

	const float angle = glm::acos(glm::dot(aimDirection, moveDirectionXY));
	const float signedAngle = glm::sign(glm::cross(aimDirectionXY, moveDirectionXY).z) * angle;

	const glm::vec3 attackAxis = [angle, &moveDirectionXY, &aimDirectionXY]() {
		if (angle < 0.01f)
			return glm::vec3(0.f, 0.f, 1.f);
		else
			return glm::normalize(glm::cross(moveDirectionXY, aimDirectionXY));
		}();
	const glm::vec3 attackSegmentRoot = glm::vec3(rootTransform[3]) + glm::vec3(0.f, 0.f, 5.f);

	//Visualize angle intervals where the different attack directions will occur
	//dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, attackSegmentRoot, aimDirectionXY, attackAxis, attackDirectionAngleIncrement * 2, radialSimulationStaticData.getAttackCircle().getInnerRadius() * 0.5f, 0.f, 0, 1.f);
	//glm::mat4 leftMiddlePointAttackRotation = glm::rotate(glm::mat4(1.f), sideAngle, attackAxis);
	//glm::vec3 leftMiddlePointDirection = glm::vec3(leftMiddlePointAttackRotation * aimDirectionXY4);
	//glm::mat4 rightMiddlePointAttackRotation = glm::rotate(glm::mat4(1.f), sideAngle, -attackAxis);
	//glm::vec3 rightMiddlePointDirection = glm::vec3(rightMiddlePointAttackRotation * aimDirectionXY4);
	//dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, attackSegmentRoot, leftMiddlePointDirection, attackAxis, sideAngle, radialSimulationStaticData.getAttackCircle().getInnerRadius() * 0.5f, 1.f, 0, 1.f);
	//dAttackVisualizationUtils::drawSegmentOutline(rendererFunctor, attackSegmentRoot, rightMiddlePointDirection, -attackAxis, sideAngle, radialSimulationStaticData.getAttackCircle().getInnerRadius() * 0.5f, 1.f, 0, 1.f);

	
	if (glm::length(input.getMoveDirection()) < 0.00001f || (signedAngle < attackDirectionAngleIncrement && signedAngle > -attackDirectionAngleIncrement))
	{
		const float forwardIndicatorAngle = glm::pi<float>() / 8.f;
		dAttackVisualizationUtils::drawSegmentSolid(rendererFunctor, attackSegmentRoot, aimDirectionXY, attackAxis, attackDirectionAngleIncrement * 0.5f, radialSimulationStaticData.getAttackCircle().getOuterRadius(), radialSimulationStaticData.getAttackCircle().getInnerRadius(), 0, 1.f);
	}
	else
	{
		const float sideIndicatorAngle = glm::pi<float>() / 8.f;
		glm::mat4 sideIndicatorRotation = glm::rotate(glm::mat4(1.f), sideIndicatorAngle * 0.5f, -attackAxis);
		glm::vec3 sideIndicatorDirection = glm::vec3(sideIndicatorRotation * aimDirectionXY4);
		dAttackVisualizationUtils::drawSegmentSolid(rendererFunctor, attackSegmentRoot, sideIndicatorDirection, attackAxis, sideIndicatorAngle, radialSimulationStaticData.getAttackCircle().getOuterRadius(), radialSimulationStaticData.getAttackCircle().getInnerRadius(), 0, 1.f);
	}

	if (glm::length(input.getMoveDirection()) > 0.0001f)
		input.getRendererFunctorImpl().drawLine(rootTranslation - moveDirection*30.f, rootTranslation + moveDirection * radialSimulationStaticData.getAttackCircle().getOuterRadius(), 3, 2.f);

	// render 

}

}

#pragma optimize( "", on )



 
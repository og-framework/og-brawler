#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DAttackRadialSimulation.h"
#include "OGBrawler/DAttackSequenceId.h"
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
	// activeAttackSequence may carry a reserved sentinel (e.g. kHadoukenSequenceSentinel)
	// for the 1+ ticks between the machine writing it and the Attacking→Idle exit clearing
	// it. The aim viz does not consume the sequence, but the bare index would still read
	// ~4 billion entries out of bounds, so gate it behind isRealAttackSequence.
	if (isRealAttackSequence(simulationInitialConditions.activeAttackSequence))
	{
		const DAttackRadialSequence& sequence = radialSimulationStaticData.getAttackSequences()[simulationInitialConditions.activeAttackSequence];
		(void)sequence; // currently unused in the aim viz; bound for parity with the radial viz sites
	}
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
	// Green outer arc visualizing the forward-strike window. UE DrawDebugCircleArc sweeps
	// from -AngleWidth to +AngleWidth (it's a half-angle — see DrawDebugHelpers.cpp:497),
	// so π/16 gives a total span of π/8.
	rendererFunctor.drawCircleArc(rootTranslation, input.getAimDirection(), radialSimulationStaticData.getAttackCircle().getOuterRadius(), glm::pi<float>() / 32.f, 1, 1.f);

	// Inner-radius arc: asymmetric — keep the original tiny tick on the right side of aim
	// (at -aimArchAngle CW), but extend the left side a full quarter circle (π/2 CCW) so
	// the player always has a visible "left of me" reference relative to where they aim.
	// "Left" here matches integrate3's labeling: cross(aim, dir).z > 0 → left strike,
	// which is CCW from aim around +Z, hence the positive centerOffset.
	{
		const float innerArcLeftExtent   = glm::pi<float>() / 2.f;
		const float innerArcRightExtent  = aimArchAngle;
		const float innerArcHalfAngle    = (innerArcLeftExtent + innerArcRightExtent) * 0.5f;
		const float innerArcCenterOffset = (innerArcLeftExtent - innerArcRightExtent) * 0.5f; // positive → CCW
		const glm::mat4 innerArcCenterRot = glm::rotate(glm::mat4(1.f), innerArcCenterOffset, glm::vec3(0.f, 0.f, 1.f));
		const glm::vec3 innerArcCenter = glm::vec3(innerArcCenterRot * glm::vec4(input.getAimDirection(), 0.f));
		rendererFunctor.drawCircleArc(rootTranslation, innerArcCenter, radialSimulationStaticData.getAttackCircle().getInnerRadius(), innerArcHalfAngle, 1, 1.f);
	}


	const glm::vec4 defaultForward4(DAttackRadialSequence::defaultForward(), 0.f);
	const glm::vec3 currentDirection = glm::vec3(rootTransform * defaultForward4);
	input.getLoggingFunctor().logVec3("dAttackRadialVisualization currentDirection: ", currentDirection);

	const glm::vec3 moveDirection = glm::normalize(glm::vec3(input.getMoveDirectionWorld()));
	const glm::vec3 moveDirectionXY = glm::normalize(glm::vec3(moveDirection.x, moveDirection.y, 0.f));

	const glm::vec3 aimDirection = glm::normalize(input.getAimDirection());
	const glm::vec3 aimDirectionXY = glm::normalize(glm::vec3(input.getAimDirection().x, input.getAimDirection().y, 0.f));
	const glm::vec4 aimDirectionXY4 = glm::vec4(aimDirectionXY, 0.f);

	// Both terms must be XY-projected; mirrors the fix in dAttackMachineSimulation::integrate3.
	// Using the 3D aimDirection inflates the angle by aim's downward z and causes the viz
	// to flicker between forward and side indicators when XY directions are aligned.
	const float dotXY = glm::clamp(glm::dot(aimDirectionXY, moveDirectionXY), -1.f, 1.f);
	const float angle = glm::acos(dotXY);
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

	
	// Red lined attack-direction indicator (thicker than the green reference arcs above).
	//   Forward strike  → π/2 vertical arc from the character's up vector down to the aim
	//                     direction (XY-projected). Drawn manually with N short line
	//                     segments because drawCircleArc orients its plane via world up
	//                     (see DrawDebugHelpers.cpp:FindBestAxisVectors usage) and so
	//                     can't reliably render a vertical arc.
	//   Left strike     → π/3 horizontal arc on the inner radius, starting at aim and
	//                     extending CCW around +Z (the player's left, matching
	//                     integrate3's positive signedAngle → seq 1 labeling).
	//   Right strike    → π/3 horizontal arc, starting at aim and extending CW (player's
	//                     right, matching integrate3's negative signedAngle → seq 0).
	const float kIndicatorRadius    = radialSimulationStaticData.getAttackCircle().getInnerRadius();
	const float kIndicatorThickness = 4.f; // green reference arcs use 1.f
	const unsigned int kIndicatorColor = 0; // 0 = red per DAttackCircularVisualizationUImpl::idToColor

	const bool forwardCase =
		glm::length(input.getMoveDirection()) < 0.00001f
		|| (signedAngle < attackDirectionAngleIncrement && signedAngle > -attackDirectionAngleIncrement);

	if (forwardCase)
	{
		// Forward indicator: a forward L plus a diagonal "ceiling" line forming a closed
		// wedge over the character.
		//   Horizontal: character position → forwardEnd (inner radius along aim).
		//   Diagonal:   top (inner radius above character) → aim tip (outer radius along
		//               aim, matching the green aim line's forward end).
		//   Vertical:   forwardEnd → the point where the vertical line through forwardEnd
		//               meets the diagonal. Shortened from the previous full-innerRadius
		//               height so the three lines form a closed shape.
		const glm::vec3 up(0.f, 0.f, 1.f);
		const glm::vec3 forwardEnd = rootTranslation + aimDirectionXY * kIndicatorRadius;
		const glm::vec3 top        = rootTranslation + up * kIndicatorRadius;
		const glm::vec3 aimTip     = rootTranslation + input.getAimDirection() * radialSimulationStaticData.getAttackCircle().getOuterRadius();

		// Project onto XY to solve for s ∈ [0, 1] along the diagonal where the vertical
		// line through forwardEnd intersects: forwardEnd.xy = top.xy + s*(aimTip.xy - top.xy).
		const glm::vec3 diagonal  = aimTip - top;
		const glm::vec3 toForward = forwardEnd - top;
		const glm::vec2 diagonalXY(diagonal.x, diagonal.y);
		const glm::vec2 toForwardXY(toForward.x, toForward.y);
		const float diagonalXYLenSq = glm::dot(diagonalXY, diagonalXY);
		const float s = (diagonalXYLenSq > KINDA_SMALL_NUMBER)
			? glm::dot(toForwardXY, diagonalXY) / diagonalXYLenSq
			: 0.f;
		const glm::vec3 verticalEnd = top + s * diagonal;

		rendererFunctor.drawLine(rootTranslation, forwardEnd,  kIndicatorColor, kIndicatorThickness);
		rendererFunctor.drawLine(forwardEnd,      verticalEnd, kIndicatorColor, kIndicatorThickness);
		rendererFunctor.drawLine(top,             aimTip,      kIndicatorColor, kIndicatorThickness);
	}
	else
	{
		// Side indicator: a three-point path through (aim tip → threshold point → back
		// intersection). The threshold angle (= attackDirectionAngleIncrement = π/6 from
		// aim — defined here and matched in dAttackMachineSimulation::integrate3 where
		// it controls the forward-vs-side branch in the if-cascade) is the angle that
		// determines whether the player's move direction lands a forward or side strike.
		//   1. Point A — aim tip (rootTranslation + aimDirection*outerRadius), same as
		//      the green aim line's forward end.
		//   2. Point B — threshold point on the inner circle (rootTranslation +
		//      thresholdDir*innerRadius).
		//   3. Point C — extending in the (A → B) XY direction past B until the line
		//      intersects the inner circle a second time. The inner circle lies in the
		//      XY plane at character z, so the next-intersection math is purely 2D:
		//      ray P(t) = B + t*d with d = normalize(B.xy - A.xy); substituting into
		//      |P - center|² = r² (with |B - center|² = r²) gives
		//         t² + 2tr·dot(thresholdDir, d) = 0
		//      and the non-zero root is t = -2r·dot(thresholdDir, d).
		//   Left strike  → threshold CCW +π/6 from aim (positive +Z rotation, matches
		//                  integrate3's positive signedAngle → seq 1 labeling).
		//   Right strike → threshold CW -π/6 from aim.
		const bool isLeft = signedAngle > attackDirectionAngleIncrement;
		const float thresholdOffset = isLeft ? attackDirectionAngleIncrement : -attackDirectionAngleIncrement;
		const glm::mat4 thresholdRot = glm::rotate(glm::mat4(1.f), thresholdOffset, glm::vec3(0.f, 0.f, 1.f));
		const glm::vec3 thresholdDir = glm::vec3(thresholdRot * glm::vec4(aimDirectionXY, 0.f));

		const glm::vec3 aimTip     = rootTranslation + input.getAimDirection() * radialSimulationStaticData.getAttackCircle().getOuterRadius();
		const glm::vec3 frontPoint = rootTranslation + thresholdDir * kIndicatorRadius;

		const glm::vec2 lineDirXY = glm::normalize(glm::vec2(frontPoint.x - aimTip.x, frontPoint.y - aimTip.y));
		const float tExit         = -2.f * kIndicatorRadius * glm::dot(glm::vec2(thresholdDir.x, thresholdDir.y), lineDirXY);
		const glm::vec3 backPoint(frontPoint.x + tExit * lineDirXY.x, frontPoint.y + tExit * lineDirXY.y, rootTranslation.z);

		rendererFunctor.drawLine(aimTip,          frontPoint, kIndicatorColor, kIndicatorThickness);
		rendererFunctor.drawLine(frontPoint,      backPoint,  kIndicatorColor, kIndicatorThickness);
		rendererFunctor.drawLine(rootTranslation, backPoint,  kIndicatorColor, kIndicatorThickness);
	}

	if (glm::length(input.getMoveDirection()) > 0.0001f)
		input.getRendererFunctorImpl().drawLine(rootTranslation - moveDirection*30.f, rootTranslation + moveDirection * radialSimulationStaticData.getAttackCircle().getInnerRadius(), 3, 2.f);

	// render 

}

}

#pragma optimize( "", on )



 
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

	// moveDirectionWorld normalized — retained for the yellow move-direction line below.
	// The forward/left/right classification, the inner-circle origin point, and the
	// predicted attack-sequence id are now produced by the shared pure helper
	// dAttackVisualizationUtils::computeAttackIndicatorGeometry (see below), so the aim
	// viz and the new block-prediction viz cannot drift on either value.
	const glm::vec3 moveDirection = glm::normalize(glm::vec3(input.getMoveDirectionWorld()));


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

	// Shared classifier + inner-circle origin point. The block-prediction viz consumes the
	// same helper (and its predictedSequenceIdFromIdle) so the two indicators can't diverge.
	const dAttackVisualizationUtils::AttackIndicatorGeometry attackGeometry =
		dAttackVisualizationUtils::computeAttackIndicatorGeometry(
			input.getAimDirection(),
			input.getMoveDirection(),
			input.getMoveDirectionWorld(),
			rootTranslation,
			kIndicatorRadius);

	// Weapon-shape attack indicator (redesign — replaces the previous 3-line L / 3-point
	// path). The shape is stylised to read as a weapon held in a ready stance:
	//   - Handle: a straight LINE (drawLine primitive, same thickness as the previous
	//             indicator). Length varies by kind:
	//               - Side (left/right): handle extends from the "back" intersection
	//                 of the weapon-line with the inner circle to the "front"
	//                 intersection (i.e. the handle IS the inner-circle chord of the
	//                 weapon line). The blade then continues from the front intersection
	//                 outward to the aim tip.
	//               - Forward: handle extends 50cm from a point above the character's head
	//                 along the weapon axis; the "inner-circle chord" concept doesn't
	//                 apply to a diagonal top→aimTip line, so a fixed length is used.
	//   - Blade:  a rectangular shaft + equilateral triangular tip, from where the handle
	//             ends to the aim tip. Shaft orientation differs by kind:
	//               - Side (left/right): shaft lies flat in the XY plane (horizontal swing).
	//               - Forward:           shaft stands vertical, in the plane containing
	//                                    the weapon axis and +Z (upright thrust).
	//
	// The whole weapon indicator is HIDDEN while the radial sim is actively running an
	// attack sequence — showing "where the next attack would go" while an actual attack
	// is in-progress is confusing, and the ongoing swing viz (DAttackRadialVisualization)
	// already conveys the active attack.
	constexpr float kShaftWidth   = 15.f;   // shaft perpendicular width (blade)
	constexpr float kTriangleSide = 15.f;   // triangular tip side length (equilateral)

	if (!isRealAttackSequence(simulationInitialConditions.activeAttackSequence))
	{
		const glm::vec3 aimTip = rootTranslation + input.getAimDirection() * radialSimulationStaticData.getAttackCircle().getOuterRadius();

		// Compute the "reference handle length" — the chord of the inner circle traced by
		// a canonical (+π/6 CCW from aim) threshold line to aimTip. This is the L/R
		// handle length; the forward case reuses the same value so both indicators have
		// visually identical handle proportions.
		const glm::vec3 aimDirectionXY = glm::normalize(
			glm::vec3(input.getAimDirection().x, input.getAimDirection().y, 0.f));
		const float refThresholdAngle = glm::pi<float>() / 6.f;
		const glm::mat4 refRot = glm::rotate(glm::mat4(1.f), refThresholdAngle, glm::vec3(0.f, 0.f, 1.f));
		const glm::vec3 refThresholdDir = glm::vec3(refRot * glm::vec4(aimDirectionXY, 0.f));
		const glm::vec3 refFrontPoint = rootTranslation + refThresholdDir * kIndicatorRadius;
		const glm::vec2 refLineDirXY = glm::normalize(glm::vec2(refFrontPoint.x - aimTip.x, refFrontPoint.y - aimTip.y));
		const float kSideHandleLength = std::abs(-2.f * kIndicatorRadius
			* glm::dot(glm::vec2(refThresholdDir.x, refThresholdDir.y), refLineDirXY));

		if (attackGeometry.kind == dAttackVisualizationUtils::AttackDirectionKind::Forward)
		{
			// Forward: handle starts above the character's head (top = rootTranslation + up ·
			// innerRadius) and extends kSideHandleLength (matches L/R handle length) toward
			// the aim tip; blade continues to aim tip. Vertical shaft.
			const glm::vec3 up(0.f, 0.f, 1.f);
			const glm::vec3 top       = rootTranslation + up * kIndicatorRadius;
			const glm::vec3 weaponDir = glm::normalize(aimTip - top);
			const glm::vec3 handleEnd = top + weaponDir * kSideHandleLength;

			// Vertical shaft: perp is in the plane containing weapon axis and +Z.
			const glm::vec3 horizontalPerp = glm::normalize(glm::cross(weaponDir, glm::vec3(0.f, 0.f, 1.f)));
			const glm::vec3 shaftPerp     = glm::normalize(glm::cross(weaponDir, horizontalPerp));
			dAttackVisualizationUtils::drawWeapon(rendererFunctor,
				top, handleEnd, aimTip, shaftPerp,
				kIndicatorColor, kIndicatorThickness, kShaftWidth, kTriangleSide);
		}
		else
		{
			// Side (left or right): handle is the chord of the inner circle traced by the
			// weapon line. It ENTERS the inner circle at backPoint (behind the character)
			// and EXITS at frontPoint (the threshold point at ±π/6 from aim). Blade
			// continues from frontPoint outward to the aim tip.
			const glm::vec3 frontPoint   = attackGeometry.originOnInnerCircle;
			const glm::vec3 thresholdDir = attackGeometry.attackDirectionXY;
			const glm::vec2 lineDirXY = glm::normalize(glm::vec2(frontPoint.x - aimTip.x, frontPoint.y - aimTip.y));
			const float tExit         = -2.f * kIndicatorRadius * glm::dot(glm::vec2(thresholdDir.x, thresholdDir.y), lineDirXY);
			const glm::vec3 backPoint(frontPoint.x + tExit * lineDirXY.x,
			                         frontPoint.y + tExit * lineDirXY.y,
			                         rootTranslation.z);

			// Horizontal shaft: perp lies in XY plane, perpendicular to weapon axis.
			const glm::vec3 weaponDir = glm::normalize(aimTip - backPoint);
			const glm::vec3 shaftPerp = glm::normalize(glm::cross(weaponDir, glm::vec3(0.f, 0.f, 1.f)));
			dAttackVisualizationUtils::drawWeapon(rendererFunctor,
				backPoint, frontPoint, aimTip, shaftPerp,
				kIndicatorColor, kIndicatorThickness, kShaftWidth, kTriangleSide);
		}
	}

	if (glm::length(input.getMoveDirection()) > 0.0001f)
		input.getRendererFunctorImpl().drawLine(rootTranslation - moveDirection*15.f, rootTranslation + moveDirection * radialSimulationStaticData.getAttackCircle().getInnerRadius() * 0.4f, 3, 1.f);

	// render 

}

}

#pragma optimize( "", on )



 
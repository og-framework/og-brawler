#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <optional>
#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "DAttackRadialSimulation.h"
#include "OGBrawler/DAttackSequenceId.h"
#include "DAttackCircle.h"
#include "OGSimulation/DMathUtil.h"
#include "DAttackVisualizationUtils.h"
#include "DAttackGuardSimulation.h"
#include "OGBrawler/DAttackDirectionClassifier.h"
#include "OGSimulation/SimulationFrame.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF


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


namespace detail
{

// [og-ground-aligned-attacks task 11] The aim indicator's draw calls about rootTranslation:
// today's expressions, moved out of visualize unchanged but for their inputs. aimDirection and
// moveDirectionWorld are the world inputs (a world-aligned indicator) or their local headings (a
// ground-frame one, drawn through dAttackVisualizationUtils::GroundFrameRenderer); attackSequenceId
// is dAttackDirection::classify's id on the WORLD inputs either way.
template <typename IndicatorRendererType>
void drawAimIndicator(IndicatorRendererType rendererFunctor,
	const glm::vec3& aimDirection,
	const glm::vec2& moveStick,
	const glm::vec3& moveDirectionWorld,
	unsigned int attackSequenceId,
	bool attackInProgress,
	const glm::vec3& rootTranslation,
	const dAttackRadialSimulation::StaticData& radialSimulationStaticData)
{
	const float aimArchAngle = 0.1f;
	rendererFunctor.drawLine(rootTranslation - aimDirection * 0.3f, rootTranslation + aimDirection * radialSimulationStaticData.getAttackCircle().getOuterRadius(), 1, 0.01f);
	// Green outer arc visualizing the forward-strike window. UE DrawDebugCircleArc sweeps
	// from -AngleWidth to +AngleWidth (it's a half-angle — see DrawDebugHelpers.cpp:497),
	// so π/16 gives a total span of π/8.
	rendererFunctor.drawCircleArc(rootTranslation, aimDirection, radialSimulationStaticData.getAttackCircle().getOuterRadius(), glm::pi<float>() / 32.f, 1, 0.01f);

	// Inner-radius arc: a quarter circle on the character's screen-right of aim, overhanging
	// both ends by aimArchAngle (spans aimArchAngle .. -(π/2 + aimArchAngle) about
	// simFrame::kTurnAxis = +Y). A negative turn about +Y is clockwise seen from above, so it
	// maps forward to right and that side is screen-right. The classifier
	// (dAttackDirection::classify) labels the same side "left": movement to that side gives
	// its negative signed angle, which it maps to the left swing.
	{
		const float innerArcLeftExtent   = glm::pi<float>() / 2.f + aimArchAngle;
		const float innerArcRightExtent  = aimArchAngle;
		const float innerArcHalfAngle    = (innerArcLeftExtent + innerArcRightExtent) * 0.5f;
		const float innerArcCenterOffset = (innerArcRightExtent - innerArcLeftExtent) * 0.5f; // negative → clockwise from above
		constexpr glm::vec3 turnAxis = simFrame::kTurnAxis;
		const glm::mat4 innerArcCenterRot = glm::rotate(glm::mat4(1.f), innerArcCenterOffset, turnAxis);
		const glm::vec3 innerArcCenter = glm::vec3(innerArcCenterRot * glm::vec4(aimDirection, 0.f));
		rendererFunctor.drawCircleArc(rootTranslation, innerArcCenter, radialSimulationStaticData.getAttackCircle().getInnerRadius(), innerArcHalfAngle, 1, 0.01f);
	}

	if (const std::optional<glm::vec3> rightMarkerEnd = dAttackVisualizationUtils::aimRightMarkerEnd(
			rootTranslation, aimDirection, radialSimulationStaticData.getAttackCircle().getInnerRadius()))
	{
		rendererFunctor.drawLine(rootTranslation, *rightMarkerEnd, 1, 0.01f);
	}


	// moveDirectionWorld normalized — retained for the yellow move-direction line below.
	// The forward/left/right classification, the inner-circle origin point, and the
	// predicted attack-sequence id are now produced by dAttackDirection::classify (in visualize)
	// and the shared pure helper dAttackVisualizationUtils::attackIndicatorGeometryFor (see
	// below), so the aim viz and the new block-prediction viz cannot drift on either value.
	const glm::vec3 moveDirection = glm::normalize(glm::vec3(moveDirectionWorld));


	// Red lined attack-direction indicator (thicker than the green reference arcs above).
	//   Forward strike  → π/2 vertical arc from the character's up vector down to the aim
	//                     direction (ground-plane projected). Drawn manually with N short
	//                     segments because drawCircleArc orients its plane via world up
	//                     (see DrawDebugHelpers.cpp:FindBestAxisVectors usage) and so
	//                     can't reliably render a vertical arc.
	//   Left strike     → a horizontal weapon whose blade leaves the inner circle at the
	//                     threshold point π/6 from aim on the aim's RIGHT (clockwise seen
	//                     from above), matching the classifier's negative signedAngle → seq 1.
	//   Right strike    → the mirror image on the aim's left (counter-clockwise), matching
	//                     the classifier's positive signedAngle → seq 0.
	const float kIndicatorRadius    = radialSimulationStaticData.getAttackCircle().getInnerRadius();
	const float kIndicatorThickness = 0.04f; // green reference arcs use 1.f
	const unsigned int kIndicatorColor = 0; // 0 = red per DAttackCircularVisualizationUImpl::idToColor

	// Shared classifier + inner-circle origin point. The block-prediction viz consumes the
	// same helper (and its predictedSequenceIdFromIdle) so the two indicators can't diverge.
	const dAttackVisualizationUtils::AttackIndicatorGeometry attackGeometry =
		dAttackVisualizationUtils::attackIndicatorGeometryFor(
			attackSequenceId,
			aimDirection,
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
	//               - Forward: handle extends the side handle's length from a point above
	//                 the character's head along the weapon axis; the "inner-circle chord"
	//                 concept doesn't apply to a diagonal top→aimTip line, so it is reused.
	//   - Blade:  a rectangular shaft + equilateral triangular tip, from where the handle
	//             ends to the aim tip. Shaft orientation differs by kind:
	//               - Side (left/right): shaft lies flat in the ground plane (horizontal swing).
	//               - Forward:           shaft stands vertical, in the plane containing
	//                                    the weapon axis and up (upright thrust).
	//
	// The whole weapon indicator is HIDDEN while the radial sim is actively running an
	// attack sequence — showing "where the next attack would go" while an actual attack
	// is in-progress is confusing, and the ongoing swing viz (DAttackRadialVisualization)
	// already conveys the active attack.
	constexpr float kShaftWidth   = 0.15f;   // shaft perpendicular width (blade)
	constexpr float kTriangleSide = 0.15f;   // triangular tip side length (equilateral)

	if (!attackInProgress)
	{
		const glm::vec3 aimTip = rootTranslation + aimDirection * radialSimulationStaticData.getAttackCircle().getOuterRadius();

		// Compute the "reference handle length" — the chord of the inner circle traced by
		// a canonical (-π/6 about simFrame::kTurnAxis, i.e. π/6 clockwise from aim seen from
		// above) threshold line to aimTip. This is the L/R
		// handle length; the forward case reuses the same value so both indicators have
		// visually identical handle proportions.
		const glm::vec3 aimDirectionXY = glm::normalize(
			simFrame::flatten(aimDirection));
		const float refThresholdAngle = -glm::pi<float>() / 6.f;
		constexpr glm::vec3 turnAxis = simFrame::kTurnAxis;
		const glm::mat4 refRot = glm::rotate(glm::mat4(1.f), refThresholdAngle, turnAxis);
		const glm::vec3 refThresholdDir = glm::vec3(refRot * glm::vec4(aimDirectionXY, 0.f));
		const glm::vec3 refFrontPoint = rootTranslation + refThresholdDir * kIndicatorRadius;
		const glm::vec2 refLineDirXY = glm::normalize(simFrame::planar(refFrontPoint - aimTip));
		const float kSideHandleLength = std::abs(-2.f * kIndicatorRadius
			* glm::dot(simFrame::planar(refThresholdDir), refLineDirXY));

		if (attackGeometry.kind == dAttackVisualizationUtils::AttackDirectionKind::Forward)
		{
			// Forward: handle starts above the character's head (top = rootTranslation + up ·
			// innerRadius) and extends kSideHandleLength (matches L/R handle length) toward
			// the aim tip; blade continues to aim tip. Vertical shaft.
			constexpr glm::vec3 up = simFrame::kUp;
			const glm::vec3 top       = rootTranslation + up * kIndicatorRadius;
			const glm::vec3 weaponDir = glm::normalize(aimTip - top);
			const glm::vec3 handleEnd = top + weaponDir * kSideHandleLength;

			// Vertical shaft: perp is in the plane containing weapon axis and up.
			// The two perps are 3-D drawing directions, not chart values, so they carry a fixed
			// sign: the -1 that simFrame::kChartOrientation had before og-sim-metres-yup task 26
			// made it +1. The sign of a shaftPerp picks the winding of drawWeapon's triangles.
			const glm::vec3 horizontalPerp = -glm::normalize(glm::cross(weaponDir, up));
			const glm::vec3 shaftPerp     = -glm::normalize(glm::cross(weaponDir, horizontalPerp));
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
			const glm::vec2 lineDirXY = glm::normalize(simFrame::planar(frontPoint - aimTip));
			const float tExit         = -2.f * kIndicatorRadius * glm::dot(simFrame::planar(thresholdDir), lineDirXY);
			const glm::vec3 backPoint = simFrame::compose(simFrame::planar(frontPoint) + tExit * lineDirXY,
			                         simFrame::height(rootTranslation));

			// Horizontal shaft: perp lies in the ground plane, perpendicular to weapon axis.
			// A 3-D drawing direction with the fixed sign of the forward case above.
			const glm::vec3 weaponDir = glm::normalize(aimTip - backPoint);
			constexpr glm::vec3 up = simFrame::kUp;
			const glm::vec3 shaftPerp = -glm::normalize(glm::cross(weaponDir, up));
			dAttackVisualizationUtils::drawWeapon(rendererFunctor,
				backPoint, frontPoint, aimTip, shaftPerp,
				kIndicatorColor, kIndicatorThickness, kShaftWidth, kTriangleSide);
		}
	}

	if (glm::length(moveStick) > 0.0001f)
		rendererFunctor.drawLine(rootTranslation - moveDirection*0.15f, rootTranslation + moveDirection * radialSimulationStaticData.getAttackCircle().getInnerRadius() * 0.4f, 3, 0.01f);

}

}

template <typename RendererFunctorType, typename LoggingFunctorType>
void visualize(const Input<RendererFunctorType, LoggingFunctorType>& input,
	const dAttackRadialSimulation::State& simulationState,
	const dAttackRadialSimulation::InitialConditions& simulationInitialConditions,
	const dAttackRadialSimulation::DerivedState& radialSimulationDerivedState,
	const dAttackRadialSimulation::StaticData& radialSimulationStaticData,
	const dAttackGuardSimulation::State& guardState,
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


	const glm::vec4 defaultForward4(DAttackRadialSequence::defaultForward(), 0.f);
	const glm::vec3 currentDirection = glm::vec3(rootTransform * defaultForward4);
	input.getLoggingFunctor().logVec3("dAttackRadialVisualization currentDirection: ", currentDirection);

	// [og-ground-aligned-attacks task 11] The indicator lies in the plane normal to the guard body's
	// up while idle and to the swing frame's up during a real attack sequence (the user's OQ9
	// ruling); a flat up draws today's expressions with the world inputs. The weapon body's own
	// idle pose stays world-aligned, so it cannot give the up.
	const glm::vec3 indicatorUp = dAttackVisualizationUtils::aimIndicatorUp(simulationInitialConditions, guardState);
	const bool worldAligned = dAttackVisualizationUtils::drawsWorldAligned(indicatorUp);
	const unsigned int attackSequenceId =
		dAttackDirection::classify(input.getAimDirection(), input.getMoveDirectionWorld(), input.getMoveDirection());
	const glm::vec3 indicatorAim = worldAligned
		? input.getAimDirection()
		: dAttackVisualizationUtils::localHeading(indicatorUp, input.getAimDirection());
	const glm::vec3 indicatorMove = worldAligned
		? input.getMoveDirectionWorld()
		: dAttackVisualizationUtils::localHeading(indicatorUp, input.getMoveDirectionWorld());
	const bool attackInProgress = isRealAttackSequence(simulationInitialConditions.activeAttackSequence);

	dAttackVisualizationUtils::drawInIndicatorFrame(rendererFunctor, rootTranslation, indicatorUp,
		[&](auto indicatorRenderer) {
			detail::drawAimIndicator(indicatorRenderer, indicatorAim, input.getMoveDirection(), indicatorMove,
				attackSequenceId, attackInProgress, rootTranslation, radialSimulationStaticData);
		});
}

}

OGSIM_OPTIMIZE_ON



 
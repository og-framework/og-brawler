#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <optional>
#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext/scalar_constants.hpp"
#include "DAttackRadialSimulation.h"
#include "DAttackMachineSimulation.h"
#include "OGBrawler/DAttackDirectionClassifier.h"
#include "OGBrawler/DAttackSequenceId.h"
#include "DAttackGuardSimulation.h"
#include "DAttackCircle.h"
#include "OGSimulation/OGAssert.h"
#include "OGSimulation/SimulationFrame.h"
#include "OGSimulation/SurfaceFrame.h"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF


namespace dAttackVisualizationUtils
{	

// Shared amber palette id - the ONE hue for "an attack is going this way", used by
// BOTH the attack-direction indicator wedge (DAttackBlockPredictionVisualization.h)
// and the WindUp/WindDown swing segment (DAttackRadialVisualization.h). The RGB lives
// in DAttackCircularVisualizationUImpl::idToColor, case 14 = FColor(249, 205, 9) - the
// channel midpoint of the old Orange (243,156,18) and Yellow (255,255,0).
// `inline` is required: a namespace-scope constexpr alone has internal linkage.
inline constexpr unsigned int kAttackDirectionColorId = 14u;

inline constexpr float kAimRightMarkerOverhang = 0.1f;
inline constexpr float kAimRightMarkerMinAimXYLength = 1e-4f;

// End of the aim viz's horizontal marker line from the root: the flattened aim turned
// -π/2 about `simFrame::kTurnAxis` (+Y: clockwise seen from above, to the right, the inner arc's
// side), innerRadius + 0.1 m long.
// nullopt when aim has no horizontal part (straight up/down).
inline std::optional<glm::vec3> aimRightMarkerEnd(const glm::vec3& rootTranslation, const glm::vec3& aimDirection, float innerRadius)
{
	const glm::vec3 aimXY = simFrame::flatten(aimDirection);
	const float aimXYLength = glm::length(aimXY);
	if (!(aimXYLength > kAimRightMarkerMinAimXYLength))
	{
		return std::nullopt;
	}
	constexpr glm::vec3 turnAxis = simFrame::kTurnAxis;
	const glm::mat4 quarterTurn = glm::rotate(glm::mat4(1.f), -glm::pi<float>() / 2.f, turnAxis);
	const glm::vec3 rightDirXY = glm::vec3(quarterTurn * glm::vec4(aimXY / aimXYLength, 0.f));
	return rootTranslation + rightDirXY * (innerRadius + kAimRightMarkerOverhang);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename RendererFunctorType>
void drawSegmentOutline(RendererFunctorType renderFunctor, glm::vec3 worldOrigin, glm::vec3 worldArcDirection, glm::vec3 worldArcNormal, float arcAngle, float outerRadius, float innerRadius, unsigned int colorId, float thickness)
{
	renderFunctor.drawCircleArc(worldOrigin, worldArcDirection, outerRadius, arcAngle * 0.5f, colorId, thickness);
	renderFunctor.drawCircleArc(worldOrigin, worldArcDirection, innerRadius, arcAngle * 0.5f, colorId, thickness);
	
	glm::vec4 worldArcDirection4(worldArcDirection, 0.f);

	glm::mat4 innerRotaition = glm::rotate(glm::mat4(1.f), arcAngle*0.5f, -worldArcNormal);
	glm::vec3 innerCollisionDirection = glm::vec3(innerRotaition * worldArcDirection4);
	renderFunctor.drawLine(worldOrigin + innerCollisionDirection * innerRadius, worldOrigin + innerCollisionDirection * outerRadius, colorId, thickness);

	glm::mat4 outerRotation = glm::rotate(glm::mat4(1.f), arcAngle*0.5f, worldArcNormal);
	glm::vec3 outerCollisionDirection = glm::vec3(outerRotation * worldArcDirection4);
	renderFunctor.drawLine(worldOrigin + outerCollisionDirection * innerRadius, worldOrigin + outerCollisionDirection * outerRadius, colorId, thickness);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// `alpha` is forwarded per-call to the renderer's drawMesh (default 150 preserves the
// historical hardcoded translucency for all pre-existing call sites).
template <typename RendererFunctorType>
void drawSegmentSolid(RendererFunctorType renderFunctor, glm::vec3 worldOrigin, glm::vec3 worldArcDirection, glm::vec3 worldArcNormal, float arcAngle, float outerRadius, float innerRadius, unsigned int colorId, float thickness, unsigned int alpha = 150)
{
    arcAngle = std::abs(arcAngle);
    OG_CHECK(arcAngle != 0.f, "arcAngle must be non 0");
    const float stepSize = 0.2f;
    const glm::vec4 worldArcDirection4(worldArcDirection, 0.f);
    const float halfArcAngle = arcAngle * 0.5f;

    std::vector<glm::vec3> vertices;
    std::vector<unsigned int> indices;
    int innerIndex = vertices.size() - 1;

    //angle = -halfArcAngle
    glm::mat4 innerRotation = glm::rotate(glm::mat4(1.f), halfArcAngle, -worldArcNormal);
    glm::vec3 innerCollisionDirection = glm::vec3(innerRotation * worldArcDirection4);
    vertices.push_back(worldOrigin + innerCollisionDirection * innerRadius);
    vertices.push_back(worldOrigin + innerCollisionDirection * outerRadius);

    // Add vertices in between
    float angleStep = stepSize;
    for (float angle = halfArcAngle - angleStep; angle >= 0; angle -= angleStep)
    {
        glm::mat4 rotation = glm::rotate(glm::mat4(1.f), angle, -worldArcNormal);
        glm::vec3 direction = glm::vec3(rotation * worldArcDirection4);
        vertices.push_back(worldOrigin + direction * innerRadius);
        vertices.push_back(worldOrigin + direction * outerRadius);
    }

    //angle = 0
    glm::vec3 middleCollisionDirection = glm::vec3(worldArcDirection4);
    vertices.push_back(worldOrigin + middleCollisionDirection * innerRadius);
    vertices.push_back(worldOrigin + middleCollisionDirection * outerRadius);

    // Add vertices in between
    for (float angle = angleStep; angle < halfArcAngle; angle += angleStep)
    {
        glm::mat4 rotation = glm::rotate(glm::mat4(1.f), angle, worldArcNormal);
        glm::vec3 direction = glm::vec3(rotation * worldArcDirection4);
        vertices.push_back(worldOrigin + direction * innerRadius);
        vertices.push_back(worldOrigin + direction * outerRadius);
    }

    //angle = halfArcAngle
    glm::mat4 outerRotation = glm::rotate(glm::mat4(1.f), halfArcAngle, worldArcNormal);
    glm::vec3 outerCollisionDirection = glm::vec3(outerRotation * worldArcDirection4);
    vertices.push_back(worldOrigin + outerCollisionDirection * innerRadius);
    vertices.push_back(worldOrigin + outerCollisionDirection * outerRadius);
    int outerIndex = vertices.size() - 1;
    const int subSegmentCount = vertices.size() / 2;
    const int quadsToDraw = subSegmentCount - 1;

    // Build triangle indices
    for (int i = 0; i < quadsToDraw; ++i)
    {
        // First triangle of the quad
        indices.push_back(i*2); // Inner 1
        indices.push_back(i*2 + 1); // Outer 1
        indices.push_back(i*2 + 2); // Inner 2

        // Second triangle of the quad
        indices.push_back(i*2 + 2); // Inner 2
        indices.push_back(i*2 + 1); // Outer 1
        indices.push_back(i*2 + 3); // Outer 3
    }

    renderFunctor.drawMesh(vertices, indices, colorId, alpha);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename RendererFunctorType>
void drawArcTriangle(RendererFunctorType renderFunctor, glm::vec3 worldOrigin, glm::vec3 worldArcDirection, glm::vec3 worldArcNormal, float arcAngle, float radius, unsigned int colorId)
{
	glm::vec4 worldArcDirection4(worldArcDirection, 0.f);
	glm::mat4 innerRotaition = glm::rotate(glm::mat4(1.f), arcAngle * 0.5f, -worldArcNormal);
	glm::vec3 innerCollisionDirection = glm::vec3(innerRotaition * worldArcDirection4);
	glm::mat4 outerRotation = glm::rotate(glm::mat4(1.f), arcAngle * 0.5f, worldArcNormal);
	glm::vec3 outerCollisionDirection = glm::vec3(outerRotation * worldArcDirection4);
	renderFunctor.drawTriangle(worldOrigin, worldOrigin + innerCollisionDirection * radius, worldOrigin + outerCollisionDirection * radius, colorId);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Weapon-shape helper — replaces the previous 3-line "L / 3-point-path" attack indicator
// with a stylised weapon: handle line, rectangular shaft (2 triangles), equilateral
// triangular tip (1 triangle), and a thin center mid-line down the blade.
//   handleStart    — near end of the handle
//   handleEnd      — where the handle line ends and the shaft rectangle begins
//   weaponEnd      — tip of the weapon (triangle apex, e.g. aim tip)
//   shaftPerp      — unit vector perpendicular to weapon axis, in the plane the shaft
//                    rectangle should occupy (horizontal plane for side swings,
//                    vertical plane containing weapon axis + up for forward thrust /
//                    perpendicular to the swing rotation axis for the radial-viz active
//                    swing). Caller computes.
//   colorId        — draw color for all primitives
//   handleThickness — line thickness for the handle (mid-line uses half of this)
//   shaftWidth     — shaft rectangle perpendicular width (in m)
//   triangleSide   — equilateral triangle tip side length (in m)
template <typename RendererFunctorType>
void drawWeapon(RendererFunctorType renderFunctor,
    const glm::vec3& handleStart,
    const glm::vec3& handleEnd,
    const glm::vec3& weaponEnd,
    const glm::vec3& shaftPerp,
    unsigned int colorId,
    float handleThickness,
    float shaftWidth,
    float triangleSide)
{
    const glm::vec3 weaponAxis     = glm::normalize(weaponEnd - handleStart);
    const float     triangleHeight = triangleSide * 0.5f * 1.7320508f;   // (√3/2)·side
    const glm::vec3 triangleBase   = weaponEnd - weaponAxis * triangleHeight;

    // Handle line.
    renderFunctor.drawLine(handleStart, handleEnd, colorId, handleThickness);

    // Shaft rectangle (2 triangles).
    const glm::vec3 shaftHalfW = shaftPerp * (shaftWidth * 0.5f);
    renderFunctor.drawTriangle(handleEnd - shaftHalfW, handleEnd + shaftHalfW, triangleBase + shaftHalfW, colorId);
    renderFunctor.drawTriangle(handleEnd - shaftHalfW, triangleBase + shaftHalfW, triangleBase - shaftHalfW, colorId);

    // Triangle tip (equilateral, apex at weaponEnd).
    const glm::vec3 triangleHalfB = shaftPerp * (triangleSide * 0.5f);
    renderFunctor.drawTriangle(triangleBase - triangleHalfB, triangleBase + triangleHalfB, weaponEnd, colorId);

    // Center mid-line down the blade (half the handle thickness).
    renderFunctor.drawLine(handleEnd, weaponEnd, colorId, handleThickness * 0.5f);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Classification of the pending attack direction from Idle, mirroring the
// forward/left/right if-cascade in dAttackMachineSimulation::integrate3.
enum class AttackDirectionKind { Forward, Left, Right };

// Geometry shared by the aim-indicator draw (dAttackAimVisualization) and the new
// attacker-side block-prediction viz (dAttackBlockPredictionVisualization). Extracting
// it into one pure helper keeps the two consumers from drifting on either the
// inner-circle origin point or the predicted attack-sequence id.
struct AttackIndicatorGeometry
{
	AttackDirectionKind kind;
	glm::vec3 originOnInnerCircle;             // block-line anchor on the inner circle
	glm::vec3 attackDirectionXY;              // unit ground-plane vector pointing into the sector
	unsigned int predictedSequenceIdFromIdle; // 4 (forward), 1 (left), 0 (right) — mirrors integrate3
};

// The geometry for an already classified sequence id: computeAttackIndicatorGeometry is this
// on dAttackDirection::classify's id. A ground-frame indicator (og-ground-aligned-attacks
// task 11, below) classifies the WORLD aim and move and builds the geometry on the aim's
// local heading.
inline AttackIndicatorGeometry attackIndicatorGeometryFor(
	unsigned int attackSequenceId,
	const glm::vec3& aimDirection,
	const glm::vec3& rootTranslation,
	float innerRadius)
{
	const glm::vec3 aimDirectionXY =
		glm::normalize(simFrame::flatten(aimDirection));

	AttackIndicatorGeometry geometry;
	if (attackSequenceId == dAttackDirection::kForwardSequenceId)
	{
		geometry.kind                       = AttackDirectionKind::Forward;
		geometry.attackDirectionXY          = aimDirectionXY;
		geometry.originOnInnerCircle        = rootTranslation + aimDirectionXY * innerRadius;
		geometry.predictedSequenceIdFromIdle = 4;
	}
	else
	{
		const bool isLeft            = attackSequenceId == dAttackDirection::kLeftSequenceId;
		const float thresholdOffset  = isLeft ? -dAttackDirection::kForwardConeHalfAngle
		                                      : dAttackDirection::kForwardConeHalfAngle;
		constexpr glm::vec3 turnAxis = simFrame::kTurnAxis;
		const glm::mat4 thresholdRot = glm::rotate(glm::mat4(1.f), thresholdOffset, turnAxis);
		const glm::vec3 thresholdDir = glm::vec3(thresholdRot * glm::vec4(aimDirectionXY, 0.f));

		geometry.kind                       = isLeft ? AttackDirectionKind::Left : AttackDirectionKind::Right;
		geometry.attackDirectionXY          = thresholdDir;
		geometry.originOnInnerCircle        = rootTranslation + thresholdDir * innerRadius;
		geometry.predictedSequenceIdFromIdle = isLeft ? 1u : 0u;
	}
	return geometry;
}

// Pure classifier + inner-circle origin-point computation. The sequence id and the
// forward/side branch mirror dAttackMachineSimulation::integrate3 (lines 573-588); the
// origin point mirrors dAttackAimVisualization's forwardEnd (forward) / frontPoint (side).
//
// NOTE: the forward-case magnitude gate uses the FIVE-zero epsilon 0.00001f matching
// integrate3 line 573 — NOT the four-zero (0.0001f) yellow-move-line render gate in
// dAttackAimVisualization, which is a separate, pre-existing inconsistency left untouched.
//
// Pre-existing boundary behaviour preserved byte-for-byte: the side test is a strict
// `signedAngle < -pi/6` (it was `> pi/6` before og-sim-metres-yup task 26 negated the
// planar angles), so exactly signedAngle == -pi/6 falls to the Right branch (seq 0) —
// the same UX quirk the inline aim-viz code had; NOT fixed here.
inline AttackIndicatorGeometry computeAttackIndicatorGeometry(
	const glm::vec3& aimDirection,
	const glm::vec2& moveDirection,        // 2D stick, matches PlayerInput::moveStick
	const glm::vec3& moveDirectionWorld,   // separate 3D field
	const glm::vec3& rootTranslation,
	float innerRadius)
{
	// ⛔ THE DECISION IS NOT MADE HERE. dAttackDirection::classify is the ONE
	// definition, shared with the authoritative simulation path, so the indicator
	// cannot draw one direction while the attack goes another. This function only
	// turns that decision into geometry.
	const unsigned int attackSequenceId =
		dAttackDirection::classify(aimDirection, moveDirectionWorld, moveDirection);

	return attackIndicatorGeometryFor(attackSequenceId, aimDirection, rootTranslation, innerRadius);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// [og-ground-aligned-attacks task 11] Indicators drawn in a body's ground frame.
//
// The up an indicator follows (the user's OQ9 ruling at G3, 2026-10-29): while idle, the guard
// body's up, i.e. the ground normal the next swing will freeze into its frame; during a real
// attack sequence, the radial body's swing-frame up R_ic * kUp; the guard indicators always the
// guard body's up. The weapon's idle pose stays world-aligned by design, which is why an idle
// indicator takes the guard body's up and not the weapon body's.
//
// An indicator whose up drawsWorldAligned() runs today's world-horizontal expressions verbatim
// with the caller's renderer (the flat rule). Otherwise it runs the same expressions on the world
// aim and move taken into the ground frame (localHeading) about the indicator's centre, and draws
// through GroundFrameRenderer, which tilts every point back about that centre. The attack
// direction is classified on the WORLD aim and move either way: dAttackDirection::classify is the
// machine's classifier and it stays world, so the indicator cannot pick a different
// swing than the machine.

// True when an indicator about `up` draws with today's world-horizontal expressions: a flat up
// (surfaceFrame::isFlat), and also an up that does not point up or is not finite, which a display
// path must not hand to surfaceFrame's tilt (it asserts dot(kUp, n) > 0).
inline bool drawsWorldAligned(const glm::vec3& up)
{
	constexpr glm::vec3 worldUp = simFrame::kUp;
	return surfaceFrame::isFlat(up) || !(glm::dot(worldUp, up) > 0.f);
}

// The guard body's up, from its body transform built the way DAttackTargetVisualizationTwo.h
// builds guardTransform. dAttackGuardSimulation::integrate poses the guard in the ground frame
// every tick (og-ground-aligned-attacks task 6), so this is the current ground normal.
inline glm::vec3 guardBodyUp(const dAttackGuardSimulation::State& guardState)
{
	const glm::mat4 guardTransform = glm::translate(glm::mat4(1.f), guardState.bodyState.position)
	                                * glm::mat4_cast(guardState.bodyState.rotation);
	return surfaceFrame::surfaceUpOf(guardTransform);
}

// The radial body's swing-frame up, R_ic * kUp: the same expression as the up= field of
// [Radial.setInitialConditions] (dAttackRadialSimulation::setInitialConditions).
inline glm::vec3 swingFrameUp(const dAttackRadialSimulation::InitialConditions& initialConditions)
{
	constexpr glm::vec3 up = simFrame::kUp;
	return glm::mat3(glm::rotate(glm::mat4(1.f), initialConditions.initialAimAngle, initialConditions.initialAimRotationAxis)) * up;
}

// The aim indicator's up: the swing frame's during a real attack sequence, else the guard body's.
inline glm::vec3 aimIndicatorUp(const dAttackRadialSimulation::InitialConditions& initialConditions,
	const dAttackGuardSimulation::State& guardState)
{
	if (isRealAttackSequence(initialConditions.activeAttackSequence))
		return swingFrameUp(initialConditions);
	return guardBodyUp(guardState);
}

// A world direction as the indicator's local heading: taken into the ground frame and flattened,
// the expression dAttackMachineSimulation's swing frame and dAttackGuardSimulation's pose apply to
// the aim, so the indicator points where the swing will go (the orthogonal projection of the
// world direction onto the ground plane, read in the local frame).
inline glm::vec3 localHeading(const glm::vec3& up, const glm::vec3& worldDirection)
{
	return glm::normalize(simFrame::flatten(surfaceFrame::tiltFromSurface(up, worldDirection)));
}

// A world point in the indicator's local frame about `centre`; GroundFrameRenderer::toWorld inverts it.
inline glm::vec3 localPoint(const glm::vec3& centre, const glm::vec3& up, const glm::vec3& worldPoint)
{
	return centre + surfaceFrame::tiltFromSurface(up, worldPoint - centre);
}

// A renderer for an indicator computed in its local frame (up = kUp) about `centre`: every point
// p is drawn at centre + G(up) * (p - centre). drawCircleArc becomes kArcSegments lines swept about
// the local up, because DrawDebugCircleArc orients its plane by world up (FindBestAxisVectors) and
// cannot draw a tilted arc. The renderer is held by value, as every viz holds its renderer.
template <typename RendererFunctorType>
class GroundFrameRenderer
{
public:
	// The segment count DAttackRendererFunctorUImpl::drawCircleArc passes DrawDebugCircleArc.
	static constexpr int kArcSegments = 10;

	GroundFrameRenderer(RendererFunctorType renderer, const glm::vec3& centre, const glm::vec3& up)
		: m_renderer(renderer)
		, m_centre(centre)
		, m_up(up)
	{}

	glm::vec3 toWorld(const glm::vec3& point) const
	{
		return m_centre + surfaceFrame::tiltToSurface(m_up, point - m_centre);
	}

	void drawLine(const glm::vec3& start, const glm::vec3& end, unsigned int colorId, float thickness) const
	{
		m_renderer.drawLine(toWorld(start), toWorld(end), colorId, thickness);
	}

	// `angle` is the half width, as DrawDebugCircleArc's AngleWidth: the arc spans
	// direction turned by -angle .. +angle.
	void drawCircleArc(const glm::vec3& center, const glm::vec3& direction, float radius, float angle, unsigned int colorId, float thickness) const
	{
		constexpr glm::vec3 turnAxis = simFrame::kTurnAxis;
		const glm::vec4 along(glm::normalize(direction), 0.f);
		const float step = angle * 2.f / static_cast<float>(kArcSegments);
		glm::vec3 previous = center + glm::vec3(glm::rotate(glm::mat4(1.f), -angle, turnAxis) * along) * radius;
		for (int segment = 1; segment <= kArcSegments; ++segment)
		{
			const float segmentAngle = -angle + step * static_cast<float>(segment);
			const glm::vec3 next = center + glm::vec3(glm::rotate(glm::mat4(1.f), segmentAngle, turnAxis) * along) * radius;
			drawLine(previous, next, colorId, thickness);
			previous = next;
		}
	}

	void drawTriangle(const glm::vec3& one, const glm::vec3& two, const glm::vec3& three, unsigned int colorId) const
	{
		m_renderer.drawTriangle(toWorld(one), toWorld(two), toWorld(three), colorId);
	}

	void drawMesh(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices, unsigned int colorId, unsigned int alpha) const
	{
		std::vector<glm::vec3> worldVertices;
		worldVertices.reserve(vertices.size());
		for (const glm::vec3& vertex : vertices)
			worldVertices.push_back(toWorld(vertex));
		m_renderer.drawMesh(worldVertices, indices, colorId, alpha);
	}

private:
	// mutable: the draw members are const, as the UE renderer's are, and a test renderer's may not be.
	mutable RendererFunctorType m_renderer;
	glm::vec3 m_centre;
	glm::vec3 m_up;
};

// Runs `draw` with the caller's renderer when `up` drawsWorldAligned, else with a
// GroundFrameRenderer about `centre`. The caller computes draw's inputs with the same test:
// world inputs for the first, localHeading / localPoint for the second.
template <typename RendererFunctorType, typename DrawType>
void drawInIndicatorFrame(RendererFunctorType renderer, const glm::vec3& centre, const glm::vec3& up, DrawType&& draw)
{
	if (drawsWorldAligned(up))
		draw(renderer);
	else
		draw(GroundFrameRenderer<RendererFunctorType>(renderer, centre, up));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}

OGSIM_OPTIMIZE_ON
 
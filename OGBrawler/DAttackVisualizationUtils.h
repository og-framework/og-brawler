#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext/scalar_constants.hpp"
#include "DAttackRadialSimulation.h"
#include "DAttackMachineSimulation.h"
#include "DAttackCircle.h"
#include "OGSimulation/OGAssert.h"

#pragma optimize( "", off )


namespace dAttackVisualizationUtils
{	

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
//                    rectangle should occupy (horizontal/XY plane for side swings,
//                    vertical plane containing weapon axis + Z for forward thrust /
//                    perpendicular to the swing rotation axis for the radial-viz active
//                    swing). Caller computes.
//   colorId        — draw color for all primitives
//   handleThickness — line thickness for the handle (mid-line uses half of this)
//   shaftWidth     — shaft rectangle perpendicular width (in cm)
//   triangleSide   — equilateral triangle tip side length (in cm)
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
	glm::vec3 attackDirectionXY;              // unit XY vector pointing into the sector
	unsigned int predictedSequenceIdFromIdle; // 4 (forward), 1 (left), 0 (right) — mirrors integrate3
};

// Pure classifier + inner-circle origin-point computation. The sequence id and the
// forward/side branch mirror dAttackMachineSimulation::integrate3 (lines 573-588); the
// origin point mirrors dAttackAimVisualization's forwardEnd (forward) / frontPoint (side).
//
// NOTE: the forward-case magnitude gate uses the FIVE-zero epsilon 0.00001f matching
// integrate3 line 573 — NOT the four-zero (0.0001f) yellow-move-line render gate in
// dAttackAimVisualization, which is a separate, pre-existing inconsistency left untouched.
//
// Pre-existing boundary behaviour preserved byte-for-byte: the side test is a strict
// `signedAngle > pi/6`, so exactly signedAngle == pi/6 falls to the Right branch (seq 0) —
// the same UX quirk the inline aim-viz code had; NOT fixed here.
inline AttackIndicatorGeometry computeAttackIndicatorGeometry(
	const glm::vec3& aimDirection,
	const glm::vec2& moveDirection,        // 2D stick, matches PlayerInput::moveDirection
	const glm::vec3& moveDirectionWorld,   // separate 3D field
	const glm::vec3& rootTranslation,
	float innerRadius)
{
	const glm::vec3 moveDirectionN  = glm::normalize(glm::vec3(moveDirectionWorld));
	const glm::vec3 moveDirectionXY = glm::normalize(glm::vec3(moveDirectionN.x, moveDirectionN.y, 0.f));
	const glm::vec3 aimDirectionXY  = glm::normalize(glm::vec3(aimDirection.x, aimDirection.y, 0.f));

	// Both terms XY-projected; mirrors dAttackMachineSimulation::integrate3. Clamp the
	// dot to [-1, 1] so FP rounding can't push it above 1.0 and turn acos into NaN.
	const float dotXY       = glm::clamp(glm::dot(aimDirectionXY, moveDirectionXY), -1.f, 1.f);
	const float angle       = glm::acos(dotXY);
	const float signedAngle = glm::sign(glm::cross(aimDirectionXY, moveDirectionXY).z) * angle;

	const float attackDirectionAngleIncrement = glm::pi<float>() / 6.f;

	const bool forwardCase =
		glm::length(moveDirection) < 0.00001f
		|| (signedAngle < attackDirectionAngleIncrement && signedAngle > -attackDirectionAngleIncrement);

	AttackIndicatorGeometry geometry;
	if (forwardCase)
	{
		geometry.kind                       = AttackDirectionKind::Forward;
		geometry.attackDirectionXY          = aimDirectionXY;
		geometry.originOnInnerCircle        = rootTranslation + aimDirectionXY * innerRadius;
		geometry.predictedSequenceIdFromIdle = 4;
	}
	else
	{
		const bool isLeft            = signedAngle > attackDirectionAngleIncrement;
		const float thresholdOffset  = isLeft ? attackDirectionAngleIncrement : -attackDirectionAngleIncrement;
		const glm::mat4 thresholdRot = glm::rotate(glm::mat4(1.f), thresholdOffset, glm::vec3(0.f, 0.f, 1.f));
		const glm::vec3 thresholdDir = glm::vec3(thresholdRot * glm::vec4(aimDirectionXY, 0.f));

		geometry.kind                       = isLeft ? AttackDirectionKind::Left : AttackDirectionKind::Right;
		geometry.attackDirectionXY          = thresholdDir;
		geometry.originOnInnerCircle        = rootTranslation + thresholdDir * innerRadius;
		geometry.predictedSequenceIdFromIdle = isLeft ? 1u : 0u;
	}
	return geometry;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}

#pragma optimize( "", on )
 
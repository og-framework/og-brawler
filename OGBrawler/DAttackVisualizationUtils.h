#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
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

template <typename RendererFunctorType>
void drawSegmentSolid(RendererFunctorType renderFunctor, glm::vec3 worldOrigin, glm::vec3 worldArcDirection, glm::vec3 worldArcNormal, float arcAngle, float outerRadius, float innerRadius, unsigned int colorId, float thickness)
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

    renderFunctor.drawMesh(vertices, indices, colorId);
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

}

#pragma optimize( "", on )
 
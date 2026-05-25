#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>

#include <vector>

struct EllipsoidPart
{
    glm::vec3 localOffset{0.f};
    glm::vec3 radii{1.f};
    glm::quat localRotation{1.f, 0.f, 0.f, 0.f};
};

struct ConvexHullPart
{
    glm::vec3                 localOffset{0.f};
    glm::quat                 localRotation{1.f, 0.f, 0.f, 0.f};
    std::vector<glm::vec3>    corners;     // local-space vertices
    std::vector<unsigned int> triangles;   // index triplets; winding decided by builder convention
};

struct TaperedCylinderPart
{
    glm::vec3 localOffset{0.f};
    glm::quat localRotation{1.f, 0.f, 0.f, 0.f};
    float topRadius{1.f};       // radius at +Z end
    float bottomRadius{1.f};    // radius at -Z end
    float height{1.f};          // total span along local Z; before localOffset, the cylinder spans [-height/2, +height/2]
    int   segments{16};         // longitudinal segments around the cylinder
};

struct HumanoidVisualization
{
    EllipsoidPart head;
    EllipsoidPart torso;
    EllipsoidPart legs;
};

namespace humanoidVisualizationDefaults
{

inline HumanoidVisualization buildDefault()
{
    HumanoidVisualization viz;
    viz.head.localOffset    = glm::vec3(0.f,  0.f, 170.f);
    viz.head.radii          = glm::vec3(12.f, 12.f, 15.f);
    viz.head.localRotation  = glm::quat(1.f, 0.f, 0.f, 0.f);

    viz.torso.localOffset   = glm::vec3(0.f,  0.f, 120.f);
    viz.torso.radii         = glm::vec3(25.f, 18.f, 35.f);
    viz.torso.localRotation = glm::quat(1.f, 0.f, 0.f, 0.f);

    viz.legs.localOffset    = glm::vec3(0.f,  0.f, 50.f);
    viz.legs.radii          = glm::vec3(18.f, 18.f, 50.f);
    viz.legs.localRotation  = glm::quat(1.f, 0.f, 0.f, 0.f);

    return viz;
}

} // namespace humanoidVisualizationDefaults

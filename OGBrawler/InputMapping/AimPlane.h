#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/AimPlane-rationale.md

#include "OGSimulation/OGExport.h"
#include "glm/vec3.hpp"

#include <optional>

namespace dInput
{
namespace aimPlane
{

inline constexpr float kParallelTolerance = 1.e-4f;
inline constexpr float kZeroAimTolerance = 1.e-4f;

using AimAxes = glm::vec3 (*)(const glm::dvec3& offset);

OGBRAWLER_API std::optional<glm::vec3> tiltedPlaneUp(const glm::vec3& groundUp);

OGBRAWLER_API std::optional<glm::dvec3> hitOffset(const glm::dvec3& rayOrigin, const glm::dvec3& rayDirection,
	const glm::dvec3& planePoint, const glm::dvec3& planeNormal);

OGBRAWLER_API std::optional<glm::vec3> unitAim(const glm::dvec3& rayOrigin, const glm::dvec3& rayDirection,
	const glm::dvec3& planePoint, const glm::dvec3& planeNormal, AimAxes toAimAxes);

} // namespace aimPlane
} // namespace dInput

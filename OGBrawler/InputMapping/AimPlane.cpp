// SPDX-License-Identifier: BUSL-1.1
// docs/AimPlane-rationale.md
#include "OGBrawler/InputMapping/AimPlane.h"

#include "OGSimulation/SimulationFrame.h"
#include "OGSimulation/SurfaceFrame.h"
#include "glm/geometric.hpp"

#include <cmath>

namespace dInput
{
namespace aimPlane
{
namespace
{
double dotOf(const glm::dvec3& a, const glm::dvec3& b)
{
	return a.x * b.x + a.y * b.y + a.z * b.z;
}
} // namespace

std::optional<glm::vec3> tiltedPlaneUp(const glm::vec3& groundUp)
{
	constexpr glm::vec3 up = simFrame::kUp;
	if (surfaceFrame::isFlat(groundUp) || !(glm::dot(up, groundUp) > 0.f))
		return std::nullopt;
	return groundUp;
}

std::optional<glm::dvec3> hitOffset(const glm::dvec3& rayOrigin, const glm::dvec3& rayDirection,
	const glm::dvec3& planePoint, const glm::dvec3& planeNormal)
{
	const glm::dvec3 planePointToRayOrigin = rayOrigin - planePoint;
	const float alongNormal = static_cast<float>(dotOf(planeNormal, rayDirection));
	if (std::fabs(alongNormal) < kParallelTolerance)
		return std::nullopt;
	const float distance = static_cast<float>(dotOf(planeNormal, planePointToRayOrigin) / alongNormal);
	const glm::dvec3 hitPoint = rayOrigin - rayDirection * static_cast<double>(distance);
	const glm::dvec3 offset = hitPoint - planePoint;
	constexpr double zeroTolerance = kZeroAimTolerance;
	if (std::fabs(offset.x) <= zeroTolerance && std::fabs(offset.y) <= zeroTolerance && std::fabs(offset.z) <= zeroTolerance)
		return std::nullopt;
	return offset;
}

std::optional<glm::vec3> unitAim(const glm::dvec3& rayOrigin, const glm::dvec3& rayDirection,
	const glm::dvec3& planePoint, const glm::dvec3& planeNormal, AimAxes toAimAxes)
{
	const std::optional<glm::dvec3> offset = hitOffset(rayOrigin, rayDirection, planePoint, planeNormal);
	if (!offset.has_value())
		return std::nullopt;
	return glm::normalize(toAimAxes(*offset));
}

} // namespace aimPlane
} // namespace dInput

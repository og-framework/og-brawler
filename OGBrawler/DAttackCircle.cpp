// SPDX-License-Identifier: BUSL-1.1
#include "DAttackCircle.h"
#include "OGSimulation/OGAssert.h"
#include <algorithm>
#include <cmath>


DAttackCircle::DAttackCircle(unsigned int attackSegments,
	float innerRadius,
	float outerRadius,
	float thickness,
	bool offsetWithSegmentHalf,
	float forwardRangeMultiplier)
	: m_attackSegments(attackSegments)
	, m_innerRadius(innerRadius)
	, m_outerRadius(outerRadius)
	, m_thickness(thickness)
	, m_offsetWithSegmentHalf(offsetWithSegmentHalf)
	, m_forwardRangeMultiplier(forwardRangeMultiplier)
{
	OG_CHECK(attackSegments >= 1, "attackSegments must be at least 1");
	OG_CHECK(innerRadius >= 0.f, "innerRadius must be at least 0");
	OG_CHECK(outerRadius >= 0.f, "outerRadius must be at least 0");
	OG_CHECK(innerRadius < outerRadius, "innerRadius must be less than outerRadius");
	OG_CHECK(forwardRangeMultiplier >= 0.f, "forwardRangeMultiplier must be at least 0");

}

void DAttackCircle::getLines(std::vector<glm::vec3>& lines, bool getOuter) const
{
	float radius = getOuter ? m_outerRadius : m_innerRadius;
	for (unsigned int i = 0; i < m_attackSegments; ++i)
	{
		float angle = i * getSegmentLength();
		float nextAngle = (i + 1) * getSegmentLength();
		if (m_offsetWithSegmentHalf)
		{
			angle += getSegmentLength() * 0.5f;
		}
		lines.push_back(glm::vec3(radius * cos(angle) * m_forwardRangeMultiplier, radius * sin(angle), 0.f));
	}
}
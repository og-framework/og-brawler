#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include <vector>
#include "glm/vec3.hpp"

class DAttackCircle
{
public:
	OGBRAWLER_API DAttackCircle(unsigned int attackSegments,
		float innerRadius,
		float outerRadius,
		float thickness,
		bool offsetWithSegmentHalf,
		float forwardRangeMultiplier);

	float getSegmentLength() const
	{ 
		return (6.28318530717f) / m_attackSegments;
	}
	
	float getInnerRadius() const
	{
		return m_innerRadius;
	}	

	float getOuterRadius() const
	{
		return m_outerRadius;
	}

	float getThickness() const
	{
		return m_thickness;
	}

	float getHalfThickness() const
	{
		return m_thickness * 0.5f;
	}

	OGBRAWLER_API void getLines(std::vector<glm::vec3>& lines, bool getOuter) const;

private:
	DAttackCircle() = default;

	unsigned int m_attackSegments;
	float m_innerRadius;
	float m_outerRadius;
	float m_thickness;
	bool m_offsetWithSegmentHalf;
	float m_forwardRangeMultiplier;
};


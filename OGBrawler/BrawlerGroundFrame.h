#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerGroundFrame-rationale.md · docs/BrawlerGroundFrame-guards.md

#include "glm/vec3.hpp"

#include "OGSimulation/SimulationFrame.h"
#include "OGSimulation/SurfaceFrame.h"

namespace brawlerGround
{
    // ⛔G-01  docs/BrawlerGroundFrame-guards.md
    inline constexpr bool kGroundAlignedAttacks = false;

    struct GroundSample
    {
        glm::vec3 normal = simFrame::kUp;
    };

    static_assert(simFrameDetail::sameBits(GroundSample{}.normal, simFrame::kUp),
        "brawlerGround::GroundSample - a default sample's normal must have simFrame::kUp's bits "
        "(+0 zeros): it is the sample every consumer receives while kGroundAlignedAttacks is false "
        "and on unwalkable ground, and the up a consumer's flat path was written against.");
} // namespace brawlerGround

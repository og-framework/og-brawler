#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerInboundHit-rationale.md

#include "glm/vec2.hpp"
#include "OGBrawler/HitReaction.h"

namespace brawlerInboundHit
{
    class DerivedState
    {
    public:
        bool wasHitThisTick               = false;
        bool wasProjectileBlockedThisTick = false;
        bool wasGuardBlockedThisTick      = false;

        HitReactionKind reactionKind   = HitReactionKind::Stun;
        float           knockbackSpeed = 0.f;
        float           flinchDuration = 0.f;
        glm::vec2       hitDirectionXY{0.f};
    };
}

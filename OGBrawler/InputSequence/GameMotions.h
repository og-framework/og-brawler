#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGBrawler/InputSequence/InputSequence.h"
#include <vector>

// Static motion-command definitions used by buildPlayerInput.
//
// Hadouken (AimRelative movement mode): the stick angle sweeps from Back (pi)
// through DownBack (3*pi/4) to Down (pi/2), then attackLeft rising edge fires
// the projectile. In AimRelative WASD this is S -> S+A -> A + LMB, regardless
// of mouse position (because in AimRelative the WASD keys produce aim-rotated
// world directions, so S=Back, A=Down, S+A=DownBack).
//
// Caveat: in CameraRelative / MoveRelativeAim modes the WASD keys produce
// camera-frame directions but the matcher reference is still aim, so the
// firing WASD arc rotates with the mouse in those modes. Making the matcher's
// reference movement-mode-aware is a follow-up.
inline const std::vector<inputSequence::MotionCommand> kGameMotions =
{
    {
        // steps: oldest to newest
        {
            { inputSequence::angle::Back,     inputSequence::pi / 8.f, 8 },
            { inputSequence::angle::DownBack, inputSequence::pi / 8.f, 8 },
            { inputSequence::angle::Down,     inputSequence::pi / 8.f, 8 },
        },
        /* requiredButtonEdge    = */ 0b01,  // bit 0 = attackLeft
        /* windowAfterFinalStep  = */ 6,
        /* actionId              = */ inputSequence::kHadoukenActionId,
    },
};

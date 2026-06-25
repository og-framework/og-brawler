#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGBrawler/InputSequence/InputSequence.h"
#include <vector>

// Static motion-command definitions used by buildPlayerInput.
//
// All three motions below fire the same Hadouken (kHadoukenActionId); the
// machine-sim trigger block routes on the actionId, so the matcher decides
// which input shapes count. The matcher's tie-break (longer step count wins)
// means the 3-step quarter arcs beat the 1-step shortcut when both match the
// same buffer, so a clean QCF-style input is still treated as the intended one.
//
// AimRelative WASD mapping (S=Back, A=Down, D=Up; mouse-independent because in
// AimRelative the WASD keys produce aim-rotated world directions):
//   1) S -> S+A -> A + LMB   left-handed quarter arc  (Back -> DownBack -> Down)
//   2) S -> S+D -> D + LMB   right-handed quarter arc (Back -> UpBack   -> Up)
//   3) Hold S + tap RightAttack (RT)  single-step back-shortcut (Back)
//
// Caveat: in CameraRelative / MoveRelativeAim modes the WASD keys produce
// camera-frame directions but the matcher reference is still aim, so the
// firing WASD arc rotates with the mouse in those modes. Making the matcher's
// reference movement-mode-aware is a follow-up.
inline const std::vector<inputSequence::MotionCommand> kGameMotions =
{
    // Original — S -> S+A -> A + LMB (left-handed quarter arc)
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

    // Mirror motion — S -> S+D -> D + LMB (right-handed quarter arc)
    {
        {
            { inputSequence::angle::Back,   inputSequence::pi / 8.f, 8 },
            { inputSequence::angle::UpBack, inputSequence::pi / 8.f, 8 },
            { inputSequence::angle::Up,     inputSequence::pi / 8.f, 8 },
        },
        /* requiredButtonEdge    = */ 0b01,  // attackLeft rising edge
        /* windowAfterFinalStep  = */ 6,
        /* actionId              = */ inputSequence::kHadoukenActionId,
    },

    // Back-shortcut — Hold S + tap RightAttack (single-step motion)
    {
        {
            { inputSequence::angle::Back, inputSequence::pi / 8.f, 8 },
        },
        /* requiredButtonEdge    = */ 0b10,  // attackRight rising edge
        /* windowAfterFinalStep  = */ 6,
        /* actionId              = */ inputSequence::kHadoukenActionId,
    },
};

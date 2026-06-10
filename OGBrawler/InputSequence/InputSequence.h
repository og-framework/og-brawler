#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <cstdint>
#include <optional>
#include <vector>
#include <cmath>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/geometric.hpp"
#include "glm/common.hpp"

// Forward-declare the PlayerInput type the HistoryAccessor returns.
// Callers that include this header also include the full dAttackMachineSimulation header.
namespace dAttackMachineSimulation { class PlayerInput; }

namespace inputSequence
{

inline constexpr float pi = 3.14159265358979323846f;

// Named aim-relative angles (radians). Forward = aim direction (0).
// Angles increase CLOCKWISE around +Z (player looking down toward the world):
//   +pi/2 = right-of-aim ("Down" in SF numpad convention),
//   +pi   = back,
//   -pi/2 = left-of-aim.
// Use these named constants in MotionCommand definitions for readability, or
// pass arbitrary radian values for non-octant target angles.
namespace angle
{
    inline constexpr float Forward     =  0.f;
    inline constexpr float DownForward =  pi / 4.f;
    inline constexpr float Down        =  pi / 2.f;
    inline constexpr float DownBack    =  3.f * pi / 4.f;
    inline constexpr float Back        =  pi;
    inline constexpr float UpBack      = -3.f * pi / 4.f;
    inline constexpr float Up          = -pi / 2.f;
    inline constexpr float UpForward   = -pi / 4.f;
}

// Static-data POD — never serialized.
// A step matches a frame whose aim-relative stick angle is within
// angleTolerance of targetAngle (shortest-arc distance).
struct MotionStep
{
    float    targetAngle;     // radians, aim-relative direction the stick should point
    float    angleTolerance;  // radians; e.g. pi/8 reproduces classic 45°-sector matching
    uint8_t  maxGapFrames;    // max temporal gap to the next (newer) step
};

// Static-data POD — never serialized.
struct MotionCommand
{
    std::vector<MotionStep> steps;
    uint8_t  requiredButtonEdge;
    uint8_t  windowAfterFinalStep;
    uint32_t actionId;
};

inline constexpr uint32_t kNoMatch             = 0;
inline constexpr uint32_t kHadoukenActionId    = 1;
inline constexpr uint32_t kHistoryWindowFrames = 30;

// ---------------------------------------------------------------------------
// aimRelativeAngle — aim-relative angle of stick in [-pi, pi], or nullopt
// when stick is below deadzone or the reference forward is degenerate.
//
// Convention: angle 0 = stick aligned with referenceForwardXY (numpad 6).
// Angle increases CLOCKWISE around +Z, so +pi/2 = right-of-aim (numpad 2),
// +pi = back (numpad 4), -pi/2 = left-of-aim (numpad 8).
//
// Deterministic across CPUs — uses glm::clamp + std::atan2.
// ---------------------------------------------------------------------------
inline std::optional<float> aimRelativeAngle(glm::vec2 stick, glm::vec3 referenceForwardXY, float deadzone)
{
    if (glm::length(stick) < deadzone)
        return std::nullopt;

    const glm::vec2 refXY(referenceForwardXY.x, referenceForwardXY.y);
    const float refLen = glm::length(refXY);
    if (refLen < 1e-6f)
        return std::nullopt;

    const glm::vec2 fwd   = refXY / refLen;
    const glm::vec2 right(fwd.y, -fwd.x); // 90° clockwise of forward (right-hand z-up).
    const glm::vec2 sNorm = glm::normalize(stick);

    const float dotF = glm::clamp(glm::dot(fwd,   sNorm), -1.f, 1.f);
    const float dotR = glm::clamp(glm::dot(right, sNorm), -1.f, 1.f);

    return std::atan2(dotR, dotF); // [-pi, pi]
}

// ---------------------------------------------------------------------------
// angularDistance — shortest unsigned angular distance between two angles.
// Result is in [0, pi]. Wraps correctly across the +pi / -pi boundary.
// ---------------------------------------------------------------------------
inline float angularDistance(float a, float b)
{
    float d = std::fmod(a - b + pi, 2.f * pi);
    if (d < 0.f)
        d += 2.f * pi;
    return std::abs(d - pi);
}

// ---------------------------------------------------------------------------
// matchSequence
//
// HistoryAccessor: callable (uint32_t tick) -> const dAttackMachineSimulation::PlayerInput*
//   Must return nullptr for ticks outside the history window.
//
// Each MotionStep is a waypoint: a frame matches it if
//   angularDistance(stickAngle, step.targetAngle) <= step.angleTolerance.
// Steps are matched newest-to-oldest. The newest step must lie within
// cmd.windowAfterFinalStep frames of currentTick; each earlier step must lie
// within its own step.maxGapFrames of the next-newer matched frame.
//
// Returns the actionId of the longest-matching command, or kNoMatch.
// Tie-break: more steps wins.
//
// Pure — no globals, no I/O, no allocations.
// O(motions × steps × historyWindow) worst case.
// ---------------------------------------------------------------------------
template <typename HistoryAccessor>
uint32_t matchSequence(
    HistoryAccessor&&                  inputAt,
    uint32_t                           currentTick,
    glm::vec2                          currentStick,
    glm::vec3                          currentReferenceForwardXY,
    uint8_t                            currentButtonsHeld,
    uint8_t                            currentButtonsEdge,
    float                              deadzone,
    const std::vector<MotionCommand>&  defs)
{
    (void)currentStick;
    (void)currentReferenceForwardXY;
    (void)currentButtonsHeld;

    uint32_t bestActionId  = kNoMatch;
    int      bestStepCount = -1;

    const int historyFloor = static_cast<int>(currentTick)
                             - static_cast<int>(kHistoryWindowFrames);

    for (const MotionCommand& cmd : defs)
    {
        if (cmd.steps.empty())
            continue;

        if ((currentButtonsEdge & cmd.requiredButtonEdge) != cmd.requiredButtonEdge)
            continue;

        const int numSteps = static_cast<int>(cmd.steps.size());

        // prevBound is the exclusive upper tick bound for the next step search.
        // Starts at currentTick (the button-press frame).
        int  prevBound = static_cast<int>(currentTick);
        bool matched   = true;

        for (int stepIdx = numSteps - 1; stepIdx >= 0; --stepIdx)
        {
            const MotionStep& step = cmd.steps[stepIdx];

            const int gap = (stepIdx == numSteps - 1)
                            ? static_cast<int>(cmd.windowAfterFinalStep)
                            : static_cast<int>(step.maxGapFrames);

            const int searchHi  = prevBound - 1;
            const int searchLo  = prevBound - 1 - gap;
            const int clampedLo = (searchLo > historyFloor) ? searchLo : historyFloor;

            bool stepFound = false;
            for (int t = searchHi; t >= clampedLo; --t)
            {
                const dAttackMachineSimulation::PlayerInput* entry =
                    inputAt(static_cast<uint32_t>(t));
                if (!entry)
                    continue;

                const auto angleOpt = aimRelativeAngle(
                    glm::vec2(entry->moveDirectionWorld.x, entry->moveDirectionWorld.y),
                    glm::vec3(entry->aimDirection.x, entry->aimDirection.y, 0.f),
                    deadzone);
                if (!angleOpt)
                    continue;

                if (angularDistance(*angleOpt, step.targetAngle) > step.angleTolerance)
                    continue;

                prevBound = t;
                stepFound = true;
                break;
            }

            if (!stepFound)
            {
                matched = false;
                break;
            }
        }

        if (matched && numSteps > bestStepCount)
        {
            bestStepCount = numSteps;
            bestActionId  = cmd.actionId;
        }
    }

    return bestActionId;
}

} // namespace inputSequence

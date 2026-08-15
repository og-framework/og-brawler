#pragma once
// SPDX-License-Identifier: BUSL-1.1

// Pure, engine-agnostic SOURCING for the motion-sequence matcher (the Hadouken
// detector). Sibling of BrawlerInputPackaging.h and built on the same seam: the
// LIVE READ stays UE-side on UOGBrawlerInputCollectionComponent, everything that
// can be expressed over plain data lives here.
//
// WHAT THIS HEADER OWNS (og-netcode-v2-input-relay T15, AM-1):
//   * motionButtonMask   — the (attackLeft, attackRight) -> 2-bit held mask.
//   * motionButtonEdge   — the RISING-EDGE mask: held & ~heldAtPreviousTick,
//                          with "previous tick absent" meaning "treat the whole
//                          held mask as an edge" (cold start).
//   * resolveTriggeredActionId — held/edge derivation + the matchSequence call,
//                          i.e. the entire matcher invocation.
//   * DelayLineMotionHistory — the production History adapter over
//                          ClientInputDelayLine.
//
// WHY IT WAS EXTRACTED. All of the above used to live inline in
// UOGBrawlerInputCollectionComponent::buildPlayerInput, a method on a
// UActorComponent. Source/OGBrawlerTests links { Core, OGSimulation, OGBrawler }
// and NOT OGBrawlerUnreal, so no Catch2 test could reach that code at all — the
// only testable shape was a test-local re-implementation compared against
// another test-local re-implementation, which proves nothing about production.
// Templating on the history source (same trick readContinuousInputFields uses
// for the continuous read) makes the REAL function reachable from a tree that
// cannot link UE.
//
// ---------------------------------------------------------------------------
// THE `History` SHAPE (structural, deliberately not a C++ concept — matching the
// readContinuousInputFields precedent):
//
//     const dAttackMachineSimulation::PlayerInput* at(uint32_t tick) const;
//
// It MUST return nullptr for any tick outside the retained window. That is
// matchSequence's contract (InputSequence.h), and it is why the production
// adapter below is has()-gated rather than a bare at() forward — see
// DelayLineMotionHistory.
//
// ---------------------------------------------------------------------------
// WHICH HISTORY, AND WHY IT CHANGED (T15).
//
// The matcher semantically needs the client's RAW CAPTURE history contiguous to
// `currentTick - 1`. It used to read the correction cache's input column
// instead, which stores the APPLIED input keyed by APPLICATION tick — under an
// input delay `d`, slot `t` holds `capture(t - d)`. Every history read was
// therefore displaced by `d`.
//
// THE SHIFT IS UNIFORM, and that bounds the defect precisely: inter-step gap
// matching (`MotionStep::maxGapFrames`) compares two history reads to each
// other, so a uniform displacement cancels and gap matching was NEVER affected.
// The only quantity `d` corrupts is the distance from the newest matched step to
// `currentTick` — `MotionCommand::windowAfterFinalStep`. Concretely the matcher
// could not see captures `T-d .. T-1` at all, so the player had to reach the
// final motion direction `d` frames earlier than the design intends (and, in the
// edge computation, "previously held" was read from `capture(T-1-d)`, leaving a
// rising edge open for `d+1` consecutive ticks).
//
// ClientInputDelayLine is capture-tick-keyed, so reading it gives the matcher
// exactly the raw contiguous history it was specified against, at every `d`.
//
// AND THE SEAM CANNOT BE BRIDGED FROM THE LIVE SIDE — do not try. matchSequence
// `(void)`-casts currentStick, currentReferenceForwardXY AND currentButtonsHeld
// (InputSequence.h). Only currentButtonsEdge and currentTick are used. The
// current frame is therefore not considered by the match AT ALL, at any `d`, so
// the continuous fields this function forwards are carried for the signature's
// sake rather than consumed. Making the current frame count would be a change to
// matchSequence's contract, not a fix at this layer.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <vector>

#include "glm/vec3.hpp"

#include "OGBrawler/BrawlerInputPackaging.h"
#include "OGBrawler/InputSequence/InputSequence.h"
#include "OGBrawler/SimulatableBrawlerTypes.h"
#include "OGSimulation/Network/ClientInputDelayLine.h"

namespace simulatableBrawler
{

// ---------------------------------------------------------------------------
// AM-8 — CAPACITY COUPLING PIN.
//
// matchSequence hard-floors its search at `currentTick - kHistoryWindowFrames`,
// so that constant IS the matcher's deepest possible reach: no MotionCommand,
// however long its step chain or however generous its gaps, can read older.
//
// The delay line must therefore retain at least that many ticks, or the matcher
// silently loses the tail of its window — a motion would stop matching with no
// error anywhere. The margin below is `MotionStep::maxGapFrames` (8, the largest
// inter-step gap in the shipped definitions): one full extra gap of headroom, so
// a capacity cut has to be egregious before it can bite.
//
// THE MARGIN IS INDEPENDENT OF THE INPUT DELAY `d` (and of relayDelayFloorTicks).
// The line is keyed by CAPTURE tick, so the delay shifts WHICH tick
// collectInputAll reads for the applied value; it never changes which ticks are
// resident. This pin therefore holds unchanged at floor 0, at the T9 scenario-4
// floor of 8, and at the hard cap.
// ---------------------------------------------------------------------------
inline constexpr std::size_t kMotionMatcherDeepestReachTicks =
    static_cast<std::size_t>(inputSequence::kHistoryWindowFrames);

inline constexpr std::size_t kMotionMatcherResidencyMarginTicks = 8u;

static_assert(kClientInputDelayLineCapacityTicks
                  >= kMotionMatcherDeepestReachTicks + kMotionMatcherResidencyMarginTicks,
              "ClientInputDelayLine is now too short to serve the motion matcher's history "
              "window: matchSequence reaches inputSequence::kHistoryWindowFrames ticks back, "
              "and a line shorter than that (plus margin) truncates the window silently "
              "instead of failing. Raise kClientInputDelayLineCapacityTicks, or lower "
              "kHistoryWindowFrames deliberately.");

// The 2-bit held mask matchSequence's MotionCommand::requiredButtonEdge is
// specified against: bit 0 = attackLeft, bit 1 = attackRight.
inline uint8_t motionButtonMask(bool leftAttack, bool rightAttack)
{
    return static_cast<uint8_t>((leftAttack ? 0b01 : 0) | (rightAttack ? 0b10 : 0));
}

// Rising-edge mask: currently held & ~held at the PREVIOUS tick.
//
// `currentButtonsHeld` is the LIVE sample for `currentTick` and is passed in
// rather than read from history: the current tick's capture is not in the
// history source yet when this runs (the provider is called before the push),
// and that ordering is what keeps the edge honest.
//
// Two cases answer "the whole held mask is an edge": tick 0 (there is no
// previous tick) and an absent previous entry (cold start / post-resync
// window). Both are the pre-T15 behaviour, preserved verbatim.
template <typename History>
uint8_t motionButtonEdge(const History& history, uint32_t currentTick, uint8_t currentButtonsHeld)
{
    if (currentTick == 0u)
    {
        return currentButtonsHeld;
    }

    const dAttackMachineSimulation::PlayerInput* prev = history.at(currentTick - 1u);
    if (prev == nullptr)
    {
        return currentButtonsHeld;
    }

    const uint8_t prevHeld = motionButtonMask(prev->attackLeft, prev->attackRight);
    return static_cast<uint8_t>(currentButtonsHeld & ~prevHeld);
}

// THE matcher invocation, in one place.
//
// Returns inputSequence::kNoMatch when nothing matches — including when the
// history is entirely empty, which is what makes it safe to call
// unconditionally. (The pre-T15 code guarded the whole block on "is there a
// correction cache"; there is no equivalent guard here because the delay line's
// existence is co-extensive with this function's only production call site.)
template <typename History>
uint32_t resolveTriggeredActionId(const History&                             history,
                                  uint32_t                                   currentTick,
                                  const ContinuousInputFields&               fields,
                                  bool                                       leftAttack,
                                  bool                                       rightAttack,
                                  float                                      deadzone,
                                  const std::vector<inputSequence::MotionCommand>& defs)
{
    const uint8_t currentButtonsHeld = motionButtonMask(leftAttack, rightAttack);
    const uint8_t currentButtonsEdge = motionButtonEdge(history, currentTick, currentButtonsHeld);

    return inputSequence::matchSequence(
        [&history](uint32_t tick) { return history.at(tick); },
        currentTick,
        fields.moveStick,
        glm::vec3(fields.aimDirection.x, fields.aimDirection.y, 0.f),
        currentButtonsHeld,
        currentButtonsEdge,
        deadzone,
        defs);
}

// ---------------------------------------------------------------------------
// DelayLineMotionHistory — THE production History adapter (T15 / AM-4).
//
// Two things it does that a bare forward would not:
//
//  1. IT IS has()-GATED. ClientInputDelayLine::at() answers with the NEUTRAL
//     input for a tick that was never captured — it never returns null. Handing
//     that straight to matchSequence would put a fabricated entry in the history
//     for every absent tick. Behaviourally the two are near-equivalent today
//     (the neutral's (0,0,1) aim makes aimRelativeAngle return nullopt, so the
//     matcher skips it anyway), but "absent" and "neutral capture" are different
//     facts and only the gate keeps them different in the data.
//
//  2. IT UNWRAPS THE COMPOSITE. The line stores the composite
//     simulatableBrawler::PlayerInput; matchSequence is specified over the
//     machine sub-input.
//
// TICK DOMAIN. matchSequence walks its search range as a signed int and casts to
// uint32_t at the call, so a range that runs below tick 0 arrives here as a very
// large unsigned value. Casting straight back to int32_t recovers the original
// negative tick (C++20 makes that conversion modular, not implementation-
// defined), and ClientInputDelayLine::has() answers false for every negative
// tick by contract. The pre-session window therefore reads as absent, which is
// exactly right.
// ---------------------------------------------------------------------------
class DelayLineMotionHistory
{
public:
    explicit DelayLineMotionHistory(const ClientInputDelayLine<PlayerInput>& line)
        : m_line(&line)
    {
    }

    const dAttackMachineSimulation::PlayerInput* at(uint32_t tick) const
    {
        const std::int32_t signedTick = static_cast<std::int32_t>(tick);
        if (!m_line->has(signedTick))
        {
            return nullptr;
        }
        return &m_line->at(signedTick).get<dAttackMachineSimulation::PlayerInput>();
    }

private:
    const ClientInputDelayLine<PlayerInput>* m_line;
};

} // namespace simulatableBrawler

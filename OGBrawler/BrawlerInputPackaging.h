#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerInputPackaging-rationale.md · docs/BrawlerInputPackaging-guards.md

#include <cstdint>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include "OGBrawler/SimulatableBrawlerTypes.h"
#include "OGBrawler/InputSequence/InputSequence.h"

namespace simulatableBrawler
{
// ⛔G-01  docs/BrawlerInputPackaging-guards.md
struct ContinuousInputFields
{
    // ⛔G-03  docs/BrawlerInputPackaging-guards.md
    glm::vec3 aimDirection      = dAttackRadialSimulation::PlayerInput::zero().aimDirection;
    glm::vec2 moveStick         = glm::vec2(0.f, 0.f);
    glm::vec3 moveDirectionWorld = glm::vec3(0.f, 0.f, 0.f);
};

static_assert(inputSequence::kNoMatch == 0u,
    "simulatableBrawler: inputSequence::kNoMatch must stay 0. A default-constructed "
    "ContinuousInputFields packs to getZeroPlayerInput() in four slices of five, and the MACHINE "
    "slice matches ONLY because kNoMatch happens to equal that slice's own defaulted "
    "triggeredActionId. Make kNoMatch non-zero and the machine slice diverges too, with no other "
    "diagnostic anywhere in the tree. Was guard G-02, now retired; rationale section 2.");

// ⛔G-04  docs/BrawlerInputPackaging-guards.md
template <typename Src>
ContinuousInputFields readContinuousInputFields(const Src& s)
{
    ContinuousInputFields fields;
    fields.aimDirection       = s.buildAimDirection();
    fields.moveStick          = s.getMoveStick();
    fields.moveDirectionWorld = s.buildMoveDirectionWorld();
    return fields;
}

struct InputFlagFields
{
    bool holdGuard = false;
};

// ⛔G-08  docs/BrawlerInputPackaging-guards.md
inline PlayerInput makeSimPlayerInput(const ContinuousInputFields& fields,
                                      bool leftAttack,
                                      bool rightAttack,
                                      uint32_t triggeredActionId,
                                      const InputFlagFields& flagFields)
{
    // ⛔G-09  docs/BrawlerInputPackaging-guards.md
    uint8_t flags = 0u;
    // ⛔G-10  docs/BrawlerInputPackaging-guards.md
    // ⛔G-12  docs/BrawlerInputPackaging-guards.md
    if (flagFields.holdGuard)
        flags = static_cast<uint8_t>(flags | brawlerMovementSimulation::kInputFlagHoldGuard);

    brawlerMovementSimulation::PlayerInput movementInput;
    movementInput.flags = flags;

    return PlayerInput(
        dAttackRadialSimulation::PlayerInput(fields.aimDirection, leftAttack, rightAttack),
        dAttackMachineSimulation::PlayerInput(fields.aimDirection, leftAttack, rightAttack,
                                              fields.moveStick, fields.moveDirectionWorld,
                                              triggeredActionId),
        dAttackGuardSimulation::PlayerInput(fields.aimDirection),
        brawlerProjectileSimulation::PlayerInput{fields.aimDirection},
        // ⛔G-13  docs/BrawlerInputPackaging-guards.md
        movementInput,
        brawlerRingout::PlayerInput{});
}

namespace detail
{
template <typename F>
inline constexpr bool kOmittedFlagsArgumentCompiles =
    requires(const F& f) { makeSimPlayerInput(f, false, false, 0u); };
template <typename F>
inline constexpr bool kBareBoolInFlagsSlotCompiles =
    requires(const F& f) { makeSimPlayerInput(f, false, false, 0u, true); };
template <typename F>
inline constexpr bool kTrailingBoolCompiles =
    requires(const F& f) { makeSimPlayerInput(f, false, false, 0u, InputFlagFields{}, false); };
template <typename F>
inline constexpr bool kShippedCallShapeCompiles =
    requires(const F& f) { makeSimPlayerInput(f, false, false, 0u, InputFlagFields{}); };
} // namespace detail

static_assert(detail::kShippedCallShapeCompiles<ContinuousInputFields>,
    "simulatableBrawler: VACUITY CONTROL for the three call-shape assertions below. All three are "
    "negative, so if makeSimPlayerInput were renamed or its first four parameters changed, all "
    "three would go vacuously true and stop guarding anything. This line fails first instead.");

static_assert(!detail::kOmittedFlagsArgumentCompiles<ContinuousInputFields>,
    "simulatableBrawler: the flags parameter of makeSimPlayerInput must have NO DEFAULT. Give it "
    "one and a caller can omit it, compile, run, and send a released guard forever - the "
    "silent-omission trap task 52 removed. The absence of a default is also why every call site "
    "spells the neutral explicitly as {}, so \"no flags\" stays a decision somebody wrote down. "
    "Was fences T1-5 and T1-4, guards G-06 and G-07, now retired. "
    "MakeSimPlayerInputFlagsTest.cpp pins the same call as ill-formed.");

static_assert(!detail::kBareBoolInFlagsSlotCompiles<ContinuousInputFields>,
    "simulatableBrawler: a bare bool must NOT be accepted in the flags slot. InputFlagFields is an "
    "aggregate with no converting constructor precisely so that a flag is set BY NAME "
    "({.holdGuard = true}) and transposing two flags is a compile error rather than a wrong bit. "
    "Was guard G-05, now retired.");

static_assert(!detail::kTrailingBoolCompiles<ContinuousInputFields>,
    "simulatableBrawler: NEVER append another trailing bool to makeSimPlayerInput. That was task "
    "14's shape, correct for exactly one flag; four more would have been four more "
    "silent-omission traps plus a five-bool positional call in which transposing two arguments "
    "type-checks. A new per-tick signal is a NAMED FIELD on InputFlagFields and a BIT in the "
    "flags byte. Was fence T1-2, guard G-11, now retired.");

// ⛔G-14  docs/BrawlerInputPackaging-guards.md
// ⛔G-15  docs/BrawlerInputPackaging-guards.md
inline PlayerInput makeVisualizationPlayerInput(const ContinuousInputFields& fields)
{
    return makeSimPlayerInput(fields,
                              /*leftAttack*/ false,
                              /*rightAttack*/ false,
                              inputSequence::kNoMatch,
                              InputFlagFields{});
}

} // namespace simulatableBrawler

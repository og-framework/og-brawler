#pragma once
// SPDX-License-Identifier: BUSL-1.1

// Pure, engine-agnostic assembly of a simulatableBrawler::PlayerInput from the
// CONTINUOUS input fields. No UE types, no live input sampling, no tick or cache
// state — this header holds only the field -> PlayerInput packing, so both the
// per-tick sim path and the render-rate visualization path can share one
// definition of "how continuous input becomes a PlayerInput".
//
// Seam (og-netcode-v2 T12 / D5.4):
//   * The LIVE READ stays UE-side, on UOGBrawlerInputCollectionComponent — it
//     reads Enhanced Input state, camera caches and mouse-aim line-plane
//     intersections, none of which belong in the engine-agnostic core.
//   * The PURE ASSEMBLY lives here. readContinuousInputFields is templated on
//     its source so the component satisfies it structurally, and a lightweight
//     test double can stand in for it in og-brawler-tests (a tree that cannot
//     link UE).
//
// Two packers, one continuous read:
//   makeSimPlayerInput           — the per-tick path. Discrete/edge-derived
//                                  fields (attack buttons, the holdGuard button,
//                                  and triggeredActionId from the tick-stateful
//                                  motion matcher) are passed in explicitly by
//                                  the caller.
//   makeVisualizationPlayerInput — the render-rate path. Continuous fields only;
//                                  every discrete field is left neutral, so a
//                                  discrete input edge structurally CANNOT
//                                  render-echo (proposal §2.3 continuous-vs-
//                                  discrete split, enforced at the data level
//                                  rather than by per-caller judgment).
//
// makeVisualizationPlayerInput is defined in terms of makeSimPlayerInput on
// purpose: the continuous packing has exactly one implementation, and the two
// paths can only ever differ in the discrete arguments.

#include <cstdint>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include "OGBrawler/SimulatableBrawlerTypes.h"
#include "OGBrawler/InputSequence/InputSequence.h"

namespace simulatableBrawler
{
// The subset of player input that varies continuously and is therefore safe to
// re-sample at render-frame rate.
//
// ⚠ [movement-sim task 17, from the task-22 review's N-2] THE DEFAULTS ARE THE NEUTRAL POSE OF
// EVERY SLICE BUT ONE, AND THE OLD SENTENCE CLAIMED ALL FIVE. `aimDirection` defaults to the
// radial sub-sim's own `zero().aimDirection` — (0,0,1) — which is also machine's and guard's
// neutral aim, and `moveStick`/`moveDirectionWorld` default to the zeroes those types carry.
// ⛔ BUT `brawlerProjectileSimulation::PlayerInput::zero()` IS `PlayerInput{}`: its shipped
// neutral aim is (0,0,0), not (0,0,1) (that type's own comment says so, and calls fixing it a
// WIRE CHANGE). So a default-constructed `ContinuousInputFields` packs to `getZeroPlayerInput()`
// in four slices of five and differs from it in the projectile slice's aim. Nothing depends on
// the equality — `getZeroPlayerInput()` is built by `SimulationComposite::zero()`, never from
// this struct — but the sentence that asserted it was simply false.
struct ContinuousInputFields
{
    // [movement-sim task 22] The neutral aim is spelled ONCE, at the radial sub-sim's
    // own zero(). Visible here transitively through SimulatableBrawlerTypes.h, which
    // includes DAttackRadialSimulation.h — no new include was needed.
    glm::vec3 aimDirection      = dAttackRadialSimulation::PlayerInput::zero().aimDirection;
    glm::vec2 moveStick         = glm::vec2(0.f, 0.f);
    glm::vec3 moveDirectionWorld = glm::vec3(0.f, 0.f, 0.f);
};

// Reads the continuous fields from any source exposing the input-collection
// accessor shape:
//     glm::vec3 buildAimDirection() const;
//     glm::vec2 getMoveStick() const;
//     glm::vec3 buildMoveDirectionWorld() const;
//
// Templated (not an interface) so it binds to UOGBrawlerInputCollectionComponent
// without dragging UObject into this header, and to a plain test double without
// dragging UE into the test tree. This function is the SINGLE source of truth for
// the continuous read: both buildPlayerInput and buildLatestVisualizationInput
// route through it, so the two cannot drift.
template <typename Src>
ContinuousInputFields readContinuousInputFields(const Src& s)
{
    ContinuousInputFields fields;
    fields.aimDirection       = s.buildAimDirection();
    fields.moveStick          = s.getMoveStick();
    fields.moveDirectionWorld = s.buildMoveDirectionWorld();
    return fields;
}

// ⭐ [movement-sim task 52] THE NAMED-FIELD MIRROR OF `brawlerMovementSimulation::PlayerInput::flags`.
// One `bool` field per bit, defaulted to that bit's NEUTRAL value (which is `false` for every
// bit, by the packing rule at the type: all bits clear IS `PlayerInput::zero()`).
//
// ⛔ WHY THIS TYPE EXISTS, AND WHAT IT REPLACED. Task 14 passed the one flag as a trailing
// DEFAULTED `bool holdGuard = false` — correct for one flag, and task 14's reviewer endorsed it
// as such. But wall-grab (task 20), jump (21), dash (31) and ski-tuck (48) each reserve a bit,
// and four more trailing defaulted bools would have been four more SILENT-OMISSION TRAPS, plus
// a five-bool positional call site in which transposing two arguments type-checks. Both hazards
// are structural, and both are gone here:
//   * a field is set BY NAME (`{.holdGuard = true}`), so no flag can be written positionally and
//     transposing two of them is a compile error, not a wrong bit;
//   * the parameter has NO DEFAULT, so a caller cannot omit it — see the fences in
//     Tests/InputPackaging/MakeSimPlayerInputFlagsTest.cpp, which pin BOTH the omitted-argument
//     call and the bare-`bool`-in-the-flags-slot call as ILL-FORMED rather than merely unwise.
// ⚠ The neutral is still spelled explicitly at every call site (`{}`), so "no flags" stays a
// decision somebody wrote down rather than something that happened by default.
struct InputFlagFields
{
    bool holdGuard = false;   // -> brawlerMovementSimulation::kInputFlagHoldGuard (bit 0)
};

// Per-tick packer. Discrete fields are supplied by the caller because they are
// derived from state this header deliberately does not have: the attack booleans and
// the flag fields come from live button state, and triggeredActionId comes from the
// tick-stateful motion-sequence matcher, which needs the input cache and the current tick.
//
// [movement-sim task 52] `flagFields` is REQUIRED. It was `bool holdGuard = false` until this
// task; the default is gone deliberately, so the neutral call reads `{}` and an omission does
// not compile. The ONE caller that models a real button press is the sim path,
// UOGBrawlerInputCollectionComponent::buildPlayerInput.
inline PlayerInput makeSimPlayerInput(const ContinuousInputFields& fields,
                                      bool leftAttack,
                                      bool rightAttack,
                                      uint32_t triggeredActionId,
                                      const InputFlagFields& flagFields)
{
    // [movement-sim task 14] ⭐ THE ONE AND ONLY WRITER of `kInputFlagHoldGuard`, anywhere
    // in the tree. Task 51 shipped the flags byte and its READER (step 1's `frozen` gate in
    // brawlerMovementSimulation::integrate) deliberately WITHOUT a writer, so until this line
    // existed the gate was inert and `frozen` could never come from input. This is what makes
    // holding guard actually freeze movement.
    //
    // ⛔ WRITTEN BY NAMING THE CONSTANT, NEVER BY THE BIT'S NUMERIC VALUE. Spelling the set bit
    // positionally compiles and is correct only because holdGuard happens to sit at bit 0 today,
    // and goes silently wrong the moment it moves — bits 1-7 are already spoken for (wall-grab,
    // jump, dash, ski-tuck), and the packing rule at the type is the only place that
    // ordering is written down.
    //
    // ⭐ [movement-sim task 52] ADDING A FLAG — THE WHOLE RECIPE, TWO LINES, NO SIGNATURE CHANGE:
    //   1. add a `bool <name> = false;` field to InputFlagFields above, beside holdGuard;
    //   2. add one `if (flagFields.<name>) flags |= kInputFlag<Name>;` line below.
    // Callers that do not model the signal need NO edit — `{}` already value-initializes it to
    // the neutral `false`. Callers that do, name it: `{.<name> = true}`. What you must NOT do is
    // append another trailing `bool`: that was task 14's shape, correct for exactly one flag,
    // and four more of them would have been four more silent-omission traps.
    //
    // The accumulator is `uint8_t` and each `|=` is written through an explicit
    // `static_cast<uint8_t>`: `flags | k` promotes to `int`, and assigning that back to a
    // `uint8_t` is a narrowing conversion the -Werror UE modules reject (C4244). Same reason
    // task 14's conditional spelled its zero arm `uint8_t{0}` rather than `0u`.
    uint8_t flags = 0u;
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
        // [movement-sim T1] Named here because the composite is POSITIONAL: this is the
        // single place in the whole tree where a simulatableBrawler::PlayerInput is
        // assembled from fields, so appending a slice costs exactly one line and NO UE
        // edit (both UE builders route through this function).
        // [movement-sim task 14] T1's "empty at the skeleton — the movement sub-sim
        // consumes no input yet" ENDED HERE: the slice now carries the input flags byte.
        movementInput);
}

// Render-rate packer. Continuous fields only; discrete fields pinned neutral —
// no attack buttons, no holdGuard, and triggeredActionId at inputSequence::kNoMatch
// (the motion matcher is never invoked on this path). The result is COSMETIC ONLY and
// must never be fed to the simulation or the input RPC.
inline PlayerInput makeVisualizationPlayerInput(const ContinuousInputFields& fields)
{
    return makeSimPlayerInput(fields,
                              /*leftAttack*/ false,
                              /*rightAttack*/ false,
                              inputSequence::kNoMatch,
                              // [movement-sim task 14] holdGuard is a BUTTON, so it is a
                              // DISCRETE field and is pinned neutral here by exactly the
                              // rule that pins the attack buttons: a discrete input edge
                              // structurally cannot render-echo.
                              // [movement-sim task 52] Spelled `{}` — the DEFAULTED-FIELD
                              // neutral, not an omission. The parameter has no default any
                              // more, so this brace is mandatory and the render-echo rule is
                              // stated at the one place it is applied. Every flag a future
                              // task adds is neutral here for free, because each flag's
                              // neutral value is `false` and `{}` value-initializes them all.
                              InputFlagFields{});
}

} // namespace simulatableBrawler

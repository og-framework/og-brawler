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
//                                  fields (attack buttons, triggeredActionId
//                                  from the tick-stateful motion matcher) are
//                                  passed in explicitly by the caller.
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
// re-sample at render-frame rate. Defaults match getZeroPlayerInput()'s neutral
// pose (aim +Z, no movement) so a default-constructed instance packs to the same
// neutral PlayerInput.
struct ContinuousInputFields
{
    glm::vec3 aimDirection      = glm::vec3(0.f, 0.f, 1.f);
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

// Per-tick packer. Discrete fields are supplied by the caller because they are
// derived from state this header deliberately does not have: the attack booleans
// come from live button state, and triggeredActionId comes from the tick-stateful
// motion-sequence matcher, which needs the input cache and the current tick.
inline PlayerInput makeSimPlayerInput(const ContinuousInputFields& fields,
                                      bool leftAttack,
                                      bool rightAttack,
                                      uint32_t triggeredActionId)
{
    return PlayerInput(
        dAttackRadialSimulation::PlayerInput(fields.aimDirection, leftAttack, rightAttack),
        dAttackMachineSimulation::PlayerInput(fields.aimDirection, leftAttack, rightAttack,
                                              fields.moveStick, fields.moveDirectionWorld,
                                              triggeredActionId),
        dAttackGuardSimulation::PlayerInput(fields.aimDirection),
        brawlerProjectileSimulation::PlayerInput{fields.aimDirection});
}

// Render-rate packer. Continuous fields only; discrete fields pinned neutral —
// no attack buttons, and triggeredActionId at inputSequence::kNoMatch (the motion
// matcher is never invoked on this path). The result is COSMETIC ONLY and must
// never be fed to the simulation or the input RPC.
inline PlayerInput makeVisualizationPlayerInput(const ContinuousInputFields& fields)
{
    return makeSimPlayerInput(fields,
                              /*leftAttack*/ false,
                              /*rightAttack*/ false,
                              inputSequence::kNoMatch);
}

} // namespace simulatableBrawler

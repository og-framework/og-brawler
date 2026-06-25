#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGTypes.h"  // for BodyId

// Minimal stub header for the future brawler movement sub-sim.
// Today this file carries ONLY CharacterBindings — the per-character handle
// consumed by dAttackMachineSimulation (T33).
//
// FUTURE — character-movement sub-sim (planned, not yet filed):
// When the planned character-movement sub-sim lands (modeled on the
// radial/guard/projectile patterns — own PhysicsDeclaration with
// descriptor()/name/attachmentOffset/bodyStateOf, own RuntimeBindings,
// own integrate(), own State/InitialConditions/PlayerInput), it grows
// into this file alongside CharacterBindings — analogous to how
// brawlerProjectileSimulation::{State, PhysicsDeclaration, ...} all
// live in BrawlerProjectileSimulation.h today. At that point the
// capsule body becomes a regular sub-sim body created via the
// SimulationManagerUImpl::tryRegister forEach factory pass, and
// capsuleBodyId is sourced from this sub-sim's bindings.ownBodyId
// rather than the UE ACharacter::GetCapsuleComponent lookup that
// populates it today. T34 (if filed) then completes the migration by
// replacing every other sub-sim's bare parentBodyId with a
// CharacterBindings field, eliminating the duplication.
namespace brawlerMovementSimulation
{
    struct CharacterBindings
    {
        BodyId capsuleBodyId;  // character's main physics capsule
    };
}

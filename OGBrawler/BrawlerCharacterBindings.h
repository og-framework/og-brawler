#pragma once
// SPDX-License-Identifier: BUSL-1.1

// ⚠ `BodyId` comes from `OGSimulation/BodyId.h`, NOT `OGSimulation/OGTypes.h` — measured at
// task 62. (`BrawlerMovementSimulation.h`, where this struct used to live, carries an
// `OGTypes.h  // for BodyId` comment that has never been true; it got the type transitively.)
// Keep this the ONLY include this header carries — see the leaf rule below.
#include "OGSimulation/BodyId.h"

// ⭐ [movement-sim task 62] THE LEAF HEADER THAT KEEPS THE MOVEMENT/MACHINE DEPENDENCY
// POINTING ONE WAY.
//
// `CharacterBindings` used to live in `BrawlerMovementSimulation.h`, which forced
// `DAttackMachineSimulation.h` to include that whole header for ONE parameter type
// (`integrate3`'s `characterBindings`). That was harmless while movement was a skeleton. It
// stopped being harmless when movement began reading the machine's `State` (the flinch freeze)
// and `PlayerInput` (the move stick): the graph became a genuine CYCLE, and with `#pragma once`
// a cycle does not error — it silently leaves one side incomplete depending on which header the
// translation unit entered from. The workaround was to template both movement functions so the
// member accesses became dependent names. Both templates are gone now.
//
// The arrangement this header buys — read `A -> B` as "A includes B":
//     BrawlerMovementSimulation.h -> DAttackMachineSimulation.h -> BrawlerCharacterBindings.h
//     BrawlerMovementSimulation.h -------------------------------> BrawlerCharacterBindings.h
// and nothing points back: nothing reachable from `DAttackMachineSimulation.h` includes
// `BrawlerMovementSimulation.h`, directly or through any of its other includes. That is the
// invariant to protect.
//
// ⚠ [movement-sim task 64] That sentence used to name the wrong header — it read
// "`DAttackMachineSimulation.h` reaches neither of the other two", which contradicts the
// diagram two lines above: the machine header includes THIS file, directly, at
// `DAttackMachineSimulation.h:17`. The invariant is the one already stated in
// `BrawlerMovementSimulation.h` above its machine include ("KEEP THE GRAPH ACYCLIC: nothing
// reachable from `DAttackMachineSimulation.h` may include `BrawlerMovementSimulation.h`") and
// in `DAttackMachineSimulation.h` above `integrate3` ("THIS HEADER MUST NEVER INCLUDE
// `BrawlerMovementSimulation.h`, directly or through any of its other includes"). All three
// now say the same thing.
//
// ⛔ KEEP THIS A LEAF. `BodyId` is its only dependency, and that is the whole reason both
// sides can include it. Anything that needs more than `BodyId` does not belong here.
//
// ⭐ [movement-sim task 64] THE NAMESPACE IS `simulatableBrawler`, AND THAT IS THE WHOLE TASK.
// Task 62 left it as `brawlerMovementSimulation` on purpose, so the extraction would churn zero
// call sites. But that name was never a modelling decision. T35 (`OGBrawlerHadouken`,
// 2026-06-25) moved this struct OUT of `simulatableBrawler` and into a then-new
// `BrawlerMovementSimulation.h` "pre-staked as the eventual home of the planned
// character-movement sub-sim", and renamed the namespace to match the file it had landed in.
// ⭐ THE FILE MOVE EXISTED TO DODGE AN INCLUDE CYCLE — the cycle task 62 deleted. So the old
// namespace recorded where the struct LIVED, not what it IS.
//
// ⭐ AND MOVEMENT IS THE ONE SUB-SIM THAT NEVER USES THIS TYPE: every `CharacterBindings`
// mention left in `BrawlerMovementSimulation.h` is a comment, and movement takes its capsule id
// from its own `RuntimeBindings`, an alias of the generic `PhysicsRuntimeBindings`. The real
// consumers — the machine sub-sim's `integrate3`, the `SimulatableBrawler` composite,
// `BrawlerHitRoutingSystem.h` through the accessor, and the tests — all sit in or under
// `simulatableBrawler`, which is where the type started and where it belongs.
//
// ⛔ A NAMESPACE NEEDS NO INCLUDE. This header still includes only `OGSimulation/BodyId.h`,
// still compiles standalone, and no wire byte moved: nothing specializes `SerializableFields`
// on `CharacterBindings`, so it is not on the wire at all.
namespace simulatableBrawler
{

// [Task 35] The per-character handle consumed by dAttackMachineSimulation (T33).
//
// Populated ONCE at registration time, in `ASimulationManagerUImpl::tryRegister`, from THIS
// sub-simulation's own `PhysicsDeclaration` bindings: `bindings.ownBodyId`, read after the
// physics-creation fold has run (before it, that id is still zero).
//
// ⭐ [movement-sim task 13] THAT CUTOVER IS DONE. The source used to be the UE
// `ACharacter::GetCapsuleComponent` lookup; it is not any more. The VALUE never moved, because
// task 11's descriptor sets `isRoot` — the factory ADOPTS the character's capsule rather than
// creating anything, so `bindings.ownBodyId == bindings.parentBodyId == capsuleBodyId` holds by
// construction. It was a change of PROVENANCE only (architecture §4.2).
//
// ⭐ [movement-sim task 17] AND THE TWO-SOURCE TRIPWIRE THAT WATCHED THEM AGREE IS
// GONE, together with `PendingRegistration::parentBodyId`, because there is no second source
// left to disagree. The identity itself is still asserted one layer down, by
// `ChaosPhysicsFactory`'s adopt-root `checkf`.
//
// FUTURE — THE FACT IS STILL TRUE. Radial (`DAttackRadialSimulation.h:680`), guard
// (`DAttackGuardSimulation.h:257`) and projectile (`BrawlerProjectileSimulation.h:474`) still
// read a bare `bindings.parentBodyId`, while the machine sub-sim takes a `CharacterBindings`.
// Folding a bindings field into every sub-sim's `RuntimeBindings` would remove that duplication.
//
// ⛔ [movement-sim task 64] BUT THERE IS NO INITIATIVE TO POINT AT, AND THIS COMMENT USED TO
// CLAIM THERE WAS. The only specification is one prose paragraph, at
// `Initiatives/OGBrawlerHadouken/Backlog.md:965`, literally headed "T34 (if filed later)" — and
// it never was filed: there is no `### 34.` entry in that backlog, every other mention tags it
// "T34 parked", and its host initiative is itself parked at "PHASE 2 COMPLETE — AWAITING USER
// APPROVAL FOR PHASE 3". It is an UNFILED, PARKED task, not scheduled work.
//
// ⚠ AND ITS PREMISE HAS WEAKENED — both facts are in the two paragraphs directly above.
// `PendingRegistration::parentBodyId` is gone (task 17) and the two-source tripwire with it, and
// `ownBodyId == parentBodyId == capsuleBodyId` now holds BY CONSTRUCTION (task 13), asserted one
// layer down by `ChaosPhysicsFactory`'s adopt-root `checkf`. T34 was written to de-duplicate two
// genuinely independent sources. What it would buy today is naming consistency across four
// sub-sims for ~100+ LOC and no behaviour change — a materially weaker case.
struct CharacterBindings
{
    BodyId capsuleBodyId;  // character's main physics capsule
};

}

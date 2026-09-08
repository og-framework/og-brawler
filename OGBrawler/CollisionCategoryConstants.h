#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/QueryGeometry.h"

// Collision category assignments — DAttack-local sequential IDs.
// These are NOT engine channel numbers. Each adapter holds an explicit
// mapping from these IDs to the engine's native channel/layer type.
namespace collisionCategory
{
	constexpr uint32_t body  = 0;   // character hurtbox (only)
	constexpr uint32_t guard = 1;   // guard shield
	constexpr uint32_t queryRouting = 2;   // trace-channel routing for query volumes
	// [hit-resolution T13] In-flight projectile body. Own category so a projectile
	// overlap query can distinguish "hit another projectile" (should cancel both,
	// no HitFlinch on either owning character) from "hit a character body/guard"
	// (existing hit / guard-block behavior). Before this category, projectiles were
	// registered under `body`, so projectile-vs-projectile hits produced a routed
	// HitFlinch on the opposing owner via the T3/T11 rootBodyId lookup.
	constexpr uint32_t projectile = 3;   // in-flight projectile body
	// [movement-sim T6] Static level geometry. The movement sub-sim's ground/wall
	// probe and its capsule sweeps search this category; no DAttack-authored shape
	// belongs to it.
	// ⭐ [movement-sim task 17] THE MAPPING EXISTS. Task 39 mapped it to the native
	// WorldStatic object type at BOTH ChaosCategoryMapping tables in
	// SimulationManagerUImpl.cpp's BeginPlay (authority branch and client branch).
	// ⚠ ECC_WorldStatic is ECollisionChannel(0), which is ALSO the unmapped fallback,
	// so the return value cannot tell you the mapping is there — only the table's size
	// can. That caveat is spelled out in full at the authority table.
	constexpr uint32_t world = 4;   // static level geometry (engine: WorldStatic object type)
	// [movement-sim T1] the character movement sub-sim's body; NOT the hurtbox.
	//
	// ⭐ [movement-sim task 17] THIS CATEGORY IS LIVE, AND THE THREE SENTENCES THAT SAID
	// OTHERWISE ARE GONE. The old text said `character` was "deliberately UNMAPPED in the
	// engine adapter until the movement sim goes live" and that the channel note below was
	// "documentation only". All three halves of that have landed:
	//   * task 43 MAPPED it — `character -> ECC_GameTraceChannel6` at both
	//     ChaosCategoryMapping tables in SimulationManagerUImpl.cpp's BeginPlay;
	//   * task 53 DECLARED the channel — one `+DefaultChannelResponses=(...)` row in
	//     Config/DefaultEngine.ini: Channel=ECC_GameTraceChannel6, DefaultResponse=ECR_Block,
	//     Name="BrawlerCharacter";
	//   * task 11 took the movement sim LIVE — its PhysicsDeclaration ships in the
	//     SimulatableBrawler composite and runs for every character in every session.
	// What has NOT changed is the half that made the old sentence worth writing: no attack
	// or projectile query searches this category, so a `character` shape is still invisible
	// to every mask below. That is now a property of the MASKS, not of a missing mapping.
	//
	// [movement-sim T6] WHY THE CAPSULE IS NOT `body` (architecture §4.3):
	// `body` is the radial sim's 30 cm hurtbox sphere, searched by the
	// `bodyAndGuard` / `bodyGuardProjectile` masks below. If the movement capsule
	// were registered as `body`, every radial swing and every projectile would
	// additionally overlap the capsule and emit a SECOND hit carrying the same
	// `rootBodyId` — harmless for routing (the flinch flag is idempotent), but it
	// changes the contents of `attackHits[]` and the block-vs-hit classification in
	// the projectile sim, which tests `guard` first and treats everything else as
	// damage. A separate `character` category keeps the capsule invisible to every
	// existing query mask — which is also why neither mask added below is folded
	// into an existing one.
	//
	// Engine channel — user ruling #6, closed 2026-09-04: `ECC_GameTraceChannel6`,
	// verified free (ch1 is `Damageable` in DefaultEngine.ini; ch2-5 are
	// body / guard / queryRouting / projectile). Both halves are DONE: task 43 wrote the
	// adapter-side mapping, task 53 declared the channel in DefaultEngine.ini.
	constexpr uint32_t character = 5;

	// Pre-built masks for common query patterns
	constexpr CollisionCategories bodyAndGuard =
		CollisionCategories::single(body) | CollisionCategories::single(guard);
	// Projectile-sim query mask: character hurtbox + guard shield + other projectiles
	// (so projectiles detect each other and both cancel — see T13 branch in the
	// projectile sim's hit loop).
	constexpr CollisionCategories bodyGuardProjectile =
		bodyAndGuard | CollisionCategories::single(projectile);

	// [movement-sim T6] Movement sub-sim sweep masks. Deliberately NOT folded into
	// `bodyAndGuard` or `bodyGuardProjectile` above — see the `character` note.
	// Ground / wall probe: static level geometry only.
	constexpr CollisionCategories worldOnly = CollisionCategories::single(world);
	// The movement capsule's blocking set (user ruling #5, closed 2026-09-03:
	// brawler-vs-brawler is BLOCK — the solver separates, mass-ratio 50/50 and the
	// ride-up risk accepted, authored pushbox parked). ⭐ [movement-sim task 17] THE
	// CONSUMER EXISTS: task 11's `brawlerMovementSimulation::PhysicsSetup::body` shape
	// descriptor carries this mask.
	constexpr CollisionCategories worldAndCharacter =
		worldOnly | CollisionCategories::single(character);
}

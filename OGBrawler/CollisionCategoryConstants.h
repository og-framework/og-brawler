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

	// Pre-built masks for common query patterns
	constexpr CollisionCategories bodyAndGuard =
		CollisionCategories::single(body) | CollisionCategories::single(guard);
	// Projectile-sim query mask: character hurtbox + guard shield + other projectiles
	// (so projectiles detect each other and both cancel — see T13 branch in the
	// projectile sim's hit loop).
	constexpr CollisionCategories bodyGuardProjectile =
		bodyAndGuard | CollisionCategories::single(projectile);
}

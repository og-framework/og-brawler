#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/CollisionCategoryConstants-rationale.md · docs/CollisionCategoryConstants-guards.md

#include "OGSimulation/QueryGeometry.h"

// ⛔G-01  docs/CollisionCategoryConstants-guards.md
namespace collisionCategory
{
	constexpr uint32_t body  = 0;
	constexpr uint32_t guard = 1;
	constexpr uint32_t queryRouting = 2;
	constexpr uint32_t projectile = 3;
	// ⛔G-02  docs/CollisionCategoryConstants-guards.md
	// ⛔G-03  docs/CollisionCategoryConstants-guards.md
	constexpr uint32_t world = 4;
	// ⛔G-05  docs/CollisionCategoryConstants-guards.md
	// ⛔G-06  docs/CollisionCategoryConstants-guards.md
	constexpr uint32_t character = 5;

	constexpr CollisionCategories bodyAndGuard =
		CollisionCategories::single(body) | CollisionCategories::single(guard);
	constexpr CollisionCategories bodyGuardProjectile =
		bodyAndGuard | CollisionCategories::single(projectile);

	static_assert(!bodyAndGuard.contains(character) && !bodyGuardProjectile.contains(character),
		"collisionCategory: `character` must stay OUT of the attack masks. Folding it in puts "
		"the movement capsule in every radial swing and projectile query, emitting a SECOND hit "
		"per swing with the same rootBodyId - which changes attackHits[] and the block-vs-hit "
		"classification in the projectile sim. Was fence T2a-2, guard G-04, now retired.");

	constexpr CollisionCategories worldOnly = CollisionCategories::single(world);
	constexpr CollisionCategories worldAndCharacter =
		worldOnly | CollisionCategories::single(character);

	static_assert((bodyGuardProjectile.bits & worldAndCharacter.bits) == 0u
		&& (bodyAndGuard.bits & ~bodyGuardProjectile.bits) == 0u
		&& (worldOnly.bits & ~worldAndCharacter.bits) == 0u,
		"collisionCategory: the movement sweep masks are deliberately NOT folded into "
		"bodyAndGuard / bodyGuardProjectile and must share no bit with them. Consolidating the "
		"four constants into two puts the hurtbox, guard shield or projectile body in the "
		"ground probe, or static level geometry in an attack query. Was fence T2a-4, guard "
		"G-07, now retired.");
}

#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/QueryGeometry.h"

// Collision category assignments — DAttack-local sequential IDs.
// These are NOT engine channel numbers. Each adapter holds an explicit
// mapping from these IDs to the engine's native channel/layer type.
namespace collisionCategory
{
	constexpr uint32_t body  = 0;   // damageable body (weapon body, character hurtbox)
	constexpr uint32_t guard = 1;   // guard shield
	constexpr uint32_t queryRouting = 2;   // trace-channel routing for query volumes

	// Pre-built masks for common query patterns
	constexpr CollisionCategories bodyAndGuard =
		CollisionCategories::single(body) | CollisionCategories::single(guard);
}

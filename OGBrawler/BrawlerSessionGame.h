#pragma once
// SPDX-License-Identifier: BUSL-1.1
// ../../../../../../Source/OGBrawlerUnreal/docs/BrawlerSessionGameUImpl-rationale.md ·
// ../../../../../../Source/OGBrawlerUnreal/docs/BrawlerSessionGameUImpl-guards.md

#include "OGSimulation/BodyId.h"
#include "OGSimulation/CompilerControl.h"
#include "OGBrawler/SimulatableBrawler.h"

struct BrawlerSessionGame
{
	static simulatableBrawler::PlayerInput neutralInput();

	struct AuthorityPresentationSlice
	{
		uint32 ringoutScore  = 0u;
		bool   hasScoreEntry = false;
	};

	static BodyId rootBodyId(const SimulatableBrawler& simulatable);
};

OGSIM_OPTIMIZE_OFF

inline simulatableBrawler::PlayerInput BrawlerSessionGame::neutralInput()
{
	// ⛔G-01  docs/BrawlerSessionGameUImpl-guards.md
	return simulatableBrawler::getZeroPlayerInput();
}

inline BodyId BrawlerSessionGame::rootBodyId(const SimulatableBrawler& simulatable)
{
    return simulatable.getPhysicsComposite()
        .get<brawlerMovementSimulation::PhysicsDeclaration>().bindings.ownBodyId;
}

OGSIM_OPTIMIZE_ON

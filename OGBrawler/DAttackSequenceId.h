#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <limits>

// Shared attack-sequence-ID value domain for the brawler attack system.
//
// VALUE DOMAIN:
// An "attack sequence id" is an unsigned int that identifies which radial swing a
// character is performing. Real ids are the contiguous range 0..N-1 and index directly
// into staticData.getAttackSequences()[]. Two reserved sentinels sit at the very top of
// the unsigned range so they can never collide with a real index:
//   - InvalidAttackSequenceId    (UINT_MAX)     — "no attack active / idle".
//   - kHadoukenSequenceSentinel  (UINT_MAX - 1) — "a Hadouken projectile owns the attack".
//
// WHO WRITES / READS:
//   - dAttackMachineSimulation::integrate3 WRITES kHadoukenSequenceSentinel into the radial
//     InitialConditions::activeAttackSequence when a Hadouken fires (Idle -> Attacking), and
//     writes InvalidAttackSequenceId when no swing is queued.
//   - dAttackRadialSimulation::integrate CONSUMES the id: it early-returns on the sentinel
//     (the projectile owns the attack; the weapon stays idle) instead of indexing
//     attackSequences[] out of bounds.
//   - The visualization layers (DAttackRadialVisualization / DAttackAimVisualization) GATE
//     every sequence-table lookup through isRealAttackSequence(id) — both sentinels are
//     wildly out of bounds, so an un-gated index is a ~4-billion-entry OOB read (the T24
//     PIE crash).
//
// WHY THIS HEADER EXISTS:
// These constants belong to a value domain owned by neither sub-sim — the machine writes
// ids, the radial sim consumes them, the viz layers gate on them. They previously lived at
// file scope inside DAttackRadialSimulation.h purely so the machine sim could share the same
// integer domain "without a circular include — the machine includes this header" (the old
// in-line comment admitted the awkwardness). T35 cut the equivalent knot for CharacterBindings
// by relocating it to its own minimal header; T36 does the same here. This header depends on
// nothing but <limits>, so any sub-sim or viz layer can include it directly without dragging
// in the radial sim's transitive surface, and the dependency survives future include-chain
// reshuffles.
//
// FUTURE — tagged-union refactor (T24 "Open architectural note"):
// Today there is exactly one motion-special sentinel (Hadouken). If the count of
// motion-special attacks grows past 1, the flat "reserved values at the top of the unsigned
// range + a predicate" scheme becomes unwieldy. The natural successor is a tagged union /
// small enum-tagged struct for the attack-sequence id (real-index vs which-motion-special),
// and this header is the right home / anchor for that migration.

static constexpr unsigned int InvalidAttackSequenceId = std::numeric_limits<unsigned int>::max();

static constexpr unsigned int kHadoukenSequenceSentinel = std::numeric_limits<unsigned int>::max() - 1;

// True only for a real radial-swing sequence index — i.e. an id that is safe to use to
// index staticData.getAttackSequences()[]. Both reserved sentinels (InvalidAttackSequenceId
// and kHadoukenSequenceSentinel) are wildly out of bounds, so every consumer that indexes
// the sequence table (the sim and all viz layers) must gate the lookup through this helper.
inline bool isRealAttackSequence(unsigned int id)
{
	return id != InvalidAttackSequenceId
		&& id != kHadoukenSequenceSentinel;
}

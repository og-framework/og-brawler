#pragma once
// SPDX-License-Identifier: BUSL-1.1

// The client's effective input delay, split into the arms that produced it.
//
//   * Pure. Scalars and enums only -- no engine type, no canvas, no colour, no layout.
//   * A SIBLING of the lane and bar headers rather than part of either: this one answers
//     "what is the delay and who decided it", and says nothing about how it is drawn.
//
// ---------------------------------------------------------------------------
// WHAT THIS IS FOR. The effective delay is one number on screen, and one number cannot
// say whether the tier ladder or the session relay floor produced it. On the shipped
// configuration the floor is the larger of the two for most tiers, so a reader watching
// the tier change sees nothing move and concludes the tier system is broken. The
// decomposition names the arms so that "nothing moved" reads as "the floor is binding".
//
// ---------------------------------------------------------------------------
// THE UNFLOORED BASE IS OBTAINED BY RUNNING THE PRODUCTION FUNCTION, NEVER BY RESTATING
// IT. `tierInputDelayTicks` applies the floor inside and no unfloored production helper
// exists, so the base is read by running that same function against a COPY of the config
// whose floor is zero.
// ⛔ A HAND-WRITTEN COPY OF THE TIER LOOKUP HERE WOULD DRIFT FROM PRODUCTION SILENTLY,
//   and reporting what production actually computed is the entire point of the display.
// ⛔ `rttTierInputDelays` may be indexed ONLY at `kMaxConnectionTierIndex` -- the
//   sanctioned no-tier fallback expression that both production fallback sites use.
//
// ---------------------------------------------------------------------------
// EVERY DERIVED FIELD IS THE PRODUCTION HELPER'S OWN ANSWER. `clampRelayDelayFloorTicks`,
// `relayDelayFloorHardCapTicks`, `applyRelayDelayFloor` and `classifyRelayDelayFloor` are
// CALLED, not reproduced; only the four-way classification below is new here.
//
// `formulaTicks` and `publishedTicks` are carried exactly as given. A disagreement
// between either and `effectiveTicks` is the observation the display exists to make.
// ⚠ SO NEITHER IS EVER REPAIRED HERE -- repairing one would hide the finding.
// ---------------------------------------------------------------------------

#include <cstdint>

#include "OGSimulation/Network/ConnectionTierTable.h"
#include "OGSimulation/PCTimeManagement/TimeConfig.h"

namespace brawlerInputHistoryVisualization
{

// Which arm produced the unfloored base: the replicated tier, or the no-tier fallback.
enum class InputDelayBaseArm : uint8_t
{
	Tier,
	Fallback,
};

// What the session relay floor is doing to that base.
//
//   Inert             the floor is 0 -- the documented "scheduled regime OFF" mode.
//   ActiveNotBinding  a real floor, but the base is already above it.
//   Tie               floor and base agree, so neither can be said to have won.
//   Binding           the floor is what the player feels; the tier arm is invisible.
enum class RelayFloorClass : uint8_t
{
	Inert,
	ActiveNotBinding,
	Tie,
	Binding,
};

// Who decided the effective delay. `Equal` is deliberately NOT one of the arms: at a tie
// naming a winner would invent a precedence the max has no opinion about.
enum class InputDelayWinner : uint8_t
{
	Tier,
	Fallback,
	Floor,
	Equal,
};

// One reading of the client's effective input delay, with its arms exposed.
struct InputDelayDecomposition
{
	// Whether an authoritative tier has arrived. 0 is both the pre-arrival placeholder
	// and a legal tier, so `tierIndex` alone cannot answer this.
	bool tierKnown = false;

	// The tier, clamped into the legal range. Meaningful only when `tierKnown`.
	int32_t tierIndex = 0;

	// Tier 0 collapsed to zero delay by the LAN override. A nonzero floor dominates it.
	bool lanOverrideApplied = false;

	InputDelayBaseArm baseArm = InputDelayBaseArm::Fallback;

	// The arm's own answer, BEFORE the floor. Read by running production, never restated.
	int32_t baseTicks = 0;

	// The floor as stamped into the config, which may be negative or above the cap.
	int32_t floorRequested = 0;

	// That request after `clampRelayDelayFloorTicks`, and the cap it was clamped against.
	int32_t floorTicks = 0;
	int32_t floorHardCap = 0;

	// `applyRelayDelayFloor(baseTicks, cfg)` -- the max of the two arms.
	int32_t effectiveTicks = 0;

	// The recompute's own answer, and the value actually published to the sim. Carried
	// as given: a mismatch with `effectiveTicks` is a finding, not a value to correct.
	int32_t formulaTicks = 0;
	int32_t publishedTicks = 0;

	RelayFloorClass floorClass = RelayFloorClass::Inert;
	InputDelayWinner winner = InputDelayWinner::Fallback;
	RelayDelayFloorAdvisory advisory = RelayDelayFloorAdvisory::None;
};

// Split `cfg`'s effective input delay for this client into its arms and classify who won.
//
// `formulaTicks` is the recompute's answer and `publishedTicks` the value the sim is
// running on; both are recorded, neither is consulted.
inline InputDelayDecomposition decomposeInputDelay(bool              tierKnown,
                                                   int32_t           tierIndex,
                                                   const TimeConfig& cfg,
                                                   int32_t           formulaTicks,
                                                   int32_t           publishedTicks)
{
	InputDelayDecomposition reading;

	reading.tierKnown = tierKnown;
	reading.tierIndex = clampConnectionTierIndex(tierIndex);
	reading.lanOverrideApplied = tierKnown && reading.tierIndex == 0 && cfg.lanZeroDelayOverride;
	reading.baseArm = tierKnown ? InputDelayBaseArm::Tier : InputDelayBaseArm::Fallback;

	// The probe: production's own lookup against a zero-floor copy of the config, which
	// yields the pre-floor answer without a second copy of the tier rule living here.
	// ⭐ THE IDENTITY IT LEANS ON IS `RelayDelayFloorTest.cpp`'s floor-0 degenerate proof.
	TimeConfig probe = cfg;
	probe.relayDelayFloorTicks = 0;
	reading.baseTicks = tierKnown
		? tierInputDelayTicks(reading.tierIndex, probe)
		: cfg.rttTierInputDelays[kMaxConnectionTierIndex];

	reading.floorRequested = cfg.relayDelayFloorTicks;
	reading.floorTicks = clampRelayDelayFloorTicks(cfg.relayDelayFloorTicks, cfg);
	reading.floorHardCap = relayDelayFloorHardCapTicks(cfg);
	reading.effectiveTicks = applyRelayDelayFloor(reading.baseTicks, cfg);
	reading.formulaTicks = formulaTicks;
	reading.publishedTicks = publishedTicks;
	reading.advisory = classifyRelayDelayFloor(cfg);

	const InputDelayWinner baseWinner =
		tierKnown ? InputDelayWinner::Tier : InputDelayWinner::Fallback;

	// Total over `(floorTicks, baseTicks)`, and ordered: an inert floor is inert even
	// when the base is also 0, which the tie arm would otherwise claim.
	if (reading.floorTicks == 0)
	{
		reading.floorClass = RelayFloorClass::Inert;
		reading.winner = baseWinner;
	}
	else if (reading.floorTicks < reading.baseTicks)
	{
		reading.floorClass = RelayFloorClass::ActiveNotBinding;
		reading.winner = baseWinner;
	}
	else if (reading.floorTicks == reading.baseTicks)
	{
		reading.floorClass = RelayFloorClass::Tie;
		reading.winner = InputDelayWinner::Equal;
	}
	else
	{
		reading.floorClass = RelayFloorClass::Binding;
		reading.winner = InputDelayWinner::Floor;
	}

	return reading;
}

} // namespace brawlerInputHistoryVisualization

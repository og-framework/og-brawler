#pragma once
// SPDX-License-Identifier: BUSL-1.1

// Feeding the input-history display from the caches the client already keeps.
//
//   * Pure. Templated on its two sources, so og-brawler-tests reaches the REAL
//     functions from a tree that cannot link UE -- the same trick
//     BrawlerMotionMatching.h uses to make the matcher itself testable.
//   * A SIBLING of BrawlerInputHistoryVisualization.h rather than an extension of
//     it: that header is pure DATA and deliberately names no capture type. This is
//     the one that may, which is why the two are separate files.
//
// ---------------------------------------------------------------------------
// WHY POLLING IS LOSSLESS -- it rests on exactly two facts, and neither alone.
//
//   1. The source LocalInputCache holds kLocalInputCacheCapacityTicks (64) ticks,
//      about 1.07 s at the 60 Hz tickFrequency. Sweeping that whole window sees
//      every capture at least once for any poll period shorter than the window.
//   2. InputHistoryRowRing::appendCapture is IDEMPOTENT on the capture tick and
//      REJECTS ticks behind its newest row, so re-presenting the same window on
//      the next frame changes nothing.
//
// Fact 1 without fact 2 double-counts every tick it re-sees; fact 2 without fact 1
// is exact but blind to whatever scrolled out. Together they make "sweep the whole
// window, every frame" both correct and cheap.
//
// AND THE DEGRADATION IS VISIBLE RATHER THAN SILENT. If a poll gap does exceed the
// window, the lost ticks leave a hole, and a hole OPENS A NEW ROW rather than being
// folded into its neighbour -- so the display shows the discontinuity instead of
// overstating a hold.
// ---------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>
#include <optional>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include "OGBrawler/BrawlerInputHistoryVisualization.h"
#include "OGBrawler/BrawlerInputHistoryVisualizationDelay.h"
#include "OGBrawler/BrawlerInputHistoryVisualizationLanes.h"
#include "OGBrawler/BrawlerMotionMatching.h"
#include "OGSimulation/Network/LocalInputCache.h"

namespace brawlerInputHistoryVisualization
{

// The window one poll sweeps. INCLUSIVE at both ends, and always non-empty.
struct PollWindow
{
	uint32_t oldestTick = 0u;
	uint32_t newestTick = 0u;

	uint32_t tickCount() const { return newestTick - oldestTick + 1u; }
};

// `windowTicks` ticks ending at and including `newestTick`, clamped at tick 0.
// A zero width is read as one tick: an empty window would poll nothing at all.
inline PollWindow pollWindowEndingAt(uint32_t newestTick, std::size_t windowTicks)
{
	const uint32_t span = (windowTicks == 0u) ? 1u : static_cast<uint32_t>(windowTicks);

	PollWindow window;
	window.newestTick = newestTick;
	// Early session: the subtraction would otherwise wrap to the top of the tick space.
	window.oldestTick = (newestTick >= span - 1u) ? newestTick - (span - 1u) : 0u;
	return window;
}

// The capture sweep spans the SOURCE cache's own capacity: a shorter window would
// drop captures the cache still holds, a longer one asks for evicted ticks.
inline constexpr std::size_t kCapturePollWindowTicks = kLocalInputCacheCapacityTicks;

// The join sweep spans the resident correction window the inversion is sized to.
inline constexpr std::size_t kAppliedPollWindowTicks = kAppliedCaptureInversionCapacity;

// The two row fields one capture contributes. Extracted so the mapping from a
// recorded capture to a drawn row is pinnable without a ring or a window.
struct CaptureRowFields
{
	DirectionBucket direction  = DirectionBucket::Neutral;
	uint8_t         buttonMask = 0u;
};

// ---------------------------------------------------------------------------
// THE STICK THE DISPLAY CLASSIFIES IS moveDirectionWorld, NOT moveDirection.
//
// inputSequence::matchSequence classifies a HISTORY entry as
// aimRelativeAngle(entry->moveDirectionWorld.xy, entry->aimDirection.xy, deadzone).
// Reading the other field here would draw a glyph for a sector the matcher never
// tested, which is the single disagreement this display exists to rule out.
// ---------------------------------------------------------------------------
inline CaptureRowFields captureRowFieldsOf(const dAttackMachineSimulation::PlayerInput& capture,
                                           float                                        deadzone)
{
	CaptureRowFields fields;

	fields.direction = directionBucketOf(
		glm::vec2(capture.moveDirectionWorld.x, capture.moveDirectionWorld.y),
		glm::vec3(capture.aimDirection.x, capture.aimDirection.y, 0.f),
		deadzone);
	fields.buttonMask = simulatableBrawler::motionButtonMask(capture.attackLeft, capture.attackRight);

	return fields;
}

// What one poll did. `capturesPresented` counts the RESIDENT ticks swept and
// `capturesFolded` the subset that changed the ring; their gap IS the idempotence.
struct InputHistoryPollCounts
{
	uint32_t capturesPresented = 0u;
	uint32_t capturesFolded    = 0u;
};

// ---------------------------------------------------------------------------
// The `History` shape, structural rather than a C++ concept, matching the one
// BrawlerMotionMatching.h already specifies and DelayLineMotionHistory satisfies:
//
//     const dAttackMachineSimulation::PlayerInput* at(uint32_t tick) const;
//
// It MUST answer nullptr for a tick the source never captured. A source that
// substituted a neutral would fabricate a row for every absent tick.
// ---------------------------------------------------------------------------
template <typename History>
void mergeResidentCaptures(const History&          history,
                           PollWindow              window,
                           float                   deadzone,
                           InputHistoryRowRing&    ring,
                           InputHistoryPollCounts& counts)
{
	// ASCENDING, and the ring's contract is why: a descending sweep would present every
	// tick behind the newest row and fold nothing at all.
	for (uint32_t offset = 0u; offset < window.tickCount(); ++offset)
	{
		const uint32_t tick = window.oldestTick + offset;

		const dAttackMachineSimulation::PlayerInput* capture = history.at(tick);
		if (capture == nullptr)
			continue;

		++counts.capturesPresented;

		const CaptureRowFields fields   = captureRowFieldsOf(*capture, deadzone);
		const AppendResult     appended =
			ring.appendCapture(tick, fields.direction, fields.buttonMask);

		if (appended == AppendResult::OpenedRow || appended == AppendResult::ExtendedRow)
			++counts.capturesFolded;
	}
}

// ---------------------------------------------------------------------------
// The `SlotReader` shape, likewise structural -- one reader fronting the two
// diagnostic seams and one presence test, so this header never names a
// simulation peer:
//
//     AppliedCaptureRef                  appliedCaptureRef(uint32_t simTick) const;
//     std::optional<SlotStateProvenance> slotProvenance(uint32_t simTick) const;
//     bool                               hasCorrectionCache() const;
//
// The first two are asked at the SAME tick, the premise the join is built on: an
// observation naming no capture speaks for the tick it was asked about. The third
// answers a different question -- does this id hold a correction cache at all --
// asked once per poll, for classifyNoSlot's own !hasCache guard.
// ---------------------------------------------------------------------------
// Returns how many observations were filed; the window is sized so none is refused.
// Also fills `residency` with what this same sweep learned about the ring's own bounds --
// no second read of either seam. It does NOT set `hasCache`; that is the caller's own fact.
template <typename SlotReader>
uint32_t rebuildAppliedCaptureInversion(const SlotReader&        reader,
                                        PollWindow               window,
                                        AppliedCaptureInversion& inversion,
                                        WindowResidency&         residency)
{
	// REBUILT, never appended to: the window scrolls, so a kept entry would outlive the
	// slot it describes. Monotonicity belongs to the ROW summaries, not to this map.
	inversion.clear();

	uint32_t filed = 0u;

	residency.anyResident    = false;
	residency.oldestResident = 0u;
	residency.newestResident = 0u;

	for (uint32_t offset = 0u; offset < window.tickCount(); ++offset)
	{
		AppliedSlotObservation observation;
		observation.appliedTick = window.oldestTick + offset;
		observation.ref         = reader.appliedCaptureRef(observation.appliedTick);
		observation.provenance  = reader.slotProvenance(observation.appliedTick);

		if (observation.ref.kind != AppliedCaptureRefKind::NoSlot)
		{
			// ASCENDING sweep: the first resident tick seen is the oldest, and every
			// later one seen overwrites the newest -- no min/max needed.
			if (!residency.anyResident)
			{
				residency.anyResident    = true;
				residency.oldestResident = observation.appliedTick;
			}
			residency.newestResident = observation.appliedTick;
		}

		if (inversion.observe(observation))
			++filed;
	}

	return filed;
}

// Convenience overload for callers that only need the join, not the residency reading.
template <typename SlotReader>
uint32_t rebuildAppliedCaptureInversion(const SlotReader&        reader,
                                        PollWindow               window,
                                        AppliedCaptureInversion& inversion)
{
	WindowResidency discardedResidency;
	return rebuildAppliedCaptureInversion(reader, window, inversion, discardedResidency);
}

// One whole poll: fold this window's captures into the rows, and nothing else.
//
// A row is a run of identical INPUT while lineage varies per tick, so a per-row summary
// saturates and the join lands on the per-tick lane instead.
// ⛔ NO ROW CARRIES A LINEAGE.
template <typename History>
InputHistoryPollCounts pollInputHistory(const History&       history,
                                        uint32_t             newestTick,
                                        float                deadzone,
                                        InputHistoryRowRing& ring)
{
	InputHistoryPollCounts counts;

	mergeResidentCaptures(history,
		pollWindowEndingAt(newestTick, kCapturePollWindowTicks), deadzone, ring, counts);

	return counts;
}

// ---------------------------------------------------------------------------
// THE PER-TICK LANES -- fed from the SAME poll, on the SAME sim tick, as the rows above.
//
// The join is REUSED verbatim: rebuildAppliedCaptureInversion already keys its entries on
// the CAPTURE tick, which is the lane's key, so a lane poll is that call plus one write
// per entry. Nothing here re-derives an arm, a ladder or a lineage mapping.
// ---------------------------------------------------------------------------

// What one lane poll did. The provenance split IS the evidence a re-poll is cheap: on a
// quiet frame every cell is Unchanged, and one late correction shows as one Updated.
struct TickLanePollCounts
{
	// What the gate decided for this tick. Elided means NEITHER lane was written.
	LaneAdmission admission = LaneAdmission::Recorded;

	// The one reading that sees a hard resync, 0 or 1 per poll. It answers for a
	// BACKWARD resync only; a forward one is not visible to this poll.
	uint32_t axisBreaksBackward = 0u;

	uint32_t provenanceCellsRecorded  = 0u;
	uint32_t provenanceCellsUpdated   = 0u;
	uint32_t provenanceCellsUnchanged = 0u;
	uint32_t provenanceCellsRefused   = 0u;

	// classifyNoSlot's non-writing causes, each its own count.
	uint32_t provenanceCellsEvicted         = 0u;
	uint32_t provenanceCellsMissingInWindow = 0u;
	uint32_t provenanceCellsUnclassifiable  = 0u;
	// 0 or 1 per poll -- hasCache is one fact about the whole window, not per cell.
	uint32_t provenanceNoCachePolls         = 0u;

	// Joins whose capture tick has no lane tick, because the gate elided it.
	uint32_t provenanceCellsElided = 0u;

	// 0 or 1 per poll: there is exactly one live tick to sample.
	uint32_t machineCellsRecorded = 0u;
	uint32_t machineCellsIgnored  = 0u;

	// The delay lane's two halves, counted separately -- they have OPPOSITE write
	// policies, so one number could not stand for both.
	uint32_t delayClientRecorded = 0u;
	uint32_t delayClientIgnored  = 0u;
	uint32_t delayClientElided   = 0u;

	uint32_t delayServerRecorded  = 0u;
	uint32_t delayServerUpdated   = 0u;
	uint32_t delayServerUnchanged = 0u;
	uint32_t delayServerElided    = 0u;
};

// The window the retained/visible read spans, ending at the lanes' own axis tick. Both
// lanes are read through this ONE window, which is what keeps a vertical slice one tick.
inline PollWindow retainedLaneWindow(const InputHistoryTickLanes& lanes, uint32_t retainedTicks)
{
	const uint32_t newestTick = lanes.hasAxis() ? lanes.newestAxisTick() : 0u;
	return pollWindowEndingAt(newestTick, clampRetainedLaneTicks(static_cast<int64_t>(retainedTicks)));
}

// One sweep of the resident correction window into the provenance lane.
//
// A tick that has scrolled out of the resident window files no entry at all, so an
// assignment cannot erase it -- and a late correction then repaints exactly one cell.
// ⭐ WRITES, NEVER MERGES -- classifyNoSlot is why a NoSlot join may file nothing at all.
inline void pollProvenanceLane(const AppliedCaptureInversion& inversion,
                               const WindowResidency&         residency,
                               InputHistoryTickLanes&         lanes,
                               TickLanePollCounts&             counts)
{
	// hasCache is one fact about the whole window: counted once here, not per cell below.
	if (!residency.hasCache)
		++counts.provenanceNoCachePolls;

	for (std::size_t index = 0u; index < inversion.size(); ++index)
	{
		const CaptureJoin& join = inversion.at(index);

		// An elided capture has no lane tick, and a guessed one repaints another tick.
		// ⛔ THE GATE OWNS THE CONVERSION.
		const std::optional<uint32_t> laneTick = lanes.gate().laneTickOf(join.captureTick);
		if (!laneTick.has_value())
		{
			++counts.provenanceCellsElided;
			continue;
		}

		if (join.summary == RowProvenanceSummary::Pending)
		{
			const NoSlotCause cause = classifyNoSlot(join.captureTick, residency);
			if (cause != NoSlotCause::NotYetRun)
			{
				if (cause == NoSlotCause::Unclassifiable)
					++counts.provenanceCellsUnclassifiable;
				else if (cause == NoSlotCause::Evicted)
					++counts.provenanceCellsEvicted;
				else if (cause == NoSlotCause::MissingInsideWindow)
					++counts.provenanceCellsMissingInWindow;

				continue;
			}
		}

		switch (lanes.editProvenance().record(*laneTick, join.summary))
		{
		case LaneWriteResult::RecordedCell:     ++counts.provenanceCellsRecorded;  break;
		case LaneWriteResult::UpdatedCell:      ++counts.provenanceCellsUpdated;   break;
		case LaneWriteResult::IgnoredDuplicate: ++counts.provenanceCellsUnchanged; break;
		case LaneWriteResult::IgnoredStale:     ++counts.provenanceCellsRefused;   break;
		}
	}
}

// The ONE machine-state sample this poll can take: the state as it is right now, filed
// under the live LANE tick. Past ticks are unreachable, which is why the lane has holes.
inline void pollMachineStateLane(uint32_t               liveLaneTick,
                                 DAttackState           machineState,
                                 InputHistoryTickLanes& lanes,
                                 TickLanePollCounts&    counts)
{
	const LaneWriteResult result =
		lanes.editMachineState().recordIfAbsent(liveLaneTick, machineStateCellOf(machineState));

	if (result == LaneWriteResult::RecordedCell)
		++counts.machineCellsRecorded;
	else
		++counts.machineCellsIgnored;
}

// ---------------------------------------------------------------------------
// THE DELAY VERDICT LANE -- fed from the SAME rebuilt inversion pollProvenanceLane
// just walked, so no second seam sweep runs.
//
// (a) the CLIENT half is filed at the capture tick this poll's effective delay
//     applies to.
// ⛔ SUBTRACT IN SIM TICKS, THEN MAP: a lane-tick subtraction would silently skip
//   whatever the gate elided and land on the wrong tick.
//
// (b) the SERVER half is filed from every authority-named join: appliedTick minus
//     captureTick.
// (c) every Sentinel-arm join sets serverNamedNoCapture at the capture tick its own
//     entry already carries -- an observation naming no capture speaks for the tick
//     it was asked about, and that is true of every arm, not only this one.
//
// (b) and (c) share one merge-and-record write and the same counters: both are the
// SERVER's half of the cell.
// ---------------------------------------------------------------------------
inline void pollInputDelayLane(const AppliedCaptureInversion&         inversion,
                               uint32_t                                liveSimTick,
                               std::optional<InputDelayDecomposition> delay,
                               InputHistoryTickLanes&                 lanes,
                               TickLanePollCounts&                     counts)
{
	if (delay.has_value())
	{
		const int32_t  effective  = delay->effectiveTicks;
		const uint32_t effectiveU = (effective > 0) ? static_cast<uint32_t>(effective) : 0u;
		const uint32_t captureSim = (effective <= 0) ? liveSimTick
		                          : (liveSimTick >= effectiveU ? liveSimTick - effectiveU : 0u);

		const std::optional<uint32_t> laneTick = lanes.gate().laneTickOf(captureSim);
		if (!laneTick.has_value())
		{
			++counts.delayClientElided;
		}
		else
		{
			const InputDelayCell* existing = lanes.delay().find(*laneTick);

			// ⭐ FIRST SAMPLE WINS: an already-filed client half is never overwritten.
			if (existing != nullptr && existing->clientDelayTicks.has_value())
			{
				++counts.delayClientIgnored;
			}
			else
			{
				InputDelayCell merged  = (existing != nullptr) ? *existing : InputDelayCell{};
				merged.clientDelayTicks = effective;

				const LaneWriteResult result = lanes.editDelay().record(*laneTick, merged);
				if (result == LaneWriteResult::IgnoredStale)
					++counts.delayClientElided;
				else
					++counts.delayClientRecorded;
			}
		}
	}

	for (std::size_t index = 0u; index < inversion.size(); ++index)
	{
		const CaptureJoin& join = inversion.at(index);

		if (!join.authorityNamed && !join.sentinel)
			continue;

		const std::optional<uint32_t> laneTick = lanes.gate().laneTickOf(join.captureTick);
		if (!laneTick.has_value())
		{
			++counts.delayServerElided;
			continue;
		}

		const InputDelayCell* existing = lanes.delay().find(*laneTick);
		InputDelayCell        merged   = (existing != nullptr) ? *existing : InputDelayCell{};

		if (join.authorityNamed)
		{
			merged.serverLagTicks =
				static_cast<int32_t>(join.appliedTick) - static_cast<int32_t>(join.captureTick);
		}
		if (join.sentinel)
		{
			merged.serverNamedNoCapture = true;
		}

		switch (lanes.editDelay().record(*laneTick, merged))
		{
		case LaneWriteResult::RecordedCell:     ++counts.delayServerRecorded;  break;
		case LaneWriteResult::UpdatedCell:      ++counts.delayServerUpdated;   break;
		case LaneWriteResult::IgnoredDuplicate: ++counts.delayServerUnchanged; break;
		case LaneWriteResult::IgnoredStale:     ++counts.delayServerElided;    break;
		}
	}
}

// One whole lane poll: every lane, one tick, one axis, ONE gate decision.
//
// ⚠ THE TWO RE-POLL RULES DIFFER ON PURPOSE. Provenance UPDATES an already-recorded tick,
// because a correction may land after it was first written; machine state IGNORES one.
//
// `liveInput` is the panel's own classification of the capture at this tick, so the two
// displays share one idea of "no input"; nullopt means the poll could not read it.
// ⛔ NOTHING HERE PAUSES ON ITS OWN. The gate decides once and both lanes obey.
//
// `predictionOffsetTicks` is the estimator's offset, or nullopt on a role that does not
// predict. ⛔ THE TICK IT IS PAIRED WITH IS THIS POLL'S, NEVER ONE READ LATER.
//
// `delay` is this poll's decomposition, or nullopt when the delay display is not being
// fed. ⛔ NOTED ABOVE THE GATE, BESIDE THE AUTHORITY READING, for the same reason.
//
// `clock` is this poll's ONE read of the client clock, or nullopt on a role that does not
// predict. ⛔ NOTED ABOVE THE GATE TOO -- a paused display must still show the clock.
//
// The axis BREAK a BACKWARD hard resync makes is detected here and decided by the gate,
// from one reading this poll already holds: the gate's own last polled tick.
// ⛔ NO SECOND SEAM IS OPENED FOR IT, and residency stays DERIVED from the sweep.
template <typename SlotReader>
TickLanePollCounts pollInputHistoryLanes(const SlotReader&               reader,
                                         uint32_t                        liveSimTick,
                                         DAttackState                    machineState,
                                         std::optional<CaptureRowFields> liveInput,
                                         bool                            pauseWhileIdle,
                                         std::optional<uint32_t>         predictionOffsetTicks,
                                         std::optional<InputDelayDecomposition> delay,
                                         std::optional<ClockDriftReading> clock,
                                         AppliedCaptureInversion&        inversion,
                                         InputHistoryTickLanes&          lanes)
{
	TickLanePollCounts counts;

	// The clock reading is filed against THIS poll's own tick, and before the gate can
	// end the poll early. ⛔ AN ELIDED POLL STILL MOVES IT -- a frozen reading would leave
	//   the marker on a column while authority ran past everything the bar holds.
	lanes.noteAuthorityReading(predictionOffsetTicks, liveSimTick);
	lanes.noteDelayReading(delay, liveSimTick);
	lanes.noteClockDriftReading(clock, liveSimTick);

	// ⭐ FILED ABOVE THE GATE, LIKE THE THREE READINGS ABOVE -- a frozen residency edge would
	// draw long-evicted cells as still live while the player is idle.
	WindowResidency residency;
	residency.hasCache = reader.hasCorrectionCache();
	rebuildAppliedCaptureInversion(
		reader, pollWindowEndingAt(liveSimTick, kAppliedPollWindowTicks), inversion, residency);

	lanes.noteResidencyReading(residency, liveSimTick);

	const std::optional<uint32_t> lastPolledSimTick = lanes.gate().lastPolledSimTick();

	// A hard resync is the ONLY assignment to the client's prediction tick; every other
	// step the clock takes is monotone.
	// ⛔ SO A TICK BEHIND THE LAST POLLED ONE IS CERTAIN, NOT A HEURISTIC.
	// ⛔ NO FORWARD EQUIVALENT: past the clamp, only a tolerance separates a wipe from a push.
	if (lastPolledSimTick.has_value() && liveSimTick < *lastPolledSimTick)
		counts.axisBreaksBackward = 1u;

	// The break, taken from the tick the previous epoch was last polled at.
	const std::optional<uint32_t> axisBreakFromSimTick =
		(counts.axisBreaksBackward != 0u) ? lastPolledSimTick : std::nullopt;

	// A tick the poll could not classify is one worth recording.
	// ⛔ AN UNREADABLE CAPTURE IS NOT IDLE.
	const bool inactive = liveInput.has_value()
		&& laneTickIsInactive(liveInput->direction, liveInput->buttonMask, machineState);

	// ⛔ EVALUATED ONCE, HERE, AND NOWHERE ELSE. Two evaluations could disagree by a tick
	// and desynchronise the bars, which is the one property the stacked pair is for.
	counts.admission =
		lanes.editGate().admit(liveSimTick, inactive, pauseWhileIdle, axisBreakFromSimTick);

	if (counts.admission == LaneAdmission::Elided)
		return counts;

	const std::optional<uint32_t> liveLaneTick = lanes.gate().laneTickOf(liveSimTick);
	if (!liveLaneTick.has_value())
		return counts;

	pollProvenanceLane(inversion, residency, lanes, counts);
	pollInputDelayLane(inversion, liveSimTick, delay, lanes, counts);
	pollMachineStateLane(*liveLaneTick, machineState, lanes, counts);
	lanes.noteAxisTick(*liveLaneTick);

	return counts;
}

} // namespace brawlerInputHistoryVisualization

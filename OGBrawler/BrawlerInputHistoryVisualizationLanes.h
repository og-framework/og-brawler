#pragma once
// SPDX-License-Identifier: BUSL-1.1

// The frame-meter lanes: ONE CELL PER TICK, keyed on the capture tick.
//
//   * Pure. Fixed arrays, no allocation, no engine type, no globals.
//   * A SIBLING of BrawlerInputHistoryVisualization.h. That header is the ROW model
//     (a run of identical input); this one is the TICK model, and the two exist for
//     opposite reasons -- see the axis argument below.
//
// ---------------------------------------------------------------------------
// ⛔ STORAGE IS PER TICK AND MUST NEVER BE RUN-LENGTH COMPRESSED.
//
// The reference frame meter draws one cell per tick and prints a run length on the
// run's LAST cell, so runs are a RENDER-TIME reading of a per-tick store, not a
// storage shape. That distinction is not cosmetic. A row keyed on input identity
// folds many ticks into one cell, and lineage varies per tick from network events:
// merging it worst-case-wins across such a row SATURATES -- one resimulated tick in
// a 99-tick hold makes all 99 read as resimulated, permanently, and the display's
// information content then DECAYS as rows lengthen.
//
// Per-tick cells give the opposite behaviour: a correction that lands late repaints
// EXACTLY ONE CELL and cannot touch a neighbour.
// ---------------------------------------------------------------------------
// THE TWO LANES FILL DIFFERENTLY, AND THAT IS INHERENT.
//
// PROVENANCE can be back-filled: the correction cache still holds the history, so a
// poll can answer for ticks that have already passed.
// MACHINE STATE cannot. There is no per-tick machine-state history to read, so it can
// only be sampled at the tick that is live when the poll runs -- which means holes at
// startup, after a poll gap, and just after the display is switched on.
//
// This is a netcode diagnostic, so a fabricated cell would be read as evidence: a bar
// that invents plausible history is worse than one with visible holes.
// ⛔ A HOLE IS ITS OWN CELL VALUE AND IS NEVER FILLED BY CARRYING A NEIGHBOUR FORWARD.
// ---------------------------------------------------------------------------

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "OGBrawler/BrawlerInputHistoryVisualization.h"
#include "OGBrawler/BrawlerInputHistoryVisualizationDelay.h"
#include "OGBrawler/DAttackMachineSimulation.h"
#include "OGSimulation/PCTimeManagement/ClientPredictionClock.h"

namespace brawlerInputHistoryVisualization
{

// What one machine-state cell holds. NotSampled is a VALUE, not an absence to be
// papered over: it is the only honest answer for a tick the poll never saw.
enum class MachineStateCell : uint8_t
{
	NotSampled = 0,
	Attacking,
	Idle,
	GuardFlinch,
	HitFlinch,
};

// The four machine states plus NotSampled. A sweep pins this against kDAttackStateCount.
inline constexpr uint8_t kMachineStateCellCount = 5u;

// One machine state in lane vocabulary, TOTAL over DAttackState.
//
// ⛔ NO ARM MAY RETURN NotSampled -- a sampled tick that read as unsampled would be
// indistinguishable from a hole, which is the one thing this lane must not blur.
constexpr MachineStateCell machineStateCellOf(DAttackState state)
{
	switch (state)
	{
	case DAttackState::Attacking:   return MachineStateCell::Attacking;
	case DAttackState::Idle:        return MachineStateCell::Idle;
	case DAttackState::GuardFlinch: return MachineStateCell::GuardFlinch;
	case DAttackState::HitFlinch:   return MachineStateCell::HitFlinch;
	}

	// Outside the enumeration: an unsampled cell is the only claim that stays true.
	return MachineStateCell::NotSampled;
}

// ---------------------------------------------------------------------------
// CAPACITY IS FIXED; RETENTION IS A SETTING. 240 ticks is 4 s at the 60 Hz
// tickFrequency, which covers the reference meter's 251-tick span less its scrolled tail.
//
// ⛔ THE RING IS NEVER SIZED FROM THE RETENTION SETTING. Resizing a live ring at runtime
// invalidates every resident cell's slot index; allocating the max once costs 240 bytes.
// ---------------------------------------------------------------------------
inline constexpr std::size_t kTickLaneCapacity = 240u;

inline constexpr uint32_t kTickLaneDefaultRetainedTicks = 120u;
inline constexpr uint32_t kTickLaneMinRetainedTicks     = 1u;
inline constexpr uint32_t kTickLaneMaxRetainedTicks     = static_cast<uint32_t>(kTickLaneCapacity);

// A retention request outside [1, 240] is CLAMPED to the nearer end, never rejected and
// never allowed to resize anything. Taken as int64 so a negative setting clamps rather
// than wrapping through an unsigned conversion.
constexpr uint32_t clampRetainedLaneTicks(int64_t requestedTicks)
{
	if (requestedTicks < static_cast<int64_t>(kTickLaneMinRetainedTicks))
		return kTickLaneMinRetainedTicks;

	if (requestedTicks > static_cast<int64_t>(kTickLaneMaxRetainedTicks))
		return kTickLaneMaxRetainedTicks;

	return static_cast<uint32_t>(requestedTicks);
}

// What one cell write did. The two Ignored cases are both no-ops but NOT one fact:
// a duplicate is the poll re-presenting a tick, a stale write is a tick out of the lane.
enum class LaneWriteResult : uint8_t
{
	RecordedCell,
	UpdatedCell,
	IgnoredDuplicate,
	IgnoredStale,
};

// ---------------------------------------------------------------------------
// A fixed per-tick lane, indexed by `tick % capacity()`. A slot answers for a tick only
// when it still HOLDS that tick, so eviction needs no bookkeeping pass: the tick that
// overwrites a slot is exactly the one a whole capacity later.
//
// ⛔ THE RESIDENCY TEST IS PART OF THE READ. Without it a slot left behind by a poll gap
// would answer for a tick hundreds behind the newest, as though nothing had been missed.
// ---------------------------------------------------------------------------
template <typename Cell>
class TickLane
{
public:
	static constexpr std::size_t capacity() { return kTickLaneCapacity; }

	bool     hasCells() const { return m_hasNewestTick; }
	// Precondition: hasCells().
	uint32_t newestTick() const { return m_newestTick; }

	// Slots holding a value, resident or not. Never exceeds capacity(); never allocates.
	std::size_t storedCellCount() const { return m_storedCellCount; }

	void clear()
	{
		m_slots           = {};
		m_hasNewestTick   = false;
		m_newestTick      = 0u;
		m_storedCellCount = 0u;
	}

	// Within (newestTick() - capacity(), newestTick()]. Nothing is resident before the
	// first write, and a tick ahead of the newest has not happened yet.
	bool residentTick(uint32_t tick) const
	{
		if (!m_hasNewestTick || tick > m_newestTick)
			return false;

		return (m_newestTick - tick) < kTickLaneCapacity;
	}

	// The cell for `tick`, or nullptr when the lane has none it may still answer for.
	const Cell* find(uint32_t tick) const
	{
		if (!residentTick(tick))
			return nullptr;

		const Slot& slot = m_slots[tick % kTickLaneCapacity];
		return (slot.occupied && slot.tick == tick) ? &slot.value : nullptr;
	}

	// ⭐ REWRITABLE: a later poll may change this tick's cell, and touches no other.
	LaneWriteResult record(uint32_t tick, Cell value) { return write(tick, value, true); }

	// ⭐ FIRST SAMPLE WINS: re-presenting an already-sampled tick is a no-op.
	LaneWriteResult recordIfAbsent(uint32_t tick, Cell value) { return write(tick, value, false); }

private:
	struct Slot
	{
		uint32_t tick     = 0u;
		Cell     value{};
		bool     occupied = false;
	};

	LaneWriteResult write(uint32_t tick, Cell value, bool overwrite)
	{
		// A tick a whole capacity behind the newest shares a slot index with a LIVE tick,
		// so accepting it would clobber a cell the display is currently drawing.
		if (m_hasNewestTick && tick <= m_newestTick && (m_newestTick - tick) >= kTickLaneCapacity)
			return LaneWriteResult::IgnoredStale;

		Slot&      slot          = m_slots[tick % kTickLaneCapacity];
		const bool holdsThisTick = slot.occupied && slot.tick == tick;

		if (holdsThisTick && !overwrite)
			return LaneWriteResult::IgnoredDuplicate;

		LaneWriteResult result = LaneWriteResult::RecordedCell;
		if (holdsThisTick)
		{
			result = (slot.value == value) ? LaneWriteResult::IgnoredDuplicate
			                               : LaneWriteResult::UpdatedCell;
		}
		else if (!slot.occupied)
		{
			// A slot holding a DIFFERENT tick is reused, not added: the count is slots, not writes.
			++m_storedCellCount;
		}

		slot.tick     = tick;
		slot.value    = value;
		slot.occupied = true;

		if (!m_hasNewestTick || tick > m_newestTick)
		{
			m_newestTick    = tick;
			m_hasNewestTick = true;
		}

		return result;
	}

	std::array<Slot, kTickLaneCapacity> m_slots{};
	uint32_t                            m_newestTick      = 0u;
	bool                                m_hasNewestTick   = false;
	std::size_t                         m_storedCellCount = 0u;
};

using ProvenanceLane   = TickLane<RowProvenanceSummary>;
using MachineStateLane = TickLane<MachineStateCell>;

// ---------------------------------------------------------------------------
// THE DELAY VERDICT LANE -- keyed on the CAPTURE tick, exactly like provenance.
//
// The two halves have OPPOSITE write policies, because they answer different
// questions. The client half records what was true when the capture was MADE, so a
// re-presented tick must not depend on how many frames it happened to be drawn on:
// ⭐ FIRST-SAMPLE-WINS. The server half records the latest AUTHORITY answer about that
// capture, and a correction can repaint it later, the same rule provenance already
// follows: ⭐ REWRITABLE.
// ---------------------------------------------------------------------------
struct InputDelayCell
{
	// The effective delay D in force when the CLIENT applied this capture.
	std::optional<int32_t> clientDelayTicks;

	// appliedTick - captureTick from an authority-named join: how long the SERVER held
	// this capture.
	std::optional<int32_t> serverLagTicks;

	// The Sentinel arm landed here: the server substituted, naming no capture at all.
	bool serverNamedNoCapture = false;

	bool operator==(const InputDelayCell& o) const
	{
		return clientDelayTicks == o.clientDelayTicks
		    && serverLagTicks == o.serverLagTicks
		    && serverNamedNoCapture == o.serverNamedNoCapture;
	}
};

using InputDelayLane = TickLane<InputDelayCell>;

// The verdict on one cell -- TOTAL, and the rules below are applied IN THIS ORDER.
//
//   serverNamedNoCapture -> NoCaptureNamed  (checked FIRST: the server substituted)
//   no server half       -> NoVerdict       (nothing claimed yet)
//   no client half        -> LagUnverified   (a lag is shown, no comparison possible)
//   lag == D              -> Agree
//   lag == D - 1           -> LagShortByOne  (the documented send-side tear -- its OWN
//                                            class, NEVER folded into Agree)
//   lag > D                -> ServerLater
//   otherwise (lag < D - 1) -> ServerEarlier  (the server's delay reads SMALLER than the
//                                            client's, which lateness alone cannot cause)
enum class InputDelayVerdict : uint8_t
{
	NoVerdict,
	Agree,
	LagShortByOne,
	ServerLater,
	ServerEarlier,
	LagUnverified,
	NoCaptureNamed,
};

inline constexpr uint8_t kInputDelayVerdictCount = 7u;

constexpr InputDelayVerdict delayVerdictOf(const InputDelayCell& cell)
{
	if (cell.serverNamedNoCapture)
		return InputDelayVerdict::NoCaptureNamed;

	if (!cell.serverLagTicks.has_value())
		return InputDelayVerdict::NoVerdict;

	if (!cell.clientDelayTicks.has_value())
		return InputDelayVerdict::LagUnverified;

	const int32_t lag = *cell.serverLagTicks;
	const int32_t d   = *cell.clientDelayTicks;

	if (lag == d)
		return InputDelayVerdict::Agree;
	if (lag == d - 1)
		return InputDelayVerdict::LagShortByOne;
	if (lag > d)
		return InputDelayVerdict::ServerLater;

	return InputDelayVerdict::ServerEarlier;
}

// ---------------------------------------------------------------------------
// THE IDLE GATE -- ONE PREDICATE, EVALUATED ONCE, OBEYED BY BOTH LANES.
//
// A fixed window that spends its cells on a player standing still holds no signal, so a
// tick where nothing is happening is ELIDED: neither lane is written, and the lane axis
// does not move. The window then spans the last N ticks that were worth keeping.
//
// Two evaluations can disagree by a tick, and a vertical slice through the two bars
// would then be two different capture ticks -- which is what the stacked pair is FOR.
// ⛔ THE TWO LANES MUST NEVER EVALUATE THEIR OWN COPY OF THIS.
//
// ---------------------------------------------------------------------------
// WHAT THE PAUSE COSTS, AND WHY THAT IS ACCEPTABLE.
//
// Provenance is deliberately NOT in the predicate, so a correction or a resimulation
// landing while the player stands still is elided with everything else: a rollback while
// idle becomes invisible. That cost was weighed and taken.
// ⭐ THE PAUSE IS A SETTING, and turning it off restores full-fidelity recording, so a
// desync-while-idle investigation is one console line away rather than a rebuild.
//
// The other half of the answer is that an elision is never silent: the span collapses to
// ONE lane tick carrying the number of ticks it removed, so a reader who cannot see what
// happened in the gap can always see how large the gap was.
// ⛔ THE DISPLAY NEVER INVENTS ADJACENCY IT DOES NOT HAVE.
// ---------------------------------------------------------------------------

// A tick is inactive when the DISPLAY'S OWN idea of "no input" holds and the machine is
// idle. The direction bucket and the button mask are the panel's, not a second notion.
constexpr bool laneTickIsInactive(DirectionBucket direction,
                                  uint8_t         buttonMask,
                                  DAttackState    machineState)
{
	return direction == DirectionBucket::Neutral
	       && buttonMask == 0u
	       && machineState == DAttackState::Idle;
}

// ---------------------------------------------------------------------------
// HYSTERESIS. 15 ticks is 0.25 s at the 60 Hz tickFrequency: far longer than the one- to
// three-tick chatter a stick resting on the deadzone produces, and an eighth of the
// 120-tick default window, so a pause that turns out to be wrong costs one marker cell
// and never more than an eighth of what is on screen. Much below that the pause does not
// pay for itself; much above it the idle wall it exists to remove comes back.
//
// Being late to start recording something interesting is the one failure this display
// cannot absorb, so the two directions are deliberately not symmetric.
// ⛔ ENGAGING IS SLOW AND RESUMING IS INSTANT, never the other way round.
// ---------------------------------------------------------------------------
inline constexpr uint32_t kLanePauseEngageTicks = 15u;

// Two kinds of axis discontinuity. Sweep against this count, never a literal 2.
enum class LaneAxisEventKind : uint8_t { Elision, Resync };
inline constexpr uint8_t kLaneAxisEventKindCount = 2u;

// One discontinuity of the lane axis: where it began on the SIMULATION axis, what it
// removed or repeated, and the single LANE tick it now occupies.
struct LaneAxisEvent
{
	LaneAxisEventKind kind         = LaneAxisEventKind::Elision;
	uint32_t          simTick      = 0u;   // Elision: first elided tick. Resync: new epoch's first tick.
	uint32_t          fromSimTick  = 0u;   // Resync only: the previous epoch's last polled tick.
	uint32_t          skippedTicks = 0u;   // Elision only; 0 for a Resync.
	uint32_t          laneTick     = 0u;   // the marker cell, empty in every lane.
};

// A span costs at least two lane ticks -- its own marker, plus the recorded tick that
// ended it -- so half the lane's capacity is more entries than one window can ever show.
inline constexpr std::size_t kLaneElisionLedgerCapacity = kTickLaneCapacity / 2u;

// What the gate decided for one tick. Both lanes take THIS value.
enum class LaneAdmission : uint8_t
{
	Recorded,
	Elided,
};

// ---------------------------------------------------------------------------
// The gate, and with it the SIMULATION-tick to LANE-tick mapping. Storage stays dense in
// lane ticks, which is the whole point: 240 lane ticks are 240 ticks worth keeping, and a
// long idle can no longer evict the activity either side of it.
//
// Everything the lanes store, evict and read is keyed on the lane tick, and this class
// is the only one that knows the difference.
// ⛔ A LANE TICK IS NOT A SIM TICK ONCE ANYTHING HAS BEEN ELIDED.
// ---------------------------------------------------------------------------
class LaneIdleGate
{
public:
	// The one evaluation. `inactive` is laneTickIsInactive's answer for this tick. A
	// value for `axisBreakFromSimTick` opens a new epoch instead of admitting normally.
	LaneAdmission admit(uint32_t simTick, bool inactive, bool pauseWhileIdle,
	                    std::optional<uint32_t> axisBreakFromSimTick = std::nullopt)
	{
		if (axisBreakFromSimTick.has_value())
		{
			// ⛔ BYPASSES THE EARLY RETURN BELOW -- that early return is what hid a resync.
			fileAxisBreak(simTick, *axisBreakFromSimTick);
			m_inactiveRun = 0u;
		}
		else if (m_hasLastSimTick && simTick <= m_lastSimTick)
		{
			// The poll runs at render rate, so it re-presents a tick on every frame drawn
			// without the simulation advancing. ⛔ ONE DECISION PER CAPTURE TICK.
			return m_lastAdmission;
		}

		m_hasLastSimTick = true;
		m_lastSimTick    = simTick;
		m_inactiveRun    = inactive ? (m_inactiveRun + 1u) : 0u;

		if (m_inactiveRun > kLanePauseEngageTicks)
		{
			// Held at the trigger, so a long idle cannot run the counter over.
			m_inactiveRun = kLanePauseEngageTicks + 1u;

			if (pauseWhileIdle)
			{
				if (!m_paused)
				{
					m_paused           = true;
					m_spanFirstSimTick = simTick;
				}

				m_lastAdmission = LaneAdmission::Elided;
				return m_lastAdmission;
			}
		}

		if (m_paused)
			closeSpan(simTick);

		m_lastAdmission = LaneAdmission::Recorded;
		return m_lastAdmission;
	}

	// The lane tick `simTick` was recorded at, or nullopt when it has no cell (elided,
	// never run, or too old). ⛔ A GUESS HERE WOULD WRITE A CELL AT THE WRONG TICK.
	std::optional<uint32_t> laneTickOf(uint32_t simTick) const
	{
		// The span still open has no ledger entry yet, and no lane tick either.
		if (m_paused && simTick >= m_spanFirstSimTick)
			return std::nullopt;

		for (std::size_t back = m_ledgerSize; back != 0u; --back)
		{
			const LaneAxisEvent& event = entry(back - 1u);

			if (event.kind == LaneAxisEventKind::Resync)
			{
				if (simTick >= event.simTick)
					return laneTickAcrossOffset(simTick, event);

				// Between the old epoch's last polled tick and the new epoch's first: a
				// forward resync's never-simulated range.
				if (simTick > event.fromSimTick)
					return std::nullopt;

				continue;
			}

			if (simTick >= event.simTick + event.skippedTicks)
				return laneTickAcrossOffset(simTick, event);

			if (simTick >= event.simTick)
				return std::nullopt;
		}

		// Before the first event the two axes were the same; once one has been dropped,
		// nothing left here can place a tick older than the ones that remain.
		return m_ledgerDropped ? std::nullopt : std::optional<uint32_t>(simTick);
	}

	bool     paused() const { return m_paused; }
	uint32_t consecutiveInactiveTicks() const { return m_inactiveRun; }

	// nullopt before the first admit(); the poll's own break detection reads this.
	std::optional<uint32_t> lastPolledSimTick() const
	{
		return m_hasLastSimTick ? std::optional<uint32_t>(m_lastSimTick) : std::nullopt;
	}

	std::size_t          axisEventCount() const { return m_ledgerSize; }
	// Precondition: index < axisEventCount(). Oldest first, as the ledger holds them.
	const LaneAxisEvent& axisEventAt(std::size_t index) const { return entry(index); }

private:
	// DERIVED so a ledger entry cannot disagree with the mapping it describes.
	// ⛔ CAST AFTER THE SUBTRACTION -- an operand cast to uint32_t here wraps silently.
	static constexpr int64_t offsetAfter(const LaneAxisEvent& event)
	{
		return static_cast<int64_t>(event.simTick) + static_cast<int64_t>(event.skippedTicks)
		     - static_cast<int64_t>(event.laneTick) - 1;
	}

	// ⛔ THE OPERANDS STAY int64_t UNTIL AFTER THIS SUBTRACTION.
	static constexpr uint32_t laneTickAcrossOffset(uint32_t simTick, const LaneAxisEvent& event)
	{
		return static_cast<uint32_t>(static_cast<int64_t>(simTick) - offsetAfter(event));
	}

	const LaneAxisEvent& entry(std::size_t index) const
	{
		return m_ledger[(m_ledgerFirst + index) % kLaneElisionLedgerCapacity];
	}

	// The span ends at the first tick admitted after it, and collapses to ONE lane tick,
	// which stays empty in both lanes and is what the marker is drawn on.
	void closeSpan(uint32_t resumeSimTick)
	{
		const uint32_t skipped        = resumeSimTick - m_spanFirstSimTick;
		const uint32_t markerLaneTick = static_cast<uint32_t>(
			static_cast<int64_t>(m_spanFirstSimTick) - m_offset);

		file(LaneAxisEvent{ LaneAxisEventKind::Elision, m_spanFirstSimTick, 0u, skipped, markerLaneTick });

		m_offset += static_cast<int64_t>(skipped) - 1;
		m_paused = false;
	}

	// The marker claims the OLD epoch's next lane tick, from `m_lastSimTick` before
	// admit() overwrites it for the new epoch.
	void fileAxisBreak(uint32_t simTick, uint32_t fromSimTick)
	{
		if (m_paused)
			closeSpan(fromSimTick + 1u);

		const uint32_t markerLaneTick =
			static_cast<uint32_t>(static_cast<int64_t>(m_lastSimTick) - m_offset) + 1u;

		file(LaneAxisEvent{ LaneAxisEventKind::Resync, simTick, fromSimTick, 0u, markerLaneTick });

		m_offset = static_cast<int64_t>(simTick) - static_cast<int64_t>(markerLaneTick) - 1;
	}

	void file(LaneAxisEvent event)
	{
		if (m_ledgerSize == kLaneElisionLedgerCapacity)
		{
			m_ledgerFirst   = (m_ledgerFirst + 1u) % kLaneElisionLedgerCapacity;
			m_ledgerDropped = true;
		}
		else
		{
			++m_ledgerSize;
		}

		m_ledger[(m_ledgerFirst + m_ledgerSize - 1u) % kLaneElisionLedgerCapacity] = event;
	}

	std::array<LaneAxisEvent, kLaneElisionLedgerCapacity> m_ledger{};
	std::size_t                                          m_ledgerFirst   = 0u;
	std::size_t                                          m_ledgerSize    = 0u;
	bool                                                 m_ledgerDropped = false;

	// laneTick = simTick - m_offset, for every tick after the newest event. SIGNED: a
	// backward resync runs the lane tick ahead of the sim tick.
	int64_t m_offset = 0;

	uint32_t m_inactiveRun      = 0u;
	uint32_t m_spanFirstSimTick = 0u;
	bool     m_paused           = false;

	uint32_t      m_lastSimTick    = 0u;
	bool          m_hasLastSimTick = false;
	LaneAdmission m_lastAdmission  = LaneAdmission::Recorded;
};

// ---------------------------------------------------------------------------
// WHAT THE CLIENT'S CLOCK SAYS. The prediction tick the display's own axis is at, and
// the estimator's offset -- the number of ticks the target sits above authority.
// ⛔ THE OFFSET IS READ FROM THE ESTIMATOR, NEVER RE-DERIVED HERE.
// ---------------------------------------------------------------------------

// The client's clock and its offset, read as ONE pair so the two cannot be a frame apart.
struct PredictionOffsetReading
{
	uint32_t predictionTick = 0u;
	uint32_t offsetTicks    = 0u;
};

// prediction - offset, floored at tick 0. ⛔ THE ONE DERIVATION OF THE AUTHORITY TICK.
constexpr uint32_t authorityTickOf(const PredictionOffsetReading& reading)
{
	return (reading.predictionTick >= reading.offsetTicks)
	           ? reading.predictionTick - reading.offsetTicks
	           : 0u;
}

// The poll's own delay decomposition, paired with the tick it was taken at, for the
// same reason the authority reading is paired with one: ⛔ ONE SNAPSHOT, TAKEN ONCE.
struct InputDelayReading
{
	InputDelayDecomposition decomposition;
	uint32_t                simTick = 0u;
};

// The residency reading, paired with the tick it was taken at, for the same reason.
struct ResidencyReading
{
	WindowResidency residency;
	uint32_t        simTick = 0u;
};

// The client's clock as ONE poll read it: the two ticks whose difference IS the drift,
// what the clock would do next, the tier-transition debt it is paying, and the poll's
// own tick. Skips and stalls cannot be seen as events from here -- the drift that drives
// them can, and this is that state.
// ⛔ ONE SNAPSHOT, TAKEN ONCE, exactly like the three readings above.
struct ClockDriftReading
{
	uint32_t predictionTick = 0u;
	uint32_t targetTick     = 0u;   // NetworkTimeEstimator::getTargetPredictionTick
	uint32_t authorityTick  = 0u;   // NetworkTimeEstimator::getLastAuthorityTick
	int32_t  driftTicks     = 0;    // int32(targetTick) - int32(predictionTick)

	// What advancePrediction() WOULD do next -- ClientPredictionClock::evaluateDrift().
	ClientPredictionClock::DriftAction pendingAction = ClientPredictionClock::DriftAction::None;

	uint32_t stallDebtTicks = 0u;   // getRequiredInputDelayIncreaseStallTicks()

	// STAMPED BY noteClockDriftReading from the poll's own tick, exactly as the three
	// readings above are. ⛔ A CALLER-SUPPLIED VALUE HERE IS OVERWRITTEN, NOT TRUSTED.
	uint32_t simTick = 0u;
};

// ---------------------------------------------------------------------------
// All three lanes plus the ONE tick axis they are read on, so a vertical slice through
// the bars is the same capture tick in each. The axis is monotone: a lane read must not
// walk backwards because one frame's clock read did.
//
// Once anything has been elided a lane tick is no longer a sim tick.
// ⛔ THE AXIS IS IN LANE TICKS, AND ONLY THE GATE ABOVE CONVERTS BETWEEN THE TWO.
// ---------------------------------------------------------------------------
class InputHistoryTickLanes
{
public:
	// ⛔ THE ONE GATE. Every lane is written only through the decision it returns.
	const LaneIdleGate& gate() const { return m_gate; }
	LaneIdleGate&       editGate() { return m_gate; }

	const ProvenanceLane&   provenance() const { return m_provenance; }
	ProvenanceLane&         editProvenance() { return m_provenance; }
	const MachineStateLane& machineState() const { return m_machineState; }
	MachineStateLane&       editMachineState() { return m_machineState; }
	const InputDelayLane&   delay() const { return m_delay; }
	InputDelayLane&         editDelay() { return m_delay; }

	bool     hasAxis() const { return m_hasAxis; }
	// Precondition: hasAxis().
	uint32_t newestAxisTick() const { return m_newestAxisTick; }

	void noteAxisTick(uint32_t tick)
	{
		if (!m_hasAxis || tick > m_newestAxisTick)
		{
			m_newestAxisTick = tick;
			m_hasAxis        = true;
		}
	}
	// The clock reading the poll took, paired with the very tick that poll built the axis
	// from. ⛔ ONE SNAPSHOT, TAKEN ONCE: a reading taken again at draw time would measure
	//   the marker against an axis it never saw, moving the column while the offset held.
	void noteAuthorityReading(std::optional<uint32_t> offsetTicks, uint32_t atSimTick)
	{
		m_authority = offsetTicks.has_value()
		                  ? std::optional<PredictionOffsetReading>(
		                        PredictionOffsetReading{ atSimTick, *offsetTicks })
		                  : std::nullopt;
	}

	// The poll's reading, or nullopt on a role that does not predict and on lanes nothing
	// has polled. ⛔ THE DRAW HOLDS THESE LANES AS const, so it cannot file one of its own.
	const std::optional<PredictionOffsetReading>& authorityReading() const { return m_authority; }

	// Beside noteAuthorityReading, for the same reason: filed above the gate's early
	// return, so a paused display still shows the last decomposition it saw.
	void noteDelayReading(std::optional<InputDelayDecomposition> decomposition, uint32_t atSimTick)
	{
		m_delayReading = decomposition.has_value()
		                     ? std::optional<InputDelayReading>(
		                           InputDelayReading{ *decomposition, atSimTick })
		                     : std::nullopt;
	}

	// ⛔ ONLY A POLL CAN FILE ONE -- the draw holds these lanes as const.
	const std::optional<InputDelayReading>& delayReading() const { return m_delayReading; }

	// Beside the authority and delay readings, filed above the gate for the same reason: a
	// frozen residency edge would draw long-evicted cells as still live while idle.
	void noteResidencyReading(const WindowResidency& residency, uint32_t atSimTick)
	{
		m_residencyReading = ResidencyReading{ residency, atSimTick };
	}

	// ⛔ ONLY A POLL CAN FILE ONE -- the draw holds these lanes as const.
	const std::optional<ResidencyReading>& residencyReading() const { return m_residencyReading; }

	// Beside the three readings above and filed above the gate for the same reason: a
	// paused display must still show the clock. The static run is why the reading is kept
	// rather than merely passed through -- an authority tick that stops moving is what a
	// resync storm looks like from the client, and no single reading can show it.
	//
	// It ACCUMULATES the ticks the client actually simulated, and is not the difference
	// between this tick and the one the run began on. A resync ASSIGNS the prediction
	// tick backwards, so during a storm the client's tick number is confined to a 22-tick
	// loop and that difference stays small however long the authority has been frozen.
	// ⛔ SIM TICKS OF THE CLIENT'S OWN CLOCK, NEVER POLLS: polls are render frames.
	void noteClockDriftReading(std::optional<ClockDriftReading> reading, uint32_t atSimTick)
	{
		if (!reading.has_value())
		{
			m_clockReading = std::nullopt;
			return;
		}

		reading->simTick = atSimTick;

		// A first reading has no predecessor to have been static against, so it starts
		// the run at zero rather than continuing one.
		if (!m_clockReading.has_value()
		    || m_clockReading->authorityTick != reading->authorityTick)
		{
			m_authorityStaticSimTicks = 0u;
		}
		else if (atSimTick > m_clockReading->simTick)
		{
			m_authorityStaticSimTicks += atSimTick - m_clockReading->simTick;
		}

		m_clockReading = reading;
	}

	// ⛔ ONLY A POLL CAN FILE ONE -- the draw holds these lanes as const.
	const std::optional<ClockDriftReading>& clockDriftReading() const { return m_clockReading; }

	// Sim ticks the client has simulated since the authority tick last moved. Meaningful
	// only beside a reading. ⛔ IT IS NOT A COUNT OF POLLS AND NOT A TICK NUMBER.
	uint32_t authorityStaticSimTicks() const { return m_authorityStaticSimTicks; }

	// The lineage of the capture at `tick`, or nullptr when no cell answers for it.
	const RowProvenanceSummary* provenanceAt(uint32_t tick) const { return m_provenance.find(tick); }

	// ⛔ TOTAL, and absence resolves to NotSampled rather than to a neighbour's value.
	MachineStateCell machineCellAt(uint32_t tick) const
	{
		const MachineStateCell* cell = m_machineState.find(tick);
		return (cell == nullptr) ? MachineStateCell::NotSampled : *cell;
	}

	// The delay verdict cell at `tick`, or nullptr when no cell answers for it.
	const InputDelayCell* delayCellAt(uint32_t tick) const { return m_delay.find(tick); }

private:
	LaneIdleGate     m_gate;
	ProvenanceLane   m_provenance;
	MachineStateLane m_machineState;
	InputDelayLane   m_delay;
	uint32_t         m_newestAxisTick = 0u;
	bool             m_hasAxis        = false;

	std::optional<PredictionOffsetReading> m_authority;
	std::optional<InputDelayReading>       m_delayReading;
	std::optional<ResidencyReading>        m_residencyReading;
	std::optional<ClockDriftReading>       m_clockReading;
	uint32_t                               m_authorityStaticSimTicks = 0u;
};

} // namespace brawlerInputHistoryVisualization

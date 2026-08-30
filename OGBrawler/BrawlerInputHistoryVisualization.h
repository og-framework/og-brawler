#pragma once
// SPDX-License-Identifier: BUSL-1.1

// The pure core of the input-history display: which direction glyph does one
// captured input draw?
//
//   * Pure. glm + inputSequence only. No simulation state, no rendering, no engine
//     types, no globals.
//   * The deadzone is a PARAMETER. The caller passes g_moveStickDeadzone, so the
//     glyph and the motion matcher gate on the same number by construction rather
//     than by two constants that happen to agree today.
//
// ---------------------------------------------------------------------------
// ⛔ THIS HEADER IS NOT A QUANTIZER, AND MUST NEVER BECOME ONE.
//
// The aim-relative angle convention, its deadzone gate, its degenerate-reference
// gate and its eight named angles ALL ALREADY EXIST, in inputSequence, and are
// exactly what the motion matcher matches its MotionStep targets against. This
// header only asks "which of those named angles is nearest". Re-deriving an atan2
// or a sector table here would let the drawn glyph disagree with the sector the
// matcher tested, which is the one failure this display exists to rule out.
// ---------------------------------------------------------------------------
// THE MODEL — nine buckets on an AIM-RELATIVE COMPASS, and the compass is the
// SCREEN picture: Forward is where the player is aiming, and Forward draws UP.
//
//   ForwardLeft  Forward  ForwardRight   Forward = the aim       (angle::Forward,     0)
//          Left  Neutral  Right          Left    = left-of-aim   (angle::Down,    +pi/2)
//      BackLeft     Back  BackRight      Back    = away from aim (angle::Back,       pi)
//                                        Right   = right-of-aim  (angle::Up,      -pi/2)
//
// These names disagree with inputSequence's on purpose. That header labels its
// constants on the Street Fighter numpad, which encodes a fighter facing screen-right
// and calls +pi/2 "Down"; this game aims freely, so the same angle is a SIDE. Which
// side is settled on kNamedDirections below, and it is not the side that header claims.
// ⚠ THE ANGLES ARE inputSequence'S OWN AND UNCHANGED — only the labels differ.
//
// Neutral is precisely inputSequence::aimRelativeAngle's nullopt: the stick is
// under the deadzone, or the reference forward has no XY direction to be relative
// to. Both of those questions are already answered there; neither is re-asked here.
//
// Every other input takes the nearest of the eight named angles by
// inputSequence::angularDistance. Nearest-of-eight puts the sector edges at the
// pi/8 midpoints, which is the classic 45-degree sector the matcher already
// performs with a pi/8 MotionStep tolerance — so the sectors coincide because they
// are the same construction, not because two tables were kept in step by hand.
// ---------------------------------------------------------------------------

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include "OGBrawler/InputSequence/InputSequence.h"
#include "OGSimulation/SimulationReconciliation.h"
#include "OGSimulation/SlotStateProvenance.h"

namespace brawlerInputHistoryVisualization
{

// The enumerator VALUE is the compass ORDINAL: Neutral leads at 0, then eight steps
// clockwise from Forward ON SCREEN. The named angles run the other way -- see below.
enum class DirectionBucket : uint8_t
{
	Neutral      = 0,
	Forward      = 1,
	ForwardRight = 2,
	Right        = 3,
	BackRight    = 4,
	Back         = 5,
	BackLeft     = 6,
	Left         = 7,
	ForwardLeft  = 8,
};

// Neutral plus the eight. A sweep that reaches this many has covered the compass.
inline constexpr std::size_t kDirectionBucketCount = 9u;

// One named angle and the bucket it draws as.
struct NamedDirection
{
	float           angle;
	DirectionBucket bucket;
};

// WHICH SIDE +pi/2 IS, AND WHY THIS TABLE CONTRADICTS THE HEADER IT READS FROM.
// InputSequence.h documents +pi/2 as "right-of-aim" and derives it from
// right = (fwd.y, -fwd.x), annotating that "right-hand z-up". The vectors reaching it
// are Unreal world vectors, copied component-for-component with no handedness change,
// and Unreal's own right vector is the OTHER perpendicular: the rotation carrying +X
// onto fwd carries +Y onto (-fwd.y, fwd.x). So (fwd.y, -fwd.x) is the character's
// LEFT, +pi/2 is left-of-aim, and pressing left must draw a LEFT arrow.
// ⛔ THAT SENTENCE IN InputSequence.h IS STALE. Do not "restore" this table to it.
//
// In increasing aim-relative angle from Forward. DECLARATION ORDER IS THE TIE-BREAK:
// an angle landing exactly on a pi/8 midpoint keeps the earlier entry.
inline constexpr NamedDirection kNamedDirections[] = {
	{ inputSequence::angle::Forward,     DirectionBucket::Forward      },
	{ inputSequence::angle::DownForward, DirectionBucket::ForwardLeft  },
	{ inputSequence::angle::Down,        DirectionBucket::Left         },
	{ inputSequence::angle::DownBack,    DirectionBucket::BackLeft     },
	{ inputSequence::angle::Back,        DirectionBucket::Back         },
	{ inputSequence::angle::UpBack,      DirectionBucket::BackRight    },
	{ inputSequence::angle::Up,          DirectionBucket::Right        },
	{ inputSequence::angle::UpForward,   DirectionBucket::ForwardRight },
};

// Nearest named angle to an ALREADY-RESOLVED aim-relative angle. Split out from
// directionBucketOf so the midpoint tie-break is pinnable on an exact pi/8 instead
// of on whatever a boundary stick happens to round to through atan2.
inline DirectionBucket nearestNamedDirection(float aimRelativeAngleRadians)
{
	DirectionBucket best         = kNamedDirections[0].bucket;
	float           bestDistance = inputSequence::angularDistance(
		aimRelativeAngleRadians, kNamedDirections[0].angle);

	for (const NamedDirection& candidate : kNamedDirections)
	{
		const float distance =
			inputSequence::angularDistance(aimRelativeAngleRadians, candidate.angle);

		// Strict `<` is what makes an exact midpoint keep the earlier entry.
		if (distance < bestDistance)
		{
			bestDistance = distance;
			best         = candidate.bucket;
		}
	}

	return best;
}

// The classification the display draws for one captured input.
//
// stick is the 2D move stick and referenceForwardXY the aim, in the same frames
// inputSequence::aimRelativeAngle takes them; deadzone is the caller's, never read
// from a global here — that is what keeps this header pure and testable.
inline DirectionBucket directionBucketOf(glm::vec2 stick,
                                         glm::vec3 referenceForwardXY,
                                         float     deadzone)
{
	const std::optional<float> aimRelative =
		inputSequence::aimRelativeAngle(stick, referenceForwardXY, deadzone);

	// nullopt IS the Neutral case, both halves of it. Do not re-test either here.
	if (!aimRelative.has_value())
		return DirectionBucket::Neutral;

	return nearestNamedDirection(*aimRelative);
}

// ---------------------------------------------------------------------------
// THE ROW MODEL — one row per held input state, run-length compressed.
//
// This is the state the display cannot get from anywhere else. LocalInputCache is
// kLocalInputCacheCapacityTicks (64) ticks, about 1.07 s at the 60 Hz tickFrequency,
// and the reference display spans 251 ticks and contains a SINGLE 99-tick row. A
// reader restricted to resident cache state would report that row as a truncated 64
// and be silently wrong — so tickCount below is a counter of its own and is
// deliberately NOT bounded by any source ring's capacity.
//
// The rows are folded from what the caches already hold. Nothing here re-records an
// input, and none of it is simulation state: it is client-local and diagnostics-only,
// never replicated, never in a correction payload, never in compute_checksum.
// ---------------------------------------------------------------------------

// What a row's ticks did once the simulation ran them. Unknown is the right default:
// never joined is genuinely unknown, which is NOT "pressed but not yet run".
//
// It types ONE capture tick's outcome AND a row's fold over its ticks: one ladder.
//
// ⭐ DECLARATION ORDER IS THE LADDER, ascending. The enumerator VALUE is the rank, so
// there is no second table that could drift out of step with the order below.
enum class RowProvenanceSummary : uint8_t
{
	// Nothing joined these ticks, or the join was the documented-ambiguous sentinel.
	Unknown = 0,

	// Pressed, not yet run: no cache slot for the tick. Normal inside the input delay.
	Pending,

	// A slot exists and nothing has written state for its tick.
	NoStateWritten,

	// Ran as a prediction and no authority has spoken on it. The ordinary client state.
	RanUnconfirmed,

	// The authority named this capture, but its lineage could not be read.
	LineageUnavailable,

	// The authority certified the prediction, so the state copy was skipped.
	Confirmed,

	// The authority disagreed and its state was adopted.
	Corrected,

	// Re-run during a rollback.
	Resimulated,

	// ⛔ MUST NEVER APPEAR, and that is exactly why it is representable. Zero occurrences
	// is the assertion, not the omission — SlotStateProvenance.h argues it at length.
	ProvenanceLie,
};

// For the ladder sweep. Adding an enumerator without extending the fold fails a test
// rather than silently landing on a default.
inline constexpr uint8_t kRowProvenanceSummaryCount = 9u;

// One drawn row: a run of consecutive capture ticks that all pressed the same thing.
//
// A field that split a row without being drawn would make its tick count unexplainable.
// ⛔ ROW IDENTITY IS EXACTLY WHAT THE PANEL DRAWS: direction and buttons, nothing else.
struct InputHistoryRow
{
	DirectionBucket direction = DirectionBucket::Neutral;

	// THE EXISTING motionButtonMask convention: bit 0 = attackLeft, bit 1 = attackRight.
	uint8_t buttonMask = 0u;

	uint32_t firstCaptureTick = 0u;

	// CAPTURE TICKS, unclamped on purpose: the 99-tick reference row outlives the cache.
	uint32_t tickCount = 0u;

	// Every row opens holding one tick, so this is valid on any row in the ring.
	uint32_t lastCaptureTick() const { return firstCaptureTick + tickCount - 1u; }
};

// What one appendCapture call did. The two Ignored cases are no-ops but NOT one fact.
enum class AppendResult : uint8_t
{
	OpenedRow,
	ExtendedRow,
	IgnoredDuplicate,
	IgnoredStale,
};

// The reference display's 251 ticks (4.2 s at the 60 Hz tickFrequency) fold into 19
// rows, so 64 rows carries >3x that; its floor, one row per tick, is still 64 ticks.
inline constexpr std::size_t kInputHistoryRowCapacity = 64u;

// ---------------------------------------------------------------------------
// The consumer polls a 64-slot source ring at render-frame rate, so on nearly every
// frame it re-presents ticks this ring has already folded; a merge that counted them
// twice would inflate tickCount without bound and every number on the display would
// be wrong. Idempotence is therefore a correctness requirement, not a nicety.
// ⭐ appendCapture IS IDEMPOTENT ON THE CAPTURE TICK.
// ---------------------------------------------------------------------------

// A bounded ring of rows, oldest first. Pure: one fixed array and two indices.
class InputHistoryRowRing
{
public:
	static constexpr std::size_t capacity() { return kInputHistoryRowCapacity; }

	std::size_t size() const { return m_size; }
	bool        empty() const { return m_size == 0u; }

	// Index 0 is the OLDEST resident row. Precondition: indexOldestFirst < size().
	const InputHistoryRow& at(std::size_t indexOldestFirst) const
	{
		return m_rows[(m_begin + indexOldestFirst) % kInputHistoryRowCapacity];
	}

	const InputHistoryRow& oldest() const { return at(0u); }
	const InputHistoryRow& newest() const { return at(m_size - 1u); }

	// Extends the newest row when direction and button mask both match AND captureTick is
	// that row's next tick; otherwise opens a row, evicting the oldest.
	AppendResult appendCapture(uint32_t        captureTick,
	                           DirectionBucket direction,
	                           uint8_t         buttonMask)
	{
		if (m_size == 0u)
		{
			return openRow(captureTick, direction, buttonMask);
		}

		const InputHistoryRow& current = newest();

		// At or before the newest row's last tick has already been folded. Keyed on the
		// TICK ALONE, never the fields: that is what makes a re-poll a no-op, not a recount.
		if (captureTick <= current.lastCaptureTick())
		{
			// Behind that span is stale, and must NOT rejoin whichever older row covers it.
			return captureTick >= current.firstCaptureTick ? AppendResult::IgnoredDuplicate
			                                               : AppendResult::IgnoredStale;
		}

		const bool contiguous = captureTick == current.lastCaptureTick() + 1u;
		const bool sameInput  = current.direction == direction && current.buttonMask == buttonMask;

		if (contiguous && sameInput)
		{
			// The unbounded counter: no clamp here, by design.
			++newestMutable().tickCount;
			return AppendResult::ExtendedRow;
		}

		// A gap opens a row even on identical input: folding across it overstates the hold.
		return openRow(captureTick, direction, buttonMask);
	}

private:
	InputHistoryRow& newestMutable()
	{
		return m_rows[(m_begin + m_size - 1u) % kInputHistoryRowCapacity];
	}

	AppendResult openRow(uint32_t captureTick, DirectionBucket direction, uint8_t buttonMask)
	{
		if (m_size == kInputHistoryRowCapacity)
		{
			// Drop the OLDEST by advancing the base; that never touches the newest row.
			m_begin = (m_begin + 1u) % kInputHistoryRowCapacity;
			--m_size;
		}

		InputHistoryRow opened;
		opened.direction        = direction;
		opened.buttonMask       = buttonMask;
		opened.firstCaptureTick = captureTick;
		opened.tickCount        = 1u;

		m_rows[(m_begin + m_size) % kInputHistoryRowCapacity] = opened;
		++m_size;

		return AppendResult::OpenedRow;
	}

	std::array<InputHistoryRow, kInputHistoryRowCapacity> m_rows{};
	std::size_t                                           m_begin = 0u;
	std::size_t                                           m_size  = 0u;
};


// ---------------------------------------------------------------------------
// THE PRESSED-VS-RAN JOIN — what the simulation actually did with each capture.
//
// The correction cache exposes a FORWARD map: applied tick -> which capture was
// applied there. The display needs the reverse — "my capture at tick T: did the
// simulation run it, at which applied tick, and what was that tick's lineage?" — so
// the resident window is inverted once per poll and each row folds over its own ticks.
//
// TWO EXISTING DIAGNOSTIC SEAMS FEED ONE OBSERVATION, both asked at the same tick:
//   * SimulationReconciliation::getAppliedCaptureTickRef  -> AppliedCaptureRef
//   * reconciliation.getDiagnostics().slotStateProvenance -> optional<SlotStateProvenance>
// The display asks at its own capture ticks, so an observation that names no capture
// speaks for the tick it was asked about. Only the Ref arm can name a DIFFERENT capture,
// and re-filing it under that capture tick is the inversion.
//
// Nothing folded here may feed a simulation decision, enter a correction payload, or
// reach compute_checksum; SlotStateProvenance.h's fence 2 is machine-checked and this
// code sits outside it.
// ⛔ THIS IS A DIAGNOSTIC READ AND MUST STAY ONE.
//
// NO INPUT ROW CARRIES A SUMMARY ANY MORE, and the reason is a measured one. Input
// identity and lineage vary on DIFFERENT AXES: input changes when the player changes what
// is pressed, lineage changes per tick from network events. Compressing runs on input
// identity therefore destroys lineage resolution, and a worst-case-wins merge never
// recovers -- one resimulated tick inside a 99-tick hold makes all 99 read as resimulated.
// ⛔ A READER OF THIS FOLD MUST BE KEYED ON THE CAPTURE TICK, never on a run of input.
// ---------------------------------------------------------------------------

// Worst-case-wins uses the enumerator VALUE as the rank — see the ladder on the enum.
constexpr uint8_t rowProvenanceRank(RowProvenanceSummary summary)
{
	return static_cast<uint8_t>(summary);
}

// The worse — i.e. the more reportable — of two summaries.
constexpr RowProvenanceSummary worseRowProvenance(RowProvenanceSummary left,
                                                 RowProvenanceSummary right)
{
	return rowProvenanceRank(left) >= rowProvenanceRank(right) ? left : right;
}

// One slot's lineage in display vocabulary, TOTAL over SlotStateProvenance. Every
// enumerator has its own summary; a sweep over kSlotStateProvenanceCount pins that.
constexpr RowProvenanceSummary summaryOfSlotProvenance(SlotStateProvenance provenance)
{
	switch (provenance)
	{
	case SlotStateProvenance::Empty:                         return RowProvenanceSummary::NoStateWritten;
	case SlotStateProvenance::Predicted:                     return RowProvenanceSummary::RanUnconfirmed;
	case SlotStateProvenance::AuthorityAdopted:              return RowProvenanceSummary::Corrected;
	case SlotStateProvenance::AuthorityAgreedKeptPrediction: return RowProvenanceSummary::Confirmed;
	case SlotStateProvenance::Replayed:                      return RowProvenanceSummary::Resimulated;
	case SlotStateProvenance::ReplayedOverCorrection:        return RowProvenanceSummary::ProvenanceLie;
	}

	// Outside the enumeration is what a torn cross-thread lineage byte looks like.
	return RowProvenanceSummary::Unknown;
}

// ---------------------------------------------------------------------------
// ⛔ FOUR ARMS, AND NO TWO OF THEM MAY COLLAPSE. AppliedCaptureRefKind is consumed
// directly; nothing here re-derives these states from a raw capture tick.
//
//   NoSlot   -- no slot for the tick: pressed, not yet run. Normal, not an error.
//   NoRef    -- the slot exists and no correction ever landed. The ORDINARY client
//               steady state, and the only arm a Replayed lineage can reach the
//               display through: protect-all-corrected forbids a replay writing a
//               slot whose correction bit is set, so Ref and Sentinel cannot carry it.
//   Sentinel -- a correction landed naming NO capture; documented ambiguous.
//   Ref      -- a correction landed naming a real capture; its lineage answers.
//
// Provenance refines only the two arms whose lineage describes the capture asked
// about. NoSlot has no slot to have a lineage; a Sentinel's slot ran some other
// capture, so its lineage may not be attributed to this one.
// ---------------------------------------------------------------------------
constexpr RowProvenanceSummary captureSummaryOf(AppliedCaptureRefKind              kind,
                                                std::optional<SlotStateProvenance> provenance)
{
	switch (kind)
	{
	case AppliedCaptureRefKind::NoSlot:
		return RowProvenanceSummary::Pending;

	case AppliedCaptureRefKind::Sentinel:
		return RowProvenanceSummary::Unknown;

	case AppliedCaptureRefKind::NoRef:
		// nullopt is absence, never a lineage value: no lineage is claimed for the tick.
		return provenance.has_value() ? summaryOfSlotProvenance(*provenance)
		                              : RowProvenanceSummary::RanUnconfirmed;

	case AppliedCaptureRefKind::Ref:
		return provenance.has_value() ? summaryOfSlotProvenance(*provenance)
		                              : RowProvenanceSummary::LineageUnavailable;
	}

	return RowProvenanceSummary::Unknown;
}

// One resident correction-cache slot, exactly as the two seams answer for it.
struct AppliedSlotObservation
{
	uint32_t                           appliedTick = 0u;
	AppliedCaptureRef                  ref{};
	std::optional<SlotStateProvenance> provenance{};
};

// One inverted entry: what became of the capture at captureTick.
struct CaptureJoin
{
	uint32_t             captureTick = 0u;
	uint32_t             appliedTick = 0u;
	RowProvenanceSummary summary     = RowProvenanceSummary::Unknown;

	// True only on the Ref arm, where the AUTHORITY named the capture. Every other arm
	// names none, so its entry speaks for the tick it was asked about.
	bool authorityNamed = false;

	// True only on the Sentinel arm: the server substituted, naming no capture at all.
	bool sentinel = false;
};

// StateCorrectionCache::StateBufferSize — the resident window the seams can answer for.
// A test pins this against that constant rather than trusting the literal here.
inline constexpr std::size_t kAppliedCaptureInversionCapacity = 60u;

// The inverted map, captureTick -> { appliedTick, summary }. One fixed array and a
// linear scan: it holds one resident window, is rebuilt per poll and never allocates.
class AppliedCaptureInversion
{
public:
	static constexpr std::size_t capacity() { return kAppliedCaptureInversionCapacity; }

	std::size_t size() const { return m_size; }
	bool        empty() const { return m_size == 0u; }
	void        clear() { m_size = 0u; }

	// Entry order is observation order, not tick order. Precondition: index < size().
	const CaptureJoin& at(std::size_t index) const { return m_entries[index]; }

	const CaptureJoin* find(uint32_t captureTick) const
	{
		for (std::size_t index = 0u; index < m_size; ++index)
		{
			if (m_entries[index].captureTick == captureTick)
				return &m_entries[index];
		}

		return nullptr;
	}

	// Files one observation under the capture tick it speaks for. False only when the
	// window is full, which sizing to the resident window makes unreachable in the poll.
	bool observe(const AppliedSlotObservation& observation)
	{
		const bool authorityNamed = observation.ref.kind == AppliedCaptureRefKind::Ref;
		const bool sentinel       = observation.ref.kind == AppliedCaptureRefKind::Sentinel;

		CaptureJoin entry;
		entry.captureTick    = authorityNamed ? observation.ref.captureTick : observation.appliedTick;
		entry.appliedTick    = observation.appliedTick;
		entry.summary        = captureSummaryOf(observation.ref.kind, observation.provenance);
		entry.authorityNamed = authorityNamed;
		entry.sentinel       = sentinel;

		CaptureJoin* existing = findMutable(entry.captureTick);
		if (existing != nullptr)
		{
			// An authority-named join outranks an assumed same-tick one; equals fold worst-wins.
			if (authorityNamed && !existing->authorityNamed)
				*existing = entry;
			else if (authorityNamed == existing->authorityNamed)
				existing->summary = worseRowProvenance(existing->summary, entry.summary);

			return true;
		}

		if (m_size == kAppliedCaptureInversionCapacity)
			return false;

		m_entries[m_size] = entry;
		++m_size;
		return true;
	}

private:
	CaptureJoin* findMutable(uint32_t captureTick)
	{
		for (std::size_t index = 0u; index < m_size; ++index)
		{
			if (m_entries[index].captureTick == captureTick)
				return &m_entries[index];
		}

		return nullptr;
	}

	std::array<CaptureJoin, kAppliedCaptureInversionCapacity> m_entries{};
	std::size_t                                               m_size = 0u;
};

// ---------------------------------------------------------------------------
// WHAT ONE SWEEP OF THE RESIDENT WINDOW LEARNED ABOUT THE RING ITSELF -- derived from
// the same inversion sweep above; never a second read of either seam.
// ---------------------------------------------------------------------------

// SIM ticks throughout. `hasCache` is the one field this sweep cannot answer itself.
struct WindowResidency
{
	bool     hasCache       = false;  // supplied by the caller -- the reader's presence test
	bool     anyResident    = false;  // some tick in the window answered other than NoSlot
	uint32_t oldestResident = 0u;     // min such tick; meaningful iff anyResident
	// ⛔ WINDOW-CLAMPED AT THE POLL TICK, never the ring's own frontier.
	uint32_t newestResident = 0u;     // max such tick; meaningful iff anyResident
};

// Why a NoSlot answer at one tick means what it means, once the window's own residency
// is known. This classifies the ABSENCE; RowProvenanceSummary classifies a lineage.
enum class NoSlotCause : uint8_t
{
	NoCache,              // this id has no correction cache: the authority role
	Unclassifiable,       // a cache exists but nothing in the window is resident
	NotYetRun,            // t > newestResident: the frontier's genuine "pressed, not yet run"
	Evicted,              // t < oldestResident: recycled by age
	MissingInsideWindow,  // oldestResident <= t <= newestResident: a wipe, or the push race mid-sweep
};

// For the classification sweep. A cause added without extending a fold fails a test.
inline constexpr uint8_t kNoSlotCauseCount = 5u;

// TOTAL, and the rules are applied IN THIS ORDER:
//   !hasCache                -> NoCache          (residency fields mean nothing without a cache)
//   !anyResident             -> Unclassifiable   (oldest/newest are unset; comparing would be a guess)
//   t > newestResident       -> NotYetRun
//   t < oldestResident       -> Evicted
//   otherwise                -> MissingInsideWindow
// ⚠ ORDER IS LOAD-BEARING -- an unset bound compared against would misclassify. Do not reorder.
constexpr NoSlotCause classifyNoSlot(uint32_t t, const WindowResidency& r)
{
	if (!r.hasCache)
		return NoSlotCause::NoCache;

	if (!r.anyResident)
		return NoSlotCause::Unclassifiable;

	if (t > r.newestResident)
		return NoSlotCause::NotYetRun;

	if (t < r.oldestResident)
		return NoSlotCause::Evicted;

	return NoSlotCause::MissingInsideWindow;
}

// What the RESIDENT window alone says about one row: worst-case-wins over the capture
// ticks inside its span, Unknown when no resident entry falls in it.
//
// The scan walks the inversion rather than the row's ticks: a row is unbounded by
// design and the window is one cache's worth, so this is bounded by the window.
inline RowProvenanceSummary residentRowSummary(const AppliedCaptureInversion& inversion,
                                               const InputHistoryRow&         row)
{
	// A row outside the ring can hold no ticks, and lastCaptureTick would then wrap.
	if (row.tickCount == 0u)
		return RowProvenanceSummary::Unknown;

	RowProvenanceSummary summary  = RowProvenanceSummary::Unknown;
	const uint32_t       lastTick = row.lastCaptureTick();

	for (std::size_t index = 0u; index < inversion.size(); ++index)
	{
		const CaptureJoin& join = inversion.at(index);
		if (join.captureTick >= row.firstCaptureTick && join.captureTick <= lastTick)
			summary = worseRowProvenance(summary, join.summary);
	}

	return summary;
}

} // namespace brawlerInputHistoryVisualization

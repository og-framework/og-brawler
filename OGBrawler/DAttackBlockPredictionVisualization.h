#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <vector>
#include <algorithm>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "glm/ext/scalar_constants.hpp"
#include "DAttackRadialSimulation.h"
#include "DAttackCircle.h"
#include "DAttackVisualizationUtils.h"
#include "OGSimulation/SpatialQueryAdapter.h"
#include "OGSimulation/SpatialQueryResult.h"
#include "OGSimulation/QueryGeometry.h"

#pragma optimize( "", off )

// Attacker-side block-prediction visualization. Renders a fading orange "attack
// area" pie slice (colorId 6) from the attacker's circle CENTER outward — always
// drawn, even with no defenders in range — plus a shorter blue slice (colorId 2)
// over the wedge base, drawn iff the sim's shared wouldGuardBlock predicate says
// ANY defender in range would block the pending attack (binary signal — no
// per-defender geometry; the Phase 1 blue attacker→defender line is retired).
// The fade is a concentric arc taper: three nested translucent drawSegmentSolid
// layers whose overlap compounds the per-call alpha, brightest at the center and
// fading outward. PIE-tuned (P2.2-tune): the wedge center is aim for the forward
// sequence but aim ± 5π/12 for the side sequences (the arc spans from the
// orthogonal, aim ± π/2, curving toward aim) — a LOCAL direction; the shared
// computeAttackIndicatorGeometry helper the aim viz also consumes is untouched.
//
// Templated on the READER physics-body adapter concept (PhysicsBodyReaderAdapterType,
// game-thread-safe const reads via ChaosPhysicsBodyReaderAdapter) alongside the
// SpatialQueryAdapterType / RendererFunctorType idiom the two target-viz files use.
namespace dAttackBlockPredictionVisualization
{

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class State
{
public:
	State(const std::vector<QueryVolumeId>& volumeIds)
		: m_volumeIds(volumeIds)
	{}

	State(const State& other)
		: m_volumeIds(other.m_volumeIds)
	{}

	const std::vector<QueryVolumeId>& getVolumeIds() const { return m_volumeIds; }

private:
	State() = default;

	std::vector<QueryVolumeId> m_volumeIds;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyReaderAdapterType, typename SpatialQueryAdapterType, typename RendererFunctorType>
class Input
{
public:
	Input(float deltaTime,
		const glm::vec3& aimDirection,
		const glm::vec2& moveStick,
		const glm::vec3& moveDirectionWorld,
		const PhysicsBodyReaderAdapterType& physicsReader,
		SpatialQueryAdapterType& queryAdapter,
		RendererFunctorType rendererFunctorImpl)
		: m_deltaTime(deltaTime)
		, aimDirection(aimDirection)
		, moveStick(moveStick)
		, moveDirectionWorld(moveDirectionWorld)
		, m_physicsReader(physicsReader)
		, m_queryAdapter(queryAdapter)
		, rendererFunctorImpl(rendererFunctorImpl)
	{}

	float getDeltaTime() const { return m_deltaTime; }
	const glm::vec3& getAimDirection() const { return aimDirection; }
	const glm::vec2& getMoveStick() const { return moveStick; }
	const glm::vec3& getMoveDirectionWorld() const { return moveDirectionWorld; }
	const PhysicsBodyReaderAdapterType& getPhysicsReader() const { return m_physicsReader; }
	SpatialQueryAdapterType& getQueryAdapter() const { return m_queryAdapter; }
	RendererFunctorType getRendererFunctorImpl() const { return rendererFunctorImpl; }

private:
	float m_deltaTime;
	const glm::vec3 aimDirection;
	const glm::vec2 moveStick;
	const glm::vec3 moveDirectionWorld;
	const PhysicsBodyReaderAdapterType& m_physicsReader;
	SpatialQueryAdapterType& m_queryAdapter;
	RendererFunctorType rendererFunctorImpl;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyReaderAdapterType, typename SpatialQueryAdapterType, typename RendererFunctorType>
void visualize(const Input<PhysicsBodyReaderAdapterType, SpatialQueryAdapterType, RendererFunctorType>& input,
	State& state,
	const dAttackRadialSimulation::StaticData& staticData,
	const dAttackRadialSimulation::State& radialState)
{
	auto rendererFunctor = input.getRendererFunctorImpl();

	// (0) Hide the entire viz while the sim is actively running an attack sequence.
	// The attack-direction indicator and block-prediction arc are both "what would happen
	// if you attacked NOW" indicators; while an actual attack is in-progress they're
	// misleading. The ongoing swing itself is shown by DAttackRadialVisualization.
	if (isRealAttackSequence(radialState.currenSequenceId))
		return;

	// (1) Attacker root — the weapon body position, same source the sim uses for
	// `rootTranslation` (State::bodyState.position). The full transform (with rotation)
	// parents the shared query volumes.
	const glm::vec3 attackerRoot = radialState.bodyState.position;
	const glm::mat4 rootTransform = glm::translate(glm::mat4(1.f), attackerRoot)
	                               * glm::mat4_cast(radialState.bodyState.rotation);

	// (2) Attack-indicator geometry — the predicted attack direction (the wedge's angular
	// center) plus the predicted-from-Idle sequence id (4/1/0), shared with the aim viz so
	// the two indicators cannot drift on angular position or predicted sequence.
	const float innerRadius = staticData.getAttackCircle().getInnerRadius();
	const float outerRadius = staticData.getAttackCircle().getOuterRadius();
	const dAttackVisualizationUtils::AttackIndicatorGeometry attackGeometry =
		dAttackVisualizationUtils::computeAttackIndicatorGeometry(
			input.getAimDirection(),
			input.getMoveStick(),
			input.getMoveDirectionWorld(),
			attackerRoot,
			innerRadius);

	// (3) Wedge center direction (PIE-tuned, P2.2-tune) — for the FORWARD sequence the
	// wedge is centered on the predicted attack direction (== aim). For the SIDE
	// sequences the wedge is shifted outward so its arc STARTS at the orthogonal
	// (aim ± π/2) and curves toward aim: center at aim ± 5π/12, so the π/6-wide wedge
	// spans [aim ± π/3, aim ± π/2]. Positive rotation about +Z is CCW from above,
	// matching computeAttackIndicatorGeometry's threshold-rotation convention.
	const glm::vec3 aimDirectionXY = glm::normalize(
		glm::vec3(input.getAimDirection().x, input.getAimDirection().y, 0.f));
	glm::vec3 wedgeCenterDirectionXY = attackGeometry.attackDirectionXY;
	if (attackGeometry.kind != dAttackVisualizationUtils::AttackDirectionKind::Forward)
	{
		const bool isLeft = attackGeometry.kind == dAttackVisualizationUtils::AttackDirectionKind::Left;
		const float wedgeCenterOffset = (isLeft ? 1.f : -1.f) * 5.f * glm::pi<float>() / 12.f;
		const glm::mat4 wedgeRot = glm::rotate(glm::mat4(1.f), wedgeCenterOffset, glm::vec3(0.f, 0.f, 1.f));
		wedgeCenterDirectionXY = glm::vec3(wedgeRot * glm::vec4(aimDirectionXY, 0.f));
	}

	// (4) Fading attack area — drawn ALWAYS, before the empty-report early-return below,
	// so the baseline scenario (idle attacker, no defenders in range) still shows the area.
	//
	// PIE-tuned radially-non-overlapping segment structure. Each segment has its OWN
	// per-call alpha so opacity is tweakable per band without side effects on others
	// (previous K=3 nested-layer design had alphas compound at overlap, coupling tweaks
	// across bands).
	//
	// The segment layout is DIFFERENT for forward vs left/right predicted sequences:
	//   - Forward (seq 4): only the peripheral [0, innerRadius] region is rendered.
	//     Brighter than the L/R peripheral to make the forward indicator distinct
	//     without needing an outer swing band. Rationale: a forward attack is aimed at
	//     whoever is DIRECTLY in front — the peripheral fills the sector between the
	//     character and the swing itself; no outer arc needed to convey direction.
	//   - Left/Right (seq 1/0): four-band structure covering peripheral + swing band.
	//     The swing band (segments 3, 4) needs to be visible because the L/R sector
	//     starts at the orthogonal (±5π/12 from aim) — far from the aim direction, so
	//     the peripheral alone wouldn't clearly identify the sector.
	const float areaAngularWidth = glm::pi<float>() / 6.f;	// TOTAL angle (±π/12 per side), uniform for all predicted sequences.
	const glm::vec3 areaNormal(0.f, 0.f, 1.f);				// flat ground-plane wedge, matching the wedge direction's XY-plane semantics

	// PIE-tuned segment radii (in cm; assume innerRadius = 90, outerRadius = 300):
	//   peripheralMid  = 45   — split within the peripheral [0, innerRadius] region
	//   swingBandMid   = 125  — end of primary swing band (segment 3 outer edge; also blue arc outer edge)
	//   swingBandOuter = 160  — reach-limit fade edge (segment 4 outer edge)
	const float peripheralMid  = innerRadius * 0.5f;
	const float swingBandMid   = 125.f;
	const float swingBandOuter = 160.f;

	struct WedgeSegment
	{
		float innerR;
		float outerR;
		unsigned int alpha;   // 0..255; per-call, passed straight through drawSegmentSolid → drawMesh
	};
	if (attackGeometry.kind == dAttackVisualizationUtils::AttackDirectionKind::Forward)
	{
		// Forward: peripheral (bright) + main swing-band segment 3 (dim). No segment 4.
		// User rationale: forward attack visually points straight at whoever is in
		// front; the peripheral is the primary read, and a dim swing band segment
		// gives just enough hint of reach without cluttering the aim direction.
		const WedgeSegment forwardSegments[3] = {
			{ 0.f,           peripheralMid, 55 },  // peripheral, innermost
			{ peripheralMid, innerRadius,   90 },  // peripheral, outer half (peak for forward)
			{ innerRadius,   swingBandMid,  40 },  // swing-band segment 3, DIM for forward
		};
		for (const auto& seg : forwardSegments)
			dAttackVisualizationUtils::drawSegmentSolid(rendererFunctor, attackerRoot,
				wedgeCenterDirectionXY, areaNormal, areaAngularWidth,
				seg.outerR, seg.innerR, 6 /*Orange*/, 1.f, seg.alpha);
	}
	else
	{
		// Left/Right: peripheral + full swing band, four segments.
		const WedgeSegment sideSegments[4] = {
			{ 0.f,           peripheralMid, 25 },  // peripheral, innermost
			{ peripheralMid, innerRadius,   40 },  // peripheral, outer half
			{ innerRadius,   swingBandMid,  80 },  // swing band, peak
			{ swingBandMid,  swingBandOuter, 40 }, // swing band, fade to reach limit
		};
		for (const auto& seg : sideSegments)
			dAttackVisualizationUtils::drawSegmentSolid(rendererFunctor, attackerRoot,
				wedgeCenterDirectionXY, areaNormal, areaAngularWidth,
				seg.outerR, seg.innerR, 6 /*Orange*/, 1.f, seg.alpha);
	}

	// (4b) Sweep-direction indicator — a thin orange arc line + 3 small chevrons
	// distributed along it, running from the wedge's near edge to near the aim
	// direction. Communicates that the swing MOVES from the wedge position toward
	// the aim direction (research report: HUD direction communication, Rec 1+2 hybrid).
	//
	// Only drawn for L/R attacks (forward attack is a thrust, no sweep motion).
	if (attackGeometry.kind != dAttackVisualizationUtils::AttackDirectionKind::Forward)
	{
		const float kSweepArcRadius            = 115.f;                    // radius (cm) at which the arc + chevrons sit
		const float kSweepNearWedgeAng         = glm::pi<float>() / 3.f;   // 60° — arc's wedge-side end
		const float kSweepNearAimAng           = 0.f;                       // arc ends ON the aim direction vector
		const float kSweepChevronNearAimAng    = glm::pi<float>() / 12.f;  // 15° — chevron aim-side end (chevrons stop short of the aim line so the last chevron doesn't overlap it)
		const float kSweepChevronSize          = 20.f;                     // equilateral side (cm)
		const float kSweepArcThickness         = 1.f;                      // debug-line thickness for the arc
		const float kSweepChevronHeight        = kSweepChevronSize * 0.5f * 1.7320508f;  // (√3/2)·side

		// Sign convention: left attack = +Z rotation from aim (CCW), right = -Z (CW).
		const float sign                    = (attackGeometry.kind == dAttackVisualizationUtils::AttackDirectionKind::Left) ? 1.f : -1.f;
		const float wedgeSideAngle          = sign * kSweepNearWedgeAng;
		const float arcAimSideAngle         = sign * kSweepNearAimAng;
		const float chevronAimSideAngle     = sign * kSweepChevronNearAimAng;
		const float arcMidAngle             = (wedgeSideAngle + arcAimSideAngle) * 0.5f;
		const float arcHalfAngle            = std::abs(wedgeSideAngle - arcAimSideAngle) * 0.5f;

		// Arc line — drawCircleArc spans ±arcHalfAngle around arcMidDir.
		{
			const glm::mat4 rot = glm::rotate(glm::mat4(1.f), arcMidAngle, glm::vec3(0.f, 0.f, 1.f));
			const glm::vec3 arcMidDir = glm::vec3(rot * glm::vec4(aimDirectionXY, 0.f));
			rendererFunctor.drawCircleArc(attackerRoot, arcMidDir, kSweepArcRadius, arcHalfAngle, 6 /*Orange*/, kSweepArcThickness);
		}

		// 3 chevrons distributed evenly along the arc. Each chevron's apex points
		// along the arc tangent TOWARD the aim direction.
		for (int i = 0; i < 3; ++i)
		{
			const float t = static_cast<float>(i) / 2.f;   // 0, 0.5, 1
			const float chevronAngle = wedgeSideAngle + t * (chevronAimSideAngle - wedgeSideAngle);

			const glm::mat4 radialRot = glm::rotate(glm::mat4(1.f), chevronAngle, glm::vec3(0.f, 0.f, 1.f));
			const glm::vec3 radialDir = glm::vec3(radialRot * glm::vec4(aimDirectionXY, 0.f));
			const glm::vec3 chevronCenter = attackerRoot + radialDir * kSweepArcRadius;

			// Tangent toward aim = rotate radial by (-sign · π/2) about +Z.
			// For left (sign=+1): tangent = rotate(radial, -π/2) = CW rotation → moves toward smaller +θ = toward aim.
			// For right (sign=-1): tangent = rotate(radial, +π/2) = CCW → moves toward smaller |θ| (from -60° to -15°) = toward aim.
			const glm::mat4 tangentRot = glm::rotate(glm::mat4(1.f), -sign * glm::pi<float>() * 0.5f, glm::vec3(0.f, 0.f, 1.f));
			const glm::vec3 tangentDir = glm::vec3(tangentRot * glm::vec4(radialDir, 0.f));

			// Chevron triangle: apex at (2/3)·height in tangentDir from center, base at
			// -(1/3)·height in tangentDir, corners at ±(side/2)·radialDir from base mid.
			const glm::vec3 apex      = chevronCenter + (2.f / 3.f) * kSweepChevronHeight * tangentDir;
			const glm::vec3 baseMid   = chevronCenter - (1.f / 3.f) * kSweepChevronHeight * tangentDir;
			const glm::vec3 baseLeft  = baseMid + (kSweepChevronSize * 0.5f) * radialDir;
			const glm::vec3 baseRight = baseMid - (kSweepChevronSize * 0.5f) * radialDir;

			rendererFunctor.drawTriangle(apex, baseLeft, baseRight, 6 /*Orange*/);
		}
	}

	// Blue block arc alpha (PIE-tuned): bumped from 80 to 130 so the block signal reads
	// stronger than the orange swing band it overlays (orange segment 3 = 80). The blue
	// arc is drawn identically for forward and side attacks (same inner/outer radii and
	// same angular position — the wedgeCenterDirectionXY is already correct for the
	// pending sequence).
	const unsigned int blueArcAlpha = 130;

	// (5) Parent the shared target-viz query volumes to the attacker before querying.
	for (const auto& volumeId : state.getVolumeIds())
		input.getQueryAdapter().setVolumeParentTransform(volumeId, rootTransform);

	// (6) Overlap query on the bodyAndGuard volumes — hits carry both body- and
	// guard-category shapes for each defender.
	const SpatialQueryReport queryReport = input.getQueryAdapter().overlap(state.getVolumeIds());

	if (queryReport.empty())
		return;

	// (7) Group hits per defender by rootBodyId — mirrors dAttackRadialSimulation::collisionCheck
	// (lines 419-481). Both the hurtbox (body) and guard shapes on a character report the SAME
	// rootBodyId (the capsule), so they merge into one record with both indices set. The 1337
	// sentinels distinguish body-only / guard-only cases below. Hit ordering within the report is
	// unspecified (engine broadphase), so we index by rootBodyId, never positionally.
	struct RootHitData
	{
		BodyId rootBodyId;
		unsigned int guardHitIndex = 1337;
		unsigned int bodyHitIndex = 1337;
	};
	std::vector<RootHitData> actorHits;

	for (size_t i = 0; i < queryReport.size(); ++i)
	{
		const auto& hit = queryReport[i];

		auto findIt = std::find_if(actorHits.begin(), actorHits.end(), [&hit](const RootHitData& hitData) {
			return hitData.rootBodyId == hit.rootBodyId;
			});
		if (findIt == actorHits.end())
		{
			actorHits.push_back({ hit.rootBodyId, 1337, 1337 });
			findIt = actorHits.end() - 1;
		}

		if (hit.objectCategories.contains(collisionCategory::guard))
			findIt->guardHitIndex = static_cast<unsigned int>(i);
		else if (hit.objectCategories.contains(collisionCategory::body))
			findIt->bodyHitIndex = static_cast<unsigned int>(i);
	}

	// (8) For each grouped defender, evaluate the shared block predicate and accumulate a
	// single binary "any defender blocks" signal (the per-defender blue line is retired —
	// the block indicator is now the blue arc drawn once after the loop).
	bool anyDefenderBlocks = false;
	for (const auto& actorHit : actorHits)
	{
		// Not guarding — nothing to block with (the attack would land), so no line.
		if (actorHit.guardHitIndex == 1337)
			continue;

		// No body hit — no valid defender identity (mirrors the sim's body-index sentinel skip).
		if (actorHit.bodyHitIndex == 1337)
			continue;

		// Resolvability gate — skip defenders whose guard body became transiently unresolvable
		// between the query and the transform read. Without this, getBodyTransform silently
		// returns identity and the predicate would compute against a bogus guardForward = (1,0,0).
		if (!input.getPhysicsReader().isBodyResolvable(queryReport[actorHit.guardHitIndex].bodyId))
			continue;

		// Behind-attacker filter — defenders behind the attacker (relative to aim direction)
		// cannot be hit by the pending attack regardless of what the sim's wouldGuardBlock
		// says about their guard direction, so their guard state should NOT trigger the
		// block indicator. Test: XY-projected defender-from-attacker vector dotted with the
		// XY-projected aim direction; negative → defender is behind → skip.
		{
			const glm::vec3 defenderBodyPos = queryReport[actorHit.bodyHitIndex].objectPosition;
			const glm::vec3 relative = defenderBodyPos - attackerRoot;
			const glm::vec2 relativeXY(relative.x, relative.y);
			const glm::vec2 aimXY(aimDirectionXY.x, aimDirectionXY.y);
			if (glm::dot(relativeXY, aimXY) < 0.f)
				continue;
		}

		// Fetch the guard body's world transform via the GT-safe reader adapter — same signature
		// as the sim's collisionCheck, but reading GT-interpolated state from the game thread.
		const glm::mat4 guardTransform =
			input.getPhysicsReader().getBodyTransform(queryReport[actorHit.guardHitIndex].bodyId);

		// Shared predicate — identical implementation the sim's collisionCheck calls, so the viz
		// cannot drift from the sim's actual block outcome.
		const bool willBlock = dAttackRadialSimulation::wouldGuardBlock(
			attackGeometry.predictedSequenceIdFromIdle,
			attackerRoot,
			guardTransform,
			queryReport[actorHit.guardHitIndex].objectPosition);

		if (willBlock)
		{
			anyDefenderBlocks = true;
			break;	// Binary signal — the first blocking defender suffices.
		}
		// else: this defender wouldn't block — keep looking.
	}

	// (9) Block indicator — a shorter blue pie slice over the wedge base, drawn AFTER the
	// area layers so it blends over the orange base (draw order = blend order for the
	// translucent debug-mesh batcher). Same angular center/width, center-anchor, and
	// alpha as the area; half the radial extent (blueFraction = fadeFraction * 0.5), so
	// it reads as "the base of THIS wedge" changing state rather than a second wedge.
	// Nothing drawn when the attack would land.
	if (anyDefenderBlocks)
	{
		// Blue block arc (PIE-tuned): sits on the SAME radial range as the orange swing-band
		// segment 3 (innerRadius = 90 cm → swingBandMid = 120 cm). When the block condition
		// fires, the blue arc overlays the orange segment 3 exactly, reading as "the primary
		// swing band changing state" rather than a separate visual element. Same angular
		// center/width as the orange wedge.
		dAttackVisualizationUtils::drawSegmentSolid(rendererFunctor, attackerRoot,
			wedgeCenterDirectionXY, areaNormal, areaAngularWidth,
			swingBandMid, innerRadius, 2 /*Blue*/, 1.f, blueArcAlpha);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

}

#pragma optimize( "", on )

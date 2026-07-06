#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include <algorithm>
#include <vector>
#include <limits>
#include "glm/vec3.hpp"
#include <glm/gtc/quaternion.hpp>
#include "DAttackRadialSequence.h"
#include "OGBrawler/DAttackSequenceId.h"
#include "OGBrawler/DAttackCircle.h"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"
#include "OGSimulation/PhysicsBodyState.h"
#include "OGSimulation/QueryGeometry.h"
#include "OGSimulation/SpatialQueryResult.h"
#include "OGSimulation/SpatialQueryAdapter.h"
#include "OGBrawler/CollisionCategoryConstants.h"
#include "OGBrawlerLog.h"
#include "OGSimulation/OGAssert.h"

#pragma optimize( "", off )


class DAttackRadialSequence;

// [Task 36] InvalidAttackSequenceId, kHadoukenSequenceSentinel, and isRealAttackSequence
// were relocated to the minimal shared header OGBrawler/DAttackSequenceId.h (included above).
// They form an attack-sequence-ID value domain owned by neither sub-sim — see that header.

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace dAttackRadialSimulation
{

class StaticData
{
public:
	StaticData(const std::vector<DAttackRadialSequence>& attackSequences,
		const DAttackCircle& attackCircle)
		: attackSequences(attackSequences)
		, attackCircle(attackCircle)
	{}

	StaticData(const StaticData& other)
		: attackSequences(other.attackSequences)
		, attackCircle(other.attackCircle)
	{}

	const std::vector<DAttackRadialSequence>& getAttackSequences() const { return attackSequences; }
	const DAttackCircle& getAttackCircle() const { return attackCircle; }

private:
	StaticData() = delete;

	const std::vector<DAttackRadialSequence>& attackSequences;
	const DAttackCircle& attackCircle;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct DAttackHit
{
	glm::vec3 position;
	BodyId hitRootBodyId;   // actor-level id of the struck character (SpatialQueryHit::rootBodyId)
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// All physics setup descriptors for the radial simulation.
// Body descriptors are compile-time constants; query volumes depend on StaticData.
struct PhysicsSetup
{
	static inline const PhysicalObjectDescriptor body{
		BodyDescriptor{
			.simulatePhysics = true,
			.enableGravity = false
		},
		{   // shapes
			ShapeDescriptor{
				SphereGeometry{30.f},
				CollisionCategories::single(collisionCategory::body)
			}
		}
	};

	static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& staticData)
	{
		const float outerRadius = staticData.getAttackCircle().getOuterRadius();
		return {
			QueryVolumeDescriptor{
				SphereGeometry{outerRadius},
				collisionCategory::bodyAndGuard,
				glm::mat4(1.f),
				collisionCategory::queryRouting
			},
			QueryVolumeDescriptor{
				SphereGeometry{outerRadius * 2.f},
				collisionCategory::bodyAndGuard,
				glm::mat4(1.f),
				collisionCategory::queryRouting
			}
		};
	}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Immutable bindings set once at init — body IDs, attachment offset, shape IDs, query volumes.
struct RuntimeBindings
{
	BodyId ownBodyId;
	BodyId parentBodyId;
	glm::vec3 attachmentOffset;
	std::vector<ShapeId> shapeIds;
	std::vector<QueryVolumeId> queryVolumeIds;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Mutable per-tick scratch data only.
class DerivedState
{
public:
	DerivedState()
		: attackHits(4)
		, guardHits(4)
	{}

	DerivedState(const DerivedState& other)
		: attackHits(other.attackHits)
		, guardHits(other.guardHits)
	{}

	const std::vector<DAttackHit>& getAttackHits() const { return attackHits; }
	std::vector<DAttackHit>& editAttackHits() { return attackHits; }

	// Positions where the weapon intersected another character's guard during this
	// attack. Recorded alongside hasHitGuard in integrate() and cleared at the same
	// point as attackHits (firstResimStep / new attack sequence). Visualization renders
	// these as blue spheres in dAttackRadialVisualization.
	const std::vector<DAttackHit>& getGuardHits() const { return guardHits; }
	std::vector<DAttackHit>& editGuardHits() { return guardHits; }

private:
	std::vector<DAttackHit> attackHits;
	std::vector<DAttackHit> guardHits;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class PlayerInput
{
public:
	// [Task 43] Plain aggregate — C++20 parenthesis aggregate init keeps construct-site calls valid.
	glm::vec3 aimDirection{};
	bool attackLeft = false;
	bool attackRight = false;
};


template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
class IntegrationUtils
{
public:
	IntegrationUtils(float deltaTime,
		PhysicsBodyAdapterType& physicsBodyAdapter,
		SpatialQueryAdapterType& queryAdapter)
		: m_deltaTime(deltaTime)
		, m_physicsBodyAdapter(physicsBodyAdapter)
		, m_queryAdapter(queryAdapter)
	{}

	float getDeltaTime() const { return m_deltaTime; }
	PhysicsBodyAdapterType& getPhysicsAdapter() const { return m_physicsBodyAdapter; }
	SpatialQueryAdapterType& getQueryAdapter() const { return m_queryAdapter; }

private:
	float m_deltaTime;
	PhysicsBodyAdapterType& m_physicsBodyAdapter;
	SpatialQueryAdapterType& m_queryAdapter;
};

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
using AllInput = SimulationAllInput<PlayerInput, IntegrationUtils<PhysicsBodyAdapterType, SpatialQueryAdapterType>>;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class InitialConditions
{
public:
	// [Task 45] Plain aggregate — m_ prefix dropped, getInitialRotation() inlined at call sites.
	float initialAimAngle = 0.f;
	glm::vec3 initialAimRotationAxis{0.f, 0.f, 0.f};
	unsigned int activeAttackSequence = InvalidAttackSequenceId;
	unsigned int activeRootBodyId = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class State
{
public:
	// [Task 44] Plain aggregate.
	float attackTimer = 0.f;
	unsigned int currenSequenceId = 0;
	bool hasHitGuard = false;
	PhysicsBodyState bodyState;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

struct PhysicsDeclaration
{
	static const PhysicalObjectDescriptor& descriptor() { return PhysicsSetup::body; }
	static constexpr const char* name = "WeaponAxis";
	static std::vector<QueryVolumeDescriptor> queryVolumes(const StaticData& sd) {
		return PhysicsSetup::queryVolumes(sd);
	}
	static glm::vec3 attachmentOffset(const StaticData& sd) {
		(void)sd;
		return glm::vec3(0.f, 0.f, 30.f);
	}

	using StateType = dAttackRadialSimulation::State;
	static       PhysicsBodyState& bodyStateOf(      StateType& s) { return s.bodyState; }
	static const PhysicsBodyState& bodyStateOf(const StateType& s) { return s.bodyState; }

	RuntimeBindings bindings;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// [Task 62] Dependencies — OwnedDeps/ExternalDeps layout.
struct Dependencies {
	using Owned = OwnedDeps<
		dAttackRadialSimulation::InitialConditions,
		dAttackRadialSimulation::State>;
	using External = ExternalDeps<>;
	using InputType = dAttackRadialSimulation::PlayerInput;
	Owned owned;
	External external;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

OGBRAWLER_API DAttackSegment getAttackSegment(const InitialConditions& initialConditions, const StaticData& staticData, const glm::vec3& directionInRotationPlane);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void setInitialConditions(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	OGBLOG_G("[Radial.setInitialConditions] seq=%u (attackTimer reset to 0)",
		initialConditions.activeAttackSequence);
	state.attackTimer = 0.f;
	state.currenSequenceId = initialConditions.activeAttackSequence;

	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	const auto& activeAttackSequence = staticData.getAttackSequences()[initialConditions.activeAttackSequence];

	const glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);
	glm::vec3 rootTranslation = glm::vec3(rootTransform[3]);

	glm::mat4 initialAimRotationMatrix = glm::rotate(glm::mat4(1.f), initialConditions.initialAimAngle, initialConditions.initialAimRotationAxis);
	glm::vec4 localSequenceRotationAxis4(activeAttackSequence.getRotationAxis(), 0.f);
	glm::vec3 worldSequenceRotationAxis = glm::vec3(initialAimRotationMatrix * localSequenceRotationAxis4);
	glm::mat4 initialSequenceRotationMatrix = glm::rotate(glm::mat4(1.f), activeAttackSequence.getInitialAngle(), worldSequenceRotationAxis);
	glm::mat4 initialWorldTransform = glm::translate(glm::mat4(1.0f), rootTranslation) * initialSequenceRotationMatrix * initialAimRotationMatrix;

	physics.setBodyTransform(bindings.ownBodyId, initialWorldTransform);
	physics.setBodyAngularVelocity(bindings.ownBodyId, glm::vec3(0.f, 0.f, activeAttackSequence.getInitialVelocity()));
}

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void setIdlePose(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings)
{
	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	const auto& activeAttackSequence = staticData.getAttackSequences()[state.currenSequenceId];

	const glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);
	glm::vec3 rootTranslation = glm::vec3(rootTransform[3]);

	glm::mat4 upAlignmentTransform = glm::rotate(glm::mat4(1.f), glm::pi<float>()*0.5f, glm::vec3(0.0f, -1.0f, 0.0f));
	upAlignmentTransform[3] = glm::vec4(rootTranslation, 1.f);
	physics.setBodyTransform(bindings.ownBodyId, upAlignmentTransform);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void applyTorque(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings)
{
	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();

	const auto& activeAttackSequence = staticData.getAttackSequences()[initialConditions.activeAttackSequence];
	const glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);

	float a = activeAttackSequence.getAngularAcceleration(state.attackTimer);
	glm::mat4 initialRotationMatrix = glm::rotate(glm::mat4(1.f), initialConditions.initialAimAngle, initialConditions.initialAimRotationAxis);
	glm::vec4 localSequenceRotationAxis4(activeAttackSequence.getRotationAxis(), 0.f);
	glm::vec3 worldSequenceRotationAxis = glm::vec3(rootTransform * localSequenceRotationAxis4);

	const glm::vec3& inertiaTensor = physics.getBodyInertiaTensor(bindings.ownBodyId);
	physics.addBodyTorque(bindings.ownBodyId, worldSequenceRotationAxis * a * inertiaTensor.z);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void collisionCheck(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	if (derivedState.editAttackHits().size() >= 4)
		return;

	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	auto& queryAdapter = input.getIntegrationUtils().getQueryAdapter();
	const auto& activeAttackSequence = staticData.getAttackSequences()[initialConditions.activeAttackSequence];

	const glm::mat4x4 rootTransform = physics.getBodyTransform(bindings.ownBodyId);
	const glm::vec3 rootTranslation = glm::vec3(rootTransform[3]);

	const glm::vec4 defaultForward4(DAttackRadialSequence::defaultForward(), 0.f);
	const glm::vec3 currentDirection = glm::vec3(rootTransform * defaultForward4);

	const glm::mat4 initialRotation = glm::rotate(glm::mat4(1.f), initialConditions.initialAimAngle, initialConditions.initialAimRotationAxis);
	const glm::vec3 worldSequenceRotationAxis = glm::vec3(initialRotation * glm::vec4(activeAttackSequence.getRotationAxis(), 0.f));

	const auto currentAttackSegment = getAttackSegment(initialConditions, staticData, currentDirection);
	if (currentAttackSegment.state != DAttackRadialSequenceState::Damaging)
		return;

	// Update parent transforms before querying
	for (const auto& volumeId : bindings.queryVolumeIds)
		queryAdapter.setVolumeParentTransform(volumeId, rootTransform);

	SpatialQueryReport queryReport = queryAdapter.overlap(bindings.queryVolumeIds);

	if (queryReport.empty())
		return;

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

		if (std::find_if(derivedState.editAttackHits().begin(), derivedState.editAttackHits().end(), [&hit](const DAttackHit& hitIteratorValue) {
			return hitIteratorValue.hitRootBodyId == hit.rootBodyId;
			}) != derivedState.editAttackHits().end())
		{
			continue;
		}

		const glm::vec3 hitDirection = hit.objectPosition - rootTranslation;
		const float lengthAlongRotationAxis = abs(glm::dot(hitDirection, worldSequenceRotationAxis));
		const glm::vec3 hitDirectionOnRotationPlane = hitDirection - lengthAlongRotationAxis * worldSequenceRotationAxis;
		const float hitDistance = glm::length(hitDirectionOnRotationPlane);

		const bool hitIsInCircle = hitDistance > staticData.getAttackCircle().getInnerRadius() &&
			hitDistance < staticData.getAttackCircle().getOuterRadius() &&
			lengthAlongRotationAxis < staticData.getAttackCircle().getHalfThickness();

		if (!hitIsInCircle)
			continue;

		const glm::vec3 normalizedHitDirection = glm::normalize(hitDirectionOnRotationPlane);
		const auto hitAttackSegment = getAttackSegment(initialConditions, staticData, normalizedHitDirection);

		if (currentAttackSegment.index != hitAttackSegment.index)
			continue;

		{
			auto findIt = std::find_if(actorHits.begin(), actorHits.end(), [&hit](const RootHitData& hitData) {
				return hitData.rootBodyId == hit.rootBodyId;
				});
			if (findIt == actorHits.end())
			{
				// [hit-resolution T11] Merge body and guard hits by ACTOR-level identity
				// (rootBodyId). Both the hurtbox and the guard shape on a character now
				// report the same rootBodyId (the capsule), so the two shape hits merge
				// into ONE RootHitData with both bodyHitIndex and guardHitIndex set — the
				// guard directional check below then runs on the correct pairing.
				// The 1337 sentinels guard the body-only vs guard-case fork below
				// (`bodyHitIndex == 1337` continue; `guardHitIndex == 1337` body-only
				// branch). T10 fixed these initializers from 0 to 1337; T11 restores the
				// body+guard merge that pre-D8 relied on, so the sentinel path is no
				// longer load-bearing but is kept correct for standalone-body cases.
				actorHits.push_back({ hit.rootBodyId, 1337, 1337 });
				findIt = actorHits.end() - 1;
			}

			if (hit.objectCategories.contains(collisionCategory::guard))
				findIt->guardHitIndex = static_cast<unsigned int>(i);
			else if (hit.objectCategories.contains(collisionCategory::body))
				findIt->bodyHitIndex = static_cast<unsigned int>(i);
		}
	}

	for (const auto& actorHit : actorHits)
	{
		if (actorHit.bodyHitIndex == 1337)
			continue;

		if(actorHit.guardHitIndex == 1337)
		{
			const auto& hit = queryReport[actorHit.bodyHitIndex];
			derivedState.editAttackHits().push_back({ hit.objectPosition, hit.rootBodyId });
		}
		else
		{
			const float shieldAngle = 0.25f;
			const float outerShieldAngle = glm::pi<float>() * 0.5;
			const float shieldAngleDiff = outerShieldAngle - shieldAngle;
			const float middlePointShieldAngle = shieldAngle + (shieldAngleDiff * 0.5f);

			const auto& hit = queryReport[actorHit.guardHitIndex];
			// Query guard body transform via PhysicsBodyAdapter using hit's BodyId
			const glm::mat4x4 guardTransform = physics.getBodyTransform(hit.bodyId);
			const glm::vec3 guardTranslation = glm::vec3(guardTransform[3]);
			const glm::vec3 guardForward = glm::vec3(guardTransform[0]);

			glm::vec3 collidingPosition = hit.objectPosition;
			collidingPosition.z = rootTranslation.z;
			const glm::vec3 collisionDirection = rootTranslation - collidingPosition;
			const glm::vec3 normalizedCollisionDirection = glm::normalize(collisionDirection);

			// Compute the indicator position on the attacker's weapon line. Two cases:
			//   1. If the weapon line (rootTranslation + t * currentDirection) crosses the
			//      opponent's inner circle, use the near intersection (entry side, closer
			//      to the attacker). Math: quadratic t² + 2bt + c = 0 with b = dot(v, d),
			//      c = |v|² - r², v = attacker - opponent; smaller root = -b - sqrt(...).
			//   2. If the weapon line misses the inner circle (guard hits can register via
			//      spatial overlap even when the weapon direction is off to one side), fall
			//      back to the closest point on the weapon line to the opponent — the foot
			//      of the perpendicular from opponent onto the line: t = dot(opponent - attacker, d).
			// In both cases t is clamped to the visible weapon segment [0, outerRadius] so
			// the indicator can never appear off the end of the blade or behind the attacker.
			auto weaponHitIndicatorPosition = [&]() -> glm::vec3 {
				const glm::vec2 weaponDirXY = glm::normalize(glm::vec2(currentDirection.x, currentDirection.y));
				const glm::vec2 attackerXY(rootTranslation.x, rootTranslation.y);
				const glm::vec2 opponentXY(hit.objectPosition.x, hit.objectPosition.y);
				const glm::vec2 v = attackerXY - opponentXY;
				const float innerR = staticData.getAttackCircle().getInnerRadius();
				const float outerR = staticData.getAttackCircle().getOuterRadius();
				const float b = glm::dot(v, weaponDirXY);
				const float c = glm::dot(v, v) - innerR * innerR;
				const float discriminant = b * b - c;
				float t;
				if (discriminant >= 0.f)
					t = -b - std::sqrt(discriminant); // near intersection on inner circle
				else
					t = glm::dot(opponentXY - attackerXY, weaponDirXY); // closest point on weapon line
				t = glm::clamp(t, 0.f, outerR);
				return glm::vec3(attackerXY.x + t * weaponDirXY.x, attackerXY.y + t * weaponDirXY.y, hit.objectPosition.z);
			};

			const float arcRadius = std::min(staticData.getAttackCircle().getOuterRadius(), glm::length(collisionDirection));
			const float targetAngleDifference = std::acos(glm::dot(normalizedCollisionDirection, guardForward));
			if (targetAngleDifference < outerShieldAngle)
			{
				const glm::vec3 guardAxis = glm::cross(normalizedCollisionDirection, guardForward);
				if (targetAngleDifference < shieldAngle)
				{
					if (initialConditions.activeAttackSequence == 4)
					{
						state.hasHitGuard = true;
						derivedState.editGuardHits().push_back({ weaponHitIndicatorPosition(), hit.rootBodyId });
						break;
					}
				}
				else
				{
					if (guardAxis.z > 0.f && (initialConditions.activeAttackSequence == 0 || initialConditions.activeAttackSequence == 2))
					{
						state.hasHitGuard = true;
						derivedState.editGuardHits().push_back({ weaponHitIndicatorPosition(), hit.rootBodyId });
						break;
					}
					if (guardAxis.z < 0.f && (initialConditions.activeAttackSequence == 1 || initialConditions.activeAttackSequence == 3))
					{
						state.hasHitGuard = true;
						derivedState.editGuardHits().push_back({ weaponHitIndicatorPosition(), hit.rootBodyId });
						break;
					}
				}
			}

			derivedState.editAttackHits().push_back({ hit.objectPosition, hit.rootBodyId });
		}
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void deactivate(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
	const InitialConditions& initialConditions,
	const StaticData& staticData,
	State& state,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	OGBLOG_G("[Radial.deactivate] was seq=%u timer=%.4f",
		state.currenSequenceId, state.attackTimer);
	auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
	physics.setBodyAngularVelocity(bindings.ownBodyId, glm::vec3(0.f, 0.f, 0.f));

	setIdlePose(deltaSeconds, input, initialConditions, staticData, state, bindings);

	state.attackTimer = 0.f;
	state.currenSequenceId = InvalidAttackSequenceId;

	derivedState.editAttackHits().clear();
	derivedState.editGuardHits().clear();
	state.hasHitGuard = false;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

} //anonymous namespace

template <typename PhysicsBodyAdapterType, typename SpatialQueryAdapterType>
void integrate(float deltaSeconds,
	const AllInput<PhysicsBodyAdapterType, SpatialQueryAdapterType>& input,
	const StaticData& staticData,
	Dependencies deps,
	const RuntimeBindings& bindings,
	DerivedState& derivedState)
{
	const InitialConditions& initialConditions = deps.owned.get<InitialConditions>();
	State& state = deps.owned.edit<State>();

	// [NP-6] Explicit attachment math — replaces updateLinearAttachmentToOwner()
	{
		auto& physics = input.getIntegrationUtils().getPhysicsAdapter();
		glm::mat4 parentTransform = physics.getBodyTransform(bindings.parentBodyId);
		glm::vec3 parentPosition = glm::vec3(parentTransform[3]);
		glm::mat4 childTransform = physics.getBodyTransform(bindings.ownBodyId);
		childTransform[3] = glm::vec4(parentPosition + bindings.attachmentOffset, 1.0f);
		physics.setBodyTransform(bindings.ownBodyId, childTransform);
	}

	OGBLOG_G("[Radial.integrate] ic.activeSeq=%u state.curSeq=%u state.attackTimer=%.4f",
		initialConditions.activeAttackSequence, state.currenSequenceId, state.attackTimer);

	// Hadouken sentinel: the machine sim owns this "attack" via the projectile sub-sim.
	// Keep the weapon idle (the attachment math above already re-parents it this tick) and
	// reset currenSequenceId so the machine's Attacking->Idle exit fires naturally next
	// tick. Returning here also avoids the setInitialConditions path indexing
	// attackSequences[kHadoukenSequenceSentinel] out of bounds.
	if (initialConditions.activeAttackSequence == kHadoukenSequenceSentinel)
	{
		OGBLOG_G("[Radial.branch] hadouken sentinel — weapon idle, projectile owns this attack");
		state.currenSequenceId = InvalidAttackSequenceId;
		return;
	}

	if (initialConditions.activeAttackSequence == InvalidAttackSequenceId && state.currenSequenceId != InvalidAttackSequenceId)
	{
		OGBLOG_G("[Radial.branch] deactivate (ic invalid, state active)");
		deactivate(deltaSeconds, input, initialConditions, staticData, state, bindings, derivedState);
		return;
	}

	if(state.currenSequenceId != initialConditions.activeAttackSequence)
	{
		OGBLOG_G("[Radial.branch] setInitialConditions (state.curSeq=%u -> ic.activeSeq=%u)",
			state.currenSequenceId, initialConditions.activeAttackSequence);
		setInitialConditions(deltaSeconds, input, initialConditions, staticData, state, bindings, derivedState);

		if (state.currenSequenceId == InvalidAttackSequenceId)
			OG_CHECK(false, "DAttackRadialSimulation: invalid attack sequence after setInitialConditions");
	}

	if (state.currenSequenceId == InvalidAttackSequenceId)
	{
		OGBLOG_G("[Radial.branch] idle (state.curSeq invalid)");
		setIdlePose(deltaSeconds, input, initialConditions, staticData, state, bindings);
		return;
	}

	const auto& activeAttackSequence = staticData.getAttackSequences()[state.currenSequenceId];
	if (state.attackTimer < activeAttackSequence.getDuration())
	{
		OGBLOG_G("[Radial.branch] applyTorque+tick (timer=%.4f dur=%.4f)",
			state.attackTimer, activeAttackSequence.getDuration());
		applyTorque(deltaSeconds, input, initialConditions, staticData, state, bindings);

		collisionCheck(deltaSeconds, input, initialConditions, staticData, state, bindings, derivedState);

		state.attackTimer = state.attackTimer + deltaSeconds;
	}
	else
	{
		OGBLOG_G("[Radial.branch] deactivate (timer>=duration: %.4f >= %.4f)",
			state.attackTimer, activeAttackSequence.getDuration());
		deactivate(deltaSeconds, input, initialConditions, staticData, state, bindings, derivedState);
	}
}

template <typename StateReplicator>
void network(StateReplicator& replicator)
{
	// [Task 44] State is now a plain aggregate; no getter/setter-based network registration needed.
	// network() has no callers currently.
}

}

// [Task 39] SerializableFields specializations for dAttackRadialSimulation types.

template <>
struct SerializableFields<dAttackRadialSimulation::InitialConditions>
{
	static constexpr auto get()
	{
		using IC = dAttackRadialSimulation::InitialConditions;
		return std::make_tuple(
			SIM_MEMBER(IC, initialAimAngle),
			SIM_MEMBER(IC, initialAimRotationAxis),
			SIM_MEMBER(IC, activeAttackSequence),
			SIM_MEMBER(IC, activeRootBodyId));
	}
};

template <>
struct SerializableFields<dAttackRadialSimulation::State>
{
	static constexpr auto get()
	{
		using S = dAttackRadialSimulation::State;
		return std::make_tuple(
			SIM_MEMBER(S, attackTimer),
			SIM_MEMBER(S, currenSequenceId),
			SIM_MEMBER(S, hasHitGuard),
			SIM_MEMBER(S, bodyState));
	}
};

template <>
struct SerializableFields<dAttackRadialSimulation::PlayerInput>
{
	static constexpr auto get()
	{
		return std::make_tuple(
			MemberFieldDesc<&dAttackRadialSimulation::PlayerInput::aimDirection>{},
			MemberFieldDesc<&dAttackRadialSimulation::PlayerInput::attackLeft>{},
			MemberFieldDesc<&dAttackRadialSimulation::PlayerInput::attackRight>{});
	}
};

static_assert(SimulationState<dAttackRadialSimulation::State>);
static_assert(SimulationInput<dAttackRadialSimulation::PlayerInput>);
static_assert(SimulationInitialConditions<dAttackRadialSimulation::InitialConditions>);

#pragma optimize( "", on )




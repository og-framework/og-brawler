#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerHitDetectionSystem-rationale.md · docs/BrawlerHitDetectionSystem-guards.md
// Design: og-netcode-v2-field-defects impl/design_hit_detection_system.md (task 9, option C)

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

#include "OGSimulation/SimulatableList.h"
#include "OGSimulation/StorageView.h"
#include "OGSimulation/SystemsExecutor.h"
#include "OGSimulation/SimulationTimeContext.h"
#include "OGSimulation/SystemRoleAffinity.h"
#include "OGSimulation/SpatialQueryResult.h"
#include "OGSimulation/OGAssert.h"
#include "OGBrawler/SimulatableBrawler.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawler/BrawlerProjectileHitDetection.h"
#include "OGBrawler/DAttackRadialSequence.h"
#include "OGBrawler/DAttackSequenceId.h"
#include "OGBrawler/CollisionCategoryConstants.h"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerHitDetection
{

// ∴D-01  docs/BrawlerHitDetectionSystem-rationale.md
template <typename PhysicsReaderType, typename SpatialQueryAdapterType>
void detectRadialHits(float deltaSeconds,
	const dAttackRadialSimulation::StaticData& staticData,
	const dAttackRadialSimulation::InitialConditions& initialConditions,
	const dAttackRadialSimulation::State& state,
	const dAttackRadialSimulation::RuntimeBindings& bindings,
	dAttackRadialSimulation::DerivedState& derivedState,
	const PhysicsReaderType& physics,
	SpatialQueryAdapterType& queryAdapter)
{
	using namespace dAttackRadialSimulation;

	// ⛔G-13  docs/BrawlerHitDetectionSystem-guards.md
	derivedState.editHitsThisTick().clear();
	derivedState.editGuardBlockedThisTick() = false;
	derivedState.editGuardHits().clear();

	// ⛔G-01  docs/BrawlerHitDetectionSystem-guards.md
	if (state.currenSequenceId == InvalidAttackSequenceId
		|| initialConditions.activeAttackSequence == InvalidAttackSequenceId
		|| state.currenSequenceId != initialConditions.activeAttackSequence
		|| state.currenSequenceId >= staticData.getAttackSequences().size())
		return;
	OG_CHECK(state.attackTimer > 0.f,
		"brawlerHitDetection::detectRadialHits - the radial's State and InitialConditions name the "
		"same valid sequence but attackTimer is not positive. The previous tick's swing branch "
		"leaves attackTimer >= deltaSeconds for this preIntegrate to read; see G-01.");

	if (derivedState.editAttackHits().size() >= 4)
		return;

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

		// ⛔G-02  docs/BrawlerHitDetectionSystem-guards.md
		const float signedDistanceAlongRotationAxis = glm::dot(hitDirection, worldSequenceRotationAxis);

		// ⛔G-03  docs/BrawlerHitDetectionSystem-guards.md
		const float lengthAlongRotationAxis = glm::abs(signedDistanceAlongRotationAxis);
		const glm::vec3 hitDirectionOnRotationPlane = hitDirection - signedDistanceAlongRotationAxis * worldSequenceRotationAxis;
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
				// ∴D-02  docs/BrawlerHitDetectionSystem-rationale.md
				actorHits.push_back({ hit.rootBodyId, 1337, 1337 });
				findIt = actorHits.end() - 1;
			}

			if (hit.objectCategories.contains(collisionCategory::guard))
				findIt->guardHitIndex = static_cast<unsigned int>(i);
			else if (hit.objectCategories.contains(collisionCategory::body))
				findIt->bodyHitIndex = static_cast<unsigned int>(i);
		}
	}

	// ⛔G-04  docs/BrawlerHitDetectionSystem-guards.md
	const float swingTimer = state.attackTimer - deltaSeconds;
	// ⛔G-05  docs/BrawlerHitDetectionSystem-guards.md
	const float authoredAngularVelocity =
		activeAttackSequence.getAngularVelocity(swingTimer);
	auto swingTangentAt = [&](const glm::vec3& objectPosition) -> glm::vec3
	{
		const glm::vec3 direction = objectPosition - rootTranslation;
		const glm::vec3 onPlane = direction
			- glm::dot(direction, worldSequenceRotationAxis) * worldSequenceRotationAxis;
		// ⛔G-06  docs/BrawlerHitDetectionSystem-guards.md
		if (glm::dot(onPlane, onPlane) <= 0.f)
			return glm::vec3(0.f);
		const glm::vec3 tangent = glm::cross(worldSequenceRotationAxis, glm::normalize(onPlane));
		return authoredAngularVelocity < 0.f ? -tangent : tangent;
	};

	// ⛔G-07  docs/BrawlerHitDetectionSystem-guards.md
	auto registerAttackHit = [&derivedState](const DAttackHit& registered)
	{
		derivedState.editAttackHits().push_back(registered);
		derivedState.editHitsThisTick().push_back(registered);
	};

	for (const auto& actorHit : actorHits)
	{
		if (actorHit.bodyHitIndex == 1337)
			continue;

		if(actorHit.guardHitIndex == 1337)
		{
			const auto& hit = queryReport[actorHit.bodyHitIndex];
			registerAttackHit({ hit.objectPosition, hit.rootBodyId,
				swingTangentAt(hit.objectPosition) });
		}
		else
		{
			const auto& hit = queryReport[actorHit.guardHitIndex];
			const glm::mat4x4 guardTransform = physics.getBodyTransform(hit.bodyId);

			// ∴D-03  docs/BrawlerHitDetectionSystem-rationale.md
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
					t = -b - std::sqrt(discriminant);
				else
					t = glm::dot(opponentXY - attackerXY, weaponDirXY);
				t = glm::clamp(t, 0.f, outerR);
				return glm::vec3(attackerXY.x + t * weaponDirXY.x, attackerXY.y + t * weaponDirXY.y, hit.objectPosition.z);
			};

			// ∴D-04  docs/BrawlerHitDetectionSystem-rationale.md
			if (wouldGuardBlock(initialConditions.activeAttackSequence, rootTranslation, guardTransform, hit.objectPosition))
			{
				derivedState.editGuardBlockedThisTick() = true;
				derivedState.editGuardHits().push_back({ weaponHitIndicatorPosition(), hit.rootBodyId });
				// ⛔G-08  docs/BrawlerHitDetectionSystem-guards.md
				break;
			}

			registerAttackHit({ hit.objectPosition, hit.rootBodyId,
				swingTangentAt(hit.objectPosition) });
		}
	}
}

// One character's melee detection for the previous tick, read off the state its integrate left.
template <typename PhysicsReaderType, typename SpatialQueryAdapterType>
void detectRadialHits(float deltaSeconds,
	SimulatableBrawler& attacker,
	const simulatableBrawler::StaticData& staticData,
	const PhysicsReaderType& physics,
	SpatialQueryAdapterType& queryAdapter)
{
	auto& allState = attacker.editAllState();
	const auto& state = allState.getState();
	detectRadialHits(deltaSeconds,
		staticData.m_attackSimulationStaticData,
		state.get<dAttackRadialSimulation::InitialConditions>(),
		state.get<dAttackRadialSimulation::State>(),
		attacker.getPhysicsComposite().get<dAttackRadialSimulation::PhysicsDeclaration>().bindings,
		allState.editDerivedState().edit<dAttackRadialSimulation::DerivedState>(),
		physics, queryAdapter);
}

// ∴D-05  docs/BrawlerHitDetectionSystem-rationale.md
template <typename PhysicsReaderType, typename SpatialQueryAdapterType>
class System
{
public:
	using RequiredSimulatables = SimulatableList<SimulatableBrawler>;

	// ⛔G-09  docs/BrawlerHitDetectionSystem-guards.md
	static constexpr SystemRoleAffinity kRoleAffinity = SystemRoleAffinity::AllRoles;

	// ⛔G-10  docs/BrawlerHitDetectionSystem-guards.md
	System(const PhysicsReaderType& physics, SpatialQueryAdapterType& queryAdapter)
		: m_physics(&physics)
		, m_queryAdapter(&queryAdapter)
	{}

	const PhysicsReaderType& physicsAdapter() const { return *m_physics; }
	SpatialQueryAdapterType& queryAdapter() const { return *m_queryAdapter; }

	void preIntegrate(const SimulationTimeStep& step,
	                  StorageView<SimulatableBrawler> view,
	                  const simulatableBrawler::StaticData& staticData)
	{
		std::vector<std::pair<unsigned int, SimulatableBrawler*>> ordered;
		view.forEachSimulatable<SimulatableBrawler>(
			[&ordered](unsigned int id, SimulatableBrawler& brawler)
			{
				ordered.emplace_back(id, &brawler);
			});
		// ⛔G-11  docs/BrawlerHitDetectionSystem-guards.md
		std::sort(ordered.begin(), ordered.end(),
			[](const auto& a, const auto& b) { return a.first < b.first; });

		// ⛔G-12  docs/BrawlerHitDetectionSystem-guards.md
		for (const auto& [id, attacker] : ordered)
			detectRadialHits(step.getDeltaSeconds(), *attacker, staticData, *m_physics, *m_queryAdapter);

		std::vector<ProjectileShooter> shooters;
		shooters.reserve(ordered.size());
		for (const auto& [id, shooter] : ordered)
			shooters.push_back(projectileShooterOf(*shooter));
		// ⛔G-14  docs/BrawlerHitDetectionSystem-guards.md
		detectProjectileHits(step.getDeltaSeconds(), step.getTick(), staticData.m_projectileStaticData,
			shooters, *m_physics, *m_queryAdapter);
	}

	void postIntegrate(const SimulationTimeStep& /*step*/,
	                   StorageView<SimulatableBrawler> /*view*/,
	                   const simulatableBrawler::StaticData& /*staticData*/)
	{
	}

	void onCharacterRegistered(unsigned int /*id*/,
	                           StorageView<SimulatableBrawler> /*view*/,
	                           const simulatableBrawler::StaticData& /*staticData*/)
	{
	}

	void onCharacterUnregistered(unsigned int /*id*/,
	                             StorageView<SimulatableBrawler> /*view*/,
	                             const simulatableBrawler::StaticData& /*staticData*/)
	{
	}

private:
	const PhysicsReaderType* m_physics;
	SpatialQueryAdapterType* m_queryAdapter;
};

// ∴D-06  docs/BrawlerHitDetectionSystem-rationale.md
template <typename SystemT, typename... SystemTs>
constexpr std::size_t systemIndexOf()
{
	constexpr bool matches[] = { std::is_same_v<SystemT, SystemTs>..., false };
	for (std::size_t i = 0; i < sizeof...(SystemTs); ++i)
		if (matches[i])
			return i;
	return sizeof...(SystemTs);
}

template <typename ExecutorT, typename FirstT, typename SecondT>
struct FiresBefore : std::false_type {};

template <typename SimulatablesList, typename StaticDataT, typename... SystemTs,
          typename FirstT, typename SecondT>
struct FiresBefore<SimulationSystemsExecutor<SimulatablesList, StaticDataT, SystemTs...>, FirstT, SecondT>
	: std::bool_constant<
		systemIndexOf<FirstT, SystemTs...>() < sizeof...(SystemTs)
		&& systemIndexOf<SecondT, SystemTs...>() < sizeof...(SystemTs)
		&& systemIndexOf<FirstT, SystemTs...>() < systemIndexOf<SecondT, SystemTs...>()>
{};

template <typename ExecutorT, typename FirstT, typename SecondT>
inline constexpr bool firesBefore = FiresBefore<ExecutorT, FirstT, SecondT>::value;

} // namespace brawlerHitDetection

OGSIM_OPTIMIZE_ON

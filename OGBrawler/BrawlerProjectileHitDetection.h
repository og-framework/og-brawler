#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerProjectileHitDetection-rationale.md · docs/BrawlerProjectileHitDetection-guards.md

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "OGSimulation/SpatialQueryResult.h"
#include "OGBrawler/SimulatableBrawler.h"
#include "OGBrawler/BrawlerProjectileSimulation.h"
#include "OGBrawler/CollisionCategoryConstants.h"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"
#include "glm/common.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include "OGSimulation/CompilerControl.h"
OGSIM_OPTIMIZE_OFF

namespace brawlerHitDetection
{

struct ProjectileShooter
{
	const brawlerProjectileSimulation::State* state = nullptr;
	std::array<const brawlerProjectileSimulation::RuntimeBindings*,
		brawlerProjectileSimulation::kMaxProjectilePoolSize> bindings{};
	brawlerProjectileSimulation::DerivedState* derived = nullptr;
};

static_assert(brawlerProjectileSimulation::kMaxProjectilePoolSize == 3,
	"brawlerHitDetection::projectileShooterOf names PhysicsDeclaration<0..2> one by one; a pool "
	"of a different capacity must bind every slot there.");

inline ProjectileShooter projectileShooterOf(SimulatableBrawler& shooter)
{
	using namespace brawlerProjectileSimulation;
	auto& allState = shooter.editAllState();
	const auto& physics = shooter.getPhysicsComposite();
	return ProjectileShooter{
		&allState.getState().get<State>(),
		{ &physics.get<brawlerProjectileSimulation::PhysicsDeclaration<0>>().bindings,
		  &physics.get<brawlerProjectileSimulation::PhysicsDeclaration<1>>().bindings,
		  &physics.get<brawlerProjectileSimulation::PhysicsDeclaration<2>>().bindings },
		&allState.editDerivedState().edit<DerivedState>() };
}

// ∴D-01  docs/BrawlerProjectileHitDetection-rationale.md
inline bool projectileGuardBlocks(const glm::mat4& guardTransform, const glm::vec3& spawnDir,
	float guardMiddleSectionHalfAngle)
{
	glm::vec3 guardForward = glm::vec3(guardTransform[0]);
	const float fwdLen = glm::length(guardForward);
	if (fwdLen > 0.0001f)
		guardForward /= fwdLen;

	const glm::vec3 incoming = -spawnDir;
	const float d     = glm::clamp(glm::dot(guardForward, incoming), -1.f, 1.f);
	const float angle = glm::acos(d);
	return angle <= guardMiddleSectionHalfAngle;
}

inline float elapsedSecondsFromSpawn(const brawlerProjectileSimulation::ProjectileSlot& slot, float deltaSeconds,
	std::uint32_t tick)
{
	// ⛔G-08  docs/BrawlerProjectileHitDetection-guards.md
	return tick < slot.spawnTick ? 0.f : brawlerProjectileSimulation::elapsedSecondsAt(slot, deltaSeconds, tick);
}

// ∴D-02  docs/BrawlerProjectileHitDetection-rationale.md
template <typename PhysicsReaderType, typename SpatialQueryAdapterType>
void detectProjectileHits(float deltaSeconds, std::uint32_t tick,
	const brawlerProjectileSimulation::StaticData& staticData,
	const std::vector<ProjectileShooter>& shooters,
	const PhysicsReaderType& physics,
	SpatialQueryAdapterType& queryAdapter)
{
	using namespace brawlerProjectileSimulation;

	// ⛔G-01  docs/BrawlerProjectileHitDetection-guards.md
	for (const ProjectileShooter& shooter : shooters)
		shooter.derived->detectedThisTick.fill(SlotDetection{});

	const std::uint32_t activeCount = std::min(staticData.projectilePoolSize, kMaxProjectilePoolSize);

	struct SlotRef
	{
		std::size_t   shooter = 0;
		std::uint32_t slot    = 0;
	};
	struct Contact
	{
		SlotRef         at;
		SpatialQueryHit first;
	};
	std::vector<SlotRef> inFlight;
	std::vector<Contact> contacts;

	for (std::size_t s = 0; s < shooters.size(); ++s)
	{
		for (std::uint32_t i = 0; i < activeCount; ++i)
		{
			const ProjectileSlot& slot = shooters[s].state->slots[i];
			if (!slot.isAlive(tick))
				continue;
			// ⛔G-07  docs/BrawlerProjectileHitDetection-guards.md
			if (tick < slot.spawnTick)
				continue;
			const float elapsedSeconds = elapsedSecondsFromSpawn(slot, deltaSeconds, tick);
			// ⛔G-02  docs/BrawlerProjectileHitDetection-guards.md
			if (elapsedSeconds >= staticData.maxLifetime)
				continue;
			inFlight.push_back({ s, i });

			glm::mat4 derivedTransform(1.f);
			derivedTransform[3] = glm::vec4(closedFormPosition(slot, staticData, elapsedSeconds), 1.f);
			const RuntimeBindings& bindings = *shooters[s].bindings[i];
			for (const auto& volumeId : bindings.queryVolumeIds)
				queryAdapter.setVolumeParentTransform(volumeId, derivedTransform);

			const SpatialQueryReport report = queryAdapter.overlap(bindings.queryVolumeIds);
			for (const auto& hit : report)
			{
				if (hit.bodyId == bindings.parentBodyId)
					continue;
				contacts.push_back({ { s, i }, hit });
				// ⛔G-03  docs/BrawlerProjectileHitDetection-guards.md
				break;
			}
		}
	}

	auto detectionAt = [&shooters](const SlotRef& ref) -> SlotDetection&
	{
		return shooters[ref.shooter].derived->detectedThisTick[ref.slot];
	};

	for (const Contact& contact : contacts)
	{
		if (!contact.first.objectCategories.contains(collisionCategory::projectile))
			continue;
		detectionAt(contact.at).outcome = SlotOutcome::CancelledByProjectile;
		for (const SlotRef& other : inFlight)
		{
			// ⛔G-04  docs/BrawlerProjectileHitDetection-guards.md
			if (shooters[other.shooter].bindings[other.slot]->ownBodyId == contact.first.bodyId)
				detectionAt(other).outcome = SlotOutcome::CancelledByProjectile;
		}
	}

	for (const Contact& contact : contacts)
	{
		SlotDetection& detection = detectionAt(contact.at);
		// ⛔G-05  docs/BrawlerProjectileHitDetection-guards.md
		if (detection.outcome == SlotOutcome::CancelledByProjectile)
			continue;

		const SpatialQueryHit& hit = contact.first;
		const ProjectileSlot& slot = shooters[contact.at.shooter].state->slots[contact.at.slot];
		bool blocked = false;
		glm::vec3 targetRootPosition(0.f);
		if (hit.objectCategories.contains(collisionCategory::guard))
		{
			const glm::mat4 guardTransform = physics.getBodyTransform(hit.bodyId);
			targetRootPosition = glm::vec3(guardTransform[3]);
			// ⛔G-06  docs/BrawlerProjectileHitDetection-guards.md
			blocked = projectileGuardBlocks(guardTransform, slot.spawnDir, staticData.guardMiddleSectionHalfAngle);
		}

		detection.outcome            = blocked ? SlotOutcome::BlockedByGuard : SlotOutcome::Hit;
		detection.struckRootBodyId   = hit.rootBodyId;
		detection.objectPosition     = hit.objectPosition;
		detection.targetRootPosition = targetRootPosition;
	}
}

// One shooter's pool on its own: the single-character rigs call this before `integrate`.
template <typename PhysicsReaderType, typename SpatialQueryAdapterType>
void detectProjectileHits(float deltaSeconds, std::uint32_t tick,
	const brawlerProjectileSimulation::StaticData& staticData,
	const brawlerProjectileSimulation::State& state,
	const std::array<brawlerProjectileSimulation::RuntimeBindings,
		brawlerProjectileSimulation::kMaxProjectilePoolSize>& bindings,
	brawlerProjectileSimulation::DerivedState& derived,
	const PhysicsReaderType& physics,
	SpatialQueryAdapterType& queryAdapter)
{
	ProjectileShooter shooter;
	shooter.state = &state;
	for (std::size_t i = 0; i < bindings.size(); ++i)
		shooter.bindings[i] = &bindings[i];
	shooter.derived = &derived;
	detectProjectileHits(deltaSeconds, tick, staticData, std::vector<ProjectileShooter>{ shooter },
		physics, queryAdapter);
}

} // namespace brawlerHitDetection

OGSIM_OPTIMIZE_ON

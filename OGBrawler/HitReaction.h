#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <cstdint>

#include "glm/vec3.hpp"

enum class HitReactionKind : uint8_t
{
	Stun = 0,
	Knockback = 1,
};

static_assert(sizeof(HitReactionKind) == 1u
	&& static_cast<uint8_t>(HitReactionKind::Stun) == 0u
	&& static_cast<uint8_t>(HitReactionKind::Knockback) == 1u,
	"HitReactionKind - this enumerator rides dAttackMachineSimulation::State::m_hitReaction ON THE "
	"WIRE as ONE BYTE. Stun MUST stay 0: injectCorrectionState DEFAULT-CONSTRUCTS the State before "
	"readInto, and every test peer builds one, so a zero byte has to mean the pre-task-27 HitFlinch "
	"behaviour (freeze in place) rather than a knockback with no speed and no direction. Widening "
	"the underlying type is a wire change and re-prices SimulatableBrawlerTest.cpp and "
	"RoundVsPacketBudgetTest.cpp; a THIRD kind (Launch, Crumple, Stagger) is neither - it is one "
	"more enumerator here and one more row in simulatableBrawler::StaticData::m_hitReactions.");

// [og-ground-aligned-attacks task 9, OQ15] Where a melee row's 3-D knockback direction comes from.
// StaticData only: never on the wire, so the underlying type is free to change.
enum class HitDirectionSource : uint8_t
{
	SwingTangent = 0,         // the hit's swing tangent (the default, and every shipped row)
	AuthoredInSwingFrame = 1, // HitReactionSpec::authoredDirection, rotated by the attacker's radial IC
};

struct HitReactionSpec
{
	HitReactionKind kind = HitReactionKind::Stun;
	float knockbackSpeed = 0.f;
	float lockoutDuration = 0.f;
	// [og-ground-aligned-attacks task 9, OQ15] APPENDED with defaults, so every positional
	// { kind, speed, lockout } row (simulatableBrawler::StaticData, the tests) keeps the swing tangent.
	// authoredDirection is in SWING-FRAME coordinates: simFrame::kForward is the attacker's aim and
	// simFrame::kUp its ground normal (the frame the radial IC rotates into). It is read only when
	// directionSource is AuthoredInSwingFrame, must then be non-zero (routing normalises it), and is
	// ignored on a projectile row (m_projectileHitReaction, m_projectileHitOnFlinchReaction), whose
	// direction is always the shot's spawnDir. Neither member is serialized.
	HitDirectionSource directionSource = HitDirectionSource::SwingTangent;
	glm::vec3 authoredDirection{0.f};
};

#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <cstdint>

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

struct HitReactionSpec
{
	HitReactionKind kind = HitReactionKind::Stun;
	float knockbackSpeed = 0.f;
	float lockoutDuration = 0.f;
};

#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerSyncedPlayerInput-rationale.md · docs/BrawlerSyncedPlayerInput-guards.md

#include <concepts>
#include <cstdint>
#include <tuple>
#include <type_traits>
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "OGSimulation/SimulationComposite.h"
#include "OGSimulation/SimulationComparisonGlm.h"
#include "OGSimulation/SimulationFieldDescriptors.h"

namespace simulatableBrawler
{

struct SyncedPlayerInput
{
    glm::vec3 aimDirection = glm::vec3(0.f);
    bool      attackLeft  = false;
    bool      attackRight = false;
    glm::vec2 moveStick = glm::vec2(0.f);
    glm::vec3 moveDirectionWorld = glm::vec3(0.f);
    uint32_t  triggeredActionId = 0u;
    uint8_t   flags = 0u;

    static constexpr SyncedPlayerInput zero()
    {
        return SyncedPlayerInput{ .aimDirection = glm::vec3(0.f, 0.f, 1.f) };
    }
};

template <typename V>
concept BrawlerInputView = requires(const SyncedPlayerInput& in) { { V::from(in) } -> std::same_as<V>; }
    && !Serializable<V> && std::is_trivially_copyable_v<V>;

} // namespace simulatableBrawler

template <>
struct SerializableFields<simulatableBrawler::SyncedPlayerInput>
{
    static constexpr auto get()
    {
        using In = simulatableBrawler::SyncedPlayerInput;
        return std::make_tuple(
            SIM_MEMBER(In, aimDirection),
            SIM_MEMBER(In, attackLeft),
            SIM_MEMBER(In, attackRight),
            SIM_MEMBER(In, moveStick),
            SIM_MEMBER(In, moveDirectionWorld),
            SIM_MEMBER(In, triggeredActionId),
            SIM_MEMBER(In, flags));
    }
};

static_assert(std::is_same_v<
        decltype(SerializableFields<simulatableBrawler::SyncedPlayerInput>::get()),
        std::tuple<
            MemberFieldDesc<&simulatableBrawler::SyncedPlayerInput::aimDirection>,
            MemberFieldDesc<&simulatableBrawler::SyncedPlayerInput::attackLeft>,
            MemberFieldDesc<&simulatableBrawler::SyncedPlayerInput::attackRight>,
            MemberFieldDesc<&simulatableBrawler::SyncedPlayerInput::moveStick>,
            MemberFieldDesc<&simulatableBrawler::SyncedPlayerInput::moveDirectionWorld>,
            MemberFieldDesc<&simulatableBrawler::SyncedPlayerInput::triggeredActionId>,
            MemberFieldDesc<&simulatableBrawler::SyncedPlayerInput::flags>>>,
    "simulatableBrawler::SyncedPlayerInput - APPEND ONLY. The input wire layout is POSITIONAL: "
    "reordering, inserting or removing an entry is a WIRE FORMAT CHANGE even when the byte count "
    "does not move, and every peer that has not shipped the same edit misreads the relayed input "
    "ring and the redundancy bundle. Append a field here and in the struct, and bump "
    "relayedInputRing::kWireFormatVersion and inputRedundancyBundle::kWireFormatVersion. "
    "Successor of the APPEND-ONLY assertion on the descriptors of the deleted movement input slice "
    "(brawlerMovementSimulation's PlayerInput).");

static_assert(syncSize<simulatableBrawler::SyncedPlayerInput>() == 39u,
    "simulatableBrawler::SyncedPlayerInput is 39 B on the wire: aim 12, attackLeft 1, attackRight 1, "
    "moveStick 8, moveDirectionWorld 12, triggeredActionId 4, flags 1. Every input byte is paid once "
    "per relayed ring entry, so a size change is a packet-budget change and a wire format change.");

static_assert(SimulationInput<simulatableBrawler::SyncedPlayerInput>,
    "simulatableBrawler::SyncedPlayerInput is the per-tick input the framework resolves, relays and "
    "neutral-fills: it must stay Serializable and keep its static zero().");

static_assert(std::is_trivially_copyable_v<simulatableBrawler::SyncedPlayerInput>,
    "simulatableBrawler::SyncedPlayerInput must stay plain values: a host may read an input capture "
    "slot on one thread while another thread writes it, and a member that owned memory would turn "
    "that tear into a crash.");

static_assert(simulatableBrawler::SyncedPlayerInput::zero().aimDirection == glm::vec3(0.f, 0.f, 1.f),
    "simulatableBrawler::SyncedPlayerInput::zero() aims at (0,0,1), never SyncedPlayerInput{}: it is "
    "the WIRE VALUE every peer neutral-fills a tick with, so changing it makes peers on different "
    "builds disagree about the neutral input. Was guard G-02 of BrawlerSyncedPlayerInput-guards.md.");

static_assert(simulatableBrawler::SyncedPlayerInput{}.aimDirection
        != simulatableBrawler::SyncedPlayerInput::zero().aimDirection,
    "simulatableBrawler::SyncedPlayerInput's aimDirection default must differ from zero()'s aim: the "
    "input-resolution and net-sync tests tell the game's neutral input from a value-initialised one "
    "by this field, and equal values make those controls pass while testing nothing. "
    "Was guard G-01 of BrawlerSyncedPlayerInput-guards.md.");

#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGTypes.h"
// Canonical home for the brawler composite simulation: type aggregates
// (State, DerivedState, AllState, PlayerInput, StaticData) plus the
// composite's compile-time dependency-graph validation.
//
// The integrate flow itself lives in SimulatableBrawler::integrate.

#include <vector>
#include "glm/vec3.hpp"
#include "DAttackRadialSequence.h"
#include "OGBrawler/DAttackCircle.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawler/DAttackGuardSimulation.h"
#include "OGBrawler/DAttackMachineSimulation.h"
// Phase 2 / Task 15 (2026-06-24): projectile sub-sim RE-WIRED into the brawler
// composite. The transport changes it was blocked on (og-netcode-v1-impl Stage 1/2,
// 60 Hz + split watermark-trimmed buffers) landed 2026-06-22, and the T13 closed-form
// projectile State redesign shrank the per-character wire footprint (115 B vs 196 B).
// FSimulationStateSyncBuffer::kBufferBytes was bumped to 384 (T14) to fit the re-wired
// composite (~304 B).
#include "OGBrawler/BrawlerProjectileSimulation.h"
// [hit-resolution T2] Inbound-hit signal carried on the composite DerivedState. Off-wire
// (no SerializableFields specialization), so including it here adds no bytes to the State
// composite — see current_state.md §D1.
#include "OGBrawler/BrawlerInboundHit.h"
// [Task 35] CharacterBindings relocated to its own minimal header (the eventual home of the
// planned character-movement sub-sim). Eagerly include it here so the composite — and any
// downstream consumer that includes SimulatableBrawlerTypes.h — keeps transitive visibility
// on the type (relevant for the parked T34 migration that folds it into every sub-sim's
// RuntimeBindings).
#include "OGBrawler/BrawlerMovementSimulation.h"
#include "OGSimulation/SimulationDependencies.h"

#pragma optimize("", off)

namespace simulatableBrawler
{

// Serialization wire order: radialIC -> radialState -> guardState -> guardIC (0 bytes)
//   -> machineState -> projectileIC -> projectileState
// (projectile slices appended last per Task 15; machine writes projectileIC, projectile
//  consumes it — see ExecutionOrder below.)
using State = SimulationStateComposite<
    dAttackRadialSimulation::InitialConditions,
    dAttackRadialSimulation::State,
    dAttackGuardSimulation::State,
    dAttackGuardSimulation::InitialConditions,
    dAttackMachineSimulation::State,
    brawlerProjectileSimulation::InitialConditions,
    brawlerProjectileSimulation::State
>;

class DerivedState
{
public:
    DerivedState() = default;

    dAttackRadialSimulation::DerivedState m_attackDerivedState;
    dAttackGuardSimulation::DerivedState m_guardDerivedState;
    brawlerProjectileSimulation::DerivedState m_projectileDerivedState;
    // [hit-resolution T2] Per-tick inbound-hit signal. Reset + populated by the manager's
    // routing pass (T3); read the following tick by the machine sim's integrate3 to drive
    // the HitFlinch transition. Off-wire (see D1) — no SerializableFields entry.
    brawlerInboundHit::DerivedState m_inboundHitDerivedState;
};

class AllState
{
public:
    AllState() = default;

    const State& getState() const { return m_state; }
    State& editState() { return m_state; }

    const DerivedState& getDerivedState() const { return m_derivedState; }
    DerivedState& editDerivedState() { return m_derivedState; }

private:
    State m_state;
    DerivedState m_derivedState;
};

// Serialization wire order: radialInput -> machineInput -> guardInput -> projectileInput
using PlayerInput = SimulationInputComposite<
    dAttackRadialSimulation::PlayerInput,
    dAttackMachineSimulation::PlayerInput,
    dAttackGuardSimulation::PlayerInput,
    brawlerProjectileSimulation::PlayerInput
>;

inline PlayerInput getZeroPlayerInput()
{
    return PlayerInput(
        dAttackRadialSimulation::PlayerInput(glm::vec3(0.f, 0.f, 1.f), false, false),
        dAttackMachineSimulation::PlayerInput(glm::vec3(0.f, 0.f, 1.f), false, false,
                                              glm::vec2(0.f), glm::vec3(0.f)),
        dAttackGuardSimulation::PlayerInput(glm::vec3(0.f, 0.f, 1.f)),
        brawlerProjectileSimulation::PlayerInput{}
    );
}

class StaticData
{
public:
    StaticData()
        : m_attackCircle(6.f, 90.f, 300.f, 70.f, true, 1.f)
        , m_attackSequences(
            {
                { // from idle to leftAttack
                    {
                        { 0.f, -4.f * PI / 8.f, DAttackRadialSequenceState::WindUp },
                        { 0.3f, -2.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.4f, -1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.5f, 1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.6f, 3.f * PI / 8.f, DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from idle to rightAttack
                    {
                        { 0.f, 4.f * PI / 8.f, DAttackRadialSequenceState::WindUp },
                        { 0.3f, 2.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.4f, 1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.5f, -1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.6f, -3.f * PI / 8.f, DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from leftAttack to leftAttack
                    {
                        { 0.f,   (16.f * PI / 8.f) - (12.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.1f,  (16.f * PI / 8.f) - (10.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.15f, (16.f * PI / 8.f) - (8.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.2f,  (16.f * PI / 8.f) - (6.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.25f, (16.f * PI / 8.f) - (4.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.3f,  (16.f * PI / 8.f) - (2.f  * PI / 8.f), DAttackRadialSequenceState::Damaging },
                        { 0.4f,  (16.f * PI / 8.f) + (1.f  * PI / 8.f), DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from rightAttack to rightAttack
                    {
                        { 0.f,   (-16.f * PI / 8.f) + (12.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.1f,  (-16.f * PI / 8.f) + (10.f * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.15f, (-16.f * PI / 8.f) + (8.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.2f,  (-16.f * PI / 8.f) + (6.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.25f, (-16.f * PI / 8.f) + (4.f  * PI / 8.f), DAttackRadialSequenceState::WindUp },
                        { 0.3f,  (-16.f * PI / 8.f) + (2.f  * PI / 8.f), DAttackRadialSequenceState::Damaging },
                        { 0.4f,  (-2.f * PI) - (1.f * PI / 8.f),          DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 0.f, 1.f)
                },
                { // from idle to rightAttack (vertical)
                    {
                        { 0.0f,  -8.f * PI / 8.f, DAttackRadialSequenceState::WindUp },
                        { 0.3f,  -2.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.4f,   1.f * PI / 8.f, DAttackRadialSequenceState::Damaging },
                        { 0.42f,  1.5f * PI / 8.f, DAttackRadialSequenceState::WindDown }
                    }
                    , 0.1, glm::vec3(0.f, 1.f, 0.f)
                }
            }
        )
        , m_attackSimulationStaticData(m_attackSequences, m_attackCircle)
        , m_guardSimulationStaticData(m_attackCircle)
        // 0.875 s × 800 cm/s = 700 cm = 7 m travel distance (T26).
        // 6th arg (T29; narrowed T31) = guardMiddleSectionHalfAngle 0.25 rad (≈14.3°) →
        //   only a near-centre-line guard alignment blocks the projectile, matching the
        //   radial sim's middle-section threshold (DAttackRadialSimulation.h:450 shieldAngle).
        // 7th arg (T30) = innerCircleRadius — the brawler attack circle inner radius, so a
        //   blocked projectile's marker lands on that circle (edge facing the shooter).
        // 8th arg (T30) = indicatorPersistTicks 20 ≈ 0.333 s at 60 Hz (radial guard-hit feel).
        , m_projectileStaticData(800.f, 0.875f, 40.f, 60.f, 50.f, 0.25f,
                                 m_attackCircle.getInnerRadius(), 20)
    {}

    DAttackCircle m_attackCircle;
    std::vector<DAttackRadialSequence> m_attackSequences;
    dAttackRadialSimulation::StaticData m_attackSimulationStaticData;
    dAttackGuardSimulation::StaticData m_guardSimulationStaticData;
    brawlerProjectileSimulation::StaticData m_projectileStaticData;
};

// [Task 55/60] Execution order validation — declared order must satisfy dependency edges.
// The actual sub-sim integrate calls live in SimulatableBrawler::integrate; this
// assertion is a structural check on the composite's dependency graph.
using ExecutionOrder = std::tuple<
    dAttackMachineSimulation::Dependencies,
    dAttackGuardSimulation::Dependencies,
    dAttackRadialSimulation::Dependencies,
    brawlerProjectileSimulation::Dependencies>;
inline constexpr auto executionViolation_ =
    compositeDetail::findFirstViolation<ExecutionOrder>();
using ExecutionOrderDiagnostic_ =
    compositeDetail::DecodeViolation<ExecutionOrder, executionViolation_>;
static_assert(sizeof(ExecutionOrderDiagnostic_) >= 0);

} // namespace simulatableBrawler

#pragma optimize("", on)

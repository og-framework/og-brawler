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
#include "OGSimulation/SimulationDependencies.h"

#pragma optimize("", off)

namespace simulatableBrawler
{

// Serialization wire order: radialIC -> radialState -> guardState -> guardIC (0 bytes) -> machineState
using State = SimulationStateComposite<
    dAttackRadialSimulation::InitialConditions,
    dAttackRadialSimulation::State,
    dAttackGuardSimulation::State,
    dAttackGuardSimulation::InitialConditions,
    dAttackMachineSimulation::State
>;

class DerivedState
{
public:
    DerivedState() = default;

    dAttackRadialSimulation::DerivedState m_attackDerivedState;
    dAttackGuardSimulation::DerivedState m_guardDerivedState;
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

// Serialization wire order: radialInput -> machineInput -> guardInput
using PlayerInput = SimulationInputComposite<
    dAttackRadialSimulation::PlayerInput,
    dAttackMachineSimulation::PlayerInput,
    dAttackGuardSimulation::PlayerInput
>;

inline PlayerInput getZeroPlayerInput()
{
    return PlayerInput(
        dAttackRadialSimulation::PlayerInput(glm::vec3(0.f, 0.f, 1.f), false, false),
        dAttackMachineSimulation::PlayerInput(glm::vec3(0.f, 0.f, 1.f), false, false,
                                              glm::vec2(0.f), glm::vec3(0.f)),
        dAttackGuardSimulation::PlayerInput(glm::vec3(0.f, 0.f, 1.f))
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
    {}

    DAttackCircle m_attackCircle;
    std::vector<DAttackRadialSequence> m_attackSequences;
    dAttackRadialSimulation::StaticData m_attackSimulationStaticData;
    dAttackGuardSimulation::StaticData m_guardSimulationStaticData;
};

// [Task 55/60] Execution order validation — declared order must satisfy dependency edges.
// The actual sub-sim integrate calls live in SimulatableBrawler::integrate; this
// assertion is a structural check on the composite's dependency graph.
using ExecutionOrder = std::tuple<
    dAttackMachineSimulation::Dependencies,
    dAttackGuardSimulation::Dependencies,
    dAttackRadialSimulation::Dependencies>;
inline constexpr auto executionViolation_ =
    compositeDetail::findFirstViolation<ExecutionOrder>();
using ExecutionOrderDiagnostic_ =
    compositeDetail::DecodeViolation<ExecutionOrder, executionViolation_>;
static_assert(sizeof(ExecutionOrderDiagnostic_) >= 0);

} // namespace simulatableBrawler

#pragma optimize("", on)

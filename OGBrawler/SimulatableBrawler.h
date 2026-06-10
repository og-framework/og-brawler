#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include "OGBrawler/SimulatableBrawlerTypes.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawler/DAttackGuardSimulation.h"
#include "OGBrawler/DAttackMachineSimulation.h"
// brawlerProjectileSimulation intentionally not included — see SimulatableBrawlerTypes.h.
// #include "OGBrawler/BrawlerProjectileSimulation.h"
#include "OGBrawler/DAttackMachineSimulationRuntimeTweakables.h"
#include "OGSimulation/SimulationTimeContext.h"
#include "OGSimulation/SimulationDependencies.h"
#include "OGSimulation/PhysicsBodyAdapter.h"
#include "OGSimulation/SpatialQueryAdapter.h"
#include "OGBrawler.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"

class SimulatableBrawler
{
public:
    using StateType = simulatableBrawler::State;
    using InputType = simulatableBrawler::PlayerInput;

    OGBRAWLER_API SimulatableBrawler(const simulatableBrawler::StaticData& staticData);

    const simulatableBrawler::AllState& getAllState() const { return m_allState; }
    simulatableBrawler::AllState& editAllState() { return m_allState; }

    void updateVizState() { m_vizState = m_allState; }
    const simulatableBrawler::AllState& getVizState() const { return m_vizState; }

    template <PhysicsBodyAdapter PhysAdapterT, SpatialQueryAdapter QueryAdapterT>
    void integrate(
        const SimulationTimeStep& step,
        const simulatableBrawler::PlayerInput& input,
        PhysAdapterT& physAdapter,
        QueryAdapterT& queryAdapter,
        const simulatableBrawler::StaticData& staticData);

    template <PhysicsBodyAdapter PhysAdapterT>
    void firstResimStep(PhysAdapterT& adapter, int32_t physicsStep);

    auto& editPhysicsComposite() { return m_physics; }
    const auto& getPhysicsComposite() const { return m_physics; }

private:
    simulatableBrawler::AllState m_allState;
    simulatableBrawler::AllState m_vizState;
    SimulationPhysicsComposite<
        dAttackRadialSimulation::PhysicsDeclaration,
        dAttackGuardSimulation::PhysicsDeclaration> m_physics;
};

template <PhysicsBodyAdapter PhysAdapterT, SpatialQueryAdapter QueryAdapterT>
void SimulatableBrawler::integrate(
    const SimulationTimeStep& step,
    const simulatableBrawler::PlayerInput& input,
    PhysAdapterT& physAdapter,
    QueryAdapterT& queryAdapter,
    const simulatableBrawler::StaticData& staticData)
{
    const float dt = step.getDeltaSeconds();
    dAttackRadialSimulation::IntegrationUtils<PhysAdapterT, QueryAdapterT>  radialUtils (dt, physAdapter, queryAdapter);
    dAttackGuardSimulation::IntegrationUtils<PhysAdapterT, QueryAdapterT>   guardUtils  (dt, physAdapter, queryAdapter);
    dAttackMachineSimulation::IntegrationUtils<PhysAdapterT>                machineUtils(dt, staticData.m_attackSequences, physAdapter);

    auto& state        = m_allState.editState();
    auto& derivedState = m_allState.editDerivedState();
    const auto& attackBindings = m_physics.get<dAttackRadialSimulation::PhysicsDeclaration>().bindings;
    const auto& guardBindings  = m_physics.get<dAttackGuardSimulation::PhysicsDeclaration>().bindings;

    {
        auto deps = makeDependencies<dAttackMachineSimulation::Dependencies>(state);
        dAttackMachineSimulation::integrate3(dt,
            dAttackMachineSimulation::AllInput<PhysAdapterT>(
                input.get<dAttackMachineSimulation::PlayerInput>(), machineUtils),
            deps);
    }

    {
        auto deps = makeDependencies<dAttackGuardSimulation::Dependencies>(state);
        dAttackGuardSimulation::integrate(dt,
            dAttackGuardSimulation::AllInput<PhysAdapterT, QueryAdapterT>(
                input.get<dAttackGuardSimulation::PlayerInput>(), guardUtils),
            staticData.m_guardSimulationStaticData, deps,
            guardBindings, derivedState.m_guardDerivedState);
    }

    {
        auto deps = makeDependencies<dAttackRadialSimulation::Dependencies>(state);
        dAttackRadialSimulation::integrate(dt,
            dAttackRadialSimulation::AllInput<PhysAdapterT, QueryAdapterT>(
                input.get<dAttackRadialSimulation::PlayerInput>(), radialUtils),
            staticData.m_attackSimulationStaticData, deps,
            attackBindings, derivedState.m_attackDerivedState);
    }
}

template <PhysicsBodyAdapter PhysAdapterT>
void SimulatableBrawler::firstResimStep(PhysAdapterT& adapter, int32_t physicsStep)
{
    // Resim body correction is applied in the UE/Chaos layer
    // (FSimulationManagerAsyncCallback::FirstPreResimStep_Internal) via
    // FRewindData::SetTargetStateAtFrame. Direct adapter writes here bypass
    // Chaos's rewind timeline and break ResimAsFollower bodies.
    (void)adapter;
    (void)physicsStep;
}

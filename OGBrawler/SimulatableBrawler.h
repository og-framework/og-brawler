#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include "OGBrawler/SimulatableBrawlerTypes.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawler/DAttackGuardSimulation.h"
#include "OGBrawler/DAttackMachineSimulation.h"
#include "OGBrawler/BrawlerProjectileSimulation.h"
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

    // Per-character bindings (T33). Populated ONCE at registration time
    // (SimulationManagerUImpl::tryRegister) from the authoritative parentBodyId.
    // Consumed by the machine sub-sim's integrate3 (the Hadouken trigger reads the
    // capsule transform on-demand from it). See BrawlerMovementSimulation.h.
    void setCharacterBindings(const brawlerMovementSimulation::CharacterBindings& cb) { m_characterBindings = cb; }
    const brawlerMovementSimulation::CharacterBindings& getCharacterBindings() const { return m_characterBindings; }

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
    brawlerMovementSimulation::CharacterBindings m_characterBindings;
    SimulationPhysicsComposite<
        dAttackRadialSimulation::PhysicsDeclaration,
        dAttackGuardSimulation::PhysicsDeclaration,
        brawlerProjectileSimulation::PhysicsDeclaration<0>,
        brawlerProjectileSimulation::PhysicsDeclaration<1>,
        brawlerProjectileSimulation::PhysicsDeclaration<2>> m_physics;
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
    const uint32_t currentTick = step.getTick();
    dAttackRadialSimulation::IntegrationUtils<PhysAdapterT, QueryAdapterT>     radialUtils    (dt, physAdapter, queryAdapter);
    dAttackGuardSimulation::IntegrationUtils<PhysAdapterT, QueryAdapterT>      guardUtils     (dt, physAdapter, queryAdapter);
    brawlerProjectileSimulation::IntegrationUtils<PhysAdapterT, QueryAdapterT> projectileUtils(dt, currentTick, physAdapter, queryAdapter);
    dAttackMachineSimulation::IntegrationUtils<PhysAdapterT> machineUtils(dt, staticData.m_attackSequences, physAdapter, staticData.m_projectileStaticData);

    auto& state        = m_allState.editState();
    auto& derivedState = m_allState.editDerivedState();
    const auto& attackBindings = m_physics.get<dAttackRadialSimulation::PhysicsDeclaration>().bindings;
    const auto& guardBindings  = m_physics.get<dAttackGuardSimulation::PhysicsDeclaration>().bindings;

    {
        auto deps = makeDependencies<dAttackMachineSimulation::Dependencies>(state);
        dAttackMachineSimulation::integrate3(dt,
            dAttackMachineSimulation::AllInput<PhysAdapterT>(
                input.get<dAttackMachineSimulation::PlayerInput>(), machineUtils),
            deps, m_characterBindings,
            // [hit-resolution T2] Plain by-ref inbound-hit slice (mirrors CharacterBindings).
            // Populated by the manager routing pass (T3); read here to drive HitFlinch.
            derivedState.m_inboundHitDerivedState);
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
        auto deps = makeDependencies<brawlerProjectileSimulation::Dependencies>(state);
        const std::array<brawlerProjectileSimulation::RuntimeBindings, brawlerProjectileSimulation::kMaxProjectilePoolSize> projectileBindings = {
            m_physics.get<brawlerProjectileSimulation::PhysicsDeclaration<0>>().bindings,
            m_physics.get<brawlerProjectileSimulation::PhysicsDeclaration<1>>().bindings,
            m_physics.get<brawlerProjectileSimulation::PhysicsDeclaration<2>>().bindings
        };
        brawlerProjectileSimulation::integrate(dt,
            brawlerProjectileSimulation::AllInput<PhysAdapterT, QueryAdapterT>(
                input.get<brawlerProjectileSimulation::PlayerInput>(), projectileUtils),
            staticData.m_projectileStaticData, deps,
            projectileBindings, derivedState.m_projectileDerivedState);
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

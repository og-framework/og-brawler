#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"
#include "OGBrawler/SimulatableBrawlerTypes.h"
#include "OGBrawler/DAttackRadialSimulation.h"
#include "OGBrawler/DAttackGuardSimulation.h"
#include "OGBrawler/DAttackMachineSimulation.h"
#include "OGBrawler/BrawlerProjectileSimulation.h"
#include "OGBrawler/BrawlerMovementSimulation.h"
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
    // capsule transform on-demand from it). [movement-sim task 62] Defined in
    // BrawlerCharacterBindings.h, the leaf header — it is NOT in BrawlerMovementSimulation.h
    // any more. [movement-sim task 64] And the namespace is `simulatableBrawler`, not
    // `brawlerMovementSimulation`: the old name recorded a file the struct passed through, and
    // movement is the one sub-sim that never uses the type. The leaf header says why.
    void setCharacterBindings(const simulatableBrawler::CharacterBindings& cb) { m_characterBindings = cb; }
    const simulatableBrawler::CharacterBindings& getCharacterBindings() const { return m_characterBindings; }

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
    simulatableBrawler::CharacterBindings m_characterBindings;
    SimulationPhysicsComposite<
        dAttackRadialSimulation::PhysicsDeclaration,
        dAttackGuardSimulation::PhysicsDeclaration,
        brawlerProjectileSimulation::PhysicsDeclaration<0>,
        brawlerProjectileSimulation::PhysicsDeclaration<1>,
        brawlerProjectileSimulation::PhysicsDeclaration<2>,
        brawlerMovementSimulation::PhysicsDeclaration> m_physics;
};

// One assertion per declaration in m_physics above. The concept is parameterised
// on the GAME's aggregate StaticData, so this header — the one place that knows
// both — is where it can be checked at all. A declaration that drops a member, or
// whose bindings are not the shared PhysicsRuntimeBindings, fails HERE instead of
// hundreds of lines deep inside a generic fold. The projectile's three pool slots
// are asserted individually: PhysicsDeclaration<Slot> is a class template, so each
// instantiation is a separate type and only the ones named are checked.
//
// ⚠ QUALIFIED ::PhysicsDeclaration — the sub-simulation namespaces each define a
// STRUCT of that name, so the unqualified spelling is ambiguous here.
static_assert(::PhysicsDeclaration<dAttackRadialSimulation::PhysicsDeclaration,       simulatableBrawler::StaticData>);
static_assert(::PhysicsDeclaration<dAttackGuardSimulation::PhysicsDeclaration,        simulatableBrawler::StaticData>);
static_assert(::PhysicsDeclaration<brawlerProjectileSimulation::PhysicsDeclaration<0>, simulatableBrawler::StaticData>);
static_assert(::PhysicsDeclaration<brawlerProjectileSimulation::PhysicsDeclaration<1>, simulatableBrawler::StaticData>);
static_assert(::PhysicsDeclaration<brawlerProjectileSimulation::PhysicsDeclaration<2>, simulatableBrawler::StaticData>);
static_assert(::PhysicsDeclaration<brawlerMovementSimulation::PhysicsDeclaration,     simulatableBrawler::StaticData>);

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
    // [movement-sim task 11] The tick joins dt here (the projectile's shape): the Cadence
    // model commits a direction on a period boundary and the teleport seed stamps it.
    brawlerMovementSimulation::IntegrationUtils<PhysAdapterT, QueryAdapterT>   movementUtils  (dt, currentTick, physAdapter, queryAdapter);

    auto& state        = m_allState.editState();
    auto& derivedState = m_allState.editDerivedState();
    const auto& attackBindings = m_physics.get<dAttackRadialSimulation::PhysicsDeclaration>().bindings;
    const auto& guardBindings  = m_physics.get<dAttackGuardSimulation::PhysicsDeclaration>().bindings;
    const auto& movementBindings = m_physics.get<brawlerMovementSimulation::PhysicsDeclaration>().bindings;

    {
        auto deps = makeDependencies<dAttackMachineSimulation::Dependencies>(state);
        dAttackMachineSimulation::integrate3(dt,
            dAttackMachineSimulation::AllInput<PhysAdapterT>(
                input.get<dAttackMachineSimulation::PlayerInput>(), machineUtils),
            deps, m_characterBindings,
            // [hit-resolution T2] Plain by-ref inbound-hit slice (mirrors CharacterBindings).
            // Populated by the manager routing pass (T3); read here to drive HitFlinch.
            derivedState.edit<brawlerInboundHit::DerivedState>());
    }

    {
        auto deps = makeDependencies<dAttackGuardSimulation::Dependencies>(state);
        dAttackGuardSimulation::integrate(dt,
            dAttackGuardSimulation::AllInput<PhysAdapterT, QueryAdapterT>(
                input.get<dAttackGuardSimulation::PlayerInput>(), guardUtils),
            staticData.m_guardSimulationStaticData, deps,
            guardBindings, derivedState.edit<dAttackGuardSimulation::DerivedState>());
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
            projectileBindings, derivedState.edit<brawlerProjectileSimulation::DerivedState>());
    }

    {
        auto deps = makeDependencies<dAttackRadialSimulation::Dependencies>(state);
        dAttackRadialSimulation::integrate(dt,
            dAttackRadialSimulation::AllInput<PhysAdapterT, QueryAdapterT>(
                input.get<dAttackRadialSimulation::PlayerInput>(), radialUtils),
            staticData.m_attackSimulationStaticData, deps,
            attackBindings, derivedState.edit<dAttackRadialSimulation::DerivedState>());
    }

    // [movement-sim task 11] THE MOVEMENT SUB-SIM. Runs LAST, matching ExecutionOrder — and
    // its position is no longer free: it takes `ExternalDeps<const dAttackMachineSimulation::
    // State&>` to read the flinch (and, from tasks 27/31, the committed Dashing/Launched
    // states), so the machine sub-sim MUST integrate first. That edge is declared, so
    // `findFirstViolation` above enforces it rather than this comment.
    //
    // ⚠ THE SECOND ARGUMENT IS THE MACHINE'S PlayerInput, and this is the ONE PLACE the
    // movement sub-sim can get it: the stick (`moveDirectionWorld`) is packed onto the machine
    // slice, and the framework hands no sub-sim another sub-sim's input.
    // [movement-sim task 62] `brawlerMovementSimulation::integrate` now SPELLS
    // `const dAttackMachineSimulation::PlayerInput&`. It used to be a DEDUCED template
    // parameter, because the movement header could not name that type while
    // `DAttackMachineSimulation.h` included IT for `CharacterBindings` — an include cycle. The
    // cycle is gone (the struct moved to `BrawlerCharacterBindings.h`), so this argument is
    // type-checked at the call rather than deduced to whatever it is handed.
    {
        auto deps = makeDependencies<brawlerMovementSimulation::Dependencies>(state);
        brawlerMovementSimulation::integrate(dt,
            brawlerMovementSimulation::AllInput<PhysAdapterT, QueryAdapterT>(
                input.get<brawlerMovementSimulation::PlayerInput>(), movementUtils),
            input.get<dAttackMachineSimulation::PlayerInput>(),
            staticData.m_movementStaticData, deps,
            movementBindings, derivedState.edit<brawlerMovementSimulation::DerivedState>());
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

    // ⭐ [movement-sim task 50] THIS IS A NO-OP AGAIN, which is what architecture §3.5 always
    // claimed it was. Task 11 cleared the movement sub-sim's off-wire command marker here,
    // because step 6' depended on scratch a correction could not restore. Ruling #27 (A) put
    // `State::positionCmd` ON THE WIRE and made `kFlagHasCommand` step 6's gate, so the data
    // the replay needs now arrives with the correction and there is nothing left to
    // invalidate.
    //
    // ⛔ THE CLEAR WAS NEVER THE FIX, on either path, and re-adding one would REGRESS both:
    //   * an ADOPTED correction had already zeroed the scratch by whole-struct assignment
    //     (`SimulationReconciliation.h`, `editState() = cache.getState(idx)`, over a state
    //     `injectCorrectionState` DEFAULT-CONSTRUCTED before `readInto`), so the clear was
    //     redundant there;
    //   * an AGREEING anchor keeps the client's own prediction —
    //     `tryInsertingCorrectState` only overwrites the slot `if (!predictionWasCorrect)` —
    //     so the restored data was VALID and the clear DISCARDED a good push-out for a tick.
    // Both rows are pinned in `SimulatableBrawlerTest.cpp`
    // (`ReplayAfterAdoptionReproducesContactClamp`, `AgreeingAnchorKeepsPushOut`).
}

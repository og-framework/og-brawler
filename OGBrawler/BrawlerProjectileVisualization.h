#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <algorithm>   // std::max
#include "BrawlerProjectileSimulation.h"

#pragma optimize("", off)

namespace brawlerProjectileVisualization
{

class State
{
public:
    State() {}
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename RendererFunctorType>
class Input
{
public:
    Input(RendererFunctorType rendererFunctorImpl,
          const brawlerProjectileSimulation::StaticData& staticData,
          uint32_t currentTick,
          float deltaTime)
        : m_rendererFunctorImpl(rendererFunctorImpl)
        , m_staticData(staticData)
        , m_currentTick(currentTick)
        , m_deltaTime(deltaTime)
    {}

    RendererFunctorType getRendererFunctorImpl() const { return m_rendererFunctorImpl; }
    const brawlerProjectileSimulation::StaticData& getStaticData() const { return m_staticData; }
    // T30 — currentTick + dt parallel the sim's IntegrationUtils so the viz can
    // compute each indicator's residual draw lifetime from its tickStamp.
    uint32_t getCurrentTick() const { return m_currentTick; }
    float getDeltaTime() const { return m_deltaTime; }

private:
    RendererFunctorType m_rendererFunctorImpl;
    const brawlerProjectileSimulation::StaticData& m_staticData;
    uint32_t m_currentTick;
    float m_deltaTime;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename RendererFunctorType>
void visualize(const Input<RendererFunctorType>& input,
               const brawlerProjectileSimulation::State& projectileState,
               const brawlerProjectileSimulation::DerivedState& derivedState,
               State& /*state*/)
{
    auto rendererFunctor = input.getRendererFunctorImpl();
    const float radius = input.getStaticData().colliderRadius;

    for (const auto& slot : projectileState.slots)
    {
        // Closed-form (Task 13): a slot is currently flying iff it was spawned and has not
        // yet ended. endTick stays 0 until lifetime expiry / hit, at which point the
        // projectile sim parks the body off-world. bodyState.position is the transient
        // world position the sim snaps from the closed form each tick. (T13 replaced the
        // old `isAlive` data member with the isAlive(tick)/isFree(tick) predicates, so the
        // viz now uses the tick-independent spawnTick/endTick markers directly.)
        if (slot.spawnTick == 0 || slot.endTick != 0)
            continue;
        rendererFunctor.drawSphere(slot.bodyState.position, radius, 0, 0.f);
    }

    // T30 — hit / block indicators, radial-style persistence. The sim keeps each entry
    // in DerivedState for sd.indicatorPersistTicks ticks (pruning at the top of integrate),
    // and this viz redraws every frame while the entry lives. residualSeconds shrinks the
    // draw lifetime toward zero as the entry approaches its prune tick, giving a clean
    // cutoff at the end of the persist window (matches the radial guard-hit feel).
    const uint32_t currentTick  = input.getCurrentTick();
    const float    dt           = input.getDeltaTime();
    const uint32_t persistTicks = input.getStaticData().indicatorPersistTicks;

    // [hit-resolution T4] The world-space red-point damage-hit loop was removed here:
    // derivedState.hits is now T3's manager-side routing source, not a viz marker.
    // Projectile body hits are shown target-side via the HitFlinch sphere
    // (DAttackTargetVisualizationTwo).

    // Block → 15 cm blue (colorId 2) sphere, the EXACT radial guard-hit call shape,
    // but with a time-based residual lifetime (the projectile has no attack-sequence
    // boundary to clear against, so persistence is tick-stamp driven).
    for (const auto& block : derivedState.blocks)
    {
        const float residualSeconds = std::max(0.f,
            (static_cast<int>(block.tickStamp + persistTicks) - static_cast<int>(currentTick)) * dt);
        rendererFunctor.drawSphere(block.position, 15.f, 2, residualSeconds);
    }
}

} // namespace brawlerProjectileVisualization

#pragma optimize("", on)

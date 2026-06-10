#pragma once
// SPDX-License-Identifier: BUSL-1.1

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
          const brawlerProjectileSimulation::StaticData& staticData)
        : m_rendererFunctorImpl(rendererFunctorImpl)
        , m_staticData(staticData)
    {}

    RendererFunctorType getRendererFunctorImpl() const { return m_rendererFunctorImpl; }
    const brawlerProjectileSimulation::StaticData& getStaticData() const { return m_staticData; }

private:
    RendererFunctorType m_rendererFunctorImpl;
    const brawlerProjectileSimulation::StaticData& m_staticData;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename RendererFunctorType>
void visualize(const Input<RendererFunctorType>& input,
               const brawlerProjectileSimulation::State& projectileState,
               State& /*state*/)
{
    auto rendererFunctor = input.getRendererFunctorImpl();
    const float radius = input.getStaticData().colliderRadius;

    for (const auto& slot : projectileState.slots)
    {
        if (slot.isAlive == 0)
            continue;
        rendererFunctor.drawSphere(slot.bodyState.position, radius, 0, 0.f);
    }
}

} // namespace brawlerProjectileVisualization

#pragma optimize("", on)

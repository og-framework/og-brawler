#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/BrawlerCharacterBindings-rationale.md · docs/BrawlerCharacterBindings-guards.md

// ⛔G-01  docs/BrawlerCharacterBindings-guards.md
#include "OGSimulation/BodyId.h"

// ⛔G-02  docs/BrawlerCharacterBindings-guards.md
namespace simulatableBrawler
{

struct CharacterBindings
{
    BodyId capsuleBodyId;
};

static_assert(!Serializable<CharacterBindings>,
    "simulatableBrawler::CharacterBindings is NOT on the wire, and adding a "
    "SerializableFields specialization for it is a WIRE CHANGE, not a convenience. Nothing "
    "in the tree serializes this type; the capsule id reaches a peer as the movement "
    "sub-simulation's own body id, never as this handle. Was fence G-03 of "
    "docs/BrawlerCharacterBindings-guards.md, now retired; rationale section 4.");
static_assert(Serializable<BodyId>,
    "VACUITY CONTROL for the assertion above, and it is not decoration: !Serializable<T> is "
    "true of every SimulationComposite, the on-wire simulatableBrawler::State included, so "
    "the bare form can be a fence that cannot fail. BodyId is this struct's one member and "
    "IS serialized, so this arm proves the predicate discriminates. If this line ever fails, "
    "the assertion above has stopped meaning anything. Guards G-03, retired.");

}

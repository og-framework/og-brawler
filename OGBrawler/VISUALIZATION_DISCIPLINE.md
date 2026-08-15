<!-- SPDX-License-Identifier: BUSL-1.1 -->
# Visualization discipline — the mesh-only invariant

**Scope:** the `*Visualization*` translation units in this directory
(`DAttackAimVisualization`, `DAttackRadialVisualization`,
`DAttackBlockPredictionVisualization`, `DAttackTargetVisualization`,
`DAttackTargetVisualizationTwo`, `BrawlerProjectileVisualization`, plus the shared
`DAttackVisualizationUtils` / `CharacterVisualizationData`).

**Why this file exists:** risk **R-UE1** in `risks_and_plan.md` — *"presentation-only
invariant" code-discipline leak into hitbox calculation*. This is the Stage 5 / D5.5
deliverable for that mitigation.

> **Note on location.** There is no `Visualization/` subdirectory. The visualization
> free functions live here, alongside their simulation siblings. The architecture
> documents describe a `Visualization/README.md`; that directory split was never
> made and — per proposal §7.4 — was never needed, because the invariant is enforced
> by parameter types rather than by directory boundaries. This doc therefore sits
> beside the headers it governs.

---

## 1. The invariant

**Visualization components are STRUCTURALLY prevented from affecting sim state or
hitbox calculation.**

The `visualize(...)` signature enforces this by parameter types: sim-side parameters
are `const&` (compile-time immutable), the only mutable parameter is the
visualization state, and **any function that mutates a sim-side parameter, or passes
visualization state INTO a sim-side function, is a bug.**

Concretely, from `DAttackTargetVisualization.h`:

```cpp
template <typename SpatialQueryAdapterType, typename RendererFunctorType>
void visualize(const Input<SpatialQueryAdapterType, RendererFunctorType>& input,
    State& state,                                              // <- the ONLY mutable parameter
    const dAttackRadialSimulation::StaticData& staticData,     // <- const: cannot mutate sim
    const dAttackRadialSimulation::State& simulationState,     // <- const
    const dAttackRadialSimulation::DerivedState& derivedState, // <- const
    const dAttackMachineSimulation::State& machineSimulationState,
    const dAttackRadialSimulation::State& radialState)
```

The mutable visualization `State&` does not occupy a fixed position across the
family — it is the second parameter here and the fourth in
`BrawlerProjectileVisualization::visualize`. **The rule is about `const`-ness, not
ordering:** exactly one non-`const` sim-adjacent parameter, and it is always the
visualization state.

### What this permits

Visualization **reads post-sim state through a `const&`**. That is the design, not a
tolerated exception. Three headers in this directory include
`DAttackMachineSimulation.h` purely to name the
`const dAttackMachineSimulation::State&` parameter they render from. That is correct
and must stay legal.

### What it forbids

- Writing through any sim-side parameter (blocked by `const`).
- Passing visualization state into a sim-side function.
- Reaching the **hit resolver** — the code that decides whether a hit landed.
- Any **side channel** that carries visual-blended state back into sim: a static or
  singleton a sim-side component later reads, a service call, a functor bound at the
  call site that mutates sim state.

The last one is the dangerous case. **The signature does not catch it** — R-UE1
exists precisely because a clean signature can coexist with a dirty side channel.
Sections 3 and 4 are the arms that cover it.

---

## 2. Precedent: hitbox math is server-only

The four-layer latency-hiding scheme (input delay → speculative render → correction
blend → render-side input echo) is built on one rule: **no visual-blended transform
ever enters hitbox calculation.** Layers 3 and 4 exist to make corrections and input
response *look* smooth; if either leaks into hit resolution, the game computes hits
against a transform that no other machine agrees on.

**For Honor** is the shipping precedent for the surrounding posture. Ubisoft's
12-month P2P → dedicated-server migration
(`deepdive_shipping_fighters.md` §FH-1, §FH-3) established that the simulation model
survives the transport change intact: clients still simulate, but **the server is the
authority on confirmed frames**, and hit resolution runs against server-authoritative
state on the server only. OGBrawler is already in the corrected architecture — this
document is about not regressing out of it one visualization component at a time.

The failure mode itself is the **asymmetric hit-detection problem**
(`synthesis_patterns.md` §B Axis g gate (c); `deepdive_lockstep_prediction.md`
Pattern 8, CrystalOrb present-time simulation). In P2P rollback, remote entities are
deliberately displayed slightly in the past to reduce correction frequency, so a
punch can land locally and miss remotely — hit detection disagrees between machines.
Server-authoritative present-time simulation avoids this **by construction**, which
for a melee brawler where hitbox timing decides fights is the whole ballgame. That
guarantee is only as strong as the discipline that keeps blended, client-local,
render-rate transforms out of the hitbox path.

> **Citation correction (2026-07-20).** `risks_and_plan.md` R-UE1 and this task's
> backlog entry both cite *"Task 10 §FH-2 asymmetric hit-detection"*. §FH-2 is about
> **hash verification and desync detection**, not hit detection; the For Honor
> deepdive contains no asymmetric-hit-detection material. The accurate citations are
> the ones used above: **FH-1/FH-3** for the server-authoritative precedent, and
> **`deepdive_lockstep_prediction.md` Pattern 8 / `synthesis_patterns.md` §B Axis g**
> for asymmetric hit detection. The upstream docs should be corrected.

---

## 3. Code-review checklist

Paste into PR review comments verbatim when a change touches a visualization
component:

- [ ] Does this visualization component read or mutate any sim-side state via a
      non-`const` reference or a side channel?
- [ ] Does any `*Visualization*` file `#include` a hitbox-resolution or
      sim-integration header?
- [ ] Does any visualization component write to a static or singleton that a
      sim-side component then reads?

Review a new visualization component for *"does this affect hitbox calculation?"*
the same way a new physics adapter is reviewed for *"does this affect cross-platform
determinism?"* — as a standing question, not an occasional one.

**On checklist item 2:** the automated lint (§4) covers the *hitbox-resolution*
half mechanically. The *sim-integration* half stays a human judgment call, because
naming a sim state type in a `const&` parameter is legitimate and common here — see
§1 "What this permits". Ask whether the include is there to **read state** (fine) or
to **invoke sim behaviour** (not fine).

---

## 4. Automated enforcement

```pwsh
pwsh tools/lint/visualization_hitbox_isolation.ps1
```

Fails the build (exit 1) if any `*Visualization*` translation unit in this directory
`#include`s a hitbox-**resolution** header — currently `BrawlerHitRouting*.h` (owns
`routeInbound`, writes `wasHitThisTick`) and `BrawlerInboundHit.h` (the derived-state
payload hit resolution produces). Matching is on the include's leaf filename, so
every path spelling is caught; comments are stripped, so a commented-out include is
not a violation.

Sim-state **type** headers (`DAttackMachineSimulation.h`,
`DAttackRadialSimulation.h`, `DAttackGuardSimulation.h`,
`BrawlerProjectileSimulation.h`) are deliberately **not** blacklisted — see §1.

> **A green lint is not proof the invariant holds.** It sees `#include` lines only.
> A side channel needing no include — a static, a singleton lookup, a functor bound
> to sim-mutating behaviour at the call site — is invisible to it. That is what §3
> is for. The lint proves the *cheapest* way to break the invariant is blocked.

**Contract:** a new header that **resolves** hits (decides hit/no-hit, writes
inbound-hit derived state, owns the routing pass) gets a blacklist entry in
`visualization_hitbox_isolation.ps1` in the same change. A new header that merely
**declares** sim state types does not.

Runs in the same CI step as the R-P1 configurability lint — see
`tools/lint/README.md`. Note that CI is currently **parked**
(`.github/workflows/standalone-tests.yml.disabled`), so both lints run manually or
as a pre-commit step until it is re-enabled.

---

## 5. Escalation

Per R-UE1's escalation trigger: **any PR that introduces a side channel from a
visualization component to a sim-side service** should be escalated to the netcode
initiative owner rather than resolved in review.

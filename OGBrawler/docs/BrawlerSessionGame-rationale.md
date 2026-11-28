<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `BrawlerSessionGame.h` — rationale

Companion to og-brawler's `OGBrawler/BrawlerSessionGame.h` (og-session-runtime task 26). Prohibitions:
`BrawlerSessionGame-guards.md`.

**If this file and the header disagree, the header is authoritative and this file is stale.**

---

## §1 What it is

`BrawlerSessionGame` holds the brawler's session statics that name no engine type: the facts an engine-free session runtime asks
of the game (og-session-runtime design §3.5, question Q13: "move only the statics that are already engine-free"). Unreal's
`BrawlerSessionGameUImpl` (root `Source/OGBrawlerUnreal/`) forwards to them; a Godot adapter would do the same.

| static | what |
|---|---|
| `neutralInput()` | the input the resolution uses for a tick with none: `simulatableBrawler::getZeroPlayerInput()`, whose aim is `(0,0,1)` (⛔G-01) |
| `AuthorityPresentationSlice` | the authority-only state the frame presents for one character: its ring-out score and whether the score system has an entry for it (default: 0, none) |
| `rootBodyId(simulatable)` | the character's own body, the movement composite's `PhysicsDeclaration::bindings.ownBodyId` |

Both functions are `inline` and compiled with optimisation off (`OGSIM_OPTIMIZE_OFF` pair), as their bodies were in the Unreal
class's header and `.cpp`.

## §2 What moved, and what did not

og-session-runtime task 26 step (a) moved the three items above unchanged from `BrawlerSessionGameUImpl.h` / `.cpp` (root commit
`6e7edb6`); step (b) gave this header its own documents, its guard a new id here (the Unreal file's G-01 is retired) and the
slice's score its og-brawler spelling `uint32_t` (CoreTypes' `uint32` at (a); og-simulation's `OGTypes.h` declares the same
alias for standalone builds, so both compiled). The design's list named more; they stay in the Unreal class:

| static | why it stays |
|---|---|
| `emplaceSystems(…)` | it names the engine's adapters (`EngineTraits::BodyAdapter`, `QueryAdapter`, the Jolt traits) and the Unreal composition's `SystemsExec` |
| `authorityPresentationSlice_Step(systems, id)` | it names the Unreal composition's `SystemsExec`, whose hit-detection system is built over the Jolt adapters; its body reads only the score system, so a version templated on the executor could move (left for later) |
| `backendFingerprint()` | it is `OGBuildIdentityUImpl.h`'s (the engine's CoreMinimal.h, FString), which resolves the Jolt determinism fingerprint |
| `installGameLoggers(isAuthority)` | it installs an Unreal log sink (`FString`, `UE_LOG`, `LogOGBrawler`) |
| `gravity` | it is not a static of either class: the session hooks' `BrawlerSessionHooksUImpl::gravity(staticData)` |
| `tickFrequencyHz` | it is not a static of either class: `UEBrawlerNetConfig::tickFrequencyHz`, in a file that includes og-sim-net |

og-brawler includes no og-tickdriver, og-simulation-jolt, og-sim-net or og-net header (its row in the module tier table); this
header includes og-simulation's `BodyId.h` and `CompilerControl.h` and og-brawler's `SimulatableBrawler.h`.

## §3 Tests

og-brawler-tests' `BrawlerSessionGameTest.cpp` (`[SimulatableBrawler]`, in the `[@og]` set): the neutral input equals the zero
input field by field and differs from a value-initialised input in its aim; the slice defaults to no score entry and carries its
score as `uint32_t`. The test includes this header first.

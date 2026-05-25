<!-- SPDX-License-Identifier: BUSL-1.1 -->
# og-brawler

OG Brawler is a brawler-specific gameplay framework built on [og-simulation](https://github.com/og-framework/og-simulation). It provides deterministic brawler simulation logic — attack arcs, guard states, radial sequences, input mapping, and character binding — all running inside the og-simulation tick loop.

## Position in the og-framework graph

og-brawler is the **pure C++ brawler core**. It has no build-system root and no Unreal Engine dependencies. It depends on og-simulation at link time but does not submodule it — the parent build supplies the `og_simulation` and `glm::glm-header-only` targets.

```
og-simulation  (dependency — not submoduled here)
    ↑ linked by
og-brawler  (this repo — pure source)
    ↓ consumed by
og-brawler-ue     — UE plugin shell
og-brawler-tests  — Catch2 test source
    ↓ assembled by
og-tests-cmake-runner — CMake build + test runner
og-brawler-unreal     — UE game project
```

## Related repos

| Repo | Role |
|---|---|
| [og-simulation](https://github.com/og-framework/og-simulation) | Simulation core this repo depends on |
| [og-brawler-ue](https://github.com/og-framework/og-brawler-ue) | Wraps this repo as a UE plugin |
| [og-brawler-tests](https://github.com/og-framework/og-brawler-tests) | Catch2 tests for this repo |
| [og-tests-cmake-runner](https://github.com/og-framework/og-tests-cmake-runner) | CMake harness that builds + runs tests |
| [og-brawler-unreal](https://github.com/og-framework/og-brawler-unreal) | UE game project; consumes this via og-brawler-ue |

## Quickstart

og-brawler is a **source distribution** — not directly buildable on its own. Consume it via a parent build:

**CMake (tests / standalone):** clone [og-tests-cmake-runner](https://github.com/og-framework/og-tests-cmake-runner) with `--recurse-submodules`. It assembles the full tree (og-simulation + og-brawler + Catch2 + glm).

```bash
git clone --recurse-submodules https://github.com/og-framework/og-tests-cmake-runner
cd og-tests-cmake-runner
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

**Unreal Engine:** use the [og-brawler-ue](https://github.com/og-framework/og-brawler-ue) plugin shell, consumed via [og-brawler-unreal](https://github.com/og-framework/og-brawler-unreal).

In your own CMake consumer, the parent build must provide `og_simulation` and `glm::glm-header-only` before adding this module:

```cmake
add_subdirectory(extern/og-simulation/OGSimulation)   # provides og_simulation
add_subdirectory(extern/og-simulation/Source/glm)     # provides glm::glm-header-only
add_subdirectory(extern/og-brawler/OGBrawler)
target_link_libraries(MyTarget PRIVATE og_brawler)
```

## Canonical workflow

See [`og-brawler-unreal/docs/cross-repo-dev-loop.md`](https://github.com/og-framework/og-brawler-unreal/blob/main/docs/cross-repo-dev-loop.md) for the multi-repo development workflow (submodule push order, pin management via og-tools).

## License

**[Business Source License 1.1](LICENSE)** — converts to **[MPL-2.0](LICENSES/MPL-2.0.txt)** on the Change Date printed in `LICENSE` (currently `2030-06-01`).

**What this means in practice:**

| Use case | Allowed? |
|---|---|
| Non-commercial use (personal, educational, research, hobby, open-source) | ✅ Yes |
| Commercial use in any product that is *not* a multiplayer brawler | ✅ Yes |
| Use in a software product or service whose primary gameplay is multiplayer character-vs-character melee combat (a "Competing Product") | ⛔ Please contact the maintainer to discuss |
| Modify and contribute back via PR | ✅ Yes (via [CLA](https://github.com/og-framework/og-tools/blob/main/Public/license-templates/CLA-process.md)) |

After the Change Date, the codebase converts automatically to MPL-2.0 and these restrictions lift.

**Unsure if your use is permitted? Have an interesting idea?**
Reach out to [grahnen92@gmail.com](mailto:grahnen92@gmail.com) — the Licensor welcomes such conversations and is open to case-by-case exceptions.

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the contribution decision tree and CLA signing flow. Contributions land under each file's SPDX header (`BUSL-1.1` for all source in this repo); the CLA additionally grants relicensing rights per Section 4.

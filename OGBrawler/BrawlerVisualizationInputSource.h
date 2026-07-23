#pragma once
// SPDX-License-Identifier: BUSL-1.1

// Pure, engine-agnostic statement of the D5.4 render-side-input-echo SOURCE RULE:
// given a character, which of the two available player-input sources should its
// input-carrying visualizations read this render frame?
//
// The rule (og-netcode-v2 T13 / D5.4 caller half):
//
//   LOCAL character  -> buildLatestVisualizationInput(), sampled LIVE at
//                       render-frame rate. Continuous fields are re-read every
//                       frame, so cosmetic cues (weapon direction, aim indicator,
//                       block-prediction wedge) move at display rate instead of
//                       stepping at the 60 Hz sim tick.
//
//   REMOTE proxy     -> CorrectionCache::getLatestInput(), unchanged. There is no
//                       live local input for a remote player; the server-corrected,
//                       tick-quantized cache value is the ONLY source available,
//                       and echoing our own local input onto someone else's
//                       character would be plainly wrong.
//
// Why this rule lives in the core rather than inline at the call site:
//   * it is the substance of T13, and og-brawler-tests cannot link the UE
//     translation unit (SimmableUpdateComponent.cpp) that applies it. Expressed
//     here it is directly testable, exactly like T12's packers.
//   * both input-carrying viz sites must agree; one shared definition makes
//     divergence between them impossible rather than merely unlikely.
//
// MESH-ONLY INVARIANT (proposal §2.3 / R-UE1, see VISUALIZATION_DISCIPLINE.md).
// Everything selected here is COSMETIC. The returned PlayerInput must be passed
// only to visualize() functions, which take every simulation-side parameter by
// const reference and mutate nothing but their own visualization State. It must
// never reach the simulation, the input RPC, or a correction cache. On the local
// path the value is not even tick-aligned — it is a mid-tick sample that no
// authority ever saw — so feeding it to the sim would desync by construction.
//
// NOTE (T13 scope): there is deliberately NO tier consult here. Muting the echo
// on a degraded connection tier is optional task T15 and must not leak into this
// rule; T13 always echoes on the local character.

#include <optional>
#include <utility>

#include "OGBrawler/SimulatableBrawlerTypes.h"

namespace simulatableBrawler
{

// Selects the visualization input source for one character, for one render frame.
//
//   hasLiveLocalInput : true iff this character is driven by a live local input
//                       component. This is the local-vs-remote discriminator, and
//                       it is deliberately phrased as "is there live input to
//                       read" rather than "is the net role X" — the two differ on
//                       a listen server, where the host's own pawn is
//                       ROLE_Authority yet DOES have live local input. See the
//                       listen-server note below.
//
//   sampleLive        : nullary callable returning a PlayerInput. Invoked exactly
//                       once, and ONLY on the local path. The laziness is part of
//                       the contract, not an optimisation: a remote proxy must not
//                       observe a live read at all, and a test can assert that by
//                       counting invocations.
//
//   cachedInput       : the tick-quantized correction-cache read-back. Returned
//                       verbatim on the remote path, INCLUDING nullopt (a cold or
//                       absent cache means the viz is skipped this frame, which is
//                       the pre-existing behaviour T13 must not change).
//
// LISTEN-SERVER IMPROVEMENT (intended, new in T13): getLatestInput returns nullopt
// on the authority because the authority keeps no correction caches, so the host's
// own input-carrying viz was previously skipped on every frame. The live sampler
// has no such gap, so the host now echoes like any other local character. This is
// a strict improvement, and it falls out of the rule rather than being special-cased.
template <typename LiveSampler>
std::optional<PlayerInput> selectVisualizationInput(bool hasLiveLocalInput,
                                                    LiveSampler&& sampleLive,
                                                    const std::optional<PlayerInput>& cachedInput)
{
    if (hasLiveLocalInput)
        return std::optional<PlayerInput>(std::forward<LiveSampler>(sampleLive)());

    return cachedInput;
}

} // namespace simulatableBrawler

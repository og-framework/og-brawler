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
//   REMOTE proxy     -> the LAST RELAYED INPUT for that character
//                       (SimulationNetSync::getLastRelayedInput, i.e. the relay
//                       store's `lastKnown`). There is no live local input for a
//                       remote player; the server-published, tick-quantized value
//                       is the ONLY source available, and echoing our own local
//                       input onto someone else's character would be plainly wrong.
//
// [og-netcode-v2-input-relay T7] THE REMOTE SOURCE MOVED, THE RULE DID NOT.
// It used to be `CorrectionCache::getLatestInput()`, fed by the SERVER->CLIENT
// correction-INPUT channel — **[T8] which is now deleted, making the relay store
// the only remote source that exists**. The relay store carries the same
// character's real input from the same authority, so the rule above is untouched
// in substance; what changed is which structure holds the value. Two consequences
// worth knowing here:
//   * FRESHER, not merely different. The correction channel's input is what the
//     server applied ~1 RTT ago and had to travel back to us; a relayed entry is
//     published on the same schedule but is the input the authority is applying
//     (or is about to apply) — it is the same value the sim's own proxy prediction
//     runs on, so the cue and the simulated pose can no longer be sourced from two
//     different generations of the same player's input.
//   * THE NULLOPT CONTRACT IS IDENTICAL, deliberately, so this rule needed no
//     change at all: a remote character with nothing relayed yet yields nullopt
//     exactly as a cold correction cache did, and the caller still skips the
//     input-carrying viz for that frame.
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
//   relayedInput      : the tick-quantized last-known relayed input for this
//                       character ([T7]; was named cachedInput when it came from
//                       the correction cache). Returned verbatim on the remote
//                       path, INCLUDING nullopt (an absent or cold SOURCE means
//                       the viz is skipped this frame, which is the pre-existing
//                       behaviour neither T13 nor T7 may change).
//
// LISTEN-SERVER IMPROVEMENT (intended, new in T13; UNAFFECTED by T7's re-source):
// the remote-path value is nullopt on the authority — before T7 because the
// authority keeps no correction caches, after T7 because it allocates no relay
// stores — so the host's own input-carrying viz was previously skipped on every
// frame. The live sampler has no such gap, so the host echoes like any other local
// character. That is a strict improvement, and it falls out of the rule rather than
// being special-cased; note it does not even depend on the remote source's
// emptiness, since the host takes the LOCAL branch on `hasLiveLocalInput`.
template <typename LiveSampler>
std::optional<PlayerInput> selectVisualizationInput(bool hasLiveLocalInput,
                                                    LiveSampler&& sampleLive,
                                                    const std::optional<PlayerInput>& relayedInput)
{
    if (hasLiveLocalInput)
        return std::optional<PlayerInput>(std::forward<LiveSampler>(sampleLive)());

    return relayedInput;
}

} // namespace simulatableBrawler

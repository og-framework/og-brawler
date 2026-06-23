#pragma once
// SPDX-License-Identifier: BUSL-1.1

// Product-target session constants — the ratified player-count envelope for the
// v1 netcode initiative (og-netcode-v1-impl, Phase 2a, added 2026-06-15).
//
// These are the single source of truth for "how many players a server session
// supports" and "how many local players a single client connection drives."
// They are visible to all future bandwidth / topology decisions (Phase 2a's
// MaxClientRate ceiling spike measures against the worst-case topologies these
// imply). NO production logic enforces these limits yet — the actual
// "reject the 4th player joining a Tier 1 session" gate belongs to a future
// server-session-management task and is OUT OF SCOPE for this initiative.
//
// Tier rationale:
//   Tier 1 (MVP, must ship): maxPlayersPerServer = 3 player-controlled
//     characters. Supports the playtest topologies:
//       * 1 client x 3 LPs   (couch-co-op, single-PC, 3 gamepads)
//       * 3 clients x 1 LP    (3 remote players)
//       * mixed               (e.g. 1 client x 2 LPs + 1 client x 1 LP)
//   Tier 2 (stretch, deferrable): maxPlayersPerServerTier2 = 6 player-controlled
//     characters, still up to maxLocalPlayersPerClient per client. Phase 2a (this
//     initiative's bandwidth ceiling spike) advises whether Tier 2 ships with v1
//     or is deferred to a follow-on initiative (defer-if-expensive policy).
namespace og::brawler::session
{
	// Maximum player-controlled characters per server session.
	// Tier 1 (MVP, must ship): 3 — supports playtest scenarios:
	//   1 client x 3 LPs (couch-co-op single-PC), 3 clients x 1 LP (3 remote),
	//   or mixed (1 client x 2 LPs + 1 client x 1 LP).
	// Tier 2 (stretch, deferrable): 6 — full target, up to maxLocalPlayersPerClient
	// per client. Phase 2a (this initiative's bandwidth ceiling spike) advises whether
	// Tier 2 ships with v1 or is deferred to a follow-on initiative.
	// Source: og-netcode-v1-impl initiative Phase 2a (added 2026-06-15).
	static constexpr int maxPlayersPerServer = 3;       // Tier 1 default; bump to 6 when Tier 2 lands
	static constexpr int maxPlayersPerServerTier2 = 6;  // Tier 2 stretch target

	// Maximum local players per client connection. Path A multi-`ULocalPlayer`-per-client
	// architecture (shipped OGBrawlerMultiPlayerPerClient 2026-05-30) supports up to 3 LPs
	// per parent UNetConnection via UChildConnection. Value is constant across Tier 1 and Tier 2.
	static constexpr int maxLocalPlayersPerClient = 3;
}

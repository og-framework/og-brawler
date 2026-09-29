<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: UGameInstance::GetMapOverrideName -- the engine's own startup-URL parse; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: FParse::Token -- the engine's own command-line tokenizer; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: FParse::Value -- the engine's own command-line value parse; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: FCommandLine::Get -- the engine's own command line; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: UGameViewportClient::MaxSplitscreenPlayers -- the engine's own local-player cap; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: UGameViewportClient::InputChar -- the engine's own typed-character hook; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: ENetworkFailure::OutdatedClient -- the engine's own failure enumerator; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: ENetworkFailure::PendingConnectionFailure -- the engine's own failure enumerator; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: ENetworkFailure::FailureReceived -- the engine's own failure enumerator; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: ENetworkFailure::ConnectionTimeout -- the engine's own failure enumerator; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: UEngine::GetSmallFont -- the engine's own fixed-size debug font; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: AHUD::GetTextSize -- the engine's own HUD text measure; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: ENetworkFailure::OutdatedServer -- the engine's own failure enumerator; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: ENetworkFailure -- the engine's own network failure enum; owned by the host engine, not by this repository, and must not resolve here -->
<!-- lint-external-ref: GIsEditor -- the engine's own editor flag; owned by the host engine, not by this repository, and must not resolve here -->

# The join screen's model and layout — rationale

Companion to `OGBrawler/BrawlerJoinScreen.h`, which holds everything the join screen decides:
the address field, the address parser, the recent-address list and its one-line storage form,
the "This PC" entry, the screen's phases, the failure texts, the local co-op hint, the notice for
a refused extra local player, the gamepad focus model and the layout.
**The header carries the code; this file carries the reasoning.** Its prohibitions are in
`BrawlerJoinScreen-guards.md`.

The UE layer that draws the screen, feeds it input, keeps its state across map loads and stores
the recent list is not written yet (tasks 12, 13 and 15 of the Steam-upload initiative). This file
states what that layer may rely on. ⛔ **Where the UE layer's own rationale is written, a
correction to a shared claim is owed to both documents**, exactly as for the scoreboard pair.

---

## 1. Why the model is pure code

`Source/OGBrawlerTests` links `{ Core, OGSimulation, OGBrawler }` and **not** `OGBrawlerUnreal`,
so anything written against a canvas, an `FString` or an engine delegate is untestable by
construction. The scoreboard (`BrawlerScoreboardVisualization.h`) set the pattern and this header
follows it: every decision that has a right and a wrong answer is made here, and the UE layer is
left with the calls only the engine can make.

| decision | where it lives | reachable by a Catch2 case |
|---|---|---|
| which characters the field accepts, the cursor, paste | `AddressEditBuffer` | yes |
| whether a text is an address, and its canonical form | `parseServerAddress` | yes |
| the recent list's order, dedupe, cap and storage line | `RecentAddressList` | yes |
| the address the command line carries | `commandLineMapOverrideToken` | yes |
| the phase, and which failure is shown | `JoinScreenModel` | yes |
| every text the screen or the HUD notice shows | `joinFailureText`, `localCoopHintText`, `localPlayerLimitNoticeText`, `JoinScreenModel::statusLine` | yes |
| gamepad focus | `navigatedFocus`, `JoinScreenModel::navigate` | yes |
| every rectangle, and the scale clamp | `placedJoinScreenLayout`, `clampJoinScreenScale` | yes |
| turning an engine failure into a `JoinFailureReason` | UE (task 13) | no |
| reading the command line, the clipboard, the ini | UE (task 12) | no |
| the travel call, and where the model lives across map loads | UE (task 12) | no |
| glyph widths, the caret's x, canvas calls | UE (task 12) | no |

The UE layer turns engine facts into plain values (a failure reason, a string, a key) and hands
them to the model. It computes no text and no position of its own.

---

## 2. The address field — `AddressEditBuffer`

### 2.1 The allowed set and the length

`isAddressChar` accepts `[0-9A-Za-z.:-]` and nothing else: exactly what an IPv4 address, a DNS
host name and a `:port` suffix need. A space, `/`, `?`, `_`, `@` and every non-ASCII character are
refused at the keystroke, so the field can only ever hold text that has a chance of parsing.

`kAddressMaxLength` is 64. A DNS name may be 253 characters, but the field is drawn in one line of
the engine's small font; 64 characters is wider than any host name a playtest will use (a cloud
host name such as `ec2-203-0-113-25.compute-1.amazonaws.com:7777` is 45). `parseServerAddress` applies the same limit, so a stored or command-line address
cannot be longer than the field can show.

The buffer is a class with private members, not a plain struct. That makes the three invariants —
only allowed characters, at most 64 of them, cursor inside the text — impossible to break from
outside: every mutation goes through a member that checks them.

### 2.2 ⭐ A typed character is tested as a wide character, never after narrowing

`insertTyped` is a template that accepts `wchar_t`, `char16_t` or `char32_t` and nothing else; a
`char` is a compile error (the `static_assert` in the template, which names this section).

The reason is a real trap. The engine delivers a typed character to `UGameViewportClient::InputChar`
as a `TCHAR` (16 bits on Windows). The obvious adapter code narrows it to `char` and tests that.
Narrowing keeps the low byte, so **U+013A narrows to `:` and U+012E to `.`** — a character the
allowed set must refuse would be accepted as a separator. The test case
`JoinScreen.AWideCharacterIsTestedBeforeAnyNarrowingSoU013ANeverBecomesAColon` shows both
narrowings and shows the wide path refusing them.

⭐ **Measured poison arm (2026-09-29):** a test line `buffer.insertTyped(static_cast<char>(0x13A))`
fails the OGBrawlerTests build with the assertion's message (C2338); removed, the build is green.

A UTF-16 surrogate half (an emoji arrives as two) is above `0x7F` and is refused like any other
non-ASCII unit.

### 2.3 Paste is trimmed, then all or nothing

`insertPasted` trims ASCII whitespace from both ends (a copied address often carries a newline),
then inserts **the whole text or none of it**:

* `NothingToPaste` — empty after trimming;
* `InvalidCharacter` — any character outside the allowed set, including an interior space;
* `TooLong` — the field would exceed 64 characters;
* `Inserted` — inserted at the cursor, and the cursor moves past it.

⛔ Skipping the bad characters instead would join the pieces: `1.2.3.4 7777` would become
`1.2.3.47777`, a different and valid-looking address. A refused paste leaves the field unchanged
and the UE layer can say why.

`insertPasted` accepts any character type, including `char` (UTF-8 bytes from a clipboard are fine:
every byte of a multi-byte character is above `0x7F` and is refused). Only the single-character
path forbids `char`, because only there does a narrowing cast look natural.

### 2.4 The cursor, and who draws the caret

The cursor is an index into the text. `backspace` removes the character before it,
`deleteForward` the one after it; both return `false` at the ends. The moves (`moveCursorLeft`,
`moveCursorRight`, `moveCursorToStart`, `moveCursorToEnd`) return whether anything moved.

The caret's x position depends on glyph widths, which only the engine knows. The model offers
`textBeforeCursor()`; the UE layer measures that prefix (`AHUD::GetTextSize`) and draws the caret
at its width.

`assign` replaces the whole text (trimmed) and puts the cursor at the end; it refuses, and changes
nothing, if the text could not have been typed.

### 2.5 `asciiText`, for text the engine hands over as `FString`

The command-line token, the clipboard and the stored recent line arrive as wide strings.
`asciiText` converts one to a `std::string` and returns nothing if any unit is above `0x7F`, so the
UE layer never has to narrow by hand (§2.2).

---

## 3. `parseServerAddress`

**Grammar.** `host` or `host:port`, after trimming whitespace. The checks run in this order, and
the first failure is the error returned:

1. `Empty`, then `TooLong` (over 64), then `InvalidCharacter` (outside §2.1's set).
2. `TooManyColons` — a second `:`. IPv6 is not supported; its error text says so.
3. `MissingHost` — nothing before the `:`.
4. The port, when there is a `:`: `MissingPort` (nothing after it), `PortNotNumeric`,
   `PortOutOfRange` (0, or above 65535; a long run of digits saturates rather than overflows).
   Leading zeros in a port are harmless and accepted (`007777` is 7777).
5. The host: `InvalidIPv4` or `InvalidHostname`.

⚠ **Because the port is checked before the host, `1:` reports `MissingPort`, not a bad IPv4
address.** The suite pins that (`JoinScreen.AnInvalidFieldIsRefusedAndTheErrorStaysUntilTheNextEdit`);
it was first written expecting the other answer and failed.

**IPv4 or host name.** A host made only of digits and dots is an IPv4 address and must be four
octets 0–255, none empty and none with a leading zero (`01.2.3.4` is refused: some resolvers read
a leading zero as octal, so the address would not mean what it looks like). Anything else is a
host name: dot-separated labels of 1–63 characters, not starting or ending with `-`. So `7777` on
its own (a port typed without a host) is an invalid IPv4 address, and `1.2.3.4a` is a host name
that will simply fail to resolve.

**The default port is 7777** (`kDefaultServerPort`), the port `playtest_server.bat`,
`playtest_client.bat`, `tools/run_server_template.bat` and `tools/run_client_template.bat` default
to, and the default of the og-tools host launcher.

**Canonical form.** `ServerAddress::canonical()` is `host:port` with the port always written and a
host name lowercased (host names are case-insensitive). Two spellings of one server therefore
compare equal, which is what the recent list's dedupe relies on (§4).

**Deliberately not accepted:** URL options (`127.0.0.1:7777?InitialConnectTimeout=8` — `?` is
outside the set) and map paths (`/Game/...`). Both can appear on a command line; §6 says what the
screen does with them.

Every error has its own non-empty text (`addressParseErrorText`), written for a player, and
`None` has none. The suite checks that no two texts are equal.

---

## 4. The recent-address list — `RecentAddressList`

### 4.1 The rules

* **Most recent first.** `remember` puts the address at the front.
* **Deduped by canonical form.** Remembering an address already in the list moves it to the front;
  `10.0.0.1` and `10.0.0.1:7777` are the same entry.
* **Capped at 5** (`kRecentAddressCapacity`); the oldest falls off.
* **"This PC" is never stored.** `remember` refuses `127.0.0.1:7777` (the exact canonical form of
  `kThisPcAddress`), because that entry is always shown anyway (§5). ⚠ Only that exact address:
  `127.0.0.1:17777` (the editor's PIE port) and `localhost:7777` are different text and are
  stored. Resolving names to decide "is this the local machine" is not the model's job.
* **Invalid text is refused.** Only what `parseServerAddress` accepts is stored, in canonical form.
* `remember` returns whether the list changed. Remembering the address that is already first
  returns `false`, so the UE layer can skip writing the ini.

### 4.2 One line of text

`serialize` writes the entries newest first, joined by `,` (`kRecentAddressSeparator`), for example
`10.0.0.3:1,host.example:9000,10.0.0.1:7777`. `deserialize` reads such a line back.

A `static_assert` requires the separator to be a character no address can contain and trimming
cannot remove, so one stored address can never split into two. ⭐ **Measured poison arm
(2026-09-29):** changing the separator to `.` fails the build with that message.

`deserialize` applies `remember`'s filter to each piece — trimmed, parsed, canonical, deduped,
"This PC" and invalid pieces dropped, at most five kept — so a hand-edited or damaged ini line
loads as the list `remember` would have built. It keeps the stored order (first piece = most
recent); it does not re-sort.

Where the line is stored (T10: a user-scope ini under `Saved/Config`) and when it is saved (after
a successful join, never from an editor-launched session) are the UE layer's decisions; §7.4 gives
it the switch.

---

## 5. "This PC" and the list the screen shows

`joinListEntries` returns the list the screen draws: **"This PC" first, always**, then the recent
addresses, newest first — at most `kJoinListMaxEntries` (6). "This PC" is `127.0.0.1:7777`
(`kThisPcAddress`, labelled `kThisPcLabel`) — the server a local playtest runs on this machine.
A recent entry's label is its address.

The entry exists so that a local playtest needs one press (§10).

---

## 6. The address on the command line — `commandLineMapOverrideToken`

The T10 spike's ruling for Shipping: the engine drops a command-line address in a Shipping client,
so the join screen reads `FCommandLine::Get` itself, takes the same token the engine would have
used, and joins it. In Development the engine uses the address and the join screen never loads;
in Shipping the screen appears briefly and shows *Connecting*.

`commandLineMapOverrideToken` mirrors `UGameInstance::GetMapOverrideName` (engine
Runtime/Engine/Private/GameInstance.cpp lines 736-761), which walks `FParse::Token` tokens
(Runtime/Core/Private/Misc/Parse.cpp line 947):

* whitespace separates tokens; a token that starts with `"` runs to the next `"`, which is
  dropped; an unquoted token keeps any quotes inside it and does not end at a space inside them;
* the first token that does not start with `-` is the result;
* a token that starts with `-map=` (any case) returns the rest of the token.

⚠ **One difference, on purpose.** The engine matches `-map=` with `FParse::Value`, which searches
anywhere in the token and stops the value at a separator. The model accepts only a token that
*starts* with `-map=` and takes the rest of it. A token such as `-foo-map=x` is therefore ignored
here. The difference cannot select a different server: whatever the token is, it still has to pass
`parseServerAddress`.

The token is a raw view; the screen parses it when it starts (§7.2).

---

## 7. The phases — `JoinScreenModel`

### 7.1 Four phases, one more than the backlog lists

The backlog names three screen states: `Editing`, `Connecting`, `Failed{reason}`. The model has a
fourth, **`Joined`**, which is never drawn (the screen is gone once the client is in the game). It
is needed for two things:

* **Knowing that a failure belongs to an attempt.** A connection lost while playing arrives after
  the attempt succeeded; without `Joined` the model could not tell it from a stray event (§7.3).
* **Saving the address on success.** The transition into `Joined` is where the address is
  remembered.

| from | event | to |
|---|---|---|
| `Editing`, `Failed` | `activate` / `requestJoin` with a valid address | `Connecting` |
| `Editing`, `Failed` | `activate` / `requestJoin` with an invalid address | unchanged, error shown |
| `Failed` | any successful edit | `Editing` |
| `Connecting` | `cancel` | `Editing` |
| `Connecting` | `noteJoinSucceeded` | `Joined` |
| `Connecting`, `Joined` | `noteJoinFailed(reason, text)` | `Failed` |
| `Editing`, `Failed` | `noteJoinFailed` | unchanged — ignored |

While `Connecting` or `Joined` the screen takes no input (`acceptsInput()` is false): typing,
paste, cursor moves, navigation and activation all return without changing anything.

### 7.2 Starting — `JoinScreenModel::start`

`start(recents, commandLineAddress, reachedViaFailureReturn)`:

* The field is pre-filled with the most recent address, if there is one.
* Focus is "This PC" when there is no history, and the field otherwise (§10).
* A non-empty command-line address that parses starts the model in `Connecting` to its canonical
  form, and the field shows it.
* ⛔ **When the front-end was reached by a failure return, the command-line address is ignored.**
  After a failed join the engine returns to the default map, and the command line still carries
  the address; joining it again would loop forever. The UE layer passes
  `reachedViaFailureReturn` (T10: the front-end URL carries `closed` after such a return).
* A command-line address that does not parse (a map path, URL options) leaves the screen in
  `Editing` with a notice in the status line's second row.

⚠ **The model does not know whether the engine already started the join.** In Development the
engine consumed the address itself; in Shipping nothing has travelled yet. Both start the model in
`Connecting`. Whether a travel call must still be made is the UE layer's decision (it knows the
build configuration); the model only records the attempt, so that its outcome is shown the same
way in both.

### 7.3 ⭐ The first failure of an attempt is the one shown

`noteJoinFailed` is accepted only in `Connecting` or `Joined` and moves to `Failed`. A second
failure, in `Failed`, is ignored and the first reason stays.

The engine reports one failure more than once. The T10 spike measured a version mismatch firing
`ENetworkFailure::OutdatedClient` and then `ENetworkFailure::PendingConnectionFailure`, with the
same text; the first names the cause, the second only the consequence. Keeping the first shows
"different build" rather than "can't reach server".

The same rule makes a cancelled attempt safe: after `cancel` the model is `Editing`, so the
engine's late failure for the abandoned connection is ignored rather than turning the screen red.
A failure while `Editing` for any other reason (an event nobody is waiting for) is ignored too.

### 7.4 Success, and the editor switch

`noteJoinSucceeded(rememberAddress)` moves `Connecting` to `Joined` and, when `rememberAddress` is
true, remembers the address; it returns whether the recent list changed. The UE layer passes
`false` for editor-launched sessions (`-PIEVIACONSOLE` or `GIsEditor`, backlog task 12), so a
PIE session never changes the list a player sees.

### 7.5 Refused input stays visible until the next edit

`activate` or `requestJoin` with an invalid field keeps the phase and records the error
(`rejectedFieldError`); the status line shows it in the error tone. The next successful edit
clears it. The error is not shown while the player is still typing — only after they tried to
join.

---

## 8. Failure reasons and their texts

Five reasons (`JoinFailureReason`), per the T10 spike and the lead's ruling on its flags 5 and 6.
The mapping from engine events is the UE layer's (task 13); this table records it so that the
texts below can be judged against it. Task 13 measured one correction to the spike's table: a refusal
during the login (`Server full.`) arrives as `ENetworkFailure::PendingConnectionFailure` carrying the
server's text, not as `ENetworkFailure::FailureReceived` (measured; recorded in section 13 of the
UE layer's JoinScreen rationale).

| reason | engine event (T10 Q3) | headline | detail |
|---|---|---|---|
| `CannotReachServer` | timeout or lost connection on the pending driver, unresolvable host | `Can't reach server <address>.` | what to check |
| `DifferentBuild` | `ENetworkFailure::OutdatedClient` / `ENetworkFailure::OutdatedServer` | `Different build.` | `This game is <own label>; the server runs a different build.` |
| `ConnectionLost` | `ENetworkFailure::ConnectionTimeout` on the game driver | `Connection lost.` | `The server stopped responding.` |
| `ServerRefused` | `ENetworkFailure::PendingConnectionFailure` (during the login), `ENetworkFailure::FailureReceived` (after it) | `Server refused: <server text>` | — |
| `Unknown` | anything else | `Could not join.` | the engine's text, or `Unknown error.` |

* ⛔ **"Different build" shows only the client's own label.** The client never learns the
  server's label (T10 flag 6); only a CRC of the server's version reaches it, and only in the log.
  An empty label shows as `dev` (`kDevBuildLabel`), the label a non-packaged run has.
* **"Server refused" shows the server's own text** ("Server full.", "Maximum splitscreen
  players"), because only the server knows why. With no text it says `no reason given`.

The headlines are all different (checked by walking `0 .. kJoinFailureReasonCount - 1`), so a
player can tell the five apart. A `static_assert` ties the count to the enum and keeps `Unknown`
last, so a reason appended after it cannot slip out of that walk.

`JoinStatusText` has two rows, a headline and a detail, because the panel is one line wide and a
failure needs both "what happened" and "what to do".

---

## 9. Local co-op — the hint and the limit notice

**The hint:** `After joining: Tab adds a local player, Insert removes one`. Per the backlog it
states no limit. It is built from `kLocalCoopKeyNames`, whose two strings are the engine names of
the keys `AOGBrawlerPlayerController::SetupInputComponent` binds (`OGBrawlerPlayerController.cpp`,
the two `BindKey` lines for `JoinLocalPlayer` and `LeaveLocalPlayer`). Since task 12 those two lines
build their keys from `kLocalCoopKeyNames` itself, so the hint and the binding share one source.
⛔ They are key names, not wording: guard G-01.

**The limit notice:** the engine lets one client create at most 4 local players
(`UGameViewportClient::MaxSplitscreenPlayers`, default 4, not overridden in this project's
config). The 5th Tab fails harmlessly on the client; task 13 shows `localPlayerLimitNoticeText(n)`
on the HUD for `kLocalPlayerLimitNoticeSeconds` (4 s), with `localPlayerLimitNoticeVisible`
deciding the window. The limit is a **parameter**, not a constant here: the UE layer passes the
viewport's live value, so the text stays right if the ini value is ever raised. This is not a
player cap in the sense of the initiative's NO HARD PLAYER CAP ruling — the engine refuses the
player; the notice only says so.

---

## 10. Gamepad focus

Three areas, in the order they are drawn from top to bottom: **the list** (each entry, "This PC"
first), **the field**, **the Join button**. `navigatedFocus` moves one step:

* Down: to the next list entry; from the last entry to the field; from the field to Join; Join
  stays.
* Up: to the previous list entry (the first stays); from the field to the **last** list entry
  (the one drawn just above it); from Join to the field.
* A list index past the end (the list shrank) is pulled back to the last entry first.

There is no wrap-around: a player holding the stick down ends on Join, which is where they want to
be.

**Activation** (Enter, or gamepad A — the UE layer maps both to `activate`):

* on a list entry: copies the entry's address into the field and joins it;
* on the field or the Join button: joins the field's text.

**Initial focus** is "This PC" when there is no history, so a local playtest is *start the game →
press Enter or A*. With history, focus is the field, which holds the most recent address, so
rejoining the last server is the same single press.

Any successful edit moves focus to the field: typing while an entry is focused starts typing an
address rather than being lost.

---

## 11. Layout — `placedJoinScreenLayout`

A centred panel, stacked top to bottom:

```
title · build label · list rows (1..6) · field · Join button · status headline · status detail · hint
```

The vertical order is the focus order (§10), so Up and Down move where the eye expects.

### 11.1 The scale — clamp, then resolution, then fit

1. **Clamp** the requested scale (the UE layer's console variable) to `[0.5, 4]`
   (`clampJoinScreenScale`). As in the scoreboard, a NaN is pulled to the minimum: the test
   `!(x >= min)` is true for NaN.
2. **Resolution.** Multiply by `canvasHeight / 720` (`kJoinScreenReferenceCanvasHeight`), so the
   screen is 1.5× at 1080p and 3× at 4K. ⚠ **This differs from the scoreboard**, which takes the
   console value as the whole factor. The scoreboard is an overlay a developer tunes; the join
   screen is the first thing every player sees, at whatever resolution they run, before any
   console is available, so it must be legible by default.
3. **Fit.** If the panel would not fit the canvas, the scale drops until it does. ⚠ **Fit wins
   over the minimum**: on a very small canvas the scale goes below 0.5. A panel that is small but
   whole is usable; one that is cut off is not.

A canvas with no size yet (zero, negative or NaN — the first frame) gives the clamped scale and an
origin of 0: every value is finite and nothing divides by zero.

### 11.2 Rows and text

* `listRowCount` is capped at `kJoinListMaxEntries`; the panel's height follows the number of rows
  actually drawn, so a first-time player sees one row, not six.
* The status area is always two rows (`statusHeadline`, `statusDetail`), so the panel does not
  jump when a failure appears.
* `scale` is also the text scale for the small font (`UEngine::GetSmallFont`); the title uses
  `titleTextScale` (1.4× that).
* `textInsetX` / `textOffsetY` are the scaled offsets of text inside its rectangle.

---

## 12. Inks

`JoinScreenInk` is three linear-0..1 floats, like the scoreboard's ink; a `static_assert` keeps it
at three. The panel's only translucency is its backdrop (`kJoinScreenBackdropAlpha`, 0.8), so the
game world behind the panel can dim it but never make the text or the error harder to read.
`joinStatusInk` maps the status tone: prompt → dim, progress → blue, error → red. `kJoinScreenFocusInk` is for
the focused row or control.

---

## 13. What the model deliberately does not do

* **No timeouts.** How long *Connecting* lasts is the engine's (T10: about 20 s to 60 s).
* **No engine types.** No `FString`, `FKey` or `ENetworkFailure`; the UE layer converts at the
  boundary.
* **No I/O.** The recent line is produced and consumed as text; reading and writing the ini is the
  UE layer's.
* **No key map.** Which keys mean Up, Down, activate and cancel is decided by the UE layer
  (T10 Q2: Backspace, Enter, Ctrl+V, the D-pad and face buttons).

---

## 14. Reading order

1. `JoinScreenModel` and §7 — the phases, which is where most behaviour is decided.
2. `parseServerAddress` and §3.
3. `RecentAddressList` and §4.
4. `placedJoinScreenLayout` and §11.
5. The test file, `BrawlerJoinScreenTest.cpp` in og-brawler-tests, whose case names read as a
   specification.

<!-- SPDX-License-Identifier: BUSL-1.1 -->
# `DAttackRadialSimulation.h` — rationale

<!-- lint-external-ref: collisionCheck -- RETIRED by og-netcode-v2-field-defects task 9: the radial's old in-integrate detector, replaced by brawlerHitDetection::detectRadialHits. It must NOT resolve -->
<!-- lint-external-ref: hasHitGuard -- RETIRED by og-netcode-v2-field-defects task 9: the radial State's wire flag, replaced by DerivedState::guardBlockedThisTick. It must NOT resolve -->
<!-- lint-external-ref: updateLinearAttachmentToOwner -- RETIRED by NP-6: replaced by the explicit attachment block in integrate. It must NOT resolve -->
<!-- lint-external-ref: getInitialRotation -- RETIRED by task 45: inlined at its call sites. It must NOT resolve -->
<!-- lint-external-ref: attackHits -- RETIRED by og-netcode-v2-field-defects task 27: the derived per-swing ledger, replaced by the synced State::hitTargets. It must NOT resolve -->
<!-- lint-external-ref: activeRootBodyId -- RETIRED by og-netcode-v2-field-defects task 27: the dead radial InitialConditions field (written 0, never read). It must NOT resolve -->
<!-- lint-external-ref: editAttackHits -- RETIRED by og-netcode-v2-field-defects task 27 with attackHits. It must NOT resolve -->
<!-- lint-external-ref: glm/common.hpp -- third-party glm header, shipped under og-simulation's glm/ directory, outside every doc_anchor_lint scan root -->

The law, the provenance and the history of the radial (weapon-swing) sub-simulation. The
**prohibitions** are in `DAttackRadialSimulation-guards.md`, and nothing here is a fence, except
§4.3, which is a prohibition that no tag can reach and is recorded here untagged under §9.3 of the
comment rule.

**Provenance.** Converted by og-netcode-v2-field-defects task 19 (2026-09-23). The header, as it
stood after task 9, was 672 lines, md5 `5fce5c9f…`, and carried 174 comment lines. That is 156
lines of prose in 28 runs, 16 section banners, the SPDX line and the anonymous-namespace closer.
The 28 runs are kept VERBATIM in [§A](#a-the-removed-comments-verbatim), each tagged
`<!-- pristine lines A-B -->` with its disposition. The banners carried no words and were deleted.
The sections below are the corrected reading. **Where a section and §A disagree, the section
wins.** §A is the shipped text, including the parts task 19 found false (§9).

⚠ **This file is not the source of truth for any VALUE.** Where it quotes a number, the code and
the tests win.

⚠ **Task 2 edits this header next** (closed-form pose, the whole idle state on each idle tick). It
writes under the convention: tags plus doc entries, no prose in the header.

---

## 1. Includes and the attack-sequence id domain

`InvalidAttackSequenceId`, `kHadoukenSequenceSentinel` and `isRealAttackSequence` live in
`OGBrawler/DAttackSequenceId.h` (task 36). They are a value domain owned by neither sub-sim: the
machine writes the ids, the radial consumes them and the visualizations gate on them.

`glm/common.hpp` came in for the task-32 `glm::abs` site. That site moved to
`BrawlerHitDetectionSystem.h` with detection (task 9), and nothing left in this header calls into
`glm/common.hpp`. The include is now vestigial for this header. It was kept, because removing an
include is a code change and this task was comments only. Removing it is safe only once nothing
depends on it transitively.

## 2. Physics declaration

**`PhysicsSetup`** holds the radial's body descriptor, `body` (a 30-unit sphere, `body` category,
physics on, gravity off), and `queryVolumes`, which builds two spheres (outer radius and twice the
outer radius, both `bodyAndGuard`, `queryRouting`) from `StaticData`. ⛔ The shipped comment called
the body descriptors "compile-time constants". **They are not.** `PhysicalObjectDescriptor` holds a
`std::vector<ShapeDescriptor>`, so `body` is a `static inline const` initialized at dynamic-init
time. A `static_assert` reading `PhysicsSetup::body.body.simulatePhysics` fails with **C2131**
(measured, task 19).

Detection moved out in task 9, so the radial itself no longer queries. The query volumes are
still created here because `brawlerHitDetection::System` overlaps them through the radial's
`bindings.queryVolumeIds`.

**`RuntimeBindings` is an alias for `PhysicsRuntimeBindings`, never a per-sim copy.** The
`PhysicsDeclaration` concept requires `{ d.bindings } -> std::same_as<PhysicsRuntimeBindings&>`, so
a field-identical copy is a distinct type and does not conform. ⭐ **Compiler-held:** replacing
the alias with a field-identical struct fails at `SimulatableBrawler.h`'s
`static_assert(::PhysicsDeclaration<dAttackRadialSimulation::PhysicsDeclaration, simulatableBrawler::StaticData>)`
with C2607, whose note reads *"'dAttackRadialSimulation::RuntimeBindings &' and
'PhysicsRuntimeBindings &' are different types"* (measured, task 19).

**`PhysicsDeclaration::staticDataOf` is a member TEMPLATE.** It maps the game's aggregate static
data to this sub-simulation's slice (`m_attackSimulationStaticData`), so the engine-side fold asks
each declaration for its slice instead of branching on the declaration type. This header cannot
name `simulatableBrawler::StaticData`, because `SimulatableBrawler.h` and
`SimulatableBrawlerTypes.h` include this header. ⭐ **Compiler-held:** a non-template overload
naming the aggregate fails with **C2653** *"'simulatableBrawler': is not a class or namespace
name"* (measured, task 19).

`/*shapes*/` in `PhysicsSetup::body` labels the positional second member of the
`PhysicalObjectDescriptor` aggregate. It is the rule's §6 annotation form, and it replaced a
trailing `// shapes`.

## 3. `DAttackHit`

`hitRootBodyId` is the struck character's actor-level id, `SpatialQueryHit::rootBodyId`. The body
and guard shapes of one character report the same root id. Routing resolves the target from it. It
is a per-process engine handle, so it is **not** the dedup key any more (task 27).

`targetId` (og-netcode-v2-field-defects task 27) is the struck character's peer-stable
`SimCharacterId`. The detector resolves it from `hitRootBodyId` through its own registration map, and
`integrate` records it in the synced ledger `State::hitTargets` (§4.4). It defaults to
`SimCharacterId::None`, which is also the ledger's empty entry, so a hit that names no character
cannot be recorded (an `OG_CHECK` in `recordHitTargets`). Guard hits leave it at `None`: they are
never recorded.

`swingTangent` (movement-sim task 83) is the direction the weapon was travelling through the hit
point. It is the swing plane's tangent at the hit radius, signed by the sequence's AUTHORED angular
velocity, and hit routing throws the target along it. It is unit length, or exactly `(0,0,0)` when
the hit projects onto the rotation axis. It is derived scratch: `DAttackHit` has no
`SerializableFields`, so it costs zero wire bytes.

⛔ **Corrected at task 19.** The shipped comment reasoned that routing "treats [(0,0,0)] as
degenerate and falls back to its away-from-attacker rule, **so** this must never be a NaN". Routing
is the only consumer, and its `normalisedXY` falls back whenever `lengthSq > 0.f` is false, which a
NaN also makes false. As routing stands, a NaN would fall back too. "Never NaN" is still the
contract, and it is enforced where the tangent is built: `BrawlerHitDetectionSystem.h`'s
`swingTangentAt`, under that file's guard G-06. It is not a consequence routing depends on, and no
edit to this header can break it.

## 4. `DerivedState` — the per-pass and per-tick hit signals, and the synced per-swing ledger

⛔ **Corrected at task 19.** The shipped comment called `DerivedState` "Mutable per-tick scratch
data only". **Half of it is per-SWING.** It is off-wire (derived) state holding two kinds of thing:

| member | lifetime | cleared by | filled by |
|---|---|---|---|
| `guardHits` | **one detection pass** (task 20; was the swing). Positions where the weapon met another character's guard. The visualization draws them as 15 cm blue (colorId 2) spheres. | the detector, first thing in each pass (`BrawlerHitDetectionSystem-guards.md` G-13); `deactivate()` no longer clears it | the detector, beside `guardBlockedThisTick` |
| `hitsThisTick` | **one tick**: the signal routing consumes | the detector (G-13 there) and the top of `integrate` (G-04) | the detector (`registerAttackHit`) |
| `guardBlockedThisTick` | **one tick** | the detector (G-13 there) and the top of `integrate` (G-05) | the detector, on a block |

⭐ **Task 27.** The table once had a fourth row, the per-SWING dedup ledger (a `std::vector<DAttackHit>`,
cleared only in `deactivate()`, filled by the detector). It was derived, so a correction neither
restored it nor adopted the authority's copy: a replay anchored mid-swing and an adopted phantom both
kept a stale ledger that suppressed real hits (the "frontier mid-swing" pin, and capture 3's three
missed stuns). It left `DerivedState` and is now `State::hitTargets` on the wire (§4.4).

In production the detector is `brawlerHitDetection::detectRadialHits`, called from
`brawlerHitDetection::System::preIntegrate` (task 20; `postIntegrate` before it), which fires after
every character's `integrate(T)` and physics step T, before `integrate(T+1)`. So each tick runs the
detector's clear, its fill, routing, and then the radial's own clear at the top of `integrate`.

⚠ **R0 (task 20).** `guardHits` was per-SWING only in name: a block ends the swing on the next tick,
so a swing holds at most one entry. It is per-pass now, because `deactivate` ran on the recoil tick
T+1 before the visualization snapshot of T+1 and would have erased the entry the pass had just
written for T.

### 4.1 Why reserve, and why a check rather than a comment (movement-sim task 34)

`DerivedState()` once read `: attackHits(4), guardHits(4)`, a member-init **resize** that
manufactured four default-constructed hits in each container (verified: og-brawler `1f81e90`). The
detector's cap check, `if (derivedState.editAttackHits().size() >= 4) return;`, was the first line
of the old `collisionCheck` (verified at `b572456`). A fresh `DerivedState` therefore arrived at the
cap already satisfied, and detection was a silent no-op. It was reachable, and not rarely:
`DerivedState` lives inside the character's `m_allState` (`SimulatableBrawler.h`), and
`SimulatableBrawler::integrate` runs the machine sub-sim before the radial in the same tick. An
attack held on a character's first tick makes the machine write a valid `activeAttackSequence`.
`integrate` then skips `deactivate`, the only site that clears these containers, and
`setInitialConditions` does not clear them either. The swing registered no hits for its whole
duration.

Reserving makes `size()` mean "hits recorded during the current swing" at every moment of the
object's life. ⭐ **Task 19 turned the fence into an `OG_CHECK`** at the end of the constructor:
all three containers must be empty. ⚠ **Task 27:** the derived ledger left the type, so the check
now covers `guardHits` and `hitsThisTick` (reserved to `kMaxHitTargetsPerSwing`, 3). Its message
now names what a phantom entry would break today: `integrate` records every `hitsThisTick` entry in
the synced ledger, so on any path that integrates before the detector's own clear, a phantom would
be routed and recorded. (A phantom has `targetId == None`, so `recordHitTargets`'s own check would
also fire.) It fires on BOTH realizations of the bad edit, a body
`resize` and a member-init `attackHits(4)`. A tag could not cover both, because the member-init is
typed on the constructor head and the reserve in its body (§9.1 clause C). Seen to fire
(standalone build, `assert` form) on both a body resize and a member-init `guardHits(4)`; the
control passes. It is also pinned by `DAttackRadial.FreshDerivedStateIsEmptyButReserved`.

⛔ **Two corrections to the shipped text** (§A, pristine lines 140-171):
* *"a genuine 'at most four distinct targets per swing' cap"* is false. The cap is tested ONCE,
  at the top of each detection pass. A pass that starts below four can register several hits, so
  `attackHits` can exceed four within one swing. It is "no new detection once four are recorded".
  The same overstatement stood in the `hitsThisTick` note ("the <= 4 distinct targets cap").
  ⭐ **Superseded by task 27 (user ruling R3):** the cap is now EXACTLY at most 3 per swing,
  including within one pass (§4.4).
* *"Pinned by DAttackRadialFirstTickCollisionTest.cpp — four cases walking the machine->radial
  tick-1 path end to end"* is false. Three cases walk the path: `FirstTickSwingRegistersHits`,
  `…ForwardSequence` and `FirstSwingKeepsRegisteringForItsWholeDuration`. The fourth,
  `FreshDerivedStateIsEmptyButReserved`, is stated directly on the type, as its own banner says.

### 4.2 The per-tick signals (movement-sim task 83, og-netcode-v2-field-defects task 9)

`hitsThisTick` exists because routing used to iterate `attackHits`, the per-swing ledger. One hit
therefore re-fired on every remaining tick of the swing. The knockback velocity was re-assigned
with no decay for about 0.4 s (13 m instead of 5 m in the user's PIE report), the lockout timer
restarted every tick, the stun re-entered every tick, and the direction was re-resolved from
positions that had moved, so the throw curved.

`guardBlockedThisTick` (task 9) replaced `hasHitGuard`, a `State` member that rode the wire.
`brawlerHitRouting::System` branch 5 copies it onto the attacker's inbound slice
(`brawlerInboundHit::DerivedState::wasGuardBlockedThisTick`), and the machine recoils into
`GuardFlinch` on the next tick. A derived signal is recomputed on every replayed tick, so it never
needs restoring. The limit on that is in task 9's notes: a replay anchored AFTER the detection tick
does not recompute it. ⭐ **Closed by task 20:** the pass now runs in `preIntegrate` of the consuming
tick, so a replay anchored at the end of the detection tick re-runs it on its first step.

**Where the clear sits.** It is the first thing `integrate` does after reading its dependencies. It
runs ahead of the Hadouken return, both `deactivate` exits and the idle branch, because every one of
those is a path out of the function, and a tick that leaves by any of them registers no hits (the
detector's gate reads the `currenSequenceId` `integrate` left, which each of them leaves Invalid). So
the tick must publish none. Until task 20 this was the only production clear site; the detector now
clears the same two signals first thing in each pass (its G-13). Test rigs also clear by hand
(`BrawlerHitRoutingTest.cpp`). The tags are G-04 and G-05.

### 4.3 ⛔ The ledger is NOT cleared in `integrate` or `setInitialConditions` (untagged)

Rewritten by task 27 around the synced ledger (it named the derived one). Adding a clear of
`State::hitTargets` beside the two per-tick clears would destroy the per-swing dedup, and the same
target would be registered on every Damaging tick. `deactivate()` stays its only clear site
(`state.hitTargets = {}`). `setInitialConditions` does not clear it either: a chained swing enters
through `deactivate` first, so it starts empty, which is what
`DAttackRadial.ChainedSwingCanHitTheSameTargetAgain` pins. **No tag carries this.** The edit adds a
new statement, and a tag covers only the statement it precedes (§9.1 clause B). There is no
existing line to tag, so §9.3 files it here, untagged. The end-to-end pins are
`HitRouting.RadialHitFiresOnceAcrossTheSwing` (the ledger holds one entry for the rest of the
swing) and `HitDedup.ASwingHitsAtMostThreeDistinctTargets`.

### 4.4 The synced per-swing hit ledger `State::hitTargets` (og-netcode-v2-field-defects task 27)

**What it is.** `std::array<SimCharacterId, kMaxHitTargetsPerSwing>`, 3 entries of 1 B, 0-filled
(`SimCharacterId::None`) = empty. It is the swing's answer to "have I already hit this
character?", and what the cap counts. It is **Synced**: it rides the correction wire with the rest
of the radial `State`, so a replay restores it and an adoption brings the authority's copy. The
entries are peer-stable ids (task 25), never a `BodyId` or any other per-process handle (guard
G-06). A `static_assert` beside the type pins its size to 3 × 1 B, because a wider element or a
different count is a wire and gameplay change.

**Who writes it, and when.**
* **Reader:** the detector, at `preIntegrate(t+1)`, off the `State` that `integrate(t)` left
  (Synced(t)). It is the same read point as the sequence pair (`BrawlerHitDetectionSystem-guards.md`
  G-15, G-16).
* **Writer:** only the radial's own `integrate`. Its first statement, `recordHitTargets`, appends
  each `hitsThisTick[].targetId` into the first free entry. That statement comes BEFORE the G-04
  clear, which would otherwise empty the list it reads (guard G-07; user ruling R5). Systems never
  write wire state. `recordHitTargets` `OG_CHECK`s three things: the id is not `None`, it is not
  already recorded, and a free entry exists. The detector guarantees all three, and the third is
  what makes the cap structural. With the detector's in-pass cap removed, the four-targets-in-one-pass
  case aborts on it (measured, task 27).
* **Clear:** `deactivate()` only (§4.3).

**The timing, and why it is unchanged.** Until task 27 the detector appended in `preIntegrate(t)`
and read in `preIntegrate(t+1)`. Now `integrate(t)` appends, and `preIntegrate(t+1)` reads. The read
is the same, and every read is now of synced state. A hit detected by the reduction over tick T is
therefore in the ledger from the end of T+1. That is why a replay anchored at the end of T holds
an EMPTY ledger and re-detects on the replayed T+1
(`HitDetection.Behaviour.ReplayAnchoredAtTheEndOfTheBodyHitTickFlinchesTheTarget`, section
"frontier mid-swing": `Idle` before task 27, `HitFlinch` after).

**The cap is exact (user ruling R3).** A swing registers at most 3 distinct targets, including
within one detection pass: the detector counts the ledger plus this pass's registrations. Until
task 27 it was "no new pass once four are recorded", so a pass that started at three could register
several. Once the cap is reached, the rest of the pass is not evaluated, including a guard block
from a later actor. That matches the top-of-pass return, which skips a capped swing's block test
too.

**A guard block** puts nothing in `hitsThisTick`, so nothing reaches the ledger. The swing also
ends: routing recoils the attacker into `GuardFlinch` on the next tick and the radial deactivates
(`HitDedup.GuardBlockEndsTheSwingAndTheTargetIsNotHitLaterInIt`). So "a blocked target stays
hittable later in the swing" is not a rule of this game, and nothing here relies on it.

**NoSlot (user ruling 2026-09-26: accepted).** A character the step does not integrate (no resolved
input on a replayed tick) keeps its frozen `State`, and its ledger with it. If it is frozen
mid-swing and overlapping a target, the detector re-detects that target on every such pass, and
routing delivers it again, because only `integrate` records it. The derived ledger used to
suppress that. The case needs a mid-swing character with no replay slot, i.e. a correction anchored
before a just-registered character's first slot. The user accepted it rather than reintroduce a
derived value read across a tick boundary. Pinned as measured: one registration per skipped pass,
never two (`HitDetection.Behaviour.TheDetectorOwnsThePerTickSignalsItWrites`, second section).

**Adoption.** A correction replaces the whole `State`. After a phantom, the authority's swing has
ended in `deactivate`, so the adopted ledger is empty and the next swing registers
(`HitDedup.AdoptingAnIdleAttackerMidSwingLetsTheNextSwingHit`). Before task 27 the derived ledger
survived the adoption and the swing whiffed: capture 3's three missed stuns.

**Identity across peers.** `HitDedup.SyncedBytesMatchAcrossPeersWhoseEngineHandlesDiffer` compares
two rigs whose engine handles differ byte for byte. Planting the per-process root `BodyId` in
`hitTargets[0]` made it RED: 19 mismatch ticks, the first on the hit tick at byte 80, which is
`hitTargets[0]` (radial IC 20 B + `attackTimer` 4 + `currenSequenceId` 4 + `bodyState` 52).

## 5. Inputs, state, dependencies and serialization

* **`PlayerInputView`** is the radial's input since og-syncedInput-rework task 3, and it is empty: the
  radial's `integrate` reads no input field. `from()` returns `{}` for any
  `simulatableBrawler::SyncedPlayerInput`, and the view exists only because every sub-simulation
  names a `Dependencies::InputType` of its own.
* ⚠ **The two bullets below are history.** og-syncedInput-rework task 4 (2026-10-03) deleted the
  radial `PlayerInput` and its `zero()`, and retired guards G-01 and G-02 with them (guards §R).
* **`PlayerInput`** was a plain aggregate (task 43). C++20 parenthesised aggregate init keeps
  `PlayerInput(aim, left, right)` construct sites valid. The radial's `integrate` never reads its
  own input slice. The slice exists so the composite carries it and packaging can fill it.
* **`PlayerInput::zero()`** is the neutral input, folded into the composite by
  `SimulationComposite::zero()`, which is all `getZeroPlayerInput()` now is (movement-sim task 22).
  The fences are guards G-01 and G-02. **"Plain static, not constexpr"** is a caution, not a
  prohibition. glm's `GLM_CONSTEXPR` is empty when `GLM_ARCH` has the SIMD bit or when clang lacks
  `cxx_relaxed_constexpr`, so its constexpr-ness really is build-flag dependent in principle. But
  on all three flag sets this tree compiles with today (standalone `cl`, the `OGBrawlerTests` LLT
  and the `OGBrawler` editor module), `static constexpr PlayerInput zero()` plus a `static_assert`
  on its aim **compiles** (measured, task 19; the Server and Client targets were not probed). It
  was left non-constexpr: nothing needs it, and the one guarantee a `static_assert` would add is
  already test-held (G-01).
* **`IntegrationUtils`** carries only the physics adapter (task 9). The radial poses its own weapon
  body and nothing else. Detection, the only thing here that ever queried, moved to
  `brawlerHitDetection::System`, which holds its own adapters.
* **`InitialConditions`** is a plain aggregate (task 45). The `m_` prefix was dropped and
  `getInitialRotation()` was inlined at its call sites. Task 27 retired `activeRootBodyId` (4 B):
  the machine wrote it as 0 and nothing read it. It was the composite's first slice, so the
  removal moved every later offset (`kWireFormatVersion` 5 -> 6). The slice is 20 B.
* **`State`** is a plain aggregate (task 44). `hasHitGuard` stood between `currenSequenceId` and
  `bodyState` and rode the wire (1 B). Removing it (task 9) moved every later offset, so
  `correctionStateBuffer::kWireFormatVersion` went 3 -> 4. Task 27 appended `hitTargets` (3 B,
  §4.4): 60 -> 63 B. The composite went 326 -> 325 B (−4 + 3), which was measured by the pins in
  `SimulatableBrawlerTest.cpp`.
* **`Dependencies`** uses the `OwnedDeps`/`ExternalDeps` layout (task 62).
* **`network()`** has no callers (grep, task 19). `State` is a plain aggregate, so no getter/setter
  registration is needed (task 44).
* The `SerializableFields` specializations (task 39) define the wire layout of the three types.

## 6. Fences that are CHECKS, not comments

| fence | now | seen to fail (task 19) |
|---|---|---|
| `StaticData` is non-copyable and non-movable. It holds references into its owner's sibling members (`m_attackSequences`, `m_attackCircle`), so a copy rebinds them to the source's members and dangles. The former hand-written copy ctor did exactly that (og-brawler `0076ff4`). | **`static_assert`** on all four special members, message naming the comment it replaced | re-`default`ing the copy ctor, and separately the move ctor → **C2338** with the message, in standalone, LLT and editor-module flag sets. Before task 19 the same edit **compiled**: nothing copies a `StaticData`, so the `= delete` alone held nothing once removed. |
| `DerivedState` reserves, never resizes | **`OG_CHECK`** at the end of the ctor | §4.1 |
| `integrate`'s Hadouken-sentinel return stays AHEAD of the `setInitialConditions` branch, or `setInitialConditions` indexes `attackSequences[kHadoukenSequenceSentinel]` out of bounds | **`OG_CHECK`** (`isRealAttackSequence` and in range) at the top of `setInitialConditions`. This is an ordering fence, and a tag cannot hold one (§9.1 clause E). Deleting the return trips the same check on the first Hadouken tick. | `setInitialConditions` called with the sentinel → abort with the message; called with sequence 0 → passes. `DAttackRadialAimLogTest.cpp`'s hadouken row also drives this branch through `integrate`. |
| `RuntimeBindings` stays an alias | already compiler-held | §2 (C2607) |
| `staticDataOf` stays a member template | already compiler-held | §2 (C2653) |

## 7. `wouldGuardBlock` — the shared block predicate

A pure function with no side effects. It is shared by the detector
(`brawlerHitDetection::detectRadialHits`) and the attacker-side block-prediction visualization
(`dAttackBlockPredictionVisualization`). It returns true if and only if a swing of
`activeAttackSequence`, from a pivot at `attackerRoot`, would be blocked by a guard body whose world
transform is `guardTransform` (its forward is column 0) and whose query overlap position is
`guardOverlapPosition` (`SpatialQueryHit::objectPosition` of the guard-category hit). The overlap
point is flattened to the pivot's height. `td` is the angle between the guard's forward and the
direction from the guard to the attacker. Inside `shieldAngle = 0.25f` only sequence 4 is blocked.
Between that and `outerShieldAngle = pi/2`, the sign of the cross product's z picks sequences
{0, 2} or {1, 3}. The positive outer gate is guard G-03.

⛔ **Corrected at task 19** (§A, pristine lines 361-374 and 387):
* **`attackerRoot` is the attacker's WEAPON-AXIS body position**, not "the attacker's own body
  position". The radial's `bindings.ownBodyId` is the `WeaponAxis` body, which `integrate` places at
  the parent (capsule) position plus `attachmentOffset` (0, 0, 30). In XY it coincides with the
  character, and the predicate flattens z.
* **The two callers read it differently.** The shipped text wrote
  "`State::bodyState.position / physics.getBodyTransform(bindings.ownBodyId)[3]`" as if they were
  one value. The detector passes the live transform, read after `integrate`'s attachment write. The
  visualization passes `radialState.bodyState.position`, the last captured solved pose. They are
  equal only when nothing moved the weapon since the capture.
* **"Extracting it makes sim/viz drift structurally impossible" is overstated.** Only the
  predicate cannot drift. The visualization also passes `predictedSequenceIdFromIdle`, not the
  active sequence, applies its own behind-attacker filter, and mirrors the detector's surrounding
  code by hand, and all of that can drift.
* **"Constants and the outer gate mirror the inline check this predicate was extracted from
  exactly"** describes an extraction whose source no longer exists. There is no inline check left
  to mirror. The constants are gameplay values. ⚠ `shieldAngle` (0.25 rad) is mirrored by the
  projectile's `guardMiddleSectionHalfAngle` argument in `SimulatableBrawlerTypes.h`, and nothing
  checks that the two agree.
* **The `0.5` double literal's "implicit narrowing, preserved verbatim" is value-neutral.**
  Multiplying by 0.5 only changes the exponent, so the product is exact in both float and double.
  `static_assert(float(glm::pi<float>() * 0.5) == glm::pi<float>() * 0.5f)` and
  `== glm::half_pi<float>()` both compile (task 19). The trailing "matches sim" comment is moot: the
  predicate IS what the sim calls.
* The predicate still lives in this header because the visualization includes it, and it stays
  here after detection moved (see `BrawlerHitDetectionSystem-rationale.md` §4).

## 8. `integrate`

**The ledger append** (task 27) is `integrate`'s first statement, ahead of the G-04/G-05 clears
(§4.4, guard G-07).

**The attachment block** (NP-6) replaced `updateLinearAttachmentToOwner()`. It re-parents the
weapon to the parent's position plus `bindings.attachmentOffset` every tick. It writes translation
only, and the rotation it reads back is the weapon's own.

**The Hadouken sentinel** (`kHadoukenSequenceSentinel`). The machine owns this "attack" through the
projectile sub-sim. The radial resets `currenSequenceId` to Invalid and returns. Returning is what
keeps `setInitialConditions` from indexing the table with the sentinel, now a check (§6).
⛔ **Corrected at task 19** (§A, pristine lines 562-566):
* *"so the machine's Attacking->Idle exit fires naturally next tick"* is false. The machine's exit
  gate does read the radial's Invalid `currenSequenceId`, but `integrate3` holds it off with
  `inHadoukenCommitment` until `kHadoukenCommitmentSeconds` (0.3 s, task 25) has passed. The exit
  fires then, not on the next tick.
* *"Keep the weapon idle"* overstates what the branch does. It writes no pose, and the attachment
  block moves translation only. The weapon keeps whatever pose it had. That is the idle pose
  because the machine triggers the Hadouken from its `Idle` case only, and by then the radial has
  been idle. For the same reason, the reset of `currenSequenceId` finds it already Invalid on every
  path the machine was read to take. It is defensive. This was not proven for adoption or replay
  states.

The `[Radial.*]` `OGBLOG_G` format strings are code, not comments. They are grep-sensitive and are
pinned by `DAttackRadialAimLogTest.cpp`.

## 9. R0 — every false claim task 19 found, and what is true

| # | where (pristine) | the claim | what is true |
|---|---|---|---|
| 1 | 86-87 | body descriptors are "compile-time constants" | dynamic-init `static inline const`, C2131 (§2) |
| 2 | 134 | `DerivedState` is "per-tick scratch data only" | two members are per-swing ledgers (§4) |
| 3 | 145-146, 191 | "at most four distinct targets per swing" / "the <= 4 distinct targets cap" | the cap is tested once per pass, so a pass can overshoot four (§4.1). Since task 27 it is exactly ≤ 3 (§4.4) |
| 4 | 169-170 | "four cases walking the machine->radial tick-1 path" | three walk it, and one is on the type (§4.1) |
| 5 | 205 | the detector "returns early PER ATTACKER exactly as this header's detector did" | ⚠ task-9 wording: the detector has two early returns `collisionCheck` never had (guard G-04) |
| 6 | 203-207, 539-542 | "a clear there would leave the last Damaging tick's entries live" | true only after the detector's first return (guard G-04) |
| 7 | 78-79 | tangent "must never be a NaN" *because* routing falls back on zero | routing falls back on NaN too, and the contract is enforced in the detector (§3) |
| 8 | 366 | `attackerRoot` is "the attacker's own body position" | the weapon-axis body (§7) |
| 9 | 367 | `State::bodyState.position` / `getBodyTransform(...)[3]` as one value | two callers, two different reads (§7) |
| 10 | 364-365 | sim/viz drift "structurally impossible" | only the predicate cannot drift (§7) |
| 11 | 370-371 | constants "mirror the inline check this predicate was extracted from exactly" | no inline check remains (§7) |
| 12 | 372-373, 387 | the narrowing is "preserved verbatim", "matches sim" | value-neutral: ×0.5 is exact (§7) |
| 13 | 564-565 | Attacking->Idle exit "fires naturally next tick" | after the 0.3 s Hadouken commitment (§8) |
| 14 | 254-255 | `zero()` cannot be constexpr, because glm is "build-flag dependent" | true of glm in principle; constexpr compiles on every flag set probed (§5) |
| 15 | 563 | the branch keeps "the weapon idle" | it writes no pose, and the weapon is already idle (§8) |
| 16 | 251 | the neutral aim `(0,0,1)` is "forwards" | it is +Z (`defaultUp`); `defaultForward()` is `(1,0,0)` (guard G-01) |
| 17 | 251-252 | "a value-initialised (0,0,0) aim would reach normalize()" | every reader projects to XY first, where both are the zero vector; the guard substitutes `defaultForward`. Only the test-tag half of the fence is true (guard G-01) |

Inbound citations elsewhere that this conversion made stale, or found already stale, are listed in
the task-19 implementation notes. `DAttackRadialSimulation.h:450` and `:680` were fixed where they
stood. `DAttackMachineSimulation.h`'s own copy was handed to task 18.

---

## §A The removed comments, VERBATIM

Each run is the shipped bytes of the header at md5 `5fce5c9f…`, with the section where its content
now lives. ⚠ Several are false: read the section, not the archive (§9).

<!-- pristine lines 9-9 --> → §1
```
#include "glm/common.hpp"	// the task-32 glm::abs site moved to BrawlerHitDetectionSystem.h (task 9)
```

<!-- pristine lines 32-34 --> → §1
```
// [Task 36] InvalidAttackSequenceId, kHadoukenSequenceSentinel, and isRealAttackSequence
// were relocated to the minimal shared header OGBrawler/DAttackSequenceId.h (included above).
// They form an attack-sequence-ID value domain owned by neither sub-sim — see that header.
```

<!-- pristine lines 50-53 --> → §6 — now a `static_assert`
```
	// Holds references into sibling members of the owning simulatableBrawler::StaticData
	// (attackSequences / attackCircle). Copying/moving would rebind those references to
	// the source object's members, dangling once the source is destroyed. The former
	// hand-written copy ctor did exactly that silently — now compiler-enforced non-copyable.
```

<!-- pristine lines 74-74 --> → §3
```
	BodyId hitRootBodyId;   // actor-level id of the struck character (SpatialQueryHit::rootBodyId)
```

<!-- pristine lines 75-80 --> → §3 (corrected)
```
	// [movement-sim task 83] The direction the WEAPON was travelling through this hit point:
	// the swing plane's tangent at the hit radius, signed by the sequence's AUTHORED angular
	// velocity. Hit routing throws the target along it. Unit length, or exactly (0,0,0) when
	// the hit projects onto the rotation axis and no tangent exists -- routing treats that as
	// degenerate and falls back to its away-from-attacker rule, so this must never be a NaN.
	// DERIVED SCRATCH: DAttackHit is not serialized, so this field costs ZERO wire bytes.
```

<!-- pristine lines 86-87 --> → §2 (FALSE: not compile-time constants)
```
// All physics setup descriptors for the radial simulation.
// Body descriptors are compile-time constants; query volumes depend on StaticData.
```

<!-- pristine lines 95-95 --> → kept in source as `/*shapes*/` (§2)
```
		{   // shapes
```

<!-- pristine lines 125-129 --> → §2 — already compiler-held
```
// The one shared definition lives in OGSimulation/PhysicsDeclaration.h. The
// PhysicsDeclaration concept requires `same_as<PhysicsRuntimeBindings&>`, so a
// field-identical per-sim copy is a DISTINCT type and does not conform; this
// alias keeps every existing `dAttackRadialSimulation::RuntimeBindings`
// spelling valid while making the type the shared one.
```

<!-- pristine lines 134-134 --> → §4 (FALSE: half of it is per-swing)
```
// Mutable per-tick scratch data only.
```

<!-- pristine lines 140-171 --> → §4.1 — now an `OG_CHECK` (two corrections)
```
		// ⛔ [movement-sim task 34] RESERVE, NOT RESIZE — AND THE DIFFERENCE WAS A LIVE BUG.
		// This was `: attackHits(4), guardHits(4)`, a member-init RESIZE that manufactured FOUR
		// default-constructed DAttackHit entries in each container. The detector's cap check
		// (brawlerHitDetection::detectRadialHits, BrawlerHitDetectionSystem.h; it was the first
		// line of this header's own detector until task 9 of og-netcode-v2-field-defects moved
		// detection into a system) is `if (derivedState.editAttackHits().size() >= 4) return;` —
		// a genuine "at most four distinct targets per swing" cap, since the container
		// ACCUMULATES across the whole swing (deduped by rootBodyId) and is cleared only in
		// deactivate(). So a freshly constructed DerivedState arrived at that cap ALREADY
		// SATISFIED and detection became a silent no-op.
		//
		// It was reachable, and not rarely: DerivedState is a long-lived per-character member
		// (SimulatableBrawler.h), and SimulatableBrawler::integrate runs the machine sub-sim
		// BEFORE this one in the same tick. An attack held on a character's very first tick has
		// the machine write a VALID activeAttackSequence, which makes integrate() skip the
		// deactivate branch — the ONLY site that clears these containers. setInitialConditions
		// does not clear them either. The swing then registered NO hits for its entire duration,
		// with no assert and no log, and the four phantoms additionally reached the manager's
		// hit routing (BrawlerHitRoutingSystem) and drew four guard-hit spheres at the origin.
		// ⚠ Rollback/resim makes it MORE likely, not less: a resim replays from early states.
		//
		// ⭐ THE POINT OF RESERVING INSTEAD: size() now means exactly what the guard reads it
		// as — "hits recorded during the CURRENT swing" — at every moment of the object's
		// lifetime, including before the first deactivate. The coupling cannot fail silently
		// because the failing STATE is no longer representable. An assert could not have done
		// this job: DAttackHit carries no provenance, so four phantoms and four legitimately
		// capped hits are indistinguishable at the guard.
		//
		// The 4 here is only a capacity hint (the guard's cap is what makes 4 the useful
		// number). Pinned by DAttackRadialFirstTickCollisionTest.cpp — four cases
		// walking the machine->radial tick-1 path end to end — and by SimulatableBrawlerTest's
		// "the slice ctor really ran" case, which now anchors on capacity() rather than size().
```

<!-- pristine lines 187-208 --> → §4.2 and guard G-04 (corrected)
```
	// [movement-sim task 83] THE PER-TICK HIT SIGNAL, and it is a DIFFERENT THING from
	// attackHits above -- conflating the two is the defect this member exists to close.
	// attackHits is the per-SWING DEDUP LEDGER: it accumulates for the whole swing and is
	// cleared only in deactivate(), which is exactly what makes "have I already hit this
	// character?" answerable and what the <= 4 distinct targets cap counts. Hit routing used
	// to iterate it every post-integrate, so ONE hit re-fired on EVERY remaining tick of the
	// swing: the knockback velocity was re-assigned with no decay for ~0.4 s (13 m instead of
	// 5 m, the user's PIE report), the lockout timer restarted every tick, a stun re-entered
	// every tick, and the direction was re-resolved from positions that had MOVED, so the
	// throw curved.
	// This container holds only the hits registered on the CURRENT tick. It is cleared at the
	// TOP of integrate(), unconditionally and before every early return. It is FILLED after
	// integrate, by brawlerHitDetection::System's post-integrate pass
	// (brawlerHitDetection::detectRadialHits, BrawlerHitDetectionSystem.h) -- task 9 of
	// og-netcode-v2-field-defects moved detection out of this header, so the clear always
	// precedes the fill within a tick.
	// ⛔ Do NOT move that clear into the detector: detectRadialHits early-returns when no swing
	// is active, when the swing is not Damaging, at the four-target cap and on an empty
	// overlap -- it returns early PER ATTACKER exactly as this header's detector did -- so a clear there
	// would leave the last Damaging tick's entries live for the rest of the swing -- the same
	// bug in a smaller window.
	// DERIVED SCRATCH, like attackHits: ZERO wire bytes.
```

<!-- pristine lines 212-219 --> → §4.2 and guard G-05
```
	// [og-netcode-v2-field-defects task 9] THE PER-TICK GUARD-BLOCK SIGNAL. Set by the
	// detector on the tick this character's swing was blocked by another character's guard;
	// brawlerHitRouting::System copies it onto this character's inbound slice
	// (brawlerInboundHit::DerivedState::wasGuardBlockedThisTick) and the machine recoils into
	// GuardFlinch on the next tick. It REPLACES State::hasHitGuard, which rode the wire: a
	// derived signal is recomputed on every replayed tick, so it never needs restoring.
	// Cleared beside hitsThisTick at the top of integrate(), for the same reason.
	// DERIVED SCRATCH: ZERO wire bytes.
```

<!-- pristine lines 223-226 --> → §4
```
	// Positions where the weapon intersected another character's guard during this
	// attack. Recorded by the detector alongside guardBlockedThisTick and cleared at the
	// same point as attackHits (deactivate()). Visualization renders these as blue
	// spheres in dAttackRadialVisualization.
```

<!-- pristine lines 242-242 --> → §5
```
	// [Task 43] Plain aggregate — C++20 parenthesis aggregate init keeps construct-site calls valid.
```

<!-- pristine lines 247-255 --> → guards G-01/G-02 and §5
```
	// THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
	// SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
	// [movement-sim task 22] The value is copied VERBATIM from what that function
	// handed this type before the fold; it is a wire value, not something to re-derive.
	// ⛔ (0,0,1) forwards, NOT PlayerInput{}: a value-initialised (0,0,0) aim would
	// reach normalize(), and the difference is also the TAG the input-resolution and
	// net-sync anti-vacuity tests discriminate on. Keep zero() != PlayerInput{}.
	// Plain static, not constexpr: glm's vec3 constructor constexpr-ness is build-flag
	// dependent in this tree.
```

<!-- pristine lines 260-262 --> → §5
```
// [og-netcode-v2-field-defects task 9] No query adapter: the radial poses its own weapon and
// nothing else. Hit detection -- the only thing here that ever queried -- moved to
// brawlerHitDetection::System (BrawlerHitDetectionSystem.h), which holds its own adapters.
```

<!-- pristine lines 289-289 --> → §5
```
	// [Task 45] Plain aggregate — m_ prefix dropped, getInitialRotation() inlined at call sites.
```

<!-- pristine lines 301-301 --> → §5
```
	// [Task 44] Plain aggregate.
```

<!-- pristine lines 304-306 --> → §5
```
	// [og-netcode-v2-field-defects task 9] `hasHitGuard` stood here and rode the wire (1 B);
	// it is now DerivedState::guardBlockedThisTick, routed through the inbound slice. Removing
	// it moved every later offset, so correctionStateBuffer::kWireFormatVersion went 3 -> 4.
```

<!-- pristine lines 317-323 --> → §2 — already compiler-held
```
	// Maps the GAME's aggregate static data to this sub-simulation's own slice.
	// This is what makes body creation generic: the engine-side fold asks each
	// declaration for its slice instead of branching on the declaration type.
	// A member TEMPLATE deliberately — this header cannot name
	// simulatableBrawler::StaticData, because the aggregate includes this header
	// (an include cycle). GameStaticDataType is deduced at the call site, where the aggregate is
	// complete.
```

<!-- pristine lines 344-344 --> → §5
```
// [Task 62] Dependencies — OwnedDeps/ExternalDeps layout.
```

<!-- pristine lines 361-374 --> → §7 (four corrections) and guard G-03
```
// Pure block predicate shared by the hit detector (brawlerHitDetection::detectRadialHits,
// BrawlerHitDetectionSystem.h -- this header's own detector until og-netcode-v2-field-defects
// task 9) and the attacker-side block-prediction visualization
// (dAttackBlockPredictionVisualization). Extracting it makes sim/viz drift structurally
// impossible. No side effects. Returns true iff a swing
// of `activeAttackSequence` from `attackerRoot` (the attacker's own body position,
// State::bodyState.position / physics.getBodyTransform(bindings.ownBodyId)[3]) would be
// blocked by a guard body whose current world transform is `guardTransform` and whose
// overlap position in the query is `guardOverlapPosition` (SpatialQueryHit::objectPosition
// of the guard-category hit). Constants and the outer gate mirror the inline check this
// predicate was extracted from exactly:
// shieldAngle = 0.25f, outerShieldAngle = pi/2 (0.5 double literal → implicit narrowing,
// preserved verbatim), and the POSITIVE `td < outerShieldAngle` gate (NOT a negated
// early-return) preserves the sim's "NaN falls through to attack-lands" semantics.
```

<!-- pristine lines 387-387 --> → §7 (value-neutral)
```
	const float outerShieldAngle = glm::pi<float>() * 0.5;   // implicit narrowing — matches sim
```

<!-- pristine lines 530-545 --> → guards G-04/G-05, §4.2, §4.3
```
	// [movement-sim task 83] THE PER-TICK HIT SIGNAL IS CLEARED HERE, FIRST AND
	// UNCONDITIONALLY -- ahead of the Hadouken sentinel return, the deactivate branch and the
	// idle branch. Every one of those is a path out of this function, and a tick that leaves
	// by any of them registers no hits, so it must publish none. This is the ONLY clear site,
	// which is what makes "hitsThisTick is what happened on THIS tick" true for every tick
	// rather than for the Damaging ones only.
	// [og-netcode-v2-field-defects task 9] The hits are now registered AFTER this function
	// returns, by brawlerHitDetection::System's post-integrate pass
	// (brawlerHitDetection::detectRadialHits), so this clear precedes that fill on every tick.
	// ⛔ Not in the detector: detectRadialHits early-returns per attacker when no swing is
	// active, when the swing is not Damaging, at the four-target cap and on an empty overlap,
	// so a clear there would leave the last Damaging tick's entries live and hit routing
	// would keep re-firing them. guardBlockedThisTick is the same kind of signal and is
	// cleared here for the same reason.
	// attackHits is deliberately NOT cleared here -- it is the per-SWING dedup ledger and
	// deactivate() stays its only clear site.
```

<!-- pristine lines 549-549 --> → §8
```
	// [NP-6] Explicit attachment math — replaces updateLinearAttachmentToOwner()
```

<!-- pristine lines 562-566 --> → §8 and §6 — now an `OG_CHECK` (two corrections)
```
	// Hadouken sentinel: the machine sim owns this "attack" via the projectile sub-sim.
	// Keep the weapon idle (the attachment math above already re-parents it this tick) and
	// reset currenSequenceId so the machine's Attacking->Idle exit fires naturally next
	// tick. Returning here also avoids the setInitialConditions path indexing
	// attackSequences[kHadoukenSequenceSentinel] out of bounds.
```

<!-- pristine lines 618-619 --> → §5
```
	// [Task 44] State is now a plain aggregate; no getter/setter-based network registration needed.
	// network() has no callers currently.
```

<!-- pristine lines 624-624 --> → §5
```
// [Task 39] SerializableFields specializations for dAttackRadialSimulation types.
```

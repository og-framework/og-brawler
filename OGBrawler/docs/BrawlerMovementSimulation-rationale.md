<!-- SPDX-License-Identifier: BUSL-1.1 -->
<!-- lint-external-ref: SimulationManagerUImpl.cpp -- RENAMED by og-ue-simulationcomponentandmanagersplit task 30 to OGBrawlerSimulationManager.cpp; kept in §7's verbatim carried comment -->
<!-- ------------------------------------------------------------------ -->
<!-- LINT ESCAPES for `tools/lint/doc_anchor_lint.ps1`.                  -->
<!--                                                                     -->
<!-- Every name below is one this document deliberately mentions and     -->
<!-- which deliberately does not resolve: an engine symbol outside the   -->
<!-- scan root, a RETIRED token, a name that does not exist YET and      -->
<!-- whose fence exists for that reason, a spike-probe identifier, or a  -->
<!-- private initiative document. Each is declared WITH ITS REASON,      -->
<!-- because an escape nobody reads is a way to silence a real hit -- and-->
<!-- an escape that matches NOTHING is itself a lint violation.          -->
<!-- ------------------------------------------------------------------ -->
<!-- lint-external-ref: impl/impl_notes_seam_27.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: DetachGateIsVacuousUntilJumpLands -- a RETIRED token -- task 27 DELETED this Catch2 case and replaced it with XYKnockbackDoesNotDetach; the prose exists in order to say the name is gone, so a resolving name would falsify it -->
<!-- lint-external-ref: paramName -- not an identifier at all: it is the PLACEHOLDER inside the `/*paramName=*/` call-site annotation convention (CommentExtractionRule v2 section 1.4), and the real names are the constructor parameters it stands in for -->
<!-- lint-external-ref: ASimulationManagerUImpl::m_staticData -- moved member inside carried text: the static data is UESimulationSession's m_staticData since og-ue-simulationcomponentandmanagersplit task 24a, still a by-value member constructed with the actor, before BeginPlay installs the ogblog sink, so the carried ordering reason still holds -->
<!-- lint-external-ref: dWorld -- an identifier from a REJECTED alternative spelling, never in shipped code -- section 26 names `glm::vec2(dot(dWorld, u), dot(dWorld, v))` as the wrong edit the derivation tag exists to stop; a resolving name would mean the wrong edit had landed -->
<!-- lint-external-ref: UCharacterMovementComponent -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: UCharacterMovementComponent::ComputeFloorDist -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: UCapsuleComponent::InitCapsuleSize -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: UpdatedComponent -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: AddMovementInput -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: bAutoActivate -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: CollisionShape.h -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: CapsuleComponent.h -- UE 5.6 engine symbol -- C:/dev/UnrealEngine is outside every scan root by initiative rule, so this can never resolve here -->
<!-- lint-external-ref: SurfaceKind -- a RETIRED token -- this prose exists in order to say the name is gone (task 56 replaced SurfaceKind{Airborne, Floor} with SupportState) -->
<!-- lint-external-ref: Airborne -- a RETIRED token -- this prose exists in order to say the name is gone (task 56 replaced SurfaceKind{Airborne, Floor} with SupportState) -->
<!-- lint-external-ref: dAttackMachineSimulation::PlayerInput -- a RETIRED token -- og-syncedInput-rework task 4 deleted the machine's input slice; the name survives only inside the verbatim header quote of lines 1270-1283 (section 23, the movement tick), which R0-20 corrects -->
<!-- lint-external-ref: PlayerInput::flags -- a RETIRED token -- og-syncedInput-rework task 4 deleted brawlerMovementSimulation's input slice and its flags member; the name survives only inside the verbatim header quote of lines 804-808 (section 13, the input flags byte), which R0-23 corrects: the byte is now simulatableBrawler::SyncedPlayerInput's flags -->
<!-- lint-external-ref: MachineInputT -- a RETIRED template parameter -- task 62 deleted it, and this prose records why the indirection existed and why it went -->
<!-- lint-external-ref: Jumping -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: kFlagJumping -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: Dashing -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence -->
<!-- lint-external-ref: commandIssued -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: capX -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: penDepth -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: velocityN -- an identifier from an archived spike probe or a poison arm, never in shipped code -- the superseded surface-normal actuation ruling #26 removed -->
<!-- lint-external-ref: pushOutTicks -- an identifier from an archived spike probe or a poison arm, never in shipped code -->
<!-- lint-external-ref: worldPushOut -- a name for a QUANTITY in an argument, not a declaration in code -->
<!-- lint-external-ref: dashDir -- an UNBUILT name -- the fence that quotes it exists precisely BECAUSE it does not exist yet (tasks 21 / 27 / 31); a resolving name would falsify the fence (task 31 supplies it) -->
<!-- lint-external-ref: kInputFlag -- a PREFIX, not an identifier -- the sentence is about the naming convention that keeps the input flags byte distinct from State::flags -->
<!-- lint-external-ref: Done -- a Backlog STATUS word, not a symbol -->
<!-- lint-external-ref: Backlog.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl_notes_seam_54.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl/design_hover_slope_transient.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: impl/research_spike_9.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: research_spike_9.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: design_ledge_fall_and_landing.md -- brawler-movement-simulation initiative archive; private working material, not distributed with this submodule -->
<!-- lint-external-ref: BrawlerInputPackaging.h:153 -- a DEAD citation, quoted DELIBERATELY: note R0-13 exists in order to record that task 65's published correction cites a line past the end of a file since rewritten 197 -> 128 lines. The lint is RIGHT and so is the note -->
<!-- lint-external-ref: ZeroInputIsTheFold -- a RETIRED token -- og-syncedInput-rework task 3 DELETED this Catch2 case with the six-slice input composite it tested; the header quote that names it is verbatim and R0-21 records the successor, so a resolving name would falsify both -->
<!-- lint-external-ref: detachesFromSupport -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: attackGroundNormal -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: estimateSupportV2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history (its body is estimateSupport's) -->
<!-- lint-external-ref: classifySupportV2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history (renamed classifySupport) -->
<!-- lint-external-ref: writesKnockbackVelocityV2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history (renamed writesKnockbackVelocity) -->
<!-- lint-external-ref: supportLaw -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: SupportLaw::V2 -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: SupportLaw::Legacy -- a RETIRED token -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records its history -->
<!-- lint-external-ref: BrawlerMovement.XYKnockbackDoesNotDetach -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: BrawlerMovement.SlopeKnockbackStaysOnSlope -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.InPlaneTangentSlidesAlongTheFace -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (its v2 twin ...UnderV2 remains) -->
<!-- lint-external-ref: GroundAlignedKnockback.DirectionLeavingTheFaceLaunchesOnTheHitTick -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (its v2 twin ...UnderV2 remains) -->
<!-- lint-external-ref: GroundAlignedKnockback.FlatTargetHitByASlopeTangentLaunches -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (its v2 twin ...UnderV2 remains) -->
<!-- lint-external-ref: GroundAlignedKnockback.DirectionIntoTheFaceStaysSupported -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.OverheadDownswingNeverDetaches -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (its v2 twin ...UnderV2 remains) -->
<!-- lint-external-ref: GroundAlignedKnockback.FlatTargetWithAHorizontalDirectionKeepsTodaysBits -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.UnwalkableExactFaceOverAWalkableSweepIsWorldUp -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.WalkableExactFaceStillDecides -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: GroundAlignedKnockback.V2EqualsLegacyOnEveryPlaneWithoutAnExactEdge -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: BrawlerGroundSample.AlignedSampleIsTheSharedGroundNormal -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured -->
<!-- lint-external-ref: BrawlerGroundSample.LegacyDeclaresExactlyOneQueryVolume -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (restated as DeclaresTheGroundProbeAndTheCentreProbe) -->
<!-- lint-external-ref: SupportEstimate.LegacyRegistersOneQueryVolumeAndV2Two -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (restated as RegistersTheGroundProbeAndTheCentreProbe) -->
<!-- lint-external-ref: PlatformEdge.KnockbacksAcrossTheImportedPlatformRims -- a RETIRED Catch2 case -- og-ground-aligned-attacks task 37 deleted it with the Legacy support law; the prose records what it measured (the Legacy run; the v2 run ...UnderV2 remains) -->
# `BrawlerMovementSimulation.h` — rationale

The narrative, the provenance and the **derivations** for the brawler character-movement
sub-simulation. The header carries the code and nothing else; the fences live in
`BrawlerMovementSimulation-guards.md`; everything below is the *why*.

**If this file and `BrawlerMovementSimulation.h` disagree, the header is authoritative and this
file is stale.** Fix this file; do not soften the header to match it.

⛔ **Do not move a guard into this file.** Every `⛔G-nn` tag in the header points at
`BrawlerMovementSimulation-guards.md`, and that separation is a hard gate under
`tools/lint/guard_tag_lint.ps1`. A prohibition filed as narrative is read after the edit, not
before it.

⛔ **This file is not the source of truth for any VALUE.** The constants, the gains, the flag
bits and the wire layout are in the header. The authored tunables are in
`SimulatableBrawlerTypes.h` and, for the cvar path, in the UE layer's `MovementSchemeCVar.cpp`.
Where a value appears below it is there to make an argument readable.

---

## How to read this file

⭐⭐ **Every quoted paragraph below is the header's own text, carried across unchanged.** The
only transformation applied is the removal of the `// ` comment prefix. Nothing is paraphrased,
re-worded or summarised, and each block carries an HTML comment giving the line range it
occupied in the pre-conversion header, so any paragraph can be diffed against version control.

⛔ **The reason is not tidiness.** This header's comments are not justifications — they are
**derivations a reader needs while reading the code**: a Jury-criterion stability recurrence, a
2×2 dual-basis solve, a capsule-probe geometry with a `sec θ` error term. Retyping a derivation
is how a derivation acquires a defect. Extraction is a **move**.

⚠ **R0 corrections are NOT edited into the carried text.** Every false or stale claim found by
the three audits behind this conversion — and three found by this conversion itself — is an
editorial note **immediately before** the paragraph it corrects, marked `R0-nn`. A false sentence
can therefore never be read here without its correction attached, and nothing has been laundered
by being moved. ⛔ Where the correction belongs to another task or needs a user ruling, the note
says so and the text is left alone.

⚠ **Units and frame of the carried text (og-sim-metres-yup, 2026-10-26).** Every carried paragraph was
written while the simulation ran in UE centimetres with Z up (x forward, y right). Since og-sim-metres-yup
tasks 17 and 19 it runs in **metres with Y up** (forward −Z, right +X, up +Y), and task 22 wrote every
authored constant in the header as a metre literal (`rideHeight` `0.1f`, `kProbeShrink` `0.01f`, gravity
`-9.8f`, …). So read a carried length, speed or acceleration as centimetres: divide it by 100 for the
value in today's code (`500 cm` is the `5.f` metres, `980` is `9.8`, `30 000 cm/s²` is `300`). Angles,
ticks, ratios and the servo's gains (`k`, `c`, ω, ζ) are unit-free and unchanged. Read a carried axis
through the frame's names: X is forward (`simFrame::kForward`), Y is right (`kRight`), Z is up (`kUp`),
"XY" is the ground plane and `.z` is `simFrame::height`. The editorial notes below that compare the
shipped frame with the Y-up frame were written before the flips; they are updated to say "the Z-up frame".

**Origin:** `brawler-movement-simulation` tasks 1, 5, 8, 11, 13, 14, 15, 16, 17, 19, 22, 27, 31,
39, 43, 46, 47, 49, 50, 51, 54, 56, 57, 62, 64, 65, 68, 75, 76 and 81; user rulings #5, #13,
#14(c), #16(a), #17(b), #26(a), #27(A), #28 and #29.

> ⚠ **The initiative names in that Origin line are private working material from initiative
> archives and are NOT distributed with this submodule.** They are named as provenance,
> deliberately unlinked; every claim this file *asserts* is anchored to a file in this repository
> and only to those. ⚠ A small number of the carried paragraphs quote such a document by path
> (`impl/…`). Those quotations are **verbatim** and were not edited, because the text they are
> part of is a fence or a derivation that must move unchanged.

---

---

## 1. The include graph — what this header pulls in, and why

The three include-site paragraphs, in file order. The acyclicity fence that stood at
lines 43-46 is **not** here: it is guard **G-18**, retired and deleted, because the
forbidden include is typed in `DAttackMachineSimulation.h`, and the compiler holds it there
(`DAttackMachineSimulation-rationale.md` §1).


<!-- header lines 25-33 -->
> [movement-sim task 62] `simulatableBrawler::CharacterBindings` is DEFINED IN THE LEAF HEADER
> INCLUDED ON THE NEXT LINE, not in this file any more. It is the one type the machine sub-sim
> wants from here, and keeping it in a header whose only dependency is `BodyId` is what lets the
> machine header have it without dragging in this one. Consumers of THIS header still see the
> type, transitively, so no call site changed.
> ⚠ [movement-sim task 64] THE NAMESPACE IS `simulatableBrawler`, NOT this file's
> `brawlerMovementSimulation`. It read the other way until task 64. THIS SUB-SIM NEVER USES THE
> TYPE — every mention of it left in this file is a comment; movement's capsule id comes from
> its own `RuntimeBindings` below.

> ### R0-01 — ROUTED — not this task's to fix
>
> ⚠ The paragraph below is the one the deleted acyclicity fence (guard **G-18**) sat
> under. `BrawlerCharacterBindings-guards.md:230` quotes lines 43-44 of the pre-conversion
> file verbatim; those lines no longer exist. **Routed to the lead, not fixed** — that
> file belongs to another task.


<!-- header lines 35-42 -->
> ⭐ [movement-sim task 62] THE MACHINE HEADER, INCLUDED OUTRIGHT — the dependency points ONE
> WAY now: movement -> machine. Movement reads the machine's `State` (the flinch freeze, step 3)
> and its `PlayerInput` (the move stick is packed onto the machine slice), so
> `machineFreezesMovement` and `integrate` SPELL those types instead of deducing them.
> Before task 62 this include was impossible: `DAttackMachineSimulation.h` included THIS header
> for `CharacterBindings`, so including it back was a cycle — and `#pragma once` does not turn a
> cycle into an error, it silently leaves one side incomplete depending on which header the
> translation unit entered from, which is the worst available failure mode.

⚠ **Measured by og-netcode-v2-field-defects task 18 (2026-09-23):** re-creating that cycle is not silent. Re-adding the include to `DAttackMachineSimulation.h` fails with `C2653` in every translation unit that enters from the machine header, `SimulatableBrawler.h` included, and compiles only when this header is entered first. See guard G-18.


<!-- header lines 47-55 -->
>
> ⚠ THE INPUT EDGE IS STILL UNDECLARED, and this include does not fix that. `Dependencies`
> below declares `External<const dAttackMachineSimulation::State&>`, so `findFirstViolation`
> validates the STATE edge against `ExecutionOrder`. The `PlayerInput` edge is declared NOWHERE,
> and `InputType = brawlerMovementSimulation::PlayerInput` positively asserts this sub-sim reads
> only its own input slice — untrue since step 3 began reading `machineInput.moveDirectionWorld`.
> What this include buys is that the edge is visible in the signature and CHECKED BY THE
> COMPILER. Teaching the dependency graph about INPUT edges means changing
> `OGSimulation/SimulationDependencies.h`, which every sub-sim shares: a separate framework task.

> ### R0-19 — FALSE since og-syncedInput-rework task 3 (2026-10-03): the input edge is declared
>
> ⛔ *"THE INPUT EDGE IS STILL UNDECLARED"*, *"The `PlayerInput` edge is declared NOWHERE"*, and
> the task-62 line above that movement reads the machine's `PlayerInput`.
>
> **Now.** The wire carries one flat `simulatableBrawler::SyncedPlayerInput`, and this sub-sim
> declares what it reads from it in its own header: `brawlerMovementSimulation::PlayerInputView`
> holds `flags` and `moveDirectionWorld`, filled by `PlayerInputView::from(const SyncedPlayerInput&)`.
> `Dependencies::InputType` and the `AllInput` alias name that view, so `InputType` now says
> truthfully what step 1 and step 3 read, and `integrate` no longer takes the machine's
> `PlayerInput` (that type was deleted by task 4) — step 3's `stickWorld` reads
> `input.getPlayerInput().moveDirectionWorld`. `SimulatableBrawlerTypes.h` asserts the view
> contract (`simulatableBrawler::BrawlerInputView`) over every `ExecutionOrder` entry.
>
> **What did NOT change.** The machine include stays, for the STATE edge
> (`External<const dAttackMachineSimulation::State&>`), which is what `findFirstViolation`
> checks. `OGSimulation/SimulationDependencies.h` still knows nothing about input edges: the
> view declares the edge in the type the sub-sim owns, not in the dependency graph, so no
> framework task was needed. The paragraph above stays as the record of the shape it described.

⭐ **[og-ground-aligned-attacks task 4] `OGBrawler/BrawlerGroundFrame.h`, the ground-sample leaf.** It
supplies `brawlerGround::GroundSample`, the return type of `sampleAttackGround` (§29) (until task 14 it
also held the initiative's compile-time knob, that function's default template argument). It is a leaf (glm and
og-simulation only) because the machine header needs the same type and cannot include this header back:
this header includes `DAttackMachineSimulation.h`, so the shared type lives below both.

⭐ **[og-ground-aligned-attacks tasks 13 and 37] `OGSimulation/SurfaceFrame.h` is no longer included.** Task 13
included it directly for `surfaceFrame::isFlat`, which the Legacy lift test (`detachesFromSupport`), the flat rule
and step 2's detached-frame choice read (§30). Task 37 deleted all three with the Legacy support law (§37), so the
header names nothing from it. The ground-sample leaf above still includes it.

---

## 2. Orientation — what this sub-simulation is and who owns what

The header opened with a 72-line orientation block (lines 62-133 of the pre-conversion
file). ⭐ It is **split, not moved whole**, exactly as this task's acceptance criterion
requires: THREE of its lines were must-never-move rows and went to the guards doc — **G-19**
(T3-2), **G-12** (T3-3) and **G-20** (T3-4), all three retired; the rest is below, in five
parts.

> ### R0-02 — the include count is deliberately not restated
>
> ⛔ The deleted fence said the graph was *"Re-verified at task 62 for all seven of its own
> includes"*. Measured 2026-09-11, `DAttackMachineSimulation.h` has **nine** og-brawler
> includes — eight sub-simulation headers plus `OGBrawlerLog.h`, which both the header and
> task 65's own published correction missed — and **thirteen** non-`glm`, non-`std`
> includes in total. The fence quantified over *every* include, not the og-brawler subset.
> ⭐ **No new number is written here.** A bare count in prose is a claim with a maintenance
> cost and no enforcement, and this one drifted twice in two consecutive tasks. The property
> that matters is that nothing reachable from that header reaches this one, and it is
> re-verified by walking the includes, not by counting them.


<!-- header lines 62-72 -->
> Home of the brawler character-movement sub-sim.
>
> [movement-sim task 11, 2026-09-06] THE REAL SUB-SIM. The skeleton (task 1) is gone:
> this sub-simulation now owns the character CAPSULE, decides what it is standing on,
> runs a movement model in a surface-relative 2D frame, hovers at a fixed clearance and
> writes both halves of the body's motion every tick.
>
> ============================ WHO OWNS WHAT (user ruling #14(c)) ============================
> The sim owns `{position, velocity}` and RE-PLACES the body every tick
> (`setBodyTransform` + `setBodyLinearVelocity`). The solver is used for SEPARATION ONLY,
> and its contribution is read back as a POSITIONAL push-out. The captured


<!-- header lines 74-104 -->
```text

  position   sim, engine-corrected  we write it; the post-solve capture returns
                                    `position + worldPushOut`; the sim adopts that.
  velocity   sim, exclusively       model output, plus the ONE vertical law (gravity
                                    always on, a spring-damper hover servo while
                                    `Supported` — ruling #28), plus ONE authored contact
                                    rule (kill the into-contact component, §step 6' below).
  ground     sim                    hover servo on the `sweep` probe — the capsule does
                                    not rest in contact, it floats `rideHeight` above.
  walls,     Chaos, POSITION ONLY   the capsule blocks `world` and other `character`
  brawlers                          capsules; the solver's push-out is the one thing
                                    read back. Two equal-mass capsules separate 50/50.
  rotation   locked                 `lockRotation` on the descriptor is what makes
                                    `LinearBodyState`'s fabricated identity rotation TRUE.

Why this shape and not "let Chaos integrate forces" (revision 5, retired): a refused or
unapplied Chaos rewind cannot lose a correction when the body is re-placed from `State`
on every replay tick. Task 9's q1(iii) measured that a sim-side correction converges
identically whether Chaos grants the rewind or refuses ~90% of them.

============================ ADDING A MOVEMENT MODEL ============================
A movement model is a PURE 2D FUNCTION of the surface frame. It never learns which plane
it is in: on flat ground the frame is world XY, on a slope it is the slope, on a wall
(task 20) it will be the wall. To add one:

  1. Add an enumerator to `MovementModel` (append; the value is authored data).
  2. Write one free function `computeDesiredVelocityUV_<Model>(sd, state, tick, stickUV,
     currentUV, dt)` beside the two below, and add its `case` to the `switch (sd.model)`
     in step 3. That switch is the ONLY place `sd.model` may be named — steps 2, 4 and 5
     are model-agnostic by construction and an acceptance criterion greps for it.
  3. If the model needs memory, APPEND fields to `State` and APPEND their `SIM_MEMBER`s.
```

> ### R0-03 — FALSE — an arithmetic elision, and the routing that came with it is CANCELLED
>
> ⛔ The paragraph below quotes the **margin** cost (10.264 B) and then the quotient
> produced by the **net closure** cost (10.764 B), dropping the one sentence that
> reconciles them. A reader who divides the two numbers it gives them gets 2.665 and
> concludes one of them is wrong. `RoundVsPacketBudgetTest.cpp:466-473` carries the missing
> term: the half-entry floor itself rises 0.5 B per stride byte, so net closure is
> 10.764 B per input byte and 27.352 / 10.764 = 2.5411 ✓.
>
> ⛔ **The defect is HEADER-ONLY.** Task 65 concluded the root cause was in
> `RoundVsPacketBudgetTest.cpp` and routed it there. That routing is **cancelled**: the test
> file is internally consistent and says so in the line the audit skipped. Re-verified
> 2026-09-11.
>
> ⭐ The safe form of the same figures is 15 lines further down this header, at
> §13: *"ate 10.264 B of the ordinary-join margin — a quarter of everything that was left.
> ~2.5 bytes remain."* It rounds and never states the quotient, so it never commits to the
> inconsistency. **The same author elided the same term twice and only the precise site went
> false.** Do not "fix" the other one.
>
> ⛔ **AND THE SECOND HALF OF THIS BLOCK IS FALSE TOO, AND IS TASK 58'S.**
> *"bits 1-7 are reserved for those four"* is one of **three** sites in this header (the
> others are in §13) and **eleven across six files** making the same claim. The corrected
> form is already shipped, by name, in `BrawlerInputPackaging-rationale.md`: **bits 1-7 are
> UNASSIGNED.** Task 48 adds *no* input field; tasks 20, 21 and 31 each say *"if this task
> adds an input field"*. ⚠ And **task 20 is Wall run**, not "wall-grab" — this header
> spells it correctly once (§4) and wrongly twice. **Owner: task 58. Carried verbatim,
> flagged, not rewritten.**


<!-- header lines 108-133 -->
```text
  4. Add an LLT file / cases under `[BrawlerMovement]`.

⛔⛔ STANDING RULE ON PLAYER INPUT (lead, 2026-09-06) — A NEW PER-TICK INPUT FIELD IS A
BIT IN A FLAGS BYTE, NEVER A NEW `bool` MEMBER. An input byte is about TEN TIMES more
expensive than a state byte, because it is multiplied across every ring entry in the
packet-budget scenario: `holdGuard`, one `bool`, moved the ring entry stride 81 -> 82 B and
cost 10.264 B of the join-alone margin at the character cap, leaving **27.352 B of slack
above the half-entry floor — 2.54 more input bytes before the half-entry floor guard in
`RoundVsPacketBudgetTest.cpp`'s "the pre-diet cap is 4" case goes RED.** One bool spent a
quarter of the remaining budget. Jump (task 21), dash (31), wall-grab (20) and ski-tuck
(48) are all coming; as four `bool`s they blow that fence twice over, as four bits in one
byte they cost nothing.
⭐ [movement-sim task 51, 2026-09-06] THAT SHAPE IS NOW THE SHIPPED ONE: `PlayerInput` is
`uint8_t flags`, `holdGuard` is bit 0, and bits 1-7 are reserved for those four. It moved
ZERO wire bytes — `bool` and `uint8_t` are both 1 B — so every pin in
`RoundVsPacketBudgetTest.cpp` and `SimulatableBrawlerTest.cpp` was re-quoted UNCHANGED.
That is precisely why it was worth landing BEFORE task 14, the field's only writer: a
re-layout that moves no bytes is verifiable in isolation, and 14 is then written against
the final shape instead of pinning a `bool` it would have to un-pin. The packing rule and
the constant that carries it live AT THE TYPE, on `PlayerInput` below. The full
arithmetic, with every term, is in `RoundVsPacketBudgetTest.cpp`'s pre-diet table block.

Constraints on everything in this header: `glm` only (no `<cmath>`, no engine types), and
NO PER-TICK TRANSCENDENTAL — the single `cos` is in `StaticData`'s constructor, which runs
once per session. `OGBLOG_G` fires only on surface-kind CHANGES, Cadence commits and
teleports; never unconditionally per tick.
```

> ### R0-22 — the numbers, the bit budget and "AT THE TYPE" in the quote above are history since og-syncedInput-rework tasks 3 and 4 (2026-10-03)
>
> ⛔ *"cost 10.264 B of the join-alone margin at the character cap, leaving 27.352 B of slack
> … 2.54 more input bytes before … "the pre-diet cap is 4" case goes RED"*, *"bits 1-7 are
> reserved for those four"* and *"the constant that carries it live AT THE TYPE, on
> `PlayerInput` below"*.
>
> **Now.**
> * **The numbers** were measured on the six-slice composite at the 82 B ring entry stride and
>   the cap of 4. Since task 3 the input wire is the flat 39 B
>   `simulatableBrawler::SyncedPlayerInput` and the stride is 44 B. The packet-budget table
>   derives a join-alone bound of 11; at that bound the margin is 32.728 B against a half-entry
>   floor of 22 B (10.728 B of slack), and one input byte costs 18.188 B of margin, so ONE more
>   input byte turns that floor red. The case that pins the bound is
>   "PacketBudget: the join-alone bound is 11 characters, and N = 12 crosses it"; the runtime
>   cap `kPreDietCharacterCap` is still 4. The rule the quote states (a new on/off input is a
>   bit, never a new member) is unchanged and is restated in `BrawlerSyncedPlayerInput-rationale.md`
>   §6 and §7.
> * **The bit budget:** bit 0 is holdGuard and **bits 1-7 are UNASSIGNED** (R0-03, R0-09). Task 4
>   corrected every present-tense site that said otherwise, in this repository's code, tests and
>   docs; this quote stays verbatim as history.
> * **"AT THE TYPE":** task 4 deleted this header's `PlayerInput`. The flags byte is
>   `simulatableBrawler::SyncedPlayerInput::flags`, the bit constant `kInputFlagHoldGuard` is still
>   declared in this header (guard G-05 stands on it), and this sub-sim reads the byte through its
>   own `brawlerMovementSimulation::PlayerInputView`.

---

## 3. `MovementModel` — which law turns the stick into a 2D velocity


<!-- header lines 139-141 -->
> Which movement law turns the stick into a surface-relative 2D velocity. Read once per
> session from `StaticData` so every peer runs the same one; it is NOT per-tick state and
> never travels on the wire.


<!-- header lines 144-145 -->
> CMC-like: accelerate toward `stick * maxWalkSpeed` at `acceleration`, brake toward
> zero at `brakingDeceleration`. Continuous, no committed direction.


<!-- header lines 147-148 -->
> Direction is COMMITTED every `stepPeriodTicks` and held at a constant `stepSpeed`
> in between — the "steps, not a joystick" feel.

---

## 4. `SupportState` — whether the character is being held up this tick


<!-- header lines 152-166 -->
> ⭐⭐ WHETHER THE CHARACTER IS BEING HELD UP THIS TICK — movement-sim TASK 56 / user ruling
> #28, 2026-09-07. It replaces `SurfaceKind {Airborne, Floor}`, and it is NOT a rename of the
> same idea. `SurfaceKind` named a GEOMETRIC FACT (a walkable probe hit inside the band);
> this names a DECISION about support. They differ exactly where the old name lied: during a
> jump's ascent (task 21) the probe still finds walkable floor underneath, and the servo must
> nevertheless be OFF.
>
> ⛔ SO THE DETACH GATE LIVES IN STEP 2'S CLASSIFICATION, NOT IN STEP 4.
> `detachesFromSupport` below is the one predicate that can turn a walkable probe hit into
> `Unsupported`; step 4 only READS the answer. Putting the gate in step 4 would re-create the
> value-that-is-not-true this rename exists to remove, and would give the vertical law a second
> place to disagree with the flags byte a correction restores.
>
> Stored in `State::flags` bits 1-2 — the SAME TWO BITS `SurfaceKind` used, so this moves ZERO
> wire bytes and the enum may still grow to 4 values before the field has to widen.

⭐ **[og-ground-aligned-attacks task 37]** The predicate the quote names, `detachesFromSupport`, went with the
Legacy support law. The detach decision is the rows of `classifySupport` (§34), still taken in step 2's
classification; step 4 still only reads `decision.support`.


<!-- header lines 169-170 -->
> Gravity ONLY. Ballistic flight, a jump's ascent (task 21), clearance beyond the probe
> band, or a hit whose normal failed the walkable test. The servo term does not run.


<!-- header lines 172-172 -->
> The servo term runs, along `up` (user ruling #26 a).


<!-- header lines 174-178 -->
> RESERVED for the 45°-75° band (task 48), where the servo axis follows `n` per the
> tasks-20/48 carry-forward. ⛔ v1 NEVER PRODUCES IT: step 2's walkable test is a single
> `cosMaxSlope` comparison, so anything steeper than `maxSlopeAngleDeg` classifies
> `Unsupported`. It is declared now so the wire field's value range is settled once,
> rather than widening the flags byte later.


<!-- header lines 182-187 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row — it pins a VALUE claim the comments
> above make and nothing checked.
> ⚠ AND IT IS NARROW, DELIBERATELY SO: it pins the three enumerators that EXIST, each of which
> rides State::flags bits 1-2 and is therefore a wire value. It does NOT catch a FIFTH
> enumerator — nothing expressible here does, short of a count constant this enum does not have
> — so the sentence above keeps that half.

---

## 5. `CharacterBindings` left this header


<!-- header lines 197-204 -->
> ⭐ [movement-sim task 62] `CharacterBindings` MOVED OUT of this header, to
> `OGBrawler/BrawlerCharacterBindings.h` — included at the top, so every consumer of this
> header still sees it and not one call site changed; its full provenance comment moved with
> it. It left because the machine sub-sim needs the type and must NOT include this header.
> ⭐ [movement-sim task 64] AND THEN THE NAMESPACE FOLLOWED THE TYPE: it is
> `simulatableBrawler::CharacterBindings` now. `brawlerMovementSimulation` was collateral from
> T35's file move, not a modelling call — this sub-sim never uses the type, and every real
> consumer sits in or under `simulatableBrawler`, where the struct started.

---

## 6. The sim's fixed step, and the discrete stability bound

⭐⭐ **This is the file's longest derivation and the reason task 75 argued this file may not
earn a v2 conversion.** The Jury-criterion recurrence below sat directly above the check it
justifies. It is now here, and the header carries `⛔G-01` in its place.
⚠ Whether that trade works for a reader standing at
`OG_CHECK(hoverGainsAreStable(…))` is the open question this conversion was run to answer.


<!-- header lines 208-215 -->
> ⭐⭐ THE SIM'S FIXED STEP, AND IT IS NOT AUTHORED HERE. `Config/DefaultEngine.ini`'s
> `AsyncFixedTimeStepSize=0.016667` is the authority; `integrate` is handed the real `dt` and
> uses THAT, never this. This constant exists for exactly one purpose: the discrete stability
> bound on the hover gains has to be checked where the gains are AUTHORED (`StaticData`'s
> constructor, once per session), and `dt` is not in scope there.
> ⚠ If the sim clock ever moves off 60 Hz this constant must move with it, or the check below
> silently guards the wrong region. That is why it is named, commented, and pinned by an LLT
> (`HoverGainsAreStableAtTheSimStep`) against the rig's own `kDt` rather than left a literal.


<!-- header lines 220-223 -->
> ⭐ [movement-sim task 76] THE DISCRETE STABILITY BOUND, AS A PREDICATE — so the derivation
> written out above `StaticData`'s constructor can be CHECKED BY THE COMPILER and not only read.
> The constructor's `OG_CHECK` calls this same function on the AUTHORED pair, so the runtime
> check and the two compile-time witnesses below cannot drift apart: there is one body.


<!-- header lines 230-232 -->
> ⛔ THE OBVIOUS BOUND IS THE WRONG ONE, AND THIS IS THE BUILD BREAK THAT SAYS SO. Rewrite the
> body above as the explicit-Euler `omega * dt < 2` — the reflex edit, and the one three people
> derived first — and BOTH witnesses below start passing it, so this assertion fires.


<!-- header lines 239-240 -->
> ⚠ AND THE CONTROL IS LOAD-BEARING, not decoration: `return false` satisfies the assertion
> above and would leave the bound un-guarded while looking enforced.

---

## 7. `StaticData` — the authored constants, field by field

> ### R0-04 — FALSE — the walk-speed default is spelled in two files and four places
>
> ⛔ *"the walk-speed literal is spelled ONCE, at the construction site in
> `SimulatableBrawlerTypes.h`"* is false, and was already false when task 65 measured it as
> "two locations that agree". Re-measured 2026-09-11, there are **four spellings in two
> files**, plus a fifth inside a log string:
>
> | site | what it is |
> |---|---|
> | `SimulatableBrawlerTypes.h:150` | `movementMaxWalkSpeed = 100.f` — the parameter default |
> | `MovementSchemeCVar.cpp:121` | `GMoveSpeed = 100.f` — the cvar default |
> | `MovementSchemeCVar.cpp:128` | `"… default 100. READ ONCE when …"` — the help string |
> | `MovementSchemeCVar.cpp:346` | `MaxWalkSpeed = 100.f` — ⛔ **the REFUSAL fallback** |
> | `MovementSchemeCVar.cpp:344` | `"Falling back to 100."` — inside the refusal log |
>
> ⛔ `:346` is the one that matters and no artefact in this initiative names it: it is what
> the character actually walks at when `OGBrawler.MoveSpeed` is set negative and refused. A
> change to the authored default that does not also change the refusal fallback leaves a
> cvar typo silently walking at the old speed. **That is a UE-layer hazard, not a comment
> defect — ROUTED to the owner of `MovementSchemeCVar.cpp`, not fixed here.**
>
> ⚠ `SimulatableBrawlerTypes.h:152` `movementStepSpeed = 100.f` is **not** a sixth site — a
> different knob that happens to share the value. Recorded so the next sweep does not fold
> it in.
>
> ⚠ Task 16 is `Done` (2026-09-08). The paragraph's *"Task 16 replaces the tunables with
> one-time cvar reads"* is a future tense about shipped work.


<!-- header lines 248-254 -->
> The movement sub-sim's authored constants. Held by value as
> simulatableBrawler::StaticData::m_movementStaticData, which PhysicsDeclaration::
> staticDataOf returns to the generic registration fold.
>
> ⭐ R-P1: the walk-speed literal is spelled ONCE, at the construction site in
> SimulatableBrawlerTypes.h — not here, not in the UE layer, not in the CMC. Task 16
> replaces the tunables with one-time cvar reads; nothing else about this type changes.


<!-- header lines 275-277 -->
> The ONE transcendental in this header, and it is SETUP time. Step 2 compares
> `glm::dot(probe.normal, up) >= cosMaxSlope` — a dot product against the servo's own
> axis (ruling #26 a) — so the walkable test costs no trigonometry per tick.


<!-- header lines 287-289 -->
> DERIVED ONCE, HERE, from the ω/ζ pair — the per-tick law multiplies and never squares.
> k = ω², c = 2ζω: the standard second-order form, so a tuner moves a FREQUENCY and a
> DAMPING RATIO rather than two gains whose units nobody remembers.


<!-- header lines 300-302 -->
> ⭐⭐ THE DISCRETE STABILITY BOUND, CHECKED WHERE THE GAINS ARE AUTHORED — because the
> alternative is finding it in PIE, and three people derived it wrong first.
>


<!-- header lines 310-328 -->
```text

THE ACTUAL RECURRENCE, with y = clearance − rideHeight, a = c·dt, b = (ω·dt)²:
    [y'; v'] = [[1 − b, dt(1 − a)], [−k·dt, 1 − a]] [y; v]
    trace T = 2 − a − b,  determinant D = 1 − a
Jury's criterion (|D| < 1 and |T| < 1 + D) collapses to  b + 2a < 4  and  0 < a < 2,
and the first implies the second. With W = ω·dt, b = W² and a = 2ζW that is:

         ⭐ W² + 4ζW < 4 ⭐           W = hoverFrequency · dt

→ ζ = 1.00 admits ω < 49.71;  ζ = 0.62 admits ω < 66.79;  ζ = 0.50 admits ω < 74.16.
MEASURED against the recurrence itself, not just derived: ω = 60 ζ = 1 diverges
(1, 1, 2, 3, 5, 8, 13 …) and ω = 119 ζ = 1 reaches 10⁶ in eight ticks. Both PASS
`ω < 120`. The shipped pair sits at W² + 4ζW = 2.489.

⚠ STABLE IS NOT THE SAME AS SMOOTH — read `hoverDampingRatio`'s comment below for the
monotone region, which is tighter and is where the feel lives.
[movement-sim task 76] ONE PREDICATE, TWO READERS: this check on the AUTHORED pair,
and the two static assertions beside `hoverGainsAreStable` that pin the bound's SHAPE.
They cannot disagree, because the bound is written once.
```

> ### R0-05 — FALSE ×2 — the argument count, and `ACharacter`
>
> ⛔ *"The ctor already takes 19 positional arguments, and a 20th trailing `bool`"* —
> counted on the shipped bytes 2026-09-11, `StaticData`'s constructor takes **22**
> parameters, so a trailing `bool` would be the **23rd**. The argument the sentence makes is
> unaffected and gets stronger; only the numbers are wrong.
>
> ⛔ *"`UCharacterMovementComponent` … the component the factory adopts"* and every other
> `ACharacter` in this file describe a type this game stopped using at task 19:
> `AOGBrawlerUECharacter : public APawn` (`OGBrawlerUECharacter.h:49`), and the CMC was
> retired from the gameplay path at task 15. ⭐ **The useful framing, from task 81's audit:
> this is not a lone stale file, it is the LAST present-tense site.** Every other place in
> the tree already reads *pawn* or past tense — the two `OGBrawlerSimulationManager.cpp` comments that
> said it, carried since into `OGBrawlerSimulationManager-rationale.md` §13.3 and §13.4, both say the
> factory *"ADOPTS the pawn's"* own (§13.3) or existing (§13.4) *"root capsule"*. **Owner: task 58.**
> (The §13.4 sentence is `BrawlerRosterUImpl-rationale.md` §2 since og-ue-simulationcomponentandmanagersplit task 23.)
>
> ⚠ **One `ACharacter` is NOT correctable by any comment pass**: it is inside the message of
> a `static_assert` task 76 landed (*"…instead of adopting the ACharacter's own capsule…"*).
> That is code, this task may not reword it, and a message is exactly where a stale noun is
> hardest to notice. **Routed.**


<!-- header lines 348-376 -->
```text
SIM-LOCAL, NOT SERIALIZED and deliberately not a constructor parameter.
⭐ `true` SINCE TASK 15: step 5 re-places the body every tick and this sub-simulation
IS the character's locomotion. `false` computes the whole of `State` but skips the two
adapter writes — the passenger arm, which now exists only for the LLT rig, and which the
rig PINS for itself (`Rig()` in `BrawlerMovementSimulationTest.cpp`) rather than
inheriting from here.

⛔⛔ THIS FLAG IS HALF OF A PAIR, AND THE PAIR IS LOAD-BEARING. `drivesBody` only says
what the SIM does; what the CMC does is decided by `PhysicsSetup::body.simulatePhysics`.
⭐ BOTH WERE FLIPPED TOGETHER AT TASK 15, AND NEITHER MAY BE FLIPPED BACK ALONE:
  * `simulatePhysics = true` beside `drivesBody = false` leaves NOBODY driving the
    capsule. UE 5.6's `UCharacterMovementComponent` early-returns whenever
    `UpdatedComponent->IsSimulatingPhysics()`, on BOTH its sync and its async path, and
    the component the factory adopts IS the CMC's `UpdatedComponent`
    (`SimulationManagerUImpl.cpp` hands the factory `character->GetCapsuleComponent()`).
    The CMC switches itself off, step 5 is skipped, and engine gravity is off too: an
    inert no-gravity rigid body sitting where it spawned. THE PLAYER CANNOT MOVE.
  * `drivesBody = true` beside `simulatePhysics = false` leaves TWO AUTHORITIES — the sim
    writing the body while the CMC still drives the same component.
  Read `PhysicsSetup::body`'s comment before changing either one.
⛔ The TELEPORT seed (step 0) writes the body regardless of this flag: a respawn must
move the capsule.

⚠ WHY A DEFAULTED MEMBER RATHER THAN A CTOR PARAMETER. This is a one-way architectural
switch, not a tunable: nothing in production should ever set it false again. The ctor
already takes 19 positional arguments, and a 20th trailing `bool` after a run of `float`s
is a live miswiring hazard for the tasks that extend that list (16, 20, 48). A test that
wants the passenger arm assigns it by name on its own instance, which reads better at the
call site than a positional `false` nineteen arguments deep.
```


<!-- header lines 379-379 -->
> ContinuousAccelBrake


<!-- header lines 381-381 -->
> Cadence (20 ticks = 1/3 s at the 60 Hz sim clock)


<!-- header lines 385-386 -->
> Attachment. `cosMaxSlope` is derived from `maxSlopeAngleDeg` in the ctor and is the
> form step 2 actually uses; both are kept so a debugger shows the authored degrees.


<!-- header lines 390-397 -->
> The SIM's gravity law, per character. NEGATIVE (world −Z).
> ⭐ [task 56 / ruling #28] APPLIED ON EVERY TICK, WITH NO BRANCH THAT TURNS IT OFF — the
> servo HOLDS THE BODY UP AGAINST IT while `Supported` rather than replacing it, which is
> what a suspension does and what makes the vertical velocity continuous at the support
> boundary. It used to run only while `Airborne`, and the seam between that branch and the
> dead-beat one was the defect ruling #28 removed.
> ⛔ The BODY's `enableGravity` is FALSE — see PhysicsSetup. Under ruling #14(c) engine
> gravity would double-apply against this term and corrupt the very push-out step 6' reads.


<!-- header lines 401-405 -->
> Hover geometry. Steady state is `clearance == rideHeight` with `velocityUp == 0` —
> CONTACT-FREE, which is why no standing contact manifold exists for the solver to fight.
> ⭐ Ruling #13, closed 2026-09-06 ON MEASUREMENT (task 9 q3), and these two STAND: a 40 cm
> drop at the exact edge of probe range was still detected and caught. They superseded the
> revision-6 text's 20/150.


#### `hoverFrequency` — the hover-gains group banner ∴D-05

<!-- header lines 408-417 -->
> ⭐⭐ THE HOVER SERVO'S GAINS, IN FEEL UNITS — movement-sim TASK 56 / ruling #28.
> ⛔ `maxSnapSpeed` (400 cm/s, ruling #13's dead-beat clamp) IS GONE, and the whole law it
> clamped went with it. It was a VELOCITY ceiling on a law that ASSIGNED velocity from
> position error; there is no assignment left to clamp. `hoverMaxAccel` is its replacement in
> role only — an ACCELERATION ceiling on a law that integrates. A stale ini or cvar naming
> `maxSnapSpeed` must be rejected loudly rather than silently ignored; there is no cvar/ini
> path in the tree yet (task 16 builds it), so that obligation is recorded HERE and belongs
> to task 16 when it lands.
>
> ω in rad/s. `hoverStiffness = ω²` below. Bigger = snappier and less forgiving.


#### `hoverDampingRatio` — the DISCRETE critical damping ratio ∴D-03

<!-- header lines 419-444 -->
```text
ζ, dimensionless. `hoverDamping = 2ζω` below.

⭐⭐ ζ = 1 IS *NOT* CRITICAL DAMPING HERE, AND THAT IS THE WHOLE POINT OF THIS COMMENT.
ζ = 1 is the continuous-time answer. This servo is a DISCRETE recurrence, and the value
that makes ITS response monotone is

         ⭐ ζ_crit = 1 − ω·dt / 2 ⭐            (0.6167 at ω = 46, 60 Hz)

— the ζ at which the two eigenvalues coincide (T² = 4D ⇔ (a + b)² = 4b ⇔ ζ ≥ 1 − W/2).
MEASURED on a 1 cm perturbation, error per tick:
    ζ = 1.00   .412 .483 .161 .238 .057 .120 .016 .062   ← alternates sign: it RINGS
    ζ = 0.62   .412 .141 .045 .014 .004 .001 .000 .000   ← monotone, 95 % in ~4 ticks
At ζ_crit the repeated eigenvalue is λ = 1 − W, so the error decays ×λ per tick.

⚠ AND MONOTONE NEEDS λ > 0 AS WELL, WHICH IS A SECOND, TIGHTER BOUND: λ = 1 − ω·dt > 0
⇒ ω < 1/dt = 60 rad/s at 60 Hz. Critical damping buys the FASTEST decay, not a monotone
one — ω = 70 (ζ_crit 0.417) and ω = 80 (ζ_crit 0.333) are both critically damped and both
alternate sign every tick. ⭐ SO THE SAFE TUNING REGION IS `ω < 60` WITH `ζ ≈ 1 − ω·dt/2`,
not a point. ω = 46 sits inside it with λ = +0.233.
⚠ RECORDED AS A LANDMARK AND NOT A RECOMMENDATION: ω = 60, ζ = 0.5 puts BOTH eigenvalues
at exactly 0 — a dead-beat, reached in one tick. It is knife-edge (λ crosses zero there)
and it reintroduces the abruptness ruling #28 moved away from. Do not ship it.

⚠ THE VALUES THEMSELVES ARE UNMEASURED FEEL. The LAW is ruled; ω and ζ are provisional
defaults the user tunes in PIE and then records in ruling #28. Step 0 logs both, and the
ζ_crit for the configured ω beside them, so a tuning session can see which side it is on.
```


<!-- header lines 446-452 -->
> The servo term's acceleration ceiling, cm/s². A fall that outruns it bottoms out into
> contact and is caught by depenetration plus step 6' — the same safety net the dead-beat's
> velocity clamp had under it.
> ⚠ The TOTAL vertical acceleration is `gravity + clamp(servo, ±hoverMaxAccel)`, so the true
> per-tick velocity bound is `(hoverMaxAccel + |gravity|)·dt`, not `hoverMaxAccel·dt`. The
> feed-forward is inside the clamped term; that is what buys the exact steady state, and it
> is what makes the bound asymmetric by exactly one `gravity`.


#### `hoverPullDownAccel` — the one-sided servo's one knob ∴D-04

<!-- header lines 455-469 -->
```text
⭐⭐ THE ONE-SIDED SERVO'S ONE KNOB — movement-sim TASK 57 / USER RULING #29, 2026-09-07.
A CEILING ON THE DOWNWARD PULL, in cm/s², applied ONLY above ride height. `0` — the shipped
default, and the user's own request — means the vertical law above ride height is
**gravity and nothing else**: SUPPORTED MEANS HELD UP, NEVER PULLED DOWN.

⚠ IT IS NOT `hoverMaxAccel`'s twin. `hoverMaxAccel` bounds the servo's authority in BOTH
directions and stays 30 000; this bounds the SPRING TERM ALONE, and only on the arm where
the spring would act WITH gravity. Raising it makes step-downs snappier (980 ⇒ 2 g total
⇒ a 40 cm step recaptured in ~12 ticks instead of ~17); it does not make landings firmer,
it does not change the upward bound, and it can never lift the character.
⚠ WHAT IT COSTS AT 0, MEASURED (task 57): the character rides a hair below ride height while
walking, because a tick that lands a float hair ABOVE ride height gets gravity alone and
falls `|gravity|*dt²` = 0.272 cm before the servo takes it back. That ripple is one tick
deep and is the price of the one-sided law; the design predicted it (design_ledge_fall_
and_landing.md §1 "Why no chatter at e = 0") and PIE decides whether it is visible.
```


<!-- header lines 471-473 -->
> DERIVED in the constructor from the pair above. k = ω², c = 2ζω. Not constructor
> parameters: a tuner must not be able to author a k and a c that no (ω, ζ) produces, which
> is exactly how the stability check above would be bypassed.


<!-- header lines 476-478 -->
> Knockback / launch (task 27). Travel distance is the closed form
> knockbackSpeed² / (2·launchDecel) = 2000² / 8000 = 500 cm = 5 m — the user's
> "thrown 5 metres and then quickly come to a stop".

⛔ **`knockbackSpeed` IS NO LONGER A MEMBER OF THIS TYPE — task 27, 2026-09-12**, and the
sentence above is kept because it is the provenance of the 5 m, not because the field is here.
The speed became **per attack**: `HitReactionSpec::knockbackSpeed`, authored in
`simulatableBrawler::StaticData::m_hitReactions` beside the sequence table it is indexed by. Only
`launchDecel` remains here, and that is the split the design argues for — **the decel is the LAW
and the speed is the HIT**. The closed form therefore reads
`spec.knockbackSpeed² / (2·sd.launchDecel)` and is per attack; at the shipped right/left rows it
is still exactly 5 m.

⚠ **THE CONSTRUCTOR CALL SHIFTED, AND IT IS NEARLY ALL `float`.** Removing one argument from a
21-argument positional call reassigns every constant after it and **compiles cleanly**. The call in
`SimulatableBrawlerTypes.h` now carries a `/*paramName=*/` annotation on EVERY argument — an
explicit exemption under the comment rule, and the only thing that makes this class of error
visible at the site forever. It was also checked by measurement: every remaining member was printed
with its value before and after the edit and the two lists diffed byte-identical
(`impl/task27/sd_before.txt`, `sd_after.txt`).


<!-- header lines 481-481 -->
> Dash / dodge (task 31). `dashSpeed` is ASSIGNED on the entry tick, never added.

> ### R0-06 — FALSE — a line number that is wrong at two sites
>
> ⛔ `OGBrawlerUECharacter.cpp:69` does not contain `InitCapsuleSize`; `:69` is a `// ====`
> separator. The call is at **`:96`**, `InitCapsuleSize(42.f, 96.0f)`, re-verified
> 2026-09-11. ⚠ The identical wrong number is duplicated verbatim at
> `SimulatableBrawlerTypes.h:277` — **routed, not fixed.**
>
> ⭐ The `42 / 96` contract is stated at **four** sites and the values agree at all four:
> this header, `SimulatableBrawlerTypes.h:277`, and `OGBrawlerUECharacter.{h:62,cpp:82}` —
> the last two opening *"⛔⛔ 42 / 96 IS A CONTRACT"*. Two carry the stale line number and
> two do not.

**Update (netcode task 25, 2026-09-26).** `OGBrawlerUECharacter.{h,cpp}` were converted. The header
copy of the contract is gone, and the `.cpp` statement is now guard G-01 of
`OGBrawlerUECharacter-guards.md` (Unreal module docs), tagged at the `InitCapsuleSize(42.f, 96.0f)`
call. After the conversion that call happens to sit at `OGBrawlerUECharacter.cpp:69`, so
`SimulatableBrawlerTypes.h`'s `:69` is correct again, by coincidence. Cite G-01, not a line.

**Update (og-ground-aligned-attacks task 37, 2026-11-05).** Later edits above the call (the camera cvars of
og-ground-aligned-attacks tasks 21 to 21e among them) moved it again: on 2026-11-05 it is
`OGBrawlerUECharacter.cpp:177`, the line under G-01's tag, so the `:69` in the two carried quotes is stale once
more. The quotes are kept verbatim; cite G-01.


<!-- header lines 485-487 -->
> MUST equal the ACharacter's authored capsule (OGBrawlerUECharacter.cpp:69
> InitCapsuleSize(42.f, 96.f)). The adopt-root factory path does not resize the
> capsule — it `checkf`s that the descriptor AGREES with it.

---

## 8. The ground probe's lateral inset — task 54 ∴D-02

The second of the four blocks this task's acceptance criterion names (66 lines,
493-558 of the pre-conversion file). ⭐ **Split:** the prohibition at 514-518 is guard
**G-03**, tagged on `kProbeShrink`; the defect report, the arithmetic and the cost table
are below.

> ### R0-07 — FALSE — three line numbers into `ChaosSpatialQueryAdapter.cpp`
>
> ⛔ The block below cites `bFindInitialOverlaps = true` at `:543-544` and the
> minimum-`Time` blocking hit at `:564-571`. Measured 2026-09-11 they are at **`:547`** and
> **`:565-574`**. The quoted sentence is verbatim-correct; only the numbers drifted.
> ⭐ **Do not write new numbers.** Three `grep -F`-unique content anchors exist and survive
> an edit above them:
> `localParams.bFindInitialOverlaps = true;` ·
> `// overlap comes back with Time == 0, so it naturally wins.` ·
> `glm::mat4 worldMat = transform * vol.offsetTransform;`
> ⚠ Even task 65's replacement numbers for these three had drifted again within a day,
> which is the argument for anchors, made by the correction itself.


<!-- header lines 493-513 -->
```text
⭐⭐ THE GROUND PROBE'S LATERAL INSET — movement-sim TASK 54, 2026-09-07, AND IT IS A
DEFECT FIX, NOT A TUNING KNOB. The user reported, in PIE: *"driving a character into a
static wall, the character gets pushed into the ground and then pops out and over corrects
into the air, over and over again."* Client `_3` logged **755 `[Movement.surface]`
transitions** in one session and **every single `-> Airborne` line read `clearance=0.000`**.

⛔ THE CAUSE WAS THIS DESCRIPTOR, and it is upstream of step 6' and of the servo — the two
places the mechanism was looked for first, and three separate hypotheses about them were
MEASURED WRONG (impl/impl_notes_seam_54.md §1.4). The probe used to be a capsule the SAME
SIZE as the body at the body's OWN pose, so a body pressed against a wall made the PROBE
overlap that wall. `ChaosSpatialQueryAdapter::sweep` sets `bFindInitialOverlaps = true`
(:543-544) and then takes the MINIMUM-`Time` blocking hit (:564-571) — its own comment says
*"An initial overlap comes back with Time == 0, so it naturally wins"* — so the wall BEAT
the floor. Its normal is horizontal, step 2's `walkable` test failed, and the character was
classified `Airborne` **while standing on the floor**. Gravity then ran (the sink), and the
next Floor tick's clamped `maxSnapSpeed` correction was retained by the airborne branch into
a ballistic launch (the pop). Measured, on the shipped `integrate` against an analytic
floor+wall: 19 flips / 300 ticks and a `pos.z` excursion of 96 → 181 cm.

⭐ [movement-sim task 56] THE SECOND HALF OF THAT MECHANISM — the retention line — IS NOW GONE
TOO: ruling #28 replaced the two vertical laws with one, so there is no dead-beat correction
```


<!-- header lines 519-558 -->
```text

⭐ THE FIX IS UE'S OWN `UCharacterMovementComponent::ComputeFloorDist` PATTERN: inset the
floor probe from the body's silhouette so a body IN CONTACT with a wall does not overlap it.

THE ARITHMETIC, and it depends on a convention that had to be ESTABLISHED rather than
assumed — the other reading gives a different offset. `CapsuleGeometry::halfHeight` is the
**TOTAL** half-height (centre to tip, hemisphere included). It carries no comment of its
own, so both of its consumers were followed to an engine API Epic documents:
  * the PROBE path — `ChaosSpatialQueryAdapter::registerVolume` calls
    `FCollisionShape::MakeCapsule(radius, halfHeight)`; UE 5.6's `CollisionShape.h` says
    *"Note: This is the full half-height (needs to include the sphere radius)"*.
  * the BODY path — `ChaosPhysicsFactory` calls `UCapsuleComponent::InitCapsuleSize` and
    `checkf`s against `GetUnscaledCapsuleHalfHeight()`; UE 5.6's `CapsuleComponent.h`
    documents that field as *"Half-height, from center of capsule to the end of top or
    bottom hemisphere."*
So the body's bottom tip is at `centre.z - capsuleHalfHeight`, and for a probe of radius
`r - s` and total half-height `hh - s` whose bottom tip must land in the SAME place:
    probeCentre.z - (hh - s) == centre.z - hh   ⇒   probeCentre.z == centre.z - s
⇒ shrink by `s` AND drop by `s`. The probe's cylinder is untouched (its half-length is
  `(hh - s) - (r - s) == hh - r`); only the hemisphere shrinks.

⚠ WHAT `s` BUYS AND WHAT IT COSTS — all three are for PIE to confirm, NOT measured in-engine:
  + `s` cm of immunity: a body exactly touching a wall clears the probe by `s`.
  − ground detection narrows by `s`: the capsule may overhang a ledge edge by up to `s` cm
    before the probe stops finding floor.
  − on a SLOPE the probe reads `s * (secθ - 1)` cm MORE clearance than the body has, so the
    character settles that much lower: 0 flat, 0.155 cm at 30°, 0.414 cm at the 45°
    `maxSlopeAngleDeg` cap. Second-order against the `rideHeight * cosθ` perpendicular gap
    step 4 already documents (7.07 cm at 45°), and one-way.
    ⛔ NOT removable by a different offset: zeroing it needs `offset.z = -s / n.z`, which is
    slope-dependent, and a static descriptor cannot express it. It is the price of this fix.
  `BrawlerMovement.GroundProbeSlopeBiasIsTheShrinkTerm` pins that closed form, and
  `...MeasuresTheBodysClearanceOnFlatGround` pins the flat-ground reading as EXACT.

⛔ THIS IS THE PRAGMATIC FIX, NOT THE STRUCTURAL ONE, AND THE LEAD RULED IT KNOWING THAT
(2026-09-07). `s` only buys `s`: a body the solver leaves genuinely PENETRATING a wall by
more than `s` overlaps the probe again. The structural answer is for `sweep` to return the
nearest WALKABLE blocking hit (or more than one hit), which needs a `SpatialQueryAdapter` /
`SweepHit` change that FOUR sub-simulations share — so it gets its own scoped task rather
than riding a defect fix. Do not fold it in here; do not delete this paragraph either.
```

---

## 9. `PhysicsSetup` — the adopted capsule and the attachment probe


<!-- header lines 561-572 -->
> All physics setup descriptors for the movement simulation.
>
> THE CHARACTER CAPSULE ITSELF, adopted — not a body this sub-sim creates. `isRoot` sends
> the factory down the adopt path (task 8): it configures the ACharacter's existing capsule
> component and asserts the descriptor's dimensions agree with the authored ones, so
> `bindings.ownBodyId` IS the capsule body id.
> ⭐ [movement-sim task 76] THE CAPSULE'S BODY FLAGS, HOISTED OUT OF `body`'s initializer SO THE
> COMPILER CAN READ THEM. `PhysicalObjectDescriptor` holds a `std::vector<ShapeDescriptor>` and
> is therefore NOT a literal type — `PhysicsSetup::body.body.enableGravity` is
> `error C2131: expression did not evaluate to a constant`. `BodyDescriptor` alone IS one: four
> `bool`s and a scoped enum. The hoist costs one name and buys five compile-time reads; not one
> flag's value changed.


<!-- header lines 574-596 -->
> ⭐⭐ TRUE SINCE TASK 15, and PAIRED WITH `StaticData::drivesBody`.
> The component this descriptor adopts is the ACharacter's capsule, which is also
> the CMC's `UpdatedComponent`. UE 5.6's CMC early-returns on
> `UpdatedComponent->IsSimulatingPhysics()` in BOTH its synchronous and its async
> path, so setting this true does not merely ADD physics — it SWITCHES THE CMC OFF.
> ⭐ THAT IS NOW THE INTENT, not a side effect: task 15 retired the CMC from the
> gameplay path (`MOVE_None`, component tick disabled, `bAutoActivate = false`, and
> the `Move()` / `AddMovementInput` binding deleted) and flipped `drivesBody = true`
> in the same change, so the movement sub-simulation takes over as the capsule's one
> authority. The flags are applied here, by the factory's adopt-root pass, not in
> the character's constructor.
> ⛔ NEITHER MAY BE FLIPPED BACK ALONE. `simulatePhysics = false` here while
> `drivesBody` stays true gives TWO authorities on one component; `simulatePhysics`
> true with `drivesBody` false gives NONE — with `enableGravity` false the capsule
> would not even fall, just a shovable no-gravity body, and the player could not
> move at all. It would fire the moment a brawler registers; it is not latent.
> ⭐ THE KINEMATIC CAPTURE NOTE, kept for provenance: while this shipped `false`,
> `FConstGenericParticleHandle::GetP()` falling back to X/R for a non-dynamic
> particle (`ChaosPhysicsBodyAdapter.h`, tasks 46/47) is what made the passenger arm
> observable. The body is dynamic now, so P is the integrated pose and the capture
> reads it directly.
> ⚠ `p.AsyncCharacterMovement = 1` was DELETED from Config/DefaultEngine.ini by
> task 15 — the async CMC path it enabled has no gameplay role left to play.


<!-- header lines 603-603 -->
> ADOPT the ACharacter's own capsule instead of creating a body under it.


<!-- header lines 605-606 -->
> The upright capsule. This is also the flag that makes `LinearBodyState`'s
> dropped rotation sound rather than a lie (PhysicsBodyState.h's CHOICE RULE).


<!-- header lines 608-610 -->
> The engine RE-SOLVES this body from the pushed anchor state on a replay: it
> is the one body in the game whose separation we read back, so replaying its
> recorded motion would discard exactly the information step 6' consumes.


<!-- header lines 621-624 -->
> ⭐ FREE WITH THE HOIST, and it RETIRES NO must-never-move row — the four flags this pins were
> never on that list, and claiming otherwise would leave a gate green on a row that still needs
> its prose. `simulatePhysics`/`drivesBody` is the loudest un-enforced pair in this file;
> `drivesBody` is authored data and unreachable from here, so this pins the half that is not.


<!-- header lines 644-645 -->
> Ruling #5 (closed 2026-09-03) = BLOCK. Brawler-vs-brawler separation is
> the solver's, mass-ratio 50/50, read back as position only.


<!-- header lines 657-662 -->
> THE ATTACHMENT PROBE — one capsule volume, swept straight down
> `rideHeight + snapDistance` in step 2. It searches `world` ONLY: other brawlers must not
> be ground.
>
> ⭐ [movement-sim task 54] IT IS INSET `kProbeShrink` FROM THE BODY'S SILHOUETTE AND
> DROPPED BY THE SAME AMOUNT, so its bottom tip coincides with the body's and every

> ### R0-08 — FALSE — one more line number, same file
>
> ⛔ `ChaosSpatialQueryAdapter.cpp:503` is the `m_volumes` lookup. The line the paragraph
> means, `glm::mat4 worldMat = transform * vol.offsetTransform;`, is at **`:506`**.
> Re-verified 2026-09-11. Use the anchor, not the number.


<!-- header lines 669-671 -->
> Drop the volume by exactly the shrink: `probeBottom == bodyBottom` (see the
> derivation above). `offsetTransform` is applied by the adapter as
> `transform * offsetTransform` (ChaosSpatialQueryAdapter.cpp:503).


> ### R0-25 — the spelling below is history since og-sim-metres-yup task 5a (2026-10-25); the property holds
>
> The drop is now built as `glm::vec4(simFrame::withHeight(glm::vec3(0.f), -kProbeShrink), 1.f)`
> and the check reads it back as `simFrame::height(glm::vec3(probeOffset[3]))`, the signed pick along
> `simFrame::kUp` (`SimulationFrame.h`). In the Z-up frame that pick was `.z` with no negation, and
> in the Y-up frame it is `.y`, also with no negation, so the check computes the same floats as the
> `+ probeOffset[3].z` the paragraph names (its up component), and the exactness argument is unchanged: `a + h` with `h == -s` is `a - s`. `withHeight` on a `+0` vector
> keeps the other two components `+0`, as the literal `(0, 0, -kProbeShrink)` did; `-kUp * kProbeShrink`
> would have made them `-0`. `kProbeShrink` itself is `0.01f` (metres) since og-sim-metres-yup task 22; task 5a
> had written it a wrapped `1.f`, which was 1 cm.


<!-- header lines 678-689 -->
> ⛔ [movement-sim task 76] THE SHRINK AND THE DROP ARE ONE TERM, AND THIS READS BOTH.
> `probeBottom == bodyBottom` holds only while the volume is dropped by exactly the
> amount it was inset; move either alone and the probe stops measuring the body's own
> clearance while every reading still looks plausible.
> ⚠ IT IS AN `OG_CHECK` AND NOT A `static_assert` ON PURPOSE. The `static_assert` form
> — hoisting the offset to an `inline constexpr glm::mat4` and pinning `[3].z` against
> `kProbeShrink` — compiles, and it FIRES when the drop moves alone. But the capsule
> shrink is `sd`-dependent and therefore invisible to it: change THAT half alone and it
> stays clean. That is an assertion equivalent to ONE INSTANCE of this prohibition,
> licensing the other half while looking enforced. Measured, both arms, at task 75.
> The comparison is written as `+ probeOffset[3].z` rather than `- (-...)` so it is
> EXACT in IEEE for any authored capsule: `a - s` and `a + (-s)` are the same float.

---

## 10. `RuntimeBindings`


<!-- header lines 710-716 -->
> The one shared definition lives in OGSimulation/PhysicsDeclaration.h. The
> PhysicsDeclaration concept requires `same_as<PhysicsRuntimeBindings&>`, so a
> field-identical per-sim copy is a DISTINCT type and does not conform; this
> alias keeps every existing `brawlerMovementSimulation::RuntimeBindings`
> spelling valid while making the type the shared one.
>
> For THIS declaration `parentBodyId == ownBodyId`: the adopted capsule is its own root.

---

## 11. `kWorldUp` — the servo's axis

> ### R0-24 — "the one place the up direction is spelled", the task-49 grep and "written component-wise" are history since og-sim-metres-yup task 5a (2026-10-25)
>
> `kWorldUp` is now `inline constexpr glm::vec3 kWorldUp = simFrame::kUp;`, and its assertion is the
> whole-object `static_assert(kWorldUp == simFrame::kUp, …)`. The message keeps its "world up" wording
> (user ruling Q5 of og-sim-metres-yup, 2026-10-25). The up direction is spelled once for the whole
> simulation, in `SimulationFrame.h`'s axis tables, so this header holds no `0.f, 0.f, 1.f` at all and
> task 49's one-hit grep now finds none. Its intent, no second spelling of up here, still holds.
> `kWorldUp` stays the one name the servo reads (the paragraph's list of readers is unchanged), and the
> value was the same `(0, 0, 1)` in the Z-up frame and is `(0, 1, 0)` since the flip to Y up (task 19). The
> component-wise form below compared against a
> literal the frame would no longer match after the axis flip; comparing against `simFrame::kUp` holds in
> both frames.


<!-- header lines 721-731 -->
> ⭐ THE SERVO'S AXIS — THE ONE PLACE THE UP DIRECTION IS SPELLED IN THIS HEADER.
>
> ⛔ USER RULING #26 = (a), 2026-09-06: the hover servo MEASURES and ACTUATES on world-up.
> Everything that needs "up" reads this constant, or the `up` local step 2 seeds from it:
> the probe direction, the walkable test, the airborne frame fallback, `surfaceNormal`'s
> default below and step 5's support term. There is deliberately no second `(0, 0, 1)` in
> this file — `grep "0\.f, 0\.f, 1\.f"` returning only this line is a task-49 AC.
>
> WHY A NAME AND NOT A LITERAL: tasks 20 (wall run) and 48 (ski) will need the axis to
> follow `SupportState` — a world-up correction changes the NORMAL gap by only `cosθ`, which
> is exactly 0 on a wall — and that stays ONE BRANCH on step 2's local only while the axis


<!-- header lines 737-744 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row — it pins user ruling #26 (a), which
> the paragraph above states and nothing checked.
> ⚠ WRITTEN COMPONENT-WISE ON PURPOSE. Task 49's acceptance criterion greps this file for
> `0\.f, 0\.f, 1\.f` and requires exactly ONE hit — the declaration on the line above. The
> obvious whole-object form, comparing `kWorldUp` against a freshly spelled `glm::vec3` of the
> same three components, would be a SECOND hit and would break that AC while asserting the same
> thing. (It compiles: whole-object glm equality is constexpr in this tree. It is the AC, not
> the language, that rules it out.)

---

## 12. `DerivedState` — off-wire per-tick scratch


<!-- header lines 752-754 -->
> OFF-WIRE per-tick scratch: visualization and debugging only. Nothing in `integrate`
> READS these — they are written at the end of the step they describe, so a consumer that
> started depending on one would be depending on last tick's value without saying so.

Since og-ground-aligned-attacks task 33 the slice also has `contactNormal`, the sweep's GJK normal when the probe is
walkable, else `kWorldUp`; the movement visualization's probe-sphere reconstruction reads it (§33).

---

## 13. `PlayerInput` — the input flags byte

> ### R0-23 — the type this section describes is DELETED since og-syncedInput-rework task 4 (2026-10-03)
>
> ⛔ Every quote below describes `brawlerMovementSimulation`'s own `PlayerInput`, and R0-21 says
> *"`zero()` and `PlayerInput{}` still coincide here"*.
>
> **Now.** Task 4 deleted `class PlayerInput`, its `zero()`, its `SerializableFields`
> specialization, its APPEND-ONLY descriptor assertion and its `SimulationInput` assertion. The
> byte this section is about is `simulatableBrawler::SyncedPlayerInput::flags` (byte 38 of the 39 B
> input wire since task 3); its order is held by that struct's descriptor-tuple `static_assert`,
> which names the deleted assertion it succeeds. What stays in this header:
> `kInputFlagHoldGuard` and its `static_assert` (whose message now names
> `SyncedPlayerInput::flags` and states that bits 1-7 are UNASSIGNED), and step 1's `frozen` gate.
> Guard G-05 now stands on `kInputFlagHoldGuard`. The quotes' numbers (*"ate 10.264 B … ~2.5 bytes
> remain"*) are history for the reason R0-22 gives, and the bit-budget sentences R0-09 flags were
> corrected at every present-tense site by task 4, which absorbed task 58's item 3; the quotes stay
> verbatim. Every input bit's neutral value is still 0: `SyncedPlayerInput::zero()` has `flags` = 0.

> ### R0-09 — FALSE — "bits 1-7 RESERVED, and already spoken for", twice more
>
> ⛔ Sites 2 and 3 of the three in this header (site 1 is in §2, R0-03). Same correction:
> **bits 1-7 are UNASSIGNED**, not spoken for; task 48 adds no input field, and 20, 21 and
> 31 each say *"if"*. ⚠ Task 20 is **Wall run**, not "wall-grab". **Owner: task 58**, and
> `BrawlerInputPackaging-rationale.md` already carries the corrected wording that both ends
> should converge on rather than inventing a third version. Carried verbatim, flagged.
>
> ⚠ The last sentence of the fence that used to close this block — now guard **G-05** —
> calls step 1's gate *"the single place this input is consumed"*. Measured, the constant
> is READ at six sites outside this header. See guard **G-05**'s note in
> `BrawlerMovementSimulation-guards.md`; the claim survives only on the narrow
> production-code reading.

> ### R0-18 — "holding guard freezes locomotion" is FALSE since 2026-10-02 (og-brawler-3rdControllerMode task 5)
>
> ⛔ *"holding guard freezes locomotion EXACTLY on the tick it is held"* (the paragraph below).
>
> **Now.** Bit 0 is the client's **freeze request**, not the raw guard button. The user ruled
> that hold-guard roots the character only while the scheme's own movement input is neutral
> (all schemes, keyboard included). `UOGBrawlerInputCollectionComponent::buildPlayerInput` sets
> the bit to `dInput::stickRouting::guardFreezeRequested(...)`: guard held **and** either the move
> routed with the move-stick-feeds-aim fallback off, or the move actually routed, is shorter
> than `g_moveStickDeadzone`. Guard plus movement therefore walks. Guard alone, or guard plus
> only the aim input (the mouse, or the aim stick), roots. In `AimRelativeSwapped`, guard plus
> the left stick alone roots even though the fallback routes the left stick to move.
>
> **What did NOT change.** This header: the name, bit 0, the packing, step 1's `frozen` gate
> and everything it does once the bit is set, including the exact-tick freeze. Only the
> condition under which the client raises the bit moved, and it moved in the producer because
> the simulation cannot tell a fallback move from a real one. No wire byte moved.


<!-- header lines 766-781 -->
```text
[movement-sim task 11] ONE BIT, and it is a MOVEMENT concern rather than a guard one:
holding guard freezes locomotion EXACTLY on the tick it is held (the genre's hitstop
rule), which is a property of this sub-sim's step 1, not of the guard shield.

⭐⭐ [movement-sim task 51] THE PACKING RULE, STATED AT THE TYPE SO NOBODY HAS TO GO FIND
A BACKLOG. PER-TICK INPUT IS THE SCARCE WIRE: an input byte is multiplied across every ring
entry in the packet-budget scenario, so it costs roughly TEN TIMES what a state byte costs.
Task 11's `holdGuard`, ONE `bool`, moved the ring entry stride 81 -> 82 B and ate 10.264 B
of the ordinary-join margin — a quarter of everything that was left. ~2.5 bytes remain.
⛔ SO A NEW PER-TICK INPUT SIGNAL IS A BIT IN THIS BYTE, NEVER A NEW MEMBER:
      bit 0    holdGuard (task 11).
      bits 1-7 RESERVED, and already spoken for — wall-grab (task 20), jump (21), dash
               (31), ski-tuck (48). As four `bool`s those blow the fence twice over; as
               four bits they cost nothing beyond what `holdGuard` already spent.
`bool` -> `uint8_t` is the same 1 B, which is why this re-layout moved zero wire bytes.

```


<!-- header lines 791-791 -->
> Bit 0 = holdGuard; bits 1-7 reserved. Read the packing rule above before adding one.


<!-- header lines 794-800 -->
> THE NEUTRAL INPUT for this sub-simulation, folded into the composite by
> SimulationComposite::zero() — which is all getZeroPlayerInput() now is.
> [movement-sim tasks 11, 51] ALL BITS CLEAR IS NEUTRAL: not guarding is not an action,
> and neither is any signal a reserved bit will come to carry. Unlike the radial/machine
> aim there is no (0,0,1)-style tag value here, so `zero()` and `PlayerInput{}`
> deliberately coincide — the property task 22's `ZeroInputIsTheFold` rests on, and one
> every future bit inherits for free precisely because each bit's neutral value is 0.

> ### R0-21 — the cited case is DELETED since og-syncedInput-rework task 3 (2026-10-03)
>
> ⛔ *"the property task 22's `ZeroInputIsTheFold` rests on"* (the quote above, kept verbatim).
>
> **Now.** That Catch2 case was deleted with the six-slice input composite it tested; the name
> no longer resolves. `zero()` and `PlayerInput{}` still coincide here (`zero()` returns
> `PlayerInput{}`), but this slice's `zero()` is no longer what goes on the wire. The neutral
> wire input is `simulatableBrawler::SyncedPlayerInput::zero()`, whose `flags` is 0, and
> `SimulatableBrawler.SyncedPlayerInput.ZeroSerializesToTheCapturedBytes` pins its 39 bytes,
> the flags byte at index 38 included. `SyncedPlayerInputTest.cpp`'s `is_same` `static_assert`
> holds that `simulatableBrawler::PlayerInput` is that type. The rule the quote states, that
> every input bit's neutral value is 0, holds for the flags byte on the new wire as well.


<!-- header lines 804-808 -->
> `PlayerInput::flags` bit assignments — the INPUT flags byte. Deliberately `kInputFlag`-
> prefixed to keep it distinct from `State::flags`, whose own bit constants (`kFlagFrozen`,
> `kFlagSupportMask`, `kFlagHasCommand`) are declared further down this file: the two bytes
> ride DIFFERENT WIRES — the relayed input ring and the correction state buffer — and a mask
> applied across them would be silently wrong.


<!-- header lines 811-811 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row.

---

## 14. `IntegrationUtils`


<!-- header lines 834-836 -->
> Current simulation tick — the Cadence model's commit boundary and the teleport
> stamp. Projectile's shape; plumbed from SimulationTimeStep at the
> SimulatableBrawler::integrate call site.

---

## 15. `InitialConditions` — the teleport seed


<!-- header lines 855-857 -->
> THE TELEPORT SEED — spawn and respawn. `teleportPending` is a COUNTER-FREE edge: the UE
> layer (task 13) sets it non-zero, step 0 consumes it and clears it back to zero in the
> same tick. It is on the wire because a respawn must replay identically.

---

## 16. `State` — the only state, field by field


<!-- header lines 867-882 -->
> THE ONLY STATE.
>
> `bodyState` is written by the generic post-solve capture pass
> (SimulationIntegrationExecutor::captureBodyStatesAll, via PhysicsDeclaration::bodyStateOf)
> and pushed back into Chaos's rewind timeline on a resim — neither of which this sub-sim
> writes a line of code for. Its presence in SerializableFields is what puts the body on the
> wire and in the checksum.
>
> [movement-sim task 5, 2026-09-02] `LinearBodyState`, NOT `PhysicsBodyState`: position +
> linearVelocity only, 24 B instead of 52 B. Both generic sites above keep compiling
> untouched — capture ASSIGNS a captured `PhysicsBodyState` in (the narrowing `operator=`),
> the rewind push CONVERTS one back out (the implicit widening `operator PhysicsBodyState()`),
> and neither site names a body-state type. See the CHOICE RULE at the top of
> OGSimulation/PhysicsBodyState.h. The rotation drop is sound because
> `PhysicsSetup::body`'s descriptor now sets `lockRotation` — the condition that header
> names, which did not exist when the swap landed.


<!-- header lines 886-898 -->
> ON WIRE 24. `position` is LIVE — it is last tick's post-solve capture, i.e. where the
> engine left the body after integrating our velocity and pushing it out of whatever it
> overlapped. That is what makes step 6's positional adoption FREE: there is nothing to
> copy, the capture already put the solved position here.
>
> ⚠ `bodyState.linearVelocity` IS DEAD WEIGHT — 12 B captured every tick, on the wire,
> in the checksum, and NEVER READ by a line of this sub-simulation. That is not an
> oversight; it is ruling #17(b), taken deliberately: option (a) — a NEW position-only
> 12 B body-state seam type — would have cost a mirror of task 4 across the capture
> bridge, the rewind push and every adapter, to save 12 B. The waste is the price of not
> introducing that seam type, and reclaiming it is a later wire-budget pass. (Its name is
> deliberately not spelled here: ruling #17 is the searchable anchor, and an acceptance
> criterion greps this file for the rejected type.)


<!-- header lines 903-905 -->
> ON WIRE 12. THE SIM'S velocity, and the only velocity any code here reads: assigned by
> the model, the hover servo, the gravity law and (later) dash / launch. No engine term
> has ever touched it.


<!-- header lines 908-910 -->
> ON WIRE 8 — the Cadence model's committed direction, in the surface frame. Zero under
> every other model; the layout does NOT change with `sd.model`, so switching models
> cannot move a wire byte.


<!-- header lines 913-913 -->
> ON WIRE 4 — the tick the current Cadence step began (also stamped by a teleport).


<!-- header lines 916-917 -->
> ON WIRE 1 — bit 0 frozen, bits 1-2 SupportState, bit 3 "a body command was issued last
> tick" (`kFlagHasCommand`, and step 6' below is its READER — see the note there).

> ### R0-10 — ⛔⛔ NEW — FALSE, and neither R0 audit inventoried it
>
> ⛔ The paragraph below ends *"and no file outside this header names the field at all"*, of
> `state.velocity`. Measured 2026-09-11 that is **false**:
> `BrawlerMovementVisualization.h:356` reads it (`position + state.velocity * …`), and
> `BrawlerMovementSimulationTest.cpp` names it at dozens of sites — `:378` READS it (the
> write on that line targets `bodyState.position`) and `:447` is a genuine **write**.
> *(Dated record: the three line numbers above are the 2026-09-11 files' and no longer point at those
> statements; the files have moved since. The finding stands: the test file still reads and writes
> `state.velocity`.)*
>
> ⭐ The argument the sentence supports — that nothing writes `state.velocity` between step
> 5 and the next step 6′ **inside the tick** — is sound, and that is the claim to keep. The
> tree-wide quantifier is what is wrong.
>
> ⚠ **This is the third instance of one defect shape in this single header** (the others:
> `kInputFlagHoldGuard`'s *"ONLY reader"*, §23; `kFlagHasCommand`'s *"its only reader"*,
> §23), and a fourth lives in the sibling file's history — task 66 found
> *"THE ONE AND ONLY WRITER … anywhere in the tree"* false in `BrawlerInputPackaging.h` and
> task 74 deleted it. **Found by classifying on the QUANTIFIER rather than on whether a path
> appears**, which is precisely what task 81's audit recommended a third audit should do.


<!-- header lines 920-937 -->
> ON WIRE 12 — the pose we HANDED the engine last tick, and the one input step 6'
> cannot re-derive: `bodyState.position` has since been overwritten by the capture, so
> the command is gone unless it is remembered. Wire size 61 B.
>
> ⭐⭐ [movement-sim task 50] THIS WAS OFF-WIRE SCRATCH, AND THAT WAS A LATENT DEFECT —
> ruling #27 (A). A correction destroys off-wire scratch on BOTH of its paths, which the
> off-wire block this replaces got wrong: `SimulationReconciliation::injectCorrectionState`
> DEFAULT-CONSTRUCTS the state before `readInto`, `StateCorrectionCache::
> tryInsertingCorrectState` stores that whole struct on a DISAGREEING landing, and
> `prepareResimAll` assigns it back WHOLE (`editState() = cache.getState(idx)`) — it does
> not restore "the serialized fields only". So after an adoption the scratch was ZEROED,
> step 6' skipped the first replayed tick, the into-contact velocity component was not
> killed, and the replay ended the tick `acceleration·dt` — 34.133 cm/s at the shipped
> tunables — above the authority, against `kDefaultSimilarityEpsilon` = 0.0001. The
> authority's NEXT correction then disagreed as well: ONE RESIM PER TICK for the duration
> of a wall press, each one a lossy Chaos rewind. 12 B buys the whole of that back.
> Pinned by `SimulatableBrawlerTest.cpp`'s `ReplayAfterAdoptionReproducesContactClamp`.
>


<!-- header lines 950-959 -->
> ⭐ [movement-sim task 76] A STRUCTURED-BINDING DECOMPOSITION IS AN EXPRESSIBLE STATEMENT ABOUT
> MEMBER COUNT: it compiles only if the aggregate has exactly that many public data members.
> ⚠ THE BINDING NAMES ARE FRESH NAMES, NOT THE MEMBERS' — they are spelled after the members so
> the order is readable here, but a RENAME is INVISIBLE to this assertion. The wire-order
> assertion at the bottom of this file is what catches that, as a hard error on the member
> pointer. Verified, both ways, at task 76.
> ⛔ `sizeof` AND `offsetof` ARE BOTH BLIND HERE, and that is the finding. `flags` is a `uint8_t`
> followed by a 4-aligned `glm::vec3`, so there are THREE PADDING BYTES after it: adding
> `bool commandIssued` in that slot leaves `sizeof(State) == 64` and every offset (0/24/36/44/
> 48/52) UNCHANGED. Measured at task 75, both candidates, on that exact edit.


<!-- header lines 974-978 -->
> `State::flags` bit assignments. Bits 1-2 hold a `SupportState`, so the enum may grow to 4
> values before this field has to widen.
> ⭐ [movement-sim task 56] THE BITS DID NOT MOVE. `SurfaceKind` used the same shift and the same
> mask; only the enumerator names and the third value changed, which is why this task re-quoted
> `kComposite == 321u` and `syncSize<State>() == 61u` UNCHANGED rather than re-measuring them.


<!-- header lines 984-984 -->
> ⭐ [movement-sim task 76] RETIRES NO must-never-move row.


<!-- header lines 991-994 -->
> Below this magnitude a push-out is engine noise, not contact: task 9's q3 measured a
> CONTACT-FREE hover (`pushOutTicks == 0`) with peak interpenetration 0.000005 cm, and q6
> measured 0.6 microns of ride-up between two blocking capsules. 0.05 cm is two orders of
> magnitude above both and is the threshold the spike itself used.

---

## 17. `PhysicsDeclaration` and `Dependencies`


<!-- header lines 1004-1010 -->
> Maps the GAME's aggregate static data to this sub-simulation's own slice.
> This is what makes body creation generic: the engine-side fold asks each
> declaration for its slice instead of branching on the declaration type.
> A member TEMPLATE deliberately — this header cannot name
> simulatableBrawler::StaticData, because the aggregate includes this header
> (an include cycle). GameStaticDataType is deduced at the call site, where the aggregate is
> complete.


<!-- header lines 1017-1017 -->
> Zero, and structurally so: an `isRoot` body is not attached under anything.


<!-- header lines 1024-1025 -->
> [movement-sim task 5] Return type follows State::bodyState. The generic
> consumers are written against `BodyStateLike`, not against a concrete type.


<!-- header lines 1038-1040 -->
> The FLINCH read (step 1) and, from tasks 27/31, the committed states. A SOFT edge:
> ⚠ [task 27, 2026-09-12] The task-27 half of that sentence has LANDED: step 1 reads
> `m_hitReaction` through this same dependency and `committed` is now
> `machineLaunchesMovement(machineState)`. Task 31’s `Dashing` is the half still outstanding.
> the machine sub-sim is declared FIRST in simulatableBrawler::ExecutionOrder and this
> one LAST, so `findFirstViolation` is satisfied without moving anything.

---

## 18. Pure helpers — `moveTowards` and `buildTangentFrame`


<!-- header lines 1048-1048 -->
> Pure helpers — no state, no adapter, no engine.


<!-- header lines 1051-1053 -->
> Move `current` toward `target` by at most `maxDelta`. The genre's law for every decaying
> quantity (launch decel, braking); exact at the endpoint, so a brake reaches EXACTLY zero
> rather than asymptotically approaching it.


> ### R0-26 — "world +X", "world XY" and "+Y" below name the Z-up frame's axes; since og-sim-metres-yup task 5a (2026-10-25) the code names them through `simFrame`
>
> The reference is `simFrame::kForward` unless `glm::abs(simFrame::planar(n).x) >= 0.9f`, then
> `simFrame::kRight`; `v` is `simFrame::kChartOrientation * glm::cross(n, u)`; the placeholders `u`,
> `v` that `integrate` passes in start as `kForward`, `kRight`. In the Z-up frame these were `+X`,
> `+Y`, `n.x` and `+1`, so every float was unchanged. `planar(n).x` is the forward component, a
> signed pick and not a dot product, so the comparison is the one the code made before, `glm::abs(n.x) < 0.9f`. The factor
> is routing rule 4: in the Y-up frame `cross(n, u)` comes out as `-kRight` on flat ground, and
> `kChartOrientation` (`-1` there) turns it back to `kRight`, so the frame keeps the chart's
> (forward, right) order. "Right-handed" below is the Z-up frame's reading of the basis; what
> holds in both frames is that `(u, v)` follow `(kForward, kRight)` on flat ground. §26 relies on it.
> Since og-sim-metres-yup task 26 the chart is (forward, left) and `kChartOrientation` is `+1`, so
> `v` is `cross(n, u)` itself, `kLeft` on flat ground: `(u, v)` follow the chart's own axes,
> `(kForward, kLeft)`, and the stick projection and every chart value written into `(a, b)` follow
> them with it.


<!-- header lines 1063-1070 -->
> THE SURFACE FRAME. Builds a right-handed orthonormal basis (u, v, n) with `n` as up.
>
> `u` is world +X projected into the plane and renormalised, so on FLAT GROUND the frame is
> exactly world XY (u = +X, v = +Y) — which is what makes "same stick, flat vs slope" a
> meaningful comparison rather than an arbitrary rotation. The +Y fallback is only reachable
> when `n` is within ~26° of world +X, i.e. a near-vertical wall (task 20).
>
> One `sqrt` per call and no transcendental. `n` MUST already be unit length.

---

## 19. The dual-basis decomposition — task 57 / ruling #29 ∴D-01

The third of the four named blocks (35 lines, 1082-1116). ⚠ **Nothing in it was a fence**, so
it carries **no guard id**: `⛔G-nn` marks a prohibition and this block states none.
⛔ **As task 68 shipped it, `decomposeVelocity` had no marker of any kind, and that was the
sharpest single case in this file's reader test** —
`a = (dot(planar, u) - dot(planar, up) * s) / (1 - s * s)` stood with nothing above it to say
the shape was deliberate rather than clumsy, and the reflex edit — *"simplify this back to three
dot products"* — leaves every flat-ground case in the suite green, by the `s == 0` identity
below.
⭐ **Task 82 closed that case.** This heading ends in `∴D-01`, and that tag is the line
immediately above the function's first statement in `BrawlerMovementSimulation.h`, the fast-path
test `if (glm::abs(glm::dot(v, up)) <= kDualBasisFastPathTolerance)`. That test chooses between the
2×2 solve below and the exact solve of §19.1, so the one tag covers both arms (until
og-ground-aligned-attacks task 30 the function was the 2×2 solve alone, and the tag sat immediately
above `a`'s expression). It is a DERIVATION tag, not a
fence: it prohibits nothing and says only *the reasoning for this expression is here*.
`tools/lint/guard_tag_lint.ps1` gates the join in both directions — delete the expression and
the tag goes with it, leaving this section an ORPHAN under CHECK 2; strip `∴D-01` from this
heading and CHECK 1 rejects the tag that still names it.

> ### R0-27 — FALSE for a normal with a sideways part: "only `v` is perpendicular to both (`v = cross(n, u)` is horizontal)"
>
> `buildTangentFrame` builds `u` from `kForward`, so `v = cross(n, u)` is horizontal only when `n`
> lies in the up–forward plane: flat ground and every face that leans toward ±Z. A normal with a
> sideways (X) component gives `v` a vertical part. For n = (0.5, 0.866, 0), a 30° face rising
> along X (UE Y): `u = (0, 0, −1)`, `v = (−0.866, 0.5, 0)` and `dot(v, up) = 0.5`. Then `planar`
> is not in the (u, up) plane, the "2x2 solve" below is a projection, and the part of V outside
> that plane is dropped: a 20 m/s fall came back as `(a, b, c) = (0, −10, −15)`, which recomposes
> to (8.66, −20, 0) (og-ground-aligned-attacks task 23, finding M4; reproduced by
> `BrawlerVelocityDecomposition.APureFallLeavesTheTangentChannelsEmpty`). So "`v` is perpendicular
> to the other two" and "SOLVE ... EXACTLY" in the quote below hold only on those frames. The
> quote is the 2×2 arm as it was written; since og-ground-aligned-attacks task 30 the function runs
> it only where the premise holds, and §19.1 is the solve for every other frame.


<!-- header lines 1082-1116 -->
```text
⭐⭐ THE DUAL-BASIS DECOMPOSITION — movement-sim TASK 57 / USER RULING #29, 2026-09-07.
The ONE place `state.velocity` is split into the two channels steps 3 and 4 consume.

⛔ `(u, v, up)` SPANS SPACE BUT IS NOT ORTHONORMAL. `u` is the slope tangent and `up` is world
up (ruling #26 a), so `dot(u, up) = sin(theta)` on a theta-degree face; only `v` is perpendicular
to both (`v = cross(n, u)` is horizontal). Splitting a vector with plain dot products therefore
DOUBLE-COUNTS, and both halves of that were live defects under task 56:
  * a FALL leaked into the tangential channel — `dot(V, u)` of a 2000 cm/s descent on 30° is
    1000 cm/s of "walk" the model then brakes. Positive feedback, because the tangential term
    has its own vertical component: MEASURED, the task-56 tree DIVERGES on a slope landing;
  * a WALK leaked into the vertical channel — walking a 30° slope at 600 cm/s genuinely rises
    at 300 cm/s, `dot(V, up)` reported exactly that, and the servo DAMPED it. The character
    settled 0.4197 cm off ride height (sign following the walk direction) for no reason but
    the arithmetic.

Solve `V = a*u + b*v + c*up` exactly instead. `v` is perpendicular to the other two, so `b` is a
plain dot and can be removed first; what is left lives in the (u, up) plane and is a 2x2 solve:
    b      = dot(V, v)
    planar = V - b*v
    s      = dot(u, up)                                    // sin(theta); 0 on flat ground
    a      = (dot(planar, u) - dot(planar, up)*s) / (1 - s*s)
    c      =  dot(planar, up) - a*s
Checks: a pure walk `V = 600*u` gives `(a, c) = (600, 0)` — the servo no longer sees the walk;
a pure fall `V = -2000*up` gives `(a, c) = (0, -2000)` — nothing leaks into the model.

⭐ AT `s == 0` IT REDUCES TO TODAY'S DOTS *BIT-EXACTLY*, and that is the correctness check
rather than a nicety: `x - y*0.f` is `x` and `x / 1.f` is `x` for every finite float, and on flat
ground `buildTangentFrame` returns exactly `u = (1,0,0)`, `v = (0,1,0)`. So every flat-ground
case in the suite is byte-identical across this change, and any that moved would have been
reporting a bug in this function.

⚠ DEGENERATE ONLY AT `s == 1`, a vertical face, which `Supported` cannot classify: step 2's
walkable test caps the normal at `cosMaxSlope`. The assert below is the statement of that
coupling — if a future `SupportState` arm lets the frame tilt to vertical, this is where it
surfaces, not in a silent division by zero.
```

### 19.1 The exact solve for every frame — og-ground-aligned-attacks task 30

**The two arms.** `decomposeVelocity` first tests `glm::abs(glm::dot(v, up)) <= kDualBasisFastPathTolerance`
(`1.e-6f`).

- **Fast path:** the 2×2 solve quoted above, the same operations in the same order, with its `OG_CHECK`.
- **Otherwise, the dual solve:**

  ```text
  vxUp = cross(v, up)     upxU = cross(up, u)     uxV = cross(u, v)
  det  = dot(u, vxUp)
  a = dot(V, vxUp) / det     b = dot(V, upxU) / det     c = dot(V, uxV) / det
  ```

**Why the dual solve is exact.** Each of the three crosses is perpendicular to two of `u`, `v`, `up`.
`dot(V, cross(v, up))` therefore removes `b·v` and `c·up` from `V = a·u + b·v + c·up` and leaves
`a·dot(u, cross(v, up))`, which is `a·det`; `b` and `c` follow the same way. All three share the one
determinant, because the scalar triple product is unchanged by a cyclic shift:
`dot(u, cross(v, up)) = dot(v, cross(up, u)) = dot(up, cross(u, v))`.

**Why `det > 0`.** For `buildTangentFrame`'s frame, `u` is a unit vector perpendicular to `n` and
`v = kChartOrientation · cross(n, u)`. With `kChartOrientation = +1` (R0-26), `cross(u, v) =
cross(u, cross(n, u)) = n·dot(u, u) − u·dot(u, n) = n`, so `det = dot(up, n)`. That is at least
`cosMaxSlope` (cos 45°) on a walkable normal and 1 for world up, the only two kinds of `n` that
`integrate` hands to `buildTangentFrame`. Two checks hold the coupling:

- a `static_assert` on `simFrame::kChartOrientation > 0.f`, because a negative orientation would
  negate `det`;
- `OG_CHECK(det > 0.f, …)` in the dual arm, which fails for a frame that `buildTangentFrame` did not
  make for an up-facing normal.

**What `c` is.** Since `cross(u, v) = n`, `c = dot(V, n) / dot(up, n)`: the speed along up at which
the capsule closes on (or leaves) the face. A velocity along the face has `c = 0`, and a fall gives
`c = V.up` with `a = b = 0`, which are the two checks of the quote above, now on every frame.

**Why the fast path is kept, and why the 2×2 lines are kept verbatim.** On a frame whose `v` is
horizontal the 2×2 solve is already exact. The dual solve gives the same channels there, but
not the same bits: with the dual solve forced on every frame (task 30's poison arm B),
`BrawlerVelocityDecomposition.TheFastPathIsTheLegacySolveBitForBit` found 15,645 of 30,000 vectors
with a differing channel on flat ground and the ±Z 30° faces (the first: `a` 27.2208405 against
27.2208385). Both golden traces then failed in exact mode (the flat one from tick 168, the slope one
from tick 11). Keeping the five lines unchanged keeps every float on those frames
unchanged, and the bit-identity case pins it against a frozen copy of them (`legacyDecompose`).

**Why `1.e-6f`.**

- On flat ground and on every face leaning toward ±Z, `dot(v, up)` is exactly zero [derived]: `n`
  and `u` have no X component, so `v`'s up component, `n.z·u.x − n.x·u.z`, is a difference of products
  with a zero factor. Those frames take the fast path at any tolerance of zero or above.
- How much the 2×2 solve drops grows with `|dot(v, up)|`: over 20,000 random walkable frames and
  unit velocities, the residual was at most 1.40 × `|dot(v, up)|` × `|V|` [measured, task 30, float64].
  At the tolerance that is at most 1.4e-6 · `|V|`.
- A tilted GJK normal leaves the fast path. In the flat golden trace one tick does: character A on
  the 20° ramp at tick 233, with `n` 0.66° off the up–forward plane and `dot(v, up) = −0.0124`.

**Where it changed behaviour** (task 30):

- `PlatformEdge.KnockbacksAcrossTheImportedPlatformRims`, the imported platform's rims: the ticks
  whose decomposition residual exceeded 1e-3 m/s fell from 2018 (the largest 9.2049 m/s) to 0.
- The flat golden trace: the one off-plane tick above. Its vertical channel moved by −2.99e-6 m/s
  [derived: both solves recomputed in float64 from the tick's printed frame normal and velocity], the
  servo's damping (`1 − 57.04·dt` ≈ 0.049) kept about 5% of that, and two recorded fields changed on ticks 233 and 234 by
  1.49e-7 m/s, so the golden was re-recorded. The slope golden's every tick took the fast path
  (`max |dot(v, up)|` = 0 over 10,360 calls), and it is unchanged.

**What did not change.** `buildTangentFrame` is unchanged. Building `u` from the fall line instead
(task 23's option E) was rejected: it is ill-conditioned near flat ground and flips `u`'s sign on
−Z faces, which would spin the uv chart that `committedStepDir` is stored in.

---

## 20. `machineFreezesMovement`

> ### R0-11 — UNRESOLVABLE — and it misreads at the site
>
> ⚠ *"Behaviour is identical: the same two enumerators, in the same order."* compares
> today's spelled-out `machineFreezesMovement` to a **template task 62 deleted**, which
> cannot be read. ⛔ And it misreads against code that *can* be read: the enum declares
> `GuardFlinch` before `HitFlinch` — the `DAttackState` declaration in `DAttackMachineSimulation.h`,
> re-verified 2026-09-11 — while this function tests `HitFlinch` first, so *"in the same
> order"* is the reverse of the declaration it sits beside.
> ⇒ **Recommend deleting the clause** when this text next has an owner; it
> defends a property against code nobody can see and is actively misleading against code
> they can. Carried verbatim here rather than silently dropped.


<!-- header lines 1142-1150 -->
> The genre's hitstop: a flinch freezes locomotion EXACTLY on the tick it is in effect. It
> does not decay the velocity, it zeroes the model's contribution for that tick.
>
> ⭐⭐ [task 27, 2026-09-12] **`HitFlinch` ALONE NO LONGER FREEZES.** The unconditional
> "`HitFlinch` ⇒ frozen" became "`HitFlinch` AND `m_hitReaction == Stun` ⇒ frozen", and a
> SIBLING predicate, `machineLaunchesMovement`, answers the other half: `HitFlinch` AND
> `Knockback` ⇒ **committed**, which step 3 tests FIRST. A stun is therefore exactly what a
> `HitFlinch` was before this task — the hitstop above, unchanged — and a knockback is the
> other branch of the same state. `HitReactionKind::Stun` is 0, so a default-constructed machine
> `State` in `HitFlinch` still freezes, which is what keeps every pre-task-27 rig honest.
> ⛔ The prohibition that rides both predicates is `G-22` in the guards doc: the reaction byte is
> meaningless outside `HitFlinch` and must never be tested without it.
>
⭐ **[task 84, 2026-09-20] THERE IS A THIRD PREDICATE IN THIS GROUP NOW:
`machineLocksMovement`, which is `m_currentState == DAttackState::Attacking` and nothing else.**
It carries NO tag, and that is the deliberate answer to the rule’s first question: it is one
comparison, and a reader with only the identifier in front of them cannot get it wrong.
⛔ It is specifically NOT a third clause of `machineFreezesMovement`, and not a second tenant of
`machineLaunchesMovement`: an attack neither freezes the body nor commits it — it RAMPS it, in
step 3’s own `locked` branch, and the prohibition that rides the ramp is `⛔G-24`.
⚠ Unlike its two siblings it does NOT read `m_hitReaction`, so `⛔G-22` does not extend to it;
G-22’s site is still exactly the pair that reads the byte.

> [movement-sim task 62] This was a template purely to keep `.m_currentState` and the
> enumerator DEPENDENT names while the machine State type was incomplete here. The machine
> header is included now, so both are spelled out and the compiler checks them.
> ⚠ `DAttackState` is at FILE SCOPE in `DAttackMachineSimulation.h`, outside
> `namespace dAttackMachineSimulation` — that is why the enum is unqualified while the State is
> not. Behaviour is identical: the same two enumerators, in the same order.

---

## 21. The movement models — step 3's dispatch targets


<!-- header lines 1158-1162 -->
> MOVEMENT MODELS — step 3's dispatch targets.
>
> Each is a pure function of the SURFACE FRAME: `stickUV` and the returned velocity are both
> 2D coordinates in (u, v). No model knows whether that plane is the floor, a slope or a
> wall, and no model may touch `state.bodyState`.


<!-- header lines 1165-1167 -->
> CMC-like. Accelerate toward the stick's target velocity at `acceleration`; with no stick,
> brake toward zero at `brakingDeceleration`. Stateless — it reads `currentUV` and writes no
> State slice at all.


<!-- header lines 1180-1185 -->
> "Steps, not a joystick". The direction is COMMITTED at a period boundary and held at a
> constant speed until the next one, so a mid-step stick flick cannot steer the character —
> which is the whole feel. A neutral stick at a boundary commits a STAND (zero direction);
> the timer keeps running either way, so the cadence stays on the beat.
>
> This model owns exactly two State fields, and touches nothing else.


<!-- header lines 1192-1193 -->
> Unsigned arithmetic is deliberate and safe: `stepStartTick` is only ever stamped from
> a tick that has already happened, so the difference never wraps.

---

## 22. The detach gate


<!-- header lines 1211-1213 -->
> ⛔ [movement-sim task 76] THE REQUIRES-EXPRESSION MUST BE DEPENDENT. A bare `requires { ... }`
> at namespace scope is not a SFINAE context in MSVC: the ill-formed id emits a hard error AND
> fires the assertion, which reads exactly like "not expressible" and is not.


<!-- header lines 1218-1223 -->
> ⭐ [movement-sim task 76] THE `Launched` HALF OF THE FENCE BELOW, AS A TRIPWIRE FOR THE PERSON
> EDITING `DAttackMachineSimulation.h` — the build breaks there, in the file that makes the
> statement false, and not here.
> ⚠ ONLY THAT HALF CONVERTS, so the fence below keeps its `Jumping` clause. Adding
> `kFlagJumping = 1u << 4` is INVISIBLE to any expression over the three flag constants: the new
> name is simply not in it. There is nothing to assert until the bit and its writer exist.


<!-- header lines 1234-1237 -->
> ⭐⭐ THE DETACH GATE — movement-sim TASK 56 / ruling #28. The ONE place a walkable probe hit
> can still be classified `Unsupported`, and the reason `SupportState`'s name is true rather
> than aspirational: support is a DECISION, and this is where it is taken.
>


<!-- header lines 1250-1260 -->
>
> ⚠ SO NO LLT CAN REACH THE `true` ARM, AND NONE PRETENDS TO. What IS verified is the WIRING —
> that step 2 consults this predicate at all — and it was verified by POISONING it to `return
> true` and watching `HoverHoldsRideHeight`, `HoverLiftsWhenLow` and
> `WallPressKeepsTheCharacterOnTheFloor` go RED together (the character falls); the run is
> recorded in the impl notes and the poison was reverted. `DetachGateIsVacuousUntilJumpLands`
> pins the vacuity itself, and says in the case what it does and does not prove.
>
> Parameters are the shape the filled version needs, so task 21 changes a body and not a
> signature: the state carrying the future `Jumping` bit, and the axis an upward `Launched`
> would be measured along.

⭐⭐ **THE TWO PARAGRAPHS ABOVE ARE NOW FALSE, AND THEY ARE KEPT BECAUSE THEY RECORD WHY —
task 27, 2026-09-12.** The gate is filled and BOTH arms are reachable:
`return committed && glm::dot(state.velocity, up) > 0.f;`.
* "No LLT can reach the `true` arm" — **superseded**. `BrawlerMovement.XYKnockbackDoesNotDetach`
  drives three arms: uncommitted at any vertical speed (false), committed and horizontal or
  settling (false), committed and rising (**true**), plus a discriminator that repeats the rising
  velocity UNCOMMITTED so the case is a statement about `committed` and not about the sign of
  `vz`. It replaced `DetachGateIsVacuousUntilJumpLands`, whose own header instructed exactly that:
  *"a vacuity pin that outlives its vacuity is a false comfort."*
* "Task 21 changes a body and not a signature" — **wrong about which task, and about the
  signature.** Task 27 changed both: the predicate gained a third parameter, `bool committed`,
  because the decision is no longer derivable from `State` alone. It is well-defined at the call
  site: `committed` is computed in step 1 and the surface normal `n` is keyed on `walkable`, NOT on
  `support`, so the detach decision sits strictly between them and feeds nothing the frame depends
  on.
* ⚠ **AND IT IS NOT A CLEAN WIN ON A SLOPE.** `dot(state.velocity, up)` is world z, and on a
  slope the TANGENTIAL channel contributes `knockbackSpeed · sin θ` to it — exactly the
  world-z-is-not-the-vertical-channel trap section 19 and task 57 both paid for. An up-slope
  knockback therefore reaches the true arm on the tick after the shove. The arm is the
  architect’s and is built as ruled; the measurement, and the recommendation to key it on the
  vertical CHANNEL instead, are in `impl/impl_notes_seam_27.md` and in guard `G-07`.

⭐ **[og-ground-aligned-attacks task 13, 2026-10-28] Task 27's "true the day a lift is authored" is
now.** The user's OQ6 ruling (knockback along the attacker's swing tangent; crease hits launch) needs
the arm to read the target's ground, so the predicate gained a slope arm (knob-gated until task 14) that tests the
velocity against the probe's face normal: `committed && dot(velocity, face) > kMinLaunchSine·|velocity|`.
A same-face up-slope knockback no longer reaches it; a direction that leaves the face does, on any
tick. The flat arm is the task-27 one, verbatim. Details, and the tangent frame while detached, in §30;
the prohibitions in guard G-07.

⭐ **[og-ground-aligned-attacks task 37, 2026-11-05] The gate this section describes is retired with the Legacy
support law.** `detachesFromSupport`, its flat arm and its slope arm are deleted, and guard G-07 is retired with them
(guards doc §R). The detach decision is the event rows of `classifySupport` (§34): the launch row on a written hit
tick, decided by the direction, and the separation row at or above the hover (guard G-31). Both rows read the
direction or the velocity, never an enumerator, and the `!detail::kHasLaunched<DAttackState>` tripwire's message
now names `classifySupport`. The history above is kept because it records why the decision lives in step 2 and why
it reads the velocity.

---

## 23. The movement tick — steps 6′, 0, 1, 2, 3, 4, 5

Step 4's comment block is the longest in the file (126 lines, 1563-1688) and the fourth
block the acceptance criterion names, alongside `THE SERVO IS ONE-SIDED` (34 lines,
1702-1735). ⭐ **Both are split:** three prohibitions left for the guards doc — **G-09**
(do not scale `rideHeight` with slope), **G-10** (the two forbidden servo variants) and
**G-11** (step 4 has no geometry of its own) — and everything else is below.


<!-- header lines 1270-1283 -->
> THE MOVEMENT TICK — architecture §3.3, revision 6.
>
> Steps run 6' -> 0 -> 1 -> 2 -> 3 -> 4 -> 5. Step 6' is numbered for the tick it belongs to
> (it closes LAST tick's loop) and runs FIRST here, which is the only order that works: the
> engine's answer to last tick's command is not available until the capture has landed.
>
> ⚠ `machineInput` IS THE MACHINE'S INPUT SLICE, not this sub-sim's: the move stick is packed
> onto `dAttackMachineSimulation::PlayerInput`, and the single call site in
> `SimulatableBrawler.h` is the one place it can be supplied. See the undeclared-input-edge
> note at the machine include, at the top of this file.
> [movement-sim task 62] It is SPELLED here now rather than deduced through a `MachineInputT`
> template parameter; that indirection existed only to survive the include cycle. The default
> template argument it carried was documentation — the parameter was always deduced from the
> argument at that one call site, so the default never participated.

> ### R0-20 — FALSE since og-syncedInput-rework task 3 (2026-10-03): there is no `machineInput`
>
> ⛔ *"`machineInput` IS THE MACHINE'S INPUT SLICE"* and *"the single call site in
> `SimulatableBrawler.h` is the one place it can be supplied"*.
>
> **Now.** `integrate`'s parameter list is `(deltaSeconds, input, sd, deps, bindings,
> derivedState, inboundHit)`. The move stick is a field of this sub-sim's own
> `brawlerMovementSimulation::PlayerInputView`, and `SimulatableBrawler::integrate` builds that view
> with `PlayerInputView::from(input)` and passes it through `AllInput` like every other sub-sim's. The
> prose block at the call site that explained the second argument was deleted with it. See
> R0-19 at the machine include.


<!-- header lines 1302-1307 -->
> ---- step 6': ADOPT THE ENGINE'S POSITIONAL PUSH-OUT --------------------------------
> The POSITION half of the adoption is free and structural: `bodyState.position` IS the
> post-solve capture, written by the generic capture pass before this function was
> called. There is nothing to copy. What is NOT free is separating the engine's
> contribution from our own command, which is the whole of the arithmetic below, and the
> one authored rule that follows from it.

> ### R0-12 — ⛔⛔ NEW — FALSE, and neither R0 audit inventoried it
>
> ⛔ *"`kFlagHasCommand` IS READ HERE, and this is its only reader."* Measured 2026-09-11,
> the constant is READ at **four** sites outside this header —
> `BrawlerMovementSimulationTest.cpp:343`, `:1512`, `:1532` and `SimulatableBrawlerTest.cpp:903`.
> *(Dated record: those line numbers are the 2026-09-11 files' and no longer point at the reads; the files have moved
> since. The finding stands: both test files still read `kFlagHasCommand`, e.g. the rig's `hasCommandBit()`.)*
> ⭐ The true and useful form is the one the initiative already converged on for the
> symmetrical claim in the sibling file: **this is the one line in production code that
> turns the bit into behaviour.** Carried verbatim, flagged.


<!-- header lines 1310-1315 -->
> ⭐ [movement-sim task 50] `kFlagHasCommand` IS READ HERE, and this is its only
> reader. Until this task it was written by step 5 and consulted by nothing, while an
> off-wire `bool` twin carried the decision — the write-only-flag anti-pattern.
> Reading the ON-WIRE bit is what makes the gate survive a correction: the bit is
> restored with the rest of `flags`, so a replayed tick reaches this branch exactly
> when the authority's own tick did.


<!-- header lines 1318-1322 -->
> What `x += v·dt` alone would have produced from last tick's command. Anything
> else in the captured position is the solver: a wall, or another brawler.
> ⚠ `state.velocity` IS last tick's commanded velocity — see the
> commanded-velocity paragraph on `State`. BOTH operands are serialized now,
> which is the whole property this step needed and did not have.


<!-- header lines 1327-1349 -->
```text
THE AUTHORED CONTACT RULE, and the only way contact reaches velocity at all.
It has TWO arms since movement-sim task 57 / ruling #29, chosen by the
ORIENTATION of the contact, and the arms remove DIFFERENT components:

  LANDING (`dot(n, up) >= cosMaxSlope`) — a floor-like face absorbs the
    VERTICAL component and keeps the horizontal. That is the genre's rule
    (landing zeroes vertical, horizontal survives) and it is the fix for the
    user's *"if there is a slope where I landed my brawler goes flying"*: the
    into-contact kill below turns a straight-down 2000 cm/s fall onto a 30°
    face into 1000 cm/s DOWN THE SLOPE, because removing the normal component
    of a vertical vector leaves its whole tangential part behind. An inelastic
    bounce is right for a wall and wrong for the ground.

  WALL / OTHER BRAWLER (everything else) — UNCHANGED: kill the component
    driving into the obstacle, keep the rest. On a wall this is the
    fighting-game corner clamp; on another brawler it means the pushed
    character's into-contact velocity is zeroed for the tick and NO MOMENTUM
    IS TRANSFERRED. Task 54's wall press and `WallPushOutZeroesIntoWallComponent`
    measure this arm and did not move.

⚠ `kWorldUp`, NOT step 2's `up` local: step 6' runs BEFORE step 2, so the local
does not exist yet. Same constant, same ruling #26 (a) axis — and this is the
one place in the file that needs it early.
```


<!-- header lines 1353-1354 -->
> The captured `linearVelocity` — which is where the engine put its contact
> impulse — is not read here or anywhere.


<!-- header lines 1365-1370 -->
> ---- step 0: TELEPORT SEED ----------------------------------------------------------
> The ONLY body write that ignores `sd.drivesBody`: a spawn or respawn must move the
> capsule even on an instance that is not driving it — a `drivesBody = false` peer or LLT
> rig. (In shipped production `drivesBody` is TRUE and the CMC is retired; task 15 flipped
> both together. The earlier wording here said "while the CMC is still driving it", which
> has not been the case since. [movement-sim task 17])


<!-- header lines 1388-1403 -->
> ⭐⭐ THE HOVER TUNING READOUT — movement-sim task 56. The first PIE run after ruling #28
> is a TUNING session, and ω/ζ are provisional. This prints the configured pair, the
> derived gains, and — the one number nobody guesses from continuous intuition — the
> DISCRETE critical damping ratio for the configured ω, so a tuner can see at a glance
> which side of the ringing threshold they are on.
>
> ⚠ `[Warning]` IS DELIBERATE AND IS THE ONLY VISIBILITY KNOB THIS LINE HAS. `OGBLOG_G`
> does not enter `RouteOGMessage`; ogblog's own sink sends every message to
> `LogOGBrawler`, reading only the leading severity token, and
> `Config/DefaultEngine.ini` ships `LogOGBrawler=Warning`. A bare tag would be invisible
> in exactly the session this line exists for.
> ⚠ AND IT IS HERE, IN STEP 0, RATHER THAN IN `StaticData`'s CONSTRUCTOR, FOR AN
> ORDERING REASON: `ASimulationManagerUImpl::m_staticData` is a by-value member, so it is
> constructed with the actor — BEFORE `BeginPlay` installs the ogblog sink. A log from
> there would be swallowed. The teleport seed runs once per spawn and respawn, with the
> sink installed, which is exactly the cadence a tuning readout wants.

> ### R0-13 — FALSE ×2 — a dead tense and a tree-wide quantifier
>
> ⛔ *"Nothing writes the bit yet; task 14 … **will be** its only writer."* Task 14 is
> `Done`. `simulatableBrawler::makeSimPlayerInput` holds the only line in the tree that ORs
> `kInputFlagHoldGuard` into a flags accumulator — `BrawlerInputPackaging.h:59`, inside
> `makeSimPlayerInput` at `:48`.
>
> ⛔ *"this is its ONLY reader"* is false tree-wide: six reads outside this header, four in
> `MakeSimPlayerInputFlagsTest.cpp` and two in `SimulatableBrawlerTest.cpp`.
>
> ⚠ **Task 65's published replacement for this paragraph would ship a NEW false claim.** It
> prescribed quoting `BrawlerInputPackaging.h:153`'s *"THE ONE AND ONLY WRITER of
> `kInputFlagHoldGuard`, anywhere in the tree"* — a sentence task 66 found FALSE and task 74
> DELETED, at a line number that is now past the end of a file rewritten 197 → 128 lines.
> ⭐ **That is the single clearest demonstration in this phase that an R0 correction decays
> exactly like the claim it corrects.** The correction to adopt is: *the bit is READ here,
> the one production line that turns it into behaviour; it is WRITTEN by
> `makeSimPlayerInput`, which holds the only OR in the tree; test rigs assign it directly.*


<!-- header lines 1413-1418 -->
> ---- step 1: GATE -------------------------------------------------------------------
> ⭐ [movement-sim task 51] `kInputFlagHoldGuard` IS READ HERE, and this is its ONLY
> reader — the one line that turns the input byte's bit 0 into behaviour. Nothing writes
> the bit yet; task 14 (`buildPlayerInput` -> `makeSimPlayerInput`) will be its only
> writer. Keep the pair visible: a bit written and never read is exactly the
> `kFlagHasCommand` trap that took task 50 to repair.


<!-- header lines 1422-1426 -->
> COMMITTED STATES own velocity outright — the model is suspended, not blended. ⛔ The
> machine enumerators this reads (`Dashing`, task 31; `Launched`, task 27) DO NOT EXIST
> YET: `DAttackState` is {Attacking, Idle, GuardFlinch, HitFlinch} today. The external
> dependency that will carry them is already declared and already resolved above, so
> those tasks add their enumerator, their branch in step 3 and nothing else here.

⭐⭐ **TASK 27 LANDED, AND IT DID NOT ADD AN ENUMERATOR — 2026-09-12.** The paragraph above
predicted the shape and got it half right. `committed` is now
`machineLaunchesMovement(machineState)`, which is `HitFlinch` **plus** `m_hitReaction ==
Knockback`: the reaction is DATA on the hit, not a fifth machine state. That is a user ruling, and
the cost it avoids is concrete — a fifth `DAttackState` bumps `kDAttackStateCount`, moves the
visualizer initiative’s `kMachineStateCellCount` fence and four `case` sites **in that
initiative’s files**, and leaves `HitFlinch` with no writer at all, since after task 27 every
inbound hit would have entered the new state.
⛔ `DAttackState` is therefore still {Attacking, Idle, GuardFlinch, HitFlinch} and
`kDAttackStateCount` is still `4u`. **Task 31’s `Dashing` is the only enumerator this line is
still waiting for**, and when it arrives `committed` becomes
`machineLaunchesMovement(...) || machine == Dashing`, with its own branch inside step 3’s committed
arm beside the knockback’s assign-or-decay pair. ⛔ There is deliberately NO dead `Dashing` arm
standing there today: `committed` is exactly `machineLaunchesMovement` and the code says so, because
an unreachable branch carrying no comment (the rule forbids one) reads as a bug rather than as a
reservation.

⭐⭐ **TASK 84 LANDED A THIRD BRANCH, AND IT IS NOT IN THE COMMITTED ARM — 2026-09-20.** Step
3’s dispatch is now `committed` → `locked` → `frozen` → the walk models. `locked` is
`machineLocksMovement(machineState)`, i.e. `Attacking`, and while it holds the two tangent-plane
channels are multiplied by `(R-1)/R` — the attack movement lock, derived in §27 below.
⛔ **It was proposed for the committed arm and that would have been a defect**: `committed` also
feeds `detachesFromSupport`, whose arm is keyed on `dot(velocity, up) > 0`, so an uphill slide
would detach and fly. The arm therefore still has exactly ONE tenant (the knockback) and the
paragraph above still holds for task 31’s `Dashing`, which genuinely does own velocity outright.
*(Since og-ground-aligned-attacks task 37 `committed` feeds `classifySupport`'s rows instead (§34). Its separation
row tests `dot(velocity, n)` against the face, so a slide in the face does not detach either way, and the arm
still has one tenant.)*
⚠ **Task 31 interacts with `locked` by construction and with no special case:** a dash-cancelled
into an attack simply leaves dash speed in `state.velocity` on the first `Attacking` tick, and the
ramp starts from whatever it finds. Noted, not handled here.


<!-- header lines 1429-1436 -->
> ---- step 2: ATTACH + CLEARANCE -----------------------------------------------------
> Model-agnostic by construction: the model selector is not named in this step, in step 4
> or in step 5 — only in step 3's dispatch, which is what an acceptance criterion greps for.
>
> ⭐ THE SERVO AXIS, SPELLED ONCE PER TICK. `up` is what this step measures `clearance`
> along, what the walkable test dots against, and what step 5 actuates along — one name,
> one value, user ruling #26 (a). Tasks 20/48 turn this line into a `SupportState` branch;
> today every v1 surface is `Supported` and it is `kWorldUp` unconditionally.


<!-- header lines 1449-1452 -->
> `fraction` is only meaningful when `blocked` (SpatialQueryResult.h's field-validity
> rule), so an unblocked probe has no clearance at all rather than a large one.
> ⭐ `clearance` is a gap measured ALONG `up`, and step 4 closes it ALONG `up`. That the
> two are the same axis is the whole of ruling #26 (a).


<!-- header lines 1456-1460 -->
> ⭐⭐ CLASSIFICATION IS WHERE SUPPORT IS DECIDED — movement-sim task 56 / ruling #28.
> Geometry (`walkable`) is one input; the DETACH GATE is the other, and both are read here so
> that step 4 can be a pure reader. `SupportedSteep` has no producer in v1 — the walkable
> test is a single `cosMaxSlope` comparison, so a face past `maxSlopeAngleDeg` is
> `Unsupported`, not steep-supported. Task 48 adds the second comparison HERE, not in step 4.


<!-- header lines 1465-1467 -->
> `n` is the SURFACE normal and is now used for exactly one thing: building the tangent
> frame (u, v) that step 3's models live in (plus the viz readout below). It is NO LONGER
> the actuation axis — step 5 puts the support term on `up`.


<!-- header lines 1473-1476 -->
> The tag the `[Movement.surface]` line prints. ⚠ THE STRINGS ARE THE ENUMERATOR SPELLINGS,
> not prettier prose: the PIE runbook greps for them, and a readable synonym would make the
> grep and the code disagree. `Floor` / `Airborne` are RETIRED tokens — a runbook or a saved
> log filter still looking for them is looking for a build older than task 56.

> ### R0-14 — FALSE again since 2026-10-03 (no analog walk speed); the parenthesis is also FALSE
>
> ⛔ *"The stick arrives as a WORLD XY direction whose magnitude is the stick deflection
> (`BrawlerInputPackaging.h`: `moveDirectionWorld` is the move stick rotated into camera
> space, so rotation preserves its length)."*
>
> **History, short.** When this was moved, both halves were false: the producer normalised the
> move stick. Brawler-movement-simulation task 63 held the fix for a user feel ruling (analog
> against binary). On 2026-10-02 the user ruled analog for the gamepad (og-brawler-3rdControllerMode
> task 4), and the producer scaled its unit direction by the stick deflection, which made the first
> half true. On 2026-10-03 the user reversed that (ruling #5, task 6): **no analog walk speed**. The
> producer is back to its pre-task-4 form, so the first half is false again.
>
> **Now.** `UOGBrawlerInputCollectionComponent::buildMoveDirectionWorld` returns `(0,0,0)` while the
> routed move stick is shorter than `g_moveStickDeadzone` (0.15f). Otherwise
> `buildMoveDirectionWorldFor` returns `getInputDirectionInCameraSpace`'s result, which normalises
> the input and then rotates it about Z only, so its length is 1. That holds for the gamepad, the
> keyboard and the D-pad alike. ⇒ `stickDeflection` here is **{0, 1}**: a stick at 0.16 and a stick
> at 1.0 both ask for full walk speed.
>
> **The sim is NOT changed.** Its law (`stickDeflection` is the length of the incoming vector, and
> ContinuousAccelBrake targets the stick times `maxWalkSpeed`) can still express analog speed and is
> still correct. With this producer it is constant in effect: the producer decides the length. The
> parenthesis is false either way. `BrawlerInputPackaging.h` only **carries** the field, and the
> producer normalises before it rotates, so the rotation has no deflection to preserve.
>
> The following paragraph, *"projected … with the DEFLECTION PRESERVED and the DIRECTION
> renormalised"*, is therefore again a renormalisation to 1 in effect: on a slope the character
> asks for full walk speed along the slope.


<!-- header lines 1507-1513 -->
> ---- step 3: MODEL DISPATCH ---------------------------------------------------------
> The stick arrives as a WORLD XY direction whose magnitude is the stick deflection
> (BrawlerInputPackaging.h: `moveDirectionWorld` is the move stick rotated into camera
> space, so rotation preserves its length). It is projected into the surface frame with
> the DEFLECTION PRESERVED and the DIRECTION renormalised: on a slope the character
> walks along the slope at the speed the stick asked for, instead of losing speed to the
> cosine of the incline.


<!-- header lines 1525-1530 -->
> ⭐⭐ THE ONE READ OF `state.velocity` PER TICK — movement-sim task 57 / ruling #29.
> Both channels come out of the SAME dual-basis solve, and there is deliberately no second
> plain dot of the velocity against a frame axis anywhere below: `currentUV` here and `vUp` in
> step 4 are the two halves of one decomposition, and they were the two defects R2 named. Read
> `decomposeVelocity`'s comment before touching either. `state.velocity` is not written again
> until step 5, so one solve serves both sites.


<!-- header lines 1537-1538 -->
> Tasks 31 (Dashing: ASSIGN `dashDir · dashSpeed`, replacing momentum) and 27
> (Launched: `moveTowards(currentUV, 0, launchDecel·dt)`) land their branches here.
>
> ⭐ [task 27, 2026-09-12] **TASK 27’S BRANCH IS IN, AND IT IS TWO LINES, NOT ONE.** The decay
> the sentence above names is the `else`; the `if` is the **ASSIGNMENT** on the hit tick,
> `velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed`. Revision 6 / ruling #14(c):
> the sim OWNS the velocity and a hit REPLACES it — there is no impulse and nothing sums. The
> assignment is keyed on `inboundHit.wasHitThisTick`, **not** on the machine’s
> `m_timeInCurrentState == 0`: both consume the same derived signal on the same tick, and keying on
> the hit keeps this sub-simulation independent of how the machine represents its timer. Section 26
> derives the mapping. Task 31’s `Dashing` becomes a third arm of the same inner dispatch.


<!-- header lines 1543-1543 -->
> EXACT, this tick. Not a decay.

> ### R0-15 — UNRESOLVABLE-as-stated — and the real defect is in `Backlog.md`
>
> ⚠ Near the end of the block below: *"Backlog 49 still says \"#2 stays OPEN\"; that entry
> is stale."* Task 65 verdicted this FALSE on a literal grep for `stays OPEN`, which found
> only the retraction. ⛔ That grep missed a **second spelling of its own subject**:
> `Backlog.md` task 49 also contains, 71 lines later, *"**#2 stays fully open and this task
> must not touch it.**"* — lower case, in a surviving Description bullet.
>
> ⇒ **The header's *"still says"* clause is TRUE in substance** (the Backlog entry
> contradicts itself, and a reader sent there finds the assertion this file warns about)
> and wrong to call it a single sentence. ⛔ **The real defect is in `Backlog.md`, not in
> this header** — task 49 states #2 both closed and open. **ROUTED to the lead.**
>
> ⭐ The retraction itself re-derives correctly: 417.25 ÷ 3.3329 = 125.19 ✓.


<!-- header lines 1563-1675 -->
```text
---- step 4: VERTICAL ---------------------------------------------------------------
⭐ MEASURED AND ACTUATED ON THE SAME AXIS, `up`. That is the whole of user ruling #26.

⭐⭐ ONE LAW, NOT TWO — movement-sim TASK 56 / USER RULING #28, 2026-09-07. This step used
to be an `if` with two DIFFERENT KINDS of law in its arms, and the seam between them was a
defect that no amount of tuning either arm could reach:
    Floor:     velocityUp = clamp((rideHeight − clearance) / dt, ±maxSnapSpeed)   // ASSIGNED
                                                                                  // from position
                                                                                  // error; gravity OFF
    Airborne:  velocityUp = velocity·up + gravity·dt                              // INTEGRATED;
                                                                                  // gravity ON
Crossing the boundary handed a velocity PRODUCED BY A POSITION SERVO to a law that treats
its input as MOMENTUM. That single line is the mechanism behind the wall-press LAUNCH half
of task 54 (`impl_notes_seam_54.md` §1.3, the "airborne-retention line") and behind the
user's *"falls start at ~4 m/s instead of from rest"*. Task 54 removed the SPURIOUS
crossings; ruling #28 removes the second law, so there is nothing left to retain.
⛔ THE AIRBORNE-RETENTION RULING IS THEREFORE MOOT, NOT ANSWERED. Do not reopen it: the
line it was about does not exist, and the concept has no name left to attach to.

THE LAW. Gravity is unconditional; the servo is a term ADDED to it while supported:
    a = gravity                                              ← every tick, no branch
    if (support != Unsupported)
        a += clamp(k·(rideHeight − clearance) − c·vUp − gravity, ±hoverMaxAccel)
    velocityUp = max(vUp + a·dt, −terminalFallSpeed)

⭐ THE `− gravity` INSIDE THE CLAMPED TERM IS A FEED-FORWARD, AND IT IS WHAT KEEPS THE
STEADY STATE EXACT. At `clearance == rideHeight` with `vUp == 0` the servo evaluates to
exactly `−gravity`, so `a == 0` and the character neither rises nor sinks — the hover
height is a value, not an approximation. WITHOUT it the spring would have to SAG until
`k·sag == |gravity|` to hold itself up: 0.4631 cm at the shipped k, which every existing
`clearance == rideHeight` pin would have had to be loosened to accept. One subtraction.
⚠ It is INSIDE the clamp deliberately: the clamp then bounds the servo's own authority, and
the total vertical acceleration is `gravity + clamp(…, ±hoverMaxAccel)`. So the honest
per-tick velocity bound is `(hoverMaxAccel + |gravity|)·dt`, not `hoverMaxAccel·dt`, and
`NoVelocityStepAtSupportBoundary` pins that form rather than the tidier wrong one.

⭐ WHAT THE ONE LAW BUYS, AND IT IS A PROPERTY RATHER THAN A TUNING: `velocityUp` is now
CONTINUOUS across the support boundary by construction — nothing is ever assigned, so no
classification flip can introduce a step. Measured on a 10 m fall, worst per-tick
`|Δ velocity·up|`: 483.67 cm/s under this law against the 516.33 cm/s bound, and 972.00
under the dead-beat, which destroyed 972 cm/s of real momentum in one tick on landing.
⚠ AND WHAT IT DOES NOT BUY, RECORDED BECAUSE AN AC PREDICTED OTHERWISE: a walk-off does NOT
start "from rest" under either law, because in both the servo is TRACKING a receding floor
and the character genuinely has that velocity when the probe lets go. On a first-principles
ledge corner the detach speed is −101.61 cm/s under this law and −107.07 under the dead-beat.
What changed is provenance, not magnitude: the retained value is now something the body was
physically doing, not a one-tick position correction reinterpreted as momentum.
`LedgeFallStartsFromRest` says so in the case, and is GREEN in both arms on purpose.

⛔⛔ USER RULING #26 = (a), 2026-09-06 — CORRECT ALONG WORLD-UP. TASK 49.
Architecture §3.3 as revision 6 first wrote it measured `clearance` along the SWEEP
(world −Z) and applied the correction along `n`, the SURFACE normal: a control loop
whose measurement axis and actuation axis disagree. On a slope of angle θ that bought a
one-shot lateral slide while the servo converged, and an over-correction on every tick of
it. ⛔ Task 11 shipped §3.3 VERBATIM and was not at fault; task 49 changed §3.3's own
answer. Both axes are `up`, and ruling #28 did not touch either one.

⭐ MEASURED, NOT ARGUED (task 49 AC-1, 2026-09-06). The magnitude of that effect had been
derived wrong twice, so the PRE-FIX header was driven through
`SimulatableBrawler::integrate` against an analytic 30° plane, seeded 1 cm above ride
height, logging per-tick clearance change against the commanded step:
    clearance gained / clearance commanded   1.154700, 1.154716, 1.154879, 1.154875
                                             (1/cos30° = 1.154701)
    lateral X   0.433010 cm over the whole transient for e = 1 cm (= sinθ·cosθ)
  ⇒ THE PLANE DERIVATION IS THE RIGHT ONE. The probe is cast FROM THE CAPSULE, so a step
    along `n` also moved the probe origin sideways by `s·sinθ` and the ground under it fell
    away by `s·sinθ·tanθ`; the vertical gap gained `s/cosθ` and the servo OVER-corrected.
    ⛔ The superseded `e·tanθ` model — which predicted 0.866× here and a 0.577 cm lateral —
    is DEAD, and so is the 50.86 cm figure it produced.
  ⇒ At the shipped 45° cap the pre-fix figure was 1/cos45° = 1.414: the servo OVERSHOT and
    damped-OSCILLATED, dipping below ride height on alternate ticks, and would have
    DIVERGED past 60°. Under (a) `Δclearance` equals the commanded step at EVERY angle —
    the probe origin no longer moves horizontally — so the oscillation is gone and the 60°
    ceiling is LIFTED rather than tuned around. That is the argument for the ruling, and it
    is why task 49 was a fix and not a re-tune.
  ⚠ THAT MEASUREMENT WAS TAKEN ON THE DEAD-BEAT, whose loop gain was 1 by construction, so
    the RATIOS above are a property of the AXES and not of the law. Ruling #28 changed the
    law and left the axes alone: the servo term is still built from a scalar `clearance`
    error and still composed onto `up` in step 5, so the lateral component is still
    IDENTICALLY zero at every angle. What did move is CLOSURE — the error no longer goes to
    zero in one tick, by design — which is why `HoverOnRampDoesNotDrift` now pins the
    measured spring transient instead of a one-tick closure.
⛔⛔ THIS IS *NOT* TASK 9'S 29 cm, AND NOTHING HERE MAY CLAIM IT IS (task 49, 2026-09-06).
Backlog 49, `impl/design_hover_slope_transient.md` and the comment task 11 shipped all
attribute the spike's `capX.X −9.558624 → −38.920441` to this coupling. That attribution
is FALSIFIED, on the spike's own artefacts, by two independent facts:
  1. THE SPIKE'S SERVO ALREADY ACTUATED ON WORLD-UP. `impl/Spike9Probe.cpp.{final,pre44,
     pre9b}` — all three archived revisions — build the hover velocity as
     `FVector Vel = ZeroVector; ... Vel.Z = Clamp(Err / Dt, ±maxSnapSpeed)`. There is no
     surface frame in the probe: no `u`, no `v`, no `n * velocityN`. The mismatch this
     task removes DID NOT EXIST in the code that produced the 29 cm, so it cannot be its
     cause. This is a structural fact, not a derivation.
  2. `impl/research_spike_9.md` §0.4a ACCOUNTS FOR THE WHOLE 29 cm AS SPAWN
     DEPENETRATION: the capsule was seeded 89.5 cm INSIDE the ramp, `penDepth` falls in
     four equal 22.4395 cm decrements, and Σ push.X over steps 0–3 = −38.920508 against a
     measured −38.920441 — an accounting identity to 7e−5 cm that stops dead the tick the
     penetration reaches zero. Servo motion is COMMANDED, so it lands in `predicted` and
     can never appear in `pushOut`; a push-out that explains 100 % of the lateral travel
     leaves the servo nothing to have contributed.
  Also: the motion took THREE ticks (50 ms), not "the first second".
⇒ THE DEFECT BELOW IS REAL AND MEASURED, BUT ITS MAGNITUDE IN A SHIPPING SCENARIO IS
  UNMEASURED — no run has ever exercised this servo on a slope. ⛔ The vertical errors
  inferred from the 29 cm (67.81 cm here, 50.86 under the dead model) are artefacts of an
  attribution this file does not accept; do not repeat either figure.
⛔ Task 9 observation #2 (127-step convergence) is not this either, and it is no longer
open: `research_spike_9.md` §0.1(1)/§0.2 WITHDREW it — `277 − 150` is distance-to-wall
divided by scripted speed (417.25 cm ÷ 3.3329 cm/step = 125.2), which is why all three
peers agree on it. Backlog 49 still says "#2 stays OPEN"; that entry is stale.

⚠ WHAT (a) COSTS, RECORDED SO TASKS 20/48 INHERIT IT. `rideHeight` and the probe reach
are both measured along `up`, so the PERPENDICULAR gap is `rideHeight·cosθ` — 7.07 cm at
the 45° cap, 2.59 cm at 75°. ⭐ That is NOT a regression from this task: the measurement
axis was already vertical, so the steady-state perpendicular gap is identical before and
```


<!-- header lines 1680-1688 -->
> The vertical channel of step 3's dual-basis solve. ⛔ NOT a plain dot of the velocity
> against `up`, which is what this line was until task 57: on a slope that reads the WALK's own
> vertical component as a fall and damps it (R2's mirror — walking a 30° face at 600 cm/s
> asked for 17 112 cm/s² of lift). See `decomposeVelocity`.
> ⚠ THE ACCEPTANCE GREP FOR THIS TASK is "no plain dot of `state.velocity` against `u`, `v`
> or `up` remains in this header". It returns ZERO. The two `glm::dot(state.velocity, ...)`
> calls that DO remain are both in step 6', are both authored single-component removals
> against a unit direction (`kWorldUp` and the contact normal), and are not a basis
> decomposition — the note at the branch says so at the site.


<!-- header lines 1691-1697 -->
> AUTHORED gravity (ruling #16 a) — per character, so fall speed is a design knob rather
> than a world constant. `sd.gravity` is a SIGNED SCALAR (−980), i.e. §3.3's
> `dot(g_vec, up)` with `g_vec = (0, 0, gravity)` already collapsed; widening it to a vector
> would be a `StaticData` change, which is an addition and not a hoist.
> ⭐ UNCONDITIONAL. There is deliberately no branch here that could turn it off, which is what
> `GravityRunsWhileSupported` measures by starving `hoverMaxAccel` below `|gravity|` and
> watching a SUPPORTED character sink at exactly `(gravity + hoverMaxAccel)·dt`.

> ### R0-16 — a circular citation, re-derived so it stops depending on itself
>
> ⚠ The `+/-380 cm/s per tick` in the forbidden-variants fence — now guard **G-10** — is
> quoted from `design_ledge_fall_and_landing.md` §1b, which states the same figure and does
> not derive it either. Re-derived here: it is `c·dt` times the chase speed,
> `2ζω · dt · 400` = `57.04 × (1/60) × 400` = **380.27** (cm/s, as the fence was written; with the chase speed
> in metres, 4 m/s, the same term is 3.8027 m/s per tick). ⭐ Name the TERM and the number
> survives a gain change instead of silently lying about one.


<!-- header lines 1702-1717 -->
```text
⭐⭐ THE SERVO IS ONE-SIDED — movement-sim TASK 57 / USER RULING #29, 2026-09-07.

    SUPPORTED MEANS HELD UP, NEVER PULLED DOWN.

At or below ride height (`e >= 0`) this is task 56's full spring-damper, unchanged and
still poison-tested. ABOVE ride height it is a BOUNDED SPRING PULL AND NOTHING ELSE,
capped at `sd.hoverPullDownAccel` — 0 in the shipped data, so the vertical law up there
is gravity and nothing else, which is what the user asked for.

⛔⛔ WHY THE OLD TWO-SIDED ARM WAS THE LEDGE DEFECT (R1). Its pull was bounded by
ACCELERATION (`hoverMaxAccel` = 30 g), not by speed, so a clearance that JUMPS into the
band cost -516.333 cm/s in ONE tick against gravity's -16.333: 31x too fast, MEASURED on
the task-56 tree at clearance 45 (`AboveRideHeightIsGravityOnly`). The dead-beat this
spring replaced capped the CHASE SPEED at 400 cm/s and so never had this failure mode;
that difference is what made "the spring is about as fast as the dead-beat" wrong.

```


<!-- header lines 1726-1728 -->
>
> ⚠ `hoverMaxAccel` still bounds BOTH arms and stays 30 000: catching a landing needs it.
> The pull-down knob bounds only the spring term on the upper arm.


<!-- header lines 1732-1735 -->
>
> `k` and `c` are `ω²` and `2ζω`, derived once in `StaticData`'s constructor; read the ζ
> comment there before changing either — ζ = 1 is NOT critical damping for this discrete
> recurrence and rings visibly.


<!-- header lines 1743-1745 -->
> ⚠ THE TERMINAL CLAMP IS ON THE WHOLE LAW NOW, not on an airborne branch. It never binds
> while supported at the shipped gains — the servo turns a fall around long before 2000 cm/s —
> and leaving it unconditional is what keeps this step free of a second `if` on `support`.


<!-- header lines 1748-1753 -->
> ---- step 5: WRITE ------------------------------------------------------------------
> Tangential motion stays in the SURFACE frame (u, v); support and gravity go on `up`, the
> axis step 2 measured `clearance` along. ⭐ On a uniform slope the tangential term moves the
> body ALONG the plane, so the vertical gap does not change, the servo sees ZERO error while
> walking and therefore never contributes a horizontal component of its own. That property
> is exactly what ruling #26 (a) buys.


<!-- header lines 1756-1764 -->
> Remembered for NEXT tick's step 6'. ⚠ BOTH of these are SERIALIZED — `positionCmd`
> since task 50, `flags` since task 11 — and that is the point: step 6' reads exactly
> what a correction restores, so a replayed tick reproduces the live one.
>
> ⭐ [movement-sim task 50] THERE IS ONE COMMAND MARKER NOW, and it is the wire bit.
> Task 11's off-wire `bool` twin is DELETED, and with it the documented asymmetry in
> which the two markers deliberately diverged for one replayed tick. `firstResimStep` no
> longer touches this sub-simulation's state at all (architecture §3.5 always described it
> as a no-op; it is one again), so there is no tick on which the bit and the behaviour

---

## 24. `SerializableFields` and the wire field order


<!-- header lines 1782-1782 -->
> SerializableFields specializations for brawlerMovementSimulation types.


<!-- header lines 1784-1784 -->
> THE TELEPORT SEED, on the wire: a respawn has to replay identically on both peers.


<!-- header lines 1799-1807 -->
```text

⭐ [movement-sim task 50] `positionCmd` APPENDED, +12 B (slice 49 → 61 B, composite
309 → 321 B). It is STATE, not scratch — the one input step 6' cannot re-derive — and the
reason is written out at its declaration. Appending leaves every preceding field at its
existing offset, so `correctionStateBuffer::kWireFormatVersion` is NOT bumped: the size
moved, the layout of what came before did not. (Same call task 11 made when it grew this
slice 24 → 49 B.) ⛔ Task 11's second, off-wire velocity copy and its off-wire `bool`
command marker are GONE, not merely still absent: the first held exactly `velocity`, which
is already the second entry below, and the second was a twin of `flags`' bit 3.
```


<!-- header lines 1835-1852 -->
> ⭐⭐ [movement-sim task 76] THE WIRE FIELD ORDER, PINNED BY NAME — the successor to the
> APPEND-ONLY sentences above, which until now were guarded by PROSE ALONE.

⭐ **[task 84, 2026-09-20] THE COMPOSITE GREW AND THIS SLICE DID NOT.** `dAttackMachineSimulation
::State` gained `uint32_t m_attackEndTick` (slice 21 → 25 B, composite 335 → 339 B,
`kStateWireBytes` 346 → 350 B, correction buffer 347/384 with 37 B of headroom). The attack slide
that field drives is a step-3 law here, but the CARRIER is the machine’s, because the machine is
the sub-simulation that both owns the `Idle → Attacking` transition and holds the authored
sequence table. `brawlerMovementSimulation::State` is untouched at 61 B and six members, and the
`StateHasExactlySixMembers` assertion above stands — that was the deciding argument for the
recomputed form of the law (§27): the fixed form would have needed a seventh member here.
⛔ `correctionStateBuffer::kWireFormatVersion` is NOT bumped: this is the append that grows an
EXISTING slice, the case task 27’s note names as the one that declines. Zero input bytes.
>
> ⛔ WHY NOT A SIZE OR A COUNT. `velocity` and `positionCmd` are BOTH 12 B. Transposing them
> leaves `syncSize<State>() == 61`, `FCompositeWireSize<simulatableBrawler::State> == 321`,
> `kStateWireBytes == 332`, the 82 B ring entry stride and every other pin in the tree GREEN —
> and it desynchronises every peer that has not shipped the same edit. Measured: under that one
> edit all eleven existing wire pins compile clean, a same-peer serialize/deserialize round trip
> still agrees with itself, and the same 61 bytes decode with the two vectors swapped.
> ⛔ WHY NOT `offsetof`. An offset chain measures the STRUCT, and the struct's own member order
> is wire-NEUTRAL — the tuple below decides the byte order. So it would both miss the edit that
> matters and fire on one that does not.
> ⭐ WHY THIS WORKS. `SIM_MEMBER(C, m)` expands to `MemberFieldDesc<&C::m>{}`, so each entry's
> member pointer is a NON-TYPE TEMPLATE PARAMETER and the whole sequence is carried in the
> tuple's TYPE. Comparing that type is equivalent to the prohibition: a reorder, an insertion
> and a removal all MOVE it, and a rename does not compile at all — the member pointer below
> stops naming a member. APPENDING a field is the one edit that still needs a human: it moves
> this assertion too, and that is the deliberate, versioned act the comment demands.

---

## 25. The trailing labels — the loss you feel on every line

⛔ **v2 §1.4 item 5, user ruling 2026-09-11: a trailing one-line label GOES, and the loss is
ACCEPTED rather than overlooked.** Its content belongs here, and the reader follows the pointer.

⚠ **This header had nineteen of them.** The pilot that established the rule had five, 15 to 58
characters, and called them *"the single most-felt loss in the converted file"*. Nineteen is the
largest set in the phase so far, and it includes the whole `ON WIRE n` column of `State` and
`InitialConditions` — the only place the wire size of each field was written down beside the
field. They cannot go stale in any interesting way, and they still go.

| declaration, after conversion | the label it carried |
|---|---|
| `#include "OGSimulation/OGTypes.h"` | `// for BodyId` |
| `#include "glm/common.hpp"` | `// glm::abs / glm::clamp / glm::max` |
| `#include "glm/geometric.hpp"` | `// glm::dot / glm::cross / glm::normalize / glm::length` |
| `#include "glm/trigonometric.hpp"` | `// glm::cos / glm::radians -- SETUP time only, never per tick` |
| `#include "glm/exponential.hpp"` | `// glm::sqrt` |
| `#include "OGSimulation/SpatialQueryResult.h"` | `// SweepHit -- the attachment probe's result` |
| `#include "OGSimulation/OGAssert.h"` | `// OG_CHECK -- the model-dispatch default arm` |
| `float terminalFallSpeed;` | `// POSITIVE magnitude; the fall speed is clamped to −this` |
| `{` | `// shapes` |
| `glm::vec3 surfaceNormal{kWorldUp};` | `` // the frame's `n` this tick — fed from the one axis `` |
| `glm::vec3 lastProbePoint{0.f};` | `// where the attachment sweep hit, world` |
| `glm::vec3 lastPushOut{0.f};` | `// the engine's positional correction, step 6'` |
| `uint8_t   lastSupportState = 0;` | `// SupportState, as a byte for the viz layer` |
| `inline constexpr uint8_t kInputFlagHoldGuard = 1u << 0;` | `// bit 0` |
| `uint32_t  teleportPending = 0;` | `// ON WIRE 4` |
| `glm::vec3 teleportPos{0.f};` | `// ON WIRE 12 — wire size 16 B` |
| `inline constexpr uint8_t kFlagSupportMask  = 0x06u;` | `// bits 1-2` |
| `glm::vec2 uv;` | `` // tangential, in the surface frame (u, v) — step 3's `currentUV` `` |
| `float     up;` | `` // vertical, along the servo's axis     — step 4's `vUp` `` |

⛔ **R0-17 — the first row was FALSE, and it is repaired by deletion.** `// for BodyId` on the
`OGTypes.h` include is wrong: that header does not define `BodyId`, and `BrawlerCharacterBindings-guards.md`
closes the stronger claim with a pickaxe over the whole file history —
`git log -S BodyId -- OGSimulation/OGTypes.h` returns **zero**, so it has **never** been true.
`BodyId` is declared in `OGSimulation/BodyId.h`. ⭐ The label is recorded above for the record
and is **not** reinstated; this is the one R0 finding in this file that the v2 conversion repairs
simply by removing the sentence. ⚠ `BrawlerCharacterBindings-guards.md:79` cites this exact line
as `BrawlerMovementSimulation.h:4` — **routed, not fixed**.

⭐ **The wire sizes those labels carried are not lost as FACTS** — `syncSize<State>() == 61u`
and `syncSize<InitialConditions>() == 16u` are pinned in `SimulatableBrawlerTest.cpp`, and task
76's three `SerializableFields` assertions pin the ORDER by name. What is lost is the reading of
them *at the field*, which is where a person adding a field looks.

---

## 26. The knockback assignment’s tangent-frame mapping — task 27 (retired by og-ground-aligned-attacks task 37)

⭐ **The mapping below is deleted** (∴D-06, retired: see its entry at the end of this section). Everything after this
paragraph is task 27's record.

`velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed;`

**Where the formula comes from, and why the obvious alternative is wrong.**

`hitDirectionXY` is a **unit vector in the planar chart** ((forward, right), the Z-up frame's world XY,
when this was written; (forward, left) since og-sim-metres-yup task 26), resolved
once by `brawlerHitRouting::System` as `normalise(XY(target position − attacker position))`. *(⚠ R0, og-netcode-v2-field-defects task 29:
that was task 27's rule. Since movement-sim task 83 a melee knockback's direction is the swing
tangent's XY, with this rule as the fallback, and a projectile's is its `spawnDir`
(`BrawlerHitRoutingSystem-rationale.md` §4). It is still a unit vector in the chart, which is all this
derivation uses.)* `velocityUV` is a pair of **tangent-frame
channels**, `(a, b)` such that step 5 composes `a·u + b·v + c·up`. Writing `d.x` into `a` and
`d.y` into `b` therefore looks like a type error, and it is not — it is exact, and it is exact
because of what `buildTangentFrame` produces.

For any normal `n` this step can reach, `|n.x| < 0.9`: a walkable hit satisfies
`dot(n, up) >= cosMaxSlope` = cos 45°, so `|n.x| <= 0.707`, and a non-walkable tick uses
`n = kWorldUp`, where `n.x = 0`. So the reference vector is always `(1, 0, 0)` and

```
u = normalise( (1,0,0) − n·dot((1,0,0), n) )      ⇒  u = (cos θ, 0, sin θ)  for n = (−sin θ, 0, cos θ)
v = cross(n, u)                                    ⇒  v = (0, 1, 0)
```

> ### R0-28 — FALSE for a normal with a sideways part: "`u`’s horizontal projection is X-aligned by construction and `v` is horizontal"
>
> The derivation below assumes `n = (−sin θ, 0, cos θ)`, a face that leans along the forward axis. That is flat
> ground and every face that leans toward ±Z (UE X) in today's frame, and there the paragraph holds. A walkable
> normal with a sideways part is also reachable (`|planar(n).x| < 0.9` holds for it, so the reference is still
> `kForward`), and then `u` stays horizontal while `v = kChartOrientation * cross(n, u)` gains a vertical part: for
> n = (0.5, 0.866, 0), a 30° face rising along X (UE Y), `u = (0, 0, −1)` and `dot(v, up) = 0.5` (R0-27 in §19, the
> same premise). So the map from the chart to `(a, b)` is not "the identity on components" of a horizontal pair
> there, and `u`/`v` are not `kForward`/the chart's second axis. **The conclusion still holds:** `u` and `v` are
> orthonormal and both lie in the face, so `a·u + b·v` keeps the shove's magnitude along the face, which is all the
> requirement below ("a shove on a slope stays on the slope") uses. Found by og-ground-aligned-attacks task 30 (its
> open point 1), recorded by task 34. Since task 34, `SupportLaw::V2` never takes this line on a hit tick (§34): it
> decomposes the written 3-D velocity instead.

**`u`’s horizontal projection is X-aligned by construction and `v` is horizontal.** The map from
world XY to `(a, b)` is therefore the identity on components, and because `u` and `v` are
orthonormal the MAGNITUDE is preserved exactly: a 20 m/s shove is 20 m/s along the face.

**In the frame's own terms (og-sim-metres-yup task 5a).** The derivation above is written in the
Z-up frame (the shipped frame until og-sim-metres-yup task 19), where forward is `+X`, right is `+Y`
and `kChartOrientation` is `+1`. The code says the
same thing frame-independently: the reference is `kForward` (unless `|planar(n).x| >= 0.9`), and
`v = kChartOrientation * cross(n, u)`, so on any reachable normal `u` projects onto `kForward` and `v`
is the chart's second axis: `kRight` under chart P, `kLeft` since og-sim-metres-yup task 26.
`hitDirectionXY` is a chart value in the same chart. Its two components therefore land on the channels
that compose back along the chart's two axes, and the identity map holds in every frame and chart.
Without the factor, the Y-up frame's `cross(n, u)` under chart P was `-kRight`, and every knockback
would have left mirrored across the forward axis. That is the case the deferred probe P-5a of og-sim-metres-yup
checks.

⛔ **THE REFLEX EDIT THIS TAG EXISTS TO STOP** is the projection,
`glm::vec2(dot(dWorld, u), dot(dWorld, v))`. It is the spelling every other world→frame
conversion in this file uses, it is dimensionally innocent, and it is WRONG here: `dot(dWorld, u)`
is `d.x·cos θ`, so on a 30° face the character launches at 17.32 m/s instead of 20 and
covers 3.75 m instead of the user’s 5. ⭐ **It is also invisible on flat ground**, where
`θ = 0` and the two spellings agree exactly — which is why the case that discriminates them is
`BrawlerMovement.SlopeKnockbackStaysOnSlope` and not any of the seven flat-ground knockback cases.

**What it buys, stated as the user’s requirement:** a shove on a slope stays on the slope. The
launched velocity is perpendicular to `n`, its horizontal speed is `knockbackSpeed · cos θ`, and
the character covers five metres **along the face** rather than five metres of ground.

⚠ **THE STICK IS PROJECTED, AND THAT IS NOT AN INCONSISTENCY.** Step 3’s stick handling a few
lines above genuinely does `glm::vec2(dot(stickWorld, u), dot(stickWorld, v))` and then rescales to
the stick deflection. The two are different questions: the stick asks *which way along the surface
is the player pointing*, which is a projection followed by a renormalisation; the hit asks *which
way was the character shoved*, which is already a direction in the surface’s own horizontal
plane. Reading one as a precedent for the other is the mistake.

**Decay, the other half of the arm:** `moveTowards(currentUV, glm::vec2(0.f), sd.launchDecel * dt)`
— the Smash law, constant deceleration to rest, and `moveTowards` returns the TARGET once the
remaining distance is inside one step, so the slide lands on exactly 0.000000 instead of
overshooting into a backwards crawl. `v² / (2a)` = 5 m at the shipped pair. ⭐ The step
count is a FLOAT fact, not an arithmetic one: `40·(1/60)` is `0.6666667f` (`0x3f2aaaab`, rounded up), and
thirty float steps from 20 leave about 6e-7 m/s behind (emulated in float32; in centimetres it was
`4000·(1/60)` = 66.666664 and 8e-5 cm/s), so the thirty-FIRST takes it to zero.
`BrawlerMovement.LaunchedDecelsAtLaunchDecel` pins that (tick 32), and asserts the closed form against the
shipped constants rather than restating 5.

### ∴D-06 — RETIRED by og-ground-aligned-attacks task 37 (2026-11-05): the expression it derived is deleted

**Was:** the derivation tag on step 3's hit-tick branch, whose Legacy line was
`velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed;`. It pointed at this section: why the chart
direction maps onto the tangent channels component-wise, and why the projection spelling is wrong.

**Why it retired.** Its expression is gone. Since og-ground-aligned-attacks task 34 the support law v2 took
`velocityUV = currentUV` on every hit tick (§34), and task 36 made that law the default. The Legacy law was the
line's last user, and task 37 deleted it with that law. Step 3's hit tick is now `velocityUV = currentUV`: the
decomposition of the velocity the write left, an expression a reader can change with only the identifiers in
front of them, so it carries no tag. **Successor:** §34 (the write) and §19 (the decomposition).

### 26.1 The hit-tick write of the attacker's direction (og-ground-aligned-attacks task 9b)

⭐ **[og-ground-aligned-attacks task 37] The flat rule is retired.** The predicate this subsection describes
(`writesKnockbackVelocity(inboundHit, face, walkable, committed)`, with the flat rule at guard G-27) and the
`hitDirectionXY` line it kept are deleted with the Legacy support law, and G-27 is retired. Since task 34 the
shipped law's write had no flat rule (§34), and task 36 made that law the default and re-recorded both goldens
under it. The flat rule existed to keep a flat-ground horizontal hit on the pre-initiative expressions verbatim
until the flip; after task 36 nothing was left for it to preserve. The write is now
`writesKnockbackVelocity(inboundHit, committed)`: every committed hit tick with a non-zero direction writes
`d · knockbackSpeed`, and every hit tick takes `velocityUV = currentUV`. The record below is task 9b's, and the
cases it measured were deleted with the law (§37).

User ruling OQ14, 2026-10-28: *"What if we just write the attackers tangent to the knockback and then let
the current model do its thing?"* — "Yes". Since the flip (task 10; task 14 retired the knob it
switched) the hit tick writes the attacker's 3-D direction into the velocity and the
existing law resolves it. There is no launch or slide branch.

**The write.** Step 2 computes `face` (§30), then
`writesKnockbackVelocity(inboundHit, face, walkable, committed)`, and if it holds sets
`state.velocity = inboundHit.hitDirectionWorld * inboundHit.knockbackSpeed;`. `hitDirectionWorld` is the
routing's `d` (`BrawlerHitRoutingSystem-rationale.md` §4.4: the normalised swing tangent, an authored
row direction, or the shot's `spawnDir`). The predicate is false, and the tick is today's, when the tick
is not a committed hit tick (`committed && inboundHit.wasHitThisTick`), when `d` is
zero, or under the **flat rule**: `g` (the face when `walkable`, else `simFrame::kUp`) is flat and
`simFrame::height(d) == 0.f`. A flat attacker's tangent and every `spawnDir` from flat ground are exactly
horizontal (design §3.6), so a hit on flat ground from flat ground runs today's expressions verbatim.

**Its position** is before the support classification, so the hit tick's own lift test (§30, guard
G-07), tangent frame and channel decomposition all see the written velocity. Guard G-26 holds the
position, and guard G-27 holds the flat rule at the predicate's last `return`.

**The channel branch.** On a written tick step 3 takes `velocityUV = currentUV`, the decomposition of
the written velocity, instead of the line above. Every other hit tick, and every flat-rule tick, keeps
`velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed;`. The vertical channel runs today's
law unchanged: on a written tick its input is the written velocity's up channel.

**What the law then does.** Measured on the analytic 30° plane of `GroundAlignedKnockbackTest.cpp`, the
target on the hover (`rideHeight`):

- **`d` in the target's ground (same face).** The lift test does not detach, the up channel is rounding,
  and the target slides along `d` at the knockback speed: over 24 in-plane directions the hit-tick
  velocity is within 1.96e-7 rad of `d` and its speed within 3.8e-6 m/s of 20, `Supported` on the hit
  tick and the next (`GroundAlignedKnockback.InPlaneTangentSlidesAlongTheFace`).
- **`d` leaving the ground (a convex crease: a plateau attacker's horizontal tangent).** The lift test
  detaches on the hit tick, so no servo acts and, with task 13's world-horizontal frame, gravity alone
  changes the vertical velocity: over five headings it is `height(d) · speed + gravity · dt` exactly,
  and the horizontal speed is the knockback speed
  (`GroundAlignedKnockback.DirectionLeavingTheFaceLaunchesOnTheHitTick`). A flat target hit by a 30°
  up-slope tangent detaches on the hit tick the same way, through the flat arm
  (`GroundAlignedKnockback.FlatTargetHitByASlopeTangentLaunches`).
- **`d` entering the ground (a concave crease: a horizontal tangent 30° into the face).** The target
  stays `Supported` on the hit tick. This outcome is emergent, not designed, and only printed by
  `GroundAlignedKnockback.DirectionIntoTheFaceStaysSupported`. The decomposition is oblique (`decomposeVelocity`:
  `u` along the face, `up` world-vertical), so a horizontal `d · speed` lands as `speed / cos 30°` =
  23.09 m/s on `u` and `−speed · tan 30°` = −11.55 m/s in the up channel. The servo then adds at most
  `hoverMaxAccel · dt` (4.84 m/s net of gravity) per tick: the hit tick ends at 20.58 m/s, of which
  19.74 m/s lies along the face. Over the 30-tick lockout the rig printed:
  - with no floor (servo only): 7.38 m along the face, the target dips to −0.043 m clearance
    (`rideHeight` 0.1), and from tick 4 the lift test detaches it (derived: the servo's recovery drives the
    up channel above the 2° margin), unsupported for 27 ticks;
  - with an emulated floor contact (the capsule pushed out along the plane normal, step 6′ live):
    6.27 m along the face, minimum clearance −1.7e-7 m, unsupported for 6 ticks from tick 25.

  Neither rig has Jolt's contact. Task 10's slope golden measures the real travel and minimum
  clearance, and G3 judges it; an explicit slide is the follow-up if the dip shows.

**The overhead (seq 4).** Its damaging arc is a downswing, and its tangent `(0, −cos θ, −sin θ)` in the
swing frame points down by at least cos 45°. Rotated into an attacker on flat ground or on the 30° face,
for 8 headings and θ from 2π/8 to −1.5π/8, and applied to a target on either ground, it never detaches:
480 hits, 0 detaches, the highest direction height −0.2588 (`GroundAlignedKnockback.OverheadDownswingNeverDetaches`).
Its row stays `{ HitReactionKind::Stun, 0.f, 0.72f }`, so in the shipped data no overhead knockback is
written at all. A launching overhead is an authored row direction (`HitDirectionSource::AuthoredInSwingFrame`,
og-ground-aligned-attacks task 9), not a change here.

**A horizontal direction on flat ground.** A flat target hit by a horizontal `d` (both flat planes, five
pre-hit states including an airborne one, three directions) is bit-equal for 40 ticks to the same rig with
`hitDirectionWorld` zeroed (`GroundAlignedKnockback.FlatTargetWithAHorizontalDirectionKeepsTodaysBits`):
the flat rule and the zero-`d` return give the same tick. Until task 14 the reference was the knob-off
`integrate`, whose predicate returned `false` before it read anything; on flat ground a zero `d` takes the
same `false` and every later expression is the same, so the reference computes what the knob-off run did.
A knob-only case (the knob-off `integrate` ignores the field on the 30° plane too) went with the knob.

---

## 27. The attack slide’s ratio form — task 84 ∴D-07

`velocityUV = currentUV * (static_cast<float>(remaining - 1u) / static_cast<float>(remaining));`

**The user’s requirement, verbatim:** *“when a character attacks, their movement direction is
locked and they continue in the direction they were moving, but the movement slows down during the
attack and comes to a full stop exactly the tick where the attack ends.”*

**What `remaining` counts.** `R` is the number of `Attacking` ticks left **including this one**,
read from the machine’s `m_attackEndTick`, which is the first tick the machine will be `Idle`.
Integration order decides both halves of that sentence: `SimulatableBrawler::integrate` runs
machine → guard → projectile → radial → ring-out → **movement last**, so on the tick `T` the
machine enters `Attacking` movement already sees `Attacking`, and on the tick `E` the radial
deactivates the machine — which ran before it — is still `Attacking` and exits on `E + 1`.
Movement therefore sees `Attacking` on `T … E` inclusive, the weapon returns to its idle pose on
`E`, **`E` is “the tick where the attack ends”**, and the end tick is `E + 1`.

**The derivation.** With nothing else changing, applying `(R-1)/R` each tick telescopes:

```
R on the m-th attacking tick = N - m + 1          (N ticks in the swing)
v_m = v_(m-1) * (R-1)/R = v_0 * (N - m) / N
```

which is the linear ramp `v_k = v0 * (N-k)/N` — the same displacement as a constant deceleration
of `v0 / (N * dt)`, i.e. the obvious closed form, reached without storing either operand.

**⭐⭐ Why RECOMPUTED every tick and never captured at entry**, which is the row that decided it:

* **State.** A captured form needs `v0` or a deceleration stored somewhere. On this side that is a
  **seventh member of `brawlerMovementSimulation::State`**, which
  `static_assert(detail::StateHasExactlySixMembers<State>)` forbids by ruling; on the machine side
  it is a second wire field carrying one number. The recomputed form stores nothing: it is a pure
  function of `(velocity, tick, m_attackEndTick)`, all three already on the wire, so a correction
  adopted mid-slide **replays bit-identically** — rev-6’s rule that convergence never depends on
  off-wire memory.
* **A MOVED end.** When the end tick is rewritten — the `Attacking → Attacking` chain, and task
  31’s dash-cancel — this form re-targets on the next tick and the stop is still exact. A
  captured form ramps to the OLD end and sits at zero, or overshoots the new one.
* **EXACTNESS.** At `R = 1` the factor is `float(0) / float(1)`, which is `0.f` exactly, and
  `currentUV * 0.f` is a pair of signed zeroes. `v0 - N * (v0 / N)` is **not** `0` in float and
  would need a clamp, which is a second rule to get wrong.
* **DIRECTION.** The factor is a SCALAR, so the tangent-plane direction survives every tick to
  rounding and the stick is never read. That is the whole of “their movement direction is
  locked”: nothing in the branch can turn it.

**`moveTowards` IS THE REFLEX SPELLING HERE** — the `committed` arm two lines above uses it —
**and the honest answer is that it is not WRONG, it is UNNECESSARY.**
⚠⚠ **MEASURED, 2026-09-20, and it refutes the reason this task was given for rejecting it.** The
design said `moveTowards(currentUV, glm::vec2(0.f), |currentUV| / R)` would miss zero by an ulp on
the last step. It does not: poisoned into this exact site and built, **all nine of task 84’s cases
pass unchanged, 182 assertions**, including the `== glm::vec3(0.f)` row with no `Approx` and the
direction-lock row. `moveTowards` returns `target` whenever `distanceSq <= maxDelta * maxDelta`, and
with `maxDelta = |currentUV|` on the last tick that test is the `sqrt` round-trip
`d <= (sqrt(d))²`, which on this toolchain and these values rounds the safe way. ⛔ **Do not
re-derive the claim that it misses; it was tried.**

What survives the measurement, and it is why the ratio form is the one that ships:

* **It has no comparison and no `sqrt`.** `moveTowards` computes `glm::length(currentUV)` every tick
  and then reaches zero through an inequality between a squared length and a squared root of that
  same length. The ratio reaches zero by multiplying by a float that is exactly `0.f`, which is an
  identity rather than a rounding outcome — the exactness the user asked for does not depend on a
  toolchain agreeing with itself about `sqrt`.
* **`maxDelta` would have to be spelled `|currentUV| / R`**, i.e. the caller re-derives the speed
  the ratio never needs to name, and the two forms then agree only because that expression happens
  to reconstruct the same step. A later reader tuning `maxDelta` — to `sd.brakingDeceleration * dt`,
  say, which is what every other `moveTowards` in this file passes — silently loses the exact stop
  and the ramp with it.
⚠ Neither bullet is a measured divergence. They are statements about the mechanism, and they are
labelled as such because the measured divergence is **none**.

**⛔ The two tangent-plane channels ONLY.** The vertical channel, gravity and the `Supported`
servo run afterwards unchanged, so a slide off a ledge becomes an ordinary fall with the same horizontal
ramp. **That is intended** — user ruling, 2026-09-20: the slide may carry a character over the
ring-out edge, there is no edge gate, no speed gate and no `Unsupported` refusal. The sim has no
notion of an edge, an attack is a commitment, and being hit at the edge is already lethal.

**The guard on the operand next to this one is `⛔G-24`**, and the two are deliberately separate
tags on separate expressions: this section derives the ratio, that entry forbids unguarding the
subtraction that feeds it.

**Pinned by** `BrawlerMovement.AttackSlideStopsExactlyOnTheLastAttackingTick` (the ramp to the
digit on every tick, and `== glm::vec3(0.f)` with no `Approx` on the last),
`AttackSlideDirectionIsLockedAgainstTheStick` (stick held exactly reversed),
`AttackSlideRetargetsWhenTheEndTickMoves` and, for the number the ratio depends on,
`DAttack.Integrate3.AttackEndTickMatchesTheFirstIdleTick`.

---

## 28. The ground probe, extracted — `probeGround` (og-ground-aligned-attacks task 4)

Step 2's attachment probe is a function of its own since og-ground-aligned-attacks task 4:
`probeGround(const State&, const StaticData&, const RuntimeBindings&, Q& query)` returns a `GroundProbe`
`{hit, clearance, walkable}`. Its body is step 2's lines **moved verbatim and in order**, from
`const float probeLength = sd.rideHeight + sd.snapDistance;` through the `walkable` test, with the
`const glm::vec3 up = kWorldUp;` seed repeated at its top so `-up * probeLength` and the walkable dot are
the same expressions as before. `integrate` calls it where those lines stood and binds the three results
to the names step 2 always used (`probe`, `clearance`, `walkable`), so every later reader in the step is
unchanged; step 2 keeps its own `up` local for the support classification and the tangent frame. Since
og-ground-aligned-attacks task 32 `integrate` calls it through `estimateSupport` and reads its results as
`ground.probe` (§32).

**Why extract it.** The probe gains a second caller, the attack ground sample (§29), which must issue
**the same sweep**: the same volume (`bindings.queryVolumeIds[0]`), the same pose (the body position in
`State`, which rollback restores), the same delta (straight down `rideHeight + snapDistance`), and so the
same static-world filter that volume carries. One function makes "the same sweep" a fact of the code
rather than a second copy to keep in step.

**What it does not change.** The movement still makes exactly one sweep per tick, the same call as
before (`BrawlerGroundSample.ProbeGroundIssuesTheMovementSweep` compares the call `probeGround` records
with the one `integrate` records; `BrawlerGroundSample.MovementIntegrateSweepsOncePerTick` counts them).
The function sits inside the header's `OGSIM_OPTIMIZE_OFF` pair, like `integrate`. None of the moved
lines carried a `⛔G` or `∴D` tag (checked against the pre-task header), so no tag moved.

---

## 29. The attack ground sample — `sampleAttackGround` (og-ground-aligned-attacks task 4)

⭐ **[og-ground-aligned-attacks task 37]** Today the sample is `estimateSupport(...).faceNormal` (guard G-25), the
support law v2's estimate (§33): the exact face on a face contact, and on an edge contact the centre probe's face,
or world up on a `Lip` or `None`. On an edge contact that is two sweeps, the same two the movement makes. The face
rule of the first bullet below was the Legacy law's (`attackGroundNormal`, retired with it); the bullets keep
task 4's and task 13b's record.

`sampleAttackGround(state, sd, bindings, query)`, a template on the query type, is the
single source of the ground normal the attack code is built in. `SimulatableBrawler::integrate` calls it
once per tick **before the machine**, from the movement `State` as the tick finds it, and hands the
`brawlerGround::GroundSample` to the machine and the guard (`BrawlerGroundFrame-rationale.md` §1).

- **One sweep:** one `probeGround` sweep (§28). (Until the flip, og-ground-aligned-attacks task 10, the
  shipped knob-off arm returned the default sample with no query, so the knob-off build added no sweep to
  the step; task 14 deleted that arm with the knob.) The face is the hit's exact face normal
  `SweepHit::surfaceNormal` when the adapter provided one (non-zero), else the sweep's `normal`; the sample
  is that face when the hit is blocked and the face passes the movement's own walkable test
  (`glm::dot(face, kWorldUp) >= sd.cosMaxSlope`), and `kUp` otherwise (a miss, a face past the cap, a
  vertical exact face under a walkable sweep normal). The walkable test is applied to the **face**, not
  read from the probe's own `walkable` (which tests the sweep `normal`), so an exact face past the cap is
  rejected even when the sweep normal would pass. Since og-ground-aligned-attacks task 13b this rule is the
  helper `attackGroundNormal(hit, sd)`, which step 2's `face` calls too (§31, guard G-28); the sample is
  `brawlerGround::GroundSample{attackGroundNormal(probe.hit, sd)}`, bit for bit the value the two returns gave
  before. Since task 32 it is the `faceNormal` of `estimateSupport`, the same two calls (§32).
- **Not `DerivedState::surfaceNormal`.** The movement's `DerivedState::surfaceNormal` is step 2's `n` (since
  task 32 the frame normal `classifySupport` returns, §32)
  (the sweep `normal` on walkable ground, else `kWorldUp`), written during the movement step and not
  restored on rollback: on the first replayed tick it holds the pre-rollback value. The sample reads only
  the `SweepHit` fields of its own sweep. The names are alike; the quantities are not.
- **Same tick, same position.** The sample runs before the movement block, so it sees the position the
  movement's own probe sees later in the tick, except on a teleport tick (the movement applies the
  teleport seed before its probe): the sample then uses the pre-teleport position, which is deterministic
  on every peer.

---

## 30. The surface-relative lift test and the tangent frame while detached (og-ground-aligned-attacks task 13)

⭐ **[og-ground-aligned-attacks task 37] Retired with the Legacy support law.** The lift test below
(`detachesFromSupport`, guard G-07) and the detached frame's `surfaceFrame::isFlat` term are deleted. Since task 34
the shipped law's decision is event-driven (§34: one formula on every face, no `isFlat`, and the frame of every
Unsupported tick is world up), task 36 made that law the default, and task 37 deleted the last caller. Two of this
section's facts still hold under that law: `kMinLaunchSine` (sin 2°) stays above Jolt's sweep-normal error bound
(the `static_assert` beside it), and a detached body's frame is world up, so its deceleration is world-horizontal.
`GroundAlignedKnockback.DetachedTargetDeceleratesWorldHorizontally` runs the one law since task 37 and prints the
same line as before (`ticks=7 worstAlignedVerticalErr=4.02331352e-07`). The rest is task 13's record.

Step 2 now computes, before the support classification (since og-ground-aligned-attacks task 32 these lines are
the bodies of `estimateSupport` and `classifySupport`, §32, which also gives today's names of the locals below):

- `face`: `attackGroundNormal(probe, sd)` since og-ground-aligned-attacks task 13b (§31): the probe's
  `surfaceNormal` when it is non-zero, else its `normal`, kept when the hit is blocked and passes the walkable
  test on the face itself, else `kWorldUp`. It is the attack sample's value (§29). Until task 13b it was the
  face without that test (`surfaceNormal` when non-zero, else `normal`), which let an unwalkable exact face
  under a walkable sweep normal reach the readers below; §31 has the case.
- since og-ground-aligned-attacks task 9b, the hit-tick knockback write
  (`writesKnockbackVelocity(inboundHit, face, walkable, committed)`, §26.1, guards G-26 and G-27), so the
  classification below sees the written velocity;
- `detached = detachesFromSupport(state, up, face, committed)`, and
  `support = (walkable && !detached) ? Supported : Unsupported`, as before.

**The lift test** (guard G-07). On a flat face it is task 27's `committed && dot(velocity, up) > 0` (the
knob-off build ran it everywhere until the flip, task 10). On a non-flat face it is `committed && dot(velocity, face) > kMinLaunchSine · |velocity|`, with `kMinLaunchSine` = sin 2°.
The 2° margin sits above Jolt's sweep-normal error bound (0.0221, a `static_assert`) and well above the
rounding of a velocity built in the target's own plane (a same-face tangent gives `dot(v, n)` of order
1e-6 · |v| at most; exactly 0 for the 20 m/s up-slope case), so an in-ground knockback slides and a leaving one launches. Measured on the analytic 30°
plane: an in-plane 20 m/s velocity in any of 360 directions never detaches; 5° out of the plane in every
one of them always does; 1° out does not, 3° out does.

**The tangent frame while detached.** Before task 13 the frame normal was
`n = walkable ? probe.normal : up`: keyed on the probe, not on support. A launched target still inside
the probe range (0.5 m, `rideHeight + snapDistance`) of a 30° face therefore decelerated (`launchDecel`,
step 3's committed arm) along a `u` with a vertical part, adding about ±`launchDecel · sin 30°` = ±20 m/s²
to its vertical acceleration. Now
`n = (walkable && !(detached && !surfaceFrame::isFlat(face))) ? probe.normal : up`: on a
non-flat face, a detached target uses world up, so `u` and `v` are world-horizontal, the deceleration is
horizontal and only gravity changes the vertical velocity. Measured
(`GroundAlignedKnockback.DetachedTargetDeceleratesWorldHorizontally`, 0.2 m above a 30° face, 5° out of
it at 20 m/s): the vertical velocity changes by `gravity · dt` per tick to within 4.1e-7 m/s over the 7
ticks the probe still hits; with the pre-task frame (the knob-off arm, measured by the same case until
task 14 deleted that arm) the same seed's vertical change was off by 0.333 m/s per
tick (`launchDecel · dt · sin 30°`).

**What does not change.** `n` reads only the probe and `detached`, so computing it after the
classification moves no flat value; on a flat face the expression reduces to `walkable` and every
value is today's (both goldens exact). Gravity, the servo axis (`up`, ruling #26 (a)), the landing
(step 6′, guard G-08) and the vertical law are untouched. There is still no `DAttackState::Launched`.

**Near a crease.** The face is the exact face of whatever the probe's sweep hit, and near a crease that
is not always the face the target stands over. Measured on the Jolt slope scene (30° face, flat plateau
above, flat landing below): a slope-side target within about 0.2 m of the convex top edge gets the
plateau's face (flat), while its sweep `normal` is an edge normal; the flat arm then decides, so an
up-slope velocity with any upward world component detaches there, as it does today. A flat-side target
within about 0.1 m of the concave crease gets the slope's face, so the slope arm decides for it. Closer to
the convex edge (0.1 m, task 10's slope golden) the exact face is the slope box's 60° end face, past the
walkable cap; since task 13b `face` is world up there (§31), so the flat arm decides there too.

From task 13 to task 14 `integrate` also carried a `bool` template parameter defaulted to the knob;
task 14 deleted it, so `integrate` is again a template on its two adapter types only.

---

## 31. One ground rule for the movement and the attack — `attackGroundNormal` (og-ground-aligned-attacks task 13b)

⭐ **[og-ground-aligned-attacks task 37] Retired with the Legacy support law.** `attackGroundNormal` is deleted. The
rule it introduced, one ground for the movement and the attack, holds in a wider form: both take the `faceNormal`
of the same `estimateSupport` call on the same `State` (§33; guard G-25 at the sample, and since task 37 guard G-28
at step 2's call). The record below, and the three cases it cites (deleted with the law), are task 13b's.

`inline glm::vec3 attackGroundNormal(const SweepHit& hit, const StaticData& sd)` is the ground normal both the
attack and the movement build in: the hit's `surfaceNormal` when it is non-zero, else its `normal`, returned when
`hit.blocked && glm::dot(face, kWorldUp) >= sd.cosMaxSlope`, else `kWorldUp`. Its two readers are
`sampleAttackGround` (§29) and step 2's `face` (§30). Guard G-28 forbids a second derivation. Since
og-ground-aligned-attacks task 32 the helper is called once, in `estimateSupport`, and both readers take that
call's `faceNormal` (§32).

**Why.** Task 10's slope golden found the two readers disagreeing (its impl notes' open point 2, its review's L1).
Step 2 took `face` as the exact face with no walkable test, and its `walkable` reads the sweep's GJK `normal`
(`probeGround`, §28). Within about 0.1 m of the convex edge those are different surfaces: the GJK normal is an
edge normal, (0, 0.977, 0.211) at F's spot, which passes the cap, and the exact face is the slope box's upper end
face, (0, 0.5, −0.866), 60° from up, which does not (`maxSlopeAngleDeg` is 45°). So the end face fed
`writesKnockbackVelocity` and `detachesFromSupport` while the attack sample gave world up for the same hit.

**What changed.** Only the `face` step 2 hands to those two predicates and to the `n` test. Where the exact face
is walkable (every face interior, the plateau, both sides of the concave crease) the helper returns the same
face as before. Where it is not, it returns `kWorldUp`, so the predicates run their flat arms: the flat rule
(guard G-27) writes nothing for a horizontal direction, and the lift test is the task-27 `dot(velocity, up) > 0`.
For a miss, or a blocked hit with both normals past the cap, `walkable` is false, so `support` is `Unsupported`
and `n` is `up` whatever `face` is; no value changes there. `walkable`, the clearance and snap, and the frame
normal `n = probe.normal` are untouched.

**Measured** (`GroundAlignedKnockbackTest.cpp`, the plane rig with a scripted exact face):
- `GroundAlignedKnockback.UnwalkableExactFaceOverAWalkableSweepIsWorldUp`. Sweep normals (0, 1, −0) and 20°, exact
  faces the 60° end face and a vertical one. For three horizontal directions, including the one along the face's
  horizontal part, which leaves the face by 0.866, the run is bit-equal for 40 ticks (480 ticks) to the same
  rig with its world direction zeroed (until task 14, to the knob-off run; both are the flat-rule tick, §26.1)
  and the leaving direction stays `Supported` on its hit tick. For two directions 10° out of the
  horizontal, chosen so that the end-face lift test and the world-up one disagree, the run is bit-equal to one
  whose exact face is `kWorldUp` (320 ticks), and the hit tick detaches exactly when the direction points up.
  With step 2 on the pre-task expression it fails on tick 1 (`first differing field velocity`).
- `GroundAlignedKnockback.WalkableExactFaceStillDecides`. A walkable exact face over a different walkable sweep
  normal (a 30° face over 15° and 29° sweeps, and over a flat one; the plateau over a 15° sweep) is returned bit
  for bit, and a horizontal direction that leaves the 30° face launches through the slope arm on the hit tick (9
  hits), as before.
- `BrawlerGroundSample.AlignedSampleIsTheSharedGroundNormal`. On 1049 scripted hits (the cases of
  `BrawlerGroundSampleTest.cpp` and a grid of sweep normals, exact faces and blocked flags, the cap included) the
  sample, the helper and the pre-task sample expression are bit-equal.

**The knob off (until task 14).** The knob-off predicates discarded `face`, and the `n` test read it only
with the knob on, so step 2 computed the helper's value but nothing read it; every knob-off case in
`GroundAlignedKnockbackTest.cpp` and `BrawlerGroundSampleTest.cpp` passed unchanged. Task 14 deleted
those cases with the knob.

**On the slope golden** (`BrawlerJoltSlopeGoldenTraceTest-rationale.md` §7). Only F's convex-crease hits change.
F rests 0.1 m from the edge with a vertical velocity of +1.11e-6 m/s (the hover servo's residue). On both hit
ticks (990 and 1100) the flat lift test reads that residue and detaches F, and the knockback is today's
`velocityUV` in the tangent frame of the edge normal, 12.2° from up. Thrown onto the plateau (990) F rises 0.84 m
and is `Unsupported` for 43 ticks; thrown down the slope (1100) it falls off the edge for 23. At that spot the
movement now runs the expressions the knob-off build ran until the flip (task 10): 990's measurements equal those of task 10's knob-off
recording of the same scenario field for field. 1100's do not (5.75 m of travel and 23 `Unsupported` ticks
against 5.86 m and 24): from tick 1101 F's probe finds the slope face, (0, 0.866, 0.5), where the
detached frame (§30) gives `n` = world up.

---

## 32. The support law extracted — `estimateSupport` and `classifySupport` (og-ground-aligned-attacks task 32)

⭐ **[og-ground-aligned-attacks task 37]** The split stands, but the Legacy bodies described here are gone.
`estimateSupport` is §33's estimate, and `classifySupport(state, sd, ground, committed, written, d)` is §34's
transitions (task 37 gave §34's function, then `classifySupportV2`, this name). The table below is the Legacy
law's, kept as the record of the four grounds.

Since og-ground-aligned-attacks task 32, step 2's ground and its support decision are two functions. Their bodies
are step 2's lines **moved verbatim**: the same operations in the same order, with no value changed and no wire
version bump.

- `estimateSupport(state, sd, bindings, query)`, a template on the query type, returns a `GroundSupport`
  `{probe, eligible, frameNormal, faceNormal}`. It calls `probeGround` (§28), then `attackGroundNormal(probe.hit, sd)`
  (§31, guard G-28), and copies `probe.walkable` and `probe.hit.normal` into `eligible` and `frameNormal`.
- `classifySupport(state, ground, committed)` returns a `SupportDecision` `{detached, support, frameNormal}`:
  the lift test (`detachesFromSupport`, guard G-07), the classification, and the frame normal of §30.

**The order in `integrate`.** `estimateSupport`, then the hit-tick knockback write (`writesKnockbackVelocity` and the
assignment, §26.1), then `classifySupport`. The write stays between the two, at guard G-26's tag, so the
classification still reads the written velocity. Every later reader in the step reads the two structs:
- the tangent frame, the `[Movement.surface]` line and `DerivedState::surfaceNormal` read `decision.frameNormal`;
- the support bits and step 4's servo branch (guard G-11) read `decision.support`;
- the servo's error (guard G-10) and the log read `ground.probe.clearance`;
- `DerivedState::lastProbePoint` reads `ground.probe.hit`.

`integrate` keeps its `up` local for the decomposition and the recomposition. `classifySupport` names
`kWorldUp` where step 2 named `up`; `up` is `const glm::vec3 up = kWorldUp;`, so both are the same constant.

**The attack sample** is `estimateSupport(...).faceNormal` (§29): one sweep (guard G-25) and one face rule (guard
G-28). Before task 32 it called `probeGround` and `attackGroundNormal` itself. Those were the same two calls, so
its value is unchanged.

**What each field is today.** These are the initiative's four grounds (its design, §3.1), now visible as fields.
The table is `SupportLaw::Legacy` (the shipped law until task 36, §36). Since task 33 `GroundSupport` also carries `kind` (`Face` when
`probe.walkable`, else `None`, under Legacy), and under `SupportLaw::V2` the fields take §33's values:

| field | today's value | which ground |
|---|---|---|
| `GroundSupport::probe` | the capsule sweep (§28): the hit, the clearance along `kWorldUp`, `walkable` | the clearance is measured along world up |
| `GroundSupport::eligible` | `probe.walkable`: blocked, and the sweep normal passes `cosMaxSlope` | the sweep's GJK normal |
| `GroundSupport::frameNormal` | `probe.hit.normal`, the sweep's GJK normal (on Jolt, the normalised, negated penetration axis) | the sweep's GJK normal |
| `GroundSupport::faceNormal` | `attackGroundNormal(probe.hit, sd)`: the exact face when the adapter gives one and it is walkable, else `kWorldUp` (§31) | the exact face, or world up |
| `SupportDecision::detached` | the lift test against `faceNormal`: its slope arm on a non-flat face, its flat arm (world up) on a flat one (§30) | the exact face, or world up |
| `SupportDecision::support` | `Supported` when `eligible` and not `detached`, else `Unsupported`; `SupportedSteep` is never produced | the sweep's GJK normal |
| `SupportDecision::frameNormal` | `ground.frameNormal` when `eligible`, unless the target is detached over a non-flat face; else `kWorldUp` (§30) | the GJK normal, or world up |

The fourth ground, the push-out direction of step 6′ (guard G-08), is in neither struct. Step 6′ runs before the
estimate and reads the previous tick's solve.

**Reading the older sections.** §23 to §31 and the carried text name step 2's locals as they were before
task 32. Today those values are:
- `probe` is `ground.probe.hit`, `clearance` is `ground.probe.clearance`, and `walkable` is `ground.eligible`;
- `face` is `ground.faceNormal`;
- `detached` and `support` are `decision.detached` and `decision.support`;
- `n` is `decision.frameNormal`.

**Measured** (task 32, OGBrawlerTests):
- `[@og]`: 912 cases before and after.
- Both goldens' `MatchesTheRecordedTrace` pass in exact mode, with `[GoldenTrace.signedZero] p=0 d=0 a=0 q=0 s=0 2=0`.
- The support-law scorecard's and PlatformEdge's 7,105 output lines, run with OG_SUPPORT_LAW_RUNS=1, are
  byte-identical to task 31's. They include `[SupportLaw.scorecard]`, `[PlatformEdge.summary]` (`mirrorMismatches=0`)
  and `[PlatformEdge.decomposition]`.
- PlatformEdge's mirror takes the same two functions (`PlatformEdgeTest-rationale.md` §4).

---

## 33. The support estimate under `SupportLaw::V2` — the exact face, edge contacts, the centre probe and `Lip` (og-ground-aligned-attacks task 33)

⭐ **[og-ground-aligned-attacks task 37] The switch is gone.** `SupportLaw`, the `StaticData` member `supportLaw` and
the Legacy arm are deleted. The estimate below (`estimateSupportV2` in this section) is the body of `estimateSupport` itself,
which sweeps the capsule probe and continues with the steps below. `PhysicsSetup::queryVolumes` declares both
volumes for every character, and its `OG_CHECK` holds the count at two
(`BrawlerGroundSample.DeclaresTheGroundProbeAndTheCentreProbe` checks the descriptors,
`SupportEstimate.RegistersTheGroundProbeAndTheCentreProbe` the Jolt registration). Where the text below says "under
Legacy" or "today's estimate", it describes the law of its time.

Since og-ground-aligned-attacks task 33, `estimateSupport` has a second arm, selected by authored data. Until task 36
the shipped law was `SupportLaw::Legacy`, and under it every value in §32 was unchanged (the goldens stayed exact,
with no wire version bump). Since task 36 the shipped law is `SupportLaw::V2` (§36).

**The switch.** `enum class SupportLaw : uint8_t { Legacy = 0, V2 = 1 };` and the member
`SupportLaw supportLaw` in `StaticData`, beside `drivesBody`. Its default was `SupportLaw::Legacy` until task 36
and is `SupportLaw::V2` since (§36).
- It is not a constructor parameter. The constructor's argument list is positional, and an inserted argument
  shifts its neighbours silently.
- It is not a cvar and not an ini key: it is authored data, the same on every peer.

**Why the frame must be the face.** This is the design's derivation (§3.2 of the initiative's support-law design),
kept here because it is the reason for the arm:
- the servo damps the dual-basis channel `c`, where `V = a·u + b·v + c·up` and `u`, `v` span the frame plane (§19);
- for a face with normal `n`, the vertical gap is `h = dot(X − P, n) / dot(up, n)`, so
  `dh/dt = dot(V, n) / dot(up, n) = c`, because `u` and `v` are perpendicular to `n`;
- so the servo is an exact PD on the vertical gap **if and only if the frame normal is the face the clearance is
  measured to**. A frame on a rounded edge normal (the GJK normal at a rim) tilts `u` and `v` off that face, and the
  in-plane deceleration and the damping then act on a channel that is not the gap.

**The estimate (`estimateSupportV2`).** `estimateSupport` sweeps the capsule probe first under both laws (§28). Under
V2 it then hands the probe to `estimateSupportV2`:

```
exact       = surfaceNormal provided ? surfaceNormal : normal          (exactContactOf)
faceContact = !provided || dot(exact, normal) >= kEdgeContactCos        (cos 2°)
!blocked              → None, normals kWorldUp
faceContact           → dot(exact, kWorldUp) >= cosMaxSlope ? Face(exact) : None(kWorldUp)
edge contact          → sweep the centre probe from the same pose, through the same rideHeight + snapDistance;
                        blocked, its own face contact, walkable → Face(its exact face)
                        else → probe.walkable ? Lip(kWorldUp) : None(kWorldUp)
eligible = kind != None; frameNormal = faceNormal = the normal above (groundSupportOf); the clearance is the
capsule probe's, unchanged
```

- **The edge threshold.** `kEdgeContactCos = 0.99939083f` is cos 2° (cos 2° = 0.99939082702, whose nearest float is
  the literal's). A `static_assert` keeps it below 0.9997558 = cos 0.0221, where 0.0221 rad is the bound
  sqrt(2 · 1e-4 / 0.41) = 0.0220863 on Jolt's sweep-normal error that `kMinLaunchSine` also stays above (§30). Below
  that angle a GJK outlier on a face interior, not the geometry, would make a contact an edge. Measured on Jolt
  (`SupportEstimate.FaceInteriorIsTheExactFaceWithNoCentreSweep`): on 732 vertical sweeps onto three face interiors
  the GJK normal and the exact face agree to the printed precision (`worstGjkRad=0.000000`).
- **The centre probe** is a `SphereGeometry{kCenterProbeRadius}` (0.02 m) query volume, declared by
  `PhysicsSetup::queryVolumes` at index 1 only under V2, offset straight down by
  `capsuleHalfHeight − kCenterProbeRadius`, so its bottom is the capsule's bottom. Under Legacy the list is exactly
  the one ground probe, and an `OG_CHECK` at the list's end holds the count per law
  (`BrawlerGroundSample.LegacyDeclaresExactlyOneQueryVolume` checks the descriptors,
  `SupportEstimate.LegacyRegistersOneQueryVolumeAndV2Two` the Jolt registration).
- **The centre is swept only on an edge contact** (guard G-29). A face contact is the face the clearance is measured
  to, by the derivation above. An adapter or mock that gives no `surfaceNormal` (zero) never makes an edge contact,
  so it never sweeps the centre and gets today's estimate (`GroundAlignedKnockback.V2EqualsLegacyOnEveryPlaneWithoutAnExactEdge`).
  The arm's `OG_CHECK` stops a V2 estimate whose bindings have no centre volume.
- **At a convex edge** the capsule's clearance to the edge is at least its clearance to the face's plane, because the
  edge lies on that plane. So under Face(top) at a rim the servo's error is at most the plane's, and it cannot kick.
  Measured: every V2 PlatformEdge departure classed as a down-pull has `servo=0.0000` (see "Measured" below).
- **`Lip`** (the L-hold ruling, OQ3) is support-eligible with frame and face `kWorldUp`: the servo holds the capsule
  over the edge in an upright frame. It ends where `probe.walkable` ends, so the hang extent is Legacy's.

**What V2 changed in task 33, and what it did not.** Under task 33 `classifySupport` (§30) and the knockback write
(§26.1) read `eligible`, `frameNormal` and `faceNormal`, which were V2's, while the lift test (`detachesFromSupport`),
the write predicate (`writesKnockbackVelocity`) and the frame expression stayed task 32's. So a `Lip` tick went
through the lift test's flat arm (`faceNormal` is `kWorldUp`). Since og-ground-aligned-attacks task 34 V2 has its own
transitions, frame and write (§34); the estimate in this section is unchanged.

**`GroundSupport::kind`** is `SupportKind::None`, `Face` or `Lip`. Under Legacy it is `Face` when `probe.walkable`,
else `None`, and the other fields are §32's.

**`DerivedState::contactNormal`** is the sweep's GJK normal when the probe is walkable, else `kWorldUp`. The movement
visualization's probe-sphere reconstruction needs the contact direction, which is that normal, so it reads this field
and keeps drawing `surfaceNormal` (the frame). Under Legacy the two differ only on a tick detached over a non-flat
face, where the frame is `kWorldUp` (§30).

**The attack sample** is `estimateSupport(...).faceNormal` under both laws (guard G-25): one probe, and one ground
rule per law. Under Legacy that rule is `attackGroundNormal` (guard G-28); under V2 it is the estimate above
(`BrawlerGroundSample.V2SampleIsTheEstimatesFaceNormal`).

**Measured** (task 33, OGBrawlerTests):
- Legacy: both goldens exact (`[GoldenTrace.signedZero] p=0 d=0 a=0 q=0 s=0 2=0`), and the 7,105 Legacy lines of
  the support-law scorecard and PlatformEdge byte-identical to task 32's.
- V2 on Jolt (`SupportEstimateTest-rationale.md` in og-brawler-tests): at the imported platform's 30° top rim the
  estimate is Face(top) through x = −0.015 m (at −0.1 m the capsule's contact is already the edge, and the face
  comes from the centre probe), `Lip` from −0.010 m,
  and `None` from +0.335 m, the same position at which `probe.walkable` turns false (GJK tilt 45.04°); 36 seam
  crossings stay `Face` within 6e-8 of the face; no reported wall or 60° lip face is ever `Face`.
- V2 PlatformEdge, with this task's partial law (today's lift test): 8 lifts (Legacy 66) and 19 down-pulls (Legacy
  23). The −5.00 m/s servo kick is gone: the largest V2 down-pull is −0.66 m/s, all of it the in-face deceleration
  on one or two `Face` ticks past the capsule's last pure-face contact, with the servo at 0.
  - 6 of the 8 lifts are `Lip` ticks on which the lift test's flat arm keeps a downward knockback `Supported`, and
    the damping adds +1.77 to +5.00 m/s. Task 34's transitions replace that test.
  - The other 3 (the low edge) are one `Face` tick past the capsule's last pure-face contact: the servo's gravity
    hold (+0.16 m/s) and the in-face deceleration of a downhill knockback (+0.33 m/s).

---

## 34. Support transitions and the one knockback write under `SupportLaw::V2` (og-ground-aligned-attacks task 34)

⭐ **[og-ground-aligned-attacks task 37]** `classifySupportV2` and `writesKnockbackVelocityV2` are `classifySupport`
and `writesKnockbackVelocity` since task 37, and the Legacy predicates this section contrasts them with are deleted.
Step 3's hit-tick branch is `velocityUV = currentUV` on every hit tick, so the movement reads
`inboundHit.hitDirectionXY` nowhere (§37 lists the field's remaining readers).

Since og-ground-aligned-attacks task 34, `SupportLaw::V2` decides support by events rather than by a per-tick lift
test, and writes every committed knockback the same way. `SupportLaw::Legacy` is untouched: `classifySupport`,
`detachesFromSupport` (guard G-07), `writesKnockbackVelocity` (guard G-27) and the hit-tick channel branch run
exactly as before, and both goldens stay exact with no wire version bump.

**The transitions (`classifySupportV2`).** The inputs are the restored `State`, the estimate `g` (§33), `committed`,
the write's predicate `written` and the written direction `d = inboundHit.hitDirectionWorld`. `previous` is the
SupportState the last tick stored in `State::flags` bits 1–2 (guard G-30). The first row that matches decides:

| row | condition | support | what it is |
|---|---|---|---|
| 1 | `g.kind == None` | Unsupported | the ground ended |
| 2 | `committed && previous == Unsupported && g.probe.clearance > sd.rideHeight` | Unsupported | the hysteresis: an airborne knockback re-attaches only at its hover |
| 3 | `written` | `s > kMinLaunchSine` ? Unsupported : Supported, with `s = abs(dot(d, g.faceNormal))` on a `Lip` and `s = dot(d, g.faceNormal)` on a `Face` | the launch event, decided by the direction alone |
| 4 | `committed && g.probe.clearance >= sd.rideHeight && dot(v, g.faceNormal) > max(kMinLaunchSine·‖v‖, kMinSeparationSpeed)` | Unsupported | a separation above the hover (guard G-31) |
| 5 | otherwise | Supported | |

- **One formula on every face.** There is no flat arm and no `surfaceFrame::isFlat` test. On flat ground row 4 is
  the same cone as on a slope, so the per-tick flat arm `dot(v, up) > 0` that made every flat knockback flicker
  (§30; the scorecard's flat rig: 1,803 flips under Legacy) is gone.
- **On a `Lip` a hit launches in either direction** (row 3; task 34's rework, a lead ruling). A `Lip`'s face is
  `kWorldUp` (§33), so with row 3's one-sided test a downward `d` read as "into the ground" and stayed Supported.
  But the body is over an edge, with no surface below it to push into. A downward hit there kept the body at its
  hover, and the servo's damping removed up to `hoverMaxAccel · dt` of the written downward velocity on the hit tick.
  Measured on PlatformEdge under V2: a target 0.15 m past a tip or tip corner, hit along a downward in-face direction
  (tipPlusMid +30°, tipMinusMid −30°, tipCornerPlus +30°, tipCornerMinus −30°), took a +5.0001 m/s servo kick with the
  one-sided row and 0.0000 with `|dot(d, n)|`. The test is `|dot(d, n)|` on a `Lip` only: on a `Face` an into-face hit
  has a surface to slide along. PlatformEdgeTest's V2 run pins it (its rationale §5).
- **Below its hover a body is never detached by velocity** (guard G-31). Below `rideHeight` the servo lifts a
  supported body (§23's step 4), so an upward velocity there is the servo's own correction. Above the hover the
  one-sided servo is gravity only (ruling #29), so an upward velocity there is momentum, except for the one tick on
  which the servo's push carries the body across the hover. `kMinSeparationSpeed` covers that tick.
- **Walkers** (not `committed`) reach only rows 1 and 5: they are `Supported` on every `Face` and `Lip` at any
  velocity, as under Legacy, and follow a convex crease in the face's frame.
- **`detached`** in the returned `SupportDecision` is `g.eligible && support == Unsupported` (the estimate saw
  ground, and a row other than 1 decided Unsupported). Only the PlatformEdge rig's mirror reads it.

**The frame:** `support == Supported ? g.frameNormal : kWorldUp`. Detached means world up on every face, and there is
no `isFlat` term (the initiative's 14b site 2c). Under V2 `g.frameNormal` is the estimate's face (§33).

**The write (`writesKnockbackVelocityV2`).** `committed && inboundHit.wasHitThisTick && d != glm::vec3(0.f)` writes
`d · knockbackSpeed` before the classification (guard G-26), on every face: there is no flat rule (the initiative's
14b site 3b; guard G-27 is the Legacy predicate's). Step 3's hit-tick branch is `velocityUV = currentUV` on every V2
hit tick, so V2's movement reads `inboundHit.hitDirectionXY` nowhere. A committed hit tick with a zero direction
keeps the current velocity's tangent channels.

**`kMinSeparationSpeed = 0.33f` m/s, measured (task 34 part 1).** Over every scorecard rig (flat, crease,
PlatformEdge), every plane rig of the GroundAlignedKnockback cases and both golden scenarios, run under V2 without
re-recording:

| quantity | definition | under task 33's V2 | under task 34's V2 |
|---|---|---|---|
| servo residue | largest `dot(v, n)` on a committed tick that stayed Supported while its clearance crossed `rideHeight` from below since the previous tick | 0.0761 m/s (PlatformEdge chamferPlus d = 4.5) | 0.1083 m/s (PlatformEdge tipPlusMid d = 4.9, a `Lip` slide at 1.24 m/s) |
| smallest intended separation | smallest `dot(v, n)` on the first tick of a committed run whose face changes by more than 5° at a convex crease (the crease rig's slopeToPlateau and plateauToSlope runs, PlatformEdge's top-rim sites arcTop, arcTopSeam, chamferPlus and chamferMinus) | 1.2591 m/s (PlatformEdge chamferMinus d = 3.5) | 1.2591 m/s |

The value is 3.05× the larger residue and 0.26× the smallest separation (below half of it, as the task requires).
A `static_assert` holds both bounds. Under task 34's V2 one more crossing tick reaches 0.449 m/s: a convex departure
whose first tick on the new face had a clearance below the hover, so rows 4 and 5 kept it Supported and the servo
damped it first. There the cone (`kMinLaunchSine · 19.3 m/s` = 0.675 m/s) decides, not the floor.

**Measured** (task 34, OGBrawlerTests):
- Legacy: `[@og]` 934 cases, both goldens exact (`[GoldenTrace.signedZero] p=0 d=0 a=0 q=0 s=0 2=0`), and the
  7,105 Legacy lines of the support-law scorecard and PlatformEdge byte-identical to task 33's.
- V2 flat rig: 0 flips (Legacy 1,803), 0 slide-end ticks (Legacy 120).
- V2 crease rig: each of the 22 committed convex departures is Unsupported on its crossing tick, with a vertical
  speed equal to the ballistic step from the previous tick (`worstBallisticErrMps=0.0000`). The entering crease does
  not hop (flatToSlope `apexMaxM=0.0000`, Legacy 3.2331 m), before the redirect of the initiative's task 35. That is
  the hover rule: deleting `clearance >= rideHeight` from row 4 brings 18 concave hops back.
- V2 PlatformEdge, measured from the estimate's last `Face` tick: 0 down-pulls and 1 lift. The lift is a slide onto
  the tip corner (tipCornerPlus d = 4.9, +1.77 m/s): the servo damping a downward velocity on a `Lip` on a tick that
  is not a hit tick, so row 3 does not decide it. Before row 3's `Lip` test there were 5 lifts: that slide and the
  four Lip hit ticks above (+5.0001 m/s each). The `Lip` test changed only those four placements: each now departs
  on its hit tick, and the edge-tick lifts went from 8 to 4. The flips (214), rim hangs (17), and the flat and crease
  rigs are unchanged.
- The redirect of §35 (task 35) changes the V2 crease and PlatformEdge numbers above; §35 has them.

---

## 35. The redirect into the face under `SupportLaw::V2` (og-ground-aligned-attacks task 35)

⭐ **[og-ground-aligned-attacks task 37]** The call is no longer behind `v2`: `redirectsIntoFace` runs on every tick
of the one law. The two arc-top stay runs below (2 → 84 and 1 → 0 geometric `Lip` ticks) are traced in §37.

Since og-ground-aligned-attacks task 35, V2 removes the part of a committed body's velocity that points into its
ground, after the classification (§34) and before the tangent frame is built. `SupportLaw::Legacy` never calls it:
the call is behind `v2`, so Legacy runs no new operation, and both goldens stay exact with no wire version bump.

**The rule (`redirectsIntoFace`, then the assignment in `integrate`).** With `n = g.faceNormal` and `previous` the
SupportState last tick stored in `State::flags` bits 1–2:

```
support == Supported && committed && kind == Face && (written || clearance <= rideHeight)
    && (previous == Supported || written) && dot(v, n) < −kMinLaunchSine·‖v‖
    → v −= n · dot(v, n)
```

The `Face` and hover terms are task 35's rework 1 (a lead ruling on the redirect's scope), and the hover term's
`written ||` is its rework 2 (a lead ruling: a hit tick is not a fall). The terms are ordered cheapest first; none
has a side effect, so the order does not change the result.

- **No rescale.** The user's ruling (2026-10-30, the initiative's task 19): "we should slide but lose the
  into-slope part". A horizontal 20 m/s crossing onto a 30° face leaves at 20 · cos 30° = 17.32 m/s along the face.
- **Why the decomposition alone is not enough.** §19's decomposition (the exact dual solve since task 30) splits a
  velocity into the face's two tangent channels and world up. A horizontal 20 m/s into a 30° face is
  23.09 m/s along `u` and −11.55 m/s on the up channel. Under task 34's V2 the hover rule already kept such a body
  Supported, but it slid at `V / cos θ`, and the servo absorbed the −11.55 m/s as a dip of the clearance. After the
  redirect the velocity lies in the face, so the decomposition gives 17.32 m/s along `u` and 0 on the up channel.
- **Which ticks.** A crossing onto a new face (`previous == Supported`), and a hit tick whose written direction
  points into the face (`written`; row 3 keeps it Supported, §34). The 2° cone (`kMinLaunchSine`) leaves an
  in-plane velocity untouched: a same-face slide's `dot(v, n)` is rounding.
- **Landings are not redirected** (guard G-32). A committed body that was Unsupported last tick and that this tick
  did not write is landing; the hover servo and step 6′ catch its fall (ruling #28's soft landing).
- **Not above the hover, unless this tick's hit wrote the velocity** (guard G-32). Above `rideHeight` the one-sided
  servo is gravity plus a pull-down, so a velocity into the ground there is a fall that the hysteresis and the
  servo own, as for a landing. It mirrors G-31
  (no velocity test detaches a body below its hover). Without the term the redirect removes the servo's pull-down
  on every tick above the hover where it exceeds the cone, and at a slow speed the cone is small (0.035 · ‖v‖): a
  slow body over a convex crest floats and bleeds its speed. Measured with the term deleted: the scorecard's
  slopeToPlateau runs at d = 4.5 m, ±30°, reach 0.1437 m of clearance at 0.3103 m/s on tick 114 (with it 0.1224 m
  at 0.6650 m/s) and stop short of the crease line, so 12 of the 14 runs cross instead of 14.
  - **A hit tick is redirected above the hover too** (`written ||`, rework 2). A written velocity is not a fall, so
    the hover reason does not cover it. And a body resting at its hover has a clearance equal to `rideHeight` only
    to within rounding, so without `written ||` a hit into the face on a body at rest would be decided by rounding.
    Measured: in the slope golden's v2 print, G's hit on F at tick 410 (into F's face, `dot(d, n)` −0.4815) finds
    F at rest, at a clearance that prints 0.100000 but sits a rounding above `rideHeight`. Without `written ||` it
    is not redirected: F meets the face (clearance 0 on ticks 411 and 412), the push-out and the servo take the
    velocity into the face (14.77 m/s on tick 412 against 16.90 m/s), and it slides 2.6684 m with an apex of
    1.2414 m and a smallest `minClearanceM` of 0.8101 m. With it the hit is redirected as at the hover: 3.9861 m,
    1.8964 m and 0.9706 m, the values of task 35's first expression. It stays Supported in both. Over every run
    rework 1 measured, it was the only written into-face hit tick that the hover term excluded, so the rule is
    pinned by a unit case that does not depend on the golden's trajectory as well (below).
- **Not on a `Lip`** (guard G-32). A `Lip`'s face is `kWorldUp` (§33), and a `Lip` has no surface to slide along.
  Without the term, the redirect acts on every committed Supported `Lip` tick at or below the hover whose velocity
  points down beyond the cone, which happens when a body slides down a face over its low edge or tip onto a `Lip`.
  Measured on PlatformEdge with the term deleted: the slide onto the tip corner (tipCornerPlus, d = 4.9 m along the
  fall line) gives +1.852 m/s, of which the redirect is 1.675 m/s and the servo 0.177 m/s. With both terms deleted
  (task 35's first expression) 48 more departures over the tips and low edges become lifts on their `Lip` tick (the
  estimate-reference lifts go from 1 to 49): their `Lip` tick is above the hover.
  - With both terms, PlatformEdge's one estimate lift is task 34's: tipCornerPlus d = 4.9 m, +1.769 m/s, all of it
    the servo's damping on the tip's `Lip` (`estimateRedirect` 0). The redirect does not remove it, by the `Lip` rule.

**Measured** (task 35 with reworks 1 and 2, OGBrawlerTests):
- Legacy: `[@og]` 940 cases (task 34: 934; the six new cases are V2), both goldens exact, and the 7,105 Legacy lines
  of the support-law scorecard and PlatformEdge byte-identical to task 34's.
- A committed 20 m/s horizontal crossing onto a 30° face (a two-plane mock, no decay): 17.320509 m/s along the face
  from the first tick on it, within 1.34e-5 m/s for the rest of the lockout, with no Unsupported tick. Without the
  redirect the first tick on the face slides at 23.09401 m/s (`V / cos 30°`).
- A hit-tick direction 30° into a 30° face, in nine directions: the velocity is `knockbackSpeed · (d − n · dot(d, n))`
  within 3.34e-6 m/s, Supported.
- The same hit on a committed, previously Supported body 1e-5 m above `rideHeight`, in eight directions: the
  predicate is false for an unwritten velocity and true for the written one, and the hit tick's velocity is the
  projected one plus the above-hover servo's vertical step (gravity · dt, −0.1633 m/s) within 1.07e-6 m/s,
  Supported. Without `written ||` the velocity into the face stays (an error of 10 m/s, 0.5 · 20 m/s). The
  1e-5 m is about 80 ulps of the mock body's height (1.13 m), so no rounding of the placement or of the mock's
  sweep puts it at or below `rideHeight` (measured 9.98e-6 m above it); the golden's excess is below 5e-7 m (it
  prints 0.100000). Above the hover the one-sided servo applies `max(hoverStiffness · e, −hoverPullDownAccel)`, which is 0 at the shipped
  values (`hoverPullDownAccel` is 0), so no spring acts on the body, and the tick adds only gravity's step.
- The scorecard's crease rig: on all 22 committed concave crossings that reach the new face within the lockout, the
  speed along the face after the redirect tick equals `V · cos θ` (`θ` the crossing's angle into the face) within
  9.54e-8 · V, with 0 Unsupported ticks after the crossing. Without the redirect the same ticks slide at up to
  22.43 m/s against 16.68 m/s, and the smallest probe clearance after the crossing is lower (flatToSlope d = 0.3 m
  along the fall line: 0.0000 m against 0.0168 m; d = 3.5 m: 0.0216 m against 0.0557 m). slopeToPlateau: 14 of 14
  runs cross, as under task 34.
- PlatformEdge under V2: estimate lifts 1 and down-pulls 0 (edge-tick reference 4 and 19), flips 214 and rim hangs 17,
  as under task 34. Every run line equals task 34's apart from the `estimateRedirect` field, which is 0 on every run.
  Two runs that stay on the platform near the arc top (d = 4.9 m, −30°) count other geometric `Lip` ticks (2 → 84
  and 1 → 0). That change comes with the redirect itself: every measured expression of it gives the same two counts,
  with or without the `Face` and hover terms.
- The flat rig and the flat golden's v2 print are unchanged by the redirect. The slope golden's v2 print changes at
  its crease characters (F and G): the redirect sends an into-face hit along the face at `V · cos θ`, G's hit at
  410 included (the hit-tick note above).

---

## 36. The flip: `SupportLaw::V2` is the default (og-ground-aligned-attacks task 36)

⭐ **[og-ground-aligned-attacks task 37]** `SupportLaw::Legacy` is retired and the member this section quotes is
deleted (§37).

Since og-ground-aligned-attacks task 36 the member reads `SupportLaw supportLaw = SupportLaw::V2;`. Every shipped
character, and every test or scenario that does not set the law, runs §33 to §35. `SupportLaw::Legacy` runs only
where a test names it, until the initiative's task 37 retires it.
- **The wire.** No byte moved, but a 15 peer and a 14 peer predict different movement for the same input wherever a
  body stands on a slope, a rim or a crease, or is knocked back, so the correction version went from 14 to 15
  (`c15.r3.b3`).
- **The registration.** Under V2, `PhysicsSetup::queryVolumes` also declares the centre probe (§33), so every
  shipped character now registers two query volumes.
- **The goldens.** Both were re-recorded. Each new recording equals the V2 run that task 35 compared with the old one:
  the flat golden differs from the old one in 1,521 field-samples, the first at tick 164, and the slope golden in
  2,965, the first at tick 121. The test docs of the two goldens attribute the differences.
- **A committed hit tick with no world direction.** On such a tick Legacy set the in-plane velocity to
  `hitDirectionXY · knockbackSpeed`, and V2 keeps `currentUV` (§34). The routing writes a zero world direction only
  for a hit with no knockback, which is a `Stun`, and a stun does not commit the movement:
  `machineLaunchesMovement` requires `HitReactionKind::Knockback`. Task 36 measured both goldens, the scorecard rigs
  and PlatformEdge, and found no committed hit tick with a zero world direction. The zero-direction hits there are
  all uncommitted: the slope golden's stun at 1369 and its projectile hit at 1080, and two in the flat golden. Only
  test-built slices that set `hitDirectionXY` alone produce such a tick. A stun during a slide still stops the body,
  but through the freeze (`machineFreezesMovement`), not through this tick.

---

---

## 37. The Legacy support law retired (og-ground-aligned-attacks task 37)

Since og-ground-aligned-attacks task 37 the header has one support law, the one §33 to §35 describe. Task 36 made it
the default, so every operation task 37 deleted was on a branch the shipped law never took.

**Deleted:**
- `enum class SupportLaw` and the `StaticData` member that selected the law;
- the Legacy arm of `estimateSupport` (§32) and the Legacy `classifySupport` (§30, §32);
- `detachesFromSupport` (the Legacy lift test, §22, §30) and `attackGroundNormal` (§31);
- the Legacy `writesKnockbackVelocity` and its flat rule (§26.1);
- every `surfaceFrame::isFlat` use in the header, and with them the `OGSimulation/SurfaceFrame.h` include (§1);
- step 3's Legacy hit-tick line, `velocityUV = inboundHit.hitDirectionXY * inboundHit.knockbackSpeed;` (§26).

**Renamed or folded, with the same operations in the same order:**
- the body of `estimateSupportV2` is the body of `estimateSupport`, after its `probeGround` call;
- `classifySupportV2` is `classifySupport(state, sd, ground, committed, written, d)`;
- `writesKnockbackVelocityV2` is `writesKnockbackVelocity(inboundHit, committed)`;
- `PhysicsSetup::queryVolumes` declares the centre probe for every character, after the ground probe, and its
  `OG_CHECK` holds the count at two;
- step 3's hit tick is `velocityUV = currentUV`, the line the shipped law already took.

Task 30's fast path in `decomposeVelocity` (§19.1) is unchanged.

**The guard and derivation ids.**
- G-07 (`detachesFromSupport`) and G-27 (the flat rule) are retired; their entries are in the guards doc's §R.
- ∴D-06 (the hit-tick mapping) is retired at the end of §26.
- G-28 (one ground for the movement and the attack) moved to step 2's `estimateSupport` call, its third site after
  task 32 and task 33.
- G-26 is restated: the write must come before the redirect, which reads the velocity. The classification no longer
  reads it on a written tick (row 3 decides from the direction), so "before the classification" was the Legacy
  law's reason; moving the write between the two passes every case.
- G-25 and G-29 to G-32 stand at their sites and no longer name a law.

**What G-07 carried that still holds.** Its prohibitions were written for the Legacy lift test. Three of its facts
apply to the shipped law's separation row too, which is guard G-31's site:
- the rows are keyed on the direction and the velocity, never on a machine enumerator. The
  `!detail::kHasLaunched<DAttackState>` tripwire holds this at compile time, and its message now names
  `classifySupport`;
- `kMinLaunchSine` stays above the 0.0221 bound on Jolt's sweep-normal error. The `static_assert` beside the
  constant holds this;
- the separation row tests the velocity against the face `n`, not against world up, and scales the cone by the
  speed. [derived] On a 30° face an in-face up-slope slide has `dot(v, up) = 0.5 · |v|` and `dot(v, n) = 0`. Against
  world up it would pass `max(kMinLaunchSine · |v|, kMinSeparationSpeed)` above 0.66 m/s, so every such slide at or
  above its hover would detach. No guard carries this; it is recorded here.

**`inboundHit.hitDirectionXY` is kept.** The movement no longer reads it, but it has readers outside this header:
- the routing's write (`BrawlerHitRoutingSystem.h`, `slice.hitDirectionXY = knockback ? directionXY : glm::vec2(0.f);`);
- the golden trace's `hit.inboundDirection` field (`GoldenTraceScene.h`). Deleting the member changes the golden
  format, and task 37 is bit-identical with no re-recording;
- the routing tests and the slope golden's measurement print.

**The arc-top stay runs (§35's 2 → 84 and 1 → 0).** Traced on PlatformEdge under the shipped law, with and without
the redirect, by printing every tick of the two runs at d = 4.9 m, −30° (arcTop, arcTopSeam). The runs are equal up
to a tick inside the lockout (arcTop 110, arcTopSeam 114). On the tick before it the body sits at its hover, where
the one-sided servo is 0, so gravity's step leaves `dot(v, n)` = −0.1414 m/s and the body dips 2.7 mm. On that tick
the body is committed, previously Supported, on a `Face` and below its hover, so the redirect removes the velocity
into the face. Its up channel is then 0 against −0.1633 m/s without the redirect, and the servo's damping adds
0.2593 m/s of world-vertical velocity against 0.4146 m/s. World up has an up-slope part on a 30° face, so without
the redirect the body keeps more speed along the face (3.3737 against 3.3036 m/s on arcTop's tick 110). The arcTop
run therefore stops 0.225 m short of the rim with the redirect, against 0.218 m without. At the shorter stop the
capsule probe's contact lies on the arc's 60° hull facet (exact face 60°, GJK normal −29°). That is an edge
contact, so the estimate sweeps the centre probe and takes the platform's top face (the frame's tilt prints −30°):
the tick is `Face` and Supported. The geometric `Lip` counter (a walkable sweep over an unwalkable exact face)
counts every such tick: 84 of the 85 ticks from 116 to 200 (tick 117's contact is on the top face). Without the redirect the body rests on the 30° face itself, and only ticks 114 and 115 pass over the
facet. arcTopSeam stops 0.220 m short with the redirect and 0.218 m without, and here it is the run without the
redirect that crosses the facet, on tick 115 only. Both runs stay on the platform, Supported, with no flip, lift or
change of outcome under either expression.

**Measured** (task 37, OGBrawlerTests):
- both goldens exact against task 36's recordings, with `[GoldenTrace.signedZero] p=0 d=0 a=0 q=0 s=0 2=0`;
- the support-law scorecard's and PlatformEdge's 7,292 `law=v2` lines, run with OG_SUPPORT_LAW_RUNS=1,
  byte-identical to task 36's. The rigs keep the `law=v2` label so that each line still compares with the two-law
  record;
- the GroundAlignedKnockback v2 prints byte-identical; `[GroundAlignedKnockback.detachedBasis]`, which ran the
  Legacy law by default, prints the same line under the one law;
- `[@og]` 959 → 939 cases: the cases that set the Legacy law or tested a deleted function were deleted, and two that
  set both laws keep their v2 half under a new name.

---

## §X — Coverage

This document carries **every** comment line that left `BrawlerMovementSimulation.h`, except:

| excluded | lines | where it went |
|---|---|---|
| the SPDX licence line | 1 | stayed in the header |
| horizontal rules (`////…`) | 23 | deleted — a rule carries no claim |
| the 11 live fences | 57 | `BrawlerMovementSimulation-guards.md`, G-01…G-11 |
| the 10 retired fences | 32 | `BrawlerMovementSimulation-guards.md`, §R, G-12…G-21 |
| **carried here** | **1011** | this document |
| | **1124** | = every comment line in the pre-conversion header |

⛔ **That table is derived from the same script that wrote this file and therefore proves
nothing on its own.** The check that does prove it is a separate reconciliation that reads only
the shipped header and these two shipped documents, and asserts in both directions that no
comment line was lost and that every fence quoted in the guards doc is byte-identical to the
bytes it left. It has four poison arms and all four fire. ⚠ That instrument is initiative
working material and is not distributed with this submodule; what ships here is the result.

// SPDX-License-Identifier: BUSL-1.1
#pragma once
#include "OGSimulation/OGExport.h"
#include <atomic>
#include <string>

namespace dAttackMachineSimulation
{

enum class MovementScheme : uint32_t
{
	CameraRelative = 0,
	AimRelative    = 1,
	// Movement direction is camera-relative (like CameraRelative). Aim direction is
	// rotated relative to the current movement direction — aim-stick-up makes the aim
	// direction equal the movement direction. With no movement input, aim falls back to
	// camera-relative interpretation of the aim stick. UE-side only; sim doesn't read it.
	MoveRelativeAim = 2,
};

OGBRAWLER_API extern std::atomic<MovementScheme> g_movementScheme;

// Stick deadzones — magnitudes below these are treated as no input by the UE
// input-collection layer (Move(), buildMoveDirectionWorld, buildAimDirection).
// Sim layer does not read these; they are tweakables-by-convention living next to
// g_movementScheme so the named-pipe SetVariable path can tune them at runtime.
OGBRAWLER_API extern std::atomic<float> g_moveStickDeadzone;
OGBRAWLER_API extern std::atomic<float> g_aimStickDeadzone;

// When true, the physical stick assignments are swapped: the right (aim) stick feeds
// the move direction and the left (move) stick feeds the aim direction. UE-side only;
// the sim sees the resulting (moveStick, aimDirection, moveDirectionWorld) tuple either
// way and does not read this flag.
OGBRAWLER_API extern std::atomic<bool> g_swapMoveAndAimSticks;

// Gamepad-only feature toggle: when true (default), if the aim stick is below its
// deadzone and the most recent move input came from the gamepad left stick, the move
// stick feeds BOTH the movement direction and the aim direction (both resolve to the
// same camera-relative move-stick vector). When false, the gamepad case falls through
// to mouse aim / camera forward like any other input source. Keyboard+mouse play is
// unaffected either way (the latch m_lastMoveInputWasGamepad gates this).
OGBRAWLER_API extern std::atomic<bool> g_gamepadMoveStickFeedsAim;

// ⭐⭐ NAMES THIS PATH REFUSES, AND WHY — movement-sim TASK 16, 2026-09-08. The
// obligation was ROUTED HERE FROM TASK 56: ruling #28 removed `maxSnapSpeed`, and task 56 had
// no cvar/ini path to reject it against, so it left the breadcrumb at `hoverMaxAccel`'s
// declaration in BrawlerMovementSimulation.h and handed the work to this task.
//
// ⛔ THE DEFECT THIS EXISTS TO KILL: a stale ini or a stale named-pipe script naming a
// constant that NO LONGER EXISTS used to be indistinguishable from a typo — both fell off the
// end of `SetVariable` and returned `false`, and on the UE side an unregistered cvar name is
// dropped by the console with no diagnostic at all. The operator then believes they tuned
// something. A name that USED to mean something must fail LOUDLY, not silently.
//
// ⭐ ONE TABLE, BOTH VECTORS. `SetVariable` (the named-pipe path, below) consults it, and the
// UE layer (`MovementSchemeCVar.cpp`) walks it to (a) register a tombstone `OGBrawler.<name>`
// console variable whose sink logs an Error, and (b) scan `[ConsoleVariables]` in the shipped
// ini for `OGBrawler.<name>` at the one-time read. Adding a name here arms all three.
//
// ⚠ `refusedOnCVarPath` is FALSE for a name that is dead on THIS path but alive as a cvar.
// `MoveSpeed` is exactly that: task 16 deleted the sim-side walk-speed global and made it a
// ONE-TIME read of the live `OGBrawler.MoveSpeed` cvar into the movement sub-simulation's
// StaticData, so the named-pipe name is dead while the cvar name must keep working.
struct RefusedVariable
{
    // Matched CASE-INSENSITIVELY: a stale script's casing is not something to bet on.
    const char* name;
    // Printed verbatim in the rejection. Says what happened to the constant and where the
    // value lives now, because "unknown variable" is what made the old behaviour useless.
    const char* reason;
    // True ⇒ the UE layer also tombstones `OGBrawler.<name>` and scans the ini for it.
    bool        refusedOnCVarPath;
};

// Half-open range over the refused table. Arrays rather than a container so the UE layer can
// walk it without agreeing on an allocator across the module boundary.
OGBRAWLER_API const RefusedVariable* refusedVariablesBegin();
OGBRAWLER_API const RefusedVariable* refusedVariablesEnd();

// The reason `name` is refused, or nullptr when the name is not a refused one. ⛔ A nullptr
// return does NOT mean the name is live — it means it is not KNOWN-dead; an ordinary typo also
// returns nullptr. That difference is the whole point: a refused name is discriminated from a
// typo by a non-null reason, and `SetVariable` reports the two differently.
OGBRAWLER_API const char* refusedVariableReason(const std::string& name);

OGBRAWLER_API bool SetVariable(const std::string& name, const std::string& value);

}

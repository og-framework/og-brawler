// SPDX-License-Identifier: BUSL-1.1
#include "DAttackMachineSimulationRuntimeTweakables.h"
#include <algorithm>
#include <cctype>   // std::tolower - the refused-name table matches case-insensitively
#include <cstdio>
#include <cstdlib>  // std::strtof — exception-free float parsing (UE server/release
                    // builds disable exceptions via bEnableExceptions = false, so the
                    // try/catch around std::stof we used previously fails to compile
                    // with C4530 "exception handler used, but unwind semantics are not
                    // enabled").

namespace dAttackMachineSimulation
{
    std::atomic<MovementScheme> g_movementScheme = MovementScheme::AimRelative;
    std::atomic<float>          g_moveStickDeadzone = 0.15f;
    std::atomic<float>          g_aimStickDeadzone  = 0.2f;
    std::atomic<bool>           g_swapMoveAndAimSticks = false;
    std::atomic<bool>           g_gamepadMoveStickFeedsAim = true;

    namespace {
        // ⭐⭐ THE REFUSED-NAME TABLE — movement-sim TASK 16, obligation ROUTED FROM TASK 56.
        // ⛔ EVERY ENTRY IS A NAME THAT WAS VERIFIED ABSENT (or verified authored-only) ON THE
        // TREE THIS BUILDS AGAINST, not copied from a document. Checked 2026-09-08 against
        // `brawlerMovementSimulation::StaticData` in BrawlerMovementSimulation.h and the
        // construction site in SimulatableBrawlerTypes.h, because ruling #29 (task 57) had
        // already ADDED `hoverPullDownAccel` after task 56 wrote the breadcrumb — a list taken
        // from any document would have been one constant out of date on the day it was written.
        //
        // ⚠ A name is refused for ONE OF TWO REASONS, and the reason text says which:
        //   RETIRED       — the constant is GONE (`maxSnapSpeed`, ruling #28).
        //   AUTHORED-ONLY — the constant EXISTS but is deliberately not tunable from outside.
        constexpr RefusedVariable kRefusedVariables[] = {
            // RETIRED. Ruling #28 (movement-sim task 56) replaced ruling #13's dead-beat
            // velocity assignment with a spring-damper, and `maxSnapSpeed` was the VELOCITY
            // clamp on the assignment that no longer exists. Its role-successor bounds an
            // ACCELERATION, so the old number does not even carry over rescaled.
            { "MaxSnapSpeed",
              "RETIRED by user ruling #28 (movement-sim task 56): the dead-beat velocity "
              "assignment it clamped no longer exists. The hover servo's ceiling is now the "
              "ACCELERATION bound 'hoverMaxAccel', authored in SimulatableBrawlerTypes.h. "
              "A velocity in cm/s does not carry over to it - delete this line.",
              true },
            // RETIRED on THIS path only; the cvar of the same name is live and read once.
            { "MoveSpeed",
              "RETIRED as a live named-pipe tunable by movement-sim task 16: the sim-side walk-speed "
              "global is deleted and the walk speed is a ONE-TIME read of 'OGBrawler.MoveSpeed' "
              "console variable into the movement sub-simulation's StaticData. Set that cvar "
              "from an ini before the session starts; there is no mid-session walk speed.",
              false },
            // ⭐⭐ AUTHORED-ONLY, AND THE REASON IS THE SECOND ROUTED OBLIGATION. The hover
            // gains' valid region is TWO-DIMENSIONAL - stability is `W^2 + 4*zeta*W < 4` with
            // `W = omega*dt`, and a monotone (non-ringing) response needs `omega < 1/dt` on top
            // of it. A per-variable range check on omega ALONE cannot express that: the obvious
            // `hoverFrequency < 120` admits omega=60/zeta=1, which DIVERGES 1,1,2,3,5,8,13.
            // The coupled check already exists, ONCE, where the gains are AUTHORED - the
            // `OG_CHECK` in `brawlerMovementSimulation::StaticData`'s constructor - and every
            // value that can ever reach the sim passes through it. Exposing these means moving
            // that check to where a cvar can reuse it, NOT adding a second, weaker one here.
            { "HoverFrequency",
              "NOT TUNABLE FROM OUTSIDE: omega is authored in SimulatableBrawlerTypes.h. Its "
              "valid region is COUPLED to hoverDampingRatio (stability W^2+4*zeta*W < 4 with "
              "W = omega*dt; monotonicity omega < 1/dt), so a per-variable range check on omega "
              "alone is wrong and the coupled OG_CHECK lives in the StaticData constructor.",
              true },
            { "HoverDampingRatio",
              "NOT TUNABLE FROM OUTSIDE: zeta is authored in SimulatableBrawlerTypes.h, and it "
              "is NOT 1 - the discrete critical value is 1 - omega*dt/2. Its valid region is "
              "COUPLED to hoverFrequency, so the check belongs in the StaticData constructor "
              "where both are in scope, not on a single console variable.",
              true },
        };

        bool equalsIgnoringCase(const char* a, const std::string& b)
        {
            std::size_t i = 0;
            for (; a[i] != '\0' && i < b.size(); ++i)
            {
                const unsigned char ca = static_cast<unsigned char>(a[i]);
                const unsigned char cb = static_cast<unsigned char>(b[i]);
                if (std::tolower(ca) != std::tolower(cb))
                    return false;
            }
            return a[i] == '\0' && i == b.size();
        }

        bool parseClampedFloat(const std::string& value, std::atomic<float>& target, const char* name)
        {
            char* endPtr = nullptr;
            const float parsed = std::strtof(value.c_str(), &endPtr);
            if (endPtr == value.c_str())
            {
                fprintf(stderr, "[OGBrawler] SetVariable: '%s' could not parse value '%s' as float\n", name, value.c_str());
                return false;
            }
            target = std::clamp(parsed, 0.f, 1.f);
            return true;
        }
    }

    const RefusedVariable* refusedVariablesBegin() { return kRefusedVariables; }
    const RefusedVariable* refusedVariablesEnd()
    { return kRefusedVariables + sizeof(kRefusedVariables) / sizeof(kRefusedVariables[0]); }

    const char* refusedVariableReason(const std::string& name)
    {
        for (const RefusedVariable* v = refusedVariablesBegin(); v != refusedVariablesEnd(); ++v)
            if (equalsIgnoringCase(v->name, name))
                return v->reason;
        return nullptr;
    }

    bool SetVariable(const std::string& name, const std::string& value)
    {
        // ⛔ THE REFUSED CHECK RUNS FIRST, BEFORE EVERY LIVE NAME. A refused name must never
        // be shadowed by a live handler, and putting this last would let a future handler with
        // the same spelling silently resurrect a dead constant. It is also what makes the
        // rejection DISTINGUISHABLE from a typo: a typo falls off the end of this function and
        // returns false with no explanation at all; a refused name returns false having printed
        // WHAT HAPPENED to the constant and WHERE the value lives now.
        if (const char* reason = refusedVariableReason(name))
        {
            fprintf(stderr,
                "[OGBrawler] SetVariable: REFUSED '%s' (value '%s') - %s\n",
                name.c_str(), value.c_str(), reason);
            return false;
        }
        if (name == "MovementScheme")
        {
            if (value == "AimRelative")     { g_movementScheme = MovementScheme::AimRelative;     return true; }
            if (value == "CameraRelative")  { g_movementScheme = MovementScheme::CameraRelative;  return true; }
            if (value == "MoveRelativeAim") { g_movementScheme = MovementScheme::MoveRelativeAim; return true; }
            // Legacy numeric forms ("1" = AimRelative, "3" = CameraRelative).
            if (value == "1") { fprintf(stderr, "[OGBrawler] SetVariable: legacy value '1' for MovementScheme; use 'AimRelative'\n");    g_movementScheme = MovementScheme::AimRelative;    return true; }
            if (value == "3") { fprintf(stderr, "[OGBrawler] SetVariable: legacy value '3' for MovementScheme; use 'CameraRelative'\n"); g_movementScheme = MovementScheme::CameraRelative; return true; }
            return false;
        }
        if (name == "MoveStickDeadzone")
        {
            return parseClampedFloat(value, g_moveStickDeadzone, "MoveStickDeadzone");
        }
        if (name == "AimStickDeadzone")
        {
            return parseClampedFloat(value, g_aimStickDeadzone, "AimStickDeadzone");
        }
        if (name == "SwapMoveAndAimSticks")
        {
            if (value == "0" || value == "false" || value == "False") { g_swapMoveAndAimSticks = false; return true; }
            if (value == "1" || value == "true"  || value == "True")  { g_swapMoveAndAimSticks = true;  return true; }
            fprintf(stderr, "[OGBrawler] SetVariable: 'SwapMoveAndAimSticks' expects 0/1/true/false; got '%s'\n", value.c_str());
            return false;
        }
        if (name == "GamepadMoveStickFeedsAim")
        {
            if (value == "0" || value == "false" || value == "False") { g_gamepadMoveStickFeedsAim = false; return true; }
            if (value == "1" || value == "true"  || value == "True")  { g_gamepadMoveStickFeedsAim = true;  return true; }
            fprintf(stderr, "[OGBrawler] SetVariable: 'GamepadMoveStickFeedsAim' expects 0/1/true/false; got '%s'\n", value.c_str());
            return false;
        }
        // Legacy variable name — still accepted with a warning.
        if (name == "MovementAndAimModeTest")
        {
            fprintf(stderr, "[OGBrawler] SetVariable: 'MovementAndAimModeTest' is deprecated; use 'MovementScheme' with 'AimRelative' or 'CameraRelative'\n");
            if (value == "1") { g_movementScheme = MovementScheme::AimRelative;    return true; }
            if (value == "3") { g_movementScheme = MovementScheme::CameraRelative; return true; }
            return false;
        }
        return false;
    }
}

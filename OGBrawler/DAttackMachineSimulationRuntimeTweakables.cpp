// SPDX-License-Identifier: BUSL-1.1
#include "DAttackMachineSimulationRuntimeTweakables.h"
#include <algorithm>
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
    std::atomic<float>          g_moveSpeed = 80.f;
    std::atomic<bool>           g_gamepadMoveStickFeedsAim = true;

    namespace {
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

    bool SetVariable(const std::string& name, const std::string& value)
    {
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
        if (name == "MoveSpeed")
        {
            char* endPtr = nullptr;
            const float parsed = std::strtof(value.c_str(), &endPtr);
            if (endPtr == value.c_str())
            {
                fprintf(stderr, "[OGBrawler] SetVariable: 'MoveSpeed' could not parse value '%s' as float\n", value.c_str());
                return false;
            }
            g_moveSpeed = std::max(0.f, parsed);
            return true;
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

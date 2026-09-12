// SPDX-License-Identifier: BUSL-1.1
// docs/DAttackMachineSimulationRuntimeTweakables-rationale.md · docs/DAttackMachineSimulationRuntimeTweakables-guards.md
#pragma once
#include "OGSimulation/OGExport.h"
#include <atomic>
#include <string>
#include <type_traits>

namespace dAttackMachineSimulation
{

enum class MovementScheme : uint32_t
{
	CameraRelative = 0,
	AimRelative    = 1,
	MoveRelativeAim = 2,
};

OGBRAWLER_API extern std::atomic<MovementScheme> g_movementScheme;

// ⛔G-01  docs/DAttackMachineSimulationRuntimeTweakables-guards.md
OGBRAWLER_API extern std::atomic<float> g_moveStickDeadzone;
OGBRAWLER_API extern std::atomic<float> g_aimStickDeadzone;

OGBRAWLER_API extern std::atomic<bool> g_swapMoveAndAimSticks;

OGBRAWLER_API extern std::atomic<bool> g_gamepadMoveStickFeedsAim;

// ⛔G-02  docs/DAttackMachineSimulationRuntimeTweakables-guards.md
struct RefusedVariable
{
    // ⛔G-04  docs/DAttackMachineSimulationRuntimeTweakables-guards.md
    const char* name;
    const char* reason;
    // ⛔G-03  docs/DAttackMachineSimulationRuntimeTweakables-guards.md
    bool        refusedOnCVarPath;
};

OGBRAWLER_API const RefusedVariable* refusedVariablesBegin();
OGBRAWLER_API const RefusedVariable* refusedVariablesEnd();

static_assert(std::is_same_v<decltype(refusedVariablesBegin()), const RefusedVariable*>
           && std::is_same_v<decltype(refusedVariablesEnd()),   const RefusedVariable*>,
    "dAttackMachineSimulation: the refused table must stay a RAW half-open "
    "`const RefusedVariable*` pair. A std::span or a std::vector getter is the reflex "
    "modernisation and it cannot work here: MovementSchemeCVar.cpp walks this table from "
    "the UE module, and a container in the signature would make the two modules agree on "
    "an allocator across a boundary neither side can see. Was fence T2b-4, guard G-05, "
    "now retired.");

// ⛔G-06  docs/DAttackMachineSimulationRuntimeTweakables-guards.md
OGBRAWLER_API const char* refusedVariableReason(const std::string& name);

OGBRAWLER_API bool SetVariable(const std::string& name, const std::string& value);

}

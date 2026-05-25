// SPDX-License-Identifier: BUSL-1.1
#include "DAttackMachineSimulationRuntimeTweakables.h"

namespace dAttackMachineSimulation 
{
    std::atomic<unsigned int> MovementAndAimModeTest = 3;

    bool SetVariable(const std::string& name, const std::string& value)
    {
        if (name == "MovementAndAimModeTest")
        {
            MovementAndAimModeTest = std::stoi(value);
            return true;
        }
        return false;
    }
}
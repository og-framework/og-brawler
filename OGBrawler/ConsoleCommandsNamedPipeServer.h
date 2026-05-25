#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include "OGSimulation/OGExport.h"

#ifdef _WIN32

#include <atomic>
#include <string>

namespace DAttackPipeServer
{

OGBRAWLER_API void NamedPipeServer();

}

#endif // _WIN32

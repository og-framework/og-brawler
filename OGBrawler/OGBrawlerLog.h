#pragma once
// SPDX-License-Identifier: BUSL-1.1

#include <cstdio>
#include <functional>

// Game-rule logging sink for the OGBrawler plugin (DAttackMachine, DAttackRadial,
// future DAttackGuard). Distinct from OGSimulation/SimulationLog.h's SIMLOG_G,
// which is reserved for the framework's tick/sync/reconciliation messages.
//
// At the UE instantiation site (ASimulationManagerUImpl::BeginPlay), the logger
// is set to route messages through UE_LOG(LogOGBrawler, ...). Toggle verbosity
// via DefaultEngine.ini under [Core.Log]:
//
//     LogOGBrawler=Warning   ; default — silence per-tick spam
//     LogOGBrawler=Verbose   ; enable while debugging attack/machine logic
//
// Prefix a message with "[Verbose]" or "[Warning]" to pick a non-default
// verbosity for that single line (mirrors SIMLOG_G's convention).

namespace ogblog
{
    inline std::function<void(const char*)> g_sink;

    inline void setGlobal(std::function<void(const char*)> fn) { g_sink = std::move(fn); }
}

#define OGBLOG_G(fmt, ...) \
    do { \
        if (::ogblog::g_sink) { \
            char _ogblog_g_buf[256]; \
            std::snprintf(_ogblog_g_buf, sizeof(_ogblog_g_buf), (fmt) __VA_OPT__(,) __VA_ARGS__); \
            ::ogblog::g_sink(_ogblog_g_buf); \
        } \
    } while (0)

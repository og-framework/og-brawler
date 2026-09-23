#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/OGBrawlerLog-rationale.md · docs/OGBrawlerLog-guards.md

#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <functional>
#include <type_traits>

#include "OGSimulation/SimulationLog.h"

namespace ogblog
{
    inline std::function<void(const char*)> g_sink;

    inline void setGlobal(std::function<void(const char*)> fn) { g_sink = std::move(fn); }

    inline constexpr std::size_t kLineBufferBytes = 1024;

    inline constexpr std::size_t kMaxStringArgBytes = 32;

    namespace detail
    {
        inline constexpr std::size_t kUnbounded = static_cast<std::size_t>(-1) / 2;

        enum class ArgClass : unsigned char { Int32, Float, Double, CString, Unsupported };

        template <typename T>
        constexpr ArgClass classifyArg()
        {
            if constexpr (std::is_enum_v<T> || std::is_integral_v<T>)
                return sizeof(T) <= 4 ? ArgClass::Int32 : ArgClass::Unsupported;
            else if constexpr (std::is_same_v<T, float>)
                return ArgClass::Float;
            else if constexpr (std::is_same_v<T, double> || std::is_same_v<T, long double>)
                return ArgClass::Double;
            else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
                return ArgClass::CString;
            else
                return ArgClass::Unsupported;
        }

        template <typename... Ts>
        struct TypeList {};

        template <typename... Ts>
        TypeList<std::remove_cv_t<std::decay_t<Ts>>...> typesOf(Ts&&...);

        constexpr std::size_t worstCaseFormattedLengthOf(
            const char* fmt, const ArgClass* args, std::size_t argCount)
        {
            std::size_t total = 0;
            std::size_t nextArg = 0;
            for (const char* p = fmt; *p != '\0'; ++p)
            {
                if (*p != '%') { ++total; continue; }
                ++p;
                if (*p == '%') { ++total; continue; }

                bool alternate = false;
                while (*p == '-' || *p == '+' || *p == ' ' || *p == '0' || *p == '#')
                {
                    alternate = alternate || *p == '#';
                    ++p;
                }
                std::size_t width = 0;
                while (*p >= '0' && *p <= '9') { width = width * 10 + static_cast<std::size_t>(*p - '0'); ++p; }
                bool hasPrecision = false;
                std::size_t precision = 0;
                if (*p == '.')
                {
                    hasPrecision = true;
                    ++p;
                    while (*p >= '0' && *p <= '9') { precision = precision * 10 + static_cast<std::size_t>(*p - '0'); ++p; }
                }
                if (nextArg >= argCount)
                    return kUnbounded;
                const ArgClass arg = args[nextArg++];

                std::size_t bound = kUnbounded;
                // ∴D-01  docs/OGBrawlerLog-rationale.md
                switch (*p)
                {
                case 'd': case 'i':
                    if (arg == ArgClass::Int32) bound = 11;
                    break;
                case 'u':
                    if (arg == ArgClass::Int32) bound = 10;
                    break;
                case 'x': case 'X':
                    if (arg == ArgClass::Int32) bound = 8 + (alternate ? 2 : 0);
                    break;
                case 'c':
                    if (arg == ArgClass::Int32) bound = 1;
                    break;
                case 'f': case 'F':
                {
                    const std::size_t fraction = hasPrecision ? precision : 6;
                    const std::size_t point = (fraction > 0 || alternate) ? 1 : 0;
                    if (arg == ArgClass::Float)  bound = 1 + 39 + point + fraction;
                    if (arg == ArgClass::Double) bound = 1 + 309 + point + fraction;
                    break;
                }
                case 's':
                    if (arg == ArgClass::CString)
                        bound = (hasPrecision && precision < kMaxStringArgBytes) ? precision : kMaxStringArgBytes;
                    break;
                default:
                    break;
                }
                if (bound == kUnbounded)
                    return kUnbounded;
                total += bound > width ? bound : width;
            }
            return nextArg == argCount ? total : kUnbounded;
        }

        template <typename... Ts>
        constexpr std::size_t worstCaseFormattedLength(const char* fmt, TypeList<Ts...>)
        {
            constexpr ArgClass classes[] = { classifyArg<Ts>()..., ArgClass::Unsupported };
            return worstCaseFormattedLengthOf(fmt, classes, sizeof...(Ts));
        }
    }

    inline constexpr std::size_t kIntegrateScopePrefixMaxBytes =
        detail::worstCaseFormattedLength("id=%u tick=%u ", detail::TypeList<unsigned int, uint32_t>{});
    static_assert(kIntegrateScopePrefixMaxBytes == 30);

    namespace detail
    {
        template <typename... Ts>
        constexpr bool fitsLineBuffer(const char* fmt, TypeList<Ts...> types)
        {
            return worstCaseFormattedLength(fmt, types) + kIntegrateScopePrefixMaxBytes < kLineBufferBytes;
        }

        constexpr bool startsWith(const char* line, const char* prefix)
        {
            for (; *prefix != '\0'; ++line, ++prefix)
                if (*line != *prefix)
                    return false;
            return true;
        }

        constexpr std::size_t integrateScopePrefixOffset(const char* line)
        {
            std::size_t tagStart = 0;
            if (startsWith(line, "[Verbose]") || startsWith(line, "[Warning]"))
                tagStart = 9;
            if (line[tagStart] != '[')
                return tagStart;
            for (std::size_t i = tagStart + 1; line[i] != '\0'; ++i)
            {
                if (line[i] == ']')
                    return line[i + 1] == ' ' ? i + 2 : i + 1;
            }
            return tagStart;
        }

        static_assert(integrateScopePrefixOffset("[Machine.transition] Idle") == 21);
        static_assert(integrateScopePrefixOffset("[Verbose][Radial.branch] idle") == 25);
        static_assert(integrateScopePrefixOffset("[Warning][Movement.hover] x") == 26);
        static_assert(integrateScopePrefixOffset("no tag here") == 0);
        static_assert(integrateScopePrefixOffset("[Verbose]no tag") == 9);
        static_assert(integrateScopePrefixOffset("[unclosed tag") == 0);
        static_assert(integrateScopePrefixOffset("") == 0);

        // ⛔G-03  docs/OGBrawlerLog-guards.md
#if defined(__clang__) || defined(__GNUC__)
        __attribute__((format(printf, 1, 2)))
#endif
        inline void emitFormatted(const char* fmt, ...)
        {
            char buffer[kLineBufferBytes];
            va_list args;
            va_start(args, fmt);
            const int written = std::vsnprintf(buffer, sizeof(buffer), fmt, args);
            va_end(args);

            // ⛔G-02  docs/OGBrawlerLog-guards.md
            if (const auto scope = ::simulationLog::currentIntegrateScope(); scope && written >= 0)
            {
                char prefix[kIntegrateScopePrefixMaxBytes + 1];
                const int prefixLength = std::snprintf(prefix, sizeof(prefix), "id=%u tick=%u ", scope->id, scope->tick);
                const std::size_t lineLength = std::strlen(buffer);
                const std::size_t at = integrateScopePrefixOffset(buffer);
                if (prefixLength > 0 && lineLength + static_cast<std::size_t>(prefixLength) < sizeof(buffer))
                {
                    std::memmove(buffer + at + prefixLength, buffer + at, lineLength - at + 1);
                    std::memcpy(buffer + at, prefix, static_cast<std::size_t>(prefixLength));
                }
            }
            g_sink(buffer);
        }

        static_assert(worstCaseFormattedLength("%u", TypeList<unsigned int>{}) == 10);
        static_assert(worstCaseFormattedLength("%d", TypeList<int>{}) == 11);
        static_assert(worstCaseFormattedLength("%d", TypeList<bool>{}) == 11);
        static_assert(worstCaseFormattedLength("%.4f", TypeList<float>{}) == 45);
        static_assert(worstCaseFormattedLength("%.0f", TypeList<float>{}) == 40);
        static_assert(worstCaseFormattedLength("%.4f", TypeList<double>{}) == 315);
        static_assert(worstCaseFormattedLength("%s", TypeList<const char*>{}) == kMaxStringArgBytes);
        static_assert(worstCaseFormattedLength("a%%b", TypeList<>{}) == 3);
        static_assert(worstCaseFormattedLength("x=%u y=%s", TypeList<unsigned int, const char*>{}) == 2 + 10 + 3 + kMaxStringArgBytes);
        static_assert(worstCaseFormattedLength("%u", TypeList<>{}) == kUnbounded);
        static_assert(worstCaseFormattedLength("%u", TypeList<unsigned int, unsigned int>{}) == kUnbounded);
        static_assert(worstCaseFormattedLength("%llu", TypeList<unsigned long long>{}) == kUnbounded);
        static_assert(worstCaseFormattedLength("%u", TypeList<unsigned long long>{}) == kUnbounded);
        static_assert(worstCaseFormattedLength("%e", TypeList<float>{}) == kUnbounded);
        static_assert(worstCaseFormattedLength("%s", TypeList<int>{}) == kUnbounded);
        static_assert(!fitsLineBuffer("%u", TypeList<>{}));
    }
}

// ⛔G-01  docs/OGBrawlerLog-guards.md
#define OGBLOG_G(fmt, ...) \
    do { \
        static_assert(::ogblog::detail::fitsLineBuffer((fmt), \
                decltype(::ogblog::detail::typesOf(__VA_ARGS__)){}), \
            "OGBLOG_G: this format's worst case does not fit ogblog::kLineBufferBytes, or it " \
            "uses a conversion/argument type the clip check cannot bound. The line would be " \
            "truncated SILENTLY. See OGBrawler/docs/OGBrawlerLog-guards.md, G-01."); \
        if (::ogblog::g_sink) { \
            ::ogblog::detail::emitFormatted((fmt) __VA_OPT__(,) __VA_ARGS__); \
        } \
    } while (0)

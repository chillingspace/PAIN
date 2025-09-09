#pragma once

#include <memory>
#include "Core.h"
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace PAIN {

    class Log
    {
    public:
        // Initialize the loggers
        static void PAIN_API Init()
        {
            spdlog::set_pattern("%^[%T] %n: %v%$");

            s_CoreLogger = spdlog::stdout_color_mt("PAIN_CORE");
            s_CoreLogger->set_level(spdlog::level::trace);

            s_ClientLogger = spdlog::stdout_color_mt("PAIN_APP");
            s_ClientLogger->set_level(spdlog::level::trace);
        }

        // Accessors
        inline static PAIN_API std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
        inline static PAIN_API std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }

    private:
        // Inline static members (C++17+)
        inline static PAIN_API std::shared_ptr<spdlog::logger> s_CoreLogger;
        inline static PAIN_API std::shared_ptr<spdlog::logger> s_ClientLogger;
    };

} // namespace PAIN

// ----------------------
// Core log macros
// ----------------------
#define PN_CORE_TRACE(...)    ::PAIN::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PN_CORE_INFO(...)     ::PAIN::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PN_CORE_WARN(...)     ::PAIN::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PN_CORE_ERROR(...)    ::PAIN::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PN_CORE_FATAL(...)    ::PAIN::Log::GetCoreLogger()->fatal(__VA_ARGS__)

// ----------------------
// Client log macros
// ----------------------
#define PN_TRACE(...)    ::PAIN::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PN_INFO(...)     ::PAIN::Log::GetClientLogger()->info(__VA_ARGS__)
#define PN_WARN(...)     ::PAIN::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PN_ERROR(...)    ::PAIN::Log::GetClientLogger()->error(__VA_ARGS__)
#define PN_FATAL(...)    ::PAIN::Log::GetClientLogger()->fatal(__VA_ARGS__)

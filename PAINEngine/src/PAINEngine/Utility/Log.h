#pragma once

#include <memory> 
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#ifdef PN_PLATFORM_ANDROID
#include <spdlog/sinks/android_sink.h>
#include <android/log.h>
#endif

namespace PAIN {

	class Log
	{
	public:
		static void Init();
		static void Shutdown();

		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetClientLogger() { return s_ClientLogger; }
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
		static bool s_Initialized;

	};

	void LogMemoryFullDiagnostic(const char* label);
}


// Core log macros 
#define PN_CORE_TRACE(...)    ::PAIN::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define PN_CORE_INFO(...)     ::PAIN::Log::GetCoreLogger()->info(__VA_ARGS__)
#define PN_CORE_WARN(...)     ::PAIN::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define PN_CORE_ERROR(...)    ::PAIN::Log::GetCoreLogger()->error(__VA_ARGS__)
#define PN_CORE_FATAL(...)    ::PAIN::Log::GetCoreLogger()->fatal(__VA_ARGS__)

//Client log macros
#define PN_TRACE(...)    ::PAIN::Log::GetClientLogger()->trace(__VA_ARGS__)
#define PN_INFO(...)     ::PAIN::Log::GetClientLogger()->info(__VA_ARGS__)
#define PN_WARN(...)     ::PAIN::Log::GetClientLogger()->warn(__VA_ARGS__)
#define PN_ERROR(...)    ::PAIN::Log::GetClientLogger()->error(__VA_ARGS__)
#define PN_FATAL(...)    ::PAIN::Log::GetClientLogger()->fatal(__VA_ARGS__)

// Log.h (debug-only assert wrapper)
#ifndef NDEBUG
#define PN_DEBUG_BREAK() do { assert(false && "PN_FATAL triggered"); } while(0)
#else
#define PN_DEBUG_BREAK() ((void)0)
#endif

// Then make convenience macros:
#define PN_CORE_FATAL_AND_BREAK(...) do { PN_CORE_FATAL(__VA_ARGS__); PN_DEBUG_BREAK(); } while(0)
#define PN_FATAL_AND_BREAK(...)      do { PN_FATAL(__VA_ARGS__);      PN_DEBUG_BREAK(); } while(0)

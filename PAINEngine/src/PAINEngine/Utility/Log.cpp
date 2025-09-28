#include "pch.h"
#include "Log.h"

namespace PAIN {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	void Log::Init() {
#ifdef PN_PLATFORM_ANDROID
        auto sink = std::make_shared<spdlog::sinks::android_sink_mt>("PAIN"); // Logcat tag
#else
        auto sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
#endif

        // Match your Windows format
        // Example: [12:34:56] PAIN_CORE I: message
        // %^ .. %$ enables color on color sinks; android sink ignores color codes safely.
        const char* pattern = "%^[%T] %n %l: %v%$";

        s_CoreLogger = std::make_shared<spdlog::logger>("PAIN", sink);
        s_ClientLogger = std::make_shared<spdlog::logger>("APP", sink);

        s_CoreLogger->set_level(spdlog::level::trace);
        s_ClientLogger->set_level(spdlog::level::trace);

        s_CoreLogger->set_pattern(pattern);
        s_ClientLogger->set_pattern(pattern);

        spdlog::set_default_logger(s_CoreLogger);
        spdlog::flush_on(spdlog::level::warn);

	}

}
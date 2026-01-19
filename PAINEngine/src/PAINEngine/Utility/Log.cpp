#include "pch.h"
#include "Log.h"

#ifdef PN_PLATFORM_ANDROID
#include <stdio.h>   // For popen, pclose, fopen, etc.
#include <unistd.h>  // For getpid()
#endif

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

    void LogMemory(const char* label) {
        // Only get CPU RAM (fast)
        FILE* file = fopen("/proc/self/status", "r");
        char line[256];
        long vm_rss = 0;
        if (file) {
            while (fgets(line, sizeof(line), file)) {
                if (sscanf(line, "VmRSS: %ld kB", &vm_rss) == 1) {
                    break;  // Stop after finding it
                }
            }
            fclose(file);
        }

        // Simple output (no dumpsys call)
        PN_CORE_INFO("[MEMORY] {} - CPU: {:.2f} MB", label, vm_rss / 1024.0f);
    }

    void LogMemoryFullDiagnostic(const char* label) {

        // CPU RAM
        FILE* file = fopen("/proc/self/status", "r");
        char line[256];
        long vm_rss = 0;
        if (file) {
            while (fgets(line, sizeof(line), file)) {
                sscanf(line, "VmRSS: %ld kB", &vm_rss);
            }
            fclose(file);
        }

#ifdef PN_PLATFORM_ANDROID
        // Graphics RAM
        long graphics_kb = 0;
        char cmd[256];
        sprintf(cmd, "dumpsys meminfo %d | grep Graphics", getpid());

        FILE* pipe = popen(cmd, "r");
        if (pipe) {
            char result[256];
            if (fgets(result, sizeof(result), pipe)) {
                sscanf(result, " Graphics: %ld", &graphics_kb);
            }
            pclose(pipe);
        }

        // Simple, clean output
        PN_CORE_INFO("[MEMORY] {} - CPU: {:.2f} MB | GPU: {:.2f} MB | Total: {:.2f} MB",
            label,
            vm_rss / 1024.0f,
            graphics_kb / 1024.0f,
            (vm_rss + graphics_kb) / 1024.0f);
#endif
    }

}
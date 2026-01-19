#include "pch.h"
#include "Log.h"

#ifdef PN_PLATFORM_ANDROID
#include <stdio.h>
#include <unistd.h>
#elif defined(PN_PLATFORM_WINDOWS)
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
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
        long cpu_mb = 0;

#ifdef PN_PLATFORM_ANDROID
        // Android: Read from /proc/self/status
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

        cpu_mb = vm_rss / 1024;  // Convert KB to MB

#elif defined(PN_PLATFORM_WINDOWS)
        // Windows: Use GetProcessMemoryInfo
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(),
            (PROCESS_MEMORY_COUNTERS*)&pmc,
            sizeof(pmc))) {
            // WorkingSetSize = current physical RAM usage (similar to RSS on Linux)
            cpu_mb = pmc.WorkingSetSize / (1024 * 1024);  // Convert bytes to MB
        }

#else
        // Other platforms (macOS, Linux desktop, etc.)
        cpu_mb = 0;  // Not implemented
#endif
        
       // PN_CORE_INFO("[MEMORY] {} - CPU: {:.2f} MB", label, cpu_mb);
    }

    void LogMemoryFullDiagnostic(const char* label) {
        long cpu_mb = 0;
        long gpu_mb = 0;
        long virtual_mb = 0;
        long peak_mb = 0;

#ifdef PN_PLATFORM_ANDROID
        // ANDROID - CPU RAM
        FILE* file = fopen("/proc/self/status", "r");
        char line[256];
        long vm_rss = 0;
        long vm_size = 0;
        long vm_peak = 0;

        if (file) {
            while (fgets(line, sizeof(line), file)) {
                if (sscanf(line, "VmRSS: %ld kB", &vm_rss) == 1) continue;
                if (sscanf(line, "VmSize: %ld kB", &vm_size) == 1) continue;
                if (sscanf(line, "VmPeak: %ld kB", &vm_peak) == 1) continue;
            }
            fclose(file);
        }

        cpu_mb = vm_rss / 1024;
        virtual_mb = vm_size / 1024;
        peak_mb = vm_peak / 1024;

        // ANDROID - GPU RAM (via dumpsys)
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

        gpu_mb = graphics_kb / 1024;

        // ANDROID - LOG OUTPUT
        PN_CORE_INFO("[MEMORY] {} - CPU: {:.2f} MB | GPU: {:.2f} MB | Virtual: {:.2f} MB | Peak: {:.2f} MB | Total: {:.2f} MB",
            label,
            (float)cpu_mb,
            (float)gpu_mb,
            (float)virtual_mb,
            (float)peak_mb,
            (float)(cpu_mb + gpu_mb));

#elif defined(PN_PLATFORM_WINDOWS)
        // WINDOWS - CPU RAM
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(),
            (PROCESS_MEMORY_COUNTERS*)&pmc,
            sizeof(pmc))) {
            // WorkingSetSize = physical RAM (RSS equivalent)
            cpu_mb = pmc.WorkingSetSize / (1024 * 1024);

            // PrivateUsage = committed memory (includes paged-out memory)
            virtual_mb = pmc.PrivateUsage / (1024 * 1024);

            // PeakWorkingSetSize = max physical RAM used
            peak_mb = pmc.PeakWorkingSetSize / (1024 * 1024);
        }

        // WINDOWS - GPU RAM (via DXGI or OpenGL extensions)
        // Note: GPU memory tracking on Windows requires vendor-specific APIs
        // For now, we'll leave it as 0 (requires DXGI or GL extensions)
        gpu_mb = 0;

        // TODO: Implement DXGI adapter query:
        // IDXGIAdapter3::QueryVideoMemoryInfo() for DirectX
        // Or GL_NVX_gpu_memory_info for NVIDIA OpenGL

        // WINDOWS - LOG OUTPUT
        PN_CORE_INFO("[MEMORY] {} - CPU: {:.2f} MB | Virtual: {:.2f} MB | Peak: {:.2f} MB | GPU: N/A",
            label,
            (float)cpu_mb,
            (float)virtual_mb,
            (float)peak_mb);

#else
        // OTHER PLATFORMS (macOS, Linux, etc.)
        PN_CORE_WARN("[MEMORY] {} - Platform not supported for detailed memory tracking", label);
#endif
    }

}
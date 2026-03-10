#include "pch.h"
#include "Log.h"

#ifdef _DEBUG
#include "../LayeredSystems/LevelEditor/Panels/DebugPanel.h"
#endif

#ifdef PN_PLATFORM_WINDOWS
#include <windows.h>
#include <psapi.h>
#pragma comment(lib, "psapi.lib")
#endif

#ifdef PN_PLATFORM_ANDROID
#include <unistd.h>
#include <cstdio>
#include <cstring>

// GL_NVX_gpu_memory_info extension constants
#ifndef GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#endif

#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#endif

#ifndef GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#endif

#ifndef GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX
#define GL_GPU_MEMORY_INFO_EVICTION_COUNT_NVX            0x904A
#endif

#ifndef GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_EVICTED_MEMORY_NVX            0x904B
#endif

// GL_ATI_meminfo extension constants
#ifndef GL_VBO_FREE_MEMORY_ATI
#define GL_VBO_FREE_MEMORY_ATI                           0x87FB
#endif

#ifndef GL_TEXTURE_FREE_MEMORY_ATI
#define GL_TEXTURE_FREE_MEMORY_ATI                       0x87FC
#endif

#ifndef GL_RENDERBUFFER_FREE_MEMORY_ATI
#define GL_RENDERBUFFER_FREE_MEMORY_ATI                  0x87FD
#endif
#endif

namespace PAIN {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

    // Helper function to check if OpenGL extension is supported
    static bool IsGLExtensionSupported(const char* extension) {
#ifdef PN_PLATFORM_ANDROID
        // Android/OpenGL ES: Use glGetString(GL_EXTENSIONS)
        const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
        if (!extensions) return false;

        // Search for extension in space-separated string
        const char* start = extensions;
        while (const char* where = strstr(start, extension)) {
            const char* terminator = where + strlen(extension);
            if (where == start || *(where - 1) == ' ') {
                if (*terminator == ' ' || *terminator == '\0') {
                    return true;
                }
            }
            start = terminator;
        }
        return false;
#else
        // Desktop OpenGL 3.0+: Use glGetStringi
        GLint numExtensions = 0;
        glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

        for (GLint i = 0; i < numExtensions; ++i) {
            const char* ext = (const char*)glGetStringi(GL_EXTENSIONS, i);
            if (ext && strcmp(ext, extension) == 0) {
                return true;
            }
        }
        return false;
#endif
    }

    // Helper function to query GPU memory via OpenGL extensions
    static long QueryGPUMemoryMB() {
        long gpu_mb = 0;

        // Try NVIDIA extension (GL_NVX_gpu_memory_info)
        // Works on: NVIDIA GPUs (Windows, Linux, Android)
        if (IsGLExtensionSupported("GL_NVX_gpu_memory_info")) {

            GLint total_mem_kb = 0;
            GLint available_mem_kb = 0;

            glGetIntegerv(GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX, &total_mem_kb);
            glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &available_mem_kb);

            // Calculate used memory
            long used_mem_kb = total_mem_kb - available_mem_kb;
            gpu_mb = used_mem_kb / 1024;

            PN_CORE_TRACE("[GPU] NVIDIA extension - Total: {} MB, Available: {} MB, Used: {} MB",
                total_mem_kb / 1024, available_mem_kb / 1024, gpu_mb);
        }
        // Try AMD/ATI extension (GL_ATI_meminfo)
        // Works on: AMD GPUs (Windows, Linux)
        else if (IsGLExtensionSupported("GL_ATI_meminfo")) {

            GLint mem_info[4] = { 0 };
            glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, mem_info);
            // mem_info[0] = total free memory in KB
            // mem_info[1] = largest available free block in KB
            // mem_info[2] = total auxiliary memory free in KB
            // mem_info[3] = largest auxiliary free block in KB

            long free_mem_kb = mem_info[0];

            // Estimate total GPU memory (AMD doesn't provide total, only free)
            // We'll assume used = 25% of free as a rough heuristic
            // This is not accurate but better than nothing
            long estimated_used_kb = free_mem_kb / 3;  // Rough estimate
            gpu_mb = estimated_used_kb / 1024;

            PN_CORE_TRACE("[GPU] AMD extension - Free: {} MB, Estimated Used: {} MB",
                free_mem_kb / 1024, gpu_mb);
        }
        else {
            PN_CORE_TRACE("[GPU] No supported GPU memory query extension available");
            gpu_mb = -1;  // Indicate unavailable
        }

        return gpu_mb;
    }

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

#ifdef _DEBUG
        auto debug_sink = std::make_shared<PAIN::DebugConsoleSink>();
        s_CoreLogger->sinks().push_back(debug_sink);
        s_ClientLogger->sinks().push_back(debug_sink);
#endif

	}

    void LogMemoryFullDiagnostic(const char* label) {
        long physical_ram_mb = 0;
        long virtual_committed_mb = 0;
        long peak_physical_mb = 0;
        long gpu_mb = -1;  // -1 indicates unavailable

#ifdef PN_PLATFORM_ANDROID

        // ANDROID - CPU/RAM MEMORY TRACKING


        FILE* file = fopen("/proc/self/status", "r");
        char line[256];
        long vm_rss = 0;   // Resident Set Size (physical RAM)
        long vm_size = 0;  // Virtual memory size
        long vm_hwm = 0;   // High Water Mark (peak RSS)
        long vm_peak = 0;  // Peak virtual memory

        if (file) {
            while (fgets(line, sizeof(line), file)) {
                if (sscanf(line, "VmRSS: %ld kB", &vm_rss) == 1) continue;
                if (sscanf(line, "VmSize: %ld kB", &vm_size) == 1) continue;
                if (sscanf(line, "VmHWM: %ld kB", &vm_hwm) == 1) continue;    // Peak physical
                if (sscanf(line, "VmPeak: %ld kB", &vm_peak) == 1) continue;  // Peak virtual
            }
            fclose(file);
        }

        physical_ram_mb = vm_rss / 1024;
        virtual_committed_mb = vm_size / 1024;
        peak_physical_mb = vm_hwm / 1024;  // Now tracks peak RSS (matches Windows)


        // ANDROID - PSS (Proportional Set Size) - More Accurate


        long pss_kb = 0;
        FILE* smaps = fopen("/proc/self/smaps_rollup", "r");

        if (smaps) {
            char smaps_line[256];
            while (fgets(smaps_line, sizeof(smaps_line), smaps)) {
                if (sscanf(smaps_line, "Pss: %ld kB", &pss_kb) == 1) {
                    break;  // Found PSS
                }
            }
            fclose(smaps);
        }
        else {
            // Fallback: If smaps_rollup not available, try parsing full smaps
            // (This is slower but works on older Android versions)
            smaps = fopen("/proc/self/smaps", "r");
            if (smaps) {
                char smaps_line[256];
                while (fgets(smaps_line, sizeof(smaps_line), smaps)) {
                    long pss_line = 0;
                    if (sscanf(smaps_line, "Pss: %ld kB", &pss_line) == 1) {
                        pss_kb += pss_line;  // Accumulate PSS from all mappings
                    }
                }
                fclose(smaps);
            }
        }

        long pss_mb = pss_kb / 1024;

        // ANDROID - GPU MEMORY (Multiple Methods)

        // Method 1: Try OpenGL extensions first (most accurate)
        gpu_mb = QueryGPUMemoryMB();

        // Method 2: Fallback to dumpsys (less accurate but widely available)
        if (gpu_mb < 0) {
            long graphics_kb = 0;
            long gl_kb = 0;

            // Get app's package name
            char package_name[256] = { 0 };
            FILE* cmdline = fopen("/proc/self/cmdline", "r");
            if (cmdline) {
                fgets(package_name, sizeof(package_name), cmdline);
                fclose(cmdline);
            }

            // Query meminfo with package name
            char cmd[512];
            snprintf(cmd, sizeof(cmd),
                "dumpsys meminfo %s 2>/dev/null | grep -E 'Graphics|GL mtrack'",
                package_name);

            FILE* pipe = popen(cmd, "r");
            if (pipe) {
                char result[512];
                while (fgets(result, sizeof(result), pipe)) {
                    // Try to parse different GPU memory fields
                    if (sscanf(result, " Graphics: %ld", &graphics_kb) == 1) continue;
                    if (sscanf(result, " GL mtrack: %ld", &gl_kb) == 1) continue;
                    // Some devices report it differently
                    if (sscanf(result, " GL: %ld", &gl_kb) == 1) continue;
                }
                pclose(pipe);
            }

            gpu_mb = (graphics_kb + gl_kb) / 1024;
        }

        // ANDROID - LOG OUTPUT

        if (pss_mb > 0) {
            // If PSS available, show both RSS and PSS
            PN_CORE_INFO("[MEMORY] {} - Physical (RSS): {:.2f} MB | Physical (PSS): {:.2f} MB | Virtual: {:.2f} MB | Peak Physical: {:.2f} MB | GPU: {} MB",
                label,
                (float)physical_ram_mb,
                (float)pss_mb,
                (float)virtual_committed_mb,
                (float)peak_physical_mb,
                (gpu_mb >= 0) ? std::to_string(gpu_mb) : "N/A");
        }
        else {
            // PSS not available, show RSS only
            PN_CORE_INFO("[MEMORY] {} - Physical: {:.2f} MB | Virtual: {:.2f} MB | Peak Physical: {:.2f} MB | GPU: {} MB",
                label,
                (float)physical_ram_mb,
                (float)virtual_committed_mb,
                (float)peak_physical_mb,
                (gpu_mb >= 0) ? std::to_string(gpu_mb) : "N/A");
        }

#elif defined(PN_PLATFORM_WINDOWS)

        // WINDOWS - CPU/RAM MEMORY TRACKING


        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(),
            (PROCESS_MEMORY_COUNTERS*)&pmc,
            sizeof(pmc))) {

            // WorkingSetSize = physical RAM currently in use (RSS equivalent)
            physical_ram_mb = pmc.WorkingSetSize / (1024 * 1024);

            // PrivateUsage = committed private memory (includes paged-out memory)
            virtual_committed_mb = pmc.PrivateUsage / (1024 * 1024);

            // PeakWorkingSetSize = maximum physical RAM used (peak RSS)
            peak_physical_mb = pmc.PeakWorkingSetSize / (1024 * 1024);
        }


        // WINDOWS - GPU MEMORY (OpenGL Extensions)


        gpu_mb = QueryGPUMemoryMB();


        // WINDOWS - LOG OUTPUT


        PN_CORE_INFO("[MEMORY] {} - Physical: {:.2f} MB | Virtual: {:.2f} MB | Peak Physical: {:.2f} MB | GPU: {} MB",
            label,
            (float)physical_ram_mb,
            (float)virtual_committed_mb,
            (float)peak_physical_mb,
            (gpu_mb >= 0) ? std::to_string(gpu_mb) : "N/A");

#else

        // OTHER PLATFORMS (macOS, Linux, etc.)


        PN_CORE_WARN("[MEMORY] {} - Platform not supported for detailed memory tracking", label);
#endif
    }

}
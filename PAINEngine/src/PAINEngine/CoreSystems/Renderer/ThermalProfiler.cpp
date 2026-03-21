#include "ThermalProfiler.h"
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>

#ifdef PN_PLATFORM_ANDROID
#include <dlfcn.h>
#include <filesystem>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace PAIN {

#ifdef PN_PLATFORM_ANDROID
namespace {

using AThermalManagerOpaque = void;
using ThermalAcquireFn = AThermalManagerOpaque* (*)();
using ThermalGetStatusFn = int (*)(AThermalManagerOpaque*);
using ThermalReleaseFn = void (*)(AThermalManagerOpaque*);

ThermalAcquireFn gThermalAcquire = nullptr;
ThermalGetStatusFn gThermalGetStatus = nullptr;
ThermalReleaseFn gThermalRelease = nullptr;

constexpr unsigned int kTimeElapsedExt = 0x88BF;
constexpr unsigned int kQueryResultExt = 0x8866;
constexpr unsigned int kQueryResultAvailableExt = 0x8867;
constexpr unsigned int kQueryCounterBitsExt = 0x8864;
constexpr unsigned int kGpuDisjointExt = 0x8FBB;

constexpr int kGpuStatusUnsupported = -1;
constexpr int kGpuStatusPending = -2;
constexpr int kGpuStatusDisjoint = -3;
constexpr int kGpuStatusInvalid = -4;
constexpr int kGpuStatusValid = 1;
constexpr uint64_t kMaxPendingQueryAgeFrames = 240;
constexpr size_t kMaxResolvedQueuePerPass = 16;

std::string ReadSmallFile(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) {
        return "";
    }

    char buf[128];
    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (len <= 0) {
        return "";
    }

    buf[len] = '\0';
    std::string value(buf);
    while (!value.empty() &&
           (value.back() == '\n' || value.back() == '\r' || value.back() == ' ' || value.back() == '\t')) {
        value.pop_back();
    }
    return value;
}

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string FindThermalTempPath() {
    namespace fs = std::filesystem;
    const fs::path root("/sys/class/thermal");
    std::error_code ec;
    if (!fs::exists(root, ec) || ec) {
        return "";
    }

    std::string fallbackPath;
    int bestScore = -1;
    std::string bestPath;

    for (const auto& entry : fs::directory_iterator(root, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_directory(ec) || ec) {
            ec.clear();
            continue;
        }

        const std::string name = entry.path().filename().string();
        if (name.rfind("thermal_zone", 0) != 0) {
            continue;
        }

        const fs::path tempPath = entry.path() / "temp";
        if (!fs::exists(tempPath, ec) || ec) {
            ec.clear();
            continue;
        }

        if (fallbackPath.empty()) {
            fallbackPath = tempPath.string();
        }

        const std::string type = ToLower(ReadSmallFile((entry.path() / "type").string()));
        int score = 0;
        if (type.find("cpu") != std::string::npos) score += 6;
        if (type.find("soc") != std::string::npos) score += 5;
        if (type.find("gpu") != std::string::npos) score += 4;
        if (type.find("skin") != std::string::npos) score += 3;
        if (type.find("battery") != std::string::npos) score -= 2;

        if (score > bestScore) {
            bestScore = score;
            bestPath = tempPath.string();
        }
    }

    return !bestPath.empty() ? bestPath : fallbackPath;
}

std::string FindGpuFreqPath() {
    namespace fs = std::filesystem;
    std::error_code ec;
    static const char* kKnownCandidates[] = {
        "/sys/class/kgsl/kgsl-3d0/clock_mhz",
        "/sys/class/kgsl/kgsl-3d0/gpuclk",
        "/sys/class/misc/mali0/device/clock",
        "/sys/devices/platform/mali-utgard/devfreq/governor-0/cur_freq"
    };

    for (const char* candidate : kKnownCandidates) {
        if (fs::exists(candidate, ec) && !ec) {
            return candidate;
        }
        ec.clear();
    }

    const fs::path devfreqRoot("/sys/class/devfreq");
    if (!fs::exists(devfreqRoot, ec) || ec) {
        return "";
    }

    for (const auto& entry : fs::directory_iterator(devfreqRoot, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_directory(ec) || ec) {
            ec.clear();
            continue;
        }

        const std::string name = ToLower(entry.path().filename().string());
        if (name.find("gpu") == std::string::npos &&
            name.find("kgsl") == std::string::npos &&
            name.find("mali") == std::string::npos &&
            name.find("adreno") == std::string::npos) {
            continue;
        }

        const fs::path curFreq = entry.path() / "cur_freq";
        if (fs::exists(curFreq, ec) && !ec) {
            return curFreq.string();
        }
        ec.clear();
    }

    return "";
}

float NormalizeTempToCelsius(float raw) {
    if (raw > 1000.0f) {
        return raw / 1000.0f;
    }
    if (raw > 200.0f) {
        return raw / 10.0f;
    }
    return raw;
}

float NormalizeFreqToMHz(float raw) {
    if (raw > 1000000.0f) {
        return raw / 1000000.0f;
    }
    if (raw > 10000.0f) {
        return raw / 1000.0f;
    }
    return raw;
}

}
#endif

std::unique_ptr<ThermalProfiler> g_ThermalProfiler = nullptr;

ThermalProfiler::ThermalProfiler() = default;

ThermalProfiler::~ThermalProfiler() {
    Shutdown();
}

void ThermalProfiler::Init() {
    if (initialized) return;

#ifndef PN_PLATFORM_ANDROID
    queryPool.resize(QUERY_POOL_SIZE * 2);
    glGenQueries(static_cast<GLsizei>(queryPool.size()), queryPool.data());
#else
    gpuTimingSupported = false;
    currentThermalState = -1.0f;
    currentThermalHeadroom = -1.0f;
    currentGpuFreqMHz = -1.0f;
    resolvedThisFrame = 0;
    pendingQueries.clear();
    resolvedByPass.clear();
    hasHeadroomSample = false;
#endif

    frameHistory.reserve(maxStoredFrames);

#ifdef PN_PLATFORM_ANDROID
    InitAndroidThermal();
#endif

    initialized = true;
}

void ThermalProfiler::Shutdown() {
    if (!initialized) return;

    DisableLogging();

    if (!queryPool.empty()) {
#ifndef PN_PLATFORM_ANDROID
        glDeleteQueries(static_cast<GLsizei>(queryPool.size()), queryPool.data());
#endif
        queryPool.clear();
    }

#ifdef PN_PLATFORM_ANDROID
    ShutdownAndroidGpuTiming();
    ShutdownAndroidThermal();
#endif

    initialized = false;
}

void ThermalProfiler::BeginFrame() {
    if (!initialized) return;

#ifdef PN_PLATFORM_ANDROID
    if (!androidGpuTimingInitialized) {
        androidGpuTimingInitialized = InitAndroidGpuTiming();
    }
    ProcessAndroidGpuQueries();
#endif

    frameStartCpu = std::chrono::steady_clock::now();
    currentFrame = ThermalFrame();
    currentFrame.frameNumber = frameCounter++;
    currentFrame.timestamp = frameStartCpu;
    cpuPassStart.clear();

    UpdateThermalState();

    nextQueryIndex = 0;
}

void ThermalProfiler::EndFrame() {
    if (!initialized) return;

    auto frameEndCpu = std::chrono::steady_clock::now();
    currentFrame.totalFrameTimeMs = std::chrono::duration<float, std::milli>(
        frameEndCpu - frameStartCpu).count();

    currentFrame.thermalValid = currentThermalState >= 0.0f ? 1 : 0;
    currentFrame.gpuFreqValid = currentGpuFreqMHz >= 0.0f ? 1 : 0;
    currentFrame.gpuTimingSupported = gpuTimingSupported ? 1 : 0;
#ifdef PN_PLATFORM_ANDROID
    currentFrame.gpuResolvedThisFrame = resolvedThisFrame;
    currentFrame.gpuPendingAfterFrame = static_cast<int>(pendingQueries.size());
    currentFrame.gpuDisjointTotal = static_cast<int>(disjointOccurrences);
#else
    currentFrame.gpuDisjointTotal = 0;
#endif

    if (loggingEnabled && csvFile.is_open()) {
        WriteCSVRow(currentFrame);
    }

    lastFrame = currentFrame;

    frameHistory.push_back(currentFrame);
    if (frameHistory.size() > maxStoredFrames) {
        frameHistory.erase(frameHistory.begin());
    }
}

void ThermalProfiler::BeginPass(const std::string& passName) {
    if (!initialized) return;

    PassTiming timing;
    timing.name = passName;
    timing.passInstanceId = nextPassInstanceId++;
    timing.active = true;
#ifndef PN_PLATFORM_ANDROID
    timing.queryStart = queryPool[nextQueryIndex++ % QUERY_POOL_SIZE];
    glQueryCounter(timing.queryStart, GL_TIMESTAMP);
    timing.gpuStatus = 0;
    timing.gpuSampleAgeFrames = -1;
#else
    timing.gpuTimeMs = -1.0f;
    timing.gpuStatus = gpuTimingSupported ? kGpuStatusPending : kGpuStatusUnsupported;
    timing.gpuSampleAgeFrames = -1;
    if (gpuTimingSupported && glGenQueriesEXT && glBeginQueryEXT) {
        unsigned int queryId = 0;
        glGenQueriesEXT(1, &queryId);
        if (queryId != 0) {
            timing.queryStart = queryId;
            glBeginQueryEXT(kTimeElapsedExt, queryId);
        }
    }
#endif

    cpuPassStart[timing.passInstanceId] = std::chrono::steady_clock::now();

    currentFrame.passTimings.push_back(timing);
}

void ThermalProfiler::EndPass(const std::string& passName) {
    if (!initialized) return;

    auto cpuEnd = std::chrono::steady_clock::now();

    for (auto& timing : currentFrame.passTimings) {
        if (timing.name == passName && timing.active) {
#ifndef PN_PLATFORM_ANDROID
            timing.queryEnd = queryPool[nextQueryIndex++ % QUERY_POOL_SIZE];
            glQueryCounter(timing.queryEnd, GL_TIMESTAMP);
#else
            if (timing.queryStart != 0 && gpuTimingSupported && glEndQueryEXT) {
                glEndQueryEXT(kTimeElapsedExt);
                pendingQueries.push_back({ timing.queryStart, timing.name, currentFrame.frameNumber });
            }
#endif

            auto it = cpuPassStart.find(timing.passInstanceId);
            if (it != cpuPassStart.end()) {
                timing.cpuTimeMs = std::chrono::duration<float, std::milli>(
                    cpuEnd - it->second).count();
                cpuPassStart.erase(it);
            }

#ifndef PN_PLATFORM_ANDROID
            GLint available = 0;
            int attempts = 0;
            while (available == 0 && attempts < 100) {
                glGetQueryObjectiv(timing.queryEnd, GL_QUERY_RESULT_AVAILABLE, &available);
                attempts++;
            }

            if (available) {
                GLuint64 startTime, endTime;
                glGetQueryObjectui64v(timing.queryStart, GL_QUERY_RESULT, &startTime);
                glGetQueryObjectui64v(timing.queryEnd, GL_QUERY_RESULT, &endTime);
                timing.gpuTimeMs = static_cast<float>(endTime - startTime) / 1000000.0f;
                timing.gpuStatus = 1;
                timing.gpuSampleAgeFrames = 0;
            }
#else
            ApplyResolvedGpuTiming(timing);
#endif

            timing.active = false;
            break;
        }
    }
}

void ThermalProfiler::ResetGraphicsContext() {
#ifdef PN_PLATFORM_ANDROID
    ShutdownAndroidGpuTiming();
    pendingQueries.clear();
    resolvedByPass.clear();
    resolvedThisFrame = 0;
#endif
    cpuPassStart.clear();
    currentFrame.passTimings.clear();
}

void ThermalProfiler::UpdateThermalState() {
#ifdef PN_PLATFORM_ANDROID
    UpdateAndroidThermal();
#endif
    currentFrame.thermalState = currentThermalState;
    currentFrame.thermalHeadroom = currentThermalHeadroom;
    currentFrame.gpuFrequencyMHz = currentGpuFreqMHz;
}

void ThermalProfiler::EnableLogging(const std::string& path) {
    DisableLogging();
    csvPath = path;
    csvFile.open(csvPath, std::ios::out | std::ios::trunc);
    if (csvFile.is_open()) {
        WriteCSVHeader();
        loggingEnabled = true;
    }
}

void ThermalProfiler::DisableLogging() {
    if (csvFile.is_open()) {
        csvFile.close();
    }
    loggingEnabled = false;
}

void ThermalProfiler::WriteCSVHeader() {
    if (!csvFile.is_open()) return;
    csvFile << "FrameNumber,Timestamp,TotalFrameMs,ThermalState,ThermalHeadroom,GpuFreqMHz,"
            << "ThermalValid,GpuFreqValid,GpuTimingSupported,GpuResolvedThisFrame,GpuPendingAfterFrame,GpuDisjointTotal,"
            << "PassName,GpuTimeMs,CpuTimeMs,GpuStatus,GpuSampleAgeFrames\n";
    csvFile.flush();
}

void ThermalProfiler::WriteCSVRow(const ThermalFrame& frame) {
    if (!csvFile.is_open()) return;

    auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        frame.timestamp.time_since_epoch()).count();

    if (frame.passTimings.empty()) {
        csvFile << frame.frameNumber << "," << timeMs << ","
                << frame.totalFrameTimeMs << "," << frame.thermalState << ","
                << frame.thermalHeadroom << "," << frame.gpuFrequencyMHz << ","
                << frame.thermalValid << "," << frame.gpuFreqValid << ","
                << frame.gpuTimingSupported << "," << frame.gpuResolvedThisFrame << ","
                << frame.gpuPendingAfterFrame << "," << frame.gpuDisjointTotal << ","
                << "N/A,-1,0,-2,-1\n";
    } else {
        for (const auto& pass : frame.passTimings) {
            csvFile << frame.frameNumber << "," << timeMs << ","
                    << frame.totalFrameTimeMs << "," << frame.thermalState << ","
                    << frame.thermalHeadroom << "," << frame.gpuFrequencyMHz << ","
                    << frame.thermalValid << "," << frame.gpuFreqValid << ","
                    << frame.gpuTimingSupported << "," << frame.gpuResolvedThisFrame << ","
                    << frame.gpuPendingAfterFrame << "," << frame.gpuDisjointTotal << ","
                    << pass.name << "," << pass.gpuTimeMs << "," << pass.cpuTimeMs << ","
                    << pass.gpuStatus << "," << pass.gpuSampleAgeFrames << "\n";
        }
    }
    csvFile.flush();
}

float ThermalProfiler::GetAveragePassTime(const std::string& passName) const {
    if (frameHistory.empty()) return 0.0f;

    float total = 0.0f;
    uint32_t count = 0;

    for (const auto& frame : frameHistory) {
        for (const auto& pass : frame.passTimings) {
            if (pass.name == passName && pass.gpuTimeMs > 0) {
                total += pass.gpuTimeMs;
                count++;
            }
        }
    }

    return count > 0 ? total / static_cast<float>(count) : 0.0f;
}

float ThermalProfiler::GetMaxPassTime(const std::string& passName) const {
    float maxTime = 0.0f;

    for (const auto& frame : frameHistory) {
        for (const auto& pass : frame.passTimings) {
            if (pass.name == passName && pass.gpuTimeMs > maxTime) {
                maxTime = pass.gpuTimeMs;
            }
        }
    }

    return maxTime;
}

#ifdef PN_PLATFORM_ANDROID
void ThermalProfiler::InitAndroidThermal() {
    thermalLibHandle = dlopen("libandroid.so", RTLD_NOW | RTLD_LOCAL);
    if (thermalLibHandle) {
        gThermalAcquire = reinterpret_cast<ThermalAcquireFn>(
            dlsym(thermalLibHandle, "AThermal_acquireManager"));
        gThermalGetStatus = reinterpret_cast<ThermalGetStatusFn>(
            dlsym(thermalLibHandle, "AThermal_getCurrentThermalStatus"));
        gThermalRelease = reinterpret_cast<ThermalReleaseFn>(
            dlsym(thermalLibHandle, "AThermal_releaseManager"));
        thermalGetHeadroom = reinterpret_cast<ThermalGetHeadroomFn>(
            dlsym(thermalLibHandle, "AThermal_getThermalHeadroom"));

        if (gThermalAcquire && gThermalGetStatus && gThermalRelease) {
            thermalManager = static_cast<void*>(gThermalAcquire());
        }
    }

    if (!thermalManager) {
        PN_CORE_WARN("AThermal API unavailable, falling back to sysfs thermal telemetry");
    }

    thermalPath = FindThermalTempPath();
    gpuFreqPath = FindGpuFreqPath();

    if (!thermalPath.empty()) {
        thermalFd = open(thermalPath.c_str(), O_RDONLY);
    }
    if (thermalFd < 0) {
        PN_CORE_WARN("Failed to open thermal zone for monitoring (path='{}')", thermalPath);
    }

    if (!gpuFreqPath.empty()) {
        gpuFreqFd = open(gpuFreqPath.c_str(), O_RDONLY);
    }
    if (gpuFreqFd < 0) {
        PN_CORE_WARN("Failed to open GPU frequency monitor path (path='{}')", gpuFreqPath);
    }

    currentThermalHeadroom = -1.0f;
}

bool ThermalProfiler::InitAndroidGpuTiming() {
    const char* ext = reinterpret_cast<const char*>(glGetString(GL_EXTENSIONS));
    if (!ext || std::strstr(ext, "GL_EXT_disjoint_timer_query") == nullptr) {
        gpuTimingSupported = false;
        hasDisjointTimerExt = false;
        return false;
    }

    glGenQueriesEXT = reinterpret_cast<PFNGLGENQUERIESEXTPROC>(eglGetProcAddress("glGenQueriesEXT"));
    glDeleteQueriesEXT = reinterpret_cast<PFNGLDELETEQUERIESEXTPROC>(eglGetProcAddress("glDeleteQueriesEXT"));
    glBeginQueryEXT = reinterpret_cast<PFNGLBEGINQUERYEXTPROC>(eglGetProcAddress("glBeginQueryEXT"));
    glEndQueryEXT = reinterpret_cast<PFNGLENDQUERYEXTPROC>(eglGetProcAddress("glEndQueryEXT"));
    glGetQueryivEXT = reinterpret_cast<PFNGLGETQUERYIVEXTPROC>(eglGetProcAddress("glGetQueryivEXT"));
    glGetQueryObjectivEXT = reinterpret_cast<PFNGLGETQUERYOBJECTIVEXTPROC>(eglGetProcAddress("glGetQueryObjectivEXT"));
    glGetQueryObjectuivEXT = reinterpret_cast<PFNGLGETQUERYOBJECTUIVEXTPROC>(eglGetProcAddress("glGetQueryObjectuivEXT"));
    glGetQueryObjectui64vEXT = reinterpret_cast<PFNGLGETQUERYOBJECTUI64VEXTPROC>(eglGetProcAddress("glGetQueryObjectui64vEXT"));

    const bool loaded = glGenQueriesEXT && glDeleteQueriesEXT && glBeginQueryEXT && glEndQueryEXT &&
        glGetQueryivEXT && glGetQueryObjectivEXT && (glGetQueryObjectui64vEXT || glGetQueryObjectuivEXT);

    hasDisjointTimerExt = loaded;
    gpuTimingSupported = loaded;
    if (!loaded) {
        return false;
    }

    int counterBits = 0;
    glGetQueryivEXT(kTimeElapsedExt, kQueryCounterBitsExt, &counterBits);
    if (counterBits <= 0) {
        gpuTimingSupported = false;
        hasDisjointTimerExt = false;
        return false;
    }

    int disjoint = 0;
    glGetIntegerv(kGpuDisjointExt, &disjoint);
    pendingQueries.clear();
    resolvedByPass.clear();
    resolvedThisFrame = 0;
    return true;
}

void ThermalProfiler::ShutdownAndroidGpuTiming() {
    const bool hasCurrentContext = eglGetCurrentContext() != EGL_NO_CONTEXT;
    for (const auto& pending : pendingQueries) {
        if (hasCurrentContext && pending.queryId != 0 && glDeleteQueriesEXT) {
            unsigned int q = pending.queryId;
            glDeleteQueriesEXT(1, &q);
        }
    }
    pendingQueries.clear();

    glGenQueriesEXT = nullptr;
    glDeleteQueriesEXT = nullptr;
    glBeginQueryEXT = nullptr;
    glEndQueryEXT = nullptr;
    glGetQueryivEXT = nullptr;
    glGetQueryObjectivEXT = nullptr;
    glGetQueryObjectuivEXT = nullptr;
    glGetQueryObjectui64vEXT = nullptr;

    hasDisjointTimerExt = false;
    gpuTimingSupported = false;
    androidGpuTimingInitialized = false;
}

void ThermalProfiler::ProcessAndroidGpuQueries() {
    resolvedThisFrame = 0;
    if (!gpuTimingSupported || !glGetQueryObjectivEXT || !glDeleteQueriesEXT) {
        return;
    }

    int disjoint = 0;
    glGetIntegerv(kGpuDisjointExt, &disjoint);
    if (disjoint != 0) {
        disjointOccurrences++;
        for (const auto& pending : pendingQueries) {
            if (pending.queryId != 0) {
                unsigned int queryToDelete = pending.queryId;
                glDeleteQueriesEXT(1, &queryToDelete);
            }

            ResolvedQuery invalidated;
            invalidated.sourceFrame = pending.sourceFrame;
            invalidated.gpuStatus = kGpuStatusDisjoint;
            invalidated.gpuTimeMs = -1.0f;
            auto& queue = resolvedByPass[pending.passName];
            queue.push_back(invalidated);
            while (queue.size() > kMaxResolvedQueuePerPass) {
                queue.pop_front();
            }
            resolvedThisFrame++;
        }

        pendingQueries.clear();
        return;
    }

    std::vector<PendingQuery> remaining;
    remaining.reserve(pendingQueries.size());

    for (const auto& pending : pendingQueries) {
        if (pending.queryId == 0) {
            continue;
        }

        const uint64_t queryAge = frameCounter >= pending.sourceFrame ? (frameCounter - pending.sourceFrame) : 0;
        if (queryAge > kMaxPendingQueryAgeFrames) {
            ResolvedQuery timeout;
            timeout.sourceFrame = pending.sourceFrame;
            timeout.gpuStatus = kGpuStatusInvalid;
            timeout.gpuTimeMs = -1.0f;
            auto& queue = resolvedByPass[pending.passName];
            queue.push_back(timeout);
            while (queue.size() > kMaxResolvedQueuePerPass) {
                queue.pop_front();
            }
            resolvedThisFrame++;

            unsigned int queryToDelete = pending.queryId;
            glDeleteQueriesEXT(1, &queryToDelete);
            continue;
        }

        int available = 0;
        glGetQueryObjectivEXT(pending.queryId, kQueryResultAvailableExt, &available);
        if (available == 0) {
            remaining.push_back(pending);
            continue;
        }

        ResolvedQuery resolved;
        resolved.sourceFrame = pending.sourceFrame;
        resolved.gpuStatus = kGpuStatusInvalid;
        resolved.gpuTimeMs = -1.0f;

        unsigned long long elapsedNs = 0;
        if (glGetQueryObjectui64vEXT) {
            glGetQueryObjectui64vEXT(pending.queryId, kQueryResultExt, &elapsedNs);
        } else if (glGetQueryObjectuivEXT) {
            unsigned int elapsed32 = 0;
            glGetQueryObjectuivEXT(pending.queryId, kQueryResultExt, &elapsed32);
            elapsedNs = elapsed32;
        }

        const GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            resolved.gpuStatus = kGpuStatusInvalid;
        } else if (elapsedNs > 0 && elapsedNs < 1000000000ULL) {
            resolved.gpuStatus = kGpuStatusValid;
            resolved.gpuTimeMs = static_cast<float>(elapsedNs) / 1000000.0f;
        } else {
            resolved.gpuStatus = kGpuStatusInvalid;
        }

        auto& queue = resolvedByPass[pending.passName];
        queue.push_back(resolved);
        while (queue.size() > kMaxResolvedQueuePerPass) {
            queue.pop_front();
        }
        resolvedThisFrame++;

        unsigned int queryToDelete = pending.queryId;
        glDeleteQueriesEXT(1, &queryToDelete);
    }

    pendingQueries.swap(remaining);
}

void ThermalProfiler::ApplyResolvedGpuTiming(PassTiming& timing) {
    timing.gpuTimeMs = -1.0f;
    timing.gpuSampleAgeFrames = -1;

    if (!gpuTimingSupported) {
        timing.gpuStatus = kGpuStatusUnsupported;
        return;
    }

    auto it = resolvedByPass.find(timing.name);
    if (it == resolvedByPass.end() || it->second.empty()) {
        timing.gpuStatus = kGpuStatusPending;
        return;
    }

    const ResolvedQuery resolved = it->second.front();
    it->second.pop_front();
    if (it->second.empty()) {
        resolvedByPass.erase(it);
    }

    timing.gpuStatus = resolved.gpuStatus;
    timing.gpuTimeMs = resolved.gpuStatus == kGpuStatusValid ? resolved.gpuTimeMs : -1.0f;
    timing.gpuSampleAgeFrames = resolved.sourceFrame <= currentFrame.frameNumber
        ? static_cast<int>(currentFrame.frameNumber - resolved.sourceFrame)
        : -1;
}

void ThermalProfiler::UpdateAndroidThermal() {
    if (thermalManager && gThermalGetStatus) {
        const int thermalStatus = gThermalGetStatus(static_cast<AThermalManagerOpaque*>(thermalManager));
        currentThermalState = thermalStatus >= 0 ? static_cast<float>(thermalStatus) : -1.0f;
    } else {
        currentThermalState = -1.0f;
    }

    if (currentThermalState < 0.0f && thermalFd >= 0) {
        char tempBuf[32];
        lseek(thermalFd, 0, SEEK_SET);
        const ssize_t len = read(thermalFd, tempBuf, sizeof(tempBuf) - 1);
        if (len > 0) {
            tempBuf[len] = '\0';
            const float tempC = NormalizeTempToCelsius(std::strtof(tempBuf, nullptr));
            if (std::isfinite(tempC)) {
                if (tempC < 35.0f) currentThermalState = 0.0f;
                else if (tempC < 40.0f) currentThermalState = 1.0f;
                else if (tempC < 45.0f) currentThermalState = 2.0f;
                else if (tempC < 50.0f) currentThermalState = 3.0f;
                else currentThermalState = 4.0f;
            }
        }
    }

    const auto now = std::chrono::steady_clock::now();
    if (!thermalManager || !thermalGetHeadroom) {
        currentThermalHeadroom = -1.0f;
        hasHeadroomSample = false;
    } else {
        const bool shouldRefreshHeadroom = !hasHeadroomSample ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeadroomSampleTime).count() >= 1000;

        if (shouldRefreshHeadroom) {
            lastHeadroomSampleTime = now;
            const float headroom = thermalGetHeadroom(thermalManager, 1);
            currentThermalHeadroom = (std::isfinite(headroom) && headroom >= 0.0f) ? headroom : -1.0f;
            hasHeadroomSample = true;
        }
    }

    if (gpuFreqFd >= 0) {
        char buf[32];
        lseek(gpuFreqFd, 0, SEEK_SET);
        ssize_t len = read(gpuFreqFd, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            const float rawFreq = std::strtof(buf, nullptr);
            currentGpuFreqMHz = NormalizeFreqToMHz(rawFreq);
        } else {
            currentGpuFreqMHz = -1.0f;
        }
    } else {
        currentGpuFreqMHz = -1.0f;
    }
}

void ThermalProfiler::ShutdownAndroidThermal() {
    if (thermalManager && gThermalRelease) {
        gThermalRelease(static_cast<AThermalManagerOpaque*>(thermalManager));
        thermalManager = nullptr;
    }
    if (thermalLibHandle) {
        dlclose(thermalLibHandle);
        thermalLibHandle = nullptr;
    }
    gThermalAcquire = nullptr;
    gThermalGetStatus = nullptr;
    gThermalRelease = nullptr;
    thermalGetHeadroom = nullptr;
    hasHeadroomSample = false;
    currentThermalHeadroom = -1.0f;

    if (thermalFd >= 0) {
        close(thermalFd);
        thermalFd = -1;
    }
    if (gpuFreqFd >= 0) {
        close(gpuFreqFd);
        gpuFreqFd = -1;
    }
}
#endif

}

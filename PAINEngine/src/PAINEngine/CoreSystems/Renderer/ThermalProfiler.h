#pragma once

#include "pch.h"
#include <chrono>
#include <deque>
#include <vector>
#include <string>
#include <fstream>
#include <functional>

namespace PAIN {

struct PassTiming {
    std::string name;
    uint64_t passInstanceId = 0;
    float gpuTimeMs = 0.0f;
    float cpuTimeMs = 0.0f;
    int gpuStatus = 0;
    int gpuSampleAgeFrames = -1;
    int64_t gpuSourceFrame = -1;
    uint64_t gpuSourcePassInstanceId = 0;
    uint32_t queryStart = 0;
    uint32_t queryEnd = 0;
    bool active = false;
};

struct ThermalFrame {
    uint64_t frameNumber = 0;
    float totalFrameTimeMs = 0.0f;
    float thermalState = 0.0f;
    float thermalHeadroom = -1.0f;
    float gpuFrequencyMHz = 0.0f;
    int thermalValid = 0;
    int gpuFreqValid = 0;
    int gpuTimingSupported = 0;
    int gpuResolvedThisFrame = 0;
    int gpuPendingAfterFrame = 0;
    int gpuDisjointTotal = 0;
    int gpuResolvedQueueSamples = 0;
    int gpuTimedOutTotal = 0;
    int gpuResolvedDroppedTotal = 0;
    int gpuResetDiscardedTotal = 0;
    int gpuDisjointThisFrame = 0;
    int gpuDiagnosisReady = 0;
    int gpuKeyPassesExpected = 0;
    int gpuKeyPassesSeen = 0;
    int gpuKeyPassesValid = 0;
    int gpuKeyPassesDisjoint = 0;
    int gpuKeyPassesPending = 0;
    int gpuDisjointWindowFrames = 0;
    int gpuDisjointWindowEvents = 0;
    float gpuDisjointWindowRatio = 0.0f;
    int gpuDisjointBreakerTriggered = 0;
    int gpuDisjointBreakerTotal = 0;
    int64_t gpuDisjointLastFrame = -1;
    int gpuDisjointLastQueueDepth = 0;
    int gpuDisjointLastReadySamples = 0;
    int gpuDisjointLastMaxSampleAge = -1;
    int gpuDisjointHighQueueTotal = 0;
    int gpuDisjointStaleSampleTotal = 0;
    int gpuContextResetEventsTotal = 0;
    std::vector<PassTiming> passTimings;
    std::chrono::steady_clock::time_point timestamp;
};

class ThermalProfiler {
public:
    ThermalProfiler();
    ~ThermalProfiler();

    void Init();
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    void BeginPass(const std::string& passName);
    void EndPass(const std::string& passName);
    void ResetGraphicsContext();

    void UpdateThermalState();
    float GetCurrentThermalState() const { return currentThermalState; }

    void EnableLogging(const std::string& csvPath);
    void DisableLogging();
    bool IsLoggingEnabled() const { return loggingEnabled; }

    const ThermalFrame& GetLastFrame() const { return lastFrame; }
    float GetAveragePassTime(const std::string& passName) const;
    float GetMaxPassTime(const std::string& passName) const;

    void SetMaxStoredFrames(uint32_t count) { maxStoredFrames = count; }
    void EnableOverlay(bool enable) { overlayEnabled = enable; }
    bool IsOverlayEnabled() const { return overlayEnabled; }

private:
    void WriteCSVHeader();
    void WriteCSVRow(const ThermalFrame& frame);
    void RotateQueryPool();

    bool initialized = false;
    bool loggingEnabled = false;
    bool overlayEnabled = false;

    std::ofstream csvFile;
    std::string csvPath;

    uint64_t frameCounter = 0;
    uint32_t maxStoredFrames = 300;
    std::vector<ThermalFrame> frameHistory;
    ThermalFrame currentFrame;
    ThermalFrame lastFrame;

    static constexpr uint32_t QUERY_POOL_SIZE = 64;
    std::vector<uint32_t> queryPool;
    uint32_t nextQueryIndex = 0;

    std::chrono::steady_clock::time_point frameStartCpu;
    std::unordered_map<uint64_t, std::chrono::steady_clock::time_point> cpuPassStart;
    uint64_t nextPassInstanceId = 1;
    bool gpuTimingSupported = true;

    float currentThermalState = 0.0f;
    float currentThermalHeadroom = -1.0f;
    float currentGpuFreqMHz = 0.0f;

#ifdef PN_PLATFORM_ANDROID
    int thermalFd = -1;
    int gpuFreqFd = -1;
    void* thermalManager = nullptr;
    void* thermalLibHandle = nullptr;
    std::string thermalPath;
    std::string gpuFreqPath;
    void InitAndroidThermal();
    void UpdateAndroidThermal();
    void ShutdownAndroidThermal();

    bool InitAndroidGpuTiming();
    void ShutdownAndroidGpuTiming();
    void ProcessAndroidGpuQueries();
    void ApplyResolvedGpuTiming(PassTiming& timing);
    bool androidGpuTimingInitialized = false;
    bool hasDisjointTimerExt = false;

    using PFNGLGENQUERIESEXTPROC = void (*)(int n, unsigned int* ids);
    using PFNGLDELETEQUERIESEXTPROC = void (*)(int n, const unsigned int* ids);
    using PFNGLBEGINQUERYEXTPROC = void (*)(unsigned int target, unsigned int id);
    using PFNGLENDQUERYEXTPROC = void (*)(unsigned int target);
    using PFNGLGETQUERYIVEXTPROC = void (*)(unsigned int target, unsigned int pname, int* params);
    using PFNGLGETQUERYOBJECTIVEXTPROC = void (*)(unsigned int id, unsigned int pname, int* params);
    using PFNGLGETQUERYOBJECTUIVEXTPROC = void (*)(unsigned int id, unsigned int pname, unsigned int* params);
    using PFNGLGETQUERYOBJECTUI64VEXTPROC = void (*)(unsigned int id, unsigned int pname, unsigned long long* params);

    PFNGLGENQUERIESEXTPROC glGenQueriesEXT = nullptr;
    PFNGLDELETEQUERIESEXTPROC glDeleteQueriesEXT = nullptr;
    PFNGLBEGINQUERYEXTPROC glBeginQueryEXT = nullptr;
    PFNGLENDQUERYEXTPROC glEndQueryEXT = nullptr;
    PFNGLGETQUERYIVEXTPROC glGetQueryivEXT = nullptr;
    PFNGLGETQUERYOBJECTIVEXTPROC glGetQueryObjectivEXT = nullptr;
    PFNGLGETQUERYOBJECTUIVEXTPROC glGetQueryObjectuivEXT = nullptr;
    PFNGLGETQUERYOBJECTUI64VEXTPROC glGetQueryObjectui64vEXT = nullptr;

    struct PendingQuery {
        unsigned int queryId;
        std::string passName;
        uint64_t passInstanceId = 0;
        uint64_t sourceFrame = 0;
    };
    struct ResolvedQuery {
        float gpuTimeMs = -1.0f;
        int gpuStatus = -2;
        uint64_t sourcePassInstanceId = 0;
        uint64_t sourceFrame = 0;
    };
    std::vector<PendingQuery> pendingQueries;
    std::unordered_map<std::string, std::deque<ResolvedQuery>> resolvedByPass;
    int resolvedThisFrame = 0;
    unsigned int disjointOccurrences = 0;
    unsigned int timedOutQueryCount = 0;
    unsigned int droppedResolvedSampleCount = 0;
    unsigned int resetDiscardedSampleCount = 0;
    bool disjointOccurredThisFrame = false;
    int disjointReadySamplesThisFrame = 0;
    int disjointMaxSampleAgeThisFrame = -1;
    std::deque<int> disjointFrameWindow;
    int disjointFramesInWindow = 0;
    uint64_t lastDisjointBreakFrame = 0;
    bool hasDisjointBreakFrame = false;
    unsigned int disjointBreakerTriggeredCount = 0;
    uint64_t lastDisjointEventFrame = 0;
    int lastDisjointEventQueueDepth = 0;
    int lastDisjointEventReadySamples = 0;
    int lastDisjointEventMaxSampleAge = -1;
    unsigned int disjointHighQueueCount = 0;
    unsigned int disjointStaleSampleCount = 0;
    unsigned int contextResetEventCount = 0;

    using ThermalGetHeadroomFn = float (*)(void* manager, int forecastSeconds);
    ThermalGetHeadroomFn thermalGetHeadroom = nullptr;
    std::chrono::steady_clock::time_point lastHeadroomSampleTime{};
    bool hasHeadroomSample = false;
#endif
};

extern std::unique_ptr<ThermalProfiler> g_ThermalProfiler;

}

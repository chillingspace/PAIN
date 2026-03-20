#pragma once

#include "pch.h"
#include <chrono>
#include <vector>
#include <string>
#include <fstream>

namespace PAIN {

struct PassTiming {
    std::string name;
    float gpuTimeMs = 0.0f;
    float cpuTimeMs = 0.0f;
    uint32_t queryStart = 0;
    uint32_t queryEnd = 0;
    bool active = false;
};

struct ThermalFrame {
    uint64_t frameNumber = 0;
    float totalFrameTimeMs = 0.0f;
    float thermalState = 0.0f;
    float gpuFrequencyMHz = 0.0f;
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
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> cpuPassStart;

    float currentThermalState = 0.0f;
    float currentGpuFreqMHz = 0.0f;

#ifdef PN_PLATFORM_ANDROID
    int thermalFd = -1;
    void InitAndroidThermal();
    void UpdateAndroidThermal();
    void ShutdownAndroidThermal();
#endif
};

extern std::unique_ptr<ThermalProfiler> g_ThermalProfiler;

}

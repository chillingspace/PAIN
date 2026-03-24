#pragma once

#include "pch.h"
#include <chrono>
#include <vector>
#include <string>
#include <fstream>
#include <deque>

namespace PAIN {

// Simplified pass timing - CPU only
struct PassTiming {
    std::string name;
    float cpuTimeMs = 0.0f;
};

// Simplified frame data - reliable metrics only
struct ThermalFrame {
    uint64_t frameNumber = 0;
    float totalFrameTimeMs = 0.0f;
    
    // Thermal state (from AThermal API on Android)
    float thermalState = -1.0f;      // 0-4 severity, -1 if unavailable
    float thermalHeadroom = -1.0f;   // 0-1 thermal headroom
    
    // GPU frequency (from sysfs on Android)
    float gpuFrequencyMHz = -1.0f;
    
    // GPU utilization (from sysfs on Android, or estimated from frequency)
    float gpuLoadPct = -1.0f;        // 0-100%, -1 if unavailable
    
    // Validity flags
    int thermalValid = 0;
    int gpuFreqValid = 0;
    int gpuLoadValid = 0;
    int gpuLoadEstimated = 0;        // 1 if load was estimated from frequency
    
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
    float GetCurrentGpuFrequency() const { return currentGpuFreqMHz; }
    float GetCurrentGpuLoad() const { return currentGpuLoadPct; }

    void EnableLogging(const std::string& csvPath);
    void DisableLogging();
    bool IsLoggingEnabled() const { return loggingEnabled; }

    const ThermalFrame& GetLastFrame() const { return lastFrame; }
    float GetAverageFrameTime() const;
    float GetAveragePassTime(const std::string& passName) const;

    void SetMaxStoredFrames(uint32_t count) { maxStoredFrames = count; }

private:
    void WriteCSVHeader();
    void WriteCSVRow(const ThermalFrame& frame);

    bool initialized = false;
    bool loggingEnabled = false;

    std::ofstream csvFile;
    std::string csvPath;

    uint64_t frameCounter = 0;
    uint32_t maxStoredFrames = 300;
    std::deque<ThermalFrame> frameHistory;
    ThermalFrame currentFrame;
    ThermalFrame lastFrame;

    std::chrono::steady_clock::time_point frameStartCpu;
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> passStartTimes;

    float currentThermalState = -1.0f;
    float currentThermalHeadroom = -1.0f;
    float currentGpuFreqMHz = -1.0f;
    float currentGpuLoadPct = -1.0f;
    
    // GPU frequency range for load estimation
    float gpuFreqMinMHz = -1.0f;
    float gpuFreqMaxMHz = -1.0f;
    bool gpuLoadEstimated = false;  // True if load is estimated from frequency

#ifdef PN_PLATFORM_ANDROID
    int thermalFd = -1;
    int gpuFreqFd = -1;
    int gpuLoadFd = -1;
    void* thermalManager = nullptr;
    void* thermalLibHandle = nullptr;
    std::string thermalPath;
    std::string gpuFreqPath;
    std::string gpuLoadPath;
    std::string gpuAvailFreqPath;  // For load estimation
    
    void InitAndroidThermal();
    void UpdateAndroidThermal();
    void ShutdownAndroidThermal();
    
    using ThermalGetHeadroomFn = float (*)(void* manager, int forecastSeconds);
    ThermalGetHeadroomFn thermalGetHeadroom = nullptr;
    std::chrono::steady_clock::time_point lastHeadroomSampleTime{};
    bool hasHeadroomSample = false;
#endif
};

extern std::unique_ptr<ThermalProfiler> g_ThermalProfiler;

}

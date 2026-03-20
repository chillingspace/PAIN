#include "ThermalProfiler.h"
#include <cstdlib>

#ifdef PN_PLATFORM_ANDROID
#include <fcntl.h>
#include <unistd.h>
#endif

namespace PAIN {

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
    ShutdownAndroidThermal();
#endif

    initialized = false;
}

void ThermalProfiler::BeginFrame() {
    if (!initialized) return;

    frameStartCpu = std::chrono::steady_clock::now();
    currentFrame = ThermalFrame();
    currentFrame.frameNumber = frameCounter++;
    currentFrame.timestamp = frameStartCpu;

    UpdateThermalState();

    nextQueryIndex = 0;
}

void ThermalProfiler::EndFrame() {
    if (!initialized) return;

    auto frameEndCpu = std::chrono::steady_clock::now();
    currentFrame.totalFrameTimeMs = std::chrono::duration<float, std::milli>(
        frameEndCpu - frameStartCpu).count();

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
    timing.active = true;
#ifndef PN_PLATFORM_ANDROID
    timing.queryStart = queryPool[nextQueryIndex++ % QUERY_POOL_SIZE];
    glQueryCounter(timing.queryStart, GL_TIMESTAMP);
#endif

    cpuPassStart[passName] = std::chrono::steady_clock::now();

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
#endif

            auto it = cpuPassStart.find(passName);
            if (it != cpuPassStart.end()) {
                timing.cpuTimeMs = std::chrono::duration<float, std::milli>(
                    cpuEnd - it->second).count();
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
            }
#else
            timing.gpuTimeMs = 0.0f;
#endif

            timing.active = false;
            break;
        }
    }
}

void ThermalProfiler::UpdateThermalState() {
#ifdef PN_PLATFORM_ANDROID
    UpdateAndroidThermal();
#endif
    currentFrame.thermalState = currentThermalState;
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
    csvFile << "FrameNumber,Timestamp,TotalFrameMs,ThermalState,GpuFreqMHz,"
            << "PassName,GpuTimeMs,CpuTimeMs\n";
    csvFile.flush();
}

void ThermalProfiler::WriteCSVRow(const ThermalFrame& frame) {
    if (!csvFile.is_open()) return;

    auto timeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        frame.timestamp.time_since_epoch()).count();

    if (frame.passTimings.empty()) {
        csvFile << frame.frameNumber << "," << timeMs << ","
                << frame.totalFrameTimeMs << "," << frame.thermalState << ","
                << frame.gpuFrequencyMHz << ",N/A,0,0\n";
    } else {
        for (const auto& pass : frame.passTimings) {
            csvFile << frame.frameNumber << "," << timeMs << ","
                    << frame.totalFrameTimeMs << "," << frame.thermalState << ","
                    << frame.gpuFrequencyMHz << "," << pass.name << ","
                    << pass.gpuTimeMs << "," << pass.cpuTimeMs << "\n";
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
    thermalFd = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
    if (thermalFd < 0) {
        PN_CORE_WARN("Failed to open thermal zone for monitoring");
    }
}

void ThermalProfiler::UpdateAndroidThermal() {
    if (thermalFd >= 0) {
        char buf[32];
        lseek(thermalFd, 0, SEEK_SET);
        ssize_t len = read(thermalFd, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            int tempMilliC = std::atoi(buf);
            float tempC = tempMilliC / 1000.0f;

            if (tempC < 35.0f) currentThermalState = 0.0f;
            else if (tempC < 40.0f) currentThermalState = 1.0f;
            else if (tempC < 45.0f) currentThermalState = 2.0f;
            else if (tempC < 50.0f) currentThermalState = 3.0f;
            else currentThermalState = 4.0f;
        }
    }

    int gpuFd = open("/sys/class/kgsl/kgsl-3d0/clock_mhz", O_RDONLY);
    if (gpuFd >= 0) {
        char buf[32];
        ssize_t len = read(gpuFd, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            currentGpuFreqMHz = static_cast<float>(std::atoi(buf));
        }
        close(gpuFd);
    }
}

void ThermalProfiler::ShutdownAndroidThermal() {
    if (thermalFd >= 0) {
        close(thermalFd);
        thermalFd = -1;
    }
}
#endif

}

#include "ThermalProfiler.h"
#include <array>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <string_view>

#ifdef PN_PLATFORM_ANDROID
#include <dlfcn.h>
#include <filesystem>
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

    frameHistory.clear();
    frameCounter = 0;

#ifdef PN_PLATFORM_ANDROID
    InitAndroidThermal();
#endif

    initialized = true;
}

void ThermalProfiler::Shutdown() {
    if (!initialized) return;

    DisableLogging();

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
    passStartTimes.clear();

    UpdateThermalState();
}

void ThermalProfiler::EndFrame() {
    if (!initialized) return;

    auto frameEndCpu = std::chrono::steady_clock::now();
    currentFrame.totalFrameTimeMs = std::chrono::duration<float, std::milli>(
        frameEndCpu - frameStartCpu).count();

    currentFrame.thermalValid = currentThermalState >= 0.0f ? 1 : 0;
    currentFrame.gpuFreqValid = currentGpuFreqMHz >= 0.0f ? 1 : 0;

    if (loggingEnabled && csvFile.is_open()) {
        WriteCSVRow(currentFrame);
    }

    lastFrame = currentFrame;

    frameHistory.push_back(currentFrame);
    if (frameHistory.size() > maxStoredFrames) {
        frameHistory.pop_front();
    }
}

void ThermalProfiler::BeginPass(const std::string& passName) {
    if (!initialized) return;
    passStartTimes[passName] = std::chrono::steady_clock::now();
}

void ThermalProfiler::EndPass(const std::string& passName) {
    if (!initialized) return;

    auto it = passStartTimes.find(passName);
    if (it == passStartTimes.end()) return;

    auto passEnd = std::chrono::steady_clock::now();
    float cpuTimeMs = std::chrono::duration<float, std::milli>(
        passEnd - it->second).count();

    PassTiming timing;
    timing.name = passName;
    timing.cpuTimeMs = cpuTimeMs;
    currentFrame.passTimings.push_back(timing);

    passStartTimes.erase(it);
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
            << "ThermalValid,GpuFreqValid,PassName,CpuTimeMs\n";
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
                << "N/A,0\n";
    } else {
        for (const auto& pass : frame.passTimings) {
            csvFile << frame.frameNumber << "," << timeMs << ","
                    << frame.totalFrameTimeMs << "," << frame.thermalState << ","
                    << frame.thermalHeadroom << "," << frame.gpuFrequencyMHz << ","
                    << frame.thermalValid << "," << frame.gpuFreqValid << ","
                    << pass.name << "," << pass.cpuTimeMs << "\n";
        }
    }
    csvFile.flush();
}

float ThermalProfiler::GetAverageFrameTime() const {
    if (frameHistory.empty()) return 0.0f;

    float total = 0.0f;
    for (const auto& frame : frameHistory) {
        total += frame.totalFrameTimeMs;
    }
    return total / static_cast<float>(frameHistory.size());
}

float ThermalProfiler::GetAveragePassTime(const std::string& passName) const {
    if (frameHistory.empty()) return 0.0f;

    float total = 0.0f;
    uint32_t count = 0;

    for (const auto& frame : frameHistory) {
        for (const auto& pass : frame.passTimings) {
            if (pass.name == passName) {
                total += pass.cpuTimeMs;
                count++;
            }
        }
    }

    return count > 0 ? total / static_cast<float>(count) : 0.0f;
}

#ifdef PN_PLATFORM_ANDROID

namespace {

using AThermalManagerOpaque = void;
using ThermalAcquireFn = AThermalManagerOpaque* (*)();
using ThermalGetStatusFn = int (*)(AThermalManagerOpaque*);
using ThermalReleaseFn = void (*)(AThermalManagerOpaque*);

ThermalAcquireFn gThermalAcquire = nullptr;
ThermalGetStatusFn gThermalGetStatus = nullptr;
ThermalReleaseFn gThermalRelease = nullptr;

std::string ReadSmallFile(const std::string& path) {
    int fd = open(path.c_str(), O_RDONLY);
    if (fd < 0) return "";

    char buf[128];
    ssize_t len = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (len <= 0) return "";

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
    if (!fs::exists(root, ec) || ec) return "";

    std::string fallbackPath;
    int bestScore = -1;
    std::string bestPath;

    for (const auto& entry : fs::directory_iterator(root, ec)) {
        if (ec || !entry.is_directory(ec)) { ec.clear(); continue; }

        const std::string name = entry.path().filename().string();
        if (name.rfind("thermal_zone", 0) != 0) continue;

        const fs::path tempPath = entry.path() / "temp";
        if (!fs::exists(tempPath, ec) || ec) { ec.clear(); continue; }

        if (fallbackPath.empty()) fallbackPath = tempPath.string();

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
        "/sys/class/kgsl/kgsl-3d0/devfreq/cur_freq",
        "/sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/gpuclk",
        "/sys/devices/platform/kgsl-3d0.0/kgsl/kgsl-3d0/devfreq/cur_freq",
        "/sys/devices/platform/gpusysfs/gpu_clock",
        "/sys/class/misc/mali0/device/clock",
        "/sys/devices/platform/mali/clock",
        "/sys/devices/platform/mali.0/devfreq/cur_freq",
        "/sys/devices/platform/mali-utgard/devfreq/governor-0/cur_freq"
    };

    for (const char* candidate : kKnownCandidates) {
        if (fs::exists(candidate, ec) && !ec) return candidate;
        ec.clear();
    }

    const fs::path devfreqRoot("/sys/class/devfreq");
    if (!fs::exists(devfreqRoot, ec) || ec) return "";

    for (const auto& entry : fs::directory_iterator(devfreqRoot, ec)) {
        if (ec || !entry.is_directory(ec)) { ec.clear(); continue; }

        const std::string name = ToLower(entry.path().filename().string());
        if (name.find("gpu") == std::string::npos &&
            name.find("kgsl") == std::string::npos &&
            name.find("mali") == std::string::npos &&
            name.find("adreno") == std::string::npos) continue;

        const fs::path curFreq = entry.path() / "cur_freq";
        if (fs::exists(curFreq, ec) && !ec) return curFreq.string();
        ec.clear();
    }

    return "";
}

float NormalizeTempToCelsius(float raw) {
    if (raw > 1000.0f) return raw / 1000.0f;
    if (raw > 200.0f) return raw / 10.0f;
    return raw;
}

float NormalizeFreqToMHz(float raw) {
    if (raw > 1000000.0f) return raw / 1000000.0f;
    if (raw > 10000.0f) return raw / 1000.0f;
    return raw;
}

} // anonymous namespace

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
    if (!thermalPath.empty()) {
        thermalFd = open(thermalPath.c_str(), O_RDONLY);
    }

    gpuFreqPath = FindGpuFreqPath();
    if (!gpuFreqPath.empty()) {
        gpuFreqFd = open(gpuFreqPath.c_str(), O_RDONLY);
    }

    currentThermalHeadroom = -1.0f;
}

void ThermalProfiler::UpdateAndroidThermal() {
    // Get thermal state from AThermal API
    if (thermalManager && gThermalGetStatus) {
        const int thermalStatus = gThermalGetStatus(static_cast<AThermalManagerOpaque*>(thermalManager));
        currentThermalState = thermalStatus >= 0 ? static_cast<float>(thermalStatus) : -1.0f;
    } else {
        currentThermalState = -1.0f;
    }

    // Fallback to sysfs temperature if AThermal unavailable
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

    // Get thermal headroom (throttling prediction)
    const auto now = std::chrono::steady_clock::now();
    if (!thermalManager || !thermalGetHeadroom) {
        currentThermalHeadroom = -1.0f;
        hasHeadroomSample = false;
    } else {
        const bool shouldRefresh = !hasHeadroomSample ||
            std::chrono::duration_cast<std::chrono::milliseconds>(now - lastHeadroomSampleTime).count() >= 1000;

        if (shouldRefresh) {
            lastHeadroomSampleTime = now;
            const float headroom = thermalGetHeadroom(thermalManager, 1);
            currentThermalHeadroom = (std::isfinite(headroom) && headroom >= 0.0f) ? headroom : -1.0f;
            hasHeadroomSample = true;
        }
    }

    // Get GPU frequency
    if (gpuFreqFd >= 0) {
        char buf[32];
        lseek(gpuFreqFd, 0, SEEK_SET);
        ssize_t len = read(gpuFreqFd, buf, sizeof(buf) - 1);
        if (len > 0) {
            buf[len] = '\0';
            const float rawFreq = std::strtof(buf, nullptr);
            const float normalized = NormalizeFreqToMHz(rawFreq);
            currentGpuFreqMHz = (std::isfinite(normalized) && normalized > 0.0f) ? normalized : -1.0f;
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

#endif // PN_PLATFORM_ANDROID

} // namespace PAIN

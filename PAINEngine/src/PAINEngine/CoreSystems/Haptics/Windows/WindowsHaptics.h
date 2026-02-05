#pragma once

#ifndef WINDOWS_HAPTICS_HPP
#define WINDOWS_HAPTICS_HPP

#include "../Haptics.h"

#ifdef PN_PLATFORM_WINDOWS

namespace PAIN {
    namespace Haptics {

        // Windows stub implementation - no haptics support
        class WindowsHaptics : public Haptics {
        public:
            WindowsHaptics(void* app) { init(); }
            ~WindowsHaptics() override = default;
            
            bool init() override { return false; }
            void shutdown() override {}
            
            void vibrate(int64_t duration_ms, int amplitude = -1) override {}
            void vibratePattern(const std::vector<int64_t>& timings, 
                               const std::vector<int>& amplitudes = {}) override {}
            
            void click() override {}
            void heavyClick() override {}
            void tick() override {}
            void doubleClick() override {}
            
            void stop() override {}
            
            bool hasHaptics() const override { return false; }
            bool hasAmplitudeControl() const override { return false; }
        };

    }
}

#endif // PN_PLATFORM_WINDOWS

#endif // WINDOWS_HAPTICS_HPP

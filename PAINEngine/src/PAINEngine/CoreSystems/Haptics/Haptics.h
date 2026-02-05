#pragma once

#ifndef HAPTICS_HPP
#define HAPTICS_HPP

#include "Applications/AppSystem.h"
#include <vector>
#include <cstdint>

namespace PAIN {
    namespace Haptics {

        // Predefined haptic feedback patterns
        enum class Pattern {
            CLICK,        // Short 10ms feedback
            HEAVY_CLICK,  // Strong 20ms feedback
            TICK,         // Light 5ms feedback
            DOUBLECLICK  // Two quick clicks
        };

        // Haptic effect description
        struct Effect {
            std::vector<int64_t> timings;  // Pattern timings in milliseconds (on/off alternating)
            std::vector<int> amplitudes;   // Amplitude for each timing (0-255), empty = default
            int repeat;                    // Repeat count, -1 for no repeat
            
            Effect() : repeat(-1) {}
        };

        class Haptics : public AppSystem {
        public:
            virtual ~Haptics() = default;

            // Lifetime
            virtual bool init() = 0;
            virtual void shutdown() = 0;

            // Basic vibration
            // duration_ms: vibration duration in milliseconds
            // amplitude: 0-255 (0 = default/minimum, 255 = maximum), -1 for device default
            virtual void vibrate(int64_t duration_ms, int amplitude = -1) = 0;

            // Pattern vibration
            // timings: pattern in milliseconds [delay, on, off, on, off, ...]
            // amplitudes: optional amplitude for each "on" segment (0-255)
            virtual void vibratePattern(const std::vector<int64_t>& timings, 
                                       const std::vector<int>& amplitudes = {}) = 0;

            // Predefined patterns
            virtual void click() = 0;
            virtual void heavyClick() = 0;
            virtual void tick() = 0;
            virtual void doubleClick() = 0;

            // Stop any ongoing vibration
            virtual void stop() = 0;

            // Check if haptics are available on this device
            virtual bool hasHaptics() const = 0;

            // Check if amplitude control is supported
            virtual bool hasAmplitudeControl() const = 0;

            // AppSystem interface
            void onAttach() override { init(); }
            void onDetach() override { shutdown(); }
            void onFixedUpdate(AppTiming timing) override {}
            void onUpdate(AppTiming timing) override {}
            void onEvent(Event::Event& e) override {}

            // Factory method
            static Haptics* create(void* app);
        };

    }
}

#endif

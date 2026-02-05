#pragma once

#ifndef ANDROID_HAPTICS_HPP
#define ANDROID_HAPTICS_HPP

#include "../Haptics.h"

#ifdef PN_PLATFORM_ANDROID

#include <android/native_activity.h>
#include <jni.h>

namespace PAIN {
    namespace Haptics {

        class AndroidHaptics : public Haptics {
        private:
            android_app* m_App = nullptr;
            
            // JNI references
            jclass m_VibratorClass = nullptr;
            jobject m_VibratorObj = nullptr;
            jmethodID m_VibrateMethod = nullptr;
            jmethodID m_VibratePatternMethod = nullptr;
            jmethodID m_VibrateAmplitudeMethod = nullptr;
            jmethodID m_CancelMethod = nullptr;
            jmethodID m_HasVibratorMethod = nullptr;
            jmethodID m_HasAmplitudeControlMethod = nullptr;
            
            // VibrationEffect class (API 26+)
            jclass m_VibrationEffectClass = nullptr;
            jmethodID m_CreateOneShotMethod = nullptr;
            jmethodID m_CreateWaveformMethod = nullptr;
            
            // Constants
            int m_ApiLevel = 0;
            bool m_Initialized = false;
            bool m_HasAmplitude = false;
            
            // Predefined effect constants
            jobject m_ClickEffect = nullptr;
            jobject m_HeavyClickEffect = nullptr;
            jobject m_TickEffect = nullptr;
            jobject m_DoubleClickEffect = nullptr;
            jmethodID m_PerformEffectMethod = nullptr;
            
            void initJNI();
            void cleanupJNI();
            JNIEnv* attachThread();
            void detachThread();
            
            // Predefined patterns (timings in ms)
            static constexpr int64_t CLICK_DURATION = 10;
            static constexpr int CLICK_AMPLITUDE = 128;
            static constexpr int64_t HEAVY_CLICK_DURATION = 20;
            static constexpr int HEAVY_CLICK_AMPLITUDE = 255;
            static constexpr int64_t TICK_DURATION = 5;
            static constexpr int TICK_AMPLITUDE = 64;
            static constexpr int64_t DOUBLE_CLICK_TIMING[] = {0, 10, 50, 10};
            static constexpr int DOUBLE_CLICK_AMPLITUDE[] = {128, 0, 128};
            
        public:
            AndroidHaptics(void* app);
            ~AndroidHaptics() override;
            
            bool init() override;
            void shutdown() override;
            
            void vibrate(int64_t duration_ms, int amplitude = -1) override;
            void vibratePattern(const std::vector<int64_t>& timings, 
                               const std::vector<int>& amplitudes = {}) override;
            
            void click() override;
            void heavyClick() override;
            void tick() override;
            void doubleClick() override;
            
            void stop() override;
            
            bool hasHaptics() const override;
            bool hasAmplitudeControl() const override;
        };

    }
}

#endif // PN_PLATFORM_ANDROID

#endif // ANDROID_HAPTICS_HPP

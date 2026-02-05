/*****************************************************************//**
 * \file   AndroidHaptics.cpp
 * \brief  Android haptics implementation using JNI
 *
 * \author PAIN Engine
 * \date   2025
 *********************************************************************/

#include "pch.h"
#include "AndroidHaptics.h"

#ifdef PN_PLATFORM_ANDROID

#include <android/log.h>

namespace PAIN {
    namespace Haptics {

        // JNI method signatures
        static constexpr char VIBRATOR_CLASS[] = "android/os/Vibrator";
        static constexpr char VIBRATOR_MANAGER_CLASS[] = "android/os/VibratorManager";
        static constexpr char VIBRATION_EFFECT_CLASS[] = "android/os/VibrationEffect";
        static constexpr char CONTEXT_CLASS[] = "android/content/Context";

        AndroidHaptics::AndroidHaptics(void* app) 
            : m_App(static_cast<android_app*>(app)) {
        }

        AndroidHaptics::~AndroidHaptics() {
            shutdown();
        }

        JNIEnv* AndroidHaptics::attachThread() {
            if (!m_App || !m_App->activity || !m_App->activity->vm) {
                return nullptr;
            }
            
            JavaVM* vm = m_App->activity->vm;
            JNIEnv* env = nullptr;
            
            // Check if already attached
            jint attachResult = vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6);
            if (attachResult == JNI_EDETACHED) {
                if (vm->AttachCurrentThread(&env, nullptr) != 0) {
                    __android_log_print(ANDROID_LOG_ERROR, "PAIN", "Failed to attach thread to JVM");
                    return nullptr;
                }
            } else if (attachResult != JNI_OK) {
                __android_log_print(ANDROID_LOG_ERROR, "PAIN", "Failed to get JNI environment");
                return nullptr;
            }
            
            return env;
        }

        void AndroidHaptics::detachThread() {
            if (m_App && m_App->activity && m_App->activity->vm) {
                // Only detach if we were the ones who attached
                JNIEnv* env = nullptr;
                if (m_App->activity->vm->GetEnv(reinterpret_cast<void**>(&env), JNI_VERSION_1_6) == JNI_OK) {
                    m_App->activity->vm->DetachCurrentThread();
                }
            }
        }

        void AndroidHaptics::initJNI() {
            JNIEnv* env = attachThread();
            if (!env) {
                __android_log_print(ANDROID_LOG_ERROR, "PAIN", "Failed to attach thread for JNI init");
                return;
            }

            // Get SDK version
            jclass buildClass = env->FindClass("android/os/Build$VERSION");
            if (buildClass) {
                jfieldID sdkIntField = env->GetStaticFieldID(buildClass, "SDK_INT", "I");
                if (sdkIntField) {
                    m_ApiLevel = env->GetStaticIntField(buildClass, sdkIntField);
                }
                env->DeleteLocalRef(buildClass);
            }

            // Get context (NativeActivity)
            jobject context = m_App->activity->clazz;

            // Get Vibrator system service
            jclass contextClass = env->FindClass(CONTEXT_CLASS);
            if (!contextClass) {
                __android_log_print(ANDROID_LOG_ERROR, "PAIN", "Failed to find Context class");
                detachThread();
                return;
            }

            jmethodID getSystemService = env->GetMethodID(contextClass, "getSystemService", 
                "(Ljava/lang/String;)Ljava/lang/Object;");
            jfieldID vibratorServiceField = env->GetStaticFieldID(contextClass, "VIBRATOR_SERVICE", 
                "Ljava/lang/String;");
            jobject vibratorService = env->GetStaticObjectField(contextClass, vibratorServiceField);
            
            jobject vibrator = env->CallObjectMethod(context, getSystemService, vibratorService);
            
            if (vibrator) {
                m_VibratorObj = env->NewGlobalRef(vibrator);
                
                // Get Vibrator class methods
                m_VibratorClass = reinterpret_cast<jclass>(env->NewGlobalRef(
                    env->FindClass(VIBRATOR_CLASS)));
                
                m_HasVibratorMethod = env->GetMethodID(m_VibratorClass, "hasVibrator", "()Z");
                m_CancelMethod = env->GetMethodID(m_VibratorClass, "cancel", "()V");

                // Check if device has vibrator
                if (m_HasVibratorMethod) {
                    jboolean hasVibrator = env->CallBooleanMethod(m_VibratorObj, m_HasVibratorMethod);
                    if (!hasVibrator) {
                        __android_log_print(ANDROID_LOG_WARN, "PAIN", "Device does not have vibrator");
                        cleanupJNI();
                        detachThread();
                        return;
                    }
                }

                // API 26+ (Android 8.0) - VibrationEffect and amplitude control
                if (m_ApiLevel >= 26) {
                    m_VibrationEffectClass = reinterpret_cast<jclass>(env->NewGlobalRef(
                        env->FindClass(VIBRATION_EFFECT_CLASS)));
                    
                    if (m_VibrationEffectClass) {
                        // Create one shot: createOneShot(long milliseconds, int amplitude)
                        m_CreateOneShotMethod = env->GetStaticMethodID(m_VibrationEffectClass, 
                            "createOneShot", "(JI)Landroid/os/VibrationEffect;");
                        
                        // Create waveform: createWaveform(long[] timings, int[] amplitudes, int repeat)
                        m_CreateWaveformMethod = env->GetStaticMethodID(m_VibrationEffectClass,
                            "createWaveform", "([J[II)Landroid/os/VibrationEffect;");
                        
                        // Vibrate with effect
                        m_VibrateAmplitudeMethod = env->GetMethodID(m_VibratorClass, "vibrate",
                            "(Landroid/os/VibrationEffect;)V");
                        
                        // Check amplitude control
                        m_HasAmplitudeControlMethod = env->GetMethodID(m_VibratorClass,
                            "hasAmplitudeControl", "()Z");
                        if (m_HasAmplitudeControlMethod) {
                            m_HasAmplitude = env->CallBooleanMethod(m_VibratorObj, m_HasAmplitudeControlMethod);
                        }

                        // Get predefined effects (API 29+)
                        if (m_ApiLevel >= 29) {
                            // Effect constants
                            jfieldID clickField = env->GetStaticFieldID(m_VibrationEffectClass,
                                "EFFECT_CLICK", "I");
                            jfieldID heavyClickField = env->GetStaticFieldID(m_VibrationEffectClass,
                                "EFFECT_HEAVY_CLICK", "I");
                            jfieldID tickField = env->GetStaticFieldID(m_VibrationEffectClass,
                                "EFFECT_TICK", "I");
                            jfieldID doubleClickField = env->GetStaticFieldID(m_VibrationEffectClass,
                                "EFFECT_DOUBLE_CLICK", "I");
                            
                            // Get effect method: get(int effectId)
                            jmethodID getEffectMethod = env->GetStaticMethodID(m_VibrationEffectClass,
                                "get", "(I)Landroid/os/VibrationEffect;");
                            
                            if (getEffectMethod) {
                                jint clickId = env->GetStaticIntField(m_VibrationEffectClass, clickField);
                                jobject clickEffect = env->CallStaticObjectMethod(m_VibrationEffectClass,
                                    getEffectMethod, clickId);
                                if (clickEffect) {
                                    m_ClickEffect = env->NewGlobalRef(clickEffect);
                                    env->DeleteLocalRef(clickEffect);
                                }
                                
                                jint heavyClickId = env->GetStaticIntField(m_VibrationEffectClass, heavyClickField);
                                jobject heavyClickEffect = env->CallStaticObjectMethod(m_VibrationEffectClass,
                                    getEffectMethod, heavyClickId);
                                if (heavyClickEffect) {
                                    m_HeavyClickEffect = env->NewGlobalRef(heavyClickEffect);
                                    env->DeleteLocalRef(heavyClickEffect);
                                }
                                
                                jint tickId = env->GetStaticIntField(m_VibrationEffectClass, tickField);
                                jobject tickEffect = env->CallStaticObjectMethod(m_VibrationEffectClass,
                                    getEffectMethod, tickId);
                                if (tickEffect) {
                                    m_TickEffect = env->NewGlobalRef(tickEffect);
                                    env->DeleteLocalRef(tickEffect);
                                }
                                
                                jint doubleClickId = env->GetStaticIntField(m_VibrationEffectClass, doubleClickField);
                                jobject doubleClickEffect = env->CallStaticObjectMethod(m_VibrationEffectClass,
                                    getEffectMethod, doubleClickId);
                                if (doubleClickEffect) {
                                    m_DoubleClickEffect = env->NewGlobalRef(doubleClickEffect);
                                    env->DeleteLocalRef(doubleClickEffect);
                                }
                            }
                            
                            // Perform haptic feedback method
                            m_PerformEffectMethod = env->GetMethodID(m_VibratorClass, "performHapticFeedback",
                                "(I)Z");
                        }
                    }
                } else {
                    // Legacy API < 26
                    m_VibrateMethod = env->GetMethodID(m_VibratorClass, "vibrate", "(J)V");
                    m_VibratePatternMethod = env->GetMethodID(m_VibratorClass, "vibrate", "([JI)V");
                }

                m_Initialized = true;
                __android_log_print(ANDROID_LOG_INFO, "PAIN", 
                    "Haptics initialized - API Level: %d, Amplitude Control: %s",
                    m_ApiLevel, m_HasAmplitude ? "yes" : "no");
                
                env->DeleteLocalRef(vibrator);
            }
            
            env->DeleteLocalRef(vibratorService);
            env->DeleteLocalRef(contextClass);
            
            detachThread();
        }

        void AndroidHaptics::cleanupJNI() {
            JNIEnv* env = attachThread();
            if (!env) return;

            if (m_ClickEffect) {
                env->DeleteGlobalRef(m_ClickEffect);
                m_ClickEffect = nullptr;
            }
            if (m_HeavyClickEffect) {
                env->DeleteGlobalRef(m_HeavyClickEffect);
                m_HeavyClickEffect = nullptr;
            }
            if (m_TickEffect) {
                env->DeleteGlobalRef(m_TickEffect);
                m_TickEffect = nullptr;
            }
            if (m_DoubleClickEffect) {
                env->DeleteGlobalRef(m_DoubleClickEffect);
                m_DoubleClickEffect = nullptr;
            }
            if (m_VibratorObj) {
                env->DeleteGlobalRef(m_VibratorObj);
                m_VibratorObj = nullptr;
            }
            if (m_VibratorClass) {
                env->DeleteGlobalRef(m_VibratorClass);
                m_VibratorClass = nullptr;
            }
            if (m_VibrationEffectClass) {
                env->DeleteGlobalRef(m_VibrationEffectClass);
                m_VibrationEffectClass = nullptr;
            }

            detachThread();
            m_Initialized = false;
        }

        bool AndroidHaptics::init() {
            if (m_Initialized) return true;
            if (!m_App) return false;
            
            initJNI();
            return m_Initialized;
        }

        void AndroidHaptics::shutdown() {
            if (!m_Initialized) return;
            cleanupJNI();
        }

        void AndroidHaptics::vibrate(int64_t duration_ms, int amplitude) {
            if (!m_Initialized || !m_VibratorObj) return;

            JNIEnv* env = attachThread();
            if (!env) return;

            if (m_ApiLevel >= 26 && m_CreateOneShotMethod && m_VibrateAmplitudeMethod) {
                // Use VibrationEffect for API 26+
                jint amp = (amplitude < 0) ? -1 : amplitude;  // -1 = default
                jobject effect = env->CallStaticObjectMethod(m_VibrationEffectClass, 
                    m_CreateOneShotMethod, static_cast<jlong>(duration_ms), amp);
                
                if (effect) {
                    env->CallVoidMethod(m_VibratorObj, m_VibrateAmplitudeMethod, effect);
                    env->DeleteLocalRef(effect);
                }
            } else if (m_VibrateMethod) {
                // Legacy API < 26
                env->CallVoidMethod(m_VibratorObj, m_VibrateMethod, static_cast<jlong>(duration_ms));
            }

            detachThread();
        }

        void AndroidHaptics::vibratePattern(const std::vector<int64_t>& timings, 
                                           const std::vector<int>& amplitudes) {
            if (!m_Initialized || !m_VibratorObj || timings.empty()) return;

            JNIEnv* env = attachThread();
            if (!env) return;

            // Create Java long array for timings
            jlongArray timingArray = env->NewLongArray(static_cast<jsize>(timings.size()));
            if (!timingArray) {
                detachThread();
                return;
            }
            
            std::vector<jlong> jlongTimings(timings.begin(), timings.end());
            env->SetLongArrayRegion(timingArray, 0, static_cast<jsize>(timings.size()), jlongTimings.data());

            if (m_ApiLevel >= 26 && m_CreateWaveformMethod && m_VibrateAmplitudeMethod) {
                // Use VibrationEffect with amplitudes
                jobject effect = nullptr;
                
                if (!amplitudes.empty() && m_HasAmplitude) {
                    // Create amplitude array
                    jintArray amplitudeArray = env->NewIntArray(static_cast<jsize>(amplitudes.size()));
                    if (amplitudeArray) {
                        std::vector<jint> jintAmplitudes(amplitudes.begin(), amplitudes.end());
                        env->SetIntArrayRegion(amplitudeArray, 0, static_cast<jsize>(amplitudes.size()), 
                                              jintAmplitudes.data());
                        
                        effect = env->CallStaticObjectMethod(m_VibrationEffectClass, m_CreateWaveformMethod,
                            timingArray, amplitudeArray, -1);
                        env->DeleteLocalRef(amplitudeArray);
                    }
                } else {
                    // No amplitudes, use default
                    effect = env->CallStaticObjectMethod(m_VibrationEffectClass, m_CreateWaveformMethod,
                        timingArray, nullptr, -1);
                }
                
                if (effect) {
                    env->CallVoidMethod(m_VibratorObj, m_VibrateAmplitudeMethod, effect);
                    env->DeleteLocalRef(effect);
                }
            } else if (m_VibratePatternMethod) {
                // Legacy pattern vibration
                env->CallVoidMethod(m_VibratorObj, m_VibratePatternMethod, timingArray, -1);
            }

            env->DeleteLocalRef(timingArray);
            detachThread();
        }

        void AndroidHaptics::click() {
            if (!m_Initialized) return;

            JNIEnv* env = attachThread();
            if (!env) return;

            // Try predefined effect first (API 29+)
            if (m_ClickEffect && m_VibrateAmplitudeMethod) {
                env->CallVoidMethod(m_VibratorObj, m_VibrateAmplitudeMethod, m_ClickEffect);
            } else {
                // Fallback to duration-based
                vibrate(CLICK_DURATION, CLICK_AMPLITUDE);
            }

            detachThread();
        }

        void AndroidHaptics::heavyClick() {
            if (!m_Initialized) return;

            JNIEnv* env = attachThread();
            if (!env) return;

            if (m_HeavyClickEffect && m_VibrateAmplitudeMethod) {
                env->CallVoidMethod(m_VibratorObj, m_VibrateAmplitudeMethod, m_HeavyClickEffect);
            } else {
                vibrate(HEAVY_CLICK_DURATION, HEAVY_CLICK_AMPLITUDE);
            }

            detachThread();
        }

        void AndroidHaptics::tick() {
            if (!m_Initialized) return;

            JNIEnv* env = attachThread();
            if (!env) return;

            if (m_TickEffect && m_VibrateAmplitudeMethod) {
                env->CallVoidMethod(m_VibratorObj, m_VibrateAmplitudeMethod, m_TickEffect);
            } else {
                vibrate(TICK_DURATION, TICK_AMPLITUDE);
            }

            detachThread();
        }

        void AndroidHaptics::doubleClick() {
            if (!m_Initialized) return;

            JNIEnv* env = attachThread();
            if (!env) return;

            if (m_DoubleClickEffect && m_VibrateAmplitudeMethod) {
                env->CallVoidMethod(m_VibratorObj, m_VibrateAmplitudeMethod, m_DoubleClickEffect);
            } else {
                // Use pattern
                std::vector<int64_t> timings = {0, DOUBLE_CLICK_TIMING[0], DOUBLE_CLICK_TIMING[1], 
                                               DOUBLE_CLICK_TIMING[2], DOUBLE_CLICK_TIMING[3]};
                std::vector<int> amplitudes = {DOUBLE_CLICK_AMPLITUDE[0], 0, DOUBLE_CLICK_AMPLITUDE[2]};
                vibratePattern(timings, amplitudes);
            }

            detachThread();
        }

        void AndroidHaptics::stop() {
            if (!m_Initialized || !m_CancelMethod) return;

            JNIEnv* env = attachThread();
            if (!env) return;

            env->CallVoidMethod(m_VibratorObj, m_CancelMethod);
            
            detachThread();
        }

        bool AndroidHaptics::hasHaptics() const {
            return m_Initialized && m_VibratorObj != nullptr;
        }

        bool AndroidHaptics::hasAmplitudeControl() const {
            return m_HasAmplitude;
        }

    }
}

#endif // PN_PLATFORM_ANDROID

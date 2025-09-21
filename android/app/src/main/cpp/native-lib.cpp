#include <jni.h>
#include <string>
#include <PAINENGINE/Logging/Log.h>

#include "Application.h"    // Your ECS-based Application
#include "CoreSystems/Events/Event.h"

#ifndef LOG_TAG
#define LOG_TAG "NativeTemplate"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif

// ---------------- Globals ----------------
static PAIN::Application* g_app = nullptr;

// ---------------- JNI Functions ----------------
extern "C" {

// Return a simple string
JNIEXPORT jstring JNICALL
Java_com_example_jnicpp_MainActivity_stringFromJNI(JNIEnv* env, jobject /*this*/) {
    return env->NewStringUTF("PAINEngine ECS on Android");
}

// Initialize ECS Application
JNIEXPORT jboolean JNICALL
Java_com_example_jnicpp_MainActivity_initApp(JNIEnv* env, jobject /*this*/, jobject assetManager) {
    LOGI("initApp called");

    if (!g_app) {
        LOGI("Creating PAIN::Application");
        g_app = new PAIN::Application::Application();   // same factory you use on Windows
        if (!g_app) {
            LOGE("Failed to create PAIN::Application");
            return JNI_FALSE;
        }
    }

    // AssetManager if you need to forward it to your systems
    // if (assetManager) {
    //     AAssetManager* nativeAM = AAssetManager_fromJava(env, assetManager);
    //     if (!nativeAM) {
    //         LOGE("Failed to obtain native AssetManager");
    //         return JNI_FALSE;
    //     }
    //     LOGI("AssetManager obtained successfully");
    //     // You can store `nativeAM` in your Application or ResourceManager
    // }

    return JNI_TRUE;
}

// Per-frame update (called from Java render loop)
JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_renderFrame(JNIEnv*, jobject /*this*/) {
    if (g_app) {
        // On Windows this is inside Application::Run loop.
        // On Android we call one tick per frame.
        g_app->drainEventQueue();

        // Update layers
        for (auto& layer : g_app->layer_stack) {
            layer->onUpdate();
        }

        // Update core systems
        for (auto& core : g_app->core_stack) {
            core->onUpdate();
        }
    }
}

// Cleanup ECS Application
JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_cleanupApp(JNIEnv*, jobject /*this*/) {
    LOGI("cleanupApp called");

    if (g_app) {
        delete g_app;
        g_app = nullptr;
        LOGI("Application deleted");
    }
}

// // Resize event
// JNIEXPORT void JNICALL
// Java_com_example_jnicpp_MainActivity_setDisplaySize(JNIEnv*, jobject /*this*/, jint width, jint height) {
//     if (g_app) {
//         auto evt = std::make_shared<PAIN::Event::WindowResize>(width, height);
//         g_app->pushEventQueue(evt);
//     }
// }

// // Touch input event
// JNIEXPORT void JNICALL
// Java_com_example_jnicpp_MainActivity_handleTouchEvent(JNIEnv*, jobject /*this*/, jint action, jfloat x, jfloat y) {
//     if (g_app) {
//         auto evt = std::make_shared<PAIN::Event::>(action, x, y);
//         g_app->pushEventQueue(evt);
//     }
// }

// // Key input event
// JNIEXPORT void JNICALL
// Java_com_example_jnicpp_MainActivity_handleKeyEvent(JNIEnv*, jobject /*this*/, jint key, jint action) {
//     if (g_app) {
//         auto evt = std::make_shared<PAIN::Event::KeyEvent>(key, action);
//         g_app->pushEventQueue(evt);
//     }
// }

} // extern "C"

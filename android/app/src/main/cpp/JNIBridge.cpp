#include <jni.h>
#include <memory>
#include <android/asset_manager_jni.h>
#include "PAINEngine/Applications/Application.h"

// The CreateApplication function is defined in Game.cpp and returns our game instance.
namespace PAIN {
    Application* CreateApplication();
}

// Global pointer to the C++ engine application instance.
static std::unique_ptr<PAIN::Application> g_App;

extern "C" {

JNIEXPORT void JNICALL
Java_com_pain_engine_MainActivity_nativeInit(JNIEnv *env, jclass clazz, jobject assetManager)
{
    AAssetManager* nativeAssetManager = AAssetManager_fromJava(env, assetManager);
    if (nativeAssetManager == nullptr) {
        return;
    }

    g_App.reset(PAIN::CreateApplication());
    g_App->InitializeAndroid(nativeAssetManager);
}

JNIEXPORT void JNICALL
Java_com_pain_engine_MainActivity_nativeOnSurfaceChanged(JNIEnv *env, jclass clazz, jint width, jint height)
{
    if (g_App) {}
}

JNIEXPORT void JNICALL
Java_com_pain_engine_MainActivity_nativeOnDrawFrame(JNIEnv *env, jclass clazz)
{
    if (g_App)
    {
        g_App->Update(); // Use single-frame Update() for Android
    }
}

JNIEXPORT void JNICALL
Java_com_pain_engine_MainActivity_nativeOnPause(JNIEnv *env, jclass clazz)
{
    if (g_App) {}
}

JNIEXPORT void JNICALL
Java_com_pain_engine_MainActivity_nativeOnResume(JNIEnv *env, jclass clazz)
{
    if (g_App) {}
}

JNIEXPORT void JNICALL
Java_com_pain_engine_MainActivity_nativeOnDestroy(JNIEnv *env, jclass clazz)
{
    if (g_App)
    {
        g_App.reset();
    }
}

} // extern "C"
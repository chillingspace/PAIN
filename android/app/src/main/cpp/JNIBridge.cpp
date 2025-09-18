#include <jni.h>
#include <memory>
#include <PAINEngine.h>
#include <android/asset_manager_jni.h>

std::unique_ptr<PAIN::Application> app;

extern "C" {

JNIEXPORT void JNICALL
Java_com_pain_engine_MyGLRenderer_onSurfaceCreated(JNIEnv *env, jobject thiz, jobject assetManager) {
    if (!app) {
        AAssetManager* mgr = AAssetManager_fromJava(env, assetManager);
        app = std::unique_ptr<PAIN::Application>(PAIN::CreateApplication());
        app->InitializeAndroid(mgr);
    }
}

JNIEXPORT void JNICALL
Java_com_pain_engine_MyGLRenderer_onSurfaceChanged(JNIEnv *env, jobject thiz, jint width, jint height) {
    glViewport(0, 0, width, height);
}

JNIEXPORT void JNICALL
Java_com_pain_engine_MyGLRenderer_onDrawFrame(JNIEnv *env, jobject thiz) {
    if (app) {
        app->Update();
    }
}

} // extern "C"
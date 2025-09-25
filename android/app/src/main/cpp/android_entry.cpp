// android_entry.cpp
#include <android/log.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h> // provides android_main() dispatch
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <PAINEngine.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "PAIN", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PAIN", __VA_ARGS__)

extern PAIN::Application* PAIN::CreateApplication();

extern "C" void android_main(android_app* app) {

    //Game entry
    PAIN::Application* game = PAIN::CreateApplication();

    //Check if application is created early return
    if(!game) return;

    //App command handles events
    app->userData = game;

    //Run engine
    game->Init(app);
    game->Run();
    delete game;
}
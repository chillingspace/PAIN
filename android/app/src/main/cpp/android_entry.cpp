// android_entry.cpp
#include <android/log.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h> // provides android_main() dispatch
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#include <PAINEngine.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "PAIN", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PAIN", __VA_ARGS__)

static void handle_cmd(android_app* app, int32_t cmd) {
    PAIN::Application* game = (PAIN::Application*)app->userData;

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            game->Init(app);
            LOGI("APP_CMD_INIT_WINDOW");
            break;
        case APP_CMD_TERM_WINDOW:
            game->terminate();
            LOGI("APP_CMD_TERM_WINDOW");
            break;
        case APP_CMD_GAINED_FOCUS:
            LOGI("APP_CMD_GAINED_FOCUS");
            break;
        case APP_CMD_LOST_FOCUS:
            LOGI("APP_CMD_LOST_FOCUS");
            break;
    }
}

extern PAIN::Application* PAIN::CreateApplication();

extern "C" void android_main(android_app* app) {

    //Game entry
    PAIN::Application* game = PAIN::CreateApplication();

    //Check if application is created early return
    if(!game) return;

    //App command handles events
    app->userData = game;
    app->onAppCmd = handle_cmd;

    //Game loop
    while(!game->getAppState()) {
        int events;
        android_poll_source* source;

        //Process events waiting for game to init
        while (ALooper_pollOnce(-1, nullptr, &events,
                                (void**)&source) >= 0) {
            if (source) {
                source->process(app, source);
            }

            if(game->getAppState()) break;
        }
    }

    game->Run();
    delete game;
}
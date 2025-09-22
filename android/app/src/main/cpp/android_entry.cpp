// android_entry.cpp
#include <android/log.h>
#include <android/native_activity.h>
#include <android_native_app_glue.h> // provides android_main() dispatch
#include <EGL/egl.h>
#include <GLES3/gl3.h>

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,  "PAIN", __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "PAIN", __VA_ARGS__)

static void handle_cmd(android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            // TODO: create EGL context & surface on app->window
            LOGI("APP_CMD_INIT_WINDOW");
            break;
        case APP_CMD_TERM_WINDOW:
            // TODO: destroy EGL resources
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

void android_main(android_app* app) {
    app->onAppCmd = handle_cmd;

    // You can fetch AAssetManager here and hand it to your engine (ImGui/your IO/FM0D needs it)
    // AAssetManager* mgr = app->activity->assetManager;

    // Main loop
    int events;
    android_poll_source* source = nullptr;
    while (true) {
        while (ALooper_pollOnce(0, nullptr, &events, (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                LOGI("Destroy requested");
                return;
            }
        }

        // TODO: engine tick + render
        // glClear(GL_COLOR_BUFFER_BIT);
        // eglSwapBuffers(display, surface);
    }
}
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

extern "C" void android_main(android_app* app) {
    app->onAppCmd = handle_cmd;

    // Make engine, but do NOT call Run() yet.
    auto* game = PAIN::CreateApplication();  // returns your Application*

    // Wait until APP_CMD_INIT_WINDOW to create EGL + attach window:
    int events; android_poll_source* source = nullptr;
    bool running = true;
    while (running) {
        while (ALooper_pollOnce(0, nullptr, &events, (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) { running = false; break; }
        }

        //eglSwapBuffers(display, surface);
    }
}
#pragma once

#ifdef _DEBUG
#ifdef PN_PLATFORM_ANDROID
#ifndef EDITOR_ANDROID_HPP
#define EDITOR_ANDROID_HPP

#include "../EditorPlatform.h"
#include "imgui_impl_android.h"

namespace PAIN {
    namespace Editor {

        //GLFW Editor
        class EditorAndroid : public EditorPlatform {
        private:
            ANativeWindow* a_window = nullptr;
            virtual void init() override;
            virtual void shutdown() override;
        public:
            EditorAndroid(ANativeWindow* a_window) :a_window{ a_window } { init(); }
            ~EditorAndroid() override { shutdown(); }
            virtual void beginFrame() override;
        };
    }
}

#endif
#endif
#endif
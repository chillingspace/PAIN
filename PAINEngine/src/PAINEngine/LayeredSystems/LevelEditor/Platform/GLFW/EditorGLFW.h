#pragma once

#ifdef _DEBUG
#ifdef PN_PLATFORM_WINDOWS
#ifndef EDITOR_GLFW_HPP
#define EDITOR_GLFW_HPP

#include "../EditorPlatform.h"
#include "imgui_impl_glfw.h"

namespace PAIN {
    namespace Editor {

        //GLFW Editor
        class EditorGLFW : public EditorPlatform {
        private:
            GLFWwindow* g_window = nullptr;
            void init() override;
            void shutdown() override;
        public:
            EditorGLFW(GLFWwindow* g_window) : g_window{ g_window } { init(); }
            ~EditorGLFW() override { shutdown(); }
            void beginFrame() override;

            void handleEvents(Event::Event& event) override;
        };
    }
}

#endif
#endif
#endif
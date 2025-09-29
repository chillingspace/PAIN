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

            //Mapping of glfw keys to imgui keys
            int imguiKeyMapping(int code);

            //Events that imgui listens for
            void handleKeyEvents(ImGuiIO& io, Event::Event& event);
            void handleMouseEvents(ImGuiIO& io, Event::Event& event);
            void handleWindowEvents(ImGuiIO& io, Event::Event& event);
        public:
            EditorGLFW(GLFWwindow* g_window) : g_window{ g_window } { init(); }
            ~EditorGLFW() override { shutdown(); }
            void beginFrame() override;

            void updateShortCuts(std::shared_ptr<CommandManager> command) override;

            void handleEvents(Event::Event& event) override;
        };
    }
}

#endif
#endif
#endif
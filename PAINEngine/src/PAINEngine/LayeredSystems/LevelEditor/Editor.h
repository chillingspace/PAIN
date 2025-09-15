#pragma once

#ifdef _DEBUG
#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include <ImGui/headers/imgui.h>
#include <ImGui/headers/imgui_impl_glfw.h>
#include <ImGui/headers/imgui_impl_opengl3.h>
#include "CoreSystems/Windows/Window.h"

#include "Command.h"

namespace PAIN {
    namespace Editor {

        //Editor
        class Editor : public AppSystem {
        public:
            Editor();
            ~Editor();                       // no override

            void onAttach() override;
            void onDetach() override;
            void onUpdate() override;
            void onEvent(Event::Event& event) override;

        private:

            //Actions manager
            std::unique_ptr<CommandManager> action_manager;

            void BeginFrame();

            void EndFrame();

            //Mapping of glfw keys to imgui keys
            int imguiKeyMapping(int code);

            //Events that imgui listens for
            void handleKeyEvents(ImGuiIO& io, Event::Event& event);
            void handleMouseEvents(ImGuiIO& io, Event::Event& event);
            void handleWindowEvents(ImGuiIO& io, Event::Event& event);
        };
    }
} 

// namespace PAIN
#endif // IMGUI_LAYER_HPP
#endif // PDEBUG

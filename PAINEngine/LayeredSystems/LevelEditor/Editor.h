#pragma once

#ifdef _DEBUG
#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include "CoreSystems/Windows/Window.h"

#include <ImGui/headers/imgui.h>
#include <ImGui/headers/imgui_impl_glfw.h>
#include <ImGui/headers/imgui_impl_opengl3.h>

//Command header
#include "LayeredSystems/LevelEditor/Command.h"

//Panels headers
#include "LayeredSystems/LevelEditor/Panels/Panels.h"

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

            //Panels
            std::unordered_map<std::string, std::shared_ptr<Panel::IPanel>> panels;

            //Actions manager
            std::shared_ptr<CommandManager> command_manager;

            void updateShortCuts();

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

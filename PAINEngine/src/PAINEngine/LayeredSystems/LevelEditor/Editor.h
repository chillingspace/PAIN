#pragma once

#ifdef _DEBUG
#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include "CoreSystems/Windows/Window.h"

//Panels headers
#include "Panels/Panels.h"

//Platform header
#include "Platform/EditorPlatform.h"

namespace PAIN {
    namespace Editor {

        //Panels map
        class PanelsMap : public Custom::ClassMap {
        public:
            PanelsMap() = default;
        };

        //Editor
        class Editor : public AppSystem {
        public:
            Editor(void* window);
            ~Editor();                       // no override

            void onAttach() override;
            void onDetach() override;
            void onUpdate(float dt) override;
            void onEvent(Event::Event& event) override;

        private:

            //Panels
            std::shared_ptr<PanelsMap> panels;

            //Actions manager
            std::shared_ptr<CommandManager> command_manager;

            //Platform editor
            std::shared_ptr<EditorPlatform> platform;

            template<typename T>
            void registerPanel(std::shared_ptr<T> panel);

            void BeginFrame();

            void EndFrame();

            void buildDockspace();
        };
    }
} 

// namespace PAIN
#endif // IMGUI_LAYER_HPP
#endif // PDEBUG

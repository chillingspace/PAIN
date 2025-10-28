#pragma once

#ifdef _DEBUG
#ifndef EDITOR_HPP
#define EDITOR_HPP

#include "Applications/AppSystem.h"
#include "Applications/Application.h"
#include "CoreSystems/Windows/Window.h"
#include "CoreSystems/Scene/Scene.h"

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
            ~Editor() override;

            void onAttach() override;
            void onDetach() override;
            void onFixedUpdate(AppTiming timing) override {}
            void onUpdate(AppTiming timing) override;

            void onEvent(Event::Event& event) override;
            bool isVisible() const { return editor_visible; }
            bool isPaused() const { return editor_paused; }
            bool isDebugMode() const { return editor_debug; }

            void toggleVisible() { editor_visible = !editor_visible; }
            void togglePause() { editor_paused = !editor_paused; }
            void toggleDebugMode() { editor_debug = !editor_debug; }

            // Template implementations must be in header or included .inl file
            template<typename T>
            std::shared_ptr<T> getPanel() const {
                return panels->get<T>();
            }

            template<typename T>
            std::weak_ptr<T> getPanelWeak() const {
                return getPanel<T>();
            }

        private:

            //Panels
            std::shared_ptr<PanelsMap> panels;

            //Actions manager
            std::shared_ptr<CommandManager> command_manager;

            //Platform editor
            std::shared_ptr<EditorPlatform> platform;

            // Imgui ini file path
            std::string m_imgui_ini_path;

            template<typename T>
            void registerPanel(std::shared_ptr<T> panel);

            void BeginFrame();

            void EndFrame();

            void buildDockspace();

            bool editor_visible = true;
            bool editor_paused = false;
            bool editor_debug = true;
        };
    }
} 

// namespace PAIN
#endif // IMGUI_LAYER_HPP
#endif // PDEBUG

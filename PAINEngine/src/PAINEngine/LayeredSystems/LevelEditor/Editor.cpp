#include "pch.h"
#include "Editor.h"

#ifdef _DEBUG

// Panels
#include "Panels/ToolsPanel.h"
#include "Panels/AudioPanel.h"
#include "Panels/ScenesPanel.h"
#include "Panels/ComponentsPanel.h"
#include "Panels/ResourcePanel.h"

namespace PAIN {

    namespace Editor {

        Editor::Editor(void* window) {

            //Construct platform
            platform = std::shared_ptr<EditorPlatform>(EditorPlatform::createEditorPlatform(window));

            //Create panels ptr
            panels = std::make_shared<PanelsMap>();
        }

        Editor::~Editor() {}

        void Editor::onAttach() {

            //Construct command manager
            command_manager = std::make_shared<CommandManager>();

            //Register panels
            registerPanel(std::make_shared<Panel::Tools>());
            registerPanel(std::make_shared<Panel::DebugAudioPanel>());
            registerPanel(std::make_shared<Panel::ScenesPanel>());
            registerPanel(std::make_shared<Panel::ComponentsPanel>());

            #ifdef PN_PLATFORM_WINDOWS
            registerPanel(std::make_shared<Panel::ResourcePanel>());
            #endif
        }

        void Editor::onDetach() {
        }

        void Editor::onUpdate(AppTiming timing) {
            // Update shortcuts
            platform->updateShortCuts(command_manager);

            // Begin IMGUI Frame
            platform->beginFrame();

            static bool editor_visible = true;

            // Toggle visibility with a key (say F1)
            if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
                editor_visible = !editor_visible;
            }

            if (editor_visible) {
                // Build docking space for imgui
                buildDockspace();

                // Iterate through all panels
                panels->forEachOfType<Panel::IPanel>([](std::shared_ptr<Panel::IPanel> panel) {
                    panel->drawWindow();
                    });

                static bool show_demo = true;
                if (show_demo) ImGui::ShowDemoWindow(&show_demo);
            }

            // Signal end of frame for imgui
            platform->endFrame();
        }


        template<typename T>
        void Editor::registerPanel(std::shared_ptr<T> panel) {

            //Pass services pointer onto panels
            panel->command_manager = command_manager;
            panel->services = services;
            panels->set<T>(panel);
        }

        void Editor::onEvent(Event::Event& event) {

            //Pass down events to platform for handling
            platform->handleEvents(event);
        }

        void Editor::buildDockspace() {
            ImGuiViewport* vp = ImGui::GetMainViewport();

            // Reserve vertical space for the fixed Tools panel (menu + toolbar)
            const float menu_h = ImGui::GetFrameHeight(); // same as Tools
            const float toolbar_h = .0f;                   // same as Tools
            const float tools_h = menu_h + toolbar_h;

            // Position/size the dockspace host BELOW the tools bar
            ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + tools_h));
            ImGui::SetNextWindowSize(ImVec2(vp->Size.x, vp->Size.y - tools_h));
            ImGui::SetNextWindowViewport(vp->ID);

            ImGuiWindowFlags host_flags =
                ImGuiWindowFlags_NoDocking |
                ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoBackground;   // no menubar here; Tools owns the menu

            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            ImGui::Begin("##DockSpaceHost", nullptr, host_flags);

            ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f), dock_flags);

            ImGui::End();
            ImGui::PopStyleVar(2);
        }
    }
}

#endif

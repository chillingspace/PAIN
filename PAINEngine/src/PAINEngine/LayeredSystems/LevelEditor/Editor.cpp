#include "pch.h"
#include "Editor.h"

#ifdef _DEBUG

#include "PAINEngine/Audio/AudioManager.h"

// Panels
#include "Panels/ToolsPanel.h"
#include "Panels/AudioPanel.h"
#include "Panels/ScenesPanel.h"
#include "Panels/ComponentsPanel.h"


namespace PAIN {

    namespace Editor {

        Editor::Editor(void* window) {

            //Construct platform
            platform = std::shared_ptr<EditorPlatform>(EditorPlatform::createEditorPlatform(window));
        }

        Editor::~Editor() {}

        void Editor::onAttach() {

            //Construct command manager
            command_manager = std::make_shared<CommandManager>();

            //Register panels
            panels[CLASS_STR(Panel::Tools)] = std::make_shared<Panel::Tools>(command_manager);
            //PN_CORE_INFO(panels[CLASS_STR(Panel::Tools)]->getPanelName());

            panels[CLASS_STR(Panel::DebugAudioPanel)] = std::make_shared<Panel::DebugAudioPanel>(command_manager);

            panels[CLASS_STR(Panel::ScenesPanel)] = std::make_shared<Panel::ScenesPanel>(command_manager);

            panels[CLASS_STR(Panel::ComponentsPanel)] = std::make_shared<Panel::ComponentsPanel>(command_manager);
        }

        void Editor::onDetach() {
        }

        void Editor::onUpdate() {

            //Update shortcuts
            platform->updateShortCuts(command_manager);

            //Begin IMGUI Frame
            platform->beginFrame();

            //Build docking space for imgui
            buildDockspace();

            //Update all panels
            for (auto const& panel : panels) {
                panel.second->drawWindow();
            }

            static bool show_demo = true;
            if (show_demo) ImGui::ShowDemoWindow(&show_demo);

            //Signal end of frame for imgui
            platform->endFrame();
        }

        void Editor::onEvent(Event::Event& event) {

            //Pass down events to platform for handling
            platform->handleEvents(event);
        }

        void Editor::buildDockspace() {
            ImGuiViewport* vp = ImGui::GetMainViewport();

            // Reserve vertical space for the fixed Tools panel (menu + toolbar)
            const float menu_h = ImGui::GetFrameHeight(); // same as Tools
            const float toolbar_h = .2f;                   // same as Tools
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

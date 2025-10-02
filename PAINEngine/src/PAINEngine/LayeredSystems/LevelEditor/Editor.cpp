#include "pch.h"
#include "Editor.h"

#ifdef _DEBUG

// Panels
#include "Panels/ToolsPanel.h"
#include "Panels/AudioPanel.h"
#include "Panels/ScenesPanel.h"
#include "Panels/ComponentsPanel.h"
#include "Panels/ResourcePanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/EntityPanel.h"

#include "PAINEngine/CoreSystems/Renderer/RendererLayer.h"

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
            registerPanel(std::make_shared<Panel::ViewportPanel>());
            registerPanel(std::make_shared<Panel::EntityPanel>());


            #ifdef PN_PLATFORM_WINDOWS
            registerPanel(std::make_shared<Panel::ResourcePanel>());
            #endif


            //toggleVisible();
        }

        void Editor::onDetach() {
            panels = nullptr;
            platform = nullptr;
            command_manager = nullptr;
        }

        void Editor::onUpdate(AppTiming timing) {
            // Update shortcuts
            platform->updateShortCuts(command_manager);

            // Begin IMGUI Frame
            platform->beginFrame();

#ifdef PN_PLATFORM_ANDROID
            if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
                toggleVisible();
                PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");
            }
#else
            if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
                toggleVisible();
                PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");
            }
#endif


#ifdef PN_PLATFORM_ANDROID
            // --- ADD BUTTON HERE ---
            {
                ImGuiViewport* vp = ImGui::GetMainViewport();
                ImVec2 windowPos;
                ImVec2 windowPadding(10, 10); // distance from edges

                // Bottom-left corner
                windowPos.x = vp->Pos.x + windowPadding.x;
                windowPos.y = vp->Pos.y + vp->Size.y - windowPadding.y; // start from bottom

                ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, ImVec2(0.0f, 1.0f));
                // ImVec2(0.0f, 1.0f) pivots the window at its bottom-left corner

                ImGui::Begin("##EditorToggleWindow", nullptr,
                    ImGuiWindowFlags_NoDecoration |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoMove);

                if (ImGui::Button(editor_visible ? "Hide Editor" : "Show Editor")) {
                    toggleVisible();
                    PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");
                }

                ImGui::End();
            }
            // --- END BUTTON ---
#endif

            if (editor_visible) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Build docking space
                buildDockspace();

                auto renderer = services->get<RendererLayer>();
                auto vp = panels->get<Panel::ViewportPanel>();
                if (renderer && vp) {
                    vp->setRenderTexture(renderer->getFramebufferTexture(),
                        renderer->getFramebufferWidth(),
                        renderer->getFramebufferHeight());
                }

                // Iterate through all panels
                panels->forEachOfType<Panel::IPanel>([](std::shared_ptr<Panel::IPanel> panel) {
                    panel->drawWindow();
                    });

                static bool show_demo = false; // can toggle to true if want demo
                if (show_demo) ImGui::ShowDemoWindow(&show_demo);
            }

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

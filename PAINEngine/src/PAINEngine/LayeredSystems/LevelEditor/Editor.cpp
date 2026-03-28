#include "pch.h"
#include "Editor.h"
#include <filesystem> 

#ifdef _DEBUG

// Panels
#include "Panels/ToolsPanel.h"
#include "Panels/AudioPanel.h"
#include "Panels/ScenesPanel.h"
#include "Panels/ComponentsPanel.h"
#include "Panels/ResourcePanel.h"
#include "Panels/ViewportPanel.h"
#include "Panels/EntityPanel.h"
#include "Panels/DebugPanel.h"
#include "Panels/PrefabsPanel.h"
#include "Panels/AnimationPanel.h"
#include "Panels/ScriptPanel.h"

#include "CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Serialization/sSerialization.h"
#include "CoreSystems/Path/Path.h"
#include "ECS/Controller.h"

#define PN_CORE_ASSERT(cond, msg) \
    do { if (!(cond)) { PN_CORE_ERROR(msg); assert(cond); } } while(0)


namespace PAIN {

    namespace Editor {

        Editor::Editor(void* window) {

            //Construct platform
            platform = std::shared_ptr<EditorPlatform>(EditorPlatform::createEditorPlatform(window));

            //Create panels ptr
            panels = std::make_shared<PanelsMap>();

        }

        Editor::~Editor() {
        }


        void Editor::onAttach() {

            //Construct command manager
            command_manager = std::make_shared<CommandManager>(services);

            // Get ECS Service
            auto ecs = services->get<PAIN::ECS::Controller>();

            // get serialization service
            auto ser = services->get<PAIN::Serialization::Service>();
            PN_CORE_ASSERT(ser, "Serialization::Service not found in services");

            // Create EntityPanel first and keep a reference
            auto entity_panel = std::make_shared<Panel::EntityPanel>();
            registerPanel(entity_panel);

            registerPanel(std::make_shared<Panel::AudioPanel>());
            registerPanel(std::make_shared<Panel::ScenesPanel>());

            auto components_panel = std::make_shared<Panel::ComponentsPanel>();
            registerPanel(components_panel);

            // Create ViewportPanel and link it to EntityPanel
            auto viewport_panel = std::make_shared<Panel::ViewportPanel>();
            registerPanel(viewport_panel);

            auto animation_panel = std::make_shared<Panel::AnimationPanel>();
            registerPanel(animation_panel);

            registerPanel(std::make_shared<Panel::DebugPanel>());


#ifdef PN_PLATFORM_WINDOWS
            registerPanel(std::make_shared<Panel::Tools>());
            registerPanel(std::make_shared<Panel::ResourcePanel>());
            registerPanel(std::make_shared<Panel::PrefabPanel>());
            registerPanel(std::make_shared<Panel::ScriptPanel>());
#endif
            
            // Call onAttach on all registered panels
            panels->forEachOfType<Panel::IPanel>([](std::shared_ptr<Panel::IPanel> panel) {
                panel->onAttach();
            });

            // Load ImGui settings (layout, window positions, etc.)
            // Set ImGui ini file path during initialization (before first ImGui::NewFrame())

#ifdef PN_PLATFORM_WINDOWS
            m_imgui_ini_path = services->get<Path::Path>()->resolvePath("documents://imgui.ini");
#else
            m_imgui_ini_path = services->get<Path::Path>()->resolvePath("internal://imgui.ini");
#endif

            // Check if user's ini file mesh_id; if not, copy default from config folder
            if (!std::filesystem::exists(m_imgui_ini_path)) {
#ifdef PN_PLATFORM_WINDOWS
                auto default_ini_path = services->get<Path::Path>()->resolvePath("game_folder://imgui.ini");
                if (std::filesystem::exists(default_ini_path)) {
                    std::filesystem::copy_file(default_ini_path, m_imgui_ini_path);
                }
#else
                  // Android: Copy from assets using AAssetManager (implement later)
                // For now, ImGui will create a default ini file automatically
                PN_CORE_INFO("No existing imgui.ini found, ImGui will create default");

#endif
           
            }

            // Apply to ImGui
            ImGuiIO& io = ImGui::GetIO();
            io.IniFilename = m_imgui_ini_path.c_str();

            // Load saved layout (docking state, window positions, etc.)
            ImGui::LoadIniSettingsFromDisk(io.IniFilename);
            //PN_CORE_INFO("Taken From: {}", io.IniFilename);

            //toggleVisible();
        }

        void Editor::onDetach() {
            //auto ser = services->get<PAIN::Serialization::Service>();
            //if (ser) {
            //    PN_CORE_INFO("[Editor] Requesting save on detach");
            //    ser->saveCurrentScene();
            //}

            // Once set from io.inifilename, do not have to call the write io again

            //Call onDetach on all registered panels
            panels->forEachOfType<Panel::IPanel>([](std::shared_ptr<Panel::IPanel> panel) {
                panel->onDetach();
                });

            panels = nullptr;
            platform = nullptr;
            command_manager = nullptr;
        }

        void Editor::onUpdate(AppTiming timing) {
            PN_CORE_INFO("[ED-DIAG] onUpdate start");
            // Update shortcuts
            platform->updateShortCuts(command_manager);

            // Begin IMGUI Frame
            platform->beginFrame();


            // Skip hotkeys if user is typing in a text field
            if (!ImGui::GetIO().WantTextInput) {
                if (ImGui::IsKeyPressed(ImGuiKey_F1, false)) {

                    auto scene = services->get<Scene::SceneManager>();
                    auto window = services->get<Window::Window>();

                    // When hiding editor, auto-play the scene
                    if (!scene->isPlaying()) {
                        editor_visible ? scene->onPlay() : scene->onStop();
                    }
                    toggleVisible();
                    window->hideCursor(!editor_visible);
                    PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");
                }

                if (ImGui::IsKeyPressed(ImGuiKey_F2 , false)) {
                    editor_debug_mode = (editor_debug_mode + 1) % 4; // Cycles 0 -> 1 -> 2 -> 3 -> 0
                    if (editor_debug_mode == 0) PN_CORE_INFO("Editor debug rendering: OFF");
                    else if (editor_debug_mode == 1) PN_CORE_INFO("Editor debug rendering: ON (Physics Colliders)");
                    else if (editor_debug_mode == 2) PN_CORE_INFO("Editor debug rendering: ON (BVH Tree)");
                    else PN_CORE_INFO("Editor debug rendering: ON (Original Visual AABBs)");
                }
            }

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

                ImGui::Begin("##EditorToggleWindow", nullptr,
                    ImGuiWindowFlags_NoDecoration |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoMove);

                if (ImGui::Button(editor_visible ? "Hide Editor" : "Show Editor")) {
                    auto scene = services->get<Scene::SceneManager>();

                    // When hiding editor, auto-play the scene
                    if (!scene->isPlaying()) {
                        editor_visible ? scene->onPlay() : scene->onStop();
                    }

                    toggleVisible();
                    PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");
                }

                const char* debug_mode_labels[] = {
                "Show Physics Colliders",
                "Show BVH Tree",
                "Show Original AABBs",
                "Hide Debug Lines"
                };

                // Button to toggle debug lines in andriod
                if (ImGui::Button(debug_mode_labels[editor_debug_mode])) {
                    editor_debug_mode = (editor_debug_mode + 1) % 4;

                    if (editor_debug_mode == 0)
                        PN_CORE_INFO("Editor debug rendering: OFF");
                    else if (editor_debug_mode == 1)
                        PN_CORE_INFO("Editor debug rendering: ON (Physics Colliders)");
                    else if (editor_debug_mode == 2)
                        PN_CORE_INFO("Editor debug rendering: ON (BVH Tree)");
                    else
                        PN_CORE_INFO("Editor debug rendering: ON (Original Visual AABBs)");
                }

                ImGui::End();
            }
#endif



            if (editor_visible) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Build docking space
                buildDockspace();

                auto window = services->get<Window::Window>();
                auto renderer = services->get<sRenderer>();
                auto vp = panels->get<Panel::ViewportPanel>();

				// Always unhide cursor when editor is visible
                window->hideCursor(false);

                if (renderer && vp) {
                    vp->setRenderTexture(renderer->getFramebufferTexture(), renderer->getFramebufferWidth(), renderer->getFramebufferHeight());
                }

                // Iterate through all panels
                panels->forEachOfType<Panel::IPanel>([&, timing](std::shared_ptr<Panel::IPanel> panel) {
                    panel->drawWindow(timing);
                    });

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
#ifdef PN_PLATFORM_WINDOWS
            // If window closed event triggered set editor to visible
            if (event.getType() == Event::Type::WindowClosed) {
                auto scene = services->get<Scene::SceneManager>();
                if (!scene || !scene->isPlaying()) {
                    editor_visible = true;
                }
            }
#endif
            //Pass down events to platform for handling
            platform->handleEvents(event);

            //Dispatch events on to panels
            panels->forEachOfType<Panel::IPanel>([&event](std::shared_ptr<Panel::IPanel> panel) {
                panel->onEvent(event);
                });
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

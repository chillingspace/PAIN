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
            command_manager = std::make_shared<CommandManager>();

            // Get ECS Service
            auto ecs = services->get<PAIN::ECS::Controller>();

            // get serialization service
            auto ser = services->get<PAIN::Serialization::Service>();
            PN_CORE_ASSERT(ser, "Serialization::Service not found in services");

            // if ScenesPanel uses a hooks struct, fill it:
            Panel::ScenesHooks hooks{};

            hooks.onCreate = [ser](const std::string& base) {
                if (!ser->createNewScene(base)) {
                    PN_CORE_WARN("[ScenesPanel] createNewScene failed: {}", base.c_str());
                }
                };
            hooks.onSaveAs = [ser](const std::string& base) -> bool {
                if (!ser->saveSceneAs(base)) {
                    PN_CORE_WARN("[ScenesPanel] saveSceneAs failed: {}", base.c_str());
                    return false;
                }
                return true;
                };
            hooks.onSaveCurrent = [ser](const std::string& currSceneId) -> bool {
                if (!ser->saveCurrentScene()) {
                    PN_CORE_WARN("[ScenesPanel] saveCurrentScene failed");
                    return false;
                }
                return true;
                };
            hooks.onDelete = [ser](const std::string& sceneId) -> bool {
                if (!ser->deleteSceneById(sceneId)) {
                    PN_CORE_WARN("[ScenesPanel] deleteSceneById failed: {}", sceneId.c_str());
                    return false;
                }
                return true;
                };
            hooks.onChange = [ser](const std::string& sceneId) -> bool {
                if (!ser->loadSceneById(sceneId)) {
                    PN_CORE_WARN("[ScenesPanel] loadSceneById failed: {}", sceneId.c_str());
                    return false;
                }
                PN_CORE_INFO("[ScenesPanel] Loaded {}", sceneId.c_str());
                return true;
                };
            hooks.onModifyScene = [ser](const std::string& sceneId) { ser->modifyScene(); };
            hooks.onMaskChanged = [ser](unsigned i, unsigned j, bool v) {
                ser->setMask(i, j, v);      
                ser->modifyScene();         
                };

            hooks.onLayerVisibleChanged = [ser](unsigned idx, bool vis) {
                ser->setLayerVisible(idx, vis); 
                ser->modifyScene();
                };
            hooks.onDirty = [ser]() { ser->modifyScene(); };




            auto scenesPanel = std::make_shared<Panel::ScenesPanel>(hooks);

            // Create EntityPanel first and keep a reference
            auto entity_panel = std::make_shared<Panel::EntityPanel>();
            registerPanel(entity_panel);

            registerPanel(std::make_shared<Panel::Tools>());
            registerPanel(std::make_shared<Panel::AudioPanel>());
            registerPanel(std::make_shared<Panel::ScenesPanel>());
            registerPanel(scenesPanel);
            registerPanel(std::make_shared<Panel::ComponentsPanel>());

            // Create ViewportPanel and link it to EntityPanel
            auto viewport_panel = std::make_shared<Panel::ViewportPanel>();
            viewport_panel->setEntityPanel(entity_panel);  // LINK THEM TOGETHER
            registerPanel(viewport_panel);

            registerPanel(std::make_shared<Panel::DebugPanel>());


            //Register resource panel
#ifdef PN_PLATFORM_WINDOWS
            registerPanel(std::make_shared<Panel::ResourcePanel>());
#endif

            // Call onAttach on all registered panels
            panels->forEachOfType<Panel::IPanel>([](std::shared_ptr<Panel::IPanel> panel) {
                panel->onAttach();
            });

            // Load ImGui settings (layout, window positions, etc.)
            // Set ImGui ini file path during initialization (before first ImGui::NewFrame())

#ifdef PN_PLATFORM_WINDOWS
            m_imgui_ini_path = services->get<Path::Path>()->resolvePath("documents://imgui_layout.ini");
#else
            m_imgui_ini_path = services->get<Path::Path>()->resolvePath("internal://imgui_layout.ini");
#endif

            // Check if user's ini file mesh_id; if not, copy default from config folder
            if (!std::filesystem::exists(m_imgui_ini_path)) {
#ifdef PN_PLATFORM_WINDOWS
                auto default_ini_path = services->get<Path::Path>()->resolvePath("config://imgui_layout.ini");
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
            // Update shortcuts
            platform->updateShortCuts(command_manager);

            // Begin IMGUI Frame
            platform->beginFrame();

#ifdef PN_PLATFORM_ANDROID
            if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
                toggleVisible();
                PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");

                // When hiding editor, auto-play the scene
                if (!editor_visible) {
                    if (auto viewport = services->get<Panel::ViewportPanel>()) {
                        viewport->setSimulationState(false); // false = playing
                    }
                }
            }
#else
            if (ImGui::IsKeyPressed(ImGuiKey_F1)) {
                toggleVisible();
                PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");

                // When hiding editor, auto-play the scene
                if (!editor_visible) {
                    if (auto viewport = services->get<Panel::ViewportPanel>()) {
                        viewport->setSimulationState(false); // false = playing
                    }
                }
            }
            if (ImGui::IsKeyPressed(ImGuiKey_F2)) {
                editor_debug_mode = (editor_debug_mode + 1) % 3; // Cycles 0 -> 1 -> 2 -> 0
                if (editor_debug_mode == 0) PN_CORE_INFO("Editor debug rendering: OFF");
                else if (editor_debug_mode == 1) PN_CORE_INFO("Editor debug rendering: ON (Entity AABBs)");
                else PN_CORE_INFO("Editor debug rendering: ON (BVH Tree)");
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

                ImGui::Begin("##EditorToggleWindow", nullptr,
                    ImGuiWindowFlags_NoDecoration |
                    ImGuiWindowFlags_AlwaysAutoResize |
                    ImGuiWindowFlags_NoMove);

                if (ImGui::Button(editor_visible ? "Hide Editor" : "Show Editor")) {
                    toggleVisible();
                    PN_CORE_INFO("Editor visibility: {}", editor_visible ? "ON" : "OFF");

                    // When hiding editor, auto-play the scene
                    if (!editor_visible) {
                        if (auto viewport = services->get<Panel::ViewportPanel>()) {
                            viewport->setSimulationState(false); // false = playing
                        }
                    }
                }

                const char* debug_mode_labels[] = {
                "Show Debug Lines (Entity AABBs)",
                "Show Debug Lines (BVH Tree)",
                "Hide Debug Lines"
                };

                // Button to toggle debug lines in andriod
                if (ImGui::Button(debug_mode_labels[editor_debug_mode])) {
                    editor_debug_mode = (editor_debug_mode + 1) % 3;

                    if (editor_debug_mode == 0)
                        PN_CORE_INFO("Editor debug rendering: OFF");
                    else if (editor_debug_mode == 1)
                        PN_CORE_INFO("Editor debug rendering: ON (Entity AABBs)");
                    else
                        PN_CORE_INFO("Editor debug rendering: ON (BVH Tree)");
                }

                ImGui::End();
            }
#endif



            if (editor_visible) {
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Build docking space
                buildDockspace();

                auto renderer = services->get<sRenderer>();
                auto vp = panels->get<Panel::ViewportPanel>();

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
    
            // If window closed event triggered set editor to visible
            if (event.getType() == Event::Type::WindowClosed) {
                editor_visible = true;
            }

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

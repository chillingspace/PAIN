#include "pch.h"
#include "Editor.h"
#include <filesystem> 

#ifdef _DEBUG

//#include "CoreSystems/Events/GLFW/WindowEvents.h"
//#include "CoreSystems/Events/GLFW/KeyEvents.h"
//#include "CoreSystems/Events/GLFW/MouseEvents.h"
//#include "CoreSystems/Events/GLFW/AssetEvents.h"
#include "PAINEngine/Audio/AudioManager.h"

// Panels
#include "Panels/ToolsPanel.h"
#include "Panels/AudioPanel.h"
#include "Panels/ScenesPanel.h"
#include "Panels/ComponentsPanel.h"
#include "Panels/ResourcePanel.h"
#include "Panels/ViewportPanel.h"
#include "PAINEngine/CoreSystems/Serialization/sSerialization.h"
#include "Panels/EntityPanel.h"
#include "Panels/DebugPanel.h"

#include "PAINEngine/CoreSystems/Renderer/sRenderer.h"
#include "CoreSystems/Path/Path.h"
#include "PAINEngine/ECS/Controller.h"

#define PN_CORE_ASSERT(cond, msg) \
    do { if (!(cond)) { PN_CORE_ERROR(msg); assert(cond); } } while(0)


namespace PAIN {

    namespace Editor {

        //int Editor::imguiKeyMapping(int code) {

        //    //Code within 0 - 9 range
        //    if (code >= GLFW_KEY_0 && code <= GLFW_KEY_9) {
        //        return ImGuiKey_0 + (code - GLFW_KEY_0);
        //    }
        //    //Code within A - Z range
        //    else if (code >= GLFW_KEY_A && code <= GLFW_KEY_Z) {
        //        return ImGuiKey_A + (code - GLFW_KEY_A);
        //    }
        //    //Code within F1 - F24 range
        //    else if (code >= GLFW_KEY_F1 && code <= GLFW_KEY_F24) {
        //        return ImGuiKey_F1 + (code - GLFW_KEY_F1);
        //    }
        //    //Code within KeyPad_0 - KeyPad_Equal range
        //    else if (code >= GLFW_KEY_KP_0 && code <= GLFW_KEY_KP_EQUAL) {
        //        return ImGuiKey_Keypad0 + (code - GLFW_KEY_KP_0);
        //    }
        //    //Code within Apostrophe - Slash range
        //    else if (code >= GLFW_KEY_APOSTROPHE && code <= GLFW_KEY_SLASH) {
        //        return ImGuiKey_Apostrophe + (code - GLFW_KEY_APOSTROPHE);
        //    }
        //    //Code within left bracket & grave accent
        //    else if (code >= GLFW_KEY_LEFT_BRACKET && code <= GLFW_KEY_GRAVE_ACCENT) {
        //        return ImGuiKey_LeftBracket + (code - GLFW_KEY_LEFT_BRACKET);
        //    }
        //    //Code within caps lock & pause
        //    else if (code >= GLFW_KEY_CAPS_LOCK && code <= GLFW_KEY_PAUSE) {
        //        return ImGuiKey_CapsLock + (code - GLFW_KEY_CAPS_LOCK);
        //    }
        //    //Code within caps lock & pause
        //    else if (code >= GLFW_KEY_PAGE_UP && code <= GLFW_KEY_END) {
        //        return ImGuiKey_PageUp + (code - GLFW_KEY_PAGE_UP);
        //    }

        //    //Mouse button codes
        //    switch (code) {
        //    case GLFW_MOUSE_BUTTON_LEFT: return ImGuiKey_MouseLeft;
        //    case GLFW_MOUSE_BUTTON_RIGHT: return ImGuiKey_MouseRight;
        //    case GLFW_MOUSE_BUTTON_MIDDLE: return ImGuiKey_MouseMiddle;
        //    case GLFW_MOUSE_BUTTON_4: return ImGuiKey_MouseX1;
        //    case GLFW_MOUSE_BUTTON_5: return ImGuiKey_MouseX2;
        //    }

        //    //Remaing key codes that are not in order
        //    switch (code) {
        //    case GLFW_KEY_SPACE: return ImGuiKey_Space;
        //    case GLFW_KEY_SEMICOLON: return ImGuiKey_Semicolon;
        //    case GLFW_KEY_EQUAL: return ImGuiKey_Equal;
        //    case GLFW_KEY_ESCAPE: return ImGuiKey_Escape;
        //    case GLFW_KEY_ENTER: return ImGuiKey_Enter;
        //    case GLFW_KEY_TAB: return ImGuiKey_Tab;
        //    case GLFW_KEY_BACKSPACE: return ImGuiKey_Backspace;
        //    case GLFW_KEY_INSERT: return ImGuiKey_Insert;
        //    case GLFW_KEY_DELETE: return ImGuiKey_Delete;
        //    case GLFW_KEY_RIGHT: return ImGuiKey_RightArrow;
        //    case GLFW_KEY_LEFT: return ImGuiKey_LeftArrow;
        //    case GLFW_KEY_DOWN: return ImGuiKey_DownArrow;
        //    case GLFW_KEY_UP: return ImGuiKey_UpArrow;
        //    case GLFW_KEY_LEFT_SHIFT: return ImGuiKey_LeftShift;
        //    case GLFW_KEY_LEFT_CONTROL: return ImGuiKey_LeftCtrl;
        //    case GLFW_KEY_LEFT_ALT: return ImGuiKey_LeftAlt;
        //    case GLFW_KEY_LEFT_SUPER: return ImGuiKey_LeftSuper;
        //    case GLFW_KEY_RIGHT_SHIFT: return ImGuiKey_RightShift;
        //    case GLFW_KEY_RIGHT_CONTROL: return ImGuiKey_RightCtrl;
        //    case GLFW_KEY_RIGHT_ALT: return ImGuiKey_RightAlt;
        //    case GLFW_KEY_RIGHT_SUPER: return ImGuiKey_RightSuper;
        //    case GLFW_KEY_MENU: return ImGuiKey_Menu;
        //    }

        //    return ImGuiKey_None;
        //}

        Editor::Editor(void* window) {

            //Construct platform
            platform = std::shared_ptr<EditorPlatform>(EditorPlatform::createEditorPlatform(window));

            //Create panels ptr
            panels = std::make_shared<PanelsMap>();

        }

        Editor::~Editor() {
        }


        void Editor::onAttach() {

            // IMGUI_CHECKVERSION();
            // ImGui::CreateContext();

            // ImGuiIO& io = ImGui::GetIO();
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
            // io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;   
            // ImGui::StyleColorsDark();


            // ImGui_ImplGlfw_InitForOpenGL(glfwGetCurrentContext(), true);
            // ImGui_ImplOpenGL3_Init("#version 450");

            // ImGui::LoadIniSettingsFromDisk(io.IniFilename);

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
            hooks.onSaveAs = [ser](const std::string& base) {
                if (!ser->saveSceneAs(base)) {
                    PN_CORE_WARN("[ScenesPanel] saveSceneAs failed: {}", base.c_str());
                }
                };
            hooks.onSaveCurrent = [ser](const std::string& /*currSceneId*/) {
                if (!ser->saveCurrentScene()) {
                    PN_CORE_WARN("[ScenesPanel] saveCurrentScene failed");
                }
                };
            hooks.onDelete = [ser](const std::string& sceneId) {
                if (!ser->deleteSceneById(sceneId)) {
                    PN_CORE_WARN("[ScenesPanel] deleteSceneById failed: {}", sceneId.c_str());
                }
                };
            hooks.onChange = [ser](const std::string& sceneId) {
                if (!ser->loadSceneById(sceneId)) {
                    PN_CORE_WARN("[ScenesPanel] loadSceneById failed: {}", sceneId.c_str());
                }
                else {
                    PN_CORE_INFO("[ScenesPanel] Loaded {}", sceneId.c_str());
                }
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

            //Register panels
            registerPanel(std::make_shared<Panel::EntityPanel>());
            registerPanel(std::make_shared<Panel::Tools>());
            registerPanel(std::make_shared<Panel::AudioPanel>());
            registerPanel(std::make_shared<Panel::ScenesPanel>());
            registerPanel(scenesPanel);
            registerPanel(std::make_shared<Panel::ComponentsPanel>());

            // Create ViewportPanel and register it in BOTH panels and services
            auto viewport_panel = std::make_shared<Panel::ViewportPanel>();
            registerPanel(viewport_panel);

            registerPanel(std::make_shared<Panel::DebugPanel>());


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
            // For now i do a copy in assets folder andriod
            m_imgui_ini_path = services->get<Path::Path>()->resolvePath("internal://imgui_layout.ini");
#endif

            // Check if user's ini file exists; if not, copy default from config folder
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


            //toggleVisible();
        }

        void Editor::onDetach() {
            auto ser = services->get<PAIN::Serialization::Service>();
            if (ser) {
                PN_CORE_INFO("[Editor] Requesting save on detach");
                ser->saveCurrentScene();
            }

            // Once set from io.inifilename, do not have to call the write io again

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

                ImGui::End();
            }
#endif

            // ... rest of the function


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


                static bool show_demo = false; // can toggle to true if want demo
                if (show_demo) ImGui::ShowDemoWindow(&show_demo);
            }

            platform->endFrame();
        }

        //void Editor::updateShortCuts() {
        //    //Get IO
        //    ImGuiIO& io = ImGui::GetIO();

        //    //Undo
        //    static bool z_triggered = false;
        //    if (io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_Z)) {
        //        if (!z_triggered) {
        //            command_manager->undo();
        //            z_triggered = true;
        //        }
        //    }
        //    else {
        //        z_triggered = false;
        //    }

        //    //Redo
        //    static bool y_triggered = false;
        //    if (io.KeyCtrl && ImGui::IsKeyDown(ImGuiKey_Y)) {
        //        if (!y_triggered) {
        //            command_manager->redo();
        //            y_triggered = true;
        //        }
        //    }
        //    else {
        //        y_triggered = false;
        //    }
        //}

        void Editor::onEvent(Event::Event& event) {

            //Pass down events to platform for handling
            platform->handleEvents(event);

            //ImGuiIO& io = ImGui::GetIO();
            //if (event.isInCategory(Event::Keyboard) && io.WantCaptureKeyboard) {
            //    handleKeyEvents(io, event);
            //}
            //if (event.isInCategory(Event::Mouse) && io.WantCaptureMouse) {
            //    handleMouseEvents(io, event);
            //}
            //if (event.isInCategory(Event::Application)) {
            //    handleWindowEvents(io, event);
            //}
        }

        //void Editor::handleKeyEvents(ImGuiIO& io, Event::Event& event) {
        //    Event::Dispatcher dispatcher(event);
        //    dispatcher.Dispatch<Event::KeyTriggered>([&](Event::KeyTriggered& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::KeyPressed>([&](Event::KeyPressed& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::KeyRepeated>([&](Event::KeyRepeated& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::KeyReleased>([&](Event::KeyReleased& e) -> bool {
        //        io.AddKeyEvent(static_cast<ImGuiKey>(imguiKeyMapping(e.getKeyCode())), false);
        //        return true;
        //        });
        //}

        //void Editor::handleMouseEvents(ImGuiIO& io, Event::Event& event) {
        //    Event::Dispatcher dispatcher(event);
        //    dispatcher.Dispatch<Event::MouseBtnPressed>([&](Event::MouseBtnPressed& e) -> bool {
        //        io.AddMouseButtonEvent(e.getBtnCode(), true);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::MouseBtnReleased>([&](Event::MouseBtnReleased& e) -> bool {
        //        io.AddMouseButtonEvent(e.getBtnCode(), false);
        //        return true;
        //        });
        //    dispatcher.Dispatch<Event::MouseMoved>([&](Event::MouseMoved& e) -> bool {
        //        io.AddMousePosEvent(e.getWindowPos().x, e.getWindowPos().y);
        //        return false;
        //        });
        //    dispatcher.Dispatch<Event::MouseScrolled>([&](Event::MouseScrolled& e) -> bool {
        //        io.AddMouseWheelEvent(e.getOffset().x, e.getOffset().y);
        //        return false;
        //        });
        //}

        void Editor::BuildDockspace() {
            // ImGuiViewport* vp = ImGui::GetMainViewport();

            // // Reserve vertical space for the fixed Tools panel (menu + toolbar)
            // const float menu_h = ImGui::GetFrameHeight(); // same as Tools
            // const float toolbar_h = .2f;                   // same as Tools
            // const float tools_h = menu_h + toolbar_h;

            // // Position/size the dockspace host BELOW the tools bar
            // ImGui::SetNextWindowPos(ImVec2(vp->Pos.x, vp->Pos.y + tools_h));
            // ImGui::SetNextWindowSize(ImVec2(vp->Size.x, vp->Size.y - tools_h));
            // ImGui::SetNextWindowViewport(vp->ID);

            // ImGuiWindowFlags host_flags =
            //     ImGuiWindowFlags_NoDocking |
            //     ImGuiWindowFlags_NoTitleBar |
            //     ImGuiWindowFlags_NoCollapse |
            //     ImGuiWindowFlags_NoResize |
            //     ImGuiWindowFlags_NoMove |
            //     ImGuiWindowFlags_NoBringToFrontOnFocus |
            //     ImGuiWindowFlags_NoNavFocus |
            //     ImGuiWindowFlags_NoBackground;   // no menubar here; Tools owns the menu

            // ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            // ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

            // ImGui::Begin("##DockSpaceHost", nullptr, host_flags);

            // ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_PassthruCentralNode;
            // ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
            // ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f), dock_flags);

            // ImGui::End();
            // ImGui::PopStyleVar(2);
        }

        //void Editor::handleWindowEvents(ImGuiIO& io, Event::Event& event) {
        //    Event::Dispatcher dispatcher(event);
        //    dispatcher.Dispatch<Event::WindowResized>([&](Event::WindowResized& e) -> bool {
        //        io.DisplaySize.x = static_cast<float>(e.getFrameBuffer().x);
        //        io.DisplaySize.y = static_cast<float>(e.getFrameBuffer().y);
        //        return false;
        //        });
        //}
    }

}

#endif
